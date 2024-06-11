#!/bin/bash

# Step 1: Compile each source file to LLVM IR
for file in cjq/runtime/*.c; do
    filename=$(basename -- "$file")
    filename="${filename%.*}"  # Extract filename without extension
    clang -g -S -funwind-tables -O3 -emit-llvm -o "cjq/runtime/$filename.ll" "$file" -Wno-deprecated-non-prototype
done

# Step 2: Link LLVM IR files together
llvm-link -S -o runtime_combined.ll cjq/runtime/*.ll