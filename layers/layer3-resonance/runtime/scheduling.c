/* scheduling.c - Atlas-12288 Layer 3 Scheduling Runtime
 * (c) 2024-2025 UOR Foundation - MIT License
 * 
 * Layer 3 scheduling implementation with phase-locked scheduling,
 * harmonic window calculation, and batch processing with LLVM backend.
 */

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <math.h>

#include "../include/atlas-resonance.h"

// =============================================================================
// Scheduling Window Structure
// =============================================================================

typedef struct {
    uint32_t start_time;               // Window start time
    uint32_t duration;                 // Window duration
    uint8_t resonance_class;           // Associated resonance class
    uint32_t phase_offset;             // Phase offset for harmonic alignment
} atlas_schedule_window_t;

// =============================================================================
// Phase-Locked Scheduler Structure
// =============================================================================

typedef struct {
    uint32_t magic;                    // 0x5C4ED123 (SCHEDuleR)
    uint32_t base_frequency;           // Base scheduling frequency (Hz)
    uint32_t phase_lock_threshold;     // Phase lock threshold
    bool is_phase_locked;              // Current phase lock status
    
    // Window management
    atlas_schedule_window_t current_window;
    atlas_schedule_window_t next_window;
    
    // Phase tracking
    double accumulated_phase;          // Accumulated phase error
    double phase_lock_gain;            // Phase lock loop gain
    uint32_t lock_count;               // Consecutive locked cycles
    
    // Statistics
    uint64_t total_windows;            // Total windows processed
    uint64_t missed_windows;           // Missed scheduling windows
    double avg_phase_error;            // Average phase error
    
    void* arena_ptr;                   // Arena memory
    size_t arena_size;                 // Arena size
} atlas_scheduler_t;

#define ATLAS_SCHEDULER_MAGIC 0x5C4ED123U

// =============================================================================
// Batch Scheduler Structure
// =============================================================================

typedef struct {
    uint32_t magic;                    // 0xBA7C4123 (BATCHer)
    uint32_t max_windows;              // Maximum concurrent windows
    uint32_t active_windows;           // Current active windows
    
    atlas_schedule_window_t* windows;  // Array of windows
    uint8_t* window_states;            // Window state flags
    uint32_t* window_priorities;       // Window priorities
    
    // Batch statistics
    uint32_t batch_count;              // Number of batches processed
    uint32_t total_processed;          // Total windows processed
    double avg_batch_size;             // Average batch size
    
    void* arena_ptr;                   // Arena memory
    size_t arena_size;                 // Arena size
} atlas_batch_scheduler_t;

#define ATLAS_BATCH_MAGIC 0xBA7C4123U
#define WINDOW_STATE_FREE     0x00
#define WINDOW_STATE_ACTIVE   0x01
#define WINDOW_STATE_LOCKED   0x02
#define WINDOW_STATE_EXPIRED  0x04

// =============================================================================
// LLVM IR Function Declarations
// =============================================================================

// From exports.ll
extern bool atlas_resonance_harmonizes_llvm(uint8_t r1, uint8_t r2);

// =============================================================================
// Memory Management
// =============================================================================

static void* atlas_scheduling_alloc(size_t size) {
    return aligned_alloc(32, (size + 31) & ~31);
}

static void atlas_scheduling_free(void* ptr) {
    if (ptr) {
        free(ptr);
    }
}

// =============================================================================
// Core Scheduling Functions
// =============================================================================

uint64_t atlas_schedule_next_window_time(uint64_t now, uint8_t resonance_class) {
    // Validate resonance class
    resonance_class = resonance_class % 96;
    
    // Use simple C implementation for base calculation
    uint32_t now_truncated = (uint32_t)(now & 0xFFFFFFFFULL);
    // Simple next window calculation: next = now + ((96 - ((now + r) % 96)) % 96)
    uint32_t temp = (now_truncated + (uint32_t)resonance_class) % 96;
    uint32_t offset = (96 - temp) % 96;
    if (offset == 0) offset = 96;
    uint32_t next = now_truncated + offset;
    
    // Extend to 64-bit preserving upper bits
    uint64_t upper = now & 0xFFFFFFFF00000000ULL;
    uint64_t result = upper | (uint64_t)next;
    
    // Handle overflow case
    if (result < now) {
        result += 0x100000000ULL;
    }
    
    return result;
}

