set -e
# rm -rf build .libs
phpize
CFLAGS="-arch arm64" LDFLAGS="-arch arm64" ./configure --with-wasmtime=../third_party/wasmtime-v31.0.0-aarch64-macos-c-api --build=arm64-apple-darwin --host=arm64-apple-darwin --target=arm64-apple-darwin
make
make install
DYLD_LIBRARY_PATH=../third_party/wasmtime-v31.0.0-aarch64-macos-c-api/lib php example.php
