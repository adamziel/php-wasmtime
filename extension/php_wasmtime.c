/*--------------- File: php_wasmtime.c ---------------*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "./php_wasmtime.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Define a debug logging macro
#ifdef PHP_WASMTIME_DEBUG
#define PHP_WASMTIME_DEBUG_LOG(...) fprintf(stderr, __VA_ARGS__)
#else
#define PHP_WASMTIME_DEBUG_LOG(...) // No-op when not debugging
#endif

/* Globals for class entries */
zend_class_entry *wasm_engine_ce;
zend_class_entry *wasm_module_ce;
zend_class_entry *wasm_instance_ce;
zend_class_entry *wasm_memory_ce;
zend_class_entry *wasm_global_ce;

/* Object handlers */
static zend_object_handlers wasm_engine_object_handlers;
static zend_object_handlers wasm_module_object_handlers;
static zend_object_handlers wasm_instance_object_handlers;
static zend_object_handlers wasm_memory_object_handlers;
static zend_object_handlers wasm_global_object_handlers;

/* Helper to read file into memory */
static size_t read_wasm_file(const char *filename, uint8_t **buffer) {
    FILE *f = fopen(filename, "rb");
    if(!f) {
        // Keep error logging even without debug flag for critical errors
        fprintf(stderr, "read_wasm_file: Failed to open file %s\n", filename);
        return 0;
    }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    if(size <= 0) {
        fprintf(stderr, "read_wasm_file: Invalid file size %ld for %s\n", size, filename);
        fclose(f);
        return 0;
    }
    *buffer = emalloc(size);
    if(!*buffer) {
        fprintf(stderr, "read_wasm_file: emalloc failed for %ld bytes\n", size);
        fclose(f);
        return 0;
    }
    size_t read_bytes = fread(*buffer, 1, size, f);
    fclose(f);
    if (read_bytes != (size_t)size) {
         // Keep warning logging even without debug flag
         fprintf(stderr, "read_wasm_file: WARNING - fread read %zu bytes but expected %ld\n", read_bytes, size);
         // It might be safer to treat this as an error
         // efree(*buffer);
         // *buffer = NULL;
         // return 0;
         // However, for now, we continue and return read_bytes, letting wasmtime handle the partial data.
    }
    PHP_WASMTIME_DEBUG_LOG("read_wasm_file: Successfully read %zu bytes from %s\n", read_bytes, filename);
    return read_bytes;
}

/*-------------------------------------------
  ENGINE (WasmEngine)
--------------------------------------------*/
static zend_object* wasm_engine_create_object(zend_class_entry *class_type) {
	PHP_WASMTIME_DEBUG_LOG("wasm_engine_create_object\n");
    // Allocate only the size of our struct
    php_wasm_engine_t *intern = ecalloc(1, sizeof(php_wasm_engine_t));
    zend_object_std_init(&intern->std, class_type);
    // No properties_init needed if no dynamic/declared properties
    intern->std.handlers = &wasm_engine_object_handlers;
    return &intern->std;
}

PHP_METHOD(WasmEngine, __construct)
{
    PHP_WASMTIME_DEBUG_LOG("constructor initiated\n");
    php_wasm_engine_t *engine_obj = (php_wasm_engine_t *) Z_OBJ_P(getThis());
    PHP_WASMTIME_DEBUG_LOG("wasm engine object created\n");
    engine_obj->engine = wasm_engine_new();
	PHP_WASMTIME_DEBUG_LOG("wasm engine created\n");
    if (!engine_obj->engine) {
        zend_throw_exception(NULL, "Failed to create wasm_engine_t", 0);
        return;
    }
	PHP_WASMTIME_DEBUG_LOG("wasm engine store created\n");
    engine_obj->store = wasmtime_store_new(engine_obj->engine, NULL, NULL);
	PHP_WASMTIME_DEBUG_LOG("wasm engine store created\n");
    if (!engine_obj->store) {
        zend_throw_exception(NULL, "Failed to create wasmtime_store_t", 0);
        return;
    }
    PHP_WASMTIME_DEBUG_LOG("WasmEngine::__construct - Engine object: %p, refcount: %d\n", engine_obj, GC_REFCOUNT(&engine_obj->std));
}

static void wasm_engine_free_obj(zend_object *object) {
	PHP_WASMTIME_DEBUG_LOG("wasm_engine_free_obj\n");
    php_wasm_engine_t *intern = (php_wasm_engine_t *) object;
    PHP_WASMTIME_DEBUG_LOG("WasmEngine::free_obj - Engine object: %p, refcount: %d\n", intern, GC_REFCOUNT(&intern->std));
    if (intern->store) {
        wasmtime_store_delete(intern->store);
        intern->store = NULL;
    }
    if (intern->engine) {
        wasm_engine_delete(intern->engine);
        intern->engine = NULL;
    }
    zend_object_std_dtor(&intern->std);
}

ZEND_BEGIN_ARG_INFO_EX(arginfo_wasmengine_construct, 0, 0, 0)
ZEND_END_ARG_INFO()

