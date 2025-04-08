// Build with:
// emcc -O2 subtract.c -o subtract.wasm -s WASM=1 -s SIDE_MODULE=1 -s EXPORTED_FUNCTIONS='["_subtract"]'

#include <emscripten.h>

// Export the subtract function to be callable from JavaScript/PHP
EMSCRIPTEN_KEEPALIVE
int subtract(int a, int b) {
    return a - b;
}
