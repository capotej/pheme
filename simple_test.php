<?php
// Simple test to check if extension loads
echo "Testing pheme extension loading...\n";

try {
    hello_world();
    echo "hello_world() works!\n";
} catch (Exception $e) {
    echo "hello_world() failed: " . $e->getMessage() . "\n";
}

try {
    $result = add_numbers(5, 3);
    echo "add_numbers(5, 3) = $result\n";
} catch (Exception $e) {
    echo "add_numbers() failed: " . $e->getMessage() . "\n";
}

echo "Basic tests completed!\n";
?>