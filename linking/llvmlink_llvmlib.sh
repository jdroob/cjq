#!/bin/bash

# Step 1: Compile each source file to LLVM IR
for file in cjq/llvmlib/*.c; do
    filename=$(basename -- "$file")
    filename="${filename%.*}"  # Extract filename without extension
    clang -S -funwind-tables -O2 -emit-llvm -o "cjq/llvmlib/$filename.ll" "$file"
done

# Step 2: Link LLVM IR files together
llvm-link -S -o llvmlib_combined.ll cjq/llvmlib/*.ll
