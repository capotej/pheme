#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include <libguile.h>
#include <stdio.h>

/* {{{ PHP_FUNCTION declarations
 */
PHP_FUNCTION(guile_eval);
/* }}} */

/* {{{ arginfo */
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_guile_eval, 0, 1, IS_STRING, 0)
    ZEND_ARG_TYPE_INFO(0, code, IS_STRING, 0)
ZEND_END_ARG_INFO()
/* }}} */

/* {{{ pheme_functions[]
 */
const zend_function_entry pheme_functions[] = {
    PHP_FE(guile_eval, arginfo_guile_eval)
    PHP_FE_END
};
/* }}} */

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(pheme)
{
    /* Initialize Guile explicitly before any scm_with_guile calls
     * According to Guile docs: "scm_init_guile() arranges things so that all
     * of the code in the current thread executes as if from within a call to
     * scm_with_guile." */
    
    /* Initialize Guile for the current thread */
    scm_init_guile();
    
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

/* Helper function to execute Guile eval in proper context with isolated environment */
static void* guile_eval_helper(void* data) {
    char* code = (char*)data;
    SCM result, result_as_string;
    char* result_str;
    SCM old_module;
    static int counter = 0;
    char create_cmd[256], eval_cmd[512];
    
    /* CRITICAL: Do NOT call PHP functions inside Guile context
     * Only Guile API calls should be made here */
    
    /* ISOLATION FIX: Create a fresh module for each evaluation
     * This ensures that variable definitions do not persist between calls */
    
    /* Save the current module */
    old_module = scm_current_module();
    
    /* Create a fresh module with standard bindings using Scheme code
     * Use module-use! to import (guile) bindings into the new module */
    snprintf(create_cmd, sizeof(create_cmd),
             "(let ((m (make-module))) (module-use! m (resolve-module '(guile))) m)");
    
    /* Create the fresh module and set it as current */
    SCM fresh_module = scm_eval_string(scm_from_locale_string(create_cmd));
    scm_set_current_module(fresh_module);
    
    /* Evaluate the user's code in the fresh module */
    snprintf(eval_cmd, sizeof(eval_cmd), "(begin %s)", code);
    result = scm_c_eval_string(eval_cmd);
    
    /* Restore the original module */
    scm_set_current_module(old_module);
    
    /* Check for errors - NOTE: SCM_FALSEP only checks for #f, not errors! */
    if (SCM_FALSEP(result)) {
        return NULL;
    }
    
    /* CRITICAL FIX: Convert any type to string using scm_object_to_string
     * This handles numbers, symbols, lists, etc. - not just strings */
    result_as_string = scm_object_to_string(result, SCM_UNDEFINED);
    
    /* Now convert the Scheme string to C string */
    result_str = scm_to_locale_string(result_as_string);
    
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

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "s", &code, &code_len) == FAILURE) {
        php_error_docref(NULL, E_WARNING, "PHeme: Failed to parse parameters");
        return;
    }

    /* CRITICAL FIX: Copy PHP string to persistent memory before passing to scm_with_guile
     * The PHP string pointer may become invalid when scm_with_guile switches contexts.
     * We must use estrndup to allocate memory that persists across context switches. */
    code_copy = estrndup(code, code_len);
    
    /* CRITICAL FIX: Use scm_with_guile for thread-safe execution with persistent copy */
    result_str = (char*)scm_with_guile(guile_eval_helper, code_copy);
    
    /* CRITICAL FIX: Free the copied string after Guile call completes */
    efree(code_copy);
    
    if (result_str == NULL) {
        php_error_docref(NULL, E_WARNING, "PHeme: Error evaluating Scheme code: %s", code);
        RETURN_FALSE;
    }
    
    /* Return the result as a PHP string and free the Guile-allocated memory */
    RETVAL_STRING(result_str);
    free(result_str);  /* Essential: free memory allocated by Guile */
}
/* }}} */

