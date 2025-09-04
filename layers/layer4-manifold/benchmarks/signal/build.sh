#!/bin/bash
# Build script for Atlas Layer 4 Signal Processing Benchmarks
# (c) 2024-2025 UOR Foundation - MIT License

echo "Building Atlas Layer 4 Signal Processing Benchmarks..."
echo "========================================================="

CC=gcc
CFLAGS="-std=c99 -Wall -Wextra -O3 -march=native -mtune=native"
LDFLAGS="-lm -lz -lpthread"

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Function to build a single benchmark
build_benchmark() {
    local source=$1
    local target=$(basename "$source" .c)
    
    echo -n "Building $target... "
    
    if $CC $CFLAGS -o "$target" "$source" $LDFLAGS 2>/dev/null; then
        echo -e "${GREEN}✓${NC}"
        return 0
    else
        echo -e "${RED}✗${NC}"
        echo "Error building $target:"
        $CC $CFLAGS -o "$target" "$source" $LDFLAGS
        return 1
    fi
}

# Build benchmarks that compile successfully
success_count=0
total_count=4

echo "Building individual benchmarks..."

# r96_fourier_bench compiles successfully
if build_benchmark "r96_fourier_bench.c"; then
    ((success_count++))
fi

# Try other benchmarks (may have compilation issues that need fixing)
echo -e "\n${YELLOW}Note: The following benchmarks may need additional fixes:${NC}"

echo -n "Checking convolution_bench.c... "
if gcc -std=c99 -fsyntax-only convolution_bench.c 2>/dev/null; then
    echo -e "${GREEN}✓ (syntax OK)${NC}"
    if build_benchmark "convolution_bench.c"; then
        ((success_count++))
    fi
else
    echo -e "${YELLOW}⚠ (needs typedef fixes)${NC}"
fi

echo -n "Checking compression_bench.c... "
if gcc -std=c99 -fsyntax-only compression_bench.c 2>/dev/null; then
    echo -e "${GREEN}✓ (syntax OK)${NC}"
    if build_benchmark "compression_bench.c"; then
        ((success_count++))
    fi
else
    echo -e "${YELLOW}⚠ (needs fixes)${NC}"
fi

echo -n "Checking filtering_bench.c... "
if gcc -std=c99 -fsyntax-only filtering_bench.c 2>/dev/null; then
    echo -e "${GREEN}✓ (syntax OK)${NC}"
    if build_benchmark "filtering_bench.c"; then
        ((success_count++))
    fi
else
    echo -e "${YELLOW}⚠ (needs fixes)${NC}"
fi

echo ""
echo "Build Summary:"
echo "=============="
echo "Successfully built: $success_count/$total_count benchmarks"

if [ "$success_count" -gt 0 ]; then
    echo ""
    echo "Available benchmarks:"
    for exe in r96_fourier_bench convolution_bench compression_bench filtering_bench; do
        if [ -x "$exe" ]; then
            echo "  ./$exe"
        fi
    done
fi

echo ""
echo "To run a benchmark:"
echo "  ./r96_fourier_bench    # R96 Fourier Transform vs FFT"
echo "  ./convolution_bench    # Universal Number Convolution"  
echo "  ./compression_bench    # Holographic Compression"
echo "  ./filtering_bench      # Resonance-based Filtering"

echo ""
echo -e "${GREEN}Build complete!${NC}"

# Make script executable
chmod +x "$0"