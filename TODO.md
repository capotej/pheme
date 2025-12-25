# Pheme Code Review TODO


### 🟡 Whitespace-Only Code Validation
**File:** `pheme.c:362-364`
**Issue:** Whitespace-only code passes through empty check and may cause unexpected Scheme errors
**Fix:** Trim whitespace before evaluation

## Thread Safety (Medium Priority)

### 🟡 Guile Initialization in PHP_MINIT
**File:** `pheme.c:180-183`
**Issue:** `scm_init_guile()` is called only once for the main thread
**Impact:** Worker threads need their own initialization
**Fix:** Document thread limitations or implement per-thread initialization

## Security (Medium Priority)

### 🟠 No Execution Timeout
**Issue:** Scheme code execution could hang indefinitely
**Fix:** Consider adding timeout mechanism for `eval()` calls

### 🟠 No Sandbox/Resource Limits
**Issue:** No sandboxing - Scheme code has full access to Guile's capabilities
**Fix:** Document security implications; consider sandboxing for production use

### 🟠 Unrestricted Resource Usage
**Issue:** No memory or computation quotas enforced
**Fix:** Consider implementing resource limits

## Code Quality (Low Priority)

### 🟢 Missing clone_obj Implementation
**File:** `pheme.c:189`
**Issue:** `guile_context_handlers.clone_obj = NULL` - objects cannot be cloned
**Fix:** Add explicit comment explaining why cloning is disabled, or implement deep cloning

### 🟢 Per-Evaluation Memory Allocation
**File:** `pheme.c:147-159`
**Issue:** `eval_cmd` is allocated/freed for every `eval()` call
**Fix:** Consider using thread-local buffer or stack allocation for small code strings

### 🟢 Documentation Gaps
- [ ] Create `README.md` with installation and usage examples
- [ ] Add API documentation for `GuileContext` methods
- [ ] Document thread-safety limitations
- [ ] Document security considerations

## Testing (Low Priority)

### 🟢 Missing Test Cases
- [ ] Error handling for invalid Scheme syntax
- [ ] Concurrent access from multiple threads
- [ ] Memory pressure scenarios
- [ ] Large result values

## Test Coverage Status

✅ **Existing Tests:**
- `tests/guile_context_basic.phpt` - Basic functionality
- `tests/guile_context_isolation.phpt` - Context isolation
- `tests/guile_context_empty_code.phpt` - Empty code handling
- `tests/guile_context_free.phpt` - Free/destruct behavior

❌ **Missing Tests:**
- Invalid Scheme syntax error handling
- Thread safety verification
- Resource limit behavior
- Large result handling

## Priority Summary

| Priority | Items |
|----------|-------|
| High (Critical) | NULL checks, memory leak fixes |
| Medium | Error consistency, thread safety, security |
| Low | Performance, documentation, testing |

## References

- Guile Integration: [GUILE.md](GUILE.md)
- Main Source: [pheme.c](pheme.c)
- Agent Rules: [AGENTS.md](AGENTS.md)
