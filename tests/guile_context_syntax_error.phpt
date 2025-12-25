--TEST--
Test error handling for invalid Scheme syntax
--DESCRIPTION--
Verifies that GuileContext::eval() properly throws exceptions for various
types of invalid Scheme syntax. Tests unmatched parentheses, undefined
functions, malformed expressions, and type errors. Each error should throw
a PHP Exception.
--FILE--
<?php
echo "=== Testing Error Handling for Invalid Scheme Syntax ===\n\n";

$ctx = new GuileContext();
echo "Created GuileContext instance\n";

// Test 1: Unmatched parentheses
echo "\n--- Test 1: Unmatched parentheses ---\n";
try {
    $ctx->eval('(+ 1 2');
    echo "FAIL: Unmatched parentheses - no exception thrown\n";
} catch (Exception $e) {
    echo "PASS: Unmatched parentheses throws exception\n";
}

// Test 2: Undefined variable (runtime error)
echo "\n--- Test 2: Undefined variable ---\n";
try {
    $ctx->eval('(undefined-variable)');
    echo "FAIL: Undefined variable - no exception thrown\n";
} catch (Exception $e) {
    echo "PASS: Undefined variable throws exception\n";
}

// Test 3: Subtract with wrong args (minus requires at least 1 arg)
echo "\n--- Test 3: Subtract with no arguments ---\n";
try {
    $ctx->eval('(-)');
    echo "FAIL: Subtract with no arguments - no exception thrown\n";
} catch (Exception $e) {
    echo "PASS: Subtract with no arguments throws exception\n";
}

// Test 4: Invalid define syntax (define expects a symbol, not a string)
echo "\n--- Test 4: Invalid define syntax ---\n";
try {
    $ctx->eval('(define "wrong" 42)');
    echo "FAIL: Invalid define syntax - no exception thrown\n";
} catch (Exception $e) {
    echo "PASS: Invalid define syntax throws exception\n";
}

// Test 5: Division by zero (runtime error)
echo "\n--- Test 5: Division by zero (runtime error) ---\n";
try {
    $ctx->eval('(/ 1 0)');
    echo "FAIL: Division by zero - no exception thrown\n";
} catch (Exception $e) {
    echo "PASS: Division by zero throws exception\n";
}

// Test 6: Car on non-pair
echo "\n--- Test 6: Car on non-pair ---\n";
try {
    $ctx->eval('(car 5)');
    echo "FAIL: Car on non-pair - no exception thrown\n";
} catch (Exception $e) {
    echo "PASS: Car on non-pair throws exception\n";
}

// Test 7: Cdr on non-pair
echo "\n--- Test 7: Cdr on non-pair ---\n";
try {
    $ctx->eval('(cdr 5)');
    echo "FAIL: Cdr on non-pair - no exception thrown\n";
} catch (Exception $e) {
    echo "PASS: Cdr on non-pair throws exception\n";
}

// Test 8: Cons with wrong number of args
echo "\n--- Test 8: Cons with wrong number of args ---\n";
try {
    $ctx->eval('(cons 1 2 3)');
    echo "FAIL: Cons with too many args - no exception thrown\n";
} catch (Exception $e) {
    echo "PASS: Cons with too many args throws exception\n";
}

echo "\n=== All syntax error tests completed ===\n";
?>
--EXPECTF--
=== Testing Error Handling for Invalid Scheme Syntax ===

Created GuileContext instance

--- Test 1: Unmatched parentheses ---
%a
PASS: Unmatched parentheses throws exception

--- Test 2: Undefined variable ---
%a
PASS: Undefined variable throws exception

--- Test 3: Subtract with no arguments ---
%a
PASS: Subtract with no arguments throws exception

--- Test 4: Invalid define syntax ---
%a
PASS: Invalid define syntax throws exception

--- Test 5: Division by zero (runtime error) ---
%a
PASS: Division by zero throws exception

--- Test 6: Car on non-pair ---
%a
PASS: Car on non-pair throws exception

--- Test 7: Cdr on non-pair ---
%a
PASS: Cdr on non-pair throws exception

--- Test 8: Cons with wrong number of args ---
%a
PASS: Cons with too many args throws exception

=== All syntax error tests completed ===
