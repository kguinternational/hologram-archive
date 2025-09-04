/* traditional_fft.c - Traditional Cooley-Tukey FFT for Baseline Comparison
 * (c) 2024-2025 UOR Foundation - MIT License
 * 
 * This file implements the standard Cooley-Tukey FFT algorithm without Atlas-specific
 * optimizations to provide baseline performance comparisons against Atlas R96 Fourier
 * transform approaches.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <assert.h>
#include <stdbool.h>
#include <complex.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// =============================================================================
// Traditional FFT Data Structures
// =============================================================================

typedef struct {
    double complex* data;
    int size;
    bool is_frequency_domain;
} traditional_fft_buffer_t;

typedef struct {
    double complex* twiddle_factors;
    int* bit_reversal_table;
    int size;
    int log2_size;
} traditional_fft_plan_t;

// =============================================================================
// Utility Functions
// =============================================================================

/**
 * Calculate integer log base 2.
 */
static int traditional_log2_int(int n) {
    int log2_val = 0;
    while (n > 1) {
        n >>= 1;
        log2_val++;
    }
    return log2_val;
}

/**
 * Check if number is a power of 2.
 */
static bool traditional_is_power_of_2(int n) {
    return n > 0 && (n & (n - 1)) == 0;
}

/**
 * Reverse bits in an integer.
 */
static int traditional_reverse_bits(int value, int num_bits) {
    int result = 0;
    for (int i = 0; i < num_bits; i++) {
        result = (result << 1) | (value & 1);
        value >>= 1;
    }
    return result;
}

// =============================================================================
// FFT Buffer Management
// =============================================================================

/**
 * Create FFT buffer with specified size.
 */
traditional_fft_buffer_t* traditional_fft_buffer_create(int size) {
    if (!traditional_is_power_of_2(size)) {
        return NULL; // Only power-of-2 sizes supported
    }
    
    traditional_fft_buffer_t* buffer = malloc(sizeof(traditional_fft_buffer_t));
    if (!buffer) return NULL;
    
    buffer->data = calloc(size, sizeof(double complex));
    if (!buffer->data) {
        free(buffer);
        return NULL;
    }
    
    buffer->size = size;
    buffer->is_frequency_domain = false;
    
    return buffer;
}

/**
 * Destroy FFT buffer.
 */
void traditional_fft_buffer_destroy(traditional_fft_buffer_t* buffer) {
    if (buffer) {
        free(buffer->data);
        free(buffer);
    }
}

/**
 * Fill buffer with real signal data.
 */
void traditional_fft_buffer_fill_real(traditional_fft_buffer_t* buffer, 
                                     const double* real_data, int count) {
    assert(buffer && real_data && count <= buffer->size);
    
    for (int i = 0; i < count; i++) {
        buffer->data[i] = real_data[i] + 0.0 * I;
    }
    
    // Zero-pad if necessary
    for (int i = count; i < buffer->size; i++) {
        buffer->data[i] = 0.0 + 0.0 * I;
    }
    
    buffer->is_frequency_domain = false;
}

/**
 * Fill buffer with complex signal data.
 */
void traditional_fft_buffer_fill_complex(traditional_fft_buffer_t* buffer,
                                        const double complex* complex_data, int count) {
    assert(buffer && complex_data && count <= buffer->size);
    
    for (int i = 0; i < count; i++) {
        buffer->data[i] = complex_data[i];
    }
    
    // Zero-pad if necessary
    for (int i = count; i < buffer->size; i++) {
        buffer->data[i] = 0.0 + 0.0 * I;
    }
    
    buffer->is_frequency_domain = false;
}

/**
 * Generate test signal (sum of sinusoids).
 */
void traditional_fft_generate_test_signal(traditional_fft_buffer_t* buffer,
                                         const double* frequencies,
                                         const double* amplitudes,
                                         int num_components,
                                         double sample_rate) {
    assert(buffer && frequencies && amplitudes);
    
    for (int i = 0; i < buffer->size; i++) {
        double t = (double)i / sample_rate;
        double sample = 0.0;
        
        for (int j = 0; j < num_components; j++) {
            sample += amplitudes[j] * sin(2.0 * M_PI * frequencies[j] * t);
        }
        
        buffer->data[i] = sample + 0.0 * I;
    }
    
    buffer->is_frequency_domain = false;
}

