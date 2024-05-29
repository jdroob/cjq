#!/bin/bash

./build/compile_llvmgen.sh
./linking/llvmlink_clib.sh
./linking/llvmlink_frontend.sh
./linking/llvmlink_jqsrc.sh
./linking/llvmlink_llvmlib.sh
./linking/llvmlink_pylib.sh
./linking/llvmlink_runtime.sh
./linking/llvmlink_trace.sh