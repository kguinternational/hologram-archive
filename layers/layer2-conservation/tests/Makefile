# Makefile for Atlas-12288 Layer 2 Conservation Tests
# (c) 2024-2025 UOR Foundation - MIT License

CC = clang
CFLAGS = -std=c11 -Wall -Wextra -O2 -g
LLVM_FLAGS = -Wno-override-module

# Directories
SRC_DIR = ../runtime
INCLUDE_DIR = ../include
LLVM_DIR = ../llvm

# LLVM IR files
LLVM_SOURCES = \
	$(LLVM_DIR)/batch-ops.ll \
	$(LLVM_DIR)/domains.ll \
	$(LLVM_DIR)/memory.ll \
	$(LLVM_DIR)/witness.ll

# C source files
C_SOURCES = \
	$(SRC_DIR)/conservation.c

# Test targets
TESTS = test-batch-ops test-conservation test-properties test-concurrency

# Build all tests
all: $(TESTS)

# Test batch operations
test-batch-ops: test-batch-ops.c $(C_SOURCES) $(LLVM_SOURCES)
	@echo "Building batch operations test..."
	$(CC) $(CFLAGS) -I$(INCLUDE_DIR) -o $@ \
		test-batch-ops.c $(C_SOURCES) $(LLVM_SOURCES) $(LLVM_FLAGS) -lm

# Test basic conservation functions
test-conservation: test-conservation.c $(C_SOURCES) $(LLVM_SOURCES)
	@echo "Building conservation test..."
	$(CC) $(CFLAGS) -I$(INCLUDE_DIR) -o $@ \
		test-conservation.c $(C_SOURCES) $(LLVM_SOURCES) $(LLVM_FLAGS) -lm

# Test conservation properties
test-properties: test-properties.c $(C_SOURCES) $(LLVM_SOURCES)
	@echo "Building properties test..."
	$(CC) $(CFLAGS) -I$(INCLUDE_DIR) -o $@ \
		test-properties.c $(C_SOURCES) $(LLVM_SOURCES) $(LLVM_FLAGS) -lm

# Test concurrency properties
test-concurrency: test-concurrency.c $(C_SOURCES) $(LLVM_SOURCES)
	@echo "Building concurrency test..."
	$(CC) $(CFLAGS) -I$(INCLUDE_DIR) -pthread -o $@ \
		test-concurrency.c $(C_SOURCES) $(LLVM_SOURCES) $(LLVM_FLAGS) -lm

# Run all tests
test: all
	@echo "Running Atlas-12288 Layer 2 Conservation Tests..."
	@echo "=================================================="
	@echo
	@echo "Running batch operations tests..."
	./test-batch-ops
	@echo
	@echo "Running conservation tests..."
	./test-conservation
	@echo
	@echo "Running properties tests..."
	./test-properties
	@echo
	@echo "Running concurrency tests..."
	./test-concurrency
	@echo
	@echo "All tests completed!"

# Run only batch operation tests
test-batch: test-batch-ops
	@echo "Running batch operations tests..."
	./test-batch-ops

# Benchmark batch operations
benchmark: test-batch-ops
	@echo "Running batch operations benchmark..."
	./test-batch-ops --benchmark 2>/dev/null | grep -E "(Performance|Speedup|Best)"

# Clean build artifacts
clean:
	rm -f $(TESTS)
	rm -f *.o *.ll.o
	rm -f core dump.*

# Install test dependencies (if needed)
deps:
	@echo "Installing test dependencies..."
	# Add any dependency installation commands here

# Check for memory leaks (requires valgrind)
memcheck: test-batch-ops
	@echo "Running memory leak check..."
	valgrind --tool=memcheck --leak-check=full --show-leak-kinds=all \
		--track-origins=yes --verbose ./test-batch-ops

# Profile performance (requires perf)
profile: test-batch-ops
	@echo "Running performance profile..."
	perf record -g ./test-batch-ops
	perf report

# Generate code coverage (requires gcov)
coverage: CFLAGS += --coverage
coverage: clean test
	@echo "Generating code coverage report..."
	gcov *.gcda
	@echo "Coverage files generated: *.gcov"

# Lint and static analysis
lint:
	@echo "Running static analysis..."
	clang-tidy test-*.c $(C_SOURCES) -- -I$(INCLUDE_DIR)

# Format code
format:
	@echo "Formatting source code..."
	clang-format -i test-*.c

# Help target
help:
	@echo "Atlas-12288 Layer 2 Conservation Test Makefile"
	@echo "==============================================="
	@echo
	@echo "Available targets:"
	@echo "  all          - Build all tests"
	@echo "  test         - Build and run all tests"
	@echo "  test-batch   - Build and run only batch operation tests"
	@echo "  benchmark    - Run batch operations benchmark"
	@echo "  clean        - Clean build artifacts"
	@echo "  deps         - Install test dependencies"
	@echo "  memcheck     - Check for memory leaks (requires valgrind)"
	@echo "  profile      - Profile performance (requires perf)"
	@echo "  coverage     - Generate code coverage (requires gcov)"
	@echo "  lint         - Run static analysis (requires clang-tidy)"
	@echo "  format       - Format source code (requires clang-format)"
	@echo "  help         - Show this help message"

.PHONY: all test test-batch benchmark clean deps memcheck profile coverage lint format help