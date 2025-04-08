<?php

function benchmark(string $name, callable $func) {
	echo "$name ";
	$start_time = microtime(true);
	$func();
	$end_time = microtime(true);
	$execution_time = ($end_time - $start_time);
	echo " [" . number_format($execution_time, 4) . "s]\n";
}