/* traditional_compression.c - Traditional Compression Algorithms for Baseline Comparison
 * (c) 2024-2025 UOR Foundation - MIT License
 * 
 * This file implements standard compression algorithms (RLE and Huffman) without 
 * Atlas-specific optimizations to provide baseline performance comparisons against
 * Atlas Universal Number conservation-based compression approaches.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>

// =============================================================================
// Traditional Compression Data Structures
// =============================================================================

typedef struct {
    uint8_t* data;
    size_t size;
    size_t capacity;
} traditional_buffer_t;

typedef struct {
    uint8_t symbol;
    uint32_t frequency;
} traditional_frequency_t;

typedef struct traditional_huffman_node {
    uint8_t symbol;
    uint32_t frequency;
    struct traditional_huffman_node* left;
    struct traditional_huffman_node* right;
    bool is_leaf;
} traditional_huffman_node_t;

typedef struct {
    char* code;
    int length;
} traditional_huffman_code_t;

typedef struct {
    traditional_huffman_node_t* root;
    traditional_huffman_code_t codes[256];
    uint32_t frequencies[256];
} traditional_huffman_table_t;

// =============================================================================
// Buffer Management
// =============================================================================

/**
 * Create a new buffer with specified initial capacity.
 */
traditional_buffer_t* traditional_buffer_create(size_t initial_capacity) {
    traditional_buffer_t* buffer = malloc(sizeof(traditional_buffer_t));
    if (!buffer) return NULL;
    
    buffer->data = malloc(initial_capacity);
    if (!buffer->data) {
        free(buffer);
        return NULL;
    }
    
    buffer->size = 0;
    buffer->capacity = initial_capacity;
    
    return buffer;
}

/**
 * Destroy buffer and free memory.
 */
void traditional_buffer_destroy(traditional_buffer_t* buffer) {
    if (buffer) {
        free(buffer->data);
        free(buffer);
    }
}

/**
 * Resize buffer if necessary to accommodate new data.
 */
static int traditional_buffer_ensure_capacity(traditional_buffer_t* buffer, size_t required_size) {
    if (required_size <= buffer->capacity) {
        return 0; // No resize needed
    }
    
    size_t new_capacity = buffer->capacity;
    while (new_capacity < required_size) {
        new_capacity *= 2;
    }
    
    uint8_t* new_data = realloc(buffer->data, new_capacity);
    if (!new_data) return -1;
    
    buffer->data = new_data;
    buffer->capacity = new_capacity;
    
    return 0;
}

/**
 * Append data to buffer.
 */
int traditional_buffer_append(traditional_buffer_t* buffer, const uint8_t* data, size_t size) {
    assert(buffer && data);
    
    if (traditional_buffer_ensure_capacity(buffer, buffer->size + size) != 0) {
        return -1;
    }
    
    memcpy(buffer->data + buffer->size, data, size);
    buffer->size += size;
    
    return 0;
}

/**
 * Append single byte to buffer.
 */
int traditional_buffer_append_byte(traditional_buffer_t* buffer, uint8_t byte) {
    return traditional_buffer_append(buffer, &byte, 1);
}

// =============================================================================
// Run-Length Encoding (RLE) Implementation
// =============================================================================

/**
 * Compress data using Run-Length Encoding.
 * Format: [count][value] pairs where count > 0
 * Uses escape byte 0xFF for runs >= 255
 */
