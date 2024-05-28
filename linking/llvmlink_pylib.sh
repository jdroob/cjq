#!/bin/bash

# Step 1: Compile each source file to LLVM IR
for file in cjq/pylib/*.c; do
    filename=$(basename -- "$file")
    filename="${filename%.*}"  # Extract filename without extension
    clang -S -funwind-tables -O2 -emit-llvm -o "cjq/pylib/$filename.ll" "$file"
done

# Step 2: Link LLVM IR files together
llvm-link -S -o pylib_combined.ll cjq/pylib/*.ll
