#!/bin/bash

llvm-link -S -o runjq.ll runtime_combined.ll frontend_combined.ll jq_src_combined.ll llvmlib_combined.ll pylib_combined.ll ir.ll
opt -S -passes='default<O3>,inline' runjq.ll -o runjq_inlined.ll
clang -O3 runjq_inlined.ll -fPIE -lm -lpython3.12 -lonig -pg -o runjq -no-pie