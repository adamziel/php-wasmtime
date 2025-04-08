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
    if(!f) return 0;
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    if(size <= 0) {
        fclose(f);
        return 0;
    }
    *buffer = emalloc(size);
    if(!*buffer) {
        fclose(f);
        return 0;
    }
    size_t read_bytes = fread(*buffer, 1, size, f);
    fclose(f);
    return read_bytes;
}

/*-------------------------------------------
  ENGINE (WasmEngine)
--------------------------------------------*/
static zend_object* wasm_engine_create_object(zend_class_entry *class_type) {
    php_wasm_engine_t *intern = ecalloc(1, sizeof(php_wasm_engine_t) + zend_object_properties_size(class_type));
    zend_object_std_init(&intern->std, class_type);
    object_properties_init(&intern->std, class_type);
    intern->std.handlers = &wasm_engine_object_handlers;
    return &intern->std;
}

PHP_METHOD(WasmEngine, __construct)
{
    printf("constructor initiated\n");
    php_wasm_engine_t *engine_obj = Z_WASMENGINE_P(getThis());
    printf("wasm engine object created\n");
    engine_obj->engine = wasm_engine_new();
	printf("wasm engine created\n");
    if (!engine_obj->engine) {
        zend_throw_exception(NULL, "Failed to create wasm_engine_t", 0);
        return;
    }
	printf("wasm engine store created\n");
    engine_obj->store = wasmtime_store_new(engine_obj->engine, NULL, NULL);
	printf("wasm engine store created\n");
    if (!engine_obj->store) {
        zend_throw_exception(NULL, "Failed to create wasmtime_store_t", 0);
        return;
    }
}