// =============================================================================
// FFT Plan Creation and Management
// =============================================================================

/**
 * Create FFT plan for efficient repeated transforms.
 */
traditional_fft_plan_t* traditional_fft_plan_create(int size) {
    if (!traditional_is_power_of_2(size)) {
        return NULL;
    }
    
    traditional_fft_plan_t* plan = malloc(sizeof(traditional_fft_plan_t));
    if (!plan) return NULL;
    
    plan->size = size;
    plan->log2_size = traditional_log2_int(size);
    
    // Pre-compute twiddle factors
    plan->twiddle_factors = malloc(size * sizeof(double complex));
    if (!plan->twiddle_factors) {
        free(plan);
        return NULL;
    }
    
    for (int i = 0; i < size; i++) {
        double angle = -2.0 * M_PI * i / size;
        plan->twiddle_factors[i] = cos(angle) + sin(angle) * I;
    }
    
    // Pre-compute bit reversal table
    plan->bit_reversal_table = malloc(size * sizeof(int));
    if (!plan->bit_reversal_table) {
        free(plan->twiddle_factors);
        free(plan);
        return NULL;
    }
    
    for (int i = 0; i < size; i++) {
        plan->bit_reversal_table[i] = traditional_reverse_bits(i, plan->log2_size);
    }
    
    return plan;
}

/**
 * Destroy FFT plan.
 */
void traditional_fft_plan_destroy(traditional_fft_plan_t* plan) {
    if (plan) {
        free(plan->twiddle_factors);
        free(plan->bit_reversal_table);
        free(plan);
    }
}

// =============================================================================
// Cooley-Tukey FFT Implementation
// =============================================================================

/**
 * Bit-reversal permutation of input data.
 */
static void traditional_bit_reverse_permutation(double complex* data,
                                               const int* bit_reversal_table,
                                               int size) {
    for (int i = 0; i < size; i++) {
        int j = bit_reversal_table[i];
        if (i < j) {
            double complex temp = data[i];
            data[i] = data[j];
            data[j] = temp;
        }
    }
}

/**
 * In-place Cooley-Tukey FFT (Decimation-in-Time).
 */
void traditional_fft_forward_inplace(traditional_fft_buffer_t* buffer,
                                    const traditional_fft_plan_t* plan) {
    assert(buffer && plan && buffer->size == plan->size);
    
    double complex* data = buffer->data;
    int n = buffer->size;
    
    // Bit-reversal permutation
    traditional_bit_reverse_permutation(data, plan->bit_reversal_table, n);
    
    // Cooley-Tukey FFT algorithm
    for (int stage = 1; stage <= plan->log2_size; stage++) {
        int m = 1 << stage;           // 2^stage
        int m_half = m >> 1;          // m/2
        int twiddle_step = n / m;     // Step size for twiddle factors
        
        for (int i = 0; i < n; i += m) {
            for (int j = 0; j < m_half; j++) {
                double complex twiddle = plan->twiddle_factors[j * twiddle_step];
                double complex even = data[i + j];
                double complex odd = data[i + j + m_half];
                
                // Butterfly computation
                double complex butterfly_product = twiddle * odd;
                data[i + j] = even + butterfly_product;
                data[i + j + m_half] = even - butterfly_product;
            }
        }
    }
    
    buffer->is_frequency_domain = true;
}

/**
 * In-place Inverse FFT.
 */
void traditional_fft_inverse_inplace(traditional_fft_buffer_t* buffer,
                                    const traditional_fft_plan_t* plan) {
    assert(buffer && plan && buffer->size == plan->size);
    
    double complex* data = buffer->data;
    int n = buffer->size;
    
    // Conjugate the input
    for (int i = 0; i < n; i++) {
        data[i] = conj(data[i]);
    }
    
    // Forward FFT
    traditional_fft_forward_inplace(buffer, plan);
    
    // Conjugate and scale the output
    for (int i = 0; i < n; i++) {
        data[i] = conj(data[i]) / n;
    }
    
    buffer->is_frequency_domain = false;
}

/**
 * Out-of-place forward FFT.
 */
