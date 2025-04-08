<?php

/**
 * Exported WASM functions:
 * 
 * (export "memory" (memory 0))
 * (export "__wbindgen_describe___wbg_log_c788c5626f3f9c3a" (func $__wbindgen_describe___wbg_log_c788c5626f3f9c3a))
 * (export "__wbg_wp_html_tag_processor_free" (func $__wbg_wp_html_tag_processor_free))
 * (export "wp_html_tag_processor_new" (func $wp_html_tag_processor_new))
 * (export "__wbindgen_describe_wp_html_tag_processor_new" (func $__wbindgen_describe_wp_html_tag_processor_new))
 * (export "wp_html_tag_processor_is_tag_closer" (func $wp_html_tag_processor_is_tag_closer))
 * (export "wp_html_tag_processor_get_updated_html" (func $wp_html_tag_processor_get_updated_html))
 * (export "__wbindgen_describe_wp_html_tag_processor_get_updated_html" (func $__wbindgen_describe_wp_html_tag_processor_get_updated_html))
 * (export "wp_html_tag_processor_next_token" (func $wp_html_tag_processor_next_token))
 * (export "wp_html_tag_processor_get_tag" (func $wp_html_tag_processor_get_tag))
 * (export "wp_html_tag_processor_get_token_type" (func $wp_html_tag_processor_get_token_type))
 * (export "wp_html_tag_processor_get_token_name" (func $wp_html_tag_processor_get_token_name))
 * (export "wp_html_tag_processor_get_modifiable_text" (func $wp_html_tag_processor_get_modifiable_text))
 * (export "__wbg_wp_html_processor_free" (func $__wbg_wp_html_processor_free))
 * (export "wp_html_processor_create_full_parser" (func $wp_html_processor_create_full_parser))
 * (export "__wbindgen_describe_wp_html_processor_create_full_parser" (func $__wbindgen_describe_wp_html_processor_create_full_parser))
 * (export "wp_html_processor_is_tag_closer" (func $wp_html_processor_is_tag_closer))
 * (export "__wbindgen_describe_wp_html_processor_is_tag_closer" (func $__wbindgen_describe_wp_html_processor_is_tag_closer))
 * (export "wp_html_processor_next_token" (func $wp_html_processor_next_token))
 * (export "wp_html_processor_get_tag" (func $wp_html_processor_get_tag))
 * (export "__wbindgen_describe_wp_html_processor_get_tag" (func $__wbindgen_describe_wp_html_processor_get_tag))
 * (export "wp_html_processor_get_token_type" (func $wp_html_processor_get_token_type))
 * (export "wp_html_processor_get_token_name" (func $wp_html_processor_get_token_name))
 * (export "wp_html_processor_get_attribute" (func $wp_html_processor_get_attribute))
 * (export "__wbindgen_describe_wp_html_processor_get_attribute" (func $__wbindgen_describe_wp_html_processor_get_attribute))
 * (export "wp_html_processor_class_list" (func $wp_html_processor_class_list))
 * (export "__wbindgen_describe_wp_html_processor_class_list" (func $__wbindgen_describe_wp_html_processor_class_list))
 * (export "wp_html_processor_get_modifiable_text" (func $wp_html_processor_get_modifiable_text))
 * (export "__wbindgen_describe_wp_html_processor_get_modifiable_text" (func $__wbindgen_describe_wp_html_processor_get_modifiable_text))
 * (export "wp_html_processor_get_last_error" (func $wp_html_processor_get_last_error))
 * (export "__wbindgen_describe_wp_html_processor_get_last_error" (func $__wbindgen_describe_wp_html_processor_get_last_error))
 * (export "wp_html_processor_get_breadcrumbs" (func $wp_html_processor_get_breadcrumbs))
 * (export "__wbindgen_describe_wp_html_tag_processor_get_token_name" (func $__wbindgen_describe_wp_html_processor_get_tag))
 * (export "__wbindgen_describe_wp_html_tag_processor_get_tag" (func $__wbindgen_describe_wp_html_processor_get_tag))
 * (export "__wbindgen_describe_wp_html_tag_processor_get_token_type" (func $__wbindgen_describe_wp_html_processor_get_last_error))
 * (export "__wbindgen_describe_wp_html_processor_get_token_name" (func $__wbindgen_describe_wp_html_processor_get_tag))
 * (export "__wbindgen_describe_wp_html_processor_get_token_type" (func $__wbindgen_describe_wp_html_processor_get_last_error))
 * (export "__wbindgen_describe_wp_html_tag_processor_next_token" (func $__wbindgen_describe_wp_html_processor_is_tag_closer))
 * (export "__wbindgen_describe_wp_html_tag_processor_is_tag_closer" (func $__wbindgen_describe_wp_html_processor_is_tag_closer))
 * (export "__wbindgen_describe_wp_html_processor_next_token" (func $__wbindgen_describe_wp_html_processor_is_tag_closer))
 * (export "__wbindgen_describe_wp_html_tag_processor_get_modifiable_text" (func $__wbindgen_describe_wp_html_processor_get_modifiable_text))
 * (export "__wbindgen_describe_wp_html_processor_get_breadcrumbs" (func $__wbindgen_describe_wp_html_processor_class_list))
 * (export "__wbindgen_malloc" (func $__wbindgen_malloc))
 * (export "__wbindgen_realloc" (func $__wbindgen_realloc))
 * (export "__wbindgen_free" (func $__wbindgen_free))
 * (export "__wbindgen_exn_store" (func $__wbindgen_exn_store))
 * (export "__externref_table_alloc" (func $__externref_table_alloc))
 * (export "__externref_table_dealloc" (func $__externref_table_dealloc))
 * (export "__externref_drop_slice" (func $__externref_drop_slice))
 * (export "__externref_heap_live_count" (func $__externref_heap_live_count))
 * (export "__data_end" (global 1))
 * (export "__heap_base" (global 2))
 */

