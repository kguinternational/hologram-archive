# Makefile for Atlas-12288 Examples

CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -O2 -I../include
LDFLAGS = -L../runtime/lib -L../llvm/lib
LIBS = -latlas-runtime -latlas -lm

# Detect OS for shared library paths
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
    RPATH_FLAG = -Wl,-rpath,'$$ORIGIN/../runtime/lib:$$ORIGIN/../llvm/lib'
endif
ifeq ($(UNAME_S),Darwin)
    RPATH_FLAG = -Wl,-rpath,@loader_path/../runtime/lib:@loader_path/../llvm/lib
endif

# Targets
EXAMPLES = projection_seed

all: $(EXAMPLES)

projection_seed: projection_seed.c
	@echo "[CC] Building projection_seed example..."
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS) $(RPATH_FLAG) $(LIBS)
	@echo "[OK] projection_seed built successfully"

run: projection_seed
	@echo "[RUN] Running projection_seed example..."
	@./projection_seed
	@echo ""
	@echo "[OUTPUT] Shard metadata:"
	@cat shard_metadata.json | head -20
	@echo "... (truncated)"

clean:
	rm -f $(EXAMPLES) *.o shard_metadata.json

install: $(EXAMPLES)
	@echo "[INSTALL] Installing examples..."
	install -d $(PREFIX)/bin
	install -m 755 $(EXAMPLES) $(PREFIX)/bin/
	@echo "[OK] Examples installed to $(PREFIX)/bin"

.PHONY: all run clean install

help:
	@echo "Atlas-12288 Examples"
	@echo "==================="
	@echo ""
	@echo "Targets:"
	@echo "  make              - Build all examples"
	@echo "  make run          - Run projection_seed example"
	@echo "  make clean        - Remove built files"
	@echo "  make install      - Install examples to PREFIX/bin"
	@echo ""
	@echo "Examples:"
	@echo "  projection_seed   - L2/L3 integration for L4 unblocking"
	@echo ""
	@echo "The projection_seed example demonstrates:"
	@echo "  • Domain creation and conservation"
	@echo "  • Resonance histogram generation"
	@echo "  • Cluster building with CSR format"
	@echo "  • Witness generation and verification"
	@echo "  • JSON shard metadata emission"