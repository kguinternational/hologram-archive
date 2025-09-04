/* benchmark_runner.c - Atlas Layer 4 Benchmark Runner
 * (c) 2024-2025 UOR Foundation - MIT License
 * 
 * Main benchmark runner program that executes all Layer 4 benchmarks,
 * collects results, generates reports, and identifies performance regressions.
 */

#include "benchmark_framework.h"
#include "benchmark_utils.h"
#include "../../include/atlas-manifold.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <time.h>
#include <math.h>
#include <errno.h>
#include <signal.h>
#include <sys/stat.h>

// =============================================================================
// Configuration and Constants
// =============================================================================

#define MAX_BENCHMARKS 64
#define MAX_FILENAME_LENGTH 512
#define BENCHMARK_DATA_SIZE 12288  // Atlas size
#define DEFAULT_OUTPUT_DIR "benchmark_results"
#define BASELINE_FILENAME "baseline.json"

// Benchmark suite configuration
typedef struct {
    char output_dir[MAX_FILENAME_LENGTH];
    char baseline_file[MAX_FILENAME_LENGTH];
    bool generate_baseline;
    bool compare_to_baseline;
    bool verbose_output;
    bool use_colors;
    bool write_csv;
    bool write_json;
    bool write_text;
    int iterations;
    int warmup_iterations;
    double regression_threshold;
    bool abort_on_regression;
} benchmark_suite_config_t;

// Benchmark registry entry
typedef struct {
    const char* name;
    const char* description;
    benchmark_func_t function;
    benchmark_setup_func_t setup;
    benchmark_cleanup_func_t cleanup;
    void* user_data;
    bool enabled;
} benchmark_entry_t;

// Global configuration
static benchmark_suite_config_t g_config = {0};
static volatile sig_atomic_t g_interrupted = 0;

// =============================================================================
// Signal Handling
// =============================================================================

static void signal_handler(int sig) {
    (void)sig;
    g_interrupted = 1;
    printf("\nBenchmark interrupted by user. Finishing current benchmark...\n");
}

static void setup_signal_handlers(void) {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
}

// =============================================================================
// Benchmark Test Data and Context
// =============================================================================

typedef struct {
    uint8_t* test_data;
    size_t data_size;
    atlas_projection_t projection;
    temp_boundary_region_t region;
    temp_transform_params_t transform;
} benchmark_test_context_t;

static int setup_test_context(benchmark_context_t* ctx, void* user_data) {
    (void)ctx; // Suppress unused parameter warning
    benchmark_test_context_t* test_ctx = (benchmark_test_context_t*)user_data;
    if (!test_ctx) {
        return -1;
    }
    
    // Generate fresh conserved test data
    return benchmark_generate_conserved_data(test_ctx->test_data, 
                                           test_ctx->data_size, 
                                           (uint32_t)time(NULL));
}

static int cleanup_test_context(benchmark_context_t* ctx, void* user_data) {
    // Verify conservation after each iteration
    benchmark_test_context_t* test_ctx = (benchmark_test_context_t*)user_data;
    if (!test_ctx) {
        return 0;
    }
    
    return benchmark_conservation_verify(ctx, test_ctx->test_data, test_ctx->data_size);
}

// =============================================================================
// Individual Benchmark Functions
// =============================================================================

// Benchmark: Projection creation (stubbed for testing)
static int benchmark_projection_create(benchmark_context_t* ctx, void* user_data) {
    benchmark_test_context_t* test_ctx = (benchmark_test_context_t*)user_data;
    
    benchmark_start_timer(ctx);
    
    // Simulate projection creation with a simple computation
    uint32_t checksum = 0;
    for (size_t i = 0; i < test_ctx->data_size; i++) {
        checksum += test_ctx->test_data[i] * (i + 1);
    }
    
    benchmark_stop_timer(ctx);
    
    // Simulate failure condition occasionally for testing
    if (checksum == 0) {
        benchmark_mark_failed(ctx, "Simulated projection creation failure");
        return -1;
    }
    
    return 0;
}

