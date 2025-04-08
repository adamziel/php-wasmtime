## PHP-WasmTime extension

Building and installing the extension:

```bash
cd extension
bash build.sh
```

Make sure Wasmtime dynamic library is available in the system.

## Benchmarking against vanilla PHP

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
