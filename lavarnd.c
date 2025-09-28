#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>      // For struct timeval
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/select.h>    // For select
#include <linux/videodev2.h>
#include <unistd.h>
#include <openssl/sha.h>   // For SHA1; install libssl-dev if needed
#include <getopt.h>        // For command-line parsing
#include <math.h>          // For sqrt and ceil
#include <stdint.h>        // For uint32_t
#include <signal.h>        // For signal handling

#define DEFAULT_DEVICE "/dev/video0"  // Default video device
#define WIDTH 640                     // Low res for faster capture/noise focus
#define HEIGHT 480
#define MAX_BUFFERS 8                 // Maximum number of buffers for array sizing
#define DEFAULT_BUFFERS 8             // Default number of buffers
#define RANDOM_LEN_DEFAULT 64         // Default random output length
#define DEFAULT_OUTPUT_TYPE "hex"     // Default output type
#define SHA_DIGESTSIZE 20
#define DEFAULT_NWAY 5                // Default nway for blender if not calculated
#define SELECT_TIMEOUT_SEC 0.5        // Timeout for select in seconds

// LavaRnd macros from lavarnd.c
#define LAVA_DIVUP(x, y) (((x) + (y) - 1) / (y))
#define LAVA_DIVDOWN(x, y) ((x) / (y))
#define LAVA_ROUNDUP(x, y) (LAVA_DIVUP((x), (y)) * (y))
#define LAVA_ROUNDDOWN(x, y) (LAVA_DIVDOWN((x), (y)) * (y))
#define LAVA_TURN_SUBDIFF(len, nway) LAVA_DIVUP((len), (nway))
#define LAVA_BLK_TURN_SUBDIFF(len, nway) LAVA_ROUNDUP(LAVA_TURN_SUBDIFF((len), (nway)), SHA_DIGESTSIZE)
#define LAVA_BLK_TURN_LEN(len, nway) (LAVA_BLK_TURN_SUBDIFF((len), (nway)) * (nway))
#define LAVA_EFFECTIVE_LEN(salt_len, len, nway) (LAVA_ROUNDUP((salt_len), (nway)) + (len))
#define LAVA_SALT_BLK_TURN_SUBDIFF(salt_len, len, nway) LAVA_BLK_TURN_SUBDIFF(LAVA_EFFECTIVE_LEN((salt_len), (len), (nway)), (nway))
#define LAVA_SALT_BLK_TURN_LEN(salt_len, len, nway) (LAVA_SALT_BLK_TURN_SUBDIFF((salt_len), (len), (nway)) * (nway))
#define LAVA_SALT_BLK_INDXSTEP(salt_len, len, nway) ({ \
    size_t eff = LAVA_EFFECTIVE_LEN((salt_len), (len), (nway)); \
    size_t mod = eff % (nway); \
    (mod == 0 ? (nway) : mod); \
})
#define LAVA_SALT_BLK_TURN_SUBLEN(salt_len, len, nway, indx) ({ \
    size_t eff = LAVA_EFFECTIVE_LEN((salt_len), (len), (nway)); \
    size_t fl = eff / (nway); \
    size_t step = LAVA_SALT_BLK_INDXSTEP((salt_len), (len), (nway)); \
    ((indx) < step ? fl + 1 : fl); \
})

// Simplified LavaRnd-inspired structures/macros (from lavarnd.c)
typedef unsigned int u_int32_t;  // For compatibility with old code
typedef unsigned char u_int8_t;

static u_int32_t *turned = NULL;
static size_t turned_maxlen = 0;
static int running = 1;  // Flag to control continuous loop

// Signal handler for SIGINT (Ctrl+C)
static void signal_handler(int sig) {
    running = 0;
}

// XOR-fold-rotate function from lavarnd.c (core LavaRnd algo)
static void lava_xor_fold_rot(u_int32_t *buf, size_t words, u_int32_t *fold) {
    size_t i;
    fold[0] = fold[1] = fold[2] = fold[3] = fold[4] = 0;
    for (i = 0; i < words; ++i) {
        fold[0] ^= buf[i];
        fold[1] ^= (buf[i] >> 8) | (buf[i] << 24);
        fold[2] ^= (buf[i] >> 16) | (buf[i] << 16);
        fold[3] ^= (buf[i] >> 24) | (buf[i] << 8);
        fold[4] ^= buf[i] ^ (buf[i] << 13) ^ (buf[i] >> 19);
    }
}

