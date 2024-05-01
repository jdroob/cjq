from llvmlite import binding as llvm
from llvmlite import ir
from ctypes import *
import sys


def jq_lower(ocodes_ptr, cjq_state_ptr):
    """
    Uses llvmlite C-binding feature to call cjq_execute from llvmlite.
    """
    print("Made it to jq_lower")
    print(f"ocodes_ptr passed to jq_lower: {hex(id(ocodes_ptr))}")
    print(f"cjq_state_ptr passed to jq_lower: {hex(id(cjq_state_ptr))}")
    # Need this to call C functions from Python
    so_file = "/home/rubio/cjq/jq_util.so"
    
    jq_util_funcs = CDLL(so_file)
   
    # Create LLVM module
    module = ir.Module()
    # Set the data layout
    #TODO: Find better way than hard-coding
    data_layout_str = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
    module.data_layout = llvm.create_target_data(data_layout_str)
    module.triple = llvm.get_process_triple()
    
    # Create a void pointer type
    void_ptr_type = ir.IntType(8).as_pointer()
    
    # Define entry point
    main_func_type = ir.FunctionType(ir.VoidType(), [void_ptr_type])
    main_func = ir.Function(module, main_func_type, "jq_program")
    main_block = main_func.append_basic_block("entry")
    builder = ir.IRBuilder(main_block)
    
    # Get jq_program arg
    _cjq_state_ptr, = main_func.args
    
    # Define opcode-functions
    _opcode_TOP = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_TOP')
    
    _opcode_BACKTRACK_TOP = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_BACKTRACK_TOP')
    
    _opcode_SUBEXP_BEGIN = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_SUBEXP_BEGIN')
    
    _opcode_BACKTRACK_SUBEXP_BEGIN = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_BACKTRACK_SUBEXP_BEGIN')
    
    _opcode_PUSHK_UNDER = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_PUSHK_UNDER')
    
    _opcode_BACKTRACK_PUSHK_UNDER = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_BACKTRACK_PUSHK_UNDER')
    
    _opcode_INDEX = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_INDEX')
    
    _opcode_BACKTRACK_INDEX = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_BACKTRACK_INDEX')
    
    _opcode_SUBEXP_END = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_SUBEXP_END')
    
    _opcode_BACKTRACK_SUBEXP_END = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_BACKTRACK_SUBEXP_END')
    
    _opcode_CALL_BUILTIN_plus = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_CALL_BUILTIN_plus')
    
    _opcode_BACKTRACK_CALL_BUILTIN_plus = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_BACKTRACK_CALL_BUILTIN_plus')
    
    _opcode_RET = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_RET')
    
    _opcode_BACKTRACK_RET = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_BACKTRACK_RET')
    
    # Define the argument types for the C function
    jq_util_funcs._get_num_opcodes.argtypes = []
    jq_util_funcs._get_num_opcodes.restype = c_int
    print("Calling _get_num_opcodes")
    num_opcodes = jq_util_funcs._get_num_opcodes()
    print(f"This is the num_opcodes we got from lowering.py: {num_opcodes}")
    
    # Define the argument types for the C function
    jq_util_funcs._get_opcode_list_len.argtypes = [c_void_p]
    jq_util_funcs._get_opcode_list_len.restype = c_int
    # Get opcode_list length from cjq_state
    opcode_list_len = jq_util_funcs._get_opcode_list_len(ocodes_ptr)
    print("Made it to back to jq_lower")
    print(f"This is the opcode list length we got from lowering.py: {opcode_list_len}")
    # Get all opcodes from opcode_list
    jq_util_funcs._opcode_list_at.argtypes = [c_void_p, c_int]
    jq_util_funcs._opcode_list_at.restype = c_uint8
    for i in range(opcode_list_len):
        curr_opcode = jq_util_funcs._opcode_list_at(ocodes_ptr, i)
        match curr_opcode:
            case 35:
                builder.comment("Placeholder for call to TOP opcode-function")
                builder.call(_opcode_TOP, [_cjq_state_ptr])
            case 23:
                builder.comment("Placeholder for call to SUBEXP_BEGIN opcode-function")
                builder.call(_opcode_SUBEXP_BEGIN, [_cjq_state_ptr])
            case 4:
                builder.comment("Placeholder for call to PUSHK_UNDER opcode-function")
                builder.call(_opcode_PUSHK_UNDER, [_cjq_state_ptr])
            case 24:
                builder.comment("Placeholder for call to SUBEXP_END opcode-function")
                builder.call(_opcode_SUBEXP_END, [_cjq_state_ptr])
            case 10:
                builder.comment("Placeholder for call to INDEX opcode-function")
                builder.call(_opcode_INDEX, [_cjq_state_ptr])
            case 27:
                builder.comment("Placeholder for call to CALL_BUILTIN_plus opcode-function")
                builder.call(_opcode_CALL_BUILTIN_plus, [_cjq_state_ptr])
            case 29:
                builder.comment("Placeholder for call to RET opcode-function")
                builder.call(_opcode_RET, [_cjq_state_ptr])
            case _:
                print(curr_opcode)
                print(curr_opcode+num_opcodes)
                backtracking_opcodes = [35+num_opcodes, 23+num_opcodes, 4+num_opcodes, 10+num_opcodes, 24+num_opcodes, 27+num_opcodes, 29+num_opcodes]
                if curr_opcode in backtracking_opcodes:
                    if curr_opcode == backtracking_opcodes[0]:
                        builder.call(_opcode_BACKTRACK_TOP, [_cjq_state_ptr])
                        builder.comment("Placeholder for call to _opcode_BACKTRACK_TOP opcode-function")
                    elif curr_opcode == backtracking_opcodes[1]:
                        builder.call(_opcode_BACKTRACK_SUBEXP_BEGIN, [_cjq_state_ptr])
                        builder.comment("Placeholder for call to _opcode_BACKTRACK_SUBEXP_BEGIN opcode-function")
                    elif curr_opcode == backtracking_opcodes[2]:
                        builder.call(_opcode_BACKTRACK_PUSHK_UNDER, [_cjq_state_ptr])
                        builder.comment("Placeholder for call to _opcode_BACKTRACK_PUSHK_UNDER opcode-function")
                    elif curr_opcode == backtracking_opcodes[3]:
                        builder.call(_opcode_BACKTRACK_INDEX, [_cjq_state_ptr])
                        builder.comment("Placeholder for call to _opcode_BACKTRACK_INDEX opcode-function")
                    elif curr_opcode == backtracking_opcodes[4]:
                        builder.call(_opcode_BACKTRACK_SUBEXP_END, [_cjq_state_ptr])
                        builder.comment("Placeholder for call to _opcode_BACKTRACK_SUBEXP_END opcode-function")
                    elif curr_opcode == backtracking_opcodes[5]:
                        builder.call(_opcode_BACKTRACK_CALL_BUILTIN_plus, [_cjq_state_ptr])
                        builder.comment("Placeholder for call to _opcode_BACKTRACK_CALL_BUILTIN_plus opcode-function")
                    elif curr_opcode == backtracking_opcodes[6]:
                        builder.call(_opcode_BACKTRACK_RET, [_cjq_state_ptr])
                        builder.comment("Placeholder for call to _opcode_BACKTRACK_RET opcode-function")
                    else:
                        raise ValueError(f"Current backtracking opcode: {curr_opcode} does not match any existing opcodes")
                else:
                    raise ValueError(f"Current opcode: {curr_opcode} does not match any existing opcodes")
 
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
    
def generate_llvm_ir(ocodes_ptr, cjq_state_ptr):
    try:
        print("Made it to generate_llvm_ir")
        print(f"opcodes_ptr passed to generate_llvm_ir: {hex(id(ocodes_ptr))}")
        print(f"cjq_state_ptr passed to generate_llvm_ir: {hex(id(cjq_state_ptr))}")
        llvm_ir = jq_lower(ocodes_ptr, cjq_state_ptr)
        mod = llvm.parse_assembly(str(llvm_ir))
        mod.verify()
        print(mod)
        
        # Write LLVM IR to file
        with open("ir.ll", "w") as file:
            file.write(str(mod))
            
    except Exception as e:
        print("Error generating LLVM IR:", e)
        exit(1)

if __name__ == "__main__":
    generate_llvm_ir()