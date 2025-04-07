/*--------------- File: php_wasmtime.h ---------------*/
#ifndef PHP_WASMTIME_H
#define PHP_WASMTIME_H

#ifdef ZTS
#include "TSRM.h"
#endif

#include "php.h"
#include "zend_exceptions.h"
#include <wasm.h>
#include <wasmtime.h>


/* Declare extension globals if needed */
ZEND_BEGIN_MODULE_GLOBALS(wasmtime)
ZEND_END_MODULE_GLOBALS(wasmtime)

extern zend_module_entry wasmtime_module_entry;
#define phpext_wasmtime_ptr &wasmtime_module_entry

/* Helper macros for fetching our objects by pointer */
typedef struct _php_wasm_engine_t {
    zend_object std;
    wasm_engine_t      *engine;
    wasmtime_store_t   *store;
} php_wasm_engine_t;

static inline php_wasm_engine_t *Z_WASMENGINE_P(zval *zv) {
    return (php_wasm_engine_t*)((char*)(Z_OBJ_P(zv)) - XtOffsetOf(php_wasm_engine_t, std));
}

typedef struct _php_wasm_module_t {
    zend_object std;
    wasmtime_module_t *module;
    php_wasm_engine_t *engine_obj;
} php_wasm_module_t;

static inline php_wasm_module_t *Z_WASMMODULE_P(zval *zv) {
    return (php_wasm_module_t*)((char*)(Z_OBJ_P(zv)) - XtOffsetOf(php_wasm_module_t, std));
}

typedef struct _php_wasm_instance_t {
    zend_object std;
    wasmtime_instance_t instance;
    php_wasm_engine_t  *engine_obj;
} php_wasm_instance_t;

static inline php_wasm_instance_t *Z_WASMINSTANCE_P(zval *zv) {
    return (php_wasm_instance_t*)((char*)(Z_OBJ_P(zv)) - XtOffsetOf(php_wasm_instance_t, std));
}

typedef struct _php_wasm_memory_t {
    zend_object std;
    wasmtime_memory_t memory;
    php_wasm_engine_t *engine_obj;
} php_wasm_memory_t;

static inline php_wasm_memory_t *Z_WASMMEMORY_P(zval *zv) {
    return (php_wasm_memory_t*)((char*)(Z_OBJ_P(zv)) - XtOffsetOf(php_wasm_memory_t, std));
}

typedef struct _php_wasm_global_t {
    zend_object std;
    wasmtime_global_t global;
    php_wasm_engine_t *engine_obj;
} php_wasm_global_t;

static inline php_wasm_global_t *Z_WASMGLOBAL_P(zval *zv) {
    return (php_wasm_global_t*)((char*)(Z_OBJ_P(zv)) - XtOffsetOf(php_wasm_global_t, std));
}

/* Class entry pointers */
extern zend_class_entry *wasm_engine_ce;
extern zend_class_entry *wasm_module_ce;
extern zend_class_entry *wasm_instance_ce;
extern zend_class_entry *wasm_memory_ce;
extern zend_class_entry *wasm_global_ce;

/* Standard module functions */
PHP_MINIT_FUNCTION(wasmtime);
PHP_MSHUTDOWN_FUNCTION(wasmtime);
PHP_RINIT_FUNCTION(wasmtime);
PHP_RSHUTDOWN_FUNCTION(wasmtime);
PHP_MINFO_FUNCTION(wasmtime);

#endif /* PHP_WASMTIME_H */
