<?php

$engine = new WasmEngine();
echo "WasmEngine instance created\n";

$moduleA = new WasmModule($engine, __DIR__ . "/substring.wasm");
echo "Module substring.wasm loaded\n";

$instanceA = new WasmInstance($engine, $moduleA);
echo "WasmInstance created\n";

echo "Calling getMemory...\n";

$memory = $instanceA->getMemory('memory');
echo "getMemory returned. Type: " . gettype($memory) . "\n";

echo "Proceeding with memory operations...\n";

$string_pointer = $instanceA->call("malloc", [10]);
$memory->write($string_pointer, "Dogs are cute");

$result_pointer = $instanceA->call("substring", [$string_pointer, 0, 8]);
var_dump($result_pointer);
var_dump($memory->read($result_pointer, 8));

echo "Done!\n";


