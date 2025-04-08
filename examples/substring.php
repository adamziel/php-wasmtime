<?php

// Create a Wasm engine/runtime
$engine = new WasmEngine();
echo "WasmEngine instance created\n";
// Load two modules
$moduleA = new WasmModule($engine, __DIR__ . "/substring.wasm");  // exports a function `add` and a memory
echo "Module substring.wasm loaded\n";

// Instantiate moduleA (no imports needed if it doesn't import anything)
$instanceA = new WasmInstance($engine, $moduleA, [
	'env.abort' => function (int $msg, int $file, int $line, int $col) {
		echo "[abort] msg=$msg file=$file line=$line col=$col\n";
		throw new Exception("WASM aborted");
	},
	'console.log' => function (int $data_pointer) {
		echo "Calling a console log! :)\n";
		global $memory;
		$data = $memory->read($data_pointer, 10);
		echo "Data: $data\n";
		return 0;
	}
]);

echo "WasmInstance created\n";
// $result_pointer = $instanceA->call("hello", ["Hello, world!"]);
// var_dump($memory->read($result_pointer, 10));

$memory = $instanceA->getMemory();
$result_pointer = $instanceA->call("hello_number", [15]);
var_dump($memory->read($result_pointer, 30));

// $result = $instanceA->call("substring", ["Hello, world!", 0, 6]);        
echo "Result done\n";