static const zend_function_entry wasm_engine_methods[] = {
    PHP_ME(WasmEngine, __construct, arginfo_wasmengine_construct, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
    PHP_FE_END
};


/*-------------------------------------------
  MODULE (WasmModule)
--------------------------------------------*/

static zend_object* wasm_module_create_object(zend_class_entry *class_type) {
    PHP_WASMTIME_DEBUG_LOG("wasm_module_create_object: sizeof(php_wasm_module_t)=%zu\n", sizeof(php_wasm_module_t));
    // Allocate only the size of our struct
    php_wasm_module_t *intern = ecalloc(1, sizeof(php_wasm_module_t));
    PHP_WASMTIME_DEBUG_LOG("wasm_module_create_object: Allocated module object 'intern' at address: %p\n", intern);
    zend_object_std_init(&intern->std, class_type);
    // No properties_init needed if no dynamic/declared properties
    intern->std.handlers = &wasm_module_object_handlers;
    PHP_WASMTIME_DEBUG_LOG("wasm_module_create_object: Returning pointer to 'intern->std' at address: %p\n", &intern->std);
    return &intern->std;
}

static void wasm_module_free_obj(zend_object *object) {
	PHP_WASMTIME_DEBUG_LOG("wasm_module_free_obj\n");
    php_wasm_module_t *intern = (php_wasm_module_t *) object;

    // Decrement the reference count of the associated engine object
    if (intern->engine_obj) {
        PHP_WASMTIME_DEBUG_LOG("WasmModule::free_obj - Releasing engine object: %p, refcount before release: %d\n", intern->engine_obj, GC_REFCOUNT(&intern->engine_obj->std));
        OBJ_RELEASE(&intern->engine_obj->std);
        intern->engine_obj = NULL; // Prevent double release
    }

	/**
	 * @TODO: Sometimes, but not always, wasmtime_module_delete causes a crash:
	 *         SIGSEGV (Address boundary error)
	 *         It's likely a double free error. Could wasm_instance_free_obj have freed this earlier?
	 */
    // if (intern && intern->module) {
    //     wasmtime_module_delete(intern->module);
    //     intern->module = NULL;
    // }
    zend_object_std_dtor(&intern->std);
	PHP_WASMTIME_DEBUG_LOG("wasm_module_free_obj done\n");
}

PHP_METHOD(WasmModule, __construct)
{
    char *filename;
    size_t filename_len;
    zval *engine_zv;

    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_OBJECT_OF_CLASS(engine_zv, wasm_engine_ce)
        Z_PARAM_STRING(filename, filename_len)
    ZEND_PARSE_PARAMETERS_END();

    php_wasm_engine_t *engine_obj = (php_wasm_engine_t *) Z_OBJ_P(engine_zv);
    php_wasm_module_t *module_obj = (php_wasm_module_t *) Z_OBJ_P(getThis());
    PHP_WASMTIME_DEBUG_LOG("[Module] Received engine: %p, refcount: %d\n", engine_obj, GC_REFCOUNT(&engine_obj->std));
    module_obj->engine_obj = engine_obj;
    GC_ADDREF(&engine_obj->std); // Increment engine refcount
    PHP_WASMTIME_DEBUG_LOG("[Module] Stored engine: %p, refcount now: %d\n", engine_obj, GC_REFCOUNT(&engine_obj->std));

    uint8_t *wasm_bytes = NULL;
    size_t wasm_size = 0;

    // --- Read File --- 
    // Removed detailed logging here for brevity
    wasm_size = read_wasm_file(filename, &wasm_bytes);
    if (wasm_size == 0 || !wasm_bytes) {
        OBJ_RELEASE(&engine_obj->std); // Decrement on failure
        zend_throw_exception(NULL, "Could not read WASM file", 0);
        return;
    }

    // --- Create Module --- 
    wasmtime_module_t *mod = NULL;
    wasmtime_error_t *error = wasmtime_module_new(engine_obj->engine, wasm_bytes, wasm_size, &mod);

    // --- Free Buffer --- 
    if (wasm_bytes) {
        efree(wasm_bytes);
        wasm_bytes = NULL;
    }

    // --- Handle Module Creation Error --- 
    if (error) {
        wasm_byte_vec_t msg;
        wasmtime_error_message(error, &msg);
        wasmtime_error_delete(error);
        OBJ_RELEASE(&engine_obj->std); // Decrement on failure
        zend_throw_exception(NULL, (char*)msg.data, 0);
        wasm_byte_vec_delete(&msg);
        return;
    }

    // --- Assign Module Pointer --- 
    // Removed detailed logging here for brevity
    module_obj->module = mod;
    
    // --- Success --- 
    PHP_WASMTIME_DEBUG_LOG("[Module] Completed successfully. Final engine refcount: %d\n", GC_REFCOUNT(&engine_obj->std));
}

ZEND_BEGIN_ARG_INFO_EX(arginfo_wasmmodule_construct, 0, 0, 2)
    ZEND_ARG_OBJ_INFO(0, engine, WasmEngine, 0)
    ZEND_ARG_INFO(0, filename)
ZEND_END_ARG_INFO()

static const zend_function_entry wasm_module_methods[] = {
    PHP_ME(WasmModule, __construct, arginfo_wasmmodule_construct, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
    PHP_FE_END
};


/*-------------------------------------------
  INSTANCE (WasmInstance)
--------------------------------------------*/

/* We'll store a minimal structure for import data.
   For brevity, the extension user can pass an array of name=>import_value.
   We'll handle function imports if given a closure, memory imports if given a WasmMemory, etc.
*/

typedef struct _php_import_mapping {
    const char *import_name;
    wasmtime_extern_t ext;
} php_import_mapping;

static zend_object *wasm_instance_create_object(zend_class_entry *class_type) {
    // Allocate only the size of our struct
    php_wasm_instance_t *intern = ecalloc(1, sizeof(php_wasm_instance_t)); 
    zend_object_std_init(&intern->std, class_type);
    // No properties_init needed
    intern->std.handlers = &wasm_instance_object_handlers;
    return &intern->std;
}

static void wasm_instance_free_obj(zend_object *object) {
	PHP_WASMTIME_DEBUG_LOG("wasm_instance_free_obj\n");
    php_wasm_instance_t *intern = (php_wasm_instance_t *) object;
    
    // Decrement the reference count of the associated engine object
    if (intern->engine_obj) {
        PHP_WASMTIME_DEBUG_LOG("WasmInstance::free_obj - Releasing engine object: %p, refcount before release: %d\n", intern->engine_obj, GC_REFCOUNT(&intern->engine_obj->std));
        OBJ_RELEASE(&intern->engine_obj->std);
        intern->engine_obj = NULL; 
    }

    zend_object_std_dtor(&intern->std);
    PHP_WASMTIME_DEBUG_LOG("WasmInstance::free_obj - Completed\n");
}

/* For an imported PHP function, we keep a reference to the zval callable in env */
typedef struct _php_host_func_env {
    zval callable;
    php_wasm_engine_t *engine_obj;
} php_host_func_env;

/* Host callback from WASM => PHP */
static wasm_trap_t* php_host_func_callback(
    void *env,
    wasmtime_caller_t *caller,
    const wasmtime_val_t *args,
    size_t nargs,
    wasmtime_val_t *results,
    size_t nresults
) {
    PHP_WASMTIME_DEBUG_LOG("php_host_func_callback called with %zu arguments\n", nargs);
    
    // Get function environment and validate
    php_host_func_env *fn_env = (php_host_func_env *)env;
    if (!fn_env) {
        PHP_WASMTIME_DEBUG_LOG("Invalid function environment\n");
        if (nresults > 0) {
            results[0].kind = WASMTIME_I32;
            results[0].of.i32 = 0;
        }
        return NULL; // No trap
    }
    
    // Check if the callable is valid
    if (Z_TYPE(fn_env->callable) == IS_UNDEF || Z_TYPE(fn_env->callable) == IS_NULL) {
        PHP_WASMTIME_DEBUG_LOG("Callable is undefined or null\n");
        if (nresults > 0) {
            results[0].kind = WASMTIME_I32;
            results[0].of.i32 = 0;
        }
        return NULL; // No trap
    }
    
    // Prepare call to PHP function
    zval retval;
    ZVAL_NULL(&retval);
    
    // Allocate arguments array
    zval *args_array = NULL;
    if (nargs > 0) {
        args_array = safe_emalloc(nargs, sizeof(zval), 0);
        for (size_t i = 0; i < nargs; i++) {
            ZVAL_NULL(&args_array[i]);
            switch (args[i].kind) {
                case WASMTIME_I32:
                    ZVAL_LONG(&args_array[i], (zend_long)args[i].of.i32);
                    PHP_WASMTIME_DEBUG_LOG("  Arg %zu: i32 = %d\n", i, args[i].of.i32);
                    break;
                case WASMTIME_I64:
                    ZVAL_LONG(&args_array[i], (zend_long)args[i].of.i64);
                    PHP_WASMTIME_DEBUG_LOG("  Arg %zu: i64 = %lld\n", i, (long long)args[i].of.i64);
                    break;
                case WASMTIME_F32:
                    ZVAL_DOUBLE(&args_array[i], (double)args[i].of.f32);
                    PHP_WASMTIME_DEBUG_LOG("  Arg %zu: f32 = %f\n", i, (double)args[i].of.f32);
                    break;
                case WASMTIME_F64:
                    ZVAL_DOUBLE(&args_array[i], (double)args[i].of.f64);
                    PHP_WASMTIME_DEBUG_LOG("  Arg %zu: f64 = %f\n", i, args[i].of.f64);
                    break;
                default:
                    PHP_WASMTIME_DEBUG_LOG("  Arg %zu: unknown type\n", i);
            }
        }
    }
    
    // Call the PHP function
    zval callable_copy;
    ZVAL_COPY(&callable_copy, &fn_env->callable);
    
    zval params;
    array_init_size(&params, nargs);
    for (size_t i = 0; i < nargs; i++) {
        zval tmp;
        ZVAL_COPY(&tmp, &args_array[i]);
        zend_hash_next_index_insert(Z_ARRVAL(params), &tmp);
    }
    
    // Use call_user_function which is more stable than zend_fcall_info_*
    int call_result = call_user_function(NULL, NULL, &callable_copy, &retval, nargs, args_array);
    
    // Clean up arguments
    if (args_array) {
        for (size_t i = 0; i < nargs; i++) {
            zval_ptr_dtor(&args_array[i]);
        }
        efree(args_array);
    }
    zval_ptr_dtor(&params);
    zval_ptr_dtor(&callable_copy);
    
    if (call_result != SUCCESS) {
        PHP_WASMTIME_DEBUG_LOG("Failed to call PHP function\n");
        ZVAL_NULL(&retval); // Ensure retval is NULL if call failed
    } else {
        PHP_WASMTIME_DEBUG_LOG("PHP function call succeeded\n");
    }
    
    // Convert return value to WebAssembly result
    if (nresults > 0) {
        wasmtime_context_t *context = wasmtime_caller_context(caller);
        const wasm_functype_t *func_ty = wasmtime_func_type(context, &fn_env->callable);
        
        if (func_ty) {
            const wasm_valtype_vec_t *results_ty = wasm_functype_results(func_ty);
            
            if (results_ty->size > 0) {
                wasm_valkind_t expected_kind = wasm_valtype_kind(results_ty->data[0]);
                
                switch (expected_kind) {
                    case WASM_I32:
                        results[0].kind = WASMTIME_I32;
                        if (Z_TYPE(retval) == IS_LONG) {
                            results[0].of.i32 = (int32_t)Z_LVAL(retval);
                        } else if (Z_TYPE(retval) == IS_DOUBLE) {
                            results[0].of.i32 = (int32_t)Z_DVAL(retval);
                        } else if (Z_TYPE(retval) == IS_TRUE) {
                            results[0].of.i32 = 1;
                        } else if (Z_TYPE(retval) == IS_FALSE) {
                            results[0].of.i32 = 0;
                        } else {
                            // Default for other types
                            results[0].of.i32 = 0;
                        }
                        break;
                    
                    case WASM_I64:
                        results[0].kind = WASMTIME_I64;
                        if (Z_TYPE(retval) == IS_LONG) {
                            results[0].of.i64 = (int64_t)Z_LVAL(retval);
                        } else if (Z_TYPE(retval) == IS_DOUBLE) {
                            results[0].of.i64 = (int64_t)Z_DVAL(retval);
                        } else if (Z_TYPE(retval) == IS_TRUE) {
                            results[0].of.i64 = 1;
                        } else if (Z_TYPE(retval) == IS_FALSE) {
                            results[0].of.i64 = 0;
                        } else {
                            // Default for other types
                            results[0].of.i64 = 0;
                        }
                        break;
                    
                    case WASM_F32:
                    case WASM_F64:
                        results[0].kind = (expected_kind == WASM_F32) ? WASMTIME_F32 : WASMTIME_F64;
                        if (Z_TYPE(retval) == IS_DOUBLE) {
                            results[0].of.f64 = Z_DVAL(retval);
                        } else if (Z_TYPE(retval) == IS_LONG) {
                            results[0].of.f64 = (double)Z_LVAL(retval);
                        } else {
                            // Default for other types
                            results[0].of.f64 = 0.0;
                        }
                        break;
                    
                    default:
                        // Unsupported return type, default to 0
                        results[0].kind = WASMTIME_I32;
                        results[0].of.i32 = 0;
                }
            }
            
            wasm_functype_delete((wasm_functype_t *)func_ty);
        } else {
            // Default if we can't determine type
            results[0].kind = WASMTIME_I32;
            results[0].of.i32 = 0;
        }
    }
    
    // Clean up return value
    zval_ptr_dtor(&retval);
    
    PHP_WASMTIME_DEBUG_LOG("php_host_func_callback completed successfully\n");
    return NULL; // No trap
}

/* Freed when the function is destroyed from the store */
static void php_host_func_finalizer(void *env) {
	PHP_WASMTIME_DEBUG_LOG("php_host_func_finalizer\n");
    php_host_func_env *fn_env = (php_host_func_env *)env;
    zval_dtor(&fn_env->callable);
    efree(fn_env);
}

// Define a simple callback that just prints "Hi"
wasm_trap_t* dummy_hi_callback(
	void *env,
	wasmtime_caller_t *caller,
	const wasmtime_val_t *args,
	size_t nargs,
	wasmtime_val_t *results,
	size_t nresults
) {
	PHP_WASMTIME_DEBUG_LOG("Hi\n");
	return NULL; // No trap
}

PHP_METHOD(WasmInstance, __construct)
{
    zval *engine_zv, *module_zv;
    zval *imports_zv = NULL;

    ZEND_PARSE_PARAMETERS_START(2, 3)
        Z_PARAM_OBJECT_OF_CLASS(engine_zv, wasm_engine_ce)
        Z_PARAM_OBJECT_OF_CLASS(module_zv, wasm_module_ce)
        Z_PARAM_OPTIONAL
        Z_PARAM_ARRAY(imports_zv)
    ZEND_PARSE_PARAMETERS_END();

    php_wasm_engine_t  *engine_obj  = (php_wasm_engine_t *) Z_OBJ_P(engine_zv);
    php_wasm_module_t  *module_obj  = (php_wasm_module_t *) Z_OBJ_P(module_zv);
    php_wasm_instance_t *inst_obj   = (php_wasm_instance_t *) Z_OBJ_P(getThis());
    
    PHP_WASMTIME_DEBUG_LOG("WasmInstance::__construct - Engine zval: %p, Engine C struct: %p\n", Z_OBJ_P(engine_zv), engine_obj);
    PHP_WASMTIME_DEBUG_LOG("WasmInstance::__construct - Instance zval: %p, Instance C struct: %p\n", Z_OBJ_P(getThis()), inst_obj);
    PHP_WASMTIME_DEBUG_LOG("WasmInstance::__construct - Engine refcount BEFORE storing/addref: %d\n", GC_REFCOUNT(&engine_obj->std));

    // Store the engine object pointer
    inst_obj->engine_obj = engine_obj;
    PHP_WASMTIME_DEBUG_LOG("WasmInstance::__construct - Stored engine pointer %p into instance->engine_obj field\n", engine_obj);

    // Increment engine's refcount
    PHP_WASMTIME_DEBUG_LOG("WasmInstance::__construct - About to GC_ADDREF on engine_obj->std at address: %p\n", &engine_obj->std);
    GC_ADDREF(&engine_obj->std);
    PHP_WASMTIME_DEBUG_LOG("WasmInstance::__construct - Engine refcount AFTER addref: %d\n", GC_REFCOUNT(&engine_obj->std));

    wasmtime_context_t *context = wasmtime_store_context(engine_obj->store);
	PHP_WASMTIME_DEBUG_LOG("context: %p\n", context);

    // Retrieve expected imports from module
    wasm_importtype_vec_t import_types;
    wasmtime_module_imports(module_obj->module, &import_types);

    wasmtime_extern_t *imports = NULL;
    size_t import_count = import_types.size;
    if (import_count > 0) {
        imports = ecalloc(import_count, sizeof(wasmtime_extern_t));
    }
	PHP_WASMTIME_DEBUG_LOG("imports: %p\n", imports);

    // If user provided an imports array, we attempt name-based matching
    // In a real extension, we'd do better checking. For demonstration, we do partial matching by name.
    HashTable *ht = NULL;
    if (imports_zv && Z_TYPE_P(imports_zv) == IS_ARRAY) {
        ht = Z_ARRVAL_P(imports_zv);
    }
	PHP_WASMTIME_DEBUG_LOG("ht: %p\n", ht);
    for (size_t i = 0; i < import_count; i++) {
        const wasm_importtype_t *imp_type = import_types.data[i];
        wasm_name_t module_name, name;
        module_name = *wasm_importtype_module(imp_type);
        name = *wasm_importtype_name(imp_type);

        wasmtime_extern_t this_import;
        memset(&this_import, 0, sizeof(wasmtime_extern_t));

        zval *found = NULL;
        if (ht) {
            // naive matching: we search in the array for "moduleName.name" or just "name"
            char keybuf[256];
            snprintf(keybuf, sizeof(keybuf), "%.*s.%.*s",
                     (int)module_name.size, module_name.data,
                     (int)name.size, name.data);
            found = zend_hash_str_find(ht, keybuf, strlen(keybuf));
            if (!found) {
                // try just the name
                snprintf(keybuf, sizeof(keybuf), "%.*s", (int)name.size, name.data);
                found = zend_hash_str_find(ht, keybuf, strlen(keybuf));
            }
        }

		int assigned = 0;
        const wasm_externtype_t *et = wasm_importtype_type(imp_type);
        switch (wasm_externtype_kind(et)) {
            case WASM_EXTERN_FUNC: {
				assigned = 0;
                this_import.kind = WASMTIME_EXTERN_FUNC;
                if (found && (Z_TYPE_P(found) == IS_OBJECT || Z_TYPE_P(found) == IS_CALLABLE)) {
                    // If it's a PHP callable, create a new host function
                    if (zend_is_callable(found, 0, NULL)) {

						PHP_WASMTIME_DEBUG_LOG("found callable: %p, type: %d, name: %s, import: %.*s.%.*s\n", 
						       found, Z_TYPE_P(found), 
						       Z_TYPE_P(found) == IS_OBJECT ? Z_OBJCE_P(found)->name->val : "closure",
						       (int)module_name.size, module_name.data,
						       (int)name.size, name.data);
						
                        // Create a host function from the PHP callable
                        php_host_func_env *env = emalloc(sizeof(php_host_func_env));
                        ZVAL_COPY(&env->callable, found);
                        env->engine_obj = engine_obj;
                        
                        // Get function type from the import
                        const wasm_functype_t *ft = wasm_externtype_as_functype_const(et);
                        
                        wasmtime_func_t func;
                        wasmtime_func_new(
                            context,
                            ft,
                            php_host_func_callback,
                            env,
                            php_host_func_finalizer,
                            &func
                        );
                        
                        this_import.of.func = func;
						PHP_WASMTIME_DEBUG_LOG("func: %p\n", func);
                        PHP_WASMTIME_DEBUG_LOG("Assigned PHP callable to import %.*s.%.*s\n", 
                               (int)module_name.size, module_name.data,
                               (int)name.size, name.data);
                        assigned = 1;
                    }
                }
                if(!assigned){
                	// if not found or not callable, fill with a dummy
                    // Create a dummy function that does nothing.
                    wasm_valtype_vec_t params;
                    wasm_valtype_vec_new_empty(&params);
                    wasm_valtype_vec_t results;
                    wasm_valtype_vec_new_empty(&results);
                    wasm_functype_t *dummy_ft = wasm_functype_new(&params, &results);
                    wasmtime_func_t dummy_func;
                    php_host_func_env *env = emalloc(sizeof(php_host_func_env));
                    ZVAL_UNDEF(&env->callable);
                    env->engine_obj = engine_obj;
                    wasmtime_func_new(context, dummy_ft, php_host_func_callback, env, php_host_func_finalizer, &dummy_func);
                    wasm_functype_delete(dummy_ft);
                    this_import.of.func = dummy_func;
                }
            } break;
            case WASM_EXTERN_MEMORY: {
                if (found && Z_TYPE_P(found) == IS_OBJECT) {
                    if (instanceof_function(Z_OBJCE_P(found), wasm_memory_ce)) {
                        php_wasm_memory_t *mem_obj = (php_wasm_memory_t *) Z_OBJ_P(found);
                        this_import.kind = WASMTIME_EXTERN_MEMORY;
                        this_import.of.memory = mem_obj->memory;
                        break;
                    }
                }
                // not found => create a blank memory if the import is required. We'll skip that for brevity.
                this_import.kind = WASMTIME_EXTERN_MEMORY;
                {
                    // create a minimal memory:
                    const wasm_memorytype_t *mtype = wasm_externtype_as_memorytype_const(et);
                    wasmtime_memory_t newmem;
                    wasmtime_error_t *err = wasmtime_memory_new(context, mtype, &newmem);
                    if (!err) {
                        this_import.of.memory = newmem;
                    } else {
                        wasmtime_error_delete(err);
                        // fallback
                        memset(&this_import, 0, sizeof(wasmtime_extern_t));
                    }
                }
            } break;
            case WASM_EXTERN_GLOBAL: {
                if (found && Z_TYPE_P(found) == IS_OBJECT) {
                    if (instanceof_function(Z_OBJCE_P(found), wasm_global_ce)) {
                        php_wasm_global_t *glob_obj = (php_wasm_global_t *) Z_OBJ_P(found);
                        this_import.kind = WASMTIME_EXTERN_GLOBAL;
                        this_import.of.global = glob_obj->global;
                        break;
                    }
                }
                // create a dummy global if not found
                this_import.kind = WASMTIME_EXTERN_GLOBAL;
                {
                    const wasm_globaltype_t *gtype = wasm_externtype_as_globaltype_const(et);
                    wasmtime_global_t newglob;
                    wasm_val_t val;
                    memset(&val, 0, sizeof(val));
                    val.kind = WASMTIME_I32;
                    val.of.i32 = 0;
                    wasmtime_error_t *err = wasmtime_global_new(context, gtype, &val, &newglob);
                    if (!err) {
                        this_import.of.global = newglob;
                    } else {
                        wasmtime_error_delete(err);
                        memset(&this_import, 0, sizeof(wasmtime_extern_t));
                    }
                }
            } break;
            default:
                memset(&this_import, 0, sizeof(wasmtime_extern_t));
                break;
        }

        imports[i] = this_import;
        wasm_name_delete(&module_name);
        wasm_name_delete(&name);
    }
	PHP_WASMTIME_DEBUG_LOG("after loop\n");

	// @TODO: this causes a double free error, but
	//        commenting it out causes a memory leak.
	//        Let's investigate and debug this.
    // wasm_importtype_vec_delete(&import_types);

	PHP_WASMTIME_DEBUG_LOG("vector deleted\n");

    wasm_trap_t *trap = NULL;
    wasmtime_instance_t instance;
    wasmtime_error_t *error = wasmtime_instance_new(
        context,
        module_obj->module,
        imports, import_count,
        &instance,
        &trap
    );

	PHP_WASMTIME_DEBUG_LOG("instance: %p\n", instance);
    if (imports) {
        efree(imports);
    }

    if (error != NULL) {
        wasm_byte_vec_t msg;
        wasmtime_error_message(error, &msg);
        wasmtime_error_delete(error);
        zend_throw_exception(NULL, (char*)msg.data, 0);
        wasm_byte_vec_delete(&msg);
        return;
    }
    if (trap != NULL) {
        wasm_byte_vec_t msg;
        wasm_trap_message(trap, &msg);
        wasm_trap_delete(trap);
        zend_throw_exception(NULL, (char*)msg.data, 0);
        wasm_byte_vec_delete(&msg);
        return;
    }

    inst_obj->instance = instance;
}

ZEND_BEGIN_ARG_INFO_EX(arginfo_wasminstance_construct, 0, 0, 2)
    ZEND_ARG_OBJ_INFO(0, engine, WasmEngine, 0)
    ZEND_ARG_OBJ_INFO(0, module, WasmModule, 0)
    ZEND_ARG_ARRAY_INFO(0, imports, 1)
ZEND_END_ARG_INFO()

PHP_METHOD(WasmInstance, call)
{
    char *func_name;
    size_t func_name_len;
    zval *args_zv = NULL;

    ZEND_PARSE_PARAMETERS_START(1, 2)
        Z_PARAM_STRING(func_name, func_name_len)
        Z_PARAM_OPTIONAL
        Z_PARAM_ARRAY(args_zv)
    ZEND_PARSE_PARAMETERS_END();

    php_wasm_instance_t *inst_obj = (php_wasm_instance_t *) Z_OBJ_P(getThis());
    php_wasm_engine_t   *engine_obj = inst_obj->engine_obj;
    wasmtime_context_t  *context = wasmtime_store_context(engine_obj->store);

    wasmtime_extern_t item;
    bool found = wasmtime_instance_export_get(context, &inst_obj->instance,
            func_name, func_name_len, &item);
    if(!found || item.kind != WASMTIME_EXTERN_FUNC) {
        zend_throw_exception(NULL, "Function not found in exports", 0);
        return;
    }
    wasmtime_func_t func = item.of.func;

    // Determine the number of results expected by the function
    const wasm_functype_t* func_type = wasmtime_func_type(context, &func);
    const wasm_valtype_vec_t* result_types = wasm_functype_results(func_type);
    size_t nresults = result_types->size;

    wasmtime_val_t *args = NULL;
    size_t arg_count = 0;
    if(args_zv && Z_TYPE_P(args_zv) == IS_ARRAY) {
        arg_count = zend_hash_num_elements(Z_ARRVAL_P(args_zv));
        args = ecalloc(arg_count, sizeof(wasmtime_val_t));
        zval *val;
        uint32_t i = 0;
        ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(args_zv), val) {
            if (Z_TYPE_P(val) == IS_LONG) {
                args[i].kind = WASMTIME_I32;
                args[i].of.i32 = (int32_t) Z_LVAL_P(val);
            } else if (Z_TYPE_P(val) == IS_DOUBLE) {
                args[i].kind = WASMTIME_F64;
                args[i].of.f64 = Z_DVAL_P(val);
			} else {
				args[i].kind = WASMTIME_I32;
				args[i].of.i32 = (int32_t) zval_get_long(val);
			}

            i++;
        } ZEND_HASH_FOREACH_END();
    }

    // Allocate space for results based on the function signature
    wasmtime_val_t *results = NULL;
    if (nresults > 0) {
        results = ecalloc(nresults, sizeof(wasmtime_val_t));
    }
    wasm_trap_t *trap = NULL;

    wasmtime_error_t *err = wasmtime_func_call(
        context, &func,
        args, arg_count,
        results, nresults, // Pass the correct number of results
        &trap
    );

    if (args) efree(args);

    // Clean up function type info
    wasm_functype_delete((wasm_functype_t*) func_type); // Cast needed as wasmtime_func_type returns const

    if (err != NULL) {
        if (results) efree(results);
        wasm_byte_vec_t msg;
        wasmtime_error_message(err, &msg);
        wasmtime_error_delete(err);
        zend_throw_exception(NULL, (char*)msg.data, 0);
        wasm_byte_vec_delete(&msg);
        return;
    }
    if (trap != NULL) {
        if (results) efree(results);
        wasm_byte_vec_t msg;
        wasm_trap_message(trap, &msg);
        wasm_trap_delete(trap);
        zend_throw_exception(NULL, (char*)msg.data, 0);
        wasm_byte_vec_delete(&msg);
        return;
    }

    // Handle results based on nresults
    if (nresults == 0) {
        // No results to free
        RETURN_NULL();
    } else {
        // For now, handle only the first result, similar to before
        // Future improvement: could return an array for multiple results
        wasmtime_val_t first_result = results[0];
        efree(results); // Free the results array

        switch (first_result.kind) {
            case WASMTIME_I32:
                RETURN_LONG(first_result.of.i32);
            case WASMTIME_I64:
                RETURN_LONG((zend_long)first_result.of.i64);
            case WASMTIME_F32:
                RETURN_DOUBLE((double)first_result.of.f32);
            case WASMTIME_F64:
                RETURN_DOUBLE(first_result.of.f64);
            default:
                RETURN_NULL(); // Should not happen with known types
        }
    }
}

PHP_METHOD(WasmInstance, getMemory)
{
    PHP_WASMTIME_DEBUG_LOG("WasmInstance::getMemory\n");
    char *mem_name = "memory";
    size_t mem_name_len = 6;

    ZEND_PARSE_PARAMETERS_START(0, 1)
        Z_PARAM_OPTIONAL
        Z_PARAM_STRING(mem_name, mem_name_len)
    ZEND_PARSE_PARAMETERS_END();

    php_wasm_instance_t *intern = (php_wasm_instance_t *) Z_OBJ_P(ZEND_THIS);
    PHP_WASMTIME_DEBUG_LOG("getMemory: looking for %s\n", mem_name);
    PHP_WASMTIME_DEBUG_LOG("Instance: %p, engine_obj: %p\n", intern, intern->engine_obj);
    PHP_WASMTIME_DEBUG_LOG("Engine refcount before getMemory: %d\n", GC_REFCOUNT(&intern->engine_obj->std));
    wasmtime_context_t *ctx = wasmtime_store_context(intern->engine_obj->store);
    PHP_WASMTIME_DEBUG_LOG("Context: %p\n", ctx);

    wasmtime_extern_t item;
    bool found = wasmtime_instance_export_get(ctx, &intern->instance, mem_name, mem_name_len, &item);
    if (!found || item.kind != WASMTIME_EXTERN_MEMORY) {
        PHP_WASMTIME_DEBUG_LOG("ERROR: Memory export not found\n");
        zend_throw_exception(NULL, "Memory export not found", 0);
        RETURN_THROWS();
    }

    PHP_WASMTIME_DEBUG_LOG("getMemory: memory export found\n");
    object_init_ex(return_value, wasm_memory_ce);
    php_wasm_memory_t *mem_obj = (php_wasm_memory_t *) Z_OBJ_P(return_value);
    
    PHP_WASMTIME_DEBUG_LOG("Creating memory object: %p\n", mem_obj);
    PHP_WASMTIME_DEBUG_LOG("inst->engine_obj: %p\n", intern->engine_obj);
    
    // Copy the memory structure
    mem_obj->memory = item.of.memory;
    PHP_WASMTIME_DEBUG_LOG("mem_obj->memory: %p\n", &mem_obj->memory);
    
    // Store a direct pointer to the engine object
    mem_obj->engine_obj = intern->engine_obj;
    
    PHP_WASMTIME_DEBUG_LOG("inst->engine_obj: %p\n", intern->engine_obj);
    // Increment the reference count of the engine object
    GC_ADDREF(&intern->engine_obj->std);
    PHP_WASMTIME_DEBUG_LOG("Engine refcount after getMemory: %d\n", GC_REFCOUNT(&intern->engine_obj->std));

    PHP_WASMTIME_DEBUG_LOG("returning memory object\n");
    Z_TRY_ADDREF_P(return_value);
}


ZEND_BEGIN_ARG_INFO_EX(arginfo_wasminstance_call, 0, 0, 1)
    ZEND_ARG_INFO(0, functionName)
    ZEND_ARG_ARRAY_INFO(0, args, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_wasminstance_getMemory, 0, 0, 1)
    ZEND_ARG_INFO(0, memoryName)
ZEND_END_ARG_INFO()

static const zend_function_entry wasm_instance_methods[] = {
    PHP_ME(WasmInstance, __construct, arginfo_wasminstance_construct, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
    PHP_ME(WasmInstance, call,        arginfo_wasminstance_call,      ZEND_ACC_PUBLIC)
    PHP_ME(WasmInstance, getMemory,   arginfo_wasminstance_getMemory, ZEND_ACC_PUBLIC)
    PHP_FE_END
};



/*-------------------------------------------
  MEMORY (WasmMemory)
--------------------------------------------*/
static zend_object *wasm_memory_create_object(zend_class_entry *class_type)
{
    // Allocate only the size of our struct
    php_wasm_memory_t *intern = ecalloc(1, sizeof(php_wasm_memory_t));
    PHP_WASMTIME_DEBUG_LOG("Creating memory object: %p\n", intern);
    
    zend_object_std_init(&intern->std, class_type);
    // No properties_init needed
    
    // Initialize memory fields to zero
    memset(&intern->memory, 0, sizeof(wasmtime_memory_t));
    intern->engine_obj = NULL;
    
    intern->std.handlers = &wasm_memory_object_handlers;
    return &intern->std;
}

static void wasm_memory_free_obj(zend_object *object) {
	PHP_WASMTIME_DEBUG_LOG("wasm_memory_free_obj\n");
    php_wasm_memory_t *intern = (php_wasm_memory_t *) object;
    PHP_WASMTIME_DEBUG_LOG("WasmMemory::free_obj - Memory object: %p, refcount: %d\n", intern, GC_REFCOUNT(&intern->std));
    
    // Decrement the reference count of the associated engine object
    if (intern->engine_obj) {
        PHP_WASMTIME_DEBUG_LOG("WasmMemory::free_obj - Releasing engine object: %p, refcount before release: %d\n", intern->engine_obj, GC_REFCOUNT(&intern->engine_obj->std));
        OBJ_RELEASE(&intern->engine_obj->std);
        intern->engine_obj = NULL; // Prevent double release
    }

    // Note: wasmtime_memory_t obtained from an export doesn't need freeing here,
    // it's managed by the instance/store.
    // If memory was created via `new WasmMemory`, its wasmtime_memory_t 
    // *might* need wasmtime_memory_delete, but that requires context, which we don't store yet.
    // For now, we assume memory is managed externally or via instance exports.

    zend_object_std_dtor(&intern->std);
    PHP_WASMTIME_DEBUG_LOG("WasmMemory::free_obj - Completed\n");
}

/* Arginfo: for all Memory methods */
/* We can provide a return type or leave as any. If we know the type, let's specify. */

/* __construct(engine, initialPages, maxPages=?) */
ZEND_BEGIN_ARG_INFO_EX(arginfo_wasmmemory_construct, 0, 0, 2)
    ZEND_ARG_OBJ_INFO(0, engine, WasmEngine, 0)
    ZEND_ARG_INFO(0, initialPages)
    ZEND_ARG_INFO(0, maxPages)
ZEND_END_ARG_INFO()

/* dataSize(): int */
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_wasmmemory_datasize, 0, 0, IS_LONG, 0)
ZEND_END_ARG_INFO()

/* read(offset, length): string */
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_wasmmemory_read, 0, 2, IS_STRING, 0)
    ZEND_ARG_INFO(0, offset)
    ZEND_ARG_INFO(0, length)
