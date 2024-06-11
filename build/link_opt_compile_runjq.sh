# #!/bin/bash

# clang -g -S -funwind-tables -O3 -emit-llvm -o cjq_trace.ll cjq/trace/cjq_trace.c
# llvm-link -S -o runjq.ll runtime_combined.ll jq_src_combined.ll llvmlib_combined.ll pylib_combined.ll cjq_trace.ll ir.ll
# # opt -S -passes='default<O3>,inline' runjq.ll -o runjq_inlined.ll
# # clang -O3 runjq_inlined.ll -fPIE -lm -lpython3.12 -lonig -g -o runjq -no-pie
# clang -O3 runjq.ll -fPIE -lm -lpython3.12 -lonig -g -o runjq -no-pie

#!/bin/bash

# Ensure there's enough swap space (temporary increase for the session)
if [[ $(free | awk '/^Swap:/ {print $2}') -eq 0 ]]; then
    echo "Creating swap file..."
    sudo fallocate -l 4G /swapfile
    sudo chmod 600 /swapfile
    sudo mkswap /swapfile
    sudo swapon /swapfile
fi

# Set ulimit to allow more memory and stack size
ulimit -s unlimited
ulimit -v unlimited

echo "Starting compilation process..."

# Compile the source code with resource prioritization
echo "Compiling cjq_trace.c to LLVM IR..."
nice -n 10 ionice -c 3 clang -g -S -funwind-tables -O3 -emit-llvm -o cjq_trace.ll cjq/trace/cjq_trace.c

if [[ $? -ne 0 ]]; then
    echo "Failed to compile cjq_trace.c"
    exit 1
fi

# Split the LLVM IR files linking process
echo "Linking LLVM IR files..."
nice -n 10 ionice -c 3 llvm-link -S -o runjq_part1.ll runtime_combined.ll jq_src_combined.ll llvmlib_combined.ll
if [[ $? -ne 0 ]]; then
    echo "Failed to link first part of LLVM IR files"
    exit 1
fi

nice -n 10 ionice -c 3 llvm-link -S -o runjq_part2.ll pylib_combined.ll cjq_trace.ll ir.ll
if [[ $? -ne 0 ]]; then
    echo "Failed to link second part of LLVM IR files"
    exit 1
fi

# Link both parts together
nice -n 10 ionice -c 3 llvm-link -S -o runjq.ll runjq_part1.ll runjq_part2.ll
if [[ $? -ne 0 ]]; then
    echo "Failed to link all LLVM IR files"
    exit 1
fi

# # opt -S -passes='default<O3>,inline' runjq.ll -o runjq_inlined.ll
# # clang -O3 runjq_inlined.ll -fPIE -lm -lpython3.12 -lonig -g -o runjq -no-pie

# Compile the final executable
echo "Compiling final executable..."
nice -n 10 ionice -c 3 clang -O3 runjq.ll -fPIE -lm -lpython3.12 -lonig -g -o runjq -no-pie
if [[ $? -ne 0 ]]; then
    echo "Failed to compile final executable"
    exit 1
fi

# Cleanup swap space if it was added
if [[ -f /swapfile ]]; then
    sudo swapoff /swapfile
    sudo rm /swapfile
fi

echo "Compilation process completed successfully."
