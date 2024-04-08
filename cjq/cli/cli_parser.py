import sys
from collections import namedtuple

def parse_bytecode(stdout):
    """
    Parses the provided stdout string containing bytecode commands.

    The function extracts the bytecode commands and constructs a list of dictionaries,
    where each dictionary represents an offset-command pair.

    Args:
        stdout (str): The stdout string containing bytecode commands.

    Returns:
        bytecode (list): A list of dictionaries representing the bytecode commands.
    """
    BC_INSTR = namedtuple('BC_INSTR', ['byte_offset', 'command'])

    bytecode = []
    lines = stdout.strip().split('\n')
    for line in lines:
        if not(line.strip().startswith('0')):
            continue    # Start parsing when encountering first Bytecode instr
        if line.strip().startswith('{'):
            break  # Stop parsing when encountering the start of JSON
        parts = line.split(' ', 1)
        if len(parts) == 2:
            offset, command = parts
            bytecode.append(BC_INSTR(offset, command.strip()))
    return bytecode

def parse_program(jq_program):
    """
    Parses the jq_program input provided by the user.

    The command is expected to be of the form:
    ```
    python path/to/main.py -s '<string with jq commands>' <path-to-one-or-more-json-files>.json
    ```
    and it parses the command such that it appears as:
    ```
    ./runtime/jq/jq -s '<string with jq commands>' --debug-dump-disasm
    ```

    Args:
        jq_program (list): List of command line arguments.

    Returns:
        command (str): The parsed command.
    """
    # if len(argv) < 4:
    #     print("Usage: python script.py '<string with jq commands>' <path-to-one-or-more-json-files>.json")
    #     sys.exit(1)

    # jq_commands = argv[1]
    # json_files = argv[2:]
    
    jq_path = "./runtime/jq/jq"
    debug_dump_disasm = "--debug-dump-disasm"

    command = f"{jq_path} -s '{jq_program}' '' {debug_dump_disasm}"
    return command