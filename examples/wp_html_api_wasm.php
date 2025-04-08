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
echo "WasmEngine instance created\n";

$moduleA = new WasmModule($engine, __DIR__ . "/wp_html_api_wasm.wasm");
echo "Module substring.wasm loaded\n";

$wasm = new WasmInstance($engine, $moduleA, [
	/**
	 * @TODO: Why does the Rust module produce these imports?
	 */
	'__wbindgen_placeholder__.__wbindgen_describe' => function () {
		die("__wbindgen_describe\n");
	},
	'__wbindgen_placeholder__.__wbindgen_string_new' => function () {
		die("__wbindgen_string_new\n");
	},
	'__wbindgen_placeholder__.__wbindgen_throw' => function () {//int $ptr, int $len) {
		die("__wbindgen_throw\n");
	},
	'__wbindgen_placeholder__.__wbindgen_uint8_array_new' => function () {//int $ptr, int $len) {
		die("__wbindgen_uint8_array_new\n");
		// return 0;
	},
	'__wbindgen_externref_xform__.__wbindgen_externref_table_grow' => function () {//int $delta) {
		die("__wbindgen_externref_table_grow\n");
		// return 0;
	},
	'__wbindgen_externref_xform__.__wbindgen_externref_table_set_null' => function () {//int $idx) {
		die("__wbindgen_externref_table_set_null\n");
	}
]);
echo "WasmInstance created\n";

echo "Calling getMemory...\n";

$memory = $wasm->getMemory('memory');
echo "getMemory returned. Type: " . gettype($memory) . "\n";

echo "Proceeding with memory operations...\n";

$html = "<p><div>Hello, world!</div></p>";
$string_pointer = $wasm->call("__wbindgen_malloc", [
	strlen($html),
	1 /* alignment. Why? I don't know! Seems non-standard. */
]);
$memory->write($string_pointer, $html);

$processor_pointer = $wasm->call("wp_html_tag_processor_new", [
	$string_pointer,
	strlen($html),
]);

$result = $wasm->call("wp_html_tag_processor_next_token", [$processor_pointer]);
assert($result === 1, "wp_html_tag_processor_next_token returned 0 – no token found");

$result_pointer = $wasm->call("__wbindgen_malloc", [
	10,
	1
]);

echo "Calling wp_html_processor_get_tag...\n";
$wasm->call("wp_html_processor_get_tag", [$processor_pointer, $result_pointer]);
echo "wp_html_processor_get_tag returned\n";
$tag_name = $memory->read($result_pointer, 1);
var_dump($tag_name);
