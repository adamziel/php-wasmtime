<?php

// Create a Wasm engine/runtime
$engine = new WasmEngine();
echo "WasmEngine instance created\n";
// Load two modules
$moduleA = new WasmModule($engine, __DIR__ . "/add_i8.wasm");  // exports a function `add` and a memory
echo "Module add_i8.wasm loaded\n";

// Instantiate moduleA (no imports needed if it doesn't import anything)
$instanceA = new WasmInstance($engine, $moduleA);

echo "WasmInstance created\n";
// module add.wasm exports an `add(int,int)->int` function
$result = $instanceA->call("add", [50, 7]);        
echo "Result of add: $result\n";


