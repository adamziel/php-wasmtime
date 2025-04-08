// Build with:
// emcc -O2 substring.c -o substring.wasm -s WASM=1 -s STANDALONE_WASM=1 -s EXPORTED_FUNCTIONS='["_substring", "_string_length", "_hello_number"]' -s IMPORTED_MEMORY=1 --no-entry --import-memory

#include <emscripten.h>
#include <string.h>

// Export the substring function to be callable from JavaScript/PHP
EMSCRIPTEN_KEEPALIVE
char* substring(const char* str, int start, int length) {
    int str_len = strlen(str);
    
    // Validate parameters
    if (start < 0 || start >= str_len || length < 0) {
        return 0;
    }
    
    // Adjust length if it goes beyond the string
    if (start + length > str_len) {
        length = str_len - start;
    }
    
    // Allocate memory for the result
    char* result = (char*)malloc(length + 1);
    if (result == 0) {
        return 0;
    }
    
    // Copy the substring
    for (int i = 0; i < length; i++) {
        result[i] = str[start + i];
    }
    result[length] = '\0';
    
    return result;
}

