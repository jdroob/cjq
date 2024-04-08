# TODO: Expand on this, extensively
The goal of this project is to build a compiled implementation of the [JQ programming language]([url](https://github.com/jqlang/jq)https://github.com/jqlang/jq).

This is an [llvmlite-based]([url](https://llvmlite.readthedocs.io/en/latest/index.html)) project.

**To run:** 
1. `python path/to/main.py <program-type-flag> <jq-program> <one-or-more-JSON-files> > <output-file-name>.ll`
2. `clang path/to/test.c <output-file-name>.ll`
3. `./a.out`

## Notes:
- **program-type-flag**: 
  - *-s* to pass program as a raw string
  - *-f* to pass a file (e.g. path/to/program.jq)

- The above steps assume a proper installation of clang.
