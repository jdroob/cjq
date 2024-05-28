#!/bin/bash

# Step 1: Compile each source file to LLVM IR
for file in cjq/clib/*.c; do
    filename=$(basename -- "$file")
    filename="${filename%.*}"  # Extract filename without extension
    clang -S -funwind-tables -O3 -emit-llvm -o "cjq/clib/$filename.ll" "$file"
done

# Step 2: Link LLVM IR files together
llvm-link -S -o clib_combined.ll cjq/clib/*.ll