ZEND_END_ARG_INFO()

/* read_string_from_pointer(pointer): string */
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_wasmmemory_read_string_from_pointer, 0, 1, IS_STRING, 0)
    ZEND_ARG_INFO(0, pointer)
ZEND_END_ARG_INFO()

/* write(offset, data): void */
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_wasmmemory_write, 0, 2, IS_VOID, 0)
    ZEND_ARG_INFO(0, offset)
    ZEND_ARG_INFO(0, data)
ZEND_END_ARG_INFO()

/* size(): int */
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_wasmmemory_size, 0, 0, IS_LONG, 0)
ZEND_END_ARG_INFO()

/* grow(additionalPages): int|false (on failure) */
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX(arginfo_wasmmemory_grow, 0, 1, MAY_BE_LONG|MAY_BE_FALSE)
    ZEND_ARG_INFO(0, additional)
ZEND_END_ARG_INFO()

/* allocate(size): int */
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_wasmmemory_allocate, 0, 1, IS_LONG, 0)
    ZEND_ARG_INFO(0, size)
ZEND_END_ARG_INFO()

/* free(offset): void */
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_wasmmemory_free, 0, 1, IS_VOID, 0)
    ZEND_ARG_INFO(0, offset)
ZEND_END_ARG_INFO()

PHP_METHOD(WasmMemory, __construct)
{
    PHP_WASMTIME_DEBUG_LOG("WasmMemory::__construct\n");
    zval *engine_zv;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_OBJECT(engine_zv)
    ZEND_PARSE_PARAMETERS_END();

    php_wasm_memory_t *intern = (php_wasm_memory_t *) Z_OBJ_P(ZEND_THIS);
    php_wasm_engine_t *engine_intern = (php_wasm_engine_t *) Z_OBJ_P(engine_zv);
    PHP_WASMTIME_DEBUG_LOG("WasmMemory::__construct - Engine object: %p, refcount: %d\n", engine_intern, GC_REFCOUNT(&engine_intern->std));

    wasmtime_context_t *context = wasmtime_store_context(engine_intern->store);
    PHP_WASMTIME_DEBUG_LOG("Context: %p\n", context);

    wasm_limits_t limits;
    limits.min = (uint32_t) 1; // Assuming initial_pages is 1
    limits.max = wasm_limits_max_default;
    wasm_memorytype_t *memtype = wasm_memorytype_new(&limits);
    wasmtime_memory_t memory;
    wasmtime_error_t *err = wasmtime_memory_new(context, memtype, &memory);
    wasm_memorytype_delete(memtype);

    if (err != NULL) {
        wasm_byte_vec_t msg;
        wasmtime_error_message(err, &msg);
        wasmtime_error_delete(err);
        zend_throw_exception(NULL, (char*)msg.data, 0);
        wasm_byte_vec_delete(&msg);
        return;
    }

    intern->memory = memory;
    PHP_WASMTIME_DEBUG_LOG("Memory constructor completed successfully\n");
}

