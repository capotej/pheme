#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"

/* {{{ PHP_FUNCTION declarations
 */
PHP_FUNCTION(hello_world);
PHP_FUNCTION(add_numbers);
/* }}} */

/* {{{ arginfo */
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_hello_world, 0, 0, IS_TRUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_add_numbers, 0, 2, IS_LONG, 0)
    ZEND_ARG_TYPE_INFO(0, num1, IS_LONG, 0)
    ZEND_ARG_TYPE_INFO(0, num2, IS_LONG, 0)
ZEND_END_ARG_INFO()
/* }}} */

/* {{{ pheme_functions[]
 */
const zend_function_entry pheme_functions[] = {
    PHP_FE(hello_world, arginfo_hello_world)
    PHP_FE(add_numbers, arginfo_add_numbers)
    PHP_FE_END
};
/* }}} */

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(pheme)
{
    return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(pheme)
{
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