traditional_fft_buffer_t* traditional_fft_forward(const traditional_fft_buffer_t* input,
                                                 const traditional_fft_plan_t* plan) {
    assert(input && plan && input->size == plan->size);
    
    traditional_fft_buffer_t* output = traditional_fft_buffer_create(input->size);
    if (!output) return NULL;
    
    // Copy input to output
    memcpy(output->data, input->data, input->size * sizeof(double complex));
    
    // Perform in-place FFT
    traditional_fft_forward_inplace(output, plan);
    
    return output;
}

/**
 * Out-of-place inverse FFT.
 */
traditional_fft_buffer_t* traditional_fft_inverse(const traditional_fft_buffer_t* input,
                                                 const traditional_fft_plan_t* plan) {
    assert(input && plan && input->size == plan->size);
    
    traditional_fft_buffer_t* output = traditional_fft_buffer_create(input->size);
    if (!output) return NULL;
    
    // Copy input to output
    memcpy(output->data, input->data, input->size * sizeof(double complex));
    
    // Perform in-place inverse FFT
    traditional_fft_inverse_inplace(output, plan);
    
    return output;
}

// =============================================================================
// Convolution and Correlation using FFT
// =============================================================================

/**
 * Compute circular convolution using FFT.
 */
traditional_fft_buffer_t* traditional_fft_convolution(const traditional_fft_buffer_t* signal1,
                                                     const traditional_fft_buffer_t* signal2,
                                                     const traditional_fft_plan_t* plan) {
    assert(signal1 && signal2 && plan);
    assert(signal1->size == signal2->size && signal1->size == plan->size);
    
    // Transform both signals to frequency domain
    traditional_fft_buffer_t* freq1 = traditional_fft_forward(signal1, plan);
    traditional_fft_buffer_t* freq2 = traditional_fft_forward(signal2, plan);
    
    if (!freq1 || !freq2) {
        traditional_fft_buffer_destroy(freq1);
        traditional_fft_buffer_destroy(freq2);
        return NULL;
    }
    
    // Multiply in frequency domain (pointwise)
    for (int i = 0; i < plan->size; i++) {
        freq1->data[i] *= freq2->data[i];
    }
    
    // Transform back to time domain
    traditional_fft_buffer_t* result = traditional_fft_inverse(freq1, plan);
    
    traditional_fft_buffer_destroy(freq1);
    traditional_fft_buffer_destroy(freq2);
    
    return result;
}

/**
 * Compute cross-correlation using FFT.
 */
traditional_fft_buffer_t* traditional_fft_correlation(const traditional_fft_buffer_t* signal1,
                                                     const traditional_fft_buffer_t* signal2,
                                                     const traditional_fft_plan_t* plan) {
    assert(signal1 && signal2 && plan);
    assert(signal1->size == signal2->size && signal1->size == plan->size);
    
    // Transform both signals to frequency domain
    traditional_fft_buffer_t* freq1 = traditional_fft_forward(signal1, plan);
    traditional_fft_buffer_t* freq2 = traditional_fft_forward(signal2, plan);
    
    if (!freq1 || !freq2) {
        traditional_fft_buffer_destroy(freq1);
        traditional_fft_buffer_destroy(freq2);
        return NULL;
    }
    
    // Multiply first signal by conjugate of second (correlation in frequency domain)
    for (int i = 0; i < plan->size; i++) {
        freq1->data[i] *= conj(freq2->data[i]);
    }
    
    // Transform back to time domain
    traditional_fft_buffer_t* result = traditional_fft_inverse(freq1, plan);
    
    traditional_fft_buffer_destroy(freq1);
    traditional_fft_buffer_destroy(freq2);
    
    return result;
}

// =============================================================================
// Spectral Analysis Functions
// =============================================================================

/**
 * Calculate power spectral density.
 */
double* traditional_fft_power_spectrum(const traditional_fft_buffer_t* frequency_buffer) {
    assert(frequency_buffer && frequency_buffer->is_frequency_domain);
    
    double* power = malloc(frequency_buffer->size * sizeof(double));
    if (!power) return NULL;
    
    for (int i = 0; i < frequency_buffer->size; i++) {
        double complex c = frequency_buffer->data[i];
        power[i] = creal(c) * creal(c) + cimag(c) * cimag(c);
    }
    
    return power;
}

/**
 * Find peak frequency in spectrum.
 */
