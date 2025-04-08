<?php

$engine = new WasmEngine();
echo "WasmEngine instance created\n";

$moduleA = new WasmModule($engine, __DIR__ . "/substring.wasm");
echo "Module substring.wasm loaded\n";

// Instantiate moduleA (no imports needed if it doesn't import anything)
$instanceA = new WasmInstance($engine, $moduleA);

echo "WasmInstance created\n";

echo "Calling getMemory...\n";
$memory = $instanceA->getMemory('memory'); // Crash seems to happen after this line returns
echo "getMemory returned. Type: " . gettype($memory) . "\n"; // Check if $memory is an object
if (is_object($memory)) {
    echo "Memory object class: " . get_class($memory) . "\n";
    echo "Checking memory validity...\n";
    // Try a simple operation immediately
    try {
        $size = $memory->dataSize();
        echo "Memory dataSize(): " . $size . "\n";
    } catch (Throwable $e) {
        echo "Error calling dataSize(): " . $e->getMessage() . "\n";
    }
} else {
    echo "getMemory did not return an object.\n";
}

echo "Proceeding with memory operations...\n";

$string_pointer = $instanceA->call("malloc", [10]);
$memory->write($string_pointer, "Dogs are cute");

$result_pointer = $instanceA->call("substring", [$string_pointer, 0, 8]);
var_dump($result_pointer);
var_dump($memory->read($result_pointer, 8));

echo "Done!\n";