// Benchmark: Projection transformation (stubbed for testing) 
static int benchmark_projection_transform(benchmark_context_t* ctx, void* user_data) {
    benchmark_test_context_t* test_ctx = (benchmark_test_context_t*)user_data;
    
    benchmark_start_timer(ctx);
    
    // Simulate transformation with matrix computations
    double result = 0.0;
    for (size_t i = 0; i < test_ctx->data_size; i++) {
        double val = test_ctx->test_data[i];
        result += val * test_ctx->transform.scaling_factor;
        result += val * cos(test_ctx->transform.rotation_angle);
        result += test_ctx->transform.translation_x + test_ctx->transform.translation_y;
    }
    
    benchmark_stop_timer(ctx);
    
    // Simulate failure based on result
    if (isnan(result) || isinf(result)) {
        benchmark_mark_failed(ctx, "Transformation produced invalid result");
        return -1;
    }
    
    return 0;
}

// Benchmark: Shard extraction (stubbed for testing)
static int benchmark_shard_extract(benchmark_context_t* ctx, void* user_data) {
    benchmark_test_context_t* test_ctx = (benchmark_test_context_t*)user_data;
    
    benchmark_start_timer(ctx);
    
    // Simulate shard extraction with data copying
    size_t shard_size = test_ctx->region.page_count * 256;
    uint32_t shard_checksum = 0;
    
    for (size_t i = 0; i < shard_size && i < test_ctx->data_size; i++) {
        shard_checksum += test_ctx->test_data[i];
    }
    
    benchmark_stop_timer(ctx);
    
    // Simulate success/failure based on region validity
    if (test_ctx->region.start_coord >= test_ctx->region.end_coord) {
        benchmark_mark_failed(ctx, "Invalid region for shard extraction");
        return -1;
    }
    
    return 0;
}

// Benchmark: Shard verification (stubbed for testing)
static int benchmark_shard_verify(benchmark_context_t* ctx, void* user_data) {
    benchmark_test_context_t* test_ctx = (benchmark_test_context_t*)user_data;
    
    benchmark_start_timer(ctx);
    
    // Simulate shard verification with checksum validation
    bool verified = benchmark_verify_conservation(test_ctx->test_data, test_ctx->data_size);
    
    benchmark_stop_timer(ctx);
    
    if (!verified) {
        benchmark_mark_failed(ctx, "Shard verification failed");
        return -1;
    }
    
    return 0;
}

// Benchmark: Conservation checking
static int benchmark_conservation_check(benchmark_context_t* ctx, void* user_data) {
    benchmark_test_context_t* test_ctx = (benchmark_test_context_t*)user_data;
    
    benchmark_start_timer(ctx);
    bool conserved = benchmark_verify_conservation(test_ctx->test_data, test_ctx->data_size);
    benchmark_stop_timer(ctx);
    
    if (!conserved) {
        benchmark_mark_failed(ctx, "Conservation check failed");
        return -1;
    }
    
    return 0;
}

// Benchmark: Manifold verification (stubbed for testing)
static int benchmark_manifold_verify(benchmark_context_t* ctx, void* user_data) {
    benchmark_test_context_t* test_ctx = (benchmark_test_context_t*)user_data;
    
    benchmark_start_timer(ctx);
    
    // Simulate manifold verification with complex calculation
    double manifold_metric = 0.0;
    for (size_t i = 0; i < test_ctx->data_size; i++) {
        manifold_metric += sqrt((double)test_ctx->test_data[i]) * (i + 1);
    }
    
    benchmark_stop_timer(ctx);
    
    // Simulate failure for invalid metrics
    if (isnan(manifold_metric) || manifold_metric < 0.0) {
        benchmark_mark_failed(ctx, "Manifold verification failed");
        return -1;
    }
    
    return 0;
}