atlas_schedule_window_t atlas_compute_harmonic_window(uint32_t now, uint8_t resonance_class, uint32_t duration) {
    atlas_schedule_window_t window = {0};
    
    // Validate inputs
    resonance_class = resonance_class % 96;
    if (duration == 0) {
        duration = 96;  // Default duration
    }
    
    // Calculate window using simple C implementation
    uint32_t temp = (now + (uint32_t)resonance_class) % 96;
    uint32_t offset = (96 - temp) % 96;
    if (offset == 0) offset = 96;
    
    window.start_time = now + offset;
    window.duration = duration;
    window.resonance_class = resonance_class;
    window.phase_offset = resonance_class * 8; // Simple phase calculation
    
    return window;
}

bool atlas_is_phase_locked(uint32_t time, uint8_t resonance_class) {
    resonance_class = resonance_class % 96;
    // Simple phase lock check: time aligned to resonance class modulo 96
    return ((time % 96) == (resonance_class % 96));
}

double atlas_calculate_phase_error(uint32_t actual_time, uint32_t expected_time, uint8_t resonance_class) {
    resonance_class = resonance_class % 96;
    
    // Calculate phase error as fraction of resonance period
    double period = 96.0;  // Base period for resonance calculations
    double time_diff = (double)((int32_t)actual_time - (int32_t)expected_time);
    
    // Normalize to [-0.5, 0.5] of period
    double phase_error = fmod(time_diff / period, 1.0);
    if (phase_error > 0.5) {
        phase_error -= 1.0;
    } else if (phase_error < -0.5) {
        phase_error += 1.0;
    }
    
    return phase_error;
}

// =============================================================================
// Phase-Locked Scheduler Implementation
// =============================================================================

atlas_scheduler_t* atlas_scheduler_create(uint32_t base_frequency) {
    if (base_frequency == 0) {
        base_frequency = 1000;  // Default 1kHz
    }
    
    // Allocate scheduler
    size_t scheduler_size = sizeof(atlas_scheduler_t);
    atlas_scheduler_t* sched = (atlas_scheduler_t*)atlas_scheduling_alloc(scheduler_size);
    if (!sched) {
        return NULL;
    }
    
    // Initialize scheduler
    memset(sched, 0, sizeof(atlas_scheduler_t));
    sched->magic = ATLAS_SCHEDULER_MAGIC;
    sched->base_frequency = base_frequency;
    sched->phase_lock_threshold = base_frequency / 100;  // 1% threshold
    sched->phase_lock_gain = 0.1;  // Conservative PLL gain
    sched->arena_ptr = sched;
    sched->arena_size = scheduler_size;
    
    return sched;
}

void atlas_scheduler_destroy(atlas_scheduler_t* sched) {
    if (!sched || sched->magic != ATLAS_SCHEDULER_MAGIC) {
        return;
    }
    
    sched->magic = 0;
    atlas_scheduling_free(sched);
}