PHP_METHOD(WasmMemory, dataSize)
{
    php_wasm_memory_t *mem_obj = (php_wasm_memory_t *) Z_OBJ_P(getThis());
    
    if (!mem_obj->engine_obj) {
        zend_throw_exception(NULL, "Memory has invalid engine reference", 0);
        return;
    }
    
    wasmtime_context_t *context = wasmtime_store_context(mem_obj->engine_obj->store);
    size_t sz = wasmtime_memory_data_size(context, &mem_obj->memory);
    RETURN_LONG(sz);
}

PHP_METHOD(WasmMemory, read)
{
    zend_long offset, length;
    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_LONG(offset)
        Z_PARAM_LONG(length)
    ZEND_PARSE_PARAMETERS_END();

    PHP_WASMTIME_DEBUG_LOG("WasmMemory::read - offset: %lld, length: %lld\n", (long long)offset, (long long)length);

    PHP_WASMTIME_DEBUG_LOG("Getting memory object\n");
    php_wasm_memory_t *mem_obj = (php_wasm_memory_t *) Z_OBJ_P(getThis());
    PHP_WASMTIME_DEBUG_LOG("Memory object: %p, engine_obj: %p\n", mem_obj, mem_obj->engine_obj);
    
    if (!mem_obj->engine_obj) {
        PHP_WASMTIME_DEBUG_LOG("ERROR: engine_obj is NULL\n");
        zend_throw_exception(NULL, "Memory has invalid engine reference", 0);
        return;
    }
    PHP_WASMTIME_DEBUG_LOG("WasmMemory::read - Engine object: %p, refcount: %d\n", mem_obj->engine_obj, GC_REFCOUNT(&mem_obj->engine_obj->std));
    
    wasmtime_context_t *context = wasmtime_store_context(mem_obj->engine_obj->store);
    PHP_WASMTIME_DEBUG_LOG("Context: %p\n", context);

    size_t sz = wasmtime_memory_data_size(context, &mem_obj->memory);
    PHP_WASMTIME_DEBUG_LOG("Memory size: %zu\n", sz);
    
    if (offset < 0 || length < 0 || (size_t)offset + (size_t)length > sz) {
        zend_throw_exception(NULL, "Memory read out of bounds", 0);
        return;
    }

    uint8_t *data = wasmtime_memory_data(context, &mem_obj->memory);
    PHP_WASMTIME_DEBUG_LOG("Memory data pointer: %p\n", data);
    
    // Allocate new memory for the string to avoid double free issues
    char *copy = emalloc(length + 1);
    memcpy(copy, data + offset, length);
    copy[length] = '\0';
    
    PHP_WASMTIME_DEBUG_LOG("Read complete, returning string of length %lld\n", (long long)length);
    RETURN_STRINGL(copy, length);
}

