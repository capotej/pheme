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
    /* DIAGNOSTIC: Log module initialization */
    php_error_docref(NULL, E_NOTICE, "PHeme: Module initialization started");
    fprintf(stderr, "[PHP_MINIT] Module initialization started\n");
    
    /* CRITICAL FIX: Initialize Guile explicitly before any scm_with_guile calls
     * According to Guile docs: "scm_init_guile() arranges things so that all
     * of the code in the current thread executes as if from within a call to
     * scm_with_guile." */
    
    fprintf(stderr, "[PHP_MINIT] About to call scm_init_guile()\n");
    
    /* Initialize Guile for the current thread */
    scm_init_guile();
    
    fprintf(stderr, "[PHP_MINIT] scm_init_guile() returned successfully\n");
    php_error_docref(NULL, E_NOTICE, "PHeme: Guile initialized successfully");
    
    php_error_docref(NULL, E_NOTICE, "PHeme: Module initialization completed");
    
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

/* Helper function to execute Guile evaluation in proper context */
static void* guile_eval_helper(void* data) {
    char* code = (char*)data;
    SCM result, result_as_string;
    char* result_str;
    
    /* DIAGNOSTIC: Cannot use PHP logging here - we're in Guile thread context */
    /* Use stderr for diagnostics */
    fprintf(stderr, "[GUILE_EVAL_HELPER] Entered helper function\n");
    fprintf(stderr, "[GUILE_EVAL_HELPER] Code to evaluate: %s\n", code);
    
    /* CRITICAL: Do NOT call PHP functions inside Guile context
     * Only Guile API calls should be made here */
    
    fprintf(stderr, "[GUILE_EVAL_HELPER] About to call scm_c_eval_string\n");
    
    /* Evaluate the Scheme code */
    result = scm_c_eval_string(code);
    
    fprintf(stderr, "[GUILE_EVAL_HELPER] scm_c_eval_string returned\n");
    
    /* Check for errors - NOTE: SCM_FALSEP only checks for #f, not errors! */
    if (SCM_FALSEP(result)) {
        fprintf(stderr, "[GUILE_EVAL_HELPER] Result is #f (false)\n");
        return NULL;
    }
    
    fprintf(stderr, "[GUILE_EVAL_HELPER] About to convert result to string\n");
    
    /* CRITICAL FIX: Convert any type to string using scm_object_to_string
     * This handles numbers, symbols, lists, etc. - not just strings */
    result_as_string = scm_object_to_string(result, SCM_UNDEFINED);
    
    /* Now convert the Scheme string to C string */
    result_str = scm_to_locale_string(result_as_string);
    
    fprintf(stderr, "[GUILE_EVAL_HELPER] Conversion complete, result_str: %s\n",
            result_str ? result_str : "NULL");
    
    return result_str;
}

/* {{{ proto string guile_eval(string code)
   Evaluate Scheme code and return the result as a string */
PHP_FUNCTION(guile_eval)
{
    char *code;
    size_t code_len;
    char *result_str;
    char *code_copy;  /* Persistent copy for Guile context */

    /* DIAGNOSTIC: Log function entry */
    php_error_docref(NULL, E_NOTICE, "PHeme: guile_eval called");

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "s", &code, &code_len) == FAILURE) {
        php_error_docref(NULL, E_WARNING, "PHeme: Failed to parse parameters");
        return;
    }

    /* DIAGNOSTIC: Log the code parameter */
    php_error_docref(NULL, E_NOTICE, "PHeme: Code to evaluate: '%s'", code);

    /* CRITICAL FIX: Copy PHP string to persistent memory before passing to scm_with_guile
     * The PHP string pointer may become invalid when scm_with_guile switches contexts.
     * We must use estrndup to allocate memory that persists across context switches. */
    code_copy = estrndup(code, code_len);
    
    /* DIAGNOSTIC: Log before scm_with_guile */
    php_error_docref(NULL, E_NOTICE, "PHeme: About to call scm_with_guile with copied string");

    /* CRITICAL FIX: Use scm_with_guile for thread-safe execution with persistent copy */
    result_str = (char*)scm_with_guile(guile_eval_helper, code_copy);
    
    /* CRITICAL FIX: Free the copied string after Guile call completes */
    efree(code_copy);
    
    /* DIAGNOSTIC: Log after scm_with_guile (won't reach if segfault) */
    php_error_docref(NULL, E_NOTICE, "PHeme: scm_with_guile returned");
    
    if (result_str == NULL) {
        php_error_docref(NULL, E_WARNING, "PHeme: Error evaluating Scheme code: %s", code);
        RETURN_FALSE;
    }
    
    /* DIAGNOSTIC: Log result */
    php_error_docref(NULL, E_NOTICE, "PHeme: Result from Guile: '%s'", result_str);
    
    /* Return the result as a PHP string and free the Guile-allocated memory */
    RETVAL_STRING(result_str);
    free(result_str);  /* Essential: free memory allocated by Guile */
}
/* }}} */

