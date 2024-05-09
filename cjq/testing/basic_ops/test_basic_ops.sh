#!/bin/bash

# Command 1: Generate LLVM IR
echo "Generating LLVM IR..."
./llvm_gen -f -s -C cjq/testing/basic_ops/jq/add.jq /home/rubio/cjq/cjq/testing/basic_ops/json/prod1.json /home/rubio/cjq/cjq/testing/basic_ops/json/prod2.json --debug-dump-disasm

# Command 2: Compile runjq
echo "Compiling runjq..."
clang cjq/runtime/main.c cjq/frontend/*.c cjq/jq/src/*.c cjq/llvmlib/*.c cjq/pylib/*.c ir.ll -g -lm -o runjq -lpython3.12

# Command 3: Run runjq and write output to testresults.log
echo "Running runjq..."
./runjq -f -s -C cjq/testing/basic_ops/jq/add.jq /home/rubio/cjq/cjq/testing/basic_ops/json/prod1.json /home/rubio/cjq/cjq/testing/basic_ops/json/prod2.json --debug-dump-disasm | tee -a cjq/testing/basic_ops/testresults.log
echo "Done"