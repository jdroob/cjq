# cjq
`cjq` is an implementation of the `jq` programming language. `jq` is a powerful JSON processing tool similar
in flavor to `sed`, `awk`, and `grep`. The [standard implementation](https://github.com/jqlang/jq) of `jq` is a bytecode interpreter. `cjq` borrows many of `jq`'s internals but implements a bytecode to LLVM compiler rather than a bytecode interpreter.

`cjq` does not currently lower to LLVM IR in the same way most traditional compilers do. `cjq` works by first running the standard `jq` interpreter on a `jq` program (or filter) and 0 or more JSON documents. As the VM executes `jq` bytecode instructions, the dynamic sequence of bytecode opcodes is recorded (or traced) and lowered to LLVM IR while the VM is still runnning. Using `clang`, the generated LLVM IR can be lowered to native machine code. The resultant binary executable can then process JSON input(s) from the command line in a similar way to the standard `jq` implementation. The only difference being that the binary executable contains the traced information so the `jq` filter does not need to be provided.

Example:

1. Trace and lower to LLVM IR
```jq
./llvmgen -Cf /path/to/jq/file /path/to/json/file
```

2. Compile to native machine code
```jq
make run    # or 'make run_opt' for O3 clang optimizations
```

3. Run the binary executable on compatible JSON documents
```jq
./run -Cf /path/to/json/file
```


`cjq` is currently in version 0.1... and it shows. Obviously, tracing a dynamic sequence of opcodes becomes a bottleneck as the number of dynamic opcodes becomes larger. To help mitigate this, `cjq` does use a variant of [run-length encoding](https://en.wikipedia.org/wiki/Run-length_encoding) to compress the dynamic sequence of opcodes in an effort to slow the rapid explosion in size of the generated LLVM IR. However, compression can only reduce the code size so much. Consequently, `cjq` is not the ideal tool for executing simple `jq` filters that incur many dynamic opcodes.

Example:

```jq
echo 1000000 | ./llvmgen -Cf  path/to/jq_benchmarks/add.jq
```
add.jq
```jq
[range(.) | [.]] | add
```

The above `jq` filter incurs over 1,000,000 dynamic opcodes and produces slightly more lines of LLVM IR. This results in tracing taking a long time and compiling taking even longer. It should be noted that the above filter happens to produce a sequence of dynamic opcodes that is not as amenable to compression as others (i.e. opcode sequences of the same scale exist that can be compressed more efficiently than in this example).

`cjq` _is_ a good choice for compiling long, verbose (for `jq`) programs to an optimized binary executable. So you can process compatible JSON data without needing to enter a long `jq` filter every time.

Example (borrowed from [this](https://www.youtube.com/watch?v=EIhLl9ebeiA&t=84s&ab_channel=SzymonStepniak) tutorial):

```jq
./llvmgen '[.docs[] | { title, author_name: .author_name[0], publish_year: .publish_year[0] } | select(.publish_year != null and .author_name != null)] | group_by(.author_name) | .[] | {author_name: .[0].author_name}' path/to/openlibrary_example1.json
```

```jq
make run_opt -j4   # compile with optimizations
```

```jq
./run_opt /path/to/openlibrary_example1.json    # no more long filter required!
```

In case you're interested, `cjq` does perform quite well compared to the standard `jq` implementation [TODO: add graphs comparing performance]. The obvious caveat being the compile time overhead introduced by `cjq`. For very large opcode sequences that are not good candidates for compression, compilation can take hours... consider yourself warned⚠️

## Installation

### Dependencies

- make
- $\ge$ python version 3.10
- [python3-config](https://helpmanual.io/man1/python3-config/)
- [llvmlite](https://llvmlite.readthedocs.io/en/latest/index.html)

### Instructions
```console
# clone the cjq repo
make llvmgen    # run this command
./llvmgen   # a help menu should show up
./cjq/tests/basic_ops/test_basic_ops.sh   # run a bunch of tests to see if your installation is working. You can go to cjq/tests/basic_ops, and find a testresults.log file
# if you fail any tests, there's an installation issue
# If something fails, you might need to cd as follows
cd cjq/jq/oniguruma
# Read the oniguruma README on how to install oniguruma locally then try installing it
# Also let me know if there's a chmod type issue. Like you see an error saying access to a certain directory has been denied
```