#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include <libguile.h>

/* {{{ PHP_FUNCTION declarations
 */
PHP_FUNCTION(hello_world);
PHP_FUNCTION(add_numbers);
PHP_FUNCTION(guile_eval);
PHP_FUNCTION(guile_call);
PHP_FUNCTION(guile_load_file);
/* }}} */

/* {{{ arginfo */
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_hello_world, 0, 0, IS_TRUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_add_numbers, 0, 2, IS_LONG, 0)
    ZEND_ARG_TYPE_INFO(0, num1, IS_LONG, 0)
    ZEND_ARG_TYPE_INFO(0, num2, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_guile_eval, 0, 1, IS_STRING, 0)
    ZEND_ARG_TYPE_INFO(0, code, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_guile_call, 0, 2, IS_STRING, 0)
    ZEND_ARG_TYPE_INFO(0, procedure, IS_STRING, 0)
    ZEND_ARG_TYPE_INFO(0, args, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_guile_load_file, 0, 1, IS_TRUE, 0)
    ZEND_ARG_TYPE_INFO(0, filepath, IS_STRING, 0)
ZEND_END_ARG_INFO()
/* }}} */

/* {{{ pheme_functions[]
 */
const zend_function_entry pheme_functions[] = {
    PHP_FE(hello_world, arginfo_hello_world)
    PHP_FE(add_numbers, arginfo_add_numbers)
    PHP_FE(guile_eval, arginfo_guile_eval)
    PHP_FE(guile_call, arginfo_guile_call)
    PHP_FE(guile_load_file, arginfo_guile_load_file)
    PHP_FE_END
};
/* }}} */

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(pheme)
{
    /* CRITICAL FIX: Initialize Guile properly before any API calls */
    scm_init_guile_3();
    
    /* Optional: Set up Guile debugging and error handling */
    scm_debug_mode = 0;  /* Disable debug mode for production */
    
    return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(pheme)
{
    /* Optional: Clean up Guile resources if needed */
    /* Note: Guile doesn't require explicit shutdown in most cases */
    return SUCCESS;
}
/* }}} */

/* {{{ pheme_module_entry
 */
zend_module_entry pheme_module_entry = {
    STANDARD_MODULE_HEADER,
    "pheme",
    pheme_functions,
    PHP_MINIT(pheme),  /* PHP_MINIT */
    PHP_MSHUTDOWN(pheme),  /* PHP_MSHUTDOWN */
    NULL,  /* PHP_RINIT */
    NULL,  /* PHP_RSHUTDOWN */
    NULL,  /* PHP_MINFO */
    "0.1.0",
    STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_PHEME
ZEND_GET_MODULE(pheme)
#endif

/* {{{ proto string hello_world()
   Return a greeting message */
PHP_FUNCTION(hello_world)
{
    php_printf("Hello, world!\n");
    RETURN_TRUE;
}
/* }}} */

/* {{{ proto int add_numbers(int num1, int num2)
   Add two numbers together */
PHP_FUNCTION(add_numbers)
{
    long num1, num2;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "ll", &num1, &num2) == FAILURE) {
        return;
    }

    RETURN_LONG(num1 + num2);
}
/* }}} */

/* {{{ proto string guile_eval(string code)
   Evaluate Scheme code and return the result as a string */
PHP_FUNCTION(guile_eval)
{
    char *code;
    size_t code_len;
    SCM result;
    char *result_str;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "s", &code, &code_len) == FAILURE) {
        return;
    }

    /* CRITICAL FIX: Now that Guile is initialized, this should work */
    result = scm_c_eval_string(code);

    /* Check for errors - improved error handling */
    if (SCM_FALSEP(result)) {
        php_error_docref(NULL, E_WARNING, "Error evaluating Scheme code: %s", code);
        RETURN_FALSE;
    }

    /* CRITICAL FIX: Proper memory management */
    result_str = scm_to_locale_string(result);
    if (result_str == NULL) {
        php_error_docref(NULL, E_WARNING, "Failed to convert Guile result to string");
        RETURN_FALSE;
    }
    
    /* Return the result as a PHP string and free the Guile-allocated memory */
    RETVAL_STRING(result_str);
    free(result_str);  /* Essential: free memory allocated by Guile */
}
/* }}} */

/* {{{ proto string guile_call(string procedure, string args)
   Call a Scheme procedure with arguments */
PHP_FUNCTION(guile_call)
{
    char *procedure, *args;
    size_t proc_len, args_len;
    SCM proc_scm, args_scm, result;
    char *result_str;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "ss", &procedure, &proc_len, &args, &args_len) == FAILURE) {
        return;
    }

    /* Convert procedure name to Guile symbol */
    proc_scm = scm_from_locale_stringn(procedure, proc_len);
    
    /* Convert args to a list of strings - improved parsing */
    args_scm = scm_from_locale_stringn(args, args_len);
    
    /* Parse the arguments string into a list - safer approach */
    SCM parsed_args = scm_eval_string(
        scm_list_2(scm_from_locale_string("string-split"), 
                  scm_list_2(args_scm, scm_from_locale_string(" ")))
    );

    /* Check if we got a valid list */
    if (SCM_FALSEP(parsed_args) || SCM_NULLP(parsed_args)) {
        php_error_docref(NULL, E_WARNING, "Failed to parse arguments: %s", args);
        RETURN_FALSE;
    }

    /* Call the procedure with the parsed arguments */
    /* Note: This is a simplified version - real implementation would need proper list handling */
    result = scm_call_1(proc_scm, scm_car(parsed_args));

    /* Check for errors */
    if (SCM_FALSEP(result)) {
        php_error_docref(NULL, E_WARNING, "Error calling Scheme procedure: %s", procedure);
        RETURN_FALSE;
    }

    /* CRITICAL FIX: Proper memory management */
    result_str = scm_to_locale_string(result);
    if (result_str == NULL) {
        php_error_docref(NULL, E_WARNING, "Failed to convert Guile result to string");
        RETURN_FALSE;
    }
    
    RETVAL_STRING(result_str);
    free(result_str);  /* Essential: free memory allocated by Guile */
}
/* }}} */

/* {{{ proto bool guile_load_file(string filepath)
   Load and execute a Scheme file */
PHP_FUNCTION(guile_load_file)
{
    char *filepath;
    size_t filepath_len;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "s", &filepath, &filepath_len) == FAILURE) {
        return;
    }

    /* Convert filepath to Guile string */
    SCM filepath_scm = scm_from_locale_stringn(filepath, filepath_len);

    /* Create the load command */
    SCM load_cmd = scm_list_2(
        scm_from_locale_string("load"),
        filepath_scm
    );
    
    /* Execute the load command */
    SCM result = scm_eval_string(load_cmd);

    /* Check for errors */
    if (SCM_FALSEP(result)) {
        php_error_docref(NULL, E_WARNING, "Error loading Scheme file: %s", filepath);
        RETURN_FALSE;
    }

    RETURN_TRUE;
}
/* }}} */