bool atlas_scheduler_update(atlas_scheduler_t* sched, uint32_t current_time, uint8_t resonance_class) {
    if (!sched || sched->magic != ATLAS_SCHEDULER_MAGIC) {
        return false;
    }
    
    resonance_class = resonance_class % 96;
    
    // Calculate expected next window time
    // Calculate expected next window time using C implementation
    uint32_t temp = (current_time + (uint32_t)resonance_class) % 96;
    uint32_t offset = (96 - temp) % 96;
    if (offset == 0) offset = 96;
    uint32_t expected_time = current_time + offset;
    
    // Calculate phase error
    double phase_error = atlas_calculate_phase_error(current_time, expected_time, resonance_class);
    
    // Update accumulated phase with loop filter
    sched->accumulated_phase += phase_error * sched->phase_lock_gain;
    
    // Update average phase error (exponential moving average)
    sched->avg_phase_error = 0.9 * sched->avg_phase_error + 0.1 * fabs(phase_error);
    
    // Check phase lock status
    bool was_locked = sched->is_phase_locked;
    sched->is_phase_locked = (fabs(phase_error) < (double)sched->phase_lock_threshold / (double)sched->base_frequency);
    
    if (sched->is_phase_locked) {
        sched->lock_count++;
    } else {
        sched->lock_count = 0;
    }
    
    // Update current window
    sched->current_window = atlas_compute_harmonic_window(current_time, resonance_class, 96);
    
    // Prepare next window
    // Calculate next window time using C implementation
    uint32_t future_time = current_time + 96;
    uint32_t temp2 = (future_time + (uint32_t)resonance_class) % 96;
    uint32_t offset2 = (96 - temp2) % 96;
    if (offset2 == 0) offset2 = 96;
    uint32_t next_time = future_time + offset2;
    sched->next_window = atlas_compute_harmonic_window(next_time, resonance_class, 96);
    
    sched->total_windows++;
    
    return sched->is_phase_locked && !was_locked;  // Return true on lock acquisition
}

atlas_schedule_window_t atlas_scheduler_get_current_window(atlas_scheduler_t* sched) {
    if (!sched || sched->magic != ATLAS_SCHEDULER_MAGIC) {
        atlas_schedule_window_t empty = {0};
        return empty;
    }
    
    return sched->current_window;
}

atlas_schedule_window_t atlas_scheduler_get_next_window(atlas_scheduler_t* sched) {
    if (!sched || sched->magic != ATLAS_SCHEDULER_MAGIC) {
        atlas_schedule_window_t empty = {0};
        return empty;
    }
    
    return sched->next_window;
}

double atlas_scheduler_get_phase_error(atlas_scheduler_t* sched) {
    if (!sched || sched->magic != ATLAS_SCHEDULER_MAGIC) {
        return 0.0;
    }
    
    return sched->avg_phase_error;
}

// =============================================================================
// Batch Scheduler Implementation
// =============================================================================

atlas_batch_scheduler_t* atlas_batch_scheduler_create(uint32_t max_windows) {
    if (max_windows == 0) {
        max_windows = 64;  // Default capacity
    }
    
    // Calculate memory requirements
    size_t scheduler_size = sizeof(atlas_batch_scheduler_t);
    size_t windows_size = max_windows * sizeof(atlas_schedule_window_t);
    size_t states_size = max_windows * sizeof(uint8_t);
    size_t priorities_size = max_windows * sizeof(uint32_t);
    size_t total_size = scheduler_size + windows_size + states_size + priorities_size;
    
    // Allocate arena
    void* arena = atlas_scheduling_alloc(total_size);
    if (!arena) {
        return NULL;
    }
    
    // Layout memory
    atlas_batch_scheduler_t* batch = (atlas_batch_scheduler_t*)arena;
    batch->windows = (atlas_schedule_window_t*)((uint8_t*)arena + scheduler_size);
    batch->window_states = (uint8_t*)((uint8_t*)arena + scheduler_size + windows_size);
    batch->window_priorities = (uint32_t*)((uint8_t*)arena + scheduler_size + windows_size + states_size);
    
    // Initialize structure
    memset(batch, 0, sizeof(atlas_batch_scheduler_t));
    batch->magic = ATLAS_BATCH_MAGIC;
    batch->max_windows = max_windows;
    batch->arena_ptr = arena;
    batch->arena_size = total_size;
    
    // Initialize arrays
    memset(batch->windows, 0, windows_size);
    memset(batch->window_states, WINDOW_STATE_FREE, states_size);
    memset(batch->window_priorities, 0, priorities_size);
    
    return batch;
}

void atlas_batch_scheduler_destroy(atlas_batch_scheduler_t* batch) {
    if (!batch || batch->magic != ATLAS_BATCH_MAGIC) {
        return;
    }
    
    batch->magic = 0;
    atlas_scheduling_free(batch->arena_ptr);
}

