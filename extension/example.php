<?php
// Create a Wasm engine/runtime
$engine = new WasmEngine();

// Load two modules
$moduleA = new WasmModule($engine, "moduleA.wasm");  // exports a function `add` and a memory
$moduleB = new WasmModule($engine, "moduleB.wasm");  // imports a function `add` and a memory

// Instantiate moduleA (no imports needed if it doesn't import anything)
$instanceA = new WasmInstance($engine, $moduleA);

// Get exports from A to use as imports for B
$addFunc = $instanceA->getFunction("add");        // suppose moduleA exports an `add(int,int)->int` function
$memory  = $instanceA->getMemory("memory");       // and exports its memory

// Instantiate moduleB, supplying imports (the add function and memory from moduleA)
$instanceB = new WasmInstance($engine, $moduleB, [
    "add"    => $addFunc,
    "memory" => $memory
]);

// Now call a function from moduleB that uses the imported functionality
$result = $instanceB->call("compute", [5, 7]);
echo "Result of compute: $result\n";  // e.g., expected 12 if compute calls add(5,7)

// We can directly access and manipulate the shared memory as well (memory is shared between A and B).
$memSize = $memory->dataSize();
$data = $memory->read(0, 16);
echo "First 16 bytes of memory: ", bin2hex($data), "\n";

// Write a string into memory at offset 32
$memory->write(32, "Hello from PHP");
// Perhaps moduleB has a function to print from memory, which will now see this new data.

// Provide a PHP function import to a module (e.g., moduleC wants an import to output messages)
$moduleC = new WasmModule($engine, "moduleC.wasm");  // imports a function "log(i32)"
$instanceC = new WasmInstance($engine, $moduleC, [
    "log" => function(int $n) {  // this PHP closure will be called from WASM
        echo "[WASM] Log called with ", $n, "\n";
    }
]);

// Call a function in moduleC that triggers the log callback
$instanceC->call("triggerLog", [42]);
?>
