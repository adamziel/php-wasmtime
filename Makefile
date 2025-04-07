# Makefile for PHP Wasmtime Extension

# Directory settings
EXAMPLES_DIR = examples

# Default target
all: build

# Build the extension
build:
	@echo "Building PHP Wasmtime extension..."
	# Add your build commands here

# Build WebAssembly examples
examples:
	@echo "Building WebAssembly examples..."
	@$(MAKE) -C $(EXAMPLES_DIR)

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	# Add your clean commands here
	@$(MAKE) -C $(EXAMPLES_DIR) clean

.PHONY: all build examples clean