traditional_buffer_t* traditional_rle_compress(const uint8_t* input, size_t input_size) {
    if (!input || input_size == 0) return NULL;
    
    traditional_buffer_t* compressed = traditional_buffer_create(input_size / 2 + 256);
    if (!compressed) return NULL;
    
    size_t i = 0;
    while (i < input_size) {
        uint8_t current_byte = input[i];
        size_t run_length = 1;
        
        // Count consecutive identical bytes
        while (i + run_length < input_size && 
               input[i + run_length] == current_byte && 
               run_length < 255) {
            run_length++;
        }
        
        // Encode run
        if (run_length == 1 && current_byte != 0xFF) {
            // Single byte, no compression needed unless it's the escape byte
            if (traditional_buffer_append_byte(compressed, current_byte) != 0) {
                traditional_buffer_destroy(compressed);
                return NULL;
            }
        } else {
            // Run of length > 1 or escape byte
            uint8_t encoded[4];  // Need 4 bytes for long runs
            int encoded_size = 0;
            
            if (run_length >= 255) {
                // Long run: 0xFF [high_byte] [low_byte] [value]
                encoded[encoded_size++] = 0xFF;
                encoded[encoded_size++] = (run_length >> 8) & 0xFF;
                encoded[encoded_size++] = run_length & 0xFF;
                encoded[encoded_size++] = current_byte;
            } else {
                // Short run: 0xFF [count] [value]
                encoded[encoded_size++] = 0xFF;
                encoded[encoded_size++] = (uint8_t)run_length;
                encoded[encoded_size++] = current_byte;
            }
            
            if (traditional_buffer_append(compressed, encoded, encoded_size) != 0) {
                traditional_buffer_destroy(compressed);
                return NULL;
            }
        }
        
        i += run_length;
    }
    
    return compressed;
}

/**
 * Decompress RLE-compressed data.
 */
traditional_buffer_t* traditional_rle_decompress(const uint8_t* compressed, size_t compressed_size) {
    if (!compressed || compressed_size == 0) return NULL;
    
    traditional_buffer_t* decompressed = traditional_buffer_create(compressed_size * 4);
    if (!decompressed) return NULL;
    
    size_t i = 0;
    while (i < compressed_size) {
        uint8_t byte = compressed[i];
        
        if (byte == 0xFF && i + 2 < compressed_size) {
            // This is an encoded run
            uint8_t count_byte = compressed[i + 1];
            uint8_t value;
            size_t run_length;
            
            if (count_byte == 0xFF && i + 3 < compressed_size) {
                // Long run format: 0xFF 0xFF [high] [low] [value]
                run_length = (compressed[i + 2] << 8) | compressed[i + 3];
                value = compressed[i + 4];
                i += 5;
            } else {
                // Short run format: 0xFF [count] [value]
                run_length = count_byte;
                value = compressed[i + 2];
                i += 3;
            }
            
            // Output the run
            for (size_t j = 0; j < run_length; j++) {
                if (traditional_buffer_append_byte(decompressed, value) != 0) {
                    traditional_buffer_destroy(decompressed);
                    return NULL;
                }
            }
        } else {
            // Regular byte
            if (traditional_buffer_append_byte(decompressed, byte) != 0) {
                traditional_buffer_destroy(decompressed);
                return NULL;
            }
            i++;
        }
    }
    
    return decompressed;
}

/**
 * Calculate RLE compression ratio.
 */
double traditional_rle_compression_ratio(const uint8_t* input, size_t input_size) {
    traditional_buffer_t* compressed = traditional_rle_compress(input, input_size);
    if (!compressed) return -1.0;
    
    double ratio = (double)compressed->size / input_size;
    traditional_buffer_destroy(compressed);
    
    return ratio;
}

// =============================================================================
// Huffman Coding Implementation
// =============================================================================

/**
 * Calculate frequency of each byte in input data.
 */
static void traditional_huffman_calculate_frequencies(const uint8_t* input, size_t input_size,
                                                     uint32_t frequencies[256]) {
    memset(frequencies, 0, 256 * sizeof(uint32_t));
    
    for (size_t i = 0; i < input_size; i++) {
        frequencies[input[i]]++;
    }
}

/**
 * Create a new Huffman tree node.
 */
static traditional_huffman_node_t* traditional_huffman_create_node(uint8_t symbol, 
                                                                  uint32_t frequency,
                                                                  bool is_leaf) {
    traditional_huffman_node_t* node = malloc(sizeof(traditional_huffman_node_t));
    if (!node) return NULL;
    
    node->symbol = symbol;
    node->frequency = frequency;
    node->left = NULL;
    node->right = NULL;
    node->is_leaf = is_leaf;
    
    return node;
}

