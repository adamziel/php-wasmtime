## PHP-Wasmtime extension

This PHP extension enables running WebAssembly code (via Wasmtime):

```php
<?php

$engine = new WasmEngine();
$module = new WasmModule($engine, __DIR__ . "/hello.wasm");
$wasm = new WasmInstance($engine, $module);

$result_pointer = $wasm->call("hello", []);        
$memory = $instanceA->getMemory();

echo $memory->read($result_pointer, 26);
// Prints "Hello, World!":
```

This project was built in one day to explore what's possible. It is definitely **not** production-ready.

### Building

First, create a directory called `third_party/wasmtime-v31.0.0-aarch64-macos-c-api` and make sure you have [the relevant wasmtime release](https://github.com/bytecodealliance/wasmtime/releases/tag/v31.0.0) in there.

Then, building and installing the extension as follows:

The script assumes there's a third_party/wasmtime-v31.0.0-aarch64-macos-c-api directory with the relevant wasmtime release.
You can download it from https://github.com/bytecodealliance/wasmtime/releases/tag/v31.0.0.

```bash
cd extension
bash build.sh
```

To run it, make sure Wasmtime dynamic library is available in the system.

### Benchmarking against vanilla PHP

This benchmark parses the HTML spec file (about 1.2MB) and counts the tokens using:

1) @sirreal's Rust implementation of the WordPress HTML API: https://github.com/sirreal/wp-html-api-rs/
2) WordPress HTML API (WP_HTML_Tag_Processor)

Here are the results on my machine:

```bash
> cd examples
> bash wp_html_api_benchmark.bash 

Benchmarking WASM implementation...

Counting tokens found 939979 tokens [0.3819s]
Getting token details  [1.2521s]

Benchmarking PHP implementation...

Counting tokens found 939979 tokens [0.6115s]
Getting token details  [0.9332s]
```
