from llvmlite import ir, binding

def create_skeleton():
    """
    Create the LLVM IR entry point.
    """
    # Create an LLVM module
    module = ir.Module()
    module.triple = binding.get_process_triple()

    # Define the main function
    main_func_type = ir.FunctionType(ir.IntType(32), [])
    main_func = ir.Function(module, main_func_type, "test")

    # Define the main function body
    main_block = main_func.append_basic_block("entry")
    builder = ir.IRBuilder(main_block)

    # Return gracefully
    builder.ret(ir.Constant(ir.IntType(32), 0))  

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
                # print('do some LLVM stuff')
                #TODO - just return main module (no-op) skeleton
            case _:
                pass
                # print('some other stuff')

    return module