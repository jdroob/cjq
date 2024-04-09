# TODO: Expand on this, extensively
The goal of this project is to build a compiled implementation of the [JQ programming language]([url](https://github.com/jqlang/jq)https://github.com/jqlang/jq).

This is an [llvmlite-based]([url](https://llvmlite.readthedocs.io/en/latest/index.html)) project.

**To run:** `./cjq.sh <program-type-flag> <jq-program> <optional-output-flag-and-output-executable-name> <one-or-more-JSON-files>`

## Notes:
- **program-type-flag**: 
  - *-s* to pass program as a raw string
  - *-f* to pass a file (e.g. path/to/program.jq)
- **output-flag**
  - *-o* to pass the name of the output executable to be generated
  - If no output flag is passed, the default executable name is **jqprgm**

- The above steps assume a stable installation of clang.

## Example: JQ Program as raw string
``./cjq.sh -s '.[0] + .[1]' -o sum prod1.json prod2.json``

## Example: JQ Program as file
``./cjq.sh -f sum.jq -o sum prod1.json prod2.json``
