--TEST--
Basic GuileContext creation and evaluation test
--FILE--
<?php
echo "Testing GuileContext extension...\n";

$ctx = new GuileContext();
echo "Created GuileContext instance\n";

$result = $ctx->eval('(+ 1 2)');
echo "1 + 2 = $result\n";

$result = $ctx->eval('(define x 42)');
echo "Defined x = 42: $result\n";

$result = $ctx->eval('x');
echo "Read x: $result\n";

echo "PASS: Basic functionality works\n";
?>
--EXPECT--
Testing GuileContext extension...
Created GuileContext instance
1 + 2 = 3
Defined x = 42: #<unspecified>
Read x: 42
PASS: Basic functionality works
