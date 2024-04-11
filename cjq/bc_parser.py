from llvmlite import ir, binding

def create_skeleton():
    # Create an LLVM module
    module = ir.Module()
    module.triple = binding.get_process_triple()

    # Declare a global pointer variable
    # data_ptr = ir.GlobalVariable(module, ir.PointerType(ir.IntType(8)), 'data')
    x_var = ir.GlobalVariable(module, ir.IntType(8), 'x')
    x_var.initializer = ir.Constant(ir.IntType(8), 65)
    data = ir.GlobalVariable(module, ir.IntType(8).as_pointer(), 'data')
    data.initializer = x_var        # This actually works! - Now how do we print this from llvm?

    # Define the main function
    main_func_type = ir.FunctionType(ir.VoidType(), [ir.IntType(8).as_pointer()])
    main_func = ir.Function(module, main_func_type, "jqfunc")
    main_block = main_func.append_basic_block("entry")
    builder = ir.IRBuilder(main_block)

    arg, = main_func.args
    # argLoaded = builder.load(arg, "argLoaded")

    # Define the constant array
    # c_str_val = ir.Constant(ir.ArrayType(ir.IntType(8), 4), bytearray(b"foo\00"))
    # c_str_val_ptr = builder.alloca(ir.ArrayType(ir.IntType(8), 4))
    # print(c_str_val_ptr)
    # Store the constant array into the memory location pointed to by the global pointer
    # builder.store(c_str_val, c_str_val_ptr)
    # dptr = builder.gep(data, [ir.Constant(ir.IntType(32), 0)])
    # dc = builder.load(data, "data_contents")
    # builder.store(c_str_val_ptr, data_ptr)
    # Print the first element of the stored array (optional)
    # c_str_first_element_ptr = builder.gep(c_str_val_ptr, [ir.Constant(ir.IntType(32), 0), ir.Constant(ir.IntType(32), 0)])
    # c_str_first_element = builder.load(c_str_first_element_ptr)


    # Declare _print_str - this links this identifier to C function
    _print_str = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [arg.type]),
                            name='_print_str')
    # Declare _print_str - this links this identifier to C function
    # _print_int = ir.Function(module, 
    #                  ir.FunctionType(ir.VoidType(), [ir.IntType(8)]), 
    #                  name='_print_int')

    builder.call(_print_str, [arg])
    # builder.call(_print_int, [dc])
    # Update x
    builder.store(ir.Constant(ir.IntType(8), 80), x_var)
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