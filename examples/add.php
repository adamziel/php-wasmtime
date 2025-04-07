<?php

// Create a Wasm engine/runtime
$engine = new WasmEngine();
echo "WasmEngine instance created\n";
// Load two modules
$moduleA = new WasmModule($engine, __DIR__ . "/hello.wasm");  // exports a function `add` and a memory
echo "Module A loaded\n";

// Instantiate moduleA (no imports needed if it doesn't import anything)
$instanceA = new WasmInstance($engine, $moduleA);

echo "Instance A loaded\n";
// Get exports from A to use as imports for B
$result = $instanceA->call("add", [5, 7]);        // suppose moduleA exports an `add(int,int)->int` function
echo "Result of add: $result\n";

// Get the size of the memory

