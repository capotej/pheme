--TEST--
Test that errors throw exceptions
--DESCRIPTION--
Verifies that GuileContext::eval() throws exceptions on errors
--FILE--
<?php
echo "=== Testing Error Handling ===\n\n";

$ctx = new GuileContext();
echo "Created GuileContext instance\n";

// Test 1: Syntax error - unmatched parentheses
echo "\n--- Test 1: Syntax error (unmatched parentheses) ---\n";
try {
    $ctx->eval('(+ 1 2');
    echo "FAIL: No exception thrown\n";
} catch (Exception $e) {
    echo "Exception class: " . get_class($e) . "\n";
    echo "Exception message: " . $e->getMessage() . "\n";
    if (get_class($e) === 'Exception') {
        echo "PASS: Syntax error threw exception\n";
    } else {
        echo "FAIL: Expected Exception class\n";
    }
}

// Test 2: Runtime error - undefined variable
echo "\n--- Test 2: Runtime error (undefined variable) ---\n";
try {
    $ctx->eval('(undefined-variable 123)');
    echo "FAIL: No exception thrown\n";
} catch (Exception $e) {
    echo "Exception class: " . get_class($e) . "\n";
    echo "Exception message: " . $e->getMessage() . "\n";
    if (get_class($e) === 'Exception') {
        echo "PASS: Runtime error threw exception\n";
    } else {
        echo "FAIL: Expected Exception class\n";
    }
}

// Test 3: Runtime error - division by zero
echo "\n--- Test 3: Runtime error (division by zero) ---\n";
try {
    $ctx->eval('(/ 1 0)');
    echo "FAIL: No exception thrown\n";
} catch (Exception $e) {
    echo "Exception class: " . get_class($e) . "\n";
    echo "Exception message: " . $e->getMessage() . "\n";
    if (get_class($e) === 'Exception') {
        echo "PASS: Runtime error threw exception\n";
    } else {
        echo "FAIL: Expected Exception class\n";
    }
}

// Test 4: Runtime error - wrong type (car on non-pair)
echo "\n--- Test 4: Runtime error (wrong type - car on non-pair) ---\n";
try {
    $ctx->eval('(car 5)');
    echo "FAIL: No exception thrown\n";
} catch (Exception $e) {
    echo "Exception class: " . get_class($e) . "\n";
    echo "Exception message: " . $e->getMessage() . "\n";
    if (get_class($e) === 'Exception') {
        echo "PASS: Runtime error threw exception\n";
    } else {
        echo "FAIL: Expected Exception class\n";
    }
}

// Test 5: Valid code - no error
echo "\n--- Test 5: Valid code (no error) ---\n";
try {
    $result = $ctx->eval('(+ 1 2 3)');
    echo "Result: $result\n";
    if ($result === "6") {
        echo "PASS: Valid code executed correctly\n";
    } else {
        echo "FAIL: Unexpected result\n";
    }
} catch (Exception $e) {
    echo "FAIL: Unexpected exception: " . $e->getMessage() . "\n";
}

echo "\n=== All error handling tests completed ===\n";
?>
--EXPECTF--
=== Testing Error Handling ===

Created GuileContext instance

--- Test 1: Syntax error (unmatched parentheses) ---
Exception class: Exception
Exception message: #<unknown port>:%S
PASS: Syntax error threw exception

--- Test 2: Runtime error (undefined variable) ---
Exception class: Exception
Exception message: Unbound variable:%S
PASS: Runtime error threw exception

--- Test 3: Runtime error (division by zero) ---
Exception class: Exception
Exception message: Numerical overflow
PASS: Runtime error threw exception

--- Test 4: Runtime error (wrong type - car on non-pair) ---
Exception class: Exception
Exception message: Wrong type (expecting %S
PASS: Runtime error threw exception

--- Test 5: Valid code (no error) ---
Result: 6
PASS: Valid code executed correctly

=== All error handling tests completed ===
