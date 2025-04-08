set -e
# make clean
# rm -rf build .libs autom4te.cache/ configure* config.h config.log config.status
phpize
CFLAGS="-arch arm64 -g0 -O3" LDFLAGS="-arch arm64" ./configure --with-wasmtime=../third_party/wasmtime-v31.0.0-aarch64-macos-c-api --build=arm64-apple-darwin --host=arm64-apple-darwin --target=arm64-apple-darwin
make
make install
DYLD_LIBRARY_PATH=../third_party/wasmtime-v31.0.0-aarch64-macos-c-api/lib php example.php

# # To run the example.php file
# # Get the certificate id
# security find-identity -p codesigning
# codesign -vvvv --deep --strict --force --timestamp -s "<CERT ID>" ../third_party/wasmtime-v31.0.0-aarch64-macos-c-api/lib/libwasmtime.dylib
# xattr -d com.apple.quarantine ../third_party/wasmtime-v31.0.0-aarch64-macos-c-api/lib/libwasmtime.dylib