// Benchmark: System test (stubbed for testing)
static int benchmark_system_test(benchmark_context_t* ctx, void* user_data) {
    (void)user_data;
    
    benchmark_start_timer(ctx);
    
    // Simulate comprehensive system test
    bool system_healthy = true;
    
    // Simulate various system checks
    for (int i = 0; i < 10; i++) {
        // Simulate some computation time
        volatile double result = 0.0;
        for (int j = 0; j < 1000; j++) {
            result += sin(j * 0.001) * cos(i * 0.01);
        }
        
        // Simulate occasional test failure
        if (result > 1000.0 || result < -1000.0) {
            system_healthy = false;
            break;
        }
    }
    
    benchmark_stop_timer(ctx);
    
    if (!system_healthy) {
        benchmark_mark_failed(ctx, "System test failed");
        return -1;
    }
    
    return 0;
}

// =============================================================================
// Benchmark Registry
// =============================================================================

static benchmark_entry_t g_benchmarks[] = {
    {
        "projection_create",
        "Atlas projection creation performance",
        benchmark_projection_create,
        setup_test_context,
        cleanup_test_context,
        NULL,
        true
    },
    {
        "projection_transform", 
        "Atlas projection transformation performance",
        benchmark_projection_transform,
        setup_test_context,
        cleanup_test_context,
        NULL,
        true
    },
    {
        "shard_extract",
        "Atlas shard extraction performance",
        benchmark_shard_extract,
        setup_test_context,
        cleanup_test_context,
        NULL,
        true
    },
    {
        "shard_verify",
        "Atlas shard verification performance",
        benchmark_shard_verify,
        setup_test_context,
        cleanup_test_context,
        NULL,
        true
    },
    {
        "conservation_check",
        "Conservation law verification performance",
        benchmark_conservation_check,
        setup_test_context,
        cleanup_test_context,
        NULL,
        true
    },
    {
        "manifold_verify",
        "Manifold verification performance",
        benchmark_manifold_verify,
        setup_test_context,
        cleanup_test_context,
        NULL,
        true
    },
    {
        "system_test",
        "Complete system test performance",
        benchmark_system_test,
        NULL,
        NULL,
        NULL,
        true
    }
};

static const size_t g_benchmark_count = sizeof(g_benchmarks) / sizeof(g_benchmarks[0]);

// =============================================================================
// Utility Functions
// =============================================================================

static int create_output_directory(const char* dir) {
    struct stat st;
    if (stat(dir, &st) == 0) {
        if (S_ISDIR(st.st_mode)) {
            return 0;  // Directory exists
        } else {
            fprintf(stderr, "Error: %s exists but is not a directory\n", dir);
            return -1;
        }
    }
    
    if (mkdir(dir, 0755) != 0) {
        fprintf(stderr, "Error: Failed to create directory %s: %s\n", dir, strerror(errno));
        return -1;
    }
    
    return 0;
}

static benchmark_test_context_t* create_test_context(void) {
    benchmark_test_context_t* ctx = calloc(1, sizeof(benchmark_test_context_t));
    if (!ctx) {
        return NULL;
    }
    
    ctx->data_size = BENCHMARK_DATA_SIZE;
    ctx->test_data = malloc(ctx->data_size);
    if (!ctx->test_data) {
        free(ctx);
        return NULL;
    }
    
    // Initialize with conserved data
    benchmark_generate_conserved_data(ctx->test_data, ctx->data_size, 42);
    
    // Generate test parameters
    benchmark_generate_boundary_region(&ctx->region, 123);
    benchmark_generate_transform_params(&ctx->transform, 456);
    
    return ctx;
}

static void destroy_test_context(benchmark_test_context_t* ctx) {
    if (!ctx) {
        return;
    }
    
    free(ctx->test_data);
    free(ctx);
}

