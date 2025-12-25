--TEST--
GuileContext::eval() with empty code should return empty string
--FILE--
<?php
echo "Testing empty code handling...\n";

$ctx = new GuileContext();
echo "Created GuileContext instance\n";

// Test empty string - should return empty string without error
$result = $ctx->eval('');
echo "Empty code result: '$result'\n";
echo "Empty code length: " . strlen($result) . "\n";

// Test with some valid code before and after
$result = $ctx->eval('(+ 10 20)');
echo "1 + 2 = $result\n";

echo "PASS: Empty code handling works\n";
?>
--EXPECT--
Testing empty code handling...
Created GuileContext instance
Empty code result: ''
Empty code length: 0
1 + 2 = 30
PASS: Empty code handling works
