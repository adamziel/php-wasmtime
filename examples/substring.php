<?php

$engine = new WasmEngine();
echo "WasmEngine instance created\n";

$moduleA = new WasmModule($engine, __DIR__ . "/substring.wasm");
echo "Module substring.wasm loaded\n";

// Instantiate moduleA (no imports needed if it doesn't import anything)
$instanceA = new WasmInstance($engine, $moduleA, [
	'1' => 'a',
	'2' => 'b',
	// No SIGSEGV if this line is used instead:
	// '2' => function () { }
]);

echo "WasmInstance created\n";

$memory = $instanceA->getMemory();
echo "Memory instance created\n";
$string_pointer = $instanceA->call("malloc", [10]);
$memory->write($string_pointer, "Dogs are cute");

$result_pointer = $instanceA->call("substring", [$string_pointer, 0, 8]);
var_dump($result_pointer);
var_dump($memory->read($result_pointer, 8));

echo "Done!\n";


