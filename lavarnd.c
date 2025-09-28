#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>      // For struct timeval
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/videodev2.h>
#include <unistd.h>
#include <openssl/sha.h>   // For SHA1; install libssl-dev if needed
#include <getopt.h>        // For command-line parsing
#include <math.h>          // For sqrt in std dev
#include <stdint.h>        // For uint32_t, uint64_t

#define DEFAULT_DEVICE "/dev/video0"  // Default video device
#define WIDTH 320                     // Low res for faster capture/noise focus
#define HEIGHT 240
#define BUFFERS 4                     // Number of capture buffers
#define RANDOM_LEN_DEFAULT 64         // Default random output length
#define DEFAULT_FRAMES 1              // Default number of frames to pool
#define DEFAULT_OUTPUT_TYPE "hex"     // Default output type

// Simplified LavaRnd-inspired structures/macros (from lavarnd.c)
#define SHA_DIGESTSIZE 20
typedef unsigned int u_int32_t;  // For compatibility with old code
typedef unsigned char u_int8_t;

static u_int32_t *turned = NULL;
static size_t turned_maxlen = 0;

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

// Simplified LavaRnd process: Turn input, hash, fold (inspired by lavarnd.c)
// Added counter to avoid repetition
static int simple_lavarnd(u_int8_t *input, size_t inlen, u_int8_t *output, size_t outlen) {
    u_int32_t hash[SHA_DIGESTSIZE / 4];
    u_int32_t fold[5];
    size_t i, j = 0;
    uint64_t counter = 0;  // Counter to salt each hash iteration
    SHA_CTX ctx;

    // Basic turn: Just copy input for simplicity (extend to n-way turn if needed)
    if (turned_maxlen < inlen) {
        turned = realloc(turned, inlen);
        if (!turned) return -1;
        turned_maxlen = inlen;
    }
    memcpy(turned, input, inlen);

    // XOR-fold-rotate the whole buffer
    lava_xor_fold_rot(turned, inlen / 4, fold);

    // Hash and mix, with counter to vary each chunk
    while (j < outlen) {
        SHA1_Init(&ctx);
        SHA1_Update(&ctx, input, inlen);
        SHA1_Update(&ctx, &counter, sizeof(counter));  // Salt with counter
        counter++;
        SHA1_Final((u_int8_t *)hash, &ctx);

        for (i = 0; i < 5 && j < outlen; ++i, j += 4) {
            *(u_int32_t *)(output + j) = fold[i] ^ hash[i];
        }
    }
    return 0;
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

// Print stats for all channels
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

    printf("%s Stats:\n", stage);
    printf("  Y: mean=%.2f, std=%.2f, min=%u, max=%u\n", y_stats.mean, y_stats.stddev, y_stats.min, y_stats.max);
    printf("  U: mean=%.2f, std=%.2f, min=%u, max=%u\n", u_stats.mean, u_stats.stddev, u_stats.min, u_stats.max);
    printf("  V: mean=%.2f, std=%.2f, min=%u, max=%u\n", v_stats.mean, v_stats.stddev, v_stats.min, v_stats.max);
}

// Debias function: Subtract mean per channel (assuming YUYV: Y U Y V)
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
}

