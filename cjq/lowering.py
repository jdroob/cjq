from llvmlite import ir, binding

def create_skeleton():
    """
    Creates an LLVM module with a main function that accepts a pointer to JSON data.

    Returns:
        A tuple containing the LLVM module and an IRBuilder instance.

    Example:
        # Create a skeleton LLVM module
        module, builder = create_skeleton()

        # Now you can add more LLVM IR code using the provided IRBuilder instance.
    """
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
    builder.call(_print_str, [json_data]) # DEBUG

    # Return from main function
    builder.ret_void()

    return module, builder


def lower(bytecode):
    """
    Lowers JQ bytecode to LLVM IR.
    """
    # Generate main module skeleton
    module, builder = create_skeleton()
    
    # Build out rest of module according to bytecode instructions
    for instr in bytecode:
        match instr.command:
            case 'TOP':
                pass    # This is actually all you need to do for this instruction
            case _:
                pass

    return module