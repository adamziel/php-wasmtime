PHP_ARG_WITH([wasmtime],
  [for Wasmtime support],
  [--with-wasmtime[=DIR]    Include Wasmtime C API], [])
if test "$PHP_WASMTIME" != "no"; then
  WASMTIME_CFLAGS="-I$PHP_WASMTIME/include"
  WASMTIME_LIBS="-L$PHP_WASMTIME/lib -lwasmtime"
  PHP_EVAL_INCLINE($WASMTIME_CFLAGS)
  PHP_EVAL_LIBLINE($WASMTIME_LIBS, WASMTIME_SHARED_LIBADD)

  PHP_CHECK_LIBRARY([wasmtime], [wasm_engine_new],
    [AC_DEFINE([HAVE_WASMTIME], [1],
      [Define to 1 if wasmtime library has the 'wasm_engine_new' function
      (available since 1.0.0).])],
    [AC_MSG_WARN([wasmtime >= 1.0.0 needed for setting mtime])],
    [$WASMTIME_LIBS])

  AC_DEFINE([HAVE_WASMTIME],[1],[Have Wasmtime C API])
  PHP_NEW_EXTENSION(wasmtime, php_wasmtime.c, $ext_shared)
  
  # Ensure the extension is linked with libwasmtime
  PHP_ADD_LIBRARY_WITH_PATH(wasmtime, $PHP_WASMTIME/lib, WASMTIME_SHARED_LIBADD)

  PHP_SUBST([WASMTIME_SHARED_LIBADD])
fi