/* Helper structure for guile_call */
struct guile_call_data {
    char *procedure;
    char *args;
    char *result_str;
    int error;
};

/* Helper function to execute Guile call in proper context */
static void* guile_call_helper(void* data) {
    struct guile_call_data *call_data = (struct guile_call_data*)data;
    SCM proc_scm, args_scm, result;
    
    fprintf(stderr, "[GUILE_CALL_HELPER] Entered helper function\n");
    fprintf(stderr, "[GUILE_CALL_HELPER] Procedure: %s, Args: %s\n",
            call_data->procedure, call_data->args);
    
    call_data->error = 0;
    call_data->result_str = NULL;
    
    fprintf(stderr, "[GUILE_CALL_HELPER] Converting procedure to Guile symbol\n");
    
    /* Convert procedure name to Guile symbol */
    proc_scm = scm_from_locale_string(call_data->procedure);
    
    fprintf(stderr, "[GUILE_CALL_HELPER] Converting args to Guile string\n");
    
    /* Convert args to a list of strings - improved parsing */
    args_scm = scm_from_locale_string(call_data->args);
    
    fprintf(stderr, "[GUILE_CALL_HELPER] Parsing arguments\n");
    
    /* Parse the arguments string into a list - safer approach */
    SCM parsed_args = scm_eval_string(
        scm_list_2(scm_from_locale_string("string-split"),
                  scm_list_2(args_scm, scm_from_locale_string(" ")))
    );

    fprintf(stderr, "[GUILE_CALL_HELPER] Checking parsed args\n");
    
    /* Check if we got a valid list */
    if (SCM_FALSEP(parsed_args) || SCM_NULLP(parsed_args)) {
        fprintf(stderr, "[GUILE_CALL_HELPER] Failed to parse arguments\n");
        call_data->error = 1;
        return NULL;
    }

    fprintf(stderr, "[GUILE_CALL_HELPER] Calling procedure\n");
    
    /* Call the procedure with the parsed arguments */
    result = scm_call_1(proc_scm, scm_car(parsed_args));

    fprintf(stderr, "[GUILE_CALL_HELPER] Procedure call returned\n");
    
    /* Check for errors */
    if (SCM_FALSEP(result)) {
        fprintf(stderr, "[GUILE_CALL_HELPER] Result is #f (false)\n");
        call_data->error = 2;
        return NULL;
    }

    fprintf(stderr, "[GUILE_CALL_HELPER] Converting result to string\n");
    
    /* CRITICAL FIX: Proper memory management */
    call_data->result_str = scm_to_locale_string(result);
    
    fprintf(stderr, "[GUILE_CALL_HELPER] Conversion complete, result_str: %s\n",
            call_data->result_str ? call_data->result_str : "NULL");
    
    return NULL;
}

/* {{{ proto string guile_call(string procedure, string args)
   Call a Scheme procedure with arguments */