PHP_METHOD(WasmMemory, read_string_from_pointer)
{
    zend_long pointer;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_LONG(pointer)
    ZEND_PARSE_PARAMETERS_END();

    PHP_WASMTIME_DEBUG_LOG("WasmMemory::read_string_from_pointer - pointer: %lld\n", (long long)pointer);

    php_wasm_memory_t *mem_obj = (php_wasm_memory_t *) Z_OBJ_P(getThis());
    
    if (!mem_obj->engine_obj) {
        PHP_WASMTIME_DEBUG_LOG("ERROR: engine_obj is NULL\n");
        zend_throw_exception(NULL, "Memory has invalid engine reference", 0);
        return;
    }
    
    wasmtime_context_t *context = wasmtime_store_context(mem_obj->engine_obj->store);
    
    // First read the pointer value (4 bytes for address)
    uint8_t *data = wasmtime_memory_data(context, &mem_obj->memory);
    size_t mem_size = wasmtime_memory_data_size(context, &mem_obj->memory);
    
    // Check if we can read the pointer data (8 bytes total - 4 for address, 4 for length)
    if (pointer < 0 || (size_t)pointer + 8 > mem_size) {
        zend_throw_exception(NULL, "Memory pointer read out of bounds", 0);
        return;
    }
    
    // Read the address (first 4 bytes)
    uint32_t address;
    memcpy(&address, data + pointer, 4);
    
    // Read the length (next 4 bytes)
    uint32_t length;
    memcpy(&length, data + pointer + 4, 4);
    
    PHP_WASMTIME_DEBUG_LOG("Extracted address: %u, length: %u\n", address, length);
    
    // Now read the actual string data using the address and length
    if (address < 0 || (size_t)address + length > mem_size) {
        zend_throw_exception(NULL, "Memory string data read out of bounds", 0);
        return;
    }
    
    // Allocate memory for the string and copy it
    char *copy = emalloc(length + 1);
    memcpy(copy, data + address, length);
    copy[length] = '\0';
    
    PHP_WASMTIME_DEBUG_LOG("String read complete, returning string of length %u\n", length);
    RETURN_STRINGL(copy, length);
}


