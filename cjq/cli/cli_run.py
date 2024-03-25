import subprocess

from cli.cli_parser import parse_bytecode

def execute_command(command):
    """
    Executes the command constructed by the `parse_command` function.

    If the constructed command encounters an error during execution, the error message
    is printed to stderr.

    Otherwise, assuming no exceptions occur during execution, the raw stdout string
    produced by the command is returned.

    Args:
        command (str): The command to be executed.

    Returns:
        result.stdout (str): The raw stdout string produced by the executed command.
    """
    try:
        result = subprocess.run(command, shell=True, capture_output=True, text=True)
        if result.returncode == 0:
            # print(parse_bytecode(result.stdout))
            return parse_bytecode(result.stdout)
        else:
            print("Error executing command.")
            print("Error message:", result.stderr)
    except Exception as e:
        print("Exception occurred:", str(e))
