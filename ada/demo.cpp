//  em++ -std=c++20 -o demo.wasm demo.cpp -s STANDALONE_WASM=1 -s WASM=2 --no-entry -s EXPORTED_FUNCTIONS='["_get_host", "_malloc"]'

#include "ada.cpp"
#include "ada.h"
#include <iostream>

// Function that accepts a URL string and returns the host part
extern "C" {
    char* get_host(const char* url_str) {
        auto url = ada::parse(url_str);
        if (!url) {
            return strdup("");
        }
        
        return strdup(std::string(url->get_host()).c_str());
    }
}

#include <chrono>
#include <cstdio>

int main() {
	const char* url = "https://example.com";
	
	// Start timing
	auto start = std::chrono::high_resolution_clock::now();
	
	// Parse the URL a million times
	for (int i = 0; i < 1000000; i++) {
		auto parsed_url = ada::parse(url);
		// Use the result to prevent optimization from removing the call
		if (!parsed_url) {
			printf("Failed to parse URL\n");
			return 1;
		}
	}
	
	// End timing
	auto end = std::chrono::high_resolution_clock::now();
	
	// Calculate duration in milliseconds
	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
	
	printf("Parsed URL 1,000,000 times in %lld ms\n", duration);
	printf("Average time per parse: %.6f ms\n", duration / 1000000.0);
	return 0;
}