$engine = new WasmEngine();
// echo "WasmEngine instance created\n";

$moduleA = new WasmModule($engine, __DIR__ . "/wp_html_api_wasm.wasm");
// echo "Module substring.wasm loaded\n";

$wasm = new WasmInstance($engine, $moduleA, [
	/**
	 * @TODO: Why does the Rust module produce these imports? They don't seem to be used.
	 *        Let's put die() in each to get an alert when they are called.
	 */
	'__wbindgen_placeholder__.__wbindgen_describe' => function () { die("__wbindgen_describe\n"); },
	'__wbindgen_placeholder__.__wbindgen_string_new' => function () { die("__wbindgen_string_new\n"); },
	'__wbindgen_placeholder__.__wbindgen_throw' => function () { die("__wbindgen_throw\n"); },
	'__wbindgen_placeholder__.__wbindgen_uint8_array_new' => function () { die("__wbindgen_uint8_array_new\n"); },
	'__wbindgen_externref_xform__.__wbindgen_externref_table_grow' => function () { die("__wbindgen_externref_table_grow\n"); },
	'__wbindgen_externref_xform__.__wbindgen_externref_table_set_null' => function () { die("__wbindgen_externref_table_set_null\n"); },
]);
// echo "WasmInstance created\n";

// echo "Calling getMemory...\n";
$memory = $wasm->getMemory('memory');
// echo "getMemory returned. Type: " . gettype($memory) . "\n";

// echo "Proceeding with memory operations...\n";

/**
 * Class WPHtmlTagProcessor
 * A PHP wrapper for the WebAssembly HTML Tag Processor
 */
class WPHtmlTagProcessor {
    private $wasm;
    private $memory;
    private $processor_pointer;
    
    /**
     * Constructor
     * 
     * @param WasmInstance $wasm The WebAssembly instance
     * @param string $html The HTML to process
     */
    public function __construct(WasmInstance $wasm, string $html) {
        $this->wasm = $wasm;
        $this->memory = $wasm->getMemory('memory');
        
        // Allocate memory for the HTML string
        $string_pointer = $this->allocateString($html);
        
        // Create a new tag processor
        $this->processor_pointer = $wasm->call("wp_html_tag_processor_new", [
            $string_pointer,
            strlen($html),
        ]);
    }
    
    /**
     * Move to the next token in the HTML
     * 
     * @return bool True if a token was found, false otherwise
     */
    public function nextToken(): bool {
        $result = $this->wasm->call("wp_html_tag_processor_next_token", [$this->processor_pointer]);
        return $result === 1;
    }
    
