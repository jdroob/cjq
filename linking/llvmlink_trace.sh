#!/bin/bash

# Step 1: Compile each source file to LLVM IR
for file in cjq/trace/*.c; do
    filename=$(basename -- "$file")
    filename="${filename%.*}"  # Extract filename without extension
    clang -g -S -funwind-tables -emit-llvm -I/home/rubio/anaconda3/envs/numbaEnv/include/python3.12 -o "cjq/trace/$filename.ll" "$file"
done

# Step 2: Link LLVM IR files together
llvm-link -S -o trace_combined.ll cjq/trace/*.ll