static int run_single_benchmark(benchmark_entry_t* entry) {
    if (!entry || !entry->enabled) {
        return 0;
    }
    
    printf("Running benchmark: %s\n", entry->name);
    if (g_config.verbose_output) {
        printf("  Description: %s\n", entry->description);
    }
    
    // Create benchmark configuration
    benchmark_config_t config = benchmark_get_default_config();
    if (g_config.iterations > 0) {
        config.iterations = g_config.iterations;
    }
    if (g_config.warmup_iterations >= 0) {
        config.warmup_iterations = g_config.warmup_iterations;
    }
    
    // Create benchmark context
    benchmark_context_t* ctx = benchmark_context_create(entry->name, entry->description, &config);
    if (!ctx) {
        fprintf(stderr, "  Error: Failed to create benchmark context\n");
        return -1;
    }
    
    // Create test context if needed
    benchmark_test_context_t* test_ctx = NULL;
    if (entry->setup || entry->cleanup) {
        test_ctx = create_test_context();
        if (!test_ctx) {
            fprintf(stderr, "  Error: Failed to create test context\n");
            benchmark_context_destroy(ctx);
            return -1;
        }
        entry->user_data = test_ctx;
    }
    
    // Run benchmark
    const benchmark_results_t* results = benchmark_run(ctx, entry->function,
                                                      entry->setup, entry->cleanup,
                                                      entry->user_data);
    
    if (!results) {
        fprintf(stderr, "  Error: Benchmark execution failed\n");
        destroy_test_context(test_ctx);
        benchmark_context_destroy(ctx);
        return -1;
    }
    
    // Display results
    if (g_config.verbose_output) {
        benchmark_print_results(results, g_config.use_colors);
    } else {
        printf("  Mean time: %lu ns, Success: %s\n", 
               results->timing.mean_ns,
               results->success ? "Yes" : "No");
    }
    
    // Write output files
    char filename[MAX_FILENAME_LENGTH];
    
    if (g_config.write_text) {
        snprintf(filename, sizeof(filename), "%s/%s.txt", g_config.output_dir, entry->name);
        benchmark_write_text_report(results, filename, false);
    }
    
    if (g_config.write_csv) {
        snprintf(filename, sizeof(filename), "%s/%s.csv", g_config.output_dir, entry->name);
        benchmark_write_csv_report(results, filename, true);
    }
    
    if (g_config.write_json) {
        snprintf(filename, sizeof(filename), "%s/%s.json", g_config.output_dir, entry->name);
        benchmark_write_json_report(results, filename, true);
    }
    
    // Check for regression if baseline exists
    if (g_config.compare_to_baseline) {
        // TODO: Load baseline and compare
        printf("  (Baseline comparison not implemented)\n");
    }
    
    // Clean up
    destroy_test_context(test_ctx);
    benchmark_context_destroy(ctx);
    
    printf("  Completed successfully\n\n");
    return 0;
}

// =============================================================================
// Command Line Processing
// =============================================================================

static void print_usage(const char* program_name) {
    printf("Usage: %s [OPTIONS]\n\n", program_name);
    printf("Atlas Layer 4 Benchmark Runner\n\n");
    printf("Options:\n");
    printf("  -h, --help              Show this help message\n");
    printf("  -o, --output-dir DIR    Output directory (default: %s)\n", DEFAULT_OUTPUT_DIR);
    printf("  -b, --baseline FILE     Baseline file for comparison\n");
    printf("  -g, --generate-baseline Generate new baseline\n");
    printf("  -c, --compare           Compare against baseline\n");
    printf("  -v, --verbose           Verbose output\n");
    printf("  --no-colors             Disable colored output\n");
    printf("  --csv                   Write CSV output files\n");
    printf("  --json                  Write JSON output files\n");
    printf("  --text                  Write text output files\n");
    printf("  -i, --iterations N      Number of iterations (default: %d)\n", 
           BENCHMARK_DEFAULT_ITERATIONS);
    printf("  -w, --warmup N          Warmup iterations (default: %d)\n", 
           BENCHMARK_DEFAULT_WARMUP_ITERATIONS);
    printf("  -t, --threshold X       Regression threshold (default: 1.1)\n");
    printf("  --abort-on-regression   Abort on performance regression\n");
    printf("\n");
    printf("Examples:\n");
    printf("  %s                      Run all benchmarks with defaults\n", program_name);
    printf("  %s -v --csv --json      Run with verbose output and file generation\n", program_name);
    printf("  %s -i 5000 -w 500       Run with 5000 iterations, 500 warmup\n", program_name);
    printf("\n");
}