// SHA1 wrapper
static void lava_sha1_buf(const void *buf, size_t len, void *digest) {
    SHA_CTX ctx;
    SHA1_Init(&ctx);
    SHA1_Update(&ctx, buf, len);
    SHA1_Final(digest, &ctx);
}

// lava_blk_turn from lavarnd.c
static void *lava_blk_turn(void *input, size_t len, int nway, void *turned) {
    size_t subdiff = LAVA_BLK_TURN_SUBDIFF(len, nway);
    size_t turned_len = LAVA_BLK_TURN_LEN(len, nway);
    size_t i;
    u_int8_t *in = (u_int8_t *)input;
    u_int8_t *out = (u_int8_t *)turned;

    if (input == NULL || turned == NULL || nway < 1 || len == 0) {
        return NULL;
    }

    // Perform the nway turn
    for (i = 0; i < len; ++i) {
        size_t idx = (i % nway) * subdiff + (i / nway);
        if (idx >= turned_len) {
            fprintf(stderr, "Error: lava_blk_turn index %zu out of bounds (max %zu)\n", idx, turned_len);
            return NULL;
        }
        out[idx] = in[i];
    }

    // NUL pad the sub-buffers as needed
    for (i = len; i < turned_len; ++i) {
        if (i >= turned_len) {
            fprintf(stderr, "Error: lava_blk_turn padding index %zu out of bounds (max %zu)\n", i, turned_len);
            return NULL;
        }
        out[i] = 0;
    }

    return turned;
}

// lava_salt_blk_turn from lavarnd.c
static void *lava_salt_blk_turn(void *salt, size_t salt_len, void *input, size_t len, int nway, void *turned) {
    size_t salt_round = LAVA_ROUNDUP(salt_len, nway);
    size_t subdiff = LAVA_SALT_BLK_TURN_SUBDIFF(salt_len, len, nway);
    size_t turned_len = LAVA_SALT_BLK_TURN_LEN(salt_len, len, nway);
    size_t i;
    u_int8_t *sal = (u_int8_t *)salt;
    u_int8_t *in = (u_int8_t *)input;
    u_int8_t *out = (u_int8_t *)turned;

    if (salt == NULL || input == NULL || turned == NULL || nway < 1 || len == 0) {
        return NULL;
    }

    // Perform the nway turn on salt
    for (i = 0; i < salt_len; ++i) {
        size_t idx = (i % nway) * subdiff + (i / nway);
        if (idx >= turned_len) {
            fprintf(stderr, "Error: lava_salt_blk_turn salt index %zu out of bounds (max %zu)\n", idx, turned_len);
            return NULL;
        }
        out[idx] = sal[i];
    }

    // NUL pad the salt as needed
    for (i = salt_len; i < salt_round; ++i) {
        size_t idx = (i % nway) * subdiff + (i / nway);
        if (idx >= turned_len) {
            fprintf(stderr, "Error: lava_salt_blk_turn salt padding index %zu out of bounds (max %zu)\n", idx, turned_len);
            return NULL;
        }
        out[idx] = 0;
    }

    // Perform the nway turn on input
    for (i = 0; i < len; ++i) {
        size_t idx = ((i + salt_round) % nway) * subdiff + ((i + salt_round) / nway);
        if (idx >= turned_len) {
            fprintf(stderr, "Error: lava_salt_blk_turn input index %zu out of bounds (max %zu)\n", idx, turned_len);
            return NULL;
        }
        out[idx] = in[i];
    }

    // NUL pad the sub-buffers as needed
    for (i = salt_round + len; i < turned_len; ++i) {
        if (i >= turned_len) {
            fprintf(stderr, "Error: lava_salt_blk_turn final padding index %zu out of bounds (max %zu)\n", i, turned_len);
            return NULL;
        }
        out[i] = 0;
    }

    return turned;
}

