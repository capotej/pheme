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
#include <ctype.h>

/* {{{ Sentinel enum for error/validity tracking
 * Replaces (void*)-1 sentinel pattern to avoid type confusion
 */
typedef enum {
    PHEME_VALID = 0,      /* Valid pointer/state */
    PHEME_SENTINEL = 1    /* Freed/invalid sentinel marker */
} pheme_sentinel_t;

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
    if (obj == NULL) {
        return NULL;
    }
    return (guile_context_object_t*)((char*)obj - obj->handlers->offset);
}

/* Forward declarations for object handlers */
static zend_object *guile_context_create_object(zend_class_entry *ce);
static void guile_context_free_object(zend_object *object);

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

ZEND_BEGIN_ARG_INFO_EX(arginfo_guile_context_destruct, 0, 0, 0)
ZEND_END_ARG_INFO()
/* }}} */

/* {{{ pheme_functions[]
 */
const zend_function_entry pheme_functions[] = {
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
    PHP_ME(GuileContext, __destruct, arginfo_guile_context_destruct, ZEND_ACC_PUBLIC)
    PHP_FE_END
};
/* }}} */

/* {{{ Helper: Create a fresh Guile module
 */
static SCM create_fresh_module(void)
{
    SCM fresh_module;
    
    /* Create a fresh module with (guile) bindings imported
     * Using scm_c_eval_string for proper C integration */
    fresh_module = scm_c_eval_string(
        "(let ((m (make-module))) (module-use! m (resolve-module '(guile))) m)"
    );
    
    return fresh_module;
}

/* {{{ Helper: Create a new GuileContext
 * Called within scm_with_guile to ensure proper Guile mode
 */
static void* create_context_helper(void *data)
{
    guile_context_t *ctx;
    
    (void)data;  /* Unused */
    
    /* Use scm_calloc to zero-initialize the struct, ensuring ctx->module is
     * SCM_UNDEFINED (which equals 0) before Guile's GC can access it.
     * This prevents uninitialized memory access if GC runs between malloc
     * and module assignment. scm_calloc ensures proper integration with
     * Guile's garbage collection system. */
    ctx = (guile_context_t*)scm_calloc(sizeof(guile_context_t));
    if (ctx == NULL) {
        return NULL;
    }
    
    /* Create the module - must be done in Guile mode */
    ctx->module = create_fresh_module();
    
    return ctx;
}

/* {{{ Helper: Structure for passing data to eval helper
 */
typedef struct {
    guile_context_t *ctx;
    char *code;
    char *error_message;  /* Captured error message from Guile */
} eval_data_t;


/* {{{ Helper: Clean up Guile format placeholders from error message
 * Guile uses ~A, ~S, ~% etc. in format strings - we remove these for cleaner output
 */
static void cleanup_guile_format_placeholders(char *msg)
{
    if (msg == NULL) return;
    
    char *src = msg;
    char *dst = msg;
    
    while (*src) {
        if (*src == '~' && *(src + 1)) {
            /* Skip the ~ and the following character */
            src += 2;
        } else {
            *dst++ = *src++;
        }
    }
    *dst = '\0';
}

/* {{{ Helper: Convert SCM value to error message string
 * Extracts a human-readable error message from Guile's exception arguments
 */
static char* error_message_from_scm(SCM args)
{
    char *msg = NULL;
    
    /* Guile exceptions typically have args as: (key format-string arg1 arg2 ...)
     * where key is a symbol like 'wrong-type-arg', 'syntax-error', etc.
     * We want to extract the meaningful error message from this. */
    if (scm_is_pair(args)) {
        /* Skip the first element (key symbol) and get the message parts */
        SCM rest = SCM_CDR(args);
        
        if (scm_is_pair(rest)) {
            SCM first_msg_part = SCM_CAR(rest);
            
            if (scm_is_string(first_msg_part)) {
                /* This is likely the format string - use it directly
                 * The format string itself contains useful info like "Unbound variable: ~S" */
                msg = scm_to_locale_string(first_msg_part);
                /* Clean up Guile format placeholders like ~A, ~S, ~% */
                cleanup_guile_format_placeholders(msg);
                return msg;
            } else {
                /* Convert to string */
                SCM as_string = scm_object_to_string(first_msg_part, SCM_UNDEFINED);
                if (scm_is_string(as_string)) {
                    msg = scm_to_locale_string(as_string);
                    cleanup_guile_format_placeholders(msg);
                }
            }
        }
        
        /* Fallback: convert entire args to string */
        if (msg == NULL) {
            SCM as_string = scm_object_to_string(args, SCM_UNDEFINED);
            if (scm_is_string(as_string)) {
                msg = scm_to_locale_string(as_string);
                cleanup_guile_format_placeholders(msg);
            }
        }
    } else if (scm_is_string(args)) {
        /* args is directly a string */
        msg = scm_to_locale_string(args);
        cleanup_guile_format_placeholders(msg);
    } else {
        /* Convert to string representation */
        SCM as_string = scm_object_to_string(args, SCM_UNDEFINED);
        if (scm_is_string(as_string)) {
            msg = scm_to_locale_string(as_string);
            cleanup_guile_format_placeholders(msg);
        }
    }
    
    return msg;
}

