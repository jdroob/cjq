from llvmlite import binding as llvm
from llvmlite import ir
import ctypes

# def create_skeleton():
#     """
#     Creates an LLVM module with a main function that accepts a pointer to JSON data.

#     Returns:
#         A tuple containing the LLVM module and an IRBuilder instance.

#     Example:
#         # Create a skeleton LLVM module
#         module, builder = create_skeleton()

#         # Now you can add more LLVM IR code using the provided IRBuilder instance.
#     """
#     # Create an LLVM module
#     module = ir.Module()
#     module.triple = binding.get_process_triple()

#     # Define the main function
#     main_func_type = ir.FunctionType(ir.VoidType(), [ir.IntType(8).as_pointer()])
#     main_func = ir.Function(module, main_func_type, "jqfunc")
#     main_block = main_func.append_basic_block("entry")
#     builder = ir.IRBuilder(main_block)

#     # Read JSON data into llvm module
#     json_data, = main_func.args

#     # Declare _print_str - this links this identifier to C function
#     _print_str = ir.Function(module,
#                             ir.FunctionType(ir.VoidType(), [json_data.type]),
#                             name='_print_str')

#     # Call _print_str
#     builder.call(_print_str, [json_data]) # DEBUG

#     # Return from main function
#     builder.ret_void()

#     return module, builder

# Load the shared library containing the C function
# get_opcode = ctypes.CDLL("./lowering.so")  # Replace with the actual path to the shared library

# # Define the function prototype to match the C function
# get_opcode.call_c_function.argtypes = [ctypes.py_object, ctypes.POINTER(ctypes.c_uint16)]

def jq_lower():
    """
    Uses llvmlite C-binding feature to call C functions from llvmlite.
    Specifically, 
    """
    # print("Calling cjq.lowering.jq_lower() function from cjq/main.c")
   
    # Create an LLVM module
    module = ir.Module()
    module.triple = llvm.get_process_triple()
    
    # Define the main function
    main_func_type = ir.FunctionType(ir.VoidType(), [])
    main_func = ir.Function(module, main_func_type, "jq_program")
    main_block = main_func.append_basic_block("entry")
    builder = ir.IRBuilder(main_block)
    
    # Declare _test_execute - this links this identifier to C function
    _test_execute = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), []),
                            name='_test_execute')
    
    # Call _test_execute
    builder.call(_test_execute, []) 
    
    # Return from main function
    builder.ret_void()
    
    return module
    
    # # Access jq and PC members   # TODO: Call C function that will cast this back to a uint16_t* type
    # print("Calling the C function get_opcode()")
    # # jq_lowering = ctypes.CDLL("/home/rubio/cjq/cjq/jq_lowering.so")
    # # jq_lowering.get_opcode(jq, pc)
    # # jq_lowering.display()
    # # jq_lowering.call_c_function(jq)
    # print("Returning from get_opcode()")
    
def generate_llvm_ir():
    try:
        llvm_ir = jq_lower()
        mod = llvm.parse_assembly(str(llvm_ir))
        mod.verify()
        print(mod)
    except Exception as e:
        print("Error generating LLVM IR:", e)
        exit(1)

if __name__ == "__main__":
    generate_llvm_ir()