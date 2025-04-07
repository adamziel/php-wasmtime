PHP_ARG_WITH([wasmtime],
  [for Wasmtime support],
  [--with-wasmtime[=DIR]    Include Wasmtime C API], [])
if test "$PHP_WASMTIME" != "no"; then
  WASMTIME_CFLAGS="-I$PHP_WASMTIME/include"
  WASMTIME_LIBS="-L$PHP_WASMTIME/lib -lwasmtime"
  PHP_EVAL_INCLINE($WASMTIME_CFLAGS)
  PHP_EVAL_LIBLINE($WASMTIME_LIBS, WASMTIME_SHARED_LIBADD)
  AC_DEFINE([HAVE_WASMTIME],[1],[Have Wasmtime C API])
  PHP_NEW_EXTENSION(wasmtime, php_wasmtime.c, $ext_shared)
fi