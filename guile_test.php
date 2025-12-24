<?php
// Test just the Guile functions
echo "Testing Guile functions...\n";

try {
    echo "Testing guile_eval with simple expression...\n";
    $result = guile_eval("(+ 1 2)");
    echo "Result: $result\n";
} catch (Exception $e) {
    echo "Error: " . $e->getMessage() . "\n";
}

echo "Guile test completed!\n";
?>