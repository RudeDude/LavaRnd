#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/videodev2.h>
#include <unistd.h>
#include <openssl/sha.h>  // For SHA1; install libssl-dev if needed

#define DEVICE "/dev/video0"  // Change if your cam is /dev/video1, etc.
#define WIDTH 320             // Low res for faster capture/noise focus
#define HEIGHT 240
#define BUFFERS 4             // Number of capture buffers
#define OUTPUT_RANDOM_BYTES 64  // Amount of random output to generate

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
static int simple_lavarnd(u_int8_t *input, size_t inlen, u_int8_t *output, size_t outlen) {
    u_int32_t hash[SHA_DIGESTSIZE / 4];
    u_int32_t fold[5];
    size_t i, j = 0;
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

    // Hash and mix
    while (j < outlen) {
        SHA1_Init(&ctx);
        SHA1_Update(&ctx, input, inlen);
        SHA1_Final((u_int8_t *)hash, &ctx);

        for (i = 0; i < 5 && j < outlen; ++i, j += 4) {
            *(u_int32_t *)(output + j) = fold[i] ^ hash[i];
        }
    }
    return 0;
}

// Debias function: Subtract mean per channel (assuming YUYV: Y U Y V)
static void debias_yuyv(u_int8_t *data, size_t len) {
    unsigned long sum_y = 0, sum_u = 0, sum_v = 0;
    size_t count_y = 0, count_uv = 0;
    size_t i;

    // Calculate means
    for (i = 0; i < len; i += 4) {  // YUYV is 4 bytes per 2 pixels
        if (i + 3 >= len) break;
        sum_y += data[i] + data[i + 2];  // Y channels
        sum_u += data[i + 1];            // U
        sum_v += data[i + 3];            // V
        count_y += 2;
        count_uv++;
    }
    u_int8_t mean_y = sum_y / count_y;
    u_int8_t mean_u = sum_u / count_uv;
    u_int8_t mean_v = sum_v / count_uv;

    // Subtract means (noise extraction)
    for (i = 0; i < len; i += 4) {
        if (i + 3 >= len) break;
        data[i] = (data[i] > mean_y) ? data[i] - mean_y : 0;
        data[i + 2] = (data[i + 2] > mean_y) ? data[i + 2] - mean_y : 0;
        data[i + 1] = (data[i + 1] > mean_u) ? data[i + 1] - mean_u : 0;
        data[i + 3] = (data[i + 3] > mean_v) ? data[i + 3] - mean_v : 0;
    }
}

int main() {
    int fd = open(DEVICE, O_RDWR);
    if (fd < 0) {
        perror("Failed to open device");
        return 1;
    }

    // Query capabilities
    struct v4l2_capability cap;
    if (ioctl(fd, VIDIOC_QUERYCAP, &cap) < 0) {
        perror("VIDIOC_QUERYCAP");
        close(fd);
        return 1;
    }
    printf("Device: %s\n", cap.card);

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
        close(fd);
        return 1;
    }

    // Capture one frame (loop here for more)
    struct v4l2_buffer buf = {0};
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    if (ioctl(fd, VIDIOC_DQBUF, &buf) < 0) {
        perror("VIDIOC_DQBUF");
        goto cleanup;
    }

    // Process: Debias noise
    u_int8_t *frame_data = (u_int8_t *)buffers[buf.index];
    size_t frame_len = buf.bytesused;
    debias_yuyv(frame_data, frame_len);

    // Feed into LavaRnd-inspired RNG
    u_int8_t random_output[OUTPUT_RANDOM_BYTES];
    if (simple_lavarnd(frame_data, frame_len, random_output, OUTPUT_RANDOM_BYTES) < 0) {
        fprintf(stderr, "RNG processing failed\n");
        goto cleanup;
    }

    // Output random bytes as hex
    printf("Random output (%d bytes):\n", OUTPUT_RANDOM_BYTES);
    for (int i = 0; i < OUTPUT_RANDOM_BYTES; ++i) {
        printf("%02x", random_output[i]);
        if ((i + 1) % 16 == 0) printf("\n");
    }
    printf("\n");

    // Requeue buffer
    ioctl(fd, VIDIOC_QBUF, &buf);

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

