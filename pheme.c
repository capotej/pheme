#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include <libguile.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* {{{ GuileContext structure
 */

typedef struct _guile_context {
    SCM module;                    /* Guile module for this context */
} guile_context_t;

/* Maximum number of contexts we can track (PHP object handles are sequential) */
#define MAX_CONTEXTS 1024
static guile_context_t *context_array[MAX_CONTEXTS] = {0};

/* }}} */

/* {{{ PHP_FUNCTION declarations
 */
PHP_FUNCTION(guile_context);
/* }}} */

/* {{{ GuileContext class arginfo
 */
ZEND_BEGIN_ARG_INFO_EX(arginfo_guile_context_construct, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_guile_context_eval, 0, 1, IS_STRING, 0)
    ZEND_ARG_TYPE_INFO(0, code, IS_STRING, 0)
ZEND_END_ARG_INFO()
/* }}} */

/* {{{ pheme_functions[]
 */
const zend_function_entry pheme_functions[] = {
    PHP_FE(guile_context, arginfo_guile_context_construct)
    PHP_FE_END
};
/* }}} */

/* {{{ PHP class entry for GuileContext
 */
static zend_class_entry *guile_context_ce;

/* Declare method implementations for GuileContext class */
ZEND_METHOD(GuileContext, __construct);
ZEND_METHOD(GuileContext, eval);

static const zend_function_entry guile_context_methods[] = {
    PHP_ME(GuileContext, __construct, arginfo_guile_context_construct, ZEND_ACC_PUBLIC)
    PHP_ME(GuileContext, eval, arginfo_guile_context_eval, ZEND_ACC_PUBLIC)
    PHP_FE_END
};
/* }}} */

/* {{{ Helper: Create a fresh Guile module
 */
static SCM create_fresh_module(void)
{
    SCM fresh_module;
    
    /* Create a fresh module with (guile) bindings imported
     * Using the working Scheme approach */
    fresh_module = scm_eval_string(scm_from_locale_string(
        "(let ((m (make-module))) (module-use! m (resolve-module '(guile))) m)"
    ));
    
    return fresh_module;
}

/* {{{ Helper: Create a new GuileContext
 * Called within scm_with_guile to ensure proper Guile mode
 */
static void* create_context_helper(void *data)
{
    guile_context_t *ctx;
    
    (void)data;  /* Unused */
    
    ctx = (guile_context_t*)malloc(sizeof(guile_context_t));
    if (ctx == NULL) {
        return NULL;
    }
    
    /* Create the module - must be done in Guile mode */
    ctx->module = create_fresh_module();
    
    return ctx;
}

/* {{{ Helper: Evaluate code in a specific context
 * Called within scm_with_guile to ensure proper Guile mode
 */
