import argparse
import llvmlite.binding as llvm

from cli.cli_parser import parse_program
from cli.cli_run import execute_command
from lowering import lower

def parse_arguments():
    parser = argparse.ArgumentParser(description="Compile and execute jq program.")
    parser.add_argument("-f", "--file", help="Path to the jq program file")
    parser.add_argument("-s", "--string", help="jq program as a string")
    parser.add_argument("-o", "--output", help="Output executable name")
    parser.add_argument("input_files", nargs="+", help="Input JSON files")
    return parser.parse_args()

def read_jq_program(file_path):
    try:
        with open(file_path, "r") as file:
            return file.read()
    except FileNotFoundError:
        print("Error: File '{}' not found.".format(file_path))
        exit(1)
    except IOError as e:
        print("Error reading file '{}':".format(file_path), e)
        exit(1)

def generate_llvm_ir(jq_program):
    try:
        command = parse_program(jq_program)
        bytecode = execute_command(command)
        llvm_ir = lower(bytecode)
        return llvm_ir
    except Exception as e:
        print("Error generating LLVM IR:", e)
        exit(1)

def create_execution_engine():
    target = llvm.Target.from_default_triple()
    target_machine = target.create_target_machine()
    backing_mod = llvm.parse_assembly("")
    engine = llvm.create_mcjit_compiler(backing_mod, target_machine)
    return engine

def compile_ir(llvm_ir):
    try:
        mod = llvm.parse_assembly(str(llvm_ir))
        mod.verify()
        return mod
    except Exception as e:
        print("Error compiling LLVM IR:", e)
        exit(1)

def execute_jq_program(engine, func_ptr, mod, input_files):
    from ctypes import CFUNCTYPE, c_int

    if func_ptr is not None:
      # Turn into a Python callable using ctypes
        cfunc = CFUNCTYPE(c_int)(func_ptr)
        res = cfunc()
        return res
    else:
        print("Error: Function pointer is null.")

def print_asm(engine, mod):
    try:
        target_machine = llvm.Target.from_default_triple().create_target_machine()
        engine.finalize_object()
        asm = target_machine.emit_assembly(mod)
        print("Generated Assembly:")
        print(asm)
    except Exception as e:
        print("Error printing assembly:", e)
        exit(1)

def main():
    args = parse_arguments()

    if args.file:
        jq_program = read_jq_program(args.file)
    elif args.string:
        jq_program = args.string
    else:
        print("Error: Either jq program file or program string must be provided.")
        exit(1)

    llvm_ir = generate_llvm_ir(jq_program)
    mod = compile_ir(llvm_ir)
    print(mod)

    ### NOTE:  One feature of LLVM is that it can compile it's own code to 
    ###        executable machine instructions without ever being written to a file or using clang.
    ###        You can do this entirely in Python and have Python call the resulting function.
    ###        The below code runs entirely inside an active Python interpreter process. 
    ###        If you can't get clang to work, you can always use this as a fallback.

    ### For JIT compilation, uncomment from...
    # # HERE ...

    # # All these initializations are required for code generation!
    # llvm.initialize()
    # llvm.initialize_native_target()
    # llvm.initialize_native_asmprinter()  # yes, even this one
    
    # engine = create_execution_engine()
    # engine.add_module(mod)
    # engine.finalize_object()
    # engine.run_static_constructors()
    # # Look up the function pointer (a Python int)
    # func_ptr = engine.get_function_address('test')
    # res = execute_jq_program(engine, func_ptr, mod, args.input_files)
    # print("main(...) =", res)   # Debug
    # print_asm(engine, mod)  # Debug

    # # ... TO HERE


if __name__ == "__main__":
    main()
