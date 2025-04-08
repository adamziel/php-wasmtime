## PHP-WasmTime extension

Building and installing the extension:

```bash
cd extension
bash build.sh
```

Make sure Wasmtime dynamic library is available in the system.

## Running the examples

```bash
> php examples/wp_html_api_wasm.php                                                                       (base) 
WasmEngine instance created
Module substring.wasm loaded
WasmInstance created
Calling getMemory...
getMemory returned. Type: object
Proceeding with memory operations...
string(4) "#tag"
Found opening tag: P
string(4) "#tag"
Found opening tag: DIV
string(5) "#text"
Found text: Hello, world!
string(4) "#tag"
Found closing tag: DIV
string(4) "#tag"
Found closing tag: P
```