PHP_METHOD(WasmMemory, write)
{
    zend_long offset;
    char *buf;
    size_t buf_len;

    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_LONG(offset)
        Z_PARAM_STRING(buf, buf_len)
    ZEND_PARSE_PARAMETERS_END();

    PHP_WASMTIME_DEBUG_LOG("WasmMemory::write - offset: %lld, buf_len: %zu\n", (long long)offset, buf_len);

    php_wasm_memory_t *mem_obj = (php_wasm_memory_t *) Z_OBJ_P(getThis());
    PHP_WASMTIME_DEBUG_LOG("Memory object: %p, engine_obj: %p\n", mem_obj, mem_obj->engine_obj);
    
    if (!mem_obj->engine_obj) {
        PHP_WASMTIME_DEBUG_LOG("ERROR: engine_obj is NULL\n");
        zend_throw_exception(NULL, "Memory has invalid engine reference", 0);
        return;
    }
    PHP_WASMTIME_DEBUG_LOG("WasmMemory::write - Engine object: %p, refcount: %d\n", mem_obj->engine_obj, GC_REFCOUNT(&mem_obj->engine_obj->std));
    
    wasmtime_context_t *context = wasmtime_store_context(mem_obj->engine_obj->store);
    PHP_WASMTIME_DEBUG_LOG("Context: %p\n", context);

    size_t sz = wasmtime_memory_data_size(context, &mem_obj->memory);
    PHP_WASMTIME_DEBUG_LOG("Memory size: %zu\n", sz);
    if (offset < 0 || (size_t)offset + buf_len > sz) {
        PHP_WASMTIME_DEBUG_LOG("ERROR: Memory write out of bounds\n");
        zend_throw_exception(NULL, "Memory write out of bounds", 0);
        return;
    }

    uint8_t *data = wasmtime_memory_data(context, &mem_obj->memory);
    PHP_WASMTIME_DEBUG_LOG("Memory data pointer: %p\n", data);
    memcpy(data + offset, buf, buf_len);
    PHP_WASMTIME_DEBUG_LOG("Write complete\n");
}

