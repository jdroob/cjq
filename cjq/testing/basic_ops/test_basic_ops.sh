#!/bin/bash

# Define JSON file paths as variables
json_file1="$HOME/cjq/cjq/testing/basic_ops/json/prod_example/prod1.json"
json_file2="$HOME/cjq/cjq/testing/basic_ops/json/prod_example/prod2.json"

# Function to compare outputs and write result to testresults.log
function compare_outputs {
    if [ "$1" = "$2" ]; then
        echo "PASSED : $3" >> cjq/testing/basic_ops/testresults.log
        ((passed_tests++))
    else
        echo "FAILED : $3" >> cjq/testing/basic_ops/testresults.log
        echo "Differences in output for $3:" >> cjq/testing/basic_ops/testresults.log
        diff <(echo "$1") <(echo "$2") >> cjq/testing/basic_ops/testresults.log
        ((failed_tests++))
    fi
    ((total_tests++))
}

# Command 0: Clear testing log
echo "" > cjq/testing/basic_ops/testresults.log

# Print message indicating testing below opcode functions
echo "Testing builtin operators and functions:"
# echo "TESTING BELOW OPCODE FUNCTIONS:"
# echo -e "\nTOP"
# echo "SUBEXP_BEGIN"
# echo "SUBEXP_END"
# echo "PUSHK_UNDER"
# echo "INDEX"
# echo "CALL_BUILTIN"
# echo "LOADK"
# echo "DUP"
# echo "RET"
echo -e "\nRunning tests...\n"

# Initialize test counts
passed_tests=0
failed_tests=0
total_tests=0
total_coverage=0

# Loop through each .jq file in the directory
for jq_file in cjq/testing/basic_ops/jq/prod_example/*.jq; do
    jq_filename=$(basename "$jq_file")  # Get the filename without the path
    # echo "Running tests for $jq_filename..."
    
    # Command 1: Generate LLVM IR (suppress output)
    # echo "Generating LLVM IR for $jq_filename..." # Debug
    ./llvm_gen -f -s "$jq_file" "$json_file1" "$json_file2" --debug-dump-disasm >/dev/null
    
    # Compile runjq
    # echo "Compiling jq for $jq_filename..." # Debug
    clang cjq/runtime/main.c cjq/frontend/*.c cjq/jq/src/*.c cjq/llvmlib/*.c cjq/pylib/*.c cjq/jq/src/decNumber/decNumber.c cjq/jq/src/decNumber/decContext.c ir.ll -g -lm -o runjq -lpython3.12 -DUSE_DECNUM=1 -Wno-deprecated-non-prototype
    
    # Command 2: Run runjq and capture output
    # echo "Running cjq for $jq_filename..."  # Debug
    cjq_output=$(./runjq -f -s "$jq_file" "$json_file1" "$json_file2" --debug-dump-disasm)

    # Command 3: Run jq and capture output
    # echo "Running jq for $jq_filename..." # Debug
    jq_output=$(jq -f -s "$jq_file" "$json_file1" "$json_file2" --debug-dump-disasm)

    # Compare outputs and write result to testing.log
    compare_outputs "$cjq_output" "$jq_output" "$jq_filename"
done

# Further testing

# Define test cases
test_cases=("add_example1" "add_example2" "add_example3" "add_example4" 
            "sub_example1" "sub_example2" "muldiv_example1" "muldiv_example2"
            "muldiv_example3" "muldiv_example4" "mod_example1" "abs_example1"
            "length_example1" "utf8bytelength_example1" "keys_example1" 
            "keys_example2" "keys_unsorted_example1" "has_example1"
            "has_example2" "in_example1" "in_example2" "map_example1"
            "map_example2" "map_values_example1" "map_values_example2"
            "pick_example1" "pick_example2" "pick_example3" "pick_example4"
            "del_example1" "del_example2" "getpath_example1" "getpath_example2"
            "setpath_example1" "setpath_example2" "setpath_example3"
            "delpaths_example1" "toentries_example1" "fromentries_example1"
            "withentries_example1" "select_example1" "select_example2"
            "select_numbers" "select_strings" "select_arrays" "select_objects"
            "select_iterables" "select_booleans" "select_normals" "select_finites"
            "select_nulls" "select_values" "select_scalars" "empty_example1"
            "empty_example2" "error_example1" "error_example2" "halterror_example1"
            "halt_example1" "loc_example1" "paths_example1" "paths_example2"
            "_add_example1" "_add_example2" "_add_example3" "all_example1"
            "all_example2" "all_example3" "any_example1" "any_example2" 
            "any_example3" "flatten_example1" "flatten_example2" "flatten_example3"
            "flatten_example4" "range_example1" "range_example2" "range_example3"
            "range_example4" "floor_example1" "sqrt_example1" "tonumber_example1"
            "tostring_example1" "type_example1" "infinite_example1" "infinite_example2"
            "sort_example1" "sort_example2" "sort_example3" "groupby_example1"
            "groupby_example2" "unique_example1" "unique_example2" "unique_example3"
            "reverse_example1" "contains_example1" "contains_example2" "contains_example3"
            "contains_example4" "contains_example5" "indices_example1" "indices_example2"
            "indices_example3")

for test_case in "${test_cases[@]}"; do
    jq_file="$HOME/cjq/cjq/testing/basic_ops/jq/builtin_ops/${test_case}.jq"
    json_file="$HOME/cjq/cjq/testing/basic_ops/json/builtin_ops/${test_case}.json"

    # Command 1: Generate LLVM IR (suppress output)
    # echo "Generating LLVM IR for $jq_file..." # Debug
    ./llvm_gen -f "$jq_file" "$json_file" --debug-dump-disasm > /dev/null

    # Compile runjq
    # echo "Compiling jq for $jq_file..." # Debug
    ./compile_runjq.sh

    # Command 2: Run runjq and capture output
    # echo "Running cjq for $jq_file..."  # Debug
    cjq_output=$(./runjq -f "$jq_file" "$json_file" --debug-dump-disasm)

    # Command 3: Run jq and capture output
    # echo "Running jq for $jq_file..." # Debug
    jq_output=$(jq -f "$jq_file" "$json_file" --debug-dump-disasm)

    # Compare outputs and write result to testing.log
    compare_outputs "$cjq_output" "$jq_output" "$test_case.jq"
done


# Print test execution summary
echo "======================================"
echo "         Test Execution Summary"
echo "======================================"
echo "Total Tests: $total_tests"
echo "Passed Tests: $passed_tests"
echo "Failed Tests: $failed_tests"
awk -v var1=$passed_tests -v var2=$total_tests 'BEGIN { printf "Test Coverage: %.2f%%\n", ( var1 / var2 * 100) }'
