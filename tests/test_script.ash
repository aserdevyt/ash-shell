

# Test Script for ash shell features
# This script tests various shell mechanisms including:
# - Variables and expansion
# - Pipes and redirections
# - Background jobs
# - Conditionals (&&, ||)
# - Comments
# - Command substitution
# - File operations

echo "=== Starting ash shell feature tests ==="

# Test 1: Variable Assignment and Expansion
echo "\nTest 1: Variables"
TEST_VAR="Hello from ash"
echo "Variable content: $TEST_VAR"
PATH+=":/custom/path"
echo "Modified PATH: $PATH"

# Test 2: I/O Redirection
echo "\nTest 2: I/O Redirection"
echo "This is a test file" > test_output.txt
echo "Adding another line" >> test_output.txt
cat < test_output.txt
rm test_output.txt

# Test 3: Pipes
echo "\nTest 3: Pipes"
echo "Testing pipe mechanism" | grep "pipe"
ls -l | grep "test" | wc -l

# Test 4: Background Jobs
echo "\nTest 4: Background Jobs"
sleep 2 &
echo "Started sleep in background"
sleep 1
jobs

# Test 5: Logical Operators
echo "\nTest 5: Logical Operators"
true && echo "AND operator works"
false || echo "OR operator works"
false && echo "This should not print" || echo "Error handling works"

# Test 6: Multiple Commands
echo "\nTest 6: Multiple Commands"
echo "First" ; echo "Second" ; echo "Third"

# Test 7: Directory Operations
echo "\nTest 7: Directory Operations"
mkdir -p test_dir
cd test_dir
pwd
cd ..
rm -r test_dir

# Test 8: Command Substitution via pipes
echo "\nTest 8: Complex Command Chaining"
ls -la | grep "test" > output.txt && echo "Files listed" || echo "No files found"
rm -f output.txt

# Test 9: Error Handling
echo "\nTest 9: Error Handling"
nonexistent_command 2>/dev/null || echo "Error was caught correctly"

# Test 10: Environment Variables
echo "\nTest 10: Environment Variables"
export TEST_ENV="test_value"
echo "Exported variable: $TEST_ENV"

# Final status check
echo "\n=== Test script completed ==="
status  # Show last command status and execution time

# Cleanup any remaining background jobs
jobs