    /**
     * Get the current tag name
     * 
     * @return string The tag name
     */
    public function getTag(): string {
        // Allocate memory for the return pointer (address + length)
        $ret_ptr_loc = $this->wasm->call("__wbindgen_malloc", [8, 4]); // 8 bytes, align 4 for two i32s
        
        // Get the tag name
        $this->wasm->call("wp_html_tag_processor_get_tag", [$ret_ptr_loc, $this->processor_pointer]);
        
        // Read the string from memory
        $tag_name = $this->readStringFromPointer($ret_ptr_loc);
        
        // Free the memory allocated for the return pointer
        $this->wasm->call("__wbindgen_free", [$ret_ptr_loc, 8, 4]);
        
        return $tag_name;
    }

    /**
     * Get the modifiable text content
     * 
     * @return string The modifiable text content
     */
    public function getModifiableText(): string {
        // Allocate memory for the return pointer (address + length)
        $ret_ptr_loc = $this->wasm->call("__wbindgen_malloc", [8, 4]); // 8 bytes, align 4 for two i32s
        
        // Get the modifiable text
        $this->wasm->call("wp_html_tag_processor_get_modifiable_text", [$ret_ptr_loc, $this->processor_pointer]);
        
        // Read the string from memory
        $text = $this->readStringFromPointer($ret_ptr_loc);
        
        // Free the memory allocated for the return pointer
        $this->wasm->call("__wbindgen_free", [$ret_ptr_loc, 8, 4]);
        
        return $text;
    }
    
    /**
     * Check if the current tag is a closing tag
     * 
     * @return bool True if the tag is a closing tag, false otherwise
     */
    public function isTagCloser(): bool {
        $result = $this->wasm->call("wp_html_tag_processor_is_tag_closer", [$this->processor_pointer]);
        return $result === 1;
    }
    
    /**
     * Get the token type
     * 
     * @return int The token type
     */
    public function getTokenType(): string {
        // Allocate memory for the return pointer (address + length)
        $ret_ptr_loc = $this->wasm->call("__wbindgen_malloc", [8, 4]); // 8 bytes, align 4 for two i32s
        
        // Get the token type
        $this->wasm->call("wp_html_tag_processor_get_token_type", [$ret_ptr_loc, $this->processor_pointer]);
        
        // Read the value from memory
        $token_type = $this->readStringFromPointer($ret_ptr_loc);
        
        // Free the memory allocated for the return pointer
        $this->wasm->call("__wbindgen_free", [$ret_ptr_loc, 8, 4]);
        
        return $token_type;
    }

    /**
     * Allocate memory for a string and write it to memory
     * 
     * @param string $str The string to allocate
     * @return int Pointer to the allocated string
     */
    private function allocateString(string $str): int {
        $string_pointer = $this->wasm->call("__wbindgen_malloc", [
            strlen($str),
            4 /* Use alignment 4 */
        ]);
        $this->memory->write($string_pointer, $str);
        return $string_pointer;
    }
    
    /**
     * Read a string from memory using a pointer to a pointer+length pair
     * 
     * @param int $ptr_loc Pointer to the pointer+length pair
     * @return string The string read from memory
     */
    private function readStringFromPointer(int $ptr_loc): string {
        // Read the actual string pointer from the return location
        $string_ptr_bytes = $this->memory->read($ptr_loc, 4);
        $actual_string_ptr = unpack("I", $string_ptr_bytes)[1];
        
        // Read the string length from the return location (immediately after the pointer)
        $string_len_bytes = $this->memory->read($ptr_loc + 4, 4);
        $string_len = unpack("I", $string_len_bytes)[1];
        
        // Read the string from Wasm memory using the pointer and length
        return $this->memory->read($actual_string_ptr, $string_len);
    }
}
require_once __DIR__ . "/utils.php";

$html = file_get_contents(__DIR__ . "/html_spec.html");
echo "\n\033[1mBenchmarking WASM implementation...\033[0m\n\n";
benchmark("Counting tokens", function () use ($html, $wasm) {
	$nb_tokens = 0;
	$processor = new WPHtmlTagProcessor($wasm, $html);
	while ($processor->nextToken()) {
		$nb_tokens++;
	}
	echo "found $nb_tokens tokens";
});

benchmark("Getting token details", function () use ($html, $wasm) {
	$processor = new WPHtmlTagProcessor($wasm, $html);
	while ($processor->nextToken()) {
		switch ($processor->getTokenType()) {
			case '#tag':
				$tag_name = $processor->getTag();
				$is_closer = $processor->isTagCloser() ? "closing" : "opening";
				// echo "Found $is_closer tag: $tag_name\n";
				break;
			case '#text':
				$text = $processor->getModifiableText();
				// echo "Found text: $text\n";
				break;
		}
	}
});
