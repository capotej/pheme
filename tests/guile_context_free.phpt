--TEST--
Test GuileContext::free() method and __destruct() automatic cleanup
--FILE--
<?php
echo "=== Testing GuileContext::free() and __destruct() ===\n\n";

// Test 1: Basic free() method
echo "--- Test 1: Basic free() method ---\n";
$ctx = new GuileContext();
echo "Created GuileContext instance\n";

$result = $ctx->eval('(+ 1 2)');
echo "1 + 2 = $result\n";

$freed = $ctx->free();
echo "Called free(): " . ($freed ? "success" : "failed") . "\n";

// Test 2: free() on already-freed context should be safe
echo "\n--- Test 2: Double free() should be safe ---\n";
$freed_again = $ctx->free();
echo "Called free() again: " . ($freed_again ? "success (no-op)" : "failed") . "\n";

// Test 3: __destruct() is called automatically when object goes out of scope
echo "\n--- Test 3: Automatic __destruct() cleanup ---\n";
function create_and_discard() {
    $ctx = new GuileContext();
    $ctx->eval('(define temp-var 999)');
    echo "Created context inside function, defined temp-var\n";
    // When function returns, $ctx goes out of scope and __destruct() is called
    return true;
}

$result = create_and_discard();
echo "Function returned, object should be automatically cleaned up\n";

// Test 4: Verify isolation is maintained after free
echo "\n--- Test 4: Using context after free should fail ---\n";
$ctx2 = new GuileContext();
$ctx2->eval('(define test-val "alive")');
echo "Set test-val in fresh context\n";

$ctx2->free();
echo "Freed the context\n";

// Attempting to use freed context - should throw exception
$errorOccurred = false;
try {
    $result = $ctx2->eval('test-val');
} catch (Exception $e) {
    $errorOccurred = true;
    echo "Exception caught (expected): " . $e->getMessage() . "\n";
}

if ($errorOccurred) {
    echo "PASS: Using freed context correctly throws exception\n";
} else {
    echo "FAIL: Using freed context should throw exception but got: $result\n";
}

// Test 5: Verify multiple contexts can be created and freed independently
echo "\n--- Test 5: Multiple contexts independent lifecycle ---\n";
$ctxs = [];
for ($i = 0; $i < 3; $i++) {
    $ctxs[$i] = new GuileContext();
    $ctxs[$i]->eval("(define idx $i)");
    echo "Created context $i with idx=$i\n";
}

// Free middle one
echo "Freeing context 1...\n";
$ctxs[1]->free();

// Verify others still work
echo "Verifying context 0 and 2 still work:\n";
$val0 = $ctxs[0]->eval('idx');
$val2 = $ctxs[2]->eval('idx');
echo "Context 0 idx: $val0\n";
echo "Context 2 idx: $val2\n";

if ($val0 === '0' && $val2 === '2') {
    echo "PASS: Other contexts unaffected by middle context free\n";
} else {
    echo "FAIL: Context isolation broken\n";
}

// Clean up remaining
unset($ctxs[0]);
unset($ctxs[2]);

echo "\n=== All tests completed ===\n";
?>
--EXPECTF--
=== Testing GuileContext::free() and __destruct() ===

--- Test 1: Basic free() method ---
Created GuileContext instance
1 + 2 = 3
Called free(): success

--- Test 2: Double free() should be safe ---
Called free() again: success (no-op)

--- Test 3: Automatic __destruct() cleanup ---
Created context inside function, defined temp-var
Function returned, object should be automatically cleaned up

--- Test 4: Using context after free should fail ---
Set test-val in fresh context
Freed the context
Exception caught (expected): Context not found for this object
PASS: Using freed context correctly throws exception

--- Test 5: Multiple contexts independent lifecycle ---
Created context 0 with idx=0
Created context 1 with idx=1
Created context 2 with idx=2
Freeing context 1...
Verifying context 0 and 2 still work:
Context 0 idx: 0
Context 2 idx: 2
PASS: Other contexts unaffected by middle context free

=== All tests completed ===