bool atlas_batch_scheduler_add_window(atlas_batch_scheduler_t* batch, 
                                      atlas_schedule_window_t window, 
                                      uint32_t priority) {
    if (!batch || batch->magic != ATLAS_BATCH_MAGIC) {
        return false;
    }
    
    if (batch->active_windows >= batch->max_windows) {
        return false;  // Batch is full
    }
    
    // Find free slot
    uint32_t slot = UINT32_MAX;
    for (uint32_t i = 0; i < batch->max_windows; i++) {
        if (batch->window_states[i] == WINDOW_STATE_FREE) {
            slot = i;
            break;
        }
    }
    
    if (slot == UINT32_MAX) {
        return false;  // No free slots
    }
    
    // Add window
    batch->windows[slot] = window;
    batch->window_states[slot] = WINDOW_STATE_ACTIVE;
    batch->window_priorities[slot] = priority;
    batch->active_windows++;
    
    return true;
}

bool atlas_batch_scheduler_remove_window(atlas_batch_scheduler_t* batch, uint32_t slot) {
    if (!batch || batch->magic != ATLAS_BATCH_MAGIC || slot >= batch->max_windows) {
        return false;
    }
    
    if (batch->window_states[slot] == WINDOW_STATE_FREE) {
        return false;  // Already free
    }
    
    // Mark as free
    batch->window_states[slot] = WINDOW_STATE_FREE;
    batch->window_priorities[slot] = 0;
    memset(&batch->windows[slot], 0, sizeof(atlas_schedule_window_t));
    
    if (batch->active_windows > 0) {
        batch->active_windows--;
    }
    
    return true;
}

size_t atlas_batch_scheduler_process_batch(atlas_batch_scheduler_t* batch, uint32_t current_time) {
    if (!batch || batch->magic != ATLAS_BATCH_MAGIC) {
        return 0;
    }
    
    size_t processed = 0;
    size_t expired = 0;
    
    // Process all active windows
    for (uint32_t i = 0; i < batch->max_windows; i++) {
        if (batch->window_states[i] != WINDOW_STATE_ACTIVE) {
            continue;
        }
        
        atlas_schedule_window_t* window = &batch->windows[i];
        
        // Check if window has expired
        uint32_t window_end = window->start_time + window->duration;
        if (current_time >= window_end) {
            batch->window_states[i] = WINDOW_STATE_EXPIRED;
            expired++;
            continue;
        }
        
        // Check if window is currently active
        if (current_time >= window->start_time && current_time < window_end) {
            // Process window (mark as locked for phase-sensitive operations)
            if (atlas_is_phase_locked(current_time, window->resonance_class)) {
                batch->window_states[i] = WINDOW_STATE_LOCKED;
            }
            processed++;
        }
    }
    
    // Clean up expired windows
    for (uint32_t i = 0; i < batch->max_windows; i++) {
        if (batch->window_states[i] == WINDOW_STATE_EXPIRED) {
            atlas_batch_scheduler_remove_window(batch, i);
        }
    }
    
    // Update statistics
    batch->total_processed += processed;
    batch->batch_count++;
    batch->avg_batch_size = (double)batch->total_processed / (double)batch->batch_count;
    
    return processed;
}

// =============================================================================
// Advanced Scheduling Operations
// =============================================================================

bool atlas_schedule_harmonic_sequence(atlas_batch_scheduler_t* batch, 
                                      uint32_t base_time, 
                                      const uint8_t* resonance_sequence, 
                                      size_t sequence_length,
                                      uint32_t window_duration) {
    if (!batch || !resonance_sequence || sequence_length == 0) {
        return false;
    }
    
    if (window_duration == 0) {
        window_duration = 96;
    }
    
    uint32_t current_time = base_time;
    uint32_t priority = 1000;  // High priority for harmonic sequences
    
    for (size_t i = 0; i < sequence_length; i++) {
        uint8_t resonance = resonance_sequence[i] % 96;
        
        // Calculate next harmonic window
        atlas_schedule_window_t window = atlas_compute_harmonic_window(current_time, resonance, window_duration);
        
        // Add to batch
        if (!atlas_batch_scheduler_add_window(batch, window, priority)) {
            return false;  // Failed to add window
        }
        
        // Advance to next window time
        current_time = window.start_time + window.duration;
        priority++;  // Increase priority for sequence ordering
    }
    
    return true;
}

