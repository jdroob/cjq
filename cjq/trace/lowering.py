from llvmlite import binding as llvm
from llvmlite import ir
from ctypes import *
from enum import Enum
import os

class Opcode(Enum):
    LOADK=0
    DUP=1
    DUPN=2
    DUP2=3
    PUSHK_UNDER=4
    POP=5
    LOADV=6
    LOADVN=7
    STOREV=8
    STORE_GLOBAL=9
    INDEX=10
    INDEX_OPT=11
    EACH=12
    EACH_OPT=13
    FORK=14
    TRY_BEGIN=15
    TRY_END=16
    JUMP=17
    JUMP_F=18
    BACKTRACK=19
    APPEND=20
    INSERT=21
    RANGE=22
    SUBEXP_BEGIN=23
    SUBEXP_END=24
    PATH_BEGIN=25
    PATH_END=26
    CALL_BUILTIN=27
    CALL_JQ=28
    RET=29
    TAIL_CALL_JQ=30
    CLOSURE_PARAM=31
    CLOSURE_REF=32
    CLOSURE_CREATE=33
    CLOSURE_CREATE_C=34
    TOP=35
    CLOSURE_PARAM_REGULAR=36
    DEPS=37
    MODULEMETA=38
    GENLABEL=39
    DESTRUCTURE_ALT=40
    STOREVN=41
    ERRORK=42


