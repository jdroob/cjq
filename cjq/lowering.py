from llvmlite import binding as llvm
from llvmlite import ir
from ctypes import *
import sys


def jq_lower(cjq_state_ptr):
    """
    Uses llvmlite C-binding feature to call cjq_execute from llvmlite.
    """
    print("Made it to jq_lower")
    print(f"cjq_state_ptr passed to jq_lower: {hex(id(cjq_state_ptr))}")
    # Need this to call C functions from Python
    so_file = "/home/rubio/cjq/jq_util.so"
    
    jq_util_funcs = CDLL(so_file)
   
    # Create LLVM module
    module = ir.Module()
    module.triple = llvm.get_process_triple()
    
    # Define entry point
    main_func_type = ir.FunctionType(ir.VoidType(), [])
    main_func = ir.Function(module, main_func_type, "jq_program")
    main_block = main_func.append_basic_block("entry")
    builder = ir.IRBuilder(main_block)
    
    # Define the argument types for the C function
    jq_util_funcs._get_opcode.argtypes = [c_void_p]
    jq_util_funcs._get_opcode.restype = c_uint16  # Assuming _get_opcode returns uint16_t
    # Get current opcode from cjq_state
    opcode = jq_util_funcs._get_opcode(cjq_state_ptr)
    print("Made it to back to jq_lower")
    print(f"This is the opcode we got from lowering.py: {opcode}")
    
    # Declare _cjq_execute - this links this identifier to C function
    _cjq_execute = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), []),
                            name='_cjq_execute')
    
    # # Declare _get_opcode - this links this identifier to C function
    # _get_opcode = ir.Function(module,
    #                         ir.FunctionType(ir.IntType(16), []),
    #                         name='_get_opcode')
    
    # # For debugging - allows us to see 16-bit values returned from _get_opcode
    # _print_uint16_t = ir.Function(module,
    #                               ir.FunctionType(ir.VoidType(), [ir.IntType(16)]),
    #                               name='_print_uint16_t')
    
    # Call _get_opcode
    # opcode = builder.call(_get_opcode, []) 
    
    # # Debug - print opcode
    # builder.call(_print_uint16_t, [opcode])
    
    # Call _cjq_execute
    builder.call(_cjq_execute, []) 
    
    # Return from main
    builder.ret_void()
    
    return module
    
def generate_llvm_ir(cjq_state_ptr):
    try:
        print("Made it to generate_llvm_ir")
        print(f"cjq_state_ptr passed to generate_llvm_ir: {hex(id(cjq_state_ptr))}")
        llvm_ir = jq_lower(cjq_state_ptr)
        mod = llvm.parse_assembly(str(llvm_ir))
        mod.verify()
        print(mod)
    except Exception as e:
        print("Error generating LLVM IR:", e)
        exit(1)

if __name__ == "__main__":
    generate_llvm_ir()