PHP_METHOD(WasmMemory, size)
{
    php_wasm_memory_t *mem_obj = (php_wasm_memory_t *) Z_OBJ_P(getThis());
    
    if (!mem_obj->engine_obj) {
        zend_throw_exception(NULL, "Memory has invalid engine reference", 0);
        return;
    }
    
    wasmtime_context_t *context = wasmtime_store_context(mem_obj->engine_obj->store);
    size_t pages = wasmtime_memory_size(context, &mem_obj->memory);
    RETURN_LONG((zend_long)pages);
}

PHP_METHOD(WasmMemory, grow)
{
    zend_long additional;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_LONG(additional)
    ZEND_PARSE_PARAMETERS_END();

    php_wasm_memory_t *mem_obj = (php_wasm_memory_t *) Z_OBJ_P(getThis());
    
    if (!mem_obj->engine_obj) {
        zend_throw_exception(NULL, "Memory has invalid engine reference", 0);
        return;
    }
    
    wasmtime_context_t *context = wasmtime_store_context(mem_obj->engine_obj->store);

    uint64_t previous_size;
    wasmtime_error_t *err = wasmtime_memory_grow(context, &mem_obj->memory, (uint64_t)additional, &previous_size);
    if (err) {
        wasmtime_error_delete(err);
        RETURN_FALSE;
    }
    RETURN_LONG(previous_size);
}

PHP_METHOD(WasmMemory, allocate)
{
    zend_long size;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_LONG(size)
    ZEND_PARSE_PARAMETERS_END();

    if (size <= 0) {
        zend_throw_exception(NULL, "Allocation size must be positive", 0);
        return;
    }

    php_wasm_memory_t *mem_obj = (php_wasm_memory_t *) Z_OBJ_P(getThis());
    
    if (!mem_obj->engine_obj) {
        zend_throw_exception(NULL, "Memory has invalid engine reference", 0);
        return;
    }
    
    wasmtime_context_t *context = wasmtime_store_context(mem_obj->engine_obj->store);
    size_t mem_size = wasmtime_memory_data_size(context, &mem_obj->memory);
    
    // Simple implementation: just return the current size as the allocation point
    // In a real implementation, you would track allocations and free blocks
    // This is a placeholder that always allocates at the end of the current memory
    
    // Grow memory if necessary (each page is 64KB)
    if (mem_size < size) {
        uint64_t pages_needed = (size - mem_size + 65535) / 65536;
        uint64_t previous_size;
        wasmtime_error_t *err = wasmtime_memory_grow(context, &mem_obj->memory, pages_needed, &previous_size);
        
        if (err) {
            wasmtime_error_delete(err);
            zend_throw_exception(NULL, "Failed to grow memory for allocation", 0);
            return;
        }
        
        // Memory size after growth
        mem_size = wasmtime_memory_data_size(context, &mem_obj->memory);
    }
    
    // For now, just return the end of the used memory
    // A real implementation would need to track allocated blocks
    // This is just a placeholder that always allocates at the end
    RETURN_LONG(mem_size - size);
}

PHP_METHOD(WasmMemory, free)
{
    zend_long offset;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_LONG(offset)
    ZEND_PARSE_PARAMETERS_END();

    php_wasm_memory_t *mem_obj = (php_wasm_memory_t *) Z_OBJ_P(getThis());
    
    if (!mem_obj->engine_obj) {
        zend_throw_exception(NULL, "Memory has invalid engine reference", 0);
        return;
    }
    
    wasmtime_context_t *context = wasmtime_store_context(mem_obj->engine_obj->store);
    size_t mem_size = wasmtime_memory_data_size(context, &mem_obj->memory);
    
    // Validate offset
    if (offset < 0 || (size_t)offset >= mem_size) {
        zend_throw_exception(NULL, "Invalid memory offset for free operation", 0);
        return;
    }
    
    // This is a placeholder implementation that doesn't actually free memory
    // In a real implementation, you would mark this block as free for reuse
    // For now, this method exists but doesn't do anything meaningful
}

