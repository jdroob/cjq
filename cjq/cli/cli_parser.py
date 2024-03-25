import sys

def parse_bytecode(stdout):
    """
    Parses the provided stdout string containing bytecode commands.

    The function extracts the bytecode commands and constructs a list of dictionaries,
    where each dictionary represents an opcode-command pair.

    Args:
        stdout (str): The stdout string containing bytecode commands.

    Returns:
        bytecode (list): A list of dictionaries representing the bytecode commands.
    """
    bytecode = []
    lines = stdout.strip().split('\n')
    for line in lines:
        if line.strip().startswith('{'):
            break  # Stop parsing when encountering the start of JSON
        parts = line.split(' ', 1)
        if len(parts) == 2:
            opcode, command = parts
            bytecode.append({opcode: command.strip()})
    return bytecode

def parse_command(argv):
    """
    Parses the command line input provided by the user.

    The command is expected to be of the form:
    ```
    python <path-to-main>/main.py '<string with jq commands>' <path-to-one-or-more-json-files>.json
    ```
    and it parses the command such that it appears as:
    ```
    ./runtime/jq/jq -s '<string with jq commands>' <path-to-one-or-more-json-files>.json --debug-dump-disasm
    ```

    Args:
        argv (list): List of command line arguments.

    Returns:
        command (str): The parsed command.
    """
    if len(argv) < 4:
        print("Usage: python script.py '<string with jq commands>' <path-to-one-or-more-json-files>.json")
        sys.exit(1)

    jq_commands = argv[1]
    json_files = argv[2:]
    
    jq_path = "./runtime/jq/jq"
    debug_dump_disasm = "--debug-dump-disasm"

    command = f"{jq_path} -s '{jq_commands}' {' '.join(json_files)} {debug_dump_disasm}"
    return command