PHP_FUNCTION(guile_call)
{
    char *procedure, *args;
    size_t proc_len, args_len;
    struct guile_call_data call_data;

    php_error_docref(NULL, E_NOTICE, "PHeme: guile_call called");

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "ss", &procedure, &proc_len, &args, &args_len) == FAILURE) {
        php_error_docref(NULL, E_WARNING, "PHeme: Failed to parse parameters");
        return;
    }

    php_error_docref(NULL, E_NOTICE, "PHeme: Procedure: '%s', Args: '%s'", procedure, args);

    /* CRITICAL FIX: Copy strings to persistent memory */
    call_data.procedure = estrndup(procedure, proc_len);
    call_data.args = estrndup(args, args_len);
    call_data.result_str = NULL;
    call_data.error = 0;
    
    php_error_docref(NULL, E_NOTICE, "PHeme: About to call scm_with_guile for guile_call");
    
    /* CRITICAL FIX: Use scm_with_guile wrapper - THIS WAS MISSING! */
    scm_with_guile(guile_call_helper, &call_data);
    
    php_error_docref(NULL, E_NOTICE, "PHeme: scm_with_guile returned from guile_call");
    
    /* Free copied strings */
    efree(call_data.procedure);
    efree(call_data.args);
    
    /* Check for errors */
    if (call_data.error == 1) {
        php_error_docref(NULL, E_WARNING, "Failed to parse arguments: %s", args);
        RETURN_FALSE;
    }
    
    if (call_data.error == 2) {
        php_error_docref(NULL, E_WARNING, "Error calling Scheme procedure: %s", procedure);
        RETURN_FALSE;
    }
    
    if (call_data.result_str == NULL) {
        php_error_docref(NULL, E_WARNING, "Failed to convert Guile result to string");
        RETURN_FALSE;
    }
    
    php_error_docref(NULL, E_NOTICE, "PHeme: Result from Guile: '%s'", call_data.result_str);
    
    RETVAL_STRING(call_data.result_str);
    free(call_data.result_str);  /* Essential: free memory allocated by Guile */
}
/* }}} */

/* Helper structure for guile_load_file */
struct guile_load_data {
    char *filepath;
    int error;
};

/* Helper function to execute Guile file load in proper context */
static void* guile_load_helper(void* data) {
    struct guile_load_data *load_data = (struct guile_load_data*)data;
    SCM filepath_scm, load_cmd, result;
    
    fprintf(stderr, "[GUILE_LOAD_HELPER] Entered helper function\n");
    fprintf(stderr, "[GUILE_LOAD_HELPER] Filepath: %s\n", load_data->filepath);
    
    load_data->error = 0;
    
    fprintf(stderr, "[GUILE_LOAD_HELPER] Converting filepath to Guile string\n");
    
    /* Convert filepath to Guile string */
    filepath_scm = scm_from_locale_string(load_data->filepath);

    fprintf(stderr, "[GUILE_LOAD_HELPER] Creating load command\n");
    
    /* Create the load command */
    load_cmd = scm_list_2(
        scm_from_locale_string("load"),
        filepath_scm
    );
    
    fprintf(stderr, "[GUILE_LOAD_HELPER] Executing load command\n");
    
    /* Execute the load command */
    result = scm_eval_string(load_cmd);

    fprintf(stderr, "[GUILE_LOAD_HELPER] Load command executed\n");
    
    /* Check for errors */
    if (SCM_FALSEP(result)) {
        fprintf(stderr, "[GUILE_LOAD_HELPER] Result is #f (false) - load may have failed\n");
        load_data->error = 1;
        return NULL;
    }

    fprintf(stderr, "[GUILE_LOAD_HELPER] Load successful\n");
    
    return NULL;
}

/* {{{ proto bool guile_load_file(string filepath)
   Load and execute a Scheme file */
PHP_FUNCTION(guile_load_file)
{
    char *filepath;
    size_t filepath_len;
    struct guile_load_data load_data;

    php_error_docref(NULL, E_NOTICE, "PHeme: guile_load_file called");

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "s", &filepath, &filepath_len) == FAILURE) {
        php_error_docref(NULL, E_WARNING, "PHeme: Failed to parse parameters");
        return;
    }

    php_error_docref(NULL, E_NOTICE, "PHeme: Filepath: '%s'", filepath);

    /* CRITICAL FIX: Copy filepath to persistent memory */
    load_data.filepath = estrndup(filepath, filepath_len);
    load_data.error = 0;
    
    php_error_docref(NULL, E_NOTICE, "PHeme: About to call scm_with_guile for guile_load_file");
    
    /* CRITICAL FIX: Use scm_with_guile wrapper - THIS WAS MISSING! */
    scm_with_guile(guile_load_helper, &load_data);
    
    php_error_docref(NULL, E_NOTICE, "PHeme: scm_with_guile returned from guile_load_file");
    
    /* Free copied string */
    efree(load_data.filepath);
    
    /* Check for errors */
    if (load_data.error) {
        php_error_docref(NULL, E_WARNING, "Error loading Scheme file: %s", filepath);
        RETURN_FALSE;
    }

    RETURN_TRUE;
}
/* }}} */