int main(int argc, char *argv[]) {
    int stats = 0;
    int num_frames = DEFAULT_FRAMES;
    const char *device = DEFAULT_DEVICE;
    const char *output_type = DEFAULT_OUTPUT_TYPE;
    size_t random_len = RANDOM_LEN_DEFAULT;
    int opt;

    // Parse command-line options
    while ((opt = getopt(argc, argv, "sf:d:t:l:")) != -1) {
        switch (opt) {
            case 's':
                stats = 1;
                break;
            case 'f':
                num_frames = atoi(optarg);
                if (num_frames < 1) {
                    fprintf(stderr, "Number of frames must be at least 1\n");
                    return 1;
                }
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
            default:
                fprintf(stderr, "Usage: %s [-s] [-f num_frames] [-d device] [-t type] [-l length]\n", argv[0]);
                fprintf(stderr, "  -s: Enable stats output\n");
                fprintf(stderr, "  -t: Output type (raw, hex, b64; default: hex)\n");
                return 1;
        }
    }

    int fd = open(device, O_RDWR);
    if (fd < 0) {
        if (errno == EACCES) {
            fprintf(stderr, "Permission denied: Cannot access %s. Try adding user to 'video' group or running with sudo.\n", device);
        } else {
            perror("Failed to open device");
        }
        return 1;
    }

    // Query capabilities
    struct v4l2_capability cap;
    if (ioctl(fd, VIDIOC_QUERYCAP, &cap) < 0) {
        perror("VIDIOC_QUERYCAP");
        close(fd);
        return 1;
    }
    if (strcmp(output_type, "raw") != 0) {
        printf("Device: %s\n", cap.card);
    }

    // Set format (YUYV raw)
    struct v4l2_format fmt = {0};
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = WIDTH;
    fmt.fmt.pix.height = HEIGHT;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    fmt.fmt.pix.field = V4L2_FIELD_ANY;
    if (ioctl(fd, VIDIOC_S_FMT, &fmt) < 0) {
        perror("VIDIOC_S_FMT");
        close(fd);
        return 1;
    }

    // Request buffers
    struct v4l2_requestbuffers req = {0};
    req.count = BUFFERS;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
    if (ioctl(fd, VIDIOC_REQBUFS, &req) < 0) {
        perror("VIDIOC_REQBUFS");
        close(fd);
        return 1;
    }

    // Map buffers
    void *buffers[BUFFERS];
    size_t lengths[BUFFERS];
    for (int i = 0; i < BUFFERS; ++i) {
        struct v4l2_buffer buf = {0};
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        if (ioctl(fd, VIDIOC_QUERYBUF, &buf) < 0) {
            perror("VIDIOC_QUERYBUF");
            close(fd);
            return 1;
        }
        lengths[i] = buf.length;
        buffers[i] = mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset);
        if (buffers[i] == MAP_FAILED) {
            perror("mmap");
            close(fd);
            return 1;
        }
    }

    // Enqueue buffers
    for (int i = 0; i < BUFFERS; ++i) {
        struct v4l2_buffer buf = {0};
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        if (ioctl(fd, VIDIOC_QBUF, &buf) < 0) {
            perror("VIDIOC_QBUF");
            close(fd);
            return 1;
        }
    }

    // Start streaming
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(fd, VIDIOC_STREAMON, &type) < 0) {
        perror("VIDIOC_STREAMON");
        goto cleanup;
    }

    // Allocate pooled buffer
    size_t single_frame_len = WIDTH * HEIGHT * 2;  // YUYV: 2 bytes/pixel
    size_t pooled_len = single_frame_len * num_frames;
    u_int8_t *pooled_data = malloc(pooled_len);
    if (!pooled_data) {
        fprintf(stderr, "Failed to allocate pooled buffer\n");
        goto cleanup;
    }
    size_t pooled_offset = 0;

    // Capture num_frames frames and pool (concatenate)
    for (int frame = 0; frame < num_frames; ++frame) {
        struct v4l2_buffer buf = {0};
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        if (ioctl(fd, VIDIOC_DQBUF, &buf) < 0) {
            perror("VIDIOC_DQBUF");
            free(pooled_data);
            goto cleanup;
        }

        // Copy frame data to pooled
        size_t copy_len = buf.bytesused < single_frame_len ? buf.bytesused : single_frame_len;
        memcpy(pooled_data + pooled_offset, buffers[buf.index], copy_len);
        pooled_offset += copy_len;

        // Requeue buffer
        if (ioctl(fd, VIDIOC_QBUF, &buf) < 0) {
            perror("VIDIOC_QBUF");
            free(pooled_data);
            goto cleanup;
        }
    }

    // Adjust pooled_len to actual used
    pooled_len = pooled_offset;

    // Check for sufficient entropy (based on LavaRnd blender approx max outlen ~ sqrt(20 * inlen))
    double max_recommended_outlen = sqrt(20.0 * pooled_len);
    if (random_len > max_recommended_outlen) {
        fprintf(stderr, "Warning: Output length %zu may exceed recommended entropy from %zu input bytes (max ~%.0f). Consider increasing -f.\n",
                random_len, pooled_len, max_recommended_outlen);
    }

    // Diagnostics: Pre-debias stats
    if (stats) {
        print_stats("Pre-debias (pooled)", pooled_data, pooled_len);
    }

    // Debias the pooled data
    debias_yuyv(pooled_data, pooled_len);

    // Diagnostics: Post-debias stats
    if (stats) {
        print_stats("Post-debias (pooled)", pooled_data, pooled_len);
    }

    // Allocate random output
    u_int8_t *random_output = malloc(random_len);
    if (!random_output) {
        fprintf(stderr, "Failed to allocate random output buffer\n");
        free(pooled_data);
        goto cleanup;
    }

    // Feed into LavaRnd-inspired RNG
    if (simple_lavarnd(pooled_data, pooled_len, random_output, random_len) < 0) {
        fprintf(stderr, "RNG processing failed\n");
        free(pooled_data);
        free(random_output);
        goto cleanup;
    }

    // Output based on type
    if (!strcmp(output_type, "raw")) {
        // Binary output, no text
        fwrite(random_output, 1, random_len, stdout);
    } else {
        // Print header for hex and b64
        printf("Random output (%zu bytes from %d frames):\n", random_len, num_frames);
        if (!strcmp(output_type, "hex")) {
            for (size_t i = 0; i < random_len; ++i) {
                printf("%02x", random_output[i]);
                if ((i + 1) % 16 == 0) printf("\n");
            }
            printf("\n");
        } else if (!strcmp(output_type, "b64")) {
            char *b64 = base64_encode(random_output, random_len);
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

cleanup:
    // Stop streaming
    ioctl(fd, VIDIOC_STREAMOFF, &type);

    // Unmap buffers
    for (int i = 0; i < BUFFERS; ++i) {
        munmap(buffers[i], lengths[i]);
    }

    close(fd);
    if (turned) free(turned);  // Cleanup from lavarnd.c inspiration
    return 0;
}