static const zend_function_entry wasm_memory_methods[] = {
    PHP_ME(WasmMemory, __construct, arginfo_wasmmemory_construct, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
    PHP_ME(WasmMemory, dataSize,    arginfo_wasmmemory_datasize,   ZEND_ACC_PUBLIC)
    PHP_ME(WasmMemory, read,        arginfo_wasmmemory_read,       ZEND_ACC_PUBLIC)
	PHP_ME(WasmMemory, read_string_from_pointer, arginfo_wasmmemory_read_string_from_pointer, ZEND_ACC_PUBLIC)
    PHP_ME(WasmMemory, write,       arginfo_wasmmemory_write,      ZEND_ACC_PUBLIC)
    PHP_ME(WasmMemory, size,        arginfo_wasmmemory_size,       ZEND_ACC_PUBLIC)
    PHP_ME(WasmMemory, grow,        arginfo_wasmmemory_grow,       ZEND_ACC_PUBLIC)
    PHP_ME(WasmMemory, allocate,    arginfo_wasmmemory_allocate,   ZEND_ACC_PUBLIC)
    PHP_ME(WasmMemory, free,        arginfo_wasmmemory_free,       ZEND_ACC_PUBLIC)
    PHP_FE_END
};


/*-------------------------------------------
  GLOBAL (WasmGlobal)
--------------------------------------------*/
static zend_object *wasm_global_create_object(zend_class_entry *class_type) {
    // Allocate only the size of our struct
    php_wasm_global_t *intern = ecalloc(1, sizeof(php_wasm_global_t)); 
    zend_object_std_init(&intern->std, class_type);
    // No properties_init needed
    intern->std.handlers = &wasm_global_object_handlers;
    return &intern->std;
}

static void wasm_global_free_obj(zend_object *object) {
	PHP_WASMTIME_DEBUG_LOG("wasm_global_free_obj\n");
    php_wasm_global_t *intern = (php_wasm_global_t *) object;

    // Decrement the reference count of the associated engine object
    if (intern->engine_obj) {
        PHP_WASMTIME_DEBUG_LOG("WasmGlobal::free_obj - Releasing engine object: %p, refcount before release: %d\n", intern->engine_obj, GC_REFCOUNT(&intern->engine_obj->std));
        OBJ_RELEASE(&intern->engine_obj->std);
        intern->engine_obj = NULL; 
    }

    zend_object_std_dtor(&intern->std);
    PHP_WASMTIME_DEBUG_LOG("WasmGlobal::free_obj - Completed\n");
}

/* A constructor that creates a new global, e.g. i32 or i64. For simplicity only i32. */
PHP_METHOD(WasmGlobal, __construct)
{
    zval *engine_zv;
    zend_long initial_value;
    zend_bool mutable_ = 0;
    ZEND_PARSE_PARAMETERS_START(2, 3)
        Z_PARAM_OBJECT_OF_CLASS(engine_zv, wasm_engine_ce)
        Z_PARAM_LONG(initial_value)
        Z_PARAM_OPTIONAL
        Z_PARAM_BOOL(mutable_)
    ZEND_PARSE_PARAMETERS_END();

    php_wasm_engine_t *engine_obj = (php_wasm_engine_t *) Z_OBJ_P(engine_zv);
    php_wasm_global_t *global_obj = (php_wasm_global_t *) Z_OBJ_P(getThis());
    
    PHP_WASMTIME_DEBUG_LOG("WasmGlobal::__construct - Received engine: %p, refcount: %d\n", engine_obj, GC_REFCOUNT(&engine_obj->std));
    
    // Store the engine object and increment its refcount
    global_obj->engine_obj = engine_obj;
    GC_ADDREF(&engine_obj->std);
    PHP_WASMTIME_DEBUG_LOG("WasmGlobal::__construct - Stored engine: %p, refcount now: %d\n", engine_obj, GC_REFCOUNT(&engine_obj->std));

    wasmtime_context_t *context = wasmtime_store_context(engine_obj->store);

    wasm_valtype_t *vtype = wasm_valtype_new_i32();
    wasm_mutability_t mut = mutable_ ? WASM_VAR : WASM_CONST;
    wasm_globaltype_t *gtype = wasm_globaltype_new(vtype, mut);

    wasmtime_global_t g;
    wasmtime_error_t *err = NULL;

    wasmtime_val_t val;
    val.kind = WASMTIME_I32;
    val.of.i32 = (int32_t)initial_value;
    err = wasmtime_global_new(context, gtype, &val, &g);

    wasm_globaltype_delete(gtype);

    if (err) {
        wasm_byte_vec_t msg;
        wasmtime_error_message(err, &msg);
        wasmtime_error_delete(err);
        zend_throw_exception(NULL, (char*)msg.data, 0);
        wasm_byte_vec_delete(&msg);
        return;
    }

    global_obj->global = g;
}

PHP_METHOD(WasmGlobal, get)
{
    php_wasm_global_t *global_obj = (php_wasm_global_t *) Z_OBJ_P(getThis());
    wasmtime_context_t *context = wasmtime_store_context(global_obj->engine_obj->store);
    wasmtime_val_t val;
    wasmtime_global_get(context, &global_obj->global, &val);

    switch(val.kind) {
        case WASMTIME_I32: RETURN_LONG(val.of.i32);
        case WASMTIME_I64: RETURN_LONG((zend_long)val.of.i64);
        case WASMTIME_F32: RETURN_DOUBLE((double)val.of.f32);
        case WASMTIME_F64: RETURN_DOUBLE(val.of.f64);
        default: RETURN_NULL();
    }
}

PHP_METHOD(WasmGlobal, set)
{
    zend_long newval;
    ZEND_PARSE_PARAMETERS_START(1,1)
        Z_PARAM_LONG(newval)
    ZEND_PARSE_PARAMETERS_END();

    php_wasm_global_t *global_obj = (php_wasm_global_t *) Z_OBJ_P(getThis());
    wasmtime_context_t *context = wasmtime_store_context(global_obj->engine_obj->store);

    wasmtime_val_t val;
    val.kind = WASMTIME_I32;
    val.of.i32 = (int32_t)newval;

    wasmtime_error_t *err = wasmtime_global_set(context, &global_obj->global, &val);
    if (err) {
        wasmtime_error_delete(err);
        zend_throw_exception(NULL, "Failed to set global (might be immutable)", 0);
        return;
    }
}

/* Arginfo for WasmGlobal methods */
/* __construct(engine, initial_value, mutable=false) */
ZEND_BEGIN_ARG_INFO_EX(arginfo_wasmglobal_construct, 0, 0, 2)
    ZEND_ARG_OBJ_INFO(0, engine, WasmEngine, 0)
    ZEND_ARG_INFO(0, initialValue)
    ZEND_ARG_INFO(0, mutable)
ZEND_END_ARG_INFO()

/* get(): mixed */
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_wasmglobal_get, 0, 0, IS_MIXED, 0)
ZEND_END_ARG_INFO()

/* set(value: int) : void */
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_wasmglobal_set, 0, 1, IS_VOID, 0)
    ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

static const zend_function_entry wasm_global_methods[] = {
    PHP_ME(WasmGlobal, __construct, arginfo_wasmglobal_construct, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
    PHP_ME(WasmGlobal, get,         arginfo_wasmglobal_get,       ZEND_ACC_PUBLIC)
    PHP_ME(WasmGlobal, set,         arginfo_wasmglobal_set,       ZEND_ACC_PUBLIC)
    PHP_FE_END
};


/*-------------------------------------------
  Module entry / Startup 
--------------------------------------------*/
PHP_MINIT_FUNCTION(wasmtime)
{
    zend_class_entry ce;

    /* WasmEngine */
    INIT_CLASS_ENTRY(ce, "WasmEngine", wasm_engine_methods);
    wasm_engine_ce = zend_register_internal_class(&ce);
    wasm_engine_ce->create_object = wasm_engine_create_object;
    memcpy(&wasm_engine_object_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
    wasm_engine_object_handlers.clone_obj = NULL;
    wasm_engine_object_handlers.free_obj = wasm_engine_free_obj;

    /* WasmModule */
    INIT_CLASS_ENTRY(ce, "WasmModule", wasm_module_methods);
    wasm_module_ce = zend_register_internal_class(&ce);
    wasm_module_ce->create_object = wasm_module_create_object;
    memcpy(&wasm_module_object_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
    wasm_module_object_handlers.clone_obj = NULL;
    wasm_module_object_handlers.free_obj = wasm_module_free_obj;

    /* WasmInstance */
    INIT_CLASS_ENTRY(ce, "WasmInstance", wasm_instance_methods);
    wasm_instance_ce = zend_register_internal_class(&ce);
    wasm_instance_ce->create_object = wasm_instance_create_object;
    memcpy(&wasm_instance_object_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
    wasm_instance_object_handlers.clone_obj = NULL;
    wasm_instance_object_handlers.free_obj = wasm_instance_free_obj;

    /* WasmMemory */
    INIT_CLASS_ENTRY(ce, "WasmMemory", wasm_memory_methods);
    wasm_memory_ce = zend_register_internal_class(&ce);
    wasm_memory_ce->create_object = wasm_memory_create_object;
    memcpy(&wasm_memory_object_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
    wasm_memory_object_handlers.clone_obj = NULL;
    wasm_memory_object_handlers.free_obj = wasm_memory_free_obj;

    /* WasmGlobal */
    INIT_CLASS_ENTRY(ce, "WasmGlobal", wasm_global_methods);
    wasm_global_ce = zend_register_internal_class(&ce);
    wasm_global_ce->create_object = wasm_global_create_object;
    memcpy(&wasm_global_object_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
    wasm_global_object_handlers.clone_obj = NULL;
    wasm_global_object_handlers.free_obj = wasm_global_free_obj;

    return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION(wasmtime)
{
    return SUCCESS;
}

PHP_RINIT_FUNCTION(wasmtime)
{
#if defined(COMPILE_DL_WASMTIME) && defined(ZTS)
    ZEND_TSRMLS_CACHE_UPDATE();
#endif
    return SUCCESS;
}

PHP_RSHUTDOWN_FUNCTION(wasmtime)
{
    return SUCCESS;
}

PHP_MINFO_FUNCTION(wasmtime)
{
    php_info_print_table_start();
    php_info_print_table_header(2, "wasmtime support", "enabled");
    php_info_print_table_end();
}

/* Define the module entry */
zend_module_entry wasmtime_module_entry = {
    STANDARD_MODULE_HEADER,
    "wasmtime",               /* Extension name */
    NULL,                     /* Functions (none at global scope) */
    PHP_MINIT(wasmtime),      /* MINIT */
    PHP_MSHUTDOWN(wasmtime),  /* MSHUTDOWN */
    PHP_RINIT(wasmtime),      /* RINIT */
    PHP_RSHUTDOWN(wasmtime),  /* RSHUTDOWN */
    PHP_MINFO(wasmtime),      /* MINFO */
    NO_VERSION_YET,
    STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_WASMTIME
#ifdef ZTS
ZEND_TSRMLS_CACHE_DEFINE()
#endif
ZEND_GET_MODULE(wasmtime)
#endif