/**
 * Destroy Huffman tree recursively.
 */
static void traditional_huffman_destroy_tree(traditional_huffman_node_t* node) {
    if (node) {
        traditional_huffman_destroy_tree(node->left);
        traditional_huffman_destroy_tree(node->right);
        free(node);
    }
}

/**
 * Priority queue for building Huffman tree (simple array implementation).
 */
typedef struct {
    traditional_huffman_node_t** nodes;
    int size;
    int capacity;
} traditional_priority_queue_t;

static traditional_priority_queue_t* traditional_pq_create(int capacity) {
    traditional_priority_queue_t* pq = malloc(sizeof(traditional_priority_queue_t));
    if (!pq) return NULL;
    
    pq->nodes = malloc(capacity * sizeof(traditional_huffman_node_t*));
    if (!pq->nodes) {
        free(pq);
        return NULL;
    }
    
    pq->size = 0;
    pq->capacity = capacity;
    
    return pq;
}

static void traditional_pq_destroy(traditional_priority_queue_t* pq) {
    if (pq) {
        free(pq->nodes);
        free(pq);
    }
}

static int traditional_pq_insert(traditional_priority_queue_t* pq, traditional_huffman_node_t* node) {
    if (pq->size >= pq->capacity) return -1;
    
    // Simple insertion sort to maintain priority order (min-heap by frequency)
    int i = pq->size;
    while (i > 0 && pq->nodes[i-1]->frequency > node->frequency) {
        pq->nodes[i] = pq->nodes[i-1];
        i--;
    }
    
    pq->nodes[i] = node;
    pq->size++;
    
    return 0;
}

static traditional_huffman_node_t* traditional_pq_extract_min(traditional_priority_queue_t* pq) {
    if (pq->size == 0) return NULL;
    
    traditional_huffman_node_t* min = pq->nodes[0];
    
    // Shift remaining elements
    for (int i = 1; i < pq->size; i++) {
        pq->nodes[i-1] = pq->nodes[i];
    }
    
    pq->size--;
    return min;
}

/**
 * Build Huffman tree from frequency table.
 */
static traditional_huffman_node_t* traditional_huffman_build_tree(const uint32_t frequencies[256]) {
    traditional_priority_queue_t* pq = traditional_pq_create(512);
    if (!pq) return NULL;
    
    // Create leaf nodes for all symbols with non-zero frequency
    for (int i = 0; i < 256; i++) {
        if (frequencies[i] > 0) {
            traditional_huffman_node_t* leaf = traditional_huffman_create_node(i, frequencies[i], true);
            if (!leaf || traditional_pq_insert(pq, leaf) != 0) {
                traditional_pq_destroy(pq);
                return NULL;
            }
        }
    }
    
    // Handle edge case: only one unique symbol
    if (pq->size == 1) {
        traditional_huffman_node_t* leaf = traditional_pq_extract_min(pq);
        traditional_huffman_node_t* root = traditional_huffman_create_node(0, leaf->frequency, false);
        if (root) {
            root->left = leaf;
        }
        traditional_pq_destroy(pq);
        return root;
    }
    
    // Build tree by combining nodes with lowest frequencies
    while (pq->size > 1) {
        traditional_huffman_node_t* left = traditional_pq_extract_min(pq);
        traditional_huffman_node_t* right = traditional_pq_extract_min(pq);
        
        traditional_huffman_node_t* internal = traditional_huffman_create_node(0, 
                                               left->frequency + right->frequency, false);
        if (!internal) {
            traditional_pq_destroy(pq);
            return NULL;
        }
        
        internal->left = left;
        internal->right = right;
        
        if (traditional_pq_insert(pq, internal) != 0) {
            traditional_huffman_destroy_tree(internal);
            traditional_pq_destroy(pq);
            return NULL;
        }
    }
    
    traditional_huffman_node_t* root = traditional_pq_extract_min(pq);
    traditional_pq_destroy(pq);
    
    return root;
}

/**
 * Generate Huffman codes from tree.
 */
static void traditional_huffman_generate_codes(traditional_huffman_node_t* node, 
                                              traditional_huffman_code_t codes[256],
                                              char* current_code, int depth) {
    if (!node) return;
    
    if (node->is_leaf) {
        codes[node->symbol].code = malloc(depth + 1);
        if (codes[node->symbol].code) {
            strcpy(codes[node->symbol].code, current_code);
            codes[node->symbol].length = depth;
        }
        return;
    }
    
    // Traverse left (append '0')
    if (node->left) {
        current_code[depth] = '0';
        current_code[depth + 1] = '\0';
        traditional_huffman_generate_codes(node->left, codes, current_code, depth + 1);
    }
    
    // Traverse right (append '1')
    if (node->right) {
        current_code[depth] = '1';
        current_code[depth + 1] = '\0';
        traditional_huffman_generate_codes(node->right, codes, current_code, depth + 1);
    }
}

/**
 * Create Huffman coding table.
 */
traditional_huffman_table_t* traditional_huffman_create_table(const uint8_t* input, size_t input_size) {
    if (!input || input_size == 0) return NULL;
    
    traditional_huffman_table_t* table = malloc(sizeof(traditional_huffman_table_t));
    if (!table) return NULL;
    
    // Initialize table
    memset(table->codes, 0, sizeof(table->codes));
    traditional_huffman_calculate_frequencies(input, input_size, table->frequencies);
    
    // Build Huffman tree
    table->root = traditional_huffman_build_tree(table->frequencies);
    if (!table->root) {
        free(table);
        return NULL;
    }
    
    // Generate codes
    char current_code[256] = {0};
    traditional_huffman_generate_codes(table->root, table->codes, current_code, 0);
    
    return table;
}

/**
 * Destroy Huffman table.
 */
void traditional_huffman_destroy_table(traditional_huffman_table_t* table) {
    if (table) {
        traditional_huffman_destroy_tree(table->root);
        
        for (int i = 0; i < 256; i++) {
            free(table->codes[i].code);
        }
        
        free(table);
    }
}

/**
 * Compress data using Huffman coding.
 */
traditional_buffer_t* traditional_huffman_compress(const uint8_t* input, size_t input_size,
                                                  traditional_huffman_table_t* table) {
    if (!input || !table || input_size == 0) return NULL;
    
    traditional_buffer_t* compressed = traditional_buffer_create(input_size);
    if (!compressed) return NULL;
    
    // Write a simple header: frequencies for reconstruction
    for (int i = 0; i < 256; i++) {
        uint32_t freq = table->frequencies[i];
        uint8_t freq_bytes[4] = {
            (freq >> 24) & 0xFF,
            (freq >> 16) & 0xFF,
            (freq >> 8) & 0xFF,
            freq & 0xFF
        };
        
        if (traditional_buffer_append(compressed, freq_bytes, 4) != 0) {
            traditional_buffer_destroy(compressed);
            return NULL;
        }
    }
    
    // Encode data
    char bit_buffer[8192] = {0};
    int bit_pos = 0;
    
    for (size_t i = 0; i < input_size; i++) {
        uint8_t symbol = input[i];
        const char* code = table->codes[symbol].code;
        int code_length = table->codes[symbol].length;
        
        if (!code) {
            // Symbol not in table (shouldn't happen)
            traditional_buffer_destroy(compressed);
            return NULL;
        }
        
        // Append bits to buffer
        for (int j = 0; j < code_length; j++) {
            bit_buffer[bit_pos++] = code[j];
            
            // Flush byte when buffer has 8 bits
            if (bit_pos == 8) {
                uint8_t byte = 0;
                for (int k = 0; k < 8; k++) {
                    if (bit_buffer[k] == '1') {
                        byte |= (1 << (7 - k));
                    }
                }
                
                if (traditional_buffer_append_byte(compressed, byte) != 0) {
                    traditional_buffer_destroy(compressed);
                    return NULL;
                }
                
                bit_pos = 0;
            }
        }
    }
    
    // Flush remaining bits (pad with zeros)
    if (bit_pos > 0) {
        uint8_t byte = 0;
        for (int k = 0; k < bit_pos; k++) {
            if (bit_buffer[k] == '1') {
                byte |= (1 << (7 - k));
            }
        }
        
        if (traditional_buffer_append_byte(compressed, byte) != 0) {
            traditional_buffer_destroy(compressed);
            return NULL;
        }
    }
    
    return compressed;
}