/* {{{ Helper: Error handler for scm_c_catch
 * Called when Guile throws an exception during eval
 */
static SCM eval_error_handler(void *data, SCM key, SCM args)
{
    (void)key;  /* Unused - we catch all exceptions */
    
    eval_data_t *ed = (eval_data_t*)data;
    
    /* Extract error message and store in eval_data_t */
    if (ed->error_message == NULL) {
        ed->error_message = error_message_from_scm(args);
    }
    
    /* Return SCM_UNDEFINED to indicate error - we'll check error_message */
    return SCM_UNDEFINED;
}

/* Eval wrapper format for wrapping user code in a begin expression */
#define EVAL_WRAPPER "(begin )"

/* {{{ Helper: Body function for scm_c_catch
 * Evaluates the code within the proper module context
 */
static SCM eval_body(void *data)
{
    eval_data_t *ed = (eval_data_t*)data;
    guile_context_t *ctx = ed->ctx;
    SCM old_module, result;
    char *eval_cmd;
    size_t cmd_size;
    
    /* Save current module and switch to context's module */
    old_module = scm_current_module();
    scm_set_current_module(ctx->module);
    
    /* Dynamically allocate command buffer based on code length */
    /* Format is EVAL_WRAPPER "%s" - sizeof includes null terminator */
    cmd_size = strlen(ed->code) + sizeof(EVAL_WRAPPER);
    eval_cmd = (char*)malloc(cmd_size);
    if (eval_cmd == NULL) {
        /* Restore module before signaling error */
        scm_set_current_module(old_module);
        /* Use SCM_NULLP to throw a non-continuable error that aborts */
        scm_error(
            scm_from_locale_symbol("pheme-error"),
            "eval_body",
            "Failed to allocate memory for eval command",
            SCM_EOL, SCM_EOL
        );
        /* Unreachable - scm_error does not return */
        return SCM_UNDEFINED;
    }
    
    /* Evaluate the code - scm_c_eval_string uses current module for compilation */
    snprintf(eval_cmd, cmd_size, "(begin %s)", ed->code);
    result = scm_c_eval_string(eval_cmd);
    
    /* Free the dynamically allocated command buffer */
    free(eval_cmd);
    
    /* Restore original module */
    scm_set_current_module(old_module);
    
    return result;
}

/* {{{ Helper: Pre-unwind handler for scm_c_catch
 * Called during exception unwinding before the error handler
 */
static SCM eval_pre_unwind(void *data, SCM key, SCM args)
{
    (void)data;
    (void)key;
    (void)args;
    /* Return the throw arguments to be handled by the main handler */
    return SCM_UNDEFINED;
}

/* {{{ Helper: Evaluate code in a specific context with error capture
 * Called within scm_with_guile to ensure proper Guile mode
 * Uses scm_c_catch to capture actual error messages from Guile
 */