// Full LavaRnd algorithm from lavarnd.c
static size_t lavarnd(void *input_arg, size_t inlen, void *salt, size_t salt_len, int nway, void *output_arg, size_t outlen, int stats) {
    u_int32_t hash[SHA_DIGESTSIZE / sizeof(u_int32_t)];  // SHA1 digest as words
    u_int32_t fold[5];  // Fold array
    u_int32_t *output = (u_int32_t *)output_arg;  // Output as words
    size_t turned_len;  // Length of turned buffer
    u_int32_t *p;  // Turned buffer pointer
    size_t turnedwords;  // Turned length in words
    size_t subwords;  // Sub-buffer length in words
    size_t sublen0;  // Length of longer sub-buffers
    size_t sublen1;  // Length of shorter sub-buffers
    size_t indxjump;  // Jump index in words
    size_t i, j = 0;  // Loop counters

    // Firewall
    if (input_arg == NULL || output_arg == NULL || nway < 1 || inlen == 0 || outlen == 0) {
        fprintf(stderr, "Error: Invalid arguments to lavarnd\n");
        return 0;  // Error
    }
    if (nway * SHA_DIGESTSIZE > outlen) {
        fprintf(stderr, "Error: nway * SHA_DIGESTSIZE (%d * %d = %d) exceeds outlen (%zu)\n", 
                nway, SHA_DIGESTSIZE, nway * SHA_DIGESTSIZE, outlen);
        return 0;  // Impossible
    }

    // Allocate or reallocate turned buffer
    turned_len = salt_len > 0 ? LAVA_SALT_BLK_TURN_LEN(salt_len, inlen, nway) : LAVA_BLK_TURN_LEN(inlen, nway);
    if (stats) {
        fprintf(stderr, "Debug: Allocating turned buffer of %zu bytes (nway=%d, inlen=%zu, salt_len=%zu)\n", 
                turned_len, nway, inlen, salt_len);
    }
    if (turned_maxlen < turned_len) {
        turned = realloc(turned, turned_len);
        if (!turned) {
            fprintf(stderr, "Error: Failed to allocate turned buffer\n");
            return 0;
        }
        turned_maxlen = turned_len;
    }

    // Perform turn
    if (salt_len > 0) {
        if (stats) {
            fprintf(stderr, "Debug: Using lava_salt_blk_turn\n");
        }
        p = lava_salt_blk_turn(salt, salt_len, input_arg, inlen, nway, turned);
    } else {
        if (stats) {
            fprintf(stderr, "Debug: Using lava_blk_turn\n");
        }
        p = lava_blk_turn(input_arg, inlen, nway, turned);
    }
    if (p == NULL) {
        fprintf(stderr, "Error: Turn operation failed\n");
        return 0;
    }

    // Setup for LavaRnd loop
    turnedwords = turned_len / sizeof(u_int32_t);
    subwords = (salt_len > 0 ? LAVA_SALT_BLK_TURN_SUBDIFF(salt_len, inlen, nway) : LAVA_BLK_TURN_SUBDIFF(inlen, nway)) / sizeof(u_int32_t);
    sublen0 = salt_len > 0 ? LAVA_SALT_BLK_TURN_SUBLEN(salt_len, inlen, nway, 0) : LAVA_BLK_TURN_SUBDIFF(inlen, nway);
    indxjump = (salt_len > 0 ? LAVA_SALT_BLK_INDXSTEP(salt_len, inlen, nway) : nway) * subwords;
    sublen1 = salt_len > 0 ? LAVA_SALT_BLK_TURN_SUBLEN(salt_len, inlen, nway, nway - 1) : LAVA_BLK_TURN_SUBDIFF(inlen, nway);

    if (stats) {
        fprintf(stderr, "Debug: turned_len=%zu, turnedwords=%zu, subwords=%zu, sublen0=%zu, sublen1=%zu, indxjump=%zu\n",
                turned_len, turnedwords, subwords, sublen0, sublen1, indxjump);
    }

    // Initial fold of last sub-buffer
    if (turnedwords < subwords) {
        fprintf(stderr, "Error: turnedwords (%zu) less than subwords (%zu)\n", turnedwords, subwords);
        return 0;
    }
    lava_xor_fold_rot(turned + turnedwords - subwords, subwords, fold);

    // Process longer sub-buffers
    for (i = 0; i < indxjump && i < turnedwords && j < outlen / sizeof(u_int32_t); i += subwords) {
        if (i + subwords > turnedwords) {
            fprintf(stderr, "Error: Index %zu + subwords %zu exceeds turnedwords %zu\n", i, subwords, turnedwords);
            return 0;
        }
        lava_sha1_buf((void *)(turned + i), sublen0, hash);
        output[j++] = fold[0] ^ hash[0];
        output[j++] = fold[1] ^ hash[1];
        output[j++] = fold[2] ^ hash[2];
        output[j++] = fold[3] ^ hash[3];
        output[j++] = fold[4] ^ hash[4];
        lava_xor_fold_rot(turned + i, subwords, fold);
    }

    // Process shorter sub-buffers up to but not including last
    for (; i < turnedwords - subwords && j < outlen / sizeof(u_int32_t); i += subwords) {
        if (i + subwords > turnedwords) {
            fprintf(stderr, "Error: Index %zu + subwords %zu exceeds turnedwords %zu\n", i, subwords, turnedwords);
            return 0;
        }
        lava_sha1_buf((void *)(turned + i), sublen1, hash);
        output[j++] = fold[0] ^ hash[0];
        output[j++] = fold[1] ^ hash[1];
        output[j++] = fold[2] ^ hash[2];
        output[j++] = fold[3] ^ hash[3];
        output[j++] = fold[4] ^ hash[4];
        lava_xor_fold_rot(turned + i, subwords, fold);
    }

    // Process last sub-buffer
    if (i < turnedwords && j < outlen / sizeof(u_int32_t)) {
        lava_sha1_buf((void *)(turned + i), sublen1, hash);
        output[j++] = fold[0] ^ hash[0];
        output[j++] = fold[1] ^ hash[1];
        output[j++] = fold[2] ^ hash[2];
        output[j++] = fold[3] ^ hash[3];
        output[j++] = fold[4] ^ hash[4];
    }

    // Return output length in bytes
    return j * sizeof(u_int32_t);
}