static int parse_command_line(int argc, char* argv[]) {
    // Set defaults
    strncpy(g_config.output_dir, DEFAULT_OUTPUT_DIR, sizeof(g_config.output_dir) - 1);
    strncpy(g_config.baseline_file, BASELINE_FILENAME, sizeof(g_config.baseline_file) - 1);
    g_config.use_colors = isatty(STDOUT_FILENO);
    g_config.iterations = BENCHMARK_DEFAULT_ITERATIONS;
    g_config.warmup_iterations = BENCHMARK_DEFAULT_WARMUP_ITERATIONS;
    g_config.regression_threshold = 1.1;
    
    static struct option long_options[] = {
        {"help",               no_argument,       0, 'h'},
        {"output-dir",         required_argument, 0, 'o'},
        {"baseline",           required_argument, 0, 'b'},
        {"generate-baseline",  no_argument,       0, 'g'},
        {"compare",            no_argument,       0, 'c'},
        {"verbose",            no_argument,       0, 'v'},
        {"no-colors",          no_argument,       0, 0},
        {"csv",                no_argument,       0, 0},
        {"json",               no_argument,       0, 0},
        {"text",               no_argument,       0, 0},
        {"iterations",         required_argument, 0, 'i'},
        {"warmup",             required_argument, 0, 'w'},
        {"threshold",          required_argument, 0, 't'},
        {"abort-on-regression", no_argument,      0, 0},
        {0, 0, 0, 0}
    };
    
    int option_index = 0;
    int c;
    
    while ((c = getopt_long(argc, argv, "ho:b:gcvi:w:t:", long_options, &option_index)) != -1) {
        switch (c) {
        case 'h':
            print_usage(argv[0]);
            return 1;
            
        case 'o':
            strncpy(g_config.output_dir, optarg, sizeof(g_config.output_dir) - 1);
            break;
            
        case 'b':
            strncpy(g_config.baseline_file, optarg, sizeof(g_config.baseline_file) - 1);
            break;
            
        case 'g':
            g_config.generate_baseline = true;
            break;
            
        case 'c':
            g_config.compare_to_baseline = true;
            break;
            
        case 'v':
            g_config.verbose_output = true;
            break;
            
        case 'i':
            g_config.iterations = atoi(optarg);
            if (g_config.iterations <= 0 || g_config.iterations > BENCHMARK_MAX_ITERATIONS) {
                fprintf(stderr, "Error: Invalid iterations count: %s\n", optarg);
                return -1;
            }
            break;
            
        case 'w':
            g_config.warmup_iterations = atoi(optarg);
            if (g_config.warmup_iterations < 0 || g_config.warmup_iterations > BENCHMARK_MAX_ITERATIONS) {
                fprintf(stderr, "Error: Invalid warmup iterations: %s\n", optarg);
                return -1;
            }
            break;
            
        case 't':
            g_config.regression_threshold = atof(optarg);
            if (g_config.regression_threshold < 1.0) {
                fprintf(stderr, "Error: Invalid regression threshold: %s\n", optarg);
                return -1;
            }
            break;
            
        case 0:
            // Long options
            if (strcmp(long_options[option_index].name, "no-colors") == 0) {
                g_config.use_colors = false;
            } else if (strcmp(long_options[option_index].name, "csv") == 0) {
                g_config.write_csv = true;
            } else if (strcmp(long_options[option_index].name, "json") == 0) {
                g_config.write_json = true;
            } else if (strcmp(long_options[option_index].name, "text") == 0) {
                g_config.write_text = true;
            } else if (strcmp(long_options[option_index].name, "abort-on-regression") == 0) {
                g_config.abort_on_regression = true;
            }
            break;
            
        default:
            print_usage(argv[0]);
            return -1;
        }
    }
    
    return 0;
}

