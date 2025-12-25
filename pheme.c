#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "zend_exceptions.h"
#include <libguile.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* {{{ GuileContext structure
 */

typedef struct _guile_context {
    SCM module;                    /* Guile module for this context */
} guile_context_t;

/* Custom object handlers for thread-safe context storage */
static zend_object_handlers guile_context_handlers;

/* Structure extending zend_object to store Guile context */
typedef struct _guile_context_object {
    guile_context_t *ctx;          /* Guile context for this object */
    zend_object  std;              /* Standard PHP object */
} guile_context_object_t;

/* Helper to get guile_context_object_t from zend_object */
static inline guile_context_object_t *guile_context_from_obj(zend_object *obj) {
    return (guile_context_object_t*)((char*)obj - obj->handlers->offset);
}

/* Forward declarations for object handlers */
static zend_object *guile_context_create_object(zend_class_entry *ce);
static void guile_context_free_object(zend_object *object);

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

ZEND_BEGIN_ARG_INFO_EX(arginfo_guile_context_free, 0, 0, 0)
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
ZEND_METHOD(GuileContext, free);
ZEND_METHOD(GuileContext, __destruct);

static const zend_function_entry guile_context_methods[] = {
    PHP_ME(GuileContext, __construct, arginfo_guile_context_construct, ZEND_ACC_PUBLIC)
    PHP_ME(GuileContext, eval, arginfo_guile_context_eval, ZEND_ACC_PUBLIC)
    PHP_ME(GuileContext, free, arginfo_guile_context_free, ZEND_ACC_PUBLIC)
    PHP_ME(GuileContext, __destruct, arginfo_guile_context_free, ZEND_ACC_PUBLIC)
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
    char *eval_cmd;
    size_t cmd_size;
    
    /* Save current module and switch to context's module */
    old_module = scm_current_module();
    scm_set_current_module(ctx->module);
    
    /* Dynamically allocate command buffer based on code length */
    /* Format is "(begin %s)" which adds 8 chars (7 for wrapper + 1 for null terminator) */
    cmd_size = strlen(ed->code) + 8 + 1;
    eval_cmd = (char*)malloc(cmd_size);
    if (eval_cmd == NULL) {
        scm_set_current_module(old_module);
        return NULL;
    }
    
    /* Evaluate the code - scm_c_eval_string uses current module for compilation */
    snprintf(eval_cmd, cmd_size, "(begin %s)", ed->code);
    result = scm_c_eval_string(eval_cmd);
    
    /* Free the dynamically allocated command buffer */
    free(eval_cmd);
    
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
    
    /* Initialize custom handlers */
    memcpy(&guile_context_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
    guile_context_handlers.offset = XtOffsetOf(guile_context_object_t, std);
    guile_context_handlers.free_obj = guile_context_free_object;
    guile_context_handlers.clone_obj = NULL;
    
    /* Register GuileContext class */
    zend_class_entry ce;
    INIT_CLASS_ENTRY(ce, "GuileContext", guile_context_methods);
    guile_context_ce = zend_register_internal_class_ex(&ce, NULL);
    guile_context_ce->create_object = guile_context_create_object;
    
    return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(pheme)
{
    /* No global cleanup needed - each object manages its own context */
    return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(pheme)
{
    php_info_print_table_start();
    php_info_print_table_header(2, "pheme support", "enabled");
    php_info_print_table_row(2, "pheme version", "0.2.0");
    php_info_print_table_end();
}
/* }}} */

/* {{{ Object creation handler */
static zend_object *guile_context_create_object(zend_class_entry *ce)
{
    guile_context_object_t *obj = zend_object_alloc(sizeof(guile_context_object_t), ce);
    
    zend_object_std_init(&obj->std, ce);
    obj->ctx = NULL;
    
    obj->std.handlers = &guile_context_handlers;
    
    return &obj->std;
}
/* }}} */

/* {{{ Helper: Free Guile context and cleanup module
 * Must be called within scm_with_guile to ensure proper Guile mode
 */
static void *free_guile_context_helper(void *data)
{
    guile_context_t *ctx = (guile_context_t*)data;
    SCM old_module;
    
    if (ctx == NULL || ctx == (void*)-1) {
        return NULL;
    }
    
    /* Clear all bindings from the module to release resources
     * This allows Guile's garbage collector to reclaim the module */
    old_module = scm_current_module();
    scm_set_current_module(ctx->module);
    scm_c_eval_string("(module-clear! (current-module))");
    scm_set_current_module(old_module);
    
    /* Now free the struct */
    free(ctx);
    
    return NULL;
}

/* {{{ Object free handler */
static void guile_context_free_object(zend_object *object)
{
    guile_context_object_t *obj = guile_context_from_obj(object);
    
    /* Check for valid pointer (not NULL, not sentinel) */
    if (obj->ctx != NULL && obj->ctx != (void*)-1) {
        /* Clean up Guile module before freeing the struct */
        scm_with_guile(free_guile_context_helper, obj->ctx);
    }
    /* Mark as freed to prevent double-free and detect reuse after free */
    obj->ctx = (void*)-1;
    
    zend_object_std_dtor(&obj->std);
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
    PHP_MINFO(pheme),  /* PHP_MINFO */
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
    guile_context_object_t *obj;
    
    (void)ZEND_NUM_ARGS();
    
    /* Create PHP object - constructor will create context */
    object_init_ex(return_value, guile_context_ce);
    obj = guile_context_from_obj(Z_OBJ_P(return_value));
    
    /* Create context in Guile mode */
    obj->ctx = (guile_context_t*)scm_with_guile(create_context_helper, NULL);
    
    if (obj->ctx == NULL) {
        php_error_docref(NULL, E_WARNING, "Failed to create Guile context");
        RETURN_FALSE;
    }
}

/* {{{ proto void GuileContext::__construct()
   Constructor - creates context if not already done */
ZEND_METHOD(GuileContext, __construct)
{
    guile_context_object_t *obj = guile_context_from_obj(Z_OBJ_P(getThis()));
    
    (void)ZEND_NUM_ARGS();
    
    /* Check for freed sentinel - prevent reuse after free */
    if (obj->ctx == (void*)-1) {
        php_error_docref(NULL, E_WARNING, "Cannot reuse freed GuileContext object");
        RETURN_FALSE;
    }
    
    /* Check if context already exists for this object */
    if (obj->ctx != NULL) {
        return;
    }
    
    /* Create new context */
    obj->ctx = (guile_context_t*)scm_with_guile(create_context_helper, NULL);
    
    if (obj->ctx == NULL) {
        zend_throw_exception(NULL, "Failed to create Guile context", 0);
        return;
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
    guile_context_object_t *obj = guile_context_from_obj(Z_OBJ_P(getThis()));
    
    if (zend_parse_parameters(ZEND_NUM_ARGS(), "s", &code, &code_len) == FAILURE) {
        php_error_docref(NULL, E_WARNING, "Failed to parse parameters");
        RETURN_FALSE;
    }
    
    /* Check for NULL (never allocated) or sentinel (freed) */
    if (obj->ctx == NULL || obj->ctx == (void*)-1) {
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
    
    eval_data_t data = { obj->ctx, code_copy };
    
    result_str = (char*)scm_with_guile(eval_in_context_helper, &data);
    efree(code_copy);
    
    if (result_str == NULL) {
        php_error_docref(NULL, E_WARNING, "Error evaluating Scheme code");
        RETURN_FALSE;
    }
    
    /* Return the result as a PHP string */
    RETVAL_STRING(result_str);
}

/* {{{ proto void GuileContext::free()
    Free the Guile context and release associated resources */
ZEND_METHOD(GuileContext, free)
{
    guile_context_object_t *obj = guile_context_from_obj(Z_OBJ_P(getThis()));
    
    (void)ZEND_NUM_ARGS();
    
    /* Check for sentinel (freed) or NULL (never allocated) */
    if (obj->ctx == NULL || obj->ctx == (void*)-1) {
        /* Already freed or never allocated, nothing to do */
        RETURN_TRUE;
    }
    
    /* Clean up Guile module before freeing the struct */
    scm_with_guile(free_guile_context_helper, obj->ctx);
    
    /* Set sentinel to detect reuse after free */
    obj->ctx = (void*)-1;
    
    RETURN_TRUE;
}

/* {{{ proto void GuileContext::__destruct()
   Destructor - automatically frees the Guile context
   Note: This is handled by guile_context_free_object, but kept for API compatibility */
ZEND_METHOD(GuileContext, __destruct)
{
    /* Context is freed by the free_obj handler, nothing to do here */
    (void)ZEND_NUM_ARGS();
}
