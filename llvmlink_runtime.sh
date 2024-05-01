#!/bin/bash

# Step 1: Compile each source file to LLVM IR
for file in cjq/runtime/*.c; do
    filename=$(basename -- "$file")
    filename="${filename%.*}"  # Extract filename without extension
    clang -S -emit-llvm -o "cjq/runtime/$filename.ll" "$file"
done

# Step 2: Link LLVM IR files together
llvm-link -S -o runtime_combined.ll cjq/runtime/*.ll