// Simple base64 encoder
static char *base64_encode(const u_int8_t *data, size_t len) {
    static const char *b64chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    char *out = malloc(((len + 2) / 3) * 4 + 1);
    if (!out) return NULL;
    size_t i, j = 0;
    for (i = 0; i < len; ) {
        uint32_t val = 0;
        int pad = 0;
        val = data[i++] << 16;
        if (i < len) { val |= data[i++] << 8; } else pad++;
        if (i < len) { val |= data[i++]; } else pad++;
        out[j++] = b64chars[(val >> 18) & 63];
        out[j++] = b64chars[(val >> 12) & 63];
        out[j++] = (pad >= 2) ? '=' : b64chars[(val >> 6) & 63];
        out[j++] = (pad >= 1) ? '=' : b64chars[val & 63];
    }
    out[j] = '\0';
    return out;
}

// Structure for channel stats
typedef struct {
    double mean;
    double stddev;
    u_int8_t min;
    u_int8_t max;
} ChannelStats;

// Compute stats for a channel (Y, U, or V)
static void compute_channel_stats(u_int8_t *data, size_t len, int mod_offset, int step, ChannelStats *stats) {
    unsigned long sum = 0;
    double sum_sq = 0.0;
    size_t count = 0;
    u_int8_t min_val = 255, max_val = 0;
    size_t i;

    for (i = mod_offset; i < len; i += step) {
        u_int8_t val = data[i];
        sum += val;
        sum_sq += (double)val * val;
        if (val < min_val) min_val = val;
        if (val > max_val) max_val = val;
        count++;
    }

    if (count == 0) {
        stats->mean = 0.0;
        stats->stddev = 0.0;
        stats->min = 0;
        stats->max = 0;
        return;
    }

    stats->mean = (double)sum / count;
    stats->stddev = sqrt((sum_sq / count) - (stats->mean * stats->mean));
    stats->min = min_val;
    stats->max = max_val;
}

// Print stats for all channels to stderr
static void print_stats(const char *stage, u_int8_t *data, size_t len) {
    ChannelStats y_stats, u_stats, v_stats;

    // Y: positions 0 and 2 mod 4, step 4
    compute_channel_stats(data, len, 0, 4, &y_stats);  // Y1
    ChannelStats y2_stats;
    compute_channel_stats(data, len, 2, 4, &y2_stats); // Y2
    // Average Y1 and Y2 for simplicity (since they are similar)
    y_stats.mean = (y_stats.mean + y2_stats.mean) / 2.0;
    y_stats.stddev = (y_stats.stddev + y2_stats.stddev) / 2.0;
    y_stats.min = (y_stats.min < y2_stats.min) ? y_stats.min : y2_stats.min;
    y_stats.max = (y_stats.max > y2_stats.max) ? y_stats.max : y2_stats.max;

    compute_channel_stats(data, len, 1, 4, &u_stats);  // U: 1 mod 4
    compute_channel_stats(data, len, 3, 4, &v_stats);  // V: 3 mod 4

    fprintf(stderr, "%s Stats:\n", stage);
    fprintf(stderr, "  Y: mean=%.2f, std=%.2f, min=%u, max=%u\n", y_stats.mean, y_stats.stddev, y_stats.min, y_stats.max);
    fprintf(stderr, "  U: mean=%.2f, std=%.2f, min=%u, max=%u\n", u_stats.mean, u_stats.stddev, u_stats.min, u_stats.max);
    fprintf(stderr, "  V: mean=%.2f, std=%.2f, min=%u, max=%u\n", v_stats.mean, v_stats.stddev, v_stats.min, v_stats.max);
}

