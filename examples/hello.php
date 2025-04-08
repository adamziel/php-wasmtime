<?php

// Create a Wasm engine/runtime
$engine = new WasmEngine();
echo "WasmEngine instance created\n";
// Load two modules
$moduleA = new WasmModule($engine, __DIR__ . "/hello.wasm");  // exports a function `add` and a memory
echo "Module hello.wasm loaded\n";

// Instantiate moduleA (no imports needed if it doesn't import anything)
// $memory = new WasmMemory($engine, 1024);
$instanceA = new WasmInstance($engine, $moduleA);

echo "WasmInstance created\n";
$result_pointer = $instanceA->call("hello", []);        
echo "Result of hello: $result_pointer\n";

$memory = $instanceA->getMemory();
var_dump('Got memory instance');
var_dump($memory->read($result_pointer, 10));

echo "Done\n";