int traditional_fft_find_peak_frequency(const traditional_fft_buffer_t* frequency_buffer) {
    assert(frequency_buffer && frequency_buffer->is_frequency_domain);
    
    int peak_index = 0;
    double max_magnitude = 0.0;
    
    // Only check first half (positive frequencies)
    for (int i = 0; i < frequency_buffer->size / 2; i++) {
        double complex c = frequency_buffer->data[i];
        double magnitude = sqrt(creal(c) * creal(c) + cimag(c) * cimag(c));
        
        if (magnitude > max_magnitude) {
            max_magnitude = magnitude;
            peak_index = i;
        }
    }
    
    return peak_index;
}

/**
 * Calculate frequency resolution.
 */
double traditional_fft_frequency_resolution(int buffer_size, double sample_rate) {
    return sample_rate / buffer_size;
}

/**
 * Convert frequency bin to actual frequency.
 */
double traditional_fft_bin_to_frequency(int bin, int buffer_size, double sample_rate) {
    return (double)bin * sample_rate / buffer_size;
}

// =============================================================================
// Windowing Functions
// =============================================================================

/**
 * Apply Hann (Hanning) window to signal.
 */
void traditional_fft_apply_hann_window(traditional_fft_buffer_t* buffer) {
    assert(buffer);
    
    int n = buffer->size;
    for (int i = 0; i < n; i++) {
        double window_value = 0.5 * (1.0 - cos(2.0 * M_PI * i / (n - 1)));
        buffer->data[i] *= window_value;
    }
}

/**
 * Apply Hamming window to signal.
 */
void traditional_fft_apply_hamming_window(traditional_fft_buffer_t* buffer) {
    assert(buffer);
    
    int n = buffer->size;
    for (int i = 0; i < n; i++) {
        double window_value = 0.54 - 0.46 * cos(2.0 * M_PI * i / (n - 1));
        buffer->data[i] *= window_value;
    }
}

/**
 * Apply Blackman window to signal.
 */
void traditional_fft_apply_blackman_window(traditional_fft_buffer_t* buffer) {
    assert(buffer);
    
    int n = buffer->size;
    for (int i = 0; i < n; i++) {
        double window_value = 0.42 - 0.5 * cos(2.0 * M_PI * i / (n - 1)) + 
                             0.08 * cos(4.0 * M_PI * i / (n - 1));
        buffer->data[i] *= window_value;
    }
}

// =============================================================================
// Benchmark Functions
// =============================================================================

/**
 * Benchmark FFT performance for different sizes.
 */
double traditional_fft_benchmark_transform(int size, int iterations) {
    traditional_fft_plan_t* plan = traditional_fft_plan_create(size);
    traditional_fft_buffer_t* buffer = traditional_fft_buffer_create(size);
    
    if (!plan || !buffer) {
        traditional_fft_plan_destroy(plan);
        traditional_fft_buffer_destroy(buffer);
        return -1.0;
    }
    
    // Fill with random test data
    for (int i = 0; i < size; i++) {
        double real_part = ((double)rand() / RAND_MAX) * 2.0 - 1.0;
        double imag_part = ((double)rand() / RAND_MAX) * 2.0 - 1.0;
        buffer->data[i] = real_part + imag_part * I;
    }
    
    clock_t start = clock();
    
    for (int iter = 0; iter < iterations; iter++) {
        traditional_fft_forward_inplace(buffer, plan);
        traditional_fft_inverse_inplace(buffer, plan);
    }
    
    clock_t end = clock();
    
    traditional_fft_plan_destroy(plan);
    traditional_fft_buffer_destroy(buffer);
    
    return ((double)(end - start)) / CLOCKS_PER_SEC / iterations;
}

/**
 * Benchmark convolution performance.
 */
