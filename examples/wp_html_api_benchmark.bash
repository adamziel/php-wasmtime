#!/bin/bash

php -d opcache.enable_cli=true -d opcache.jit=on -d opcache.jit_buffer_size=50M wp_html_api_wasm.php
php -d opcache.enable_cli=true -d opcache.jit=on -d opcache.jit_buffer_size=50M wp_html_api_php.php