// Improved debias function: Mean subtraction + differential XOR to remove temporal correlations
static void debias_yuyv(u_int8_t *data, size_t len) {
    ChannelStats y_stats, u_stats, v_stats;

    // Compute means (similar to stats)
    compute_channel_stats(data, len, 0, 4, &y_stats);
    ChannelStats y2_stats;
    compute_channel_stats(data, len, 2, 4, &y2_stats);
    double mean_y = (y_stats.mean + y2_stats.mean) / 2.0;

    compute_channel_stats(data, len, 1, 4, &u_stats);
    compute_channel_stats(data, len, 3, 4, &v_stats);

    // Subtract means (noise extraction)
    size_t i;
    for (i = 0; i < len; i += 4) {
        if (i + 3 >= len) break;
        data[i] = (data[i] > mean_y) ? data[i] - (u_int8_t)mean_y : 0;
        data[i + 2] = (data[i + 2] > mean_y) ? data[i + 2] - (u_int8_t)mean_y : 0;
        data[i + 1] = (data[i + 1] > u_stats.mean) ? data[i + 1] - (u_int8_t)u_stats.mean : 0;
        data[i + 3] = (data[i + 3] > v_stats.mean) ? data[i + 3] - (u_int8_t)v_stats.mean : 0;
    }

    // Differential XOR to remove temporal correlations (XOR with previous value in each channel)
    for (i = 4; i < len; i += 4) {
        if (i + 3 >= len) break;
        data[i] ^= data[i - 4];     // Y1 channel
        data[i + 1] ^= data[i - 3]; // U channel
        data[i + 2] ^= data[i - 2]; // Y2 channel
        data[i + 3] ^= data[i - 1]; // V channel
    }
}