def jq_lower(ocodes_ptr, cjq_state_ptr):
    """
    Uses llvmlite C-binding feature to call cjq_execute from llvmlite.
    """
    
    # Get the home directory
    home_dir = os.path.expanduser("~")
    so_file = os.path.join(home_dir, "cjq/jq_util.so")
    
    # Need this to call C functions from Python
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
    _init_stack = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_init_stack')
    
    _opcode_LOADK = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_LOADK')
    
    _opcode_DUP = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_DUP')
    
    _opcode_DUPN = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_DUPN')
    
    _opcode_DUP2 = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_DUP2')
    
    _opcode_PUSHK_UNDER = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_PUSHK_UNDER')
    
    _opcode_POP = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_POP')
    
    _opcode_LOADV = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_LOADV')
    
    _opcode_LOADVN = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_LOADVN')
    
    _opcode_STOREV = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_STOREV')
    
    _opcode_STORE_GLOBAL = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_STORE_GLOBAL')
    
    _opcode_INDEX = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_INDEX')
    
    _opcode_INDEX_OPT = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_INDEX_OPT')
    
    _opcode_EACH = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_EACH')
    
    _opcode_BACKTRACK_EACH = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_BACKTRACK_EACH')
    
    _opcode_EACH_OPT = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_EACH_OPT')
    
    _opcode_BACKTRACK_EACH_OPT = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_BACKTRACK_EACH_OPT')
    
    _opcode_FORK = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_FORK')
    
    _opcode_BACKTRACK_FORK = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_BACKTRACK_FORK')
    
    _opcode_TRY_BEGIN = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_TRY_BEGIN')
    
    _opcode_BACKTRACK_TRY_BEGIN = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_BACKTRACK_TRY_BEGIN')
    
    _opcode_TRY_END = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_TRY_END')
    
    _opcode_BACKTRACK_TRY_END = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_BACKTRACK_TRY_END')
    
    _opcode_JUMP = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_JUMP')
    
    _opcode_JUMP_F = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_JUMP_F')
    
    _opcode_BACKTRACK = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_BACKTRACK')
    
    _opcode_APPEND = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_APPEND')
    
    _opcode_INSERT = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_INSERT')
    
    _opcode_RANGE = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_RANGE')
    
    _opcode_BACKTRACK_RANGE = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_BACKTRACK_RANGE')
    
    _opcode_SUBEXP_BEGIN = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_SUBEXP_BEGIN')
    
    _opcode_SUBEXP_END = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_SUBEXP_END')
    
    _opcode_PATH_BEGIN = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_PATH_BEGIN')
    
    _opcode_BACKTRACK_PATH_BEGIN = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_BACKTRACK_PATH_BEGIN')
    
    _opcode_PATH_END = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_PATH_END')
    
    _opcode_BACKTRACK_PATH_END = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_BACKTRACK_PATH_END')
    
    _opcode_CALL_BUILTIN = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_CALL_BUILTIN')
    
    _opcode_CALL_JQ = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_CALL_JQ')
    
    _opcode_RET = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_RET')
    
    _opcode_BACKTRACK_RET = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_BACKTRACK_RET')
    
    _opcode_TAIL_CALL_JQ = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_TAIL_CALL_JQ')
    
    _opcode_CLOSURE_PARAM = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_CLOSURE_PARAM')
    
    _opcode_CLOSURE_REF = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_CLOSURE_REF')
    
    _opcode_CLOSURE_CREATE = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_CLOSURE_CREATE')
    
    _opcode_CLOSURE_CREATE_C = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_CLOSURE_CREATE_C')
  
    _opcode_TOP = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_TOP')
    
    _opcode_CLOSURE_PARAM_REGULAR = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_CLOSURE_PARAM_REGULAR')
    
    _opcode_DEPS = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_DEPS')
    
    _opcode_MODULEMETA = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_MODULEMETA')
    
    _opcode_GENLABEL = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_GENLABEL')
    
    _opcode_DESTRUCTURE_ALT = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_DESTRUCTURE_ALT')
    
    _opcode_BACKTRACK_DESTRUCTURE_ALT = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_BACKTRACK_DESTRUCTURE_ALT')
    
    _opcode_STOREVN = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_STOREVN')
    
    _opcode_BACKTRACK_STOREVN = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_BACKTRACK_STOREVN')
    
    _opcode_ERRORK = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_ERRORK')
    
    
    # Initialize jq_state stack
    builder.call(_init_stack, [_cjq_state_ptr])
    
    # Define argument types for C function
    jq_util_funcs._get_num_opcodes.argtypes = []
    jq_util_funcs._get_num_opcodes.restype = c_int
    
    # Get value of NUM_OPCODES
    num_opcodes = jq_util_funcs._get_num_opcodes()
    
    # Define argument types for C function
    jq_util_funcs._get_opcode_list_len.argtypes = [c_void_p]
    jq_util_funcs._get_opcode_list_len.restype = c_int
    
    # Get opcode_list length from cjq_state
    opcode_list_len = jq_util_funcs._get_opcode_list_len(ocodes_ptr)

    # Get all opcodes from opcode_list
    jq_util_funcs._opcode_list_at.argtypes = [c_void_p, c_int]
    jq_util_funcs._opcode_list_at.restype = c_uint8
    for i in range(opcode_list_len):
        curr_opcode = jq_util_funcs._opcode_list_at(ocodes_ptr, i)
        match curr_opcode:
            case Opcode.LOADK.value:
                builder.call(_opcode_LOADK, [_cjq_state_ptr])
            case Opcode.DUP.value:
                builder.call(_opcode_DUP, [_cjq_state_ptr])
            case Opcode.DUPN.value:
                builder.call(_opcode_DUPN, [_cjq_state_ptr])
            case Opcode.DUP2.value:
                builder.call(_opcode_DUP2, [_cjq_state_ptr])
            case Opcode.PUSHK_UNDER.value:
                builder.call(_opcode_PUSHK_UNDER, [_cjq_state_ptr])
            case Opcode.POP.value:
                builder.call(_opcode_POP, [_cjq_state_ptr])
            case Opcode.LOADV.value:
                builder.call(_opcode_LOADV, [_cjq_state_ptr])
            case Opcode.LOADVN.value:
                builder.call(_opcode_LOADVN, [_cjq_state_ptr])
            case Opcode.STOREV.value:
                builder.call(_opcode_STOREV, [_cjq_state_ptr])
            case Opcode.STORE_GLOBAL.value:
                builder.call(_opcode_STORE_GLOBAL, [_cjq_state_ptr])
            case Opcode.INDEX.value:
                builder.call(_opcode_INDEX, [_cjq_state_ptr])
            case Opcode.INDEX_OPT.value:
                builder.call(_opcode_INDEX_OPT, [_cjq_state_ptr])
            case Opcode.EACH.value:
                builder.call(_opcode_EACH, [_cjq_state_ptr])
            case Opcode.EACH_OPT.value:
                builder.call(_opcode_EACH_OPT, [_cjq_state_ptr])
            case Opcode.FORK.value:
                builder.call(_opcode_FORK, [_cjq_state_ptr])
            case Opcode.TRY_BEGIN.value:
                builder.call(_opcode_TRY_BEGIN, [_cjq_state_ptr])
            case Opcode.TRY_END.value:
                builder.call(_opcode_TRY_END, [_cjq_state_ptr])
            case Opcode.JUMP.value:
                builder.call(_opcode_JUMP, [_cjq_state_ptr])
            case Opcode.JUMP_F.value:
                builder.call(_opcode_JUMP_F, [_cjq_state_ptr])
            case Opcode.BACKTRACK.value:
                builder.call(_opcode_BACKTRACK, [_cjq_state_ptr])
            case Opcode.APPEND.value:
                builder.call(_opcode_APPEND, [_cjq_state_ptr])
            case Opcode.INSERT.value:
                builder.call(_opcode_INSERT, [_cjq_state_ptr])
            case Opcode.RANGE.value:
                builder.call(_opcode_RANGE, [_cjq_state_ptr])
            case Opcode.SUBEXP_BEGIN.value:
                builder.call(_opcode_SUBEXP_BEGIN, [_cjq_state_ptr])
            case Opcode.SUBEXP_END.value:
                builder.call(_opcode_SUBEXP_END, [_cjq_state_ptr])
            case Opcode.PATH_BEGIN.value:
                builder.call(_opcode_PATH_BEGIN, [_cjq_state_ptr])
            case Opcode.PATH_END.value:
                builder.call(_opcode_PATH_END, [_cjq_state_ptr])
            case Opcode.CALL_BUILTIN.value:
                builder.call(_opcode_CALL_BUILTIN, [_cjq_state_ptr])
            case Opcode.CALL_JQ.value:
                builder.call(_opcode_CALL_JQ, [_cjq_state_ptr])
            case Opcode.RET.value:
                builder.call(_opcode_RET, [_cjq_state_ptr])
            case Opcode.TAIL_CALL_JQ.value:
                builder.call(_opcode_TAIL_CALL_JQ, [_cjq_state_ptr])
            case Opcode.CLOSURE_PARAM.value:
                builder.call(_opcode_CLOSURE_PARAM, [_cjq_state_ptr])
            case Opcode.CLOSURE_REF.value:
                builder.call(_opcode_CLOSURE_REF, [_cjq_state_ptr])
            case Opcode.CLOSURE_CREATE.value:
                builder.call(_opcode_CLOSURE_CREATE, [_cjq_state_ptr])
            case Opcode.CLOSURE_CREATE_C.value:
                builder.call(_opcode_CLOSURE_CREATE_C, [_cjq_state_ptr])
            case Opcode.TOP.value:
                builder.call(_opcode_TOP, [_cjq_state_ptr])
            case Opcode.CLOSURE_PARAM_REGULAR.value:
                builder.call(_opcode_CLOSURE_PARAM_REGULAR, [_cjq_state_ptr])
            case Opcode.DEPS.value:
                builder.call(_opcode_DEPS, [_cjq_state_ptr])
            case Opcode.MODULEMETA.value:
                builder.call(_opcode_MODULEMETA, [_cjq_state_ptr])
            case Opcode.GENLABEL.value:
                builder.call(_opcode_GENLABEL, [_cjq_state_ptr])
            case Opcode.DESTRUCTURE_ALT.value:
                builder.call(_opcode_DESTRUCTURE_ALT, [_cjq_state_ptr])
            case Opcode.STOREVN.value:
                builder.call(_opcode_STOREVN, [_cjq_state_ptr])
            case Opcode.ERRORK.value:
                builder.call(_opcode_ERRORK, [_cjq_state_ptr])
            case _:
                backtracking_opcodes = (Opcode.RANGE.value, Opcode.STOREVN.value, Opcode.PATH_BEGIN.value, Opcode.PATH_END.value, Opcode.EACH.value, Opcode.EACH_OPT.value, Opcode.TRY_BEGIN.value, Opcode.TRY_END.value, Opcode.DESTRUCTURE_ALT.value, Opcode.FORK.value, Opcode.RET.value)
                backtracking_opcodes = tuple([opcode_val + num_opcodes for opcode_val in backtracking_opcodes])
                backtracking_opcode_funcs = (_opcode_BACKTRACK_RANGE, _opcode_BACKTRACK_STOREVN, _opcode_BACKTRACK_PATH_BEGIN, _opcode_BACKTRACK_PATH_END, _opcode_BACKTRACK_EACH, _opcode_BACKTRACK_EACH_OPT, _opcode_BACKTRACK_TRY_BEGIN, _opcode_BACKTRACK_TRY_END, _opcode_BACKTRACK_DESTRUCTURE_ALT, _opcode_BACKTRACK_FORK, _opcode_BACKTRACK_RET)
                if curr_opcode in backtracking_opcodes:
                    opcode_idx = backtracking_opcodes.index(curr_opcode)
                    builder.call(backtracking_opcode_funcs[opcode_idx], [_cjq_state_ptr])
                else:
                    raise ValueError(f"Current opcode: {curr_opcode} does not match any existing opcodes")
      
    # Return from main
    builder.ret_void()
    
    return module
    
def generate_llvm_ir(ocodes_ptr, cjq_state_ptr):
    try:
        llvm_ir = jq_lower(ocodes_ptr, cjq_state_ptr)
        mod = llvm.parse_assembly(str(llvm_ir))
        mod.verify()
        
        # Write LLVM IR to file
        with open("ir.ll", "w") as file:
            file.write(str(mod))
            
    except Exception as e:
        print("Error generating LLVM IR:", e)
        exit(1)

if __name__ == "__main__":
    generate_llvm_ir()