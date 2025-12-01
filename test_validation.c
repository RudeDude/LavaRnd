#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>

// Copied from lavarnd.c
typedef unsigned char u_int8_t;

typedef struct {
    double mean;
    double stddev;
    u_int8_t min;
    u_int8_t max;
} ChannelStats;

#define BLACK_THRESHOLD 50

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

static int validate_black_frame(u_int8_t *data, size_t len, int verbose) {
    ChannelStats y_stats, y2_stats;

    // Check Y channel (luminance) - positions 0 and 2 mod 4
    compute_channel_stats(data, len, 0, 4, &y_stats);  // Y1
    compute_channel_stats(data, len, 2, 4, &y2_stats); // Y2
    
    // Average Y1 and Y2 means
    double mean_y = (y_stats.mean + y2_stats.mean) / 2.0;
    
    if (mean_y > BLACK_THRESHOLD) {
        if (verbose) {
            fprintf(stderr, "\n=== FRAME VALIDATION FAILED ===\n");
            fprintf(stderr, "ERROR: Camera appears to be UNCOVERED!\n");
            fprintf(stderr, "Y channel mean: %.2f (threshold: %d)\n", mean_y, BLACK_THRESHOLD);
            fprintf(stderr, "\nFor proper entropy generation, the camera lens must be covered.\n");
            fprintf(stderr, "Please cover the camera with a lens cap, tape, or your finger and try again.\n");
            fprintf(stderr, "================================\n\n");
        }
        return 0;  // Validation failed
    }
    
    if (verbose) {
        fprintf(stderr, "Frame validation PASSED: Y mean=%.2f (threshold: %d) - camera appears covered.\n", 
                mean_y, BLACK_THRESHOLD);
    }
    
    return 1;  // Validation passed
}

// Generate synthetic YUYV frame with specified Y mean
void generate_yuyv_frame(u_int8_t *buffer, size_t len, u_int8_t y_mean, u_int8_t y_noise) {
    // YUYV format: Y1 U Y2 V (repeating)
    for (size_t i = 0; i < len; i += 4) {
        // Y1 and Y2: target mean with some random variation
        buffer[i] = y_mean + (rand() % (2 * y_noise + 1)) - y_noise;
        buffer[i + 2] = y_mean + (rand() % (2 * y_noise + 1)) - y_noise;
        
        // U and V: chrominance (don't matter for validation, use middle values)
        buffer[i + 1] = 128;
        buffer[i + 3] = 128;
    }
}

int main() {
    size_t frame_size = 640 * 480 * 2;  // YUYV: 2 bytes per pixel
    u_int8_t *frame = malloc(frame_size);
    
    if (!frame) {
        fprintf(stderr, "Failed to allocate frame buffer\n");
        return 1;
    }
    
    printf("=== Testing Black Frame Validation ===\n\n");
    printf("Threshold: %d\n\n", BLACK_THRESHOLD);
    
    // Test 1: Very black frame (sensor noise level)
    printf("Test 1: Very black frame (Y mean = 15, noise = 5)\n");
    generate_yuyv_frame(frame, frame_size, 15, 5);
    int result1 = validate_black_frame(frame, frame_size, 1);
    printf("Result: %s\n\n", result1 ? "PASS" : "FAIL");
    
    // Test 2: Slightly brighter but still acceptable (Y mean = 30)
    printf("Test 2: Dark but noisy frame (Y mean = 30, noise = 10)\n");
    generate_yuyv_frame(frame, frame_size, 30, 10);
    int result2 = validate_black_frame(frame, frame_size, 1);
    printf("Result: %s\n\n", result2 ? "PASS" : "FAIL");
    
    // Test 3: Right at threshold (Y mean = 50)
    printf("Test 3: At threshold (Y mean = 50, noise = 5)\n");
    generate_yuyv_frame(frame, frame_size, 50, 5);
    int result3 = validate_black_frame(frame, frame_size, 1);
    printf("Result: %s\n\n", result3 ? "PASS (should be close)" : "FAIL (expected)");
    
    // Test 4: Just over threshold (Y mean = 55)
    printf("Test 4: Slightly over threshold (Y mean = 55, noise = 5)\n");
    generate_yuyv_frame(frame, frame_size, 55, 5);
    int result4 = validate_black_frame(frame, frame_size, 1);
    printf("Result: %s\n\n", result4 ? "PASS (unexpected)" : "FAIL (expected)");
    
    // Test 5: Clearly uncovered camera (Y mean = 100)
    printf("Test 5: Moderately bright (Y mean = 100, noise = 20) - uncovered camera\n");
    generate_yuyv_frame(frame, frame_size, 100, 20);
    int result5 = validate_black_frame(frame, frame_size, 1);
    printf("Result: %s\n\n", result5 ? "PASS (unexpected)" : "FAIL (expected)");
    
    // Test 6: Very bright (Y mean = 200)
    printf("Test 6: Very bright (Y mean = 200, noise = 30) - definitely uncovered\n");
    generate_yuyv_frame(frame, frame_size, 200, 30);
    int result6 = validate_black_frame(frame, frame_size, 1);
    printf("Result: %s\n\n", result6 ? "PASS (unexpected)" : "FAIL (expected)");
    
    // Summary
    printf("\n=== Summary ===\n");
    printf("Tests that should PASS (covered camera): %d/%d\n", result1 + result2, 2);
    printf("Tests that should FAIL (uncovered camera): %d/%d\n", 
           (result4 == 0) + (result5 == 0) + (result6 == 0), 3);
    
    int all_passed = result1 && result2 && !result4 && !result5 && !result6;
    printf("\nOverall: %s\n", all_passed ? "✓ ALL TESTS PASSED" : "✗ SOME TESTS FAILED");
    
    free(frame);
    return all_passed ? 0 : 1;
}