static void* eval_in_context_helper(void *data)
{
    eval_data_t *ed = (eval_data_t*)data;
    guile_context_t *ctx = ed->ctx;
    SCM old_module, result, result_as_string;
    char *result_str;
    
    /* Initialize error message pointer to NULL */
    ed->error_message = NULL;
    
    /* Save current module and switch to context's module */
    old_module = scm_current_module();
    scm_set_current_module(ctx->module);
    
    /* Use scm_c_catch to catch exceptions and capture error messages
     * The pre-unwind handler captures the error args for message extraction */
    result = scm_c_catch(
        SCM_BOOL_T,           /* Catch all exceptions (throw/catch) */
        eval_body,            /* Function to evaluate the code */
        ed,                   /* Data passed to eval_body */
        eval_error_handler,   /* Handler for exceptions */
        ed,                   /* Data passed to error handler */
        eval_pre_unwind,      /* Pre-unwind handler to capture args */
        ed                    /* Data passed to pre-unwind handler */
    );
    
    /* Restore original module */
    scm_set_current_module(old_module);
    
    /* Check if an error occurred (error_message was set) - error_message indicates the error */
    if (ed->error_message != NULL) {
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
    /* No global Guile cleanup needed.
     *
     * Guile (when initialized with scm_init_guile) does not require explicit
     * global cleanup because:
     * 1. There is no scm_shutdown_guile() function in the public API
     * 2. All Guile resources (modules, ports, etc.) are GC-managed
     * 3. Per-context cleanup is handled by GuileContext::__destruct() and free()
     * 4. The OS will reclaim all memory when the PHP process exits
     *
     * Each GuileContext object cleans up its own module via free_guile_context_helper()
     * when the object is destroyed or free() is called.
     */
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
    
    if (ctx == NULL || ctx == (guile_context_t*)PHEME_SENTINEL) {
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
    
    /* Check for valid pointer (not NULL) */
    if (obj->ctx != NULL) {
        /* Clean up Guile module before freeing the struct */
        scm_with_guile(free_guile_context_helper, obj->ctx);
    }
    /* Mark as freed to prevent double-free and detect reuse after free */
    obj->ctx = (guile_context_t*)PHEME_SENTINEL;
    
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

/* {{{ proto void GuileContext::__construct()
   Constructor - creates context if not already done */
ZEND_METHOD(GuileContext, __construct)
{
    guile_context_object_t *obj = guile_context_from_obj(Z_OBJ_P(getThis()));
    
    (void)ZEND_NUM_ARGS();
    
    /* Check for freed sentinel - prevent reuse after free */
    if (obj->ctx == (guile_context_t*)PHEME_SENTINEL) {
        zend_throw_exception(NULL, "Cannot reuse freed GuileContext object", 0);
        return;
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

/* {{{ Helper: Trim leading and trailing whitespace from a string
 * Returns a pointer to the trimmed string (within original buffer)
 * and sets *trimmed_len to the length of the trimmed string
 */
static const char *trim_string(const char *str, size_t len, size_t *trimmed_len)
{
    const char *end;
    size_t trimmed;
    
    /* Skip leading whitespace */
    while (len > 0 && isspace((unsigned char)str[len - 1])) {
        len--;
    }
    
    /* Find first non-whitespace character */
    end = str;
    while (*end && isspace((unsigned char)*end)) {
        end++;
    }
    
    /* Calculate trimmed length */
    trimmed = len - (end - str);
    
    /* Validate that trimmed_len is reasonable - must not exceed original length
     * and must not be larger than a reasonable maximum for input validation */
    if (trimmed > len || trimmed > 1048576) { /* 1MB sanity check */
        *trimmed_len = 0;
        return str;
    }
    
    *trimmed_len = trimmed;
    return end;
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
    const char *trimmed_code;
    size_t trimmed_len;
    
    if (zend_parse_parameters(ZEND_NUM_ARGS(), "s", &code, &code_len) == FAILURE) {
        zend_throw_exception(NULL, "Failed to parse parameters for eval", 0);
        return;
    }
    
    /* Trim whitespace and validate for empty code */
    trimmed_code = trim_string(code, code_len, &trimmed_len);
    if (trimmed_len == 0) {
        RETURN_EMPTY_STRING();
    }
    
    /* Check for NULL (never allocated) or sentinel (freed) */
    if (obj->ctx == NULL || obj->ctx == (guile_context_t*)PHEME_SENTINEL) {
        zend_throw_exception(NULL, "Context not found for this object", 0);
        return;
    }
    
    /* Copy PHP string to persistent memory for Guile */
    code_copy = estrndup(code, code_len);
    
    /* Evaluate in context - initialize error_message to NULL */
    eval_data_t data = { obj->ctx, code_copy, NULL };
    
    result_str = (char*)scm_with_guile(eval_in_context_helper, &data);
    efree(code_copy);
    
    /* Check for error - use error_message flag instead of sentinel pointer comparison */
    if (data.error_message != NULL) {
        /* Use captured error message from Guile, or fallback to generic message */
        const char *error_msg = data.error_message;
        /* Create error message with context about which code failed */
        zend_throw_exception_ex(NULL, 0, "%s (code: %s)", error_msg, code);
        /* Free the captured error message (malloc'd by scm_to_locale_string) */
        free((void*)data.error_message);
        return;
    }
    
    if (result_str == NULL) {
        /* Include the code in the error message for context */
        zend_throw_exception_ex(NULL, 0, "Error evaluating Scheme code: %s", code);
        return;
    }
    
    /* Return the result as a PHP string and free the malloc'd result_str */
    RETVAL_STRING(result_str);
    free(result_str);
}

/* {{{ proto void GuileContext::free()
    Free the Guile context and release associated resources */
ZEND_METHOD(GuileContext, free)
{
    guile_context_object_t *obj = guile_context_from_obj(Z_OBJ_P(getThis()));
    
    (void)ZEND_NUM_ARGS();
    
    /* Check for sentinel (freed) or NULL (never allocated) */
    if (obj->ctx == NULL || obj->ctx == (guile_context_t*)PHEME_SENTINEL) {
        /* Already freed or never allocated, nothing to do */
        RETURN_TRUE;
    }
    
    /* Clean up Guile module before freeing the struct */
    scm_with_guile(free_guile_context_helper, obj->ctx);
    
    /* Set sentinel to detect reuse after free */
    obj->ctx = (guile_context_t*)PHEME_SENTINEL;
    
    RETURN_TRUE;
}

/* {{{ proto void GuileContext::__destruct()
   Destructor - automatically frees the Guile context */
ZEND_METHOD(GuileContext, __destruct)
{
    guile_context_object_t *obj = guile_context_from_obj(Z_OBJ_P(getThis()));
    
    (void)ZEND_NUM_ARGS();
    
    /* Check for valid pointer (not NULL, not sentinel) */
    if (obj->ctx != NULL && obj->ctx != (guile_context_t*)PHEME_SENTINEL) {
        /* Clean up Guile module before freeing the struct */
        scm_with_guile(free_guile_context_helper, obj->ctx);
        obj->ctx = (guile_context_t*)PHEME_SENTINEL;
    }
}
