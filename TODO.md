# TODO: pheme.c Code Review Findings

## Critical Issues

## High-Severity Issues

### 4. Thread-Unsafe Global Context Array
**File:** `pheme.c:22`
**Severity:** High

```c
static guile_context_t *context_array[MAX_CONTEXTS] = {0};
```

**Problem:** No mutex protection for concurrent access in PHP-FPM with multiple workers. Can cause:
- Context collision between requests
- Memory corruption during array updates
- Race conditions in `MSHUTDOWN`

**Fix:** 
- Use PHP's object handlers instead of a separate array

---

### 5. Handle Reuse After Free
**File:** `pheme.c:313-341, 345-360`
**Severity:** High

**Problem:** After calling `free()` on a context, the PHP object still exists with the same handle. If:
- `__construct()` is called again
- A new object gets the same handle (unlikely but possible)

...the array entry will be overwritten, leading to use-after-free or double-free crashes.

**Fix:** Set a sentinel value (e.g., `(void*)-1`) in the array when freeing, and check for it before reuse.

---

### 6. Missing Module Cleanup in Destructor
**File:** `pheme.c:345-360`
**Severity:** High

```c
ZEND_METHOD(GuileContext, __destruct)
{
    // ...
    free(ctx);  // Only frees the struct, not the Guile module!
}
```

**Problem:** `ctx->module` is an `SCM` value representing a module in Guile's runtime. Simply freeing the struct without proper module cleanup causes:
- Memory leaks in Guile's runtime
- Resource leaks (file descriptors, etc.)

**Fix:** Use Guile's module cleanup API before freeing the struct.

---

## Medium-Severity Issues

### 7. Incorrect Constructor Return Value
**File:** `pheme.c:248-250`
**Severity:** Medium

```c
if (ctx == NULL) {
    php_error_docref(NULL, E_WARNING, "Failed to create Guile context");
    RETURN_FALSE;  // Constructors cannot return values!
}
```

**Fix:** Throw an exception instead:
```c
zend_throw_exception(NULL, "Failed to create Guile context", 0);
return;
```

---

### 8. Missing `PHP_MINFO` Function
**File:** `pheme.c:193`
**Severity:** Medium

```c
NULL,  /* PHP_MINFO */
```

**Problem:** No module information is displayed in `phpinfo()`.

**Fix:** Implement `PHP_MINFO(pheme)`:
```c
PHP_MINFO_FUNCTION(pheme)
{
    php_info_print_table_start();
    php_info_print_table_header(2, "pheme support", "enabled");
    php_info_print_table_row(2, "pheme version", "0.2.0");
    php_info_print_table_end();
}
```

---

### 9. Missing Input Validation in `eval()`
**File:** `pheme.c:260-265`
**Severity:** Medium

**Problem:** Empty strings or NULL code are accepted without validation, causing confusing Guile errors.

**Fix:** Add early validation:
```c
if (code_len == 0) {
    RETURN_EMPTY_STRING();
}
```

---

## Low-Severity Issues

### 10. Inconsistent ArgInfo for `__destruct`
**File:** `pheme.c:66`
**Severity:** Low

```c
PHP_ME(GuileContext, __destruct, arginfo_guile_context_free, ZEND_ACC_PUBLIC)
```

Uses `arginfo_guile_context_free` instead of a dedicated destruct arginfo. While harmless, it's confusing.

**Fix:** Create `arginfo_guile_context_destruct` with no arguments.

---

### 11. Magic Number `1024`
**File:** `pheme.c:21`
**Severity:** Low

```c
#define MAX_CONTEXTS 1024
```

No explanation of why 1024 was chosen. Consider making this configurable via `php.ini`.

---

### 12. Missing Error Context in Warnings
**File:** Multiple locations (e.g., `pheme.c:216, 248`)
**Severity:** Low

```c
php_error_docref(NULL, E_WARNING, "Failed to create Guile context");
```

Using `NULL` for docref makes it harder to debug which function caused the warning.

**Fix:** Use `__FILE__` or a descriptive docref string.

---

## Priority Order

1. **Immediate:** Fix critical memory issues (Double-Free #1, Buffer Overflow #2)
2. **Short-term:** Fix memory leak (#3) and thread-safety (#4)
3. **Medium-term:** Fix use-after-free (#5), module cleanup (#6), constructor (#7)
4. **Long-term:** Add MINFO (#8), input validation (#9), address minor issues (#10-12)

---

## Testing Recommendations

After fixing each issue:
1. Test with long input strings (>1024 chars) for buffer overflow
2. Test concurrent access with multiple PHP-FPM workers
3. Test object reuse after calling `free()`
4. Test empty string input to `eval()`
5. Verify no memory leaks with Valgrind or equivalent
