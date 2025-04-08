<?php

// Create a Wasm engine/runtime
$engine = new WasmEngine();
echo "WasmEngine created\n";

// Load the WebAssembly module
$module = new WasmModule($engine, __DIR__ . "/substring.wasm");
echo "Module loaded\n";

// Instantiate the module with no imports
$instance = new WasmInstance($engine, $module);
echo "Instance created\n";

// Get the memory from the instance
$memory = $instance->getMemory();
echo "Memory retrieved\n";

// Check memory size
echo "Memory size: " . $memory->size() . " pages (" . $memory->dataSize() . " bytes)\n";

// Try reading and writing to memory
$testData = "Hello WebAssembly";
$memory->write(0, $testData);
echo "Data written\n";

$readData = $memory->read(0, strlen($testData));
echo "Read data: '$readData'\n";

// Done
echo "Test completed\n"; 