static void* eval_in_context_helper(void *data)
{
    typedef struct {
        guile_context_t *ctx;
        char *code;
    } eval_data_t;
    
    eval_data_t *ed = (eval_data_t*)data;
    guile_context_t *ctx = ed->ctx;
    SCM old_module, result, result_as_string;
    char *result_str;
    char eval_cmd[1024];
    
    /* Save current module and switch to context's module */
    old_module = scm_current_module();
    scm_set_current_module(ctx->module);
    
    /* Evaluate the code - scm_c_eval_string uses current module for compilation */
    snprintf(eval_cmd, sizeof(eval_cmd), "(begin %s)", ed->code);
    result = scm_c_eval_string(eval_cmd);
    
    /* Restore original module */
    scm_set_current_module(old_module);
    
    /* Check for errors */
    if (SCM_FALSEP(result)) {
        return NULL;
    }
    
    /* Convert result to string using scm_object_to_string */
    result_as_string = scm_object_to_string(result, SCM_UNDEFINED);
    
    /* Convert the Scheme string to C string */
    result_str = scm_to_locale_string(result_as_string);
    
    return result_str;
}

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(pheme)
{
    /* Initialize Guile for the current thread */
    scm_init_guile();
    
    /* Initialize context array */
    memset(context_array, 0, sizeof(context_array));
    
    /* Register GuileContext class */
    zend_class_entry ce;
    INIT_CLASS_ENTRY(ce, "GuileContext", guile_context_methods);
    guile_context_ce = zend_register_internal_class(&ce);
    
    return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(pheme)
{
    /* Free all contexts that were allocated */
    int i;
    for (i = 0; i < MAX_CONTEXTS; i++) {
        if (context_array[i] != NULL) {
            free(context_array[i]);
            context_array[i] = NULL;
        }
    }
    
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
    "0.2.0",
    STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_PHEME
ZEND_GET_MODULE(pheme)
#endif

/* {{{ proto GuileContext guile_context()
   Create a new Guile context with persistent state across evaluations */
PHP_FUNCTION(guile_context)
{
    guile_context_t *ctx;
    zend_object *object;
    
    (void)ZEND_NUM_ARGS();  /* Unused */
    
    /* Create context in Guile mode */
    ctx = (guile_context_t*)scm_with_guile(create_context_helper, NULL);
    
    if (ctx == NULL) {
        php_error_docref(NULL, E_WARNING, "Failed to create Guile context");
        RETURN_FALSE;
    }
    
    /* Create PHP object wrapping the context */
    object = zend_objects_new(guile_context_ce);
    ZVAL_OBJ(return_value, object);
    
    /* Store context in our array using object handle as index */
    if (object->handle < MAX_CONTEXTS) {
        context_array[object->handle] = ctx;
    }
}

/* {{{ proto void GuileContext::__construct()
   Constructor - creates context if not already done */
ZEND_METHOD(GuileContext, __construct)
{
    guile_context_t *ctx;
    zend_object *object = Z_OBJ_P(getThis());
    
    (void)ZEND_NUM_ARGS();
    
    /* Check if context already exists for this object */
    if (object->handle < MAX_CONTEXTS && context_array[object->handle] != NULL) {
        return;
    }
    
    /* Create new context */
    ctx = (guile_context_t*)scm_with_guile(create_context_helper, NULL);
    
    if (ctx == NULL) {
        php_error_docref(NULL, E_WARNING, "Failed to create Guile context");
        RETURN_FALSE;
    }
    
    /* Store context in our array */
    if (object->handle < MAX_CONTEXTS) {
        context_array[object->handle] = ctx;
    }
}

/* {{{ proto string GuileContext::eval(string code)
   Evaluate Scheme code in this context */
ZEND_METHOD(GuileContext, eval)
{
    char *code;
    size_t code_len;
    char *result_str;
    char *code_copy;
    guile_context_t *ctx;
    zend_object *object = Z_OBJ_P(getThis());
    
    if (zend_parse_parameters(ZEND_NUM_ARGS(), "s", &code, &code_len) == FAILURE) {
        php_error_docref(NULL, E_WARNING, "Failed to parse parameters");
        RETURN_FALSE;
    }
    
    /* Get context from array */
    if (object->handle >= MAX_CONTEXTS) {
        php_error_docref(NULL, E_WARNING, "Context handle out of range");
        RETURN_FALSE;
    }
    
    ctx = context_array[object->handle];
    
    if (ctx == NULL) {
        php_error_docref(NULL, E_WARNING, "Context not found for this object");
        RETURN_FALSE;
    }
    
    /* Copy PHP string to persistent memory for Guile */
    code_copy = estrndup(code, code_len);
    
    /* Evaluate in context */
    typedef struct {
        guile_context_t *ctx;
        char *code;
    } eval_data_t;
    
    eval_data_t data = { ctx, code_copy };
    
    result_str = (char*)scm_with_guile(eval_in_context_helper, &data);
    efree(code_copy);
    
    if (result_str == NULL) {
        php_error_docref(NULL, E_WARNING, "Error evaluating Scheme code");
        RETURN_FALSE;
    }
    
    /* Return the result as a PHP string and free Guile-allocated memory */
    RETVAL_STRING(result_str);
    free(result_str);
}
