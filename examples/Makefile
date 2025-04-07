# Makefile for building WebAssembly examples using AssemblyScript

# Directory settings
EXAMPLES_DIR = examples
BUILD_DIR = examples
NODE_MODULES = node_modules

# AssemblyScript compiler
ASC = $(NODE_MODULES)/.bin/asc

# Find all TypeScript files in examples directory
TS_FILES = $(wildcard $(EXAMPLES_DIR)/*.ts)
WASM_FILES = $(patsubst $(EXAMPLES_DIR)/%.ts,$(BUILD_DIR)/%.wasm,$(TS_FILES))

# Default target
all: prepare $(WASM_FILES)

# Create build directory if it doesn't exist
prepare:
	@mkdir -p $(BUILD_DIR)
	@if [ ! -d "$(NODE_MODULES)" ]; then \
		echo "Installing AssemblyScript..."; \
		npm install assemblyscript; \
	fi

# Rule to compile TypeScript to WebAssembly
$(BUILD_DIR)/%.wasm: $(EXAMPLES_DIR)/%.ts
	@echo "Compiling $< to $@"
	@$(ASC) $< -o $@ -O3 --runtime minimal

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR)

.PHONY: all prepare clean
