# Hologram Container Engine and Platform Book Build System
SHELL := /bin/bash
.PHONY: all build serve clean lint format check install deps help

# Colors for output
RED := \033[0;31m
GREEN := \033[0;32m
YELLOW := \033[1;33m
BLUE := \033[0;34m
NC := \033[0m # No Color

# Directories
SRC_DIR := src
BUILD_DIR := build
THEME_DIR := theme

# Commands
MDBOOK := mdbook
NPM := npm
PYTHON := python3

# Book specific variables
BOOK_NAME := hologram-container-engine-platform
BOOK_PDF := $(BUILD_DIR)/$(BOOK_NAME).pdf

# Default target
all: check build

## help: Show this help message
help:
	@echo "Hologram Container Engine and Platform Book Build System"
	@echo "========================================================="
	@echo ""
	@echo "Available targets:"
	@grep -E '^##' Makefile | sed 's/## /  /'

## build: Build the book (HTML output)
build: check-structure
	@echo -e "$(BLUE)Building book...$(NC)"
	@$(MDBOOK) build
	@echo -e "$(GREEN)✓ Book built successfully in $(BUILD_DIR)/$(NC)"

## build-pdf: Build PDF version of the book
build-pdf: build
	@echo -e "$(BLUE)Generating PDF...$(NC)"
	@if command -v wkhtmltopdf &> /dev/null; then \
		wkhtmltopdf \
			--enable-local-file-access \
			--javascript-delay 2000 \
			--no-stop-slow-scripts \
			--debug-javascript \
			--print-media-type \
			--margin-top 25mm \
			--margin-bottom 25mm \
			--margin-left 25mm \
			--margin-right 25mm \
			--header-spacing 5 \
			--footer-spacing 5 \
			--footer-center "[page]" \
			--footer-font-size 9 \
			--enable-toc-back-links \
			--outline \
			--outline-depth 3 \
			$(BUILD_DIR)/print.html \
			$(BOOK_PDF); \
		echo -e "$(GREEN)✓ PDF generated at $(BOOK_PDF)$(NC)"; \
	else \
		echo -e "$(YELLOW)⚠ wkhtmltopdf not installed. Skipping PDF generation.$(NC)"; \
		echo -e "  Install with: sudo apt-get install wkhtmltopdf"; \
	fi

## serve: Start development server with live reload
serve:
	@echo -e "$(BLUE)Starting development server...$(NC)"
	@echo -e "$(GREEN)Server running at http://localhost:3000$(NC)"
	@echo -e "$(YELLOW)Press Ctrl+C to stop$(NC)"
	@$(MDBOOK) serve --open

## watch: Watch for changes and rebuild automatically
watch:
	@echo -e "$(BLUE)Watching for changes...$(NC)"
	@$(MDBOOK) watch

## clean: Remove build artifacts
clean:
	@echo -e "$(BLUE)Cleaning build artifacts...$(NC)"
	@rm -rf $(BUILD_DIR)
	@echo -e "$(GREEN)✓ Build directory cleaned$(NC)"

## check: Run all checks (structure, links)
check: check-structure
	@echo -e "$(GREEN)✓ All checks passed$(NC)"

check-structure:
	@echo -e "$(BLUE)Checking book structure...$(NC)"
	@if [ ! -f book.toml ]; then \
		echo -e "$(RED)✗ book.toml not found$(NC)"; \
		exit 1; \
	fi
	@if [ ! -f $(SRC_DIR)/SUMMARY.md ]; then \
		echo -e "$(RED)✗ SUMMARY.md not found$(NC)"; \
		exit 1; \
	fi
	@echo -e "$(GREEN)✓ Book structure valid$(NC)"

## stats: Show book statistics
stats:
	@echo -e "$(BLUE)Book Statistics$(NC)"
	@echo -e "$(BLUE)===============$(NC)"
	@echo -n "Chapters: "; find $(SRC_DIR) -name "*.md" -type f | wc -l
	@echo -n "Words: "; find $(SRC_DIR) -name "*.md" -type f -exec cat {} \; | wc -w
	@echo -n "Lines: "; find $(SRC_DIR) -name "*.md" -type f -exec cat {} \; | wc -l
	@echo -n "Characters: "; find $(SRC_DIR) -name "*.md" -type f -exec cat {} \; | wc -c

# Development shortcuts
.PHONY: s b c
s: serve
b: build
c: clean