static void wasm_engine_free_obj(zend_object *object) {
    php_wasm_engine_t *intern = (php_wasm_engine_t *)((char*)(object) - XtOffsetOf(php_wasm_engine_t, std));
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
static zend_object *wasm_module_create_object(zend_class_entry *class_type) {
    php_wasm_module_t *intern = ecalloc(1, sizeof(php_wasm_module_t) + zend_object_properties_size(class_type));
    zend_object_std_init(&intern->std, class_type);
    object_properties_init(&intern->std, class_type);
    intern->std.handlers = &wasm_module_object_handlers;
    return &intern->std;
}

static void wasm_module_free_obj(zend_object *object) {
    php_wasm_module_t *intern = (php_wasm_module_t *)((char*)(object) - XtOffsetOf(php_wasm_module_t, std));
    if (intern->module) {
        wasmtime_module_delete(intern->module);
        intern->module = NULL;
    }
    zend_object_std_dtor(&intern->std);
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

    php_wasm_engine_t *engine_obj = Z_WASMENGINE_P(engine_zv);
    php_wasm_module_t *module_obj = Z_WASMMODULE_P(getThis());
    module_obj->engine_obj = engine_obj;

    uint8_t *wasm_bytes = NULL;
    size_t wasm_size = read_wasm_file(filename, &wasm_bytes);
    if (wasm_size == 0 || !wasm_bytes) {
        zend_throw_exception(NULL, "Could not read WASM file", 0);
        return;
    }

    wasmtime_module_t *mod = NULL;
    wasmtime_error_t *error = wasmtime_module_new(engine_obj->engine, wasm_bytes, wasm_size, &mod);
    efree(wasm_bytes);

    if (error) {
        wasm_byte_vec_t msg;
        wasmtime_error_message(error, &msg);
        wasmtime_error_delete(error);
        zend_throw_exception(NULL, (char*)msg.data, 0);
        wasm_byte_vec_delete(&msg);
        return;
    }
    module_obj->module = mod;
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
    php_wasm_instance_t *intern = ecalloc(1, sizeof(php_wasm_instance_t) + zend_object_properties_size(class_type));
    zend_object_std_init(&intern->std, class_type);
    object_properties_init(&intern->std, class_type);
    intern->std.handlers = &wasm_instance_object_handlers;
    return &intern->std;
}

static void wasm_instance_free_obj(zend_object *object) {
    /* wasmtime_instance_t doesn't require a delete function by itself;
       it's stored inside the store. We'll just free the PHP object. */
    php_wasm_instance_t *intern = (php_wasm_instance_t *)((char*)(object) - XtOffsetOf(php_wasm_instance_t, std));
    zend_object_std_dtor(&intern->std);
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
	printf("php_host_func_callback\n");
    php_host_func_env *fn_env = (php_host_func_env *)env;

    // Prepare call to PHP function
    // Convert the wasmtime_val_t arguments to zvals.
    zval php_retval;
    ZVAL_NULL(&php_retval);

    zval php_args[16]; /* simple limit of 16 for example */
    if (nargs > 16) {
        // Return a trap if too many arguments. Could be made more dynamic.
        // But let's create no actual trap content to keep example short.
        return wasm_trap_new(wasmtime_caller_context(caller), NULL);
    }

    for (size_t i = 0; i < nargs; i++) {
        ZVAL_NULL(&php_args[i]);
		printf("php_args[%zu]: %p\n", i, &php_args[i]);
        switch (args[i].kind) {
            case WASMTIME_I32:
                ZVAL_LONG(&php_args[i], (zend_long)args[i].of.i32);
                break;
            case WASMTIME_I64:
                // If 64-bit is out of range, we'd handle that, ignoring details here.
                ZVAL_LONG(&php_args[i], (zend_long)args[i].of.i64);
                break;
            case WASMTIME_F32:
                ZVAL_DOUBLE(&php_args[i], (double)args[i].of.f32);
                break;
            case WASMTIME_F64:
                ZVAL_DOUBLE(&php_args[i], (double)args[i].of.f64);
                break;
            default:
                ZVAL_NULL(&php_args[i]);
        }
    }

    zend_fcall_info fci;
    zend_fcall_info_cache fcc;
    memset(&fci, 0, sizeof(fci));
    memset(&fcc, 0, sizeof(fcc));

    fci.size = sizeof(fci);
    fci.object = NULL;
    ZVAL_COPY_VALUE(&fci.function_name, &fn_env->callable);
    fci.param_count = (uint32_t) nargs;
    fci.params = php_args;
    fci.retval = &php_retval;

	printf("logging fci: %p\n", &fci);

    if (zend_fcall_info_init(&fn_env->callable, 0, &fci, &fcc, NULL, NULL) == FAILURE) {
		printf("zend_fcall_info_init failed\n");
        // If we can't initialize the call info, we can't invoke the function.
        return wasm_trap_new(wasmtime_caller_context(caller), NULL);
    }

	printf("about to call zend_call_function\n");
    printf("fci details:\n");
    printf("  size: %zu\n", fci.size);
    printf("  param_count: %u\n", fci.param_count);
    printf("  params: %p\n", fci.params);
    printf("  retval: %p\n", fci.retval);
    
    printf("fcc details:\n");
    printf("  function_handler: %p\n", fcc.function_handler);
    printf("  calling_scope: %p\n", fcc.calling_scope);
    printf("  called_scope: %p\n", fcc.called_scope);
    printf("  object: %p\n", fcc.object);

    if (zend_call_function(&fci, &fcc) != SUCCESS) {
		printf("zend_call_function failed\n");
        // If the PHP call fails, we could produce a trap.
        return wasm_trap_new(wasmtime_caller_context(caller), NULL);
    }

	printf("after conditions\n");

	/* Convert return value to WASM results (assuming at most 1 result) */
	if (nresults > 0) {
		const wasm_functype_t *func_ty = wasmtime_func_type(caller, NULL);
		if (!func_ty) {
			return wasm_trap_new(wasmtime_caller_context(caller), NULL);
		}

		const wasm_valtype_vec_t *results_ty = wasm_functype_results(func_ty);
		if (results_ty->size != nresults) {
			wasm_functype_delete((wasm_functype_t *)func_ty);
			return wasm_trap_new(wasmtime_caller_context(caller), NULL);
		}

		wasm_valkind_t expected_kind = wasm_valtype_kind(results_ty->data[0]);

		switch (expected_kind) {
			case WASM_I32:
				if (Z_TYPE(php_retval) != IS_LONG) {
					wasm_functype_delete((wasm_functype_t *)func_ty);
					return wasm_trap_new(wasmtime_caller_context(caller), NULL);
				}
				results[0].kind = WASMTIME_I32;
				results[0].of.i32 = (int32_t)Z_LVAL(php_retval);
				break;

			case WASM_I64:
				if (Z_TYPE(php_retval) != IS_LONG) {
					wasm_functype_delete((wasm_functype_t *)func_ty);
					return wasm_trap_new(wasmtime_caller_context(caller), NULL);
				}
				results[0].kind = WASMTIME_I64;
				results[0].of.i64 = (int64_t)Z_LVAL(php_retval);
				break;

			case WASM_F32:
			case WASM_F64:
				if (Z_TYPE(php_retval) != IS_DOUBLE) {
					wasm_functype_delete((wasm_functype_t *)func_ty);
					return wasm_trap_new(wasmtime_caller_context(caller), NULL);
				}
				results[0].kind = (expected_kind == WASM_F32) ? WASMTIME_F32 : WASMTIME_F64;
				results[0].of.f64 = Z_DVAL(php_retval);  // both store in .f64
				break;

			default:
				wasm_functype_delete((wasm_functype_t *)func_ty);
				return wasm_trap_new(wasmtime_caller_context(caller), NULL);
		}

		wasm_functype_delete((wasm_functype_t *)func_ty);
	}

    zval_dtor(&php_retval);
    for (size_t i = 0; i < nargs; i++) {
        zval_dtor(&php_args[i]);
    }

    return NULL; /* no trap */
}

/* Freed when the function is destroyed from the store */
static void php_host_func_finalizer(void *env) {
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
	printf("Hi\n"); 
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

    php_wasm_engine_t  *engine_obj  = Z_WASMENGINE_P(engine_zv);
    php_wasm_module_t  *module_obj  = Z_WASMMODULE_P(module_zv);
    php_wasm_instance_t *inst_obj   = Z_WASMINSTANCE_P(getThis());
    inst_obj->engine_obj = engine_obj;

    wasmtime_context_t *context = wasmtime_store_context(engine_obj->store);
	printf("context: %p\n", context);

    // Retrieve expected imports from module
    wasm_importtype_vec_t import_types;
    wasmtime_module_imports(module_obj->module, &import_types);

    wasmtime_extern_t *imports = NULL;
    size_t import_count = import_types.size;
    if (import_count > 0) {
        imports = ecalloc(import_count, sizeof(wasmtime_extern_t));
    }
	printf("imports: %p\n", imports);

    // If user provided an imports array, we attempt name-based matching
    // In a real extension, we'd do better checking. For demonstration, we do partial matching by name.
    HashTable *ht = NULL;
    if (imports_zv && Z_TYPE_P(imports_zv) == IS_ARRAY) {
        ht = Z_ARRVAL_P(imports_zv);
    }
	printf("ht: %p\n", ht);
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

						printf("found callable: %p, type: %d, name: %s, import: %.*s.%.*s\n", 
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
						printf("func: %p\n", func);
                        printf("Assigned PHP callable to import %.*s.%.*s\n", 
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
                        php_wasm_memory_t *mem_obj = Z_WASMMEMORY_P(found);
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
                        php_wasm_global_t *glob_obj = Z_WASMGLOBAL_P(found);
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
	printf("after loop\n");

	// @TODO: this causes a double free error, but
	//        commenting it out causes a memory leak.
	//        Let's investigate and debug this.
    // wasm_importtype_vec_delete(&import_types);

	printf("vector deleted\n");

    wasm_trap_t *trap = NULL;
    wasmtime_instance_t instance;
    wasmtime_error_t *error = wasmtime_instance_new(
        context,
        module_obj->module,
        imports, import_count,
        &instance,
        &trap
    );

	printf("instance: %p\n", instance);
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

    php_wasm_instance_t *inst_obj = Z_WASMINSTANCE_P(getThis());
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

    // We'll assume one result max for demonstration
    wasmtime_val_t results[1];
    memset(results, 0, sizeof(results));
    wasm_trap_t *trap = NULL;

    wasmtime_error_t *err = wasmtime_func_call(
        context, &func,
        args, arg_count,
        results, 1,
        &trap
    );

    if (args) efree(args);

    if (err != NULL) {
        wasm_byte_vec_t msg;
        wasmtime_error_message(err, &msg);
        wasmtime_error_delete(err);
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


    switch (results[0].kind) {
        case WASMTIME_I32:
            RETURN_LONG(results[0].of.i32);
        case WASMTIME_I64:
            RETURN_LONG((zend_long)results[0].of.i64);
        case WASMTIME_F32:
            RETURN_DOUBLE((double)results[0].of.f32);
        case WASMTIME_F64:
            RETURN_DOUBLE(results[0].of.f64);
        default:
            RETURN_NULL();
    }
}

PHP_METHOD(WasmInstance, getMemory)
{
    char *mem_name = "memory";
    size_t mem_name_len = 6;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "|s", &mem_name, &mem_name_len) == FAILURE) {
        RETURN_THROWS();
    }

    php_wasm_instance_t *inst = Z_WASMINSTANCE_P(getThis());
    wasmtime_context_t *ctx = wasmtime_store_context(inst->engine_obj->store);

    wasmtime_extern_t item;
    bool found = wasmtime_instance_export_get(ctx, &inst->instance, mem_name, mem_name_len, &item);
    if (!found || item.kind != WASMTIME_EXTERN_MEMORY) {
        zend_throw_exception(NULL, "Memory export not found", 0);
        RETURN_THROWS();
    }

    object_init_ex(return_value, wasm_memory_ce);
    php_wasm_memory_t *mem_obj = Z_WASMMEMORY_P(return_value);
	fprintf(stderr, "inst->engine_obj: %p\n", inst->engine_obj);
	mem_obj->memory = item.of.memory;
	fprintf(stderr, "mem_obj->memory: %p\n", mem_obj->memory);
	ZVAL_OBJ_COPY(&mem_obj->engine_zv, &inst->engine_obj->std);
	fprintf(stderr, "mem_obj->engine_zv: %p\n", mem_obj->engine_zv);

	fprintf(stderr, "returning memory object\n");
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
    php_wasm_memory_t *intern = ecalloc(1, sizeof(php_wasm_memory_t) + zend_object_properties_size(class_type));
    zend_object_std_init(&intern->std, class_type);
    object_properties_init(&intern->std, class_type);
    intern->std.handlers = &wasm_memory_object_handlers;
    return &intern->std;
}

static void wasm_memory_free_obj(zend_object *object)
{
    php_wasm_memory_t *intern = (php_wasm_memory_t *)((char*)(object) - XtOffsetOf(php_wasm_memory_t, std));
    zend_object_std_dtor(&intern->std);
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

PHP_METHOD(WasmMemory, __construct)
{
    zval *engine_zv;
    zend_long initial_pages, max_pages = -1;
    ZEND_PARSE_PARAMETERS_START(2, 3)
        Z_PARAM_OBJECT_OF_CLASS(engine_zv, wasm_engine_ce)
        Z_PARAM_LONG(initial_pages)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(max_pages)
    ZEND_PARSE_PARAMETERS_END();

    php_wasm_engine_t *engine_obj = Z_WASMENGINE_P(engine_zv);
    php_wasm_memory_t *mem_obj    = Z_WASMMEMORY_P(getThis());
    ZVAL_OBJ_COPY(&mem_obj->engine_zv, engine_zv);

    wasmtime_context_t *context = wasmtime_store_context(engine_obj->store);

    wasm_limits_t limits;
    limits.min = (uint32_t) initial_pages;
    if (max_pages < 0) {
        // no max
        limits.max = wasm_limits_max_default;
    } else {
        limits.max = (uint32_t) max_pages;
    }
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

    mem_obj->memory = memory;
}

PHP_METHOD(WasmMemory, dataSize)
{
    php_wasm_memory_t *mem_obj = Z_WASMMEMORY_P(getThis());
    php_wasm_engine_t *engine = Z_WASMENGINE_P(&mem_obj->engine_zv);
    wasmtime_context_t *context = wasmtime_store_context(engine->store);
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

	printf("offset: %ld, length: %ld\n", offset, length);

	printf("before Z_WASMMEMORY_P\n");
    php_wasm_memory_t *mem_obj = Z_WASMMEMORY_P(getThis());
	printf("after Z_WASMMEMORY_P\n");
	php_wasm_engine_t *engine = Z_WASMENGINE_P(&mem_obj->engine_zv);
    wasmtime_context_t *context = wasmtime_store_context(engine->store);

	printf("after wasmtime_store_context\n");
    size_t sz = wasmtime_memory_data_size(context, &mem_obj->memory);
    if (offset < 0 || length < 0 || (size_t)offset + (size_t)length > sz) {
        zend_throw_exception(NULL, "Memory read out of bounds", 0);
        return;
    }

	printf("context: %p\n", context);
    uint8_t *data = wasmtime_memory_data(context, &mem_obj->memory);
	printf("data: %p\n", data);
	/**
	 * Duplicate the string to avoid double free issue – the WASM
	 * module likely manages its own memory and will free the memory
	 * when done.
	 */
	 char *copy = emalloc(length + 1);
	memcpy(copy, data + offset, length);
	copy[length] = '\0';
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

    php_wasm_memory_t *mem_obj = Z_WASMMEMORY_P(getThis());
    php_wasm_engine_t *engine = Z_WASMENGINE_P(&mem_obj->engine_zv);
    wasmtime_context_t *context = wasmtime_store_context(engine->store);

    size_t sz = wasmtime_memory_data_size(context, &mem_obj->memory);
    if (offset < 0 || (size_t)offset + buf_len > sz) {
        zend_throw_exception(NULL, "Memory write out of bounds", 0);
        return;
    }

    uint8_t *data = wasmtime_memory_data(context, &mem_obj->memory);
    memcpy(data + offset, buf, buf_len);
}

PHP_METHOD(WasmMemory, size)
{
    php_wasm_memory_t *mem_obj = Z_WASMMEMORY_P(getThis());
    php_wasm_engine_t *engine = Z_WASMENGINE_P(&mem_obj->engine_zv);
    wasmtime_context_t *context = wasmtime_store_context(engine->store);
    size_t pages = wasmtime_memory_size(context, &mem_obj->memory);
    RETURN_LONG((zend_long)pages);
}

PHP_METHOD(WasmMemory, grow)
{
    zend_long additional;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_LONG(additional)
    ZEND_PARSE_PARAMETERS_END();

    php_wasm_memory_t *mem_obj = Z_WASMMEMORY_P(getThis());
    php_wasm_engine_t *engine = Z_WASMENGINE_P(&mem_obj->engine_zv);
    wasmtime_context_t *context = wasmtime_store_context(engine->store);

    uint64_t previous_size;
    wasmtime_error_t *err = wasmtime_memory_grow(context, &mem_obj->memory, (uint64_t)additional, &previous_size);
    if (err) {
        wasmtime_error_delete(err);
        RETURN_FALSE;
    }
    RETURN_LONG(previous_size);
}


static const zend_function_entry wasm_memory_methods[] = {
    PHP_ME(WasmMemory, __construct, arginfo_wasmmemory_construct, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
    PHP_ME(WasmMemory, dataSize,    arginfo_wasmmemory_datasize,   ZEND_ACC_PUBLIC)
    PHP_ME(WasmMemory, read,        arginfo_wasmmemory_read,       ZEND_ACC_PUBLIC)
    PHP_ME(WasmMemory, write,       arginfo_wasmmemory_write,      ZEND_ACC_PUBLIC)
    PHP_ME(WasmMemory, size,        arginfo_wasmmemory_size,       ZEND_ACC_PUBLIC)
    PHP_ME(WasmMemory, grow,        arginfo_wasmmemory_grow,       ZEND_ACC_PUBLIC)
    PHP_FE_END
};


/*-------------------------------------------
  GLOBAL (WasmGlobal)
--------------------------------------------*/
static zend_object *wasm_global_create_object(zend_class_entry *class_type) {
    php_wasm_global_t *intern = ecalloc(1, sizeof(php_wasm_global_t) + zend_object_properties_size(class_type));
    zend_object_std_init(&intern->std, class_type);
    object_properties_init(&intern->std, class_type);
    intern->std.handlers = &wasm_global_object_handlers;
    return &intern->std;
}

static void wasm_global_free_obj(zend_object *object) {
    php_wasm_global_t *intern = (php_wasm_global_t *)((char*)(object) - XtOffsetOf(php_wasm_global_t, std));
    zend_object_std_dtor(&intern->std);
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

    php_wasm_engine_t *engine_obj = Z_WASMENGINE_P(engine_zv);
    php_wasm_global_t *global_obj = Z_WASMGLOBAL_P(getThis());
    global_obj->engine_obj = engine_obj;

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
    php_wasm_global_t *global_obj = Z_WASMGLOBAL_P(getThis());
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

    php_wasm_global_t *global_obj = Z_WASMGLOBAL_P(getThis());
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
