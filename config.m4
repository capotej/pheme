dnl $Id$
dnl config.m4 for pheme extension

dnl Extension name
PHP_ARG_ENABLE(pheme,
    [whether to enable pheme extension],
    [ --enable-pheme           Enable pheme extension])

dnl Check if extension should be built
if test "$PHP_PHEME" != "no"; then

    dnl Find libguile using pkg-config
    PKG_PROG_PKG_CONFIG()

    dnl Try guile-3.0
    PKG_CHECK_MODULES([GUILE], [guile-3.0 >= 3.0.0],
        [found_guile=yes],
        [found_guile=no])

    dnl Try guile-2.2 if 3.0 not found
    if test "$found_guile" != "yes"; then
        PKG_CHECK_MODULES([GUILE], [guile-2.2 >= 2.2.0],
            [found_guile=yes],
            [found_guile=no])
    fi

    dnl Try guile-2.0 if 2.2 not found
    if test "$found_guile" != "yes"; then
        PKG_CHECK_MODULES([GUILE], [guile-2.0 >= 2.0.0],
            [found_guile=yes],
            [found_guile=no])
    fi

    if test "$found_guile" = "yes"; then
        dnl Make variables available to the script
        AC_SUBST([GUILE_CFLAGS])
        AC_SUBST([GUILE_LIBS])
        
        dnl Add flags
        INCLUDES="$INCLUDES $GUILE_CFLAGS"
        LDFLAGS="$LDFLAGS $GUILE_LIBS"
        
        AC_DEFINE([HAVE_GUILE], [1], [Whether you have GNU Guile])
    else
        AC_MSG_ERROR([Unable to find libguile. Please install Guile 2.0 or higher.])
    fi

    dnl Check for required Guile functions
    AC_CHECK_FUNCS([scm_with_guile scm_init_guile scm_eval_string scm_current_module])

    dnl Add source file
    PHP_NEW_EXTENSION(pheme, pheme.c, $ext_shared)

    dnl Add build flags
    PHP_ADD_MAKEFILE_FRAGMENT
fi
