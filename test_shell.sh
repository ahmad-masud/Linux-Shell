#!/bin/bash

# Function to run a test case
run_test() {
    local test_input=$1
    local expected_output=$2

    echo "Running test: $test_input"
    echo "$test_input" | ./shell > actual_output.txt 2>&1
    if diff -q actual_output.txt "$expected_output" > /dev/null; then
        echo "Test passed"
    else
        echo "Test failed"
        echo "Expected:"
        cat "$expected_output"
        echo "Got:"
        cat actual_output.txt
    fi
    echo ""
}

# Create expected outputs
echo "/your/current/directory" > expected_pwd.txt
echo "Too many arguments" > expected_cd_error.txt
echo "Too many arguments" > expected_help_error.txt
echo "exit: Exits the shell\npwd: Prints the current working directory\ncd: Changes the current working directory\nhelp: Prints a list of internal commands\n" > expected_help.txt
echo "Hello, World!" > expected_echo.txt

# Run tests
run_test "pwd" expected_pwd.txt
run_test "cd ..; pwd" expected_pwd.txt
run_test "help" expected_help.txt
run_test "echo Hello, World!" expected_echo.txt
run_test "cd too many arguments" expected_cd_error.txt
run_test "help too many arguments" expected_help_error.txt

# Cleanup
rm -f actual_output.txt