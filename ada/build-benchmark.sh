#!/bin/bash

g++ -std=c++20 -o demo demo.cpp
chmod +x demo
./demo
# Parsed URL 1,000,000 times in 1124 ms
# Average time per parse: 0.001124 ms

em++ -std=c++20 -o ../examples/ada.wasm demo.cpp -s STANDALONE_WASM=1 -s WASM=2 --no-entry -s EXPORTED_FUNCTIONS='["_get_host", "_malloc"]'
php ../examples/ada.php
# Parsed URL 1,000,000 times in 2624 ms