// =============================================================================
// Main Program
// =============================================================================

int main(int argc, char* argv[]) {
    printf("Atlas Layer 4 Benchmark Runner v1.0.0\n");
    printf("=======================================\n\n");
    
    // Parse command line arguments
    int parse_result = parse_command_line(argc, argv);
    if (parse_result != 0) {
        return parse_result > 0 ? 0 : 1;  // 1 = help shown, -1 = error
    }
    
    // Setup signal handlers
    setup_signal_handlers();
    
    // Initialize benchmark framework
    if (benchmark_framework_init() != 0) {
        fprintf(stderr, "Error: Failed to initialize benchmark framework\n");
        const char* error = benchmark_get_last_error();
        if (error) {
            fprintf(stderr, "  %s\n", error);
        }
        return 1;
    }
    
    // Create output directory
    if (create_output_directory(g_config.output_dir) != 0) {
        benchmark_framework_shutdown();
        return 1;
    }
    
    printf("Configuration:\n");
    printf("  Output directory: %s\n", g_config.output_dir);
    printf("  Iterations: %d\n", g_config.iterations);
    printf("  Warmup iterations: %d\n", g_config.warmup_iterations);
    printf("  Verbose output: %s\n", g_config.verbose_output ? "Yes" : "No");
    printf("  Colors: %s\n", g_config.use_colors ? "Yes" : "No");
    printf("  Output formats: %s%s%s\n",
           g_config.write_text ? "Text " : "",
           g_config.write_csv ? "CSV " : "",
           g_config.write_json ? "JSON " : "");
    printf("\n");
    
    // Run benchmarks
    int failed_benchmarks = 0;
    uint64_t total_start_time = benchmark_get_timestamp_ns();
    
    for (size_t i = 0; i < g_benchmark_count; i++) {
        if (g_interrupted) {
            printf("Benchmark suite interrupted by user.\n");
            break;
        }
        
        if (run_single_benchmark(&g_benchmarks[i]) != 0) {
            failed_benchmarks++;
            if (g_config.abort_on_regression) {
                printf("Aborting due to benchmark failure.\n");
                break;
            }
        }
    }
    
    uint64_t total_end_time = benchmark_get_timestamp_ns();
    uint64_t total_time_ns = total_end_time - total_start_time;
    
    // Print summary
    printf("=== Benchmark Suite Summary ===\n");
    printf("Total benchmarks: %zu\n", g_benchmark_count);
    printf("Failed benchmarks: %d\n", failed_benchmarks);
    printf("Success rate: %.1f%%\n", 
           (double)(g_benchmark_count - failed_benchmarks) / g_benchmark_count * 100.0);
    
    char duration_str[64];
    benchmark_format_duration(total_time_ns, duration_str, sizeof(duration_str));
    printf("Total time: %s\n", duration_str);
    printf("Output directory: %s\n", g_config.output_dir);
    
    if (failed_benchmarks > 0) {
        printf("\n%sWarning: %d benchmark(s) failed%s\n", 
               g_config.use_colors ? "\033[33m" : "",
               failed_benchmarks,
               g_config.use_colors ? "\033[0m" : "");
    }
    
    // Cleanup
    benchmark_framework_shutdown();
    
    return failed_benchmarks > 0 ? 1 : 0;
}