#!/bin/bash

# Flag variables to track whether -s or -f option is provided
has_s_option=false
has_f_option=false

# Step 1: Construct Python command based on options
python_command="python cjq/main.py"
output_file="jqprgm"
while getopts :hs:f:o: opt; do
    case $opt in 
        h) echo "Help message here..."; exit ;;
        s) python_command+=" -s '$OPTARG'" 
           has_s_option=true ;;
        f) python_command+=" -f $OPTARG" 
           has_f_option=true ;;
        o) output_file="$OPTARG" ;;
        :) echo "Missing argument for option -$OPTARG"; exit 1;;
        \?) echo "Unknown option -$OPTARG"; exit 1;;
    esac
done

# Check if either -s or -f option is provided
if ! $has_s_option && ! $has_f_option; then
    echo "Either -s or -f option must be provided."
    exit 1
fi

# Step 2: Generate LLVM IR
$python_command "${@:3}" > ir.ll

# # Step 3: Compile the LLVM IR and other source files
clang cjq/main.c cjq/llvmlib/io.c cjq/clib/lib.c ir.ll -o "$output_file" -ljson-c

# # Step 4: Execute the compiled program with JSON file paths as arguments
./"$output_file" "${@:3}"