double traditional_fft_benchmark_convolution(int size, int iterations) {
    traditional_fft_plan_t* plan = traditional_fft_plan_create(size);
    traditional_fft_buffer_t* signal1 = traditional_fft_buffer_create(size);
    traditional_fft_buffer_t* signal2 = traditional_fft_buffer_create(size);
    
    if (!plan || !signal1 || !signal2) {
        traditional_fft_plan_destroy(plan);
        traditional_fft_buffer_destroy(signal1);
        traditional_fft_buffer_destroy(signal2);
        return -1.0;
    }
    
    // Fill with test data
    for (int i = 0; i < size; i++) {
        signal1->data[i] = sin(2.0 * M_PI * i / size) + 0.0 * I;
        signal2->data[i] = cos(2.0 * M_PI * i / size) + 0.0 * I;
    }
    
    clock_t start = clock();
    
    for (int iter = 0; iter < iterations; iter++) {
        traditional_fft_buffer_t* result = traditional_fft_convolution(signal1, signal2, plan);
        traditional_fft_buffer_destroy(result);
    }
    
    clock_t end = clock();
    
    traditional_fft_plan_destroy(plan);
    traditional_fft_buffer_destroy(signal1);
    traditional_fft_buffer_destroy(signal2);
    
    return ((double)(end - start)) / CLOCKS_PER_SEC / iterations;
}

// =============================================================================
// Main Function for Testing
// =============================================================================

#ifdef TRADITIONAL_FFT_MAIN
int main(void) {
    printf("Traditional Cooley-Tukey FFT Benchmark\n");
    printf("=====================================\n\n");
    
    srand((unsigned int)time(NULL));
    
    // Test basic FFT functionality
    printf("Testing basic FFT operations...\n");
    
    int test_size = 64;
    traditional_fft_plan_t* plan = traditional_fft_plan_create(test_size);
    traditional_fft_buffer_t* buffer = traditional_fft_buffer_create(test_size);
    
    if (!plan || !buffer) {
        printf("Failed to create FFT plan or buffer\n");
        return 1;
    }
    
    // Generate test signal: sum of sinusoids
    double frequencies[] = {5.0, 12.0, 23.0};
    double amplitudes[] = {1.0, 0.5, 0.3};
    traditional_fft_generate_test_signal(buffer, frequencies, amplitudes, 3, 64.0);
    
    printf("Generated test signal with frequencies: %.1f, %.1f, %.1f Hz\n", 
           frequencies[0], frequencies[1], frequencies[2]);
    
    // Forward FFT
    traditional_fft_forward_inplace(buffer, plan);
    printf("Forward FFT completed\n");
    
    // Find peak frequency
    int peak_bin = traditional_fft_find_peak_frequency(buffer);
    double peak_freq = traditional_fft_bin_to_frequency(peak_bin, test_size, 64.0);
    printf("Peak frequency detected: %.1f Hz (bin %d)\n", peak_freq, peak_bin);
    
    // Inverse FFT
    traditional_fft_inverse_inplace(buffer, plan);
    printf("Inverse FFT completed\n");
    
    // Test windowing
    traditional_fft_buffer_t* windowed = traditional_fft_buffer_create(test_size);
    if (windowed) {
        traditional_fft_generate_test_signal(windowed, frequencies, amplitudes, 3, 64.0);
        traditional_fft_apply_hann_window(windowed);
        printf("Hann window applied\n");
        traditional_fft_buffer_destroy(windowed);
    }
    
    traditional_fft_plan_destroy(plan);
    traditional_fft_buffer_destroy(buffer);
    
    printf("\nPerformance Benchmarks:\n");
    
    // Benchmark different FFT sizes
    int sizes[] = {64, 256, 1024, 4096};
    int num_sizes = sizeof(sizes) / sizeof(sizes[0]);
    
    for (int i = 0; i < num_sizes; i++) {
        int size = sizes[i];
        int iterations = (size <= 1024) ? 1000 : 100;
        
        printf("FFT size %d:\n", size);
        
        double fft_time = traditional_fft_benchmark_transform(size, iterations);
        if (fft_time > 0) {
            printf("  Transform (forward+inverse): %.6f seconds per pair\n", fft_time);
            printf("  Throughput: %.1f transforms per second\n", 2.0 / fft_time);
        }
        
        double conv_time = traditional_fft_benchmark_convolution(size, iterations / 10);
        if (conv_time > 0) {
            printf("  Convolution: %.6f seconds per operation\n", conv_time);
        }
        
        printf("\n");
    }
    
    // Algorithm complexity analysis
    printf("Algorithm Complexity Analysis:\n");
    printf("Traditional FFT: O(N log N)\n");
    printf("Theoretical operations for N=1024: %d multiplies\n", 1024 * traditional_log2_int(1024));
    printf("Theoretical operations for N=4096: %d multiplies\n", 4096 * traditional_log2_int(4096));
    
    return 0;
}
#endif