/**
 * Calculate theoretical Huffman compression ratio.
 */
double traditional_huffman_theoretical_ratio(const uint32_t frequencies[256], size_t total_symbols) {
    if (total_symbols == 0) return -1.0;
    
    double entropy = 0.0;
    
    for (int i = 0; i < 256; i++) {
        if (frequencies[i] > 0) {
            double probability = (double)frequencies[i] / total_symbols;
            entropy -= probability * log2(probability);
        }
    }
    
    return entropy / 8.0; // Convert bits to bytes
}

// =============================================================================
// Benchmark Functions
// =============================================================================

/**
 * Generate test data with different characteristics.
 */
void traditional_generate_test_data(uint8_t* buffer, size_t size, const char* type) {
    if (strcmp(type, "random") == 0) {
        for (size_t i = 0; i < size; i++) {
            buffer[i] = rand() % 256;
        }
    } else if (strcmp(type, "repeated") == 0) {
        uint8_t pattern = rand() % 256;
        memset(buffer, pattern, size);
    } else if (strcmp(type, "alternating") == 0) {
        for (size_t i = 0; i < size; i++) {
            buffer[i] = (i % 2) ? 0xAA : 0x55;
        }
    } else if (strcmp(type, "text_like") == 0) {
        // Simulate text-like distribution (some characters more common)
        const char common_chars[] = "etaoinshrdlcumwfgypbvkjxqz ETAOINSHRDLCUMWFGYPBVKJXQZ";
        int num_common = strlen(common_chars);
        
        for (size_t i = 0; i < size; i++) {
            if (rand() % 10 < 7) {
                // 70% chance of common character
                buffer[i] = common_chars[rand() % num_common];
            } else {
                // 30% chance of any character
                buffer[i] = rand() % 256;
            }
        }
    }
}

/**
 * Benchmark RLE compression.
 */
double traditional_benchmark_rle(const uint8_t* data, size_t size, int iterations) {
    clock_t start = clock();
    
    double total_ratio = 0.0;
    for (int i = 0; i < iterations; i++) {
        traditional_buffer_t* compressed = traditional_rle_compress(data, size);
        if (compressed) {
            total_ratio += (double)compressed->size / size;
            traditional_buffer_destroy(compressed);
        }
    }
    
    clock_t end = clock();
    
    return ((double)(end - start)) / CLOCKS_PER_SEC / iterations;
}

/**
 * Benchmark Huffman compression.
 */
double traditional_benchmark_huffman(const uint8_t* data, size_t size, int iterations) {
    // Create table once
    traditional_huffman_table_t* table = traditional_huffman_create_table(data, size);
    if (!table) return -1.0;
    
    clock_t start = clock();
    
    for (int i = 0; i < iterations; i++) {
        traditional_buffer_t* compressed = traditional_huffman_compress(data, size, table);
        traditional_buffer_destroy(compressed);
    }
    
    clock_t end = clock();
    
    traditional_huffman_destroy_table(table);
    
    return ((double)(end - start)) / CLOCKS_PER_SEC / iterations;
}

// =============================================================================
// Main Function for Testing
// =============================================================================

#ifdef TRADITIONAL_COMPRESSION_MAIN
int main(void) {
    printf("Traditional Compression Algorithms Benchmark\n");
    printf("==========================================\n\n");
    
    srand((unsigned int)time(NULL));
    
    // Test different data types
    const char* test_types[] = {"random", "repeated", "alternating", "text_like"};
    int num_test_types = sizeof(test_types) / sizeof(test_types[0]);
    size_t test_size = 64 * 1024; // 64KB test data
    
    uint8_t* test_data = malloc(test_size);
    if (!test_data) {
        printf("Failed to allocate test data\n");
        return 1;
    }
    
    printf("Testing compression algorithms on %zu bytes of data:\n\n", test_size);
    
    for (int type_idx = 0; type_idx < num_test_types; type_idx++) {
        const char* type = test_types[type_idx];
        printf("Data type: %s\n", type);
        printf("----------------\n");
        
        traditional_generate_test_data(test_data, test_size, type);
        
        // Test RLE
        traditional_buffer_t* rle_compressed = traditional_rle_compress(test_data, test_size);
        if (rle_compressed) {
            double rle_ratio = (double)rle_compressed->size / test_size;
            printf("RLE compression ratio: %.3f (%.1f%% reduction)\n", 
                   rle_ratio, (1.0 - rle_ratio) * 100.0);
            
            // Test decompression
            traditional_buffer_t* rle_decompressed = traditional_rle_decompress(rle_compressed->data, 
                                                                              rle_compressed->size);
            if (rle_decompressed && rle_decompressed->size == test_size &&
                memcmp(rle_decompressed->data, test_data, test_size) == 0) {
                printf("RLE decompression: PASS\n");
            } else {
                printf("RLE decompression: FAIL\n");
            }
            
            traditional_buffer_destroy(rle_decompressed);
            traditional_buffer_destroy(rle_compressed);
        }
        
        // Test Huffman
        traditional_huffman_table_t* huffman_table = traditional_huffman_create_table(test_data, test_size);
        if (huffman_table) {
            traditional_buffer_t* huffman_compressed = traditional_huffman_compress(test_data, test_size, huffman_table);
            if (huffman_compressed) {
                double huffman_ratio = (double)huffman_compressed->size / test_size;
                printf("Huffman compression ratio: %.3f (%.1f%% reduction)\n", 
                       huffman_ratio, (1.0 - huffman_ratio) * 100.0);
                
                traditional_buffer_destroy(huffman_compressed);
            }
            
            // Calculate theoretical entropy
            double theoretical_ratio = traditional_huffman_theoretical_ratio(huffman_table->frequencies, test_size);
            if (theoretical_ratio > 0) {
                printf("Theoretical entropy ratio: %.3f\n", theoretical_ratio);
            }
            
            traditional_huffman_destroy_table(huffman_table);
        }
        
        printf("\n");
    }
    
    // Performance benchmarks
    printf("Performance Benchmarks:\n");
    printf("=====================\n");
    
    traditional_generate_test_data(test_data, test_size, "text_like");
    
    double rle_time = traditional_benchmark_rle(test_data, test_size, 100);
    if (rle_time > 0) {
        printf("RLE compression: %.6f seconds per operation\n", rle_time);
        printf("RLE throughput: %.1f MB/s\n", (test_size / (1024.0 * 1024.0)) / rle_time);
    }
    
    double huffman_time = traditional_benchmark_huffman(test_data, test_size, 10);
    if (huffman_time > 0) {
        printf("Huffman compression: %.6f seconds per operation\n", huffman_time);
        printf("Huffman throughput: %.1f MB/s\n", (test_size / (1024.0 * 1024.0)) / huffman_time);
    }
    
    printf("\nAlgorithm Complexity:\n");
    printf("RLE: O(n) time, O(n) space (worst case)\n");
    printf("Huffman: O(n log k) time for building table, O(n) for encoding\n");
    printf("where n = data size, k = alphabet size\n");
    
    free(test_data);
    return 0;
}
#endif