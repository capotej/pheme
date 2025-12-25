--TEST--
Test that two separate GuileContext instances do not share variables
--DESCRIPTION--
This test verifies that each GuileContext has its own isolated namespace
and that variables set in one context are not visible in another.
--FILE--
<?php
// Suppress PHP warnings for cleaner output
error_reporting(E_ALL & ~E_WARNING);

echo "=== Testing GuileContext Isolation ===\n\n";

// Create two separate contexts
$ctx1 = new GuileContext();
$ctx2 = new GuileContext();

echo "Created two GuileContext instances\n";

// Test 1: Set a variable in context 1
echo "\n--- Test 1: Variable isolation check ---\n";
$result1 = $ctx1->eval('(define test-var "hello from context 1")');
echo "Context 1 - Set test-var: $result1\n";

$check_ctx1 = $ctx1->eval('test-var');
echo "Context 1 - Read test-var: $check_ctx1\n";

// Now check if context 2 has this variable - it should NOT
// Guile will error when trying to access undefined variable
$check_ctx2 = @$ctx2->eval('test-var');
// The error output will appear before our message
// We check the result - it should be false
if ($check_ctx2 === false) {
    echo "Context 2 - test-var isolation verified!\n";
    $isolation_ok = true;
} else {
    echo "Context 2 - test-var value: $check_ctx2\n";
    $isolation_ok = false;
}

// Test 2: Set a different variable in context 2
echo "\n--- Test 2: Independent variable definition ---\n";
$result2 = $ctx2->eval('(define test-var "hello from context 2")');
echo "Context 2 - Set test-var: $result2\n";

$check_ctx1_again = $ctx1->eval('test-var');
echo "Context 1 - Read test-var again: $check_ctx1_again\n";

$check_ctx2_again = $ctx2->eval('test-var');
echo "Context 2 - Read test-var again: $check_ctx2_again\n";

// Test 3: Verify each context has completely independent state
echo "\n--- Test 3: State independence verification ---\n";
$ctx1->eval('(define count 100)');
$ctx2->eval('(define count 200)');

$count1 = $ctx1->eval('count');
$count2 = $ctx2->eval('count');

echo "Context 1 - count: $count1\n";
echo "Context 2 - count: $count2\n";

// Final verification
echo "\n=== Test Results ===\n";
$passed = true;

if (!$isolation_ok) {
    $passed = false;
    echo "FAIL: Contexts are sharing variables!\n";
}

if ($check_ctx1_again !== '"hello from context 1"') {
    $passed = false;
    echo "FAIL: Context 1 should retain its test-var value\n";
}

if ($check_ctx2_again !== '"hello from context 2"') {
    $passed = false;
    echo "FAIL: Context 2 should retain its test-var value\n";
}

if ($count1 !== '100') {
    $passed = false;
    echo "FAIL: Context 1 count should be 100, got: $count1\n";
}

if ($count2 !== '200') {
    $passed = false;
    echo "FAIL: Context 2 count should be 200, got: $count2\n";
}

if ($passed) {
    echo "PASS: All isolation tests passed\n";
    echo "Each GuileContext maintains its own isolated namespace.\n";
}

?>
--EXPECTF--
=== Testing GuileContext Isolation ===

Created two GuileContext instances

--- Test 1: Variable isolation check ---
Context 1 - Set test-var: #<unspecified>
Context 1 - Read test-var: "hello from context 1"
%a
Context 2 - test-var isolation verified!

--- Test 2: Independent variable definition ---
Context 2 - Set test-var: #<unspecified>
Context 1 - Read test-var again: "hello from context 1"
Context 2 - Read test-var again: "hello from context 2"

--- Test 3: State independence verification ---
Context 1 - count: 100
Context 2 - count: 200

=== Test Results ===
PASS: All isolation tests passed
Each GuileContext maintains its own isolated namespace.
