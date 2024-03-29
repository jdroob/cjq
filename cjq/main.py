import sys

import bc_parser
from cli.cli_run import execute_command
from cli.cli_parser import parse_command


def main():
  """
  Entry point to the CJQ compiler.

  This function serves as the entry point to the CJQ compiler. It parses the command line
  arguments, executes the constructed command, and parses the resulting bytecode.

  TODO: Update this doc string as more functionality is added.
  """
  command = parse_command(sys.argv)
  bytecode = execute_command(command) #; print(bytecode)
  bc_parser.parse(bytecode)

if __name__ == "__main__":
  main()