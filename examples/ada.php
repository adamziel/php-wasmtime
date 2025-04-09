<?php

// Create a Wasm engine/runtime
$engine = new WasmEngine();
echo "WasmEngine instance created\n";

// Load the module
$module = new WasmModule($engine, __DIR__ . "/ada.wasm");
echo "Module ada.wasm loaded\n";

// Instantiate the module WITH providing the full list of imports
$instance = new WasmInstance($engine, $module, [
	// (import "wasi_snapshot_preview1" "environ_get" (func $__imported_wasi_snapshot_preview1_environ_get (type 5)))
	'wasi_snapshot_preview1.environ_get' => function() { die('line: '.__LINE__); return 0; },
	// (import "wasi_snapshot_preview1" "environ_sizes_get" (func $__imported_wasi_snapshot_preview1_environ_sizes_get (type 5)))
	/**
	 * The environ_sizes_get() function is used to retrieve the sizes of the environment variable data.
	 * It returns the number of environment variables and the size of the environment variable string data.
	 */
	'wasi_snapshot_preview1.environ_sizes_get' => function() { return 0; },

	// (import "wasi_snapshot_preview1" "fd_close" (func $__imported_wasi_snapshot_preview1_fd_close (type 6)))
	'wasi_snapshot_preview1.fd_close' => function() { die('line: '.__LINE__); return 0; },
	// (import "wasi_snapshot_preview1" "fd_read" (func $__imported_wasi_snapshot_preview1_fd_read (type 4)))
	'wasi_snapshot_preview1.fd_read' => function() { die('line: '.__LINE__); return 0; },
	// (import "wasi_snapshot_preview1" "fd_seek" (func $__imported_wasi_snapshot_preview1_fd_seek (type 31)))
	'wasi_snapshot_preview1.fd_seek' => function() { die('line: '.__LINE__); return 0; },
	// (import "wasi_snapshot_preview1" "fd_write" (func $__imported_wasi_snapshot_preview1_fd_write (type 4)))
	'wasi_snapshot_preview1.fd_write' => function() { die('line: '.__LINE__); return 0; },
]);

$memory = $instance->getMemory('memory');

require_once __DIR__ . "/utils.php";

benchmark("Parsed 1,000,000 URLs in", function() use ($instance, $memory) {
	// Define a URL to write to memory
	$url = "https://example.com/api";
	$url_length = strlen($url);
	// Allocate memory for the URL
	// Use malloc to allocate memory for the URL string
	$url_ptr = $instance->call("malloc", [$url_length + 1]); // +1 for null terminator
	// Write the URL to memory
	$memory->write($url_ptr, $url);

	for($i = 0; $i < 1000000; $i++) {
		// Call the get_host function with the pointer to the URL
		$result_ptr = $instance->call("get_host", [$url_ptr]);

		// Read the result from memory
		// We don't know the exact length, so we'll read until null terminator or a reasonable length
		$max_length = 100;
		$result = $memory->read($result_ptr, $max_length);

		// echo "Host from URL: $result\n";
	}
});