int main(int argc, char *argv[]) {
    int stats = 0;
    int frame_stats = 0;
    int continuous = 0;  // New flag for continuous mode
    const char *device = DEFAULT_DEVICE;
    const char *output_type = DEFAULT_OUTPUT_TYPE;
    size_t random_len = RANDOM_LEN_DEFAULT;
    int opt;

    // Parse command-line options
    while ((opt = getopt(argc, argv, "szcd:t:l:h")) != -1) {
        switch (opt) {
            case 's':
                stats = 1;
                break;
            case 'z':
                frame_stats = 1;
                break;
            case 'c':
                continuous = 1;
                break;
            case 'd':
                device = optarg;
                break;
            case 't':
                output_type = optarg;
                if (strcmp(output_type, "raw") && strcmp(output_type, "hex") && strcmp(output_type, "b64")) {
                    fprintf(stderr, "Invalid output type: %s. Must be 'raw', 'hex', or 'b64'.\n", output_type);
                    return 1;
                }
                break;
            case 'l':
                random_len = strtoul(optarg, NULL, 0);
                if (random_len < 1) {
                    fprintf(stderr, "Random length must be at least 1\n");
                    return 1;
                }
                break;
            case 'h':
                fprintf(stderr, "Usage: %s [options]\n", argv[0]);
                fprintf(stderr, "This program captures raw frames from a USB webcam, debiases the noise (assuming a covered camera for black frames),\n");
                fprintf(stderr, "and generates random data using a LavaRnd-inspired algorithm. The number of frames is automatically calculated\n");
                fprintf(stderr, "based on the requested output length to ensure sufficient entropy. Random output is sent to stdout; all other messages\n");
                fprintf(stderr, "(stats, errors, info) are sent to stderr.\n\n");
                fprintf(stderr, "Options:\n");
                fprintf(stderr, "  -s             Enable statistical output for pre- and post-debias pooled data (to stderr).\n");
                fprintf(stderr, "                 Displays mean, standard deviation, min, and max for Y, U, V channels in the YUYV format.\n");
                fprintf(stderr, "                 Useful for verifying noise quality and debiasing effectiveness of the pooled data.\n");
                fprintf(stderr, "  -z             Enable statistical output for each individual frame before pooling (to stderr).\n");
                fprintf(stderr, "                 Displays stats for each frame's Y, U, V channels to analyze frame-to-frame variability.\n");
                fprintf(stderr, "  -c             Continuous mode: Fetch one frame, generate max random bytes from it, output, and repeat until interrupted (Ctrl+C).\n");
                fprintf(stderr, "                 Ignores -l; max output per frame based on LavaRnd formula (~3500 bytes for 640x480).\n");
                fprintf(stderr, "  -d <dev>       Video device path (default: %s).\n", DEFAULT_DEVICE);
                fprintf(stderr, "                 Use 'v4l2-ctl --list-devices' to list available devices.\n");
                fprintf(stderr, "                 Example: -d /dev/video1 for a secondary camera.\n");
                fprintf(stderr, "  -t <type>      Output type: 'raw' (binary to stdout), 'hex' (hexadecimal), 'b64' (base64) (default: %s).\n", DEFAULT_OUTPUT_TYPE);
                fprintf(stderr, "                 'raw': No headers, just binary data—ideal for piping to files or tools.\n");
                fprintf(stderr, "                 'hex': Human-readable hex dump with line breaks every 32 bytes.\n");
                fprintf(stderr, "                 'b64': Base64-encoded string, compact for text transmission.\n");
                fprintf(stderr, "  -l <len>       Length of random output in bytes (default: %d).\n", RANDOM_LEN_DEFAULT);
                fprintf(stderr, "                 Number of frames is calculated as ceil(len^2 / (%d * frame_size)) to ensure sufficient entropy.\n", SHA_DIGESTSIZE);
                fprintf(stderr, "                 Frame size is %d bytes (%dx%d YUYV, 2 bytes/pixel).\n", WIDTH * HEIGHT * 2, WIDTH, HEIGHT);
                fprintf(stderr, "                 Example: -l 10000 may require ~33 frames.\n");
                fprintf(stderr, "  -h             Display this detailed help message (to stderr).\n\n");
                fprintf(stderr, "Examples:\n");
                fprintf(stderr, "  %s -l 128 -t hex      # Generate 128 hex bytes to stdout, auto-calculate frames\n", argv[0]);
                fprintf(stderr, "  %s -s -z -d /dev/video1  # Enable pooled and per-frame stats (to stderr), use /dev/video1\n", argv[0]);
                fprintf(stderr, "  %s -t raw -l 1024 > rand.bin  # Output 1024 raw bytes to file\n", argv[0]);
                fprintf(stderr, "  %s -c -t raw > rand.bin  # Continuous raw output until Ctrl+C\n", argv[0]);
                return 0;
            default:
                fprintf(stderr, "Usage: %s [-s] [-z] [-c] [-d device] [-t type] [-l length] [-h]\n", argv[0]);
                return 1;
        }
    }

    // Setup signal handler for continuous mode
    signal(SIGINT, signal_handler);

    // Calculate number of frames based on output length (or 1 for continuous)
    size_t single_frame_len = WIDTH * HEIGHT * 2;  // YUYV: 2 bytes/pixel
    int num_frames = 1;
    size_t max_random_len = random_len;
    if (!continuous) {
        double required_input_len = (double)random_len * random_len / SHA_DIGESTSIZE;
        num_frames = (int)ceil(required_input_len / single_frame_len);
        if (num_frames < 1) num_frames = 1;
    } else {
        // In continuous mode, ignore -l, use max per frame
        max_random_len = (size_t)sqrt(SHA_DIGESTSIZE * single_frame_len);
    }

    // Calculate nway based on output length
    int nway = (int)ceil((double)max_random_len / SHA_DIGESTSIZE);
    if (nway < 1) nway = 1;

    // Adjust nway to be 1 or 5 mod 6
    while (nway % 6 != 1 && nway % 6 != 5) {
        nway++;
    }

    int fd = open(device, O_RDWR);
    if (fd < 0) {
        if (errno == EACCES) {
            fprintf(stderr, "Permission denied: Cannot access %s. Try adding user to 'video' group or running with sudo.\n", device);
        } else {
            fprintf(stderr, "Failed to open device: %s\n", strerror(errno));
        }
        return 1;
    }

    // Query capabilities
    struct v4l2_capability cap;
    if (ioctl(fd, VIDIOC_QUERYCAP, &cap) < 0) {
        fprintf(stderr, "VIDIOC_QUERYCAP: %s\n", strerror(errno));
        close(fd);
        return 1;
    }
    if (strcmp(output_type, "raw") != 0) {
        fprintf(stderr, "Device: %s\n", cap.card);
        fprintf(stderr, "Using %d frame(s) and nway=%d for %zu output bytes\n", num_frames, nway, max_random_len);
    }

    // Set format (YUYV raw)
    struct v4l2_format fmt = {0};
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = WIDTH;
    fmt.fmt.pix.height = HEIGHT;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    fmt.fmt.pix.field = V4L2_FIELD_ANY;
    if (ioctl(fd, VIDIOC_S_FMT, &fmt) < 0) {
        fprintf(stderr, "VIDIOC_S_FMT: %s\n", strerror(errno));
        close(fd);
        return 1;
    }

    // Request buffers
    int num_buffers = DEFAULT_BUFFERS;
    struct v4l2_requestbuffers req = {0};
    req.count = num_buffers;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
    if (ioctl(fd, VIDIOC_REQBUFS, &req) < 0) {
        fprintf(stderr, "VIDIOC_REQBUFS: %s\n", strerror(errno));
        close(fd);
        return 1;
    }
    if (req.count < num_buffers) {
        fprintf(stderr, "Warning: Requested %d buffers, got %d\n", num_buffers, req.count);
        num_buffers = req.count;  // Adjust to available buffers
    }

    // Map buffers
    void *buffers[MAX_BUFFERS];
    size_t lengths[MAX_BUFFERS];
    for (int i = 0; i < num_buffers; ++i) {
        struct v4l2_buffer buf = {0};
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        if (ioctl(fd, VIDIOC_QUERYBUF, &buf) < 0) {
            fprintf(stderr, "VIDIOC_QUERYBUF: %s\n", strerror(errno));
            close(fd);
            return 1;
        }
        lengths[i] = buf.length;
        buffers[i] = mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset);
        if (buffers[i] == MAP_FAILED) {
            fprintf(stderr, "mmap: %s\n", strerror(errno));
            close(fd);
            return 1;
        }
    }

    // Enqueue all buffers
    for (int i = 0; i < num_buffers; ++i) {
        struct v4l2_buffer buf = {0};
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        if (ioctl(fd, VIDIOC_QBUF, &buf) < 0) {
            fprintf(stderr, "VIDIOC_QBUF: %s\n", strerror(errno));
            close(fd);
            return 1;
        }
    }

    // Start streaming
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(fd, VIDIOC_STREAMON, &type) < 0) {
        fprintf(stderr, "VIDIOC_STREAMON: %s\n", strerror(errno));
        goto cleanup;
    }

    // Continuous mode loop or single run
    do {
        // Allocate pooled buffer (one frame for continuous)
        size_t batch_size = continuous ? 1 : (num_frames > MAX_BUFFERS ? MAX_BUFFERS : num_frames);
        size_t pooled_len = single_frame_len * batch_size;
        u_int8_t *pooled_data = malloc(pooled_len);
        if (!pooled_data) {
            fprintf(stderr, "Failed to allocate pooled buffer\n");
            goto cleanup;
        }
        size_t pooled_offset = 0;

        // Capture frames (1 in continuous, num_frames otherwise)
        int frames_to_capture = continuous ? 1 : num_frames;
        int frames_captured = 0;
        fd_set fds;
        struct timeval timeout;

        fprintf(stderr,"Getting %d frames w/ batch size %ld", frames_to_capture, batch_size);
        while (frames_captured < batch_size && running) {
            FD_ZERO(&fds);
            FD_SET(fd, &fds);
            timeout.tv_sec = (int)SELECT_TIMEOUT_SEC;
            timeout.tv_usec = (int)((SELECT_TIMEOUT_SEC - (int)SELECT_TIMEOUT_SEC) * 1000000);

            int ret = select(fd + 1, &fds, NULL, NULL, &timeout);
            if (ret < 0) {
                if (errno == EINTR) continue;  // Retry on signal interrupt
                fprintf(stderr, "select: %s\n", strerror(errno));
                free(pooled_data);
                goto cleanup;
            } else if (ret == 0) {
                fprintf(stderr, "Error: select timeout while capturing frame %d\n", frames_captured);
                free(pooled_data);
                goto cleanup;
            }

            // Dequeue a buffer
            struct v4l2_buffer buf = {0};
            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory = V4L2_MEMORY_MMAP;
            if (ioctl(fd, VIDIOC_DQBUF, &buf) < 0) {
                fprintf(stderr, "VIDIOC_DQBUF: %s\n", strerror(errno));
                free(pooled_data);
                goto cleanup;
            }

            // Process frame
            size_t copy_len = buf.bytesused < single_frame_len ? buf.bytesused : single_frame_len;
            if (pooled_offset + copy_len > pooled_len) {
                fprintf(stderr, "Error: Pooled buffer overflow (offset=%zu, copy_len=%zu, max=%zu)\n",
                        pooled_offset, copy_len, pooled_len);
                free(pooled_data);
                goto cleanup;
            }

            if( !continuous ) fprintf(stderr,".");
            // Print per-frame stats if -z is enabled
            if (frame_stats) {
                char stage[32];
                int x = num_frames - batch_size + frames_captured;
                snprintf(stage, sizeof(stage), "Frame %d", continuous ? frames_captured : x);
                print_stats(stage, buffers[buf.index], copy_len);
            }

            memcpy(pooled_data + pooled_offset, buffers[buf.index], copy_len);
            pooled_offset += copy_len;
            frames_captured++;

            // Requeue buffer
            if (ioctl(fd, VIDIOC_QBUF, &buf) < 0) {
                fprintf(stderr, "VIDIOC_QBUF: %s\n", strerror(errno));
                free(pooled_data);
                goto cleanup;
            }
        }
        if( !continuous ) fprintf(stderr,"\n");

        if (!running) {
            free(pooled_data);
            goto cleanup;
        }

        // Adjust pooled_len to actual used
        pooled_len = pooled_offset;

        // Diagnostics: Pre-debias stats for pooled data
        if (stats) {
            print_stats("Pre-debias (pooled)", pooled_data, pooled_len);
        }

        // Debias the pooled data
        debias_yuyv(pooled_data, pooled_len);

        // Diagnostics: Post-debias stats for pooled data
        if (stats) {
            print_stats("Post-debias (pooled)", pooled_data, pooled_len);
        }

        // Adjust random_len for continuous mode (max per frame)
        size_t current_random_len = continuous ? (size_t)sqrt(SHA_DIGESTSIZE * pooled_len) : random_len;

        // Allocate random output (larger to fit nway * SHA_DIGESTSIZE)
        size_t blender_outlen = nway * SHA_DIGESTSIZE;
        u_int8_t *random_output = malloc(blender_outlen);
        if (!random_output) {
            fprintf(stderr, "Failed to allocate random output buffer\n");
            free(pooled_data);
            goto cleanup;
        }

        // Feed into full LavaRnd (no salt for simplicity)
        size_t generated_len = lavarnd(pooled_data, pooled_len, NULL, 0, nway, random_output, blender_outlen, stats);
        if (generated_len == 0) {
            fprintf(stderr, "RNG processing failed\n");
            free(pooled_data);
            free(random_output);
            goto cleanup;
        }

        // Output based on type (truncate to current_random_len if needed)
        if (!strcmp(output_type, "raw")) {
            // Binary output, no text
            fwrite(random_output, 1, current_random_len, stdout);
        } else {
            // Print header for hex and b64 to stderr
            fprintf(stderr, "Random output (%zu bytes from %d frames):\n", current_random_len, frames_to_capture);
            if (!strcmp(output_type, "hex")) {
                for (size_t i = 0; i < current_random_len; ++i) {
                    printf("%02x", random_output[i]);
                    if ((i + 1) % 32 == 0) printf("\n");
                }
                printf("\n");
            } else if (!strcmp(output_type, "b64")) {
                char *b64 = base64_encode(random_output, current_random_len);
                if (b64) {
                    printf("%s\n", b64);
                    free(b64);
                } else {
                    fprintf(stderr, "Base64 encoding failed\n");
                }
            }
        }

        free(pooled_data);
        free(random_output);

    } while (continuous && running);

cleanup:
    // Stop streaming
    ioctl(fd, VIDIOC_STREAMOFF, &type);

    // Unmap buffers
    for (int i = 0; i < num_buffers; ++i) {
        if (buffers[i] != MAP_FAILED) {
            munmap(buffers[i], lengths[i]);
        }
    }

    close(fd);
    if (turned) free(turned);  // Cleanup from lavarnd.c inspiration
    return 0;
}

