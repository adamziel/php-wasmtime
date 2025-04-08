<?php

require_once __DIR__ . "/html-api/class-wp-token-map.php";
require_once __DIR__ . "/html-api/html5-named-character-references.php";
require_once __DIR__ . "/html-api/class-wp-html-decoder.php";

require_once __DIR__ . "/html-api/class-wp-html-span.php";
require_once __DIR__ . "/html-api/class-wp-html-attribute-token.php";
require_once __DIR__ . "/html-api/class-wp-html-token.php";
require_once __DIR__ . "/html-api/class-wp-html-tag-processor.php";

require_once __DIR__ . "/utils.php";

$html = file_get_contents(__DIR__ . "/html_spec.html");
echo "\n\033[1mBenchmarking PHP implementation...\033[0m\n\n";
benchmark("Counting tokens", function () use ($html) {
	$nb_tokens = 0;
	$processor = new WP_HTML_Tag_Processor($html);
	while ($processor->next_token()) {
		$nb_tokens++;
	}
	echo "found $nb_tokens tokens";
});

benchmark("Getting token details", function () use ($html) {
	$processor = new WP_HTML_Tag_Processor($html);
	while ($processor->next_token()) {
		switch ($processor->get_token_type()) {
			case '#tag':
				$tag_name = $processor->get_tag();
				$is_closer = $processor->is_tag_closer() ? "closing" : "opening";
				// echo "Found $is_closer tag: $tag_name\n";
				break;
			case '#text':
				$text = $processor->get_modifiable_text();
				// echo "Found text: $text\n";
				break;
		}
	}
});
