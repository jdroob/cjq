from llvmlite import ir, binding

def create_skeleton():
    # Create an LLVM module
    module = ir.Module()
    module.triple = binding.get_process_triple()

    # Define the main function
    main_func_type = ir.FunctionType(ir.VoidType(), [ir.IntType(8).as_pointer()])
    main_func = ir.Function(module, main_func_type, "jqfunc")
    main_block = main_func.append_basic_block("entry")
    builder = ir.IRBuilder(main_block)

    # Read JSON data into llvm module
    json_data, = main_func.args

    # Declare _print_str - this links this identifier to C function
    _print_str = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [json_data.type]),
                            name='_print_str')

    # Call _print_str
    # builder.call(_print_str, [json_data]) # DEBUG

    # Return from main function
    builder.ret_void()

    return module, builder


def parse(bytecode):
    """
    Parses JQ bytecode into intermediate representation (LLVM IR).
    """
    # Generate main module skeleton
    module, builder = create_skeleton()
    
    # Build out rest of module according to bytecode instructions
    for instr in bytecode:
        match instr.command:
            case 'TOP':
                pass
                # print('TOP actions')   # Debug
                #TODO - just return main module (no-op) skeleton
            case _:
                pass
                # print('Error: Unable to match to a bytecode instruction')     # Debug

    return module