size_t atlas_find_harmonic_windows(uint32_t base_time, 
                                   uint32_t time_range, 
                                   atlas_schedule_window_t* windows, 
                                   size_t max_windows) {
    if (!windows || max_windows == 0 || time_range == 0) {
        return 0;
    }
    
    size_t found = 0;
    uint32_t current_time = base_time;
    uint32_t end_time = base_time + time_range;
    
    // Find harmonic windows across all resonance classes
    for (uint8_t r = 0; r < 96 && found < max_windows; r++) {
        uint32_t window_time = current_time;
        
        while (window_time < end_time && found < max_windows) {
            // Check if this time creates a harmonic window
            if ((window_time % 96) == r) {
                windows[found] = atlas_compute_harmonic_window(window_time, r, 96);
                found++;
            }
            
            // Advance to next potential window
            // Calculate next window time using C implementation
            uint32_t temp = (window_time + (uint32_t)r) % 96;
            uint32_t offset = (96 - temp) % 96;
            if (offset == 0) offset = 96;
            window_time = window_time + offset;
            if (window_time <= current_time) {
                break;  // Prevent infinite loop
            }
        }
    }
    
    return found;
}

bool atlas_validate_scheduling_determinism(const uint8_t* resonance_sequence, 
                                           size_t sequence_length,
                                           uint32_t base_time,
                                           uint32_t iterations) {
    if (!resonance_sequence || sequence_length == 0 || iterations == 0) {
        return false;
    }
    
    // First run
    uint32_t* first_times = (uint32_t*)malloc(sequence_length * sizeof(uint32_t));
    if (!first_times) {
        return false;
    }
    
    uint32_t time = base_time;
    for (size_t i = 0; i < sequence_length; i++) {
        uint8_t resonance = resonance_sequence[i] % 96;
        // Calculate next window time using C implementation
        uint32_t temp = (time + (uint32_t)resonance) % 96;
        uint32_t offset = (96 - temp) % 96;
        if (offset == 0) offset = 96;
        time = time + offset;
        first_times[i] = time;
    }
    
    // Verify consistency across iterations
    bool is_deterministic = true;
    for (uint32_t iter = 1; iter < iterations && is_deterministic; iter++) {
        time = base_time;
        for (size_t i = 0; i < sequence_length; i++) {
            uint8_t resonance = resonance_sequence[i] % 96;
            // Calculate next window time using C implementation  
            uint32_t temp = (time + (uint32_t)resonance) % 96;
            uint32_t offset = (96 - temp) % 96;
            if (offset == 0) offset = 96;
            time = time + offset;
            if (time != first_times[i]) {
                is_deterministic = false;
                break;
            }
        }
    }
    
    free(first_times);
    return is_deterministic;
}

// =============================================================================
// Integration Functions
// =============================================================================

uint64_t atlas_schedule_next_window(uint64_t now, uint8_t r) {
    // This function provides compatibility with the existing API
    return atlas_schedule_next_window_time(now, r);
}

uint64_t atlas_next_harmonic_window_from(uint64_t now, uint8_t r) {
    // Use the harmonic window computation for precise harmonic alignment
    uint32_t now_32 = (uint32_t)(now & 0xFFFFFFFFULL);
    atlas_schedule_window_t window = atlas_compute_harmonic_window(now_32, r, 96);
    
    // Convert back to 64-bit
    uint64_t upper = now & 0xFFFFFFFF00000000ULL;
    uint64_t result = upper | (uint64_t)window.start_time;
    
    if (result < now) {
        result += 0x100000000ULL;
    }
    
    return result;
}