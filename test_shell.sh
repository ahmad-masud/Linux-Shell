#!/bin/bash

# Create a temporary file for testing
TMP_OUTPUT=$(mktemp)

# Function to run a test
run_test() {
    local input=$1
    local expected_output=$2
    local test_description=$3

    # Run the shell with the input and capture the output
    echo "$input" | ./shell > "$TMP_OUTPUT" 2>&1

    # Check the output
    if grep -q "$expected_output" "$TMP_OUTPUT"; then
        echo "PASS: $test_description"
    else
        echo "FAIL: $test_description"
        echo "Expected: $expected_output"
        echo "Got: $(cat $TMP_OUTPUT)"
    fi
}

# Test cases
run_test "pwd" "$HOME" "Test 'pwd' command"
run_test "cd / && pwd" "/" "Test 'cd /' command"
run_test "cd - && pwd" "$HOME" "Test 'cd -' command (previous directory)"
run_test "cd ~ && pwd" "$HOME" "Test 'cd ~' command (home directory)"
run_test "echo Hello World" "Hello World" "Test 'echo Hello World' command"
run_test "history" "history" "Test 'history' command"
run_test "help" "exit: Exits the shell" "Test 'help' command"
run_test "exit" "" "Test 'exit' command"

# Clean up
rm "$TMP_OUTPUT"