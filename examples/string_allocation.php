<?php

// Create a Wasm engine/runtime
$engine = new WasmEngine();
echo "WasmEngine instance created\n";

// Load the WebAssembly module
$module = new WasmModule($engine, __DIR__ . "/substring.wasm");
echo "Module substring.wasm loaded\n";

// Instantiate the module
$instance = new WasmInstance($engine, $module, [
    'env.abort' => function (int $msg, int $file, int $line, int $col) {
        echo "[abort] msg=$msg file=$file line=$line col=$col\n";
        throw new Exception("WASM aborted");
    },
    'console.log' => function (int $data) {
        echo "[console.log] data=$data\n";
        return 0;
    }
]);
echo "WasmInstance created\n";

// Get the memory from the instance
$memory = $instance->getMemory();
echo "Memory size: " . $memory->size() . " pages (" . $memory->dataSize() . " bytes)\n";

// Allocate some strings in WebAssembly memory
$str1 = "Hello from PHP!";
$str1 = mb_convert_encoding($str1, 'UTF-16', 'UTF-8') . "\0";
$ptr1 = $memory->allocate(strlen($str1));
$memory->write($ptr1, $str1);
echo "Allocated string 1 at offset $ptr1: '$str1'\n";

// $str2 = "WebAssembly is awesome";
// $ptr2 = $memory->allocateString($str2);
// echo "Allocated string 2 at offset $ptr2: '$str2'\n";

// Read back the strings from memory to verify
$readStr1 = $memory->read($ptr1, strlen($str1));
echo "Read string 1 from memory: '$readStr1'\n";

$number_result_pointer = $instance->call("hello_number", [15]);
echo "Number result pointer: $number_result_pointer\n";
var_dump($memory->read($number_result_pointer, 30));

// $result_pointer = $instance->call("hello", [$number_result_pointer]);
// var_dump($memory->read($result_pointer, 30));

die('----');
// Allocate a longer string to test memory growth if needed
$longStr = str_repeat("This is a very long string that might cause memory to grow. ", 10);
$ptrLong = $memory->allocate($longStr);
echo "Allocated long string at offset $ptrLong (length: " . strlen($longStr) . ")\n";
echo "Memory size after allocation: " . $memory->size() . " pages (" . $memory->dataSize() . " bytes)\n";

// You could now pass these pointers to WebAssembly functions that expect string pointers
// For example:
// $result = $instance->call("process_string", [$ptr1]);

echo "String allocation test completed.\n"; 