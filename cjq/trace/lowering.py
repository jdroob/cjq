from llvmlite import binding as llvm
from llvmlite import ir
from ctypes import *
from enum import Enum
import os

from collections import defaultdict

def max_unique_subseq(sequence):
    # subsequences = defaultdict(int)
    subsequences = set()
    current_subsequence = []

    for call in sequence:
        if not current_subsequence or call.opcode > current_subsequence[-1]:
            current_subsequence.append(call.opcode)
        else:
            if current_subsequence:
                # subsequences[tuple(current_subsequence)] += 1
                subsequences.add(tuple(current_subsequence))
            current_subsequence = [call.opcode]

    if current_subsequence:
        # subsequences[tuple(current_subsequence)] += 1
        subsequences.add(tuple(current_subsequence))

    return sorted(subsequences, key=len, reverse=True)

def generate_subseq_lis(sequence, subseqs):
    gen_lis = []
    i = 0  # Start index
    while i < len(sequence):
        matched = False
        for subseq in subseqs:
            subseq_len = len(subseq)
            if tuple(sequence[i:i + subseq_len]) == subseq:
                gen_lis.append(subseq)
                i += subseq_len  # Move the index forward by the length of the matched subsequence
                matched = True
                break
        if not matched:
            i += 1  # If no subsequence matches, move the index forward by 1
    return gen_lis

def coalesce_tuples(tuples_list):
    if not tuples_list:
        return []

    result = []
    current_tuple = list(tuples_list[0])

    for i in range(1, len(tuples_list)):
        if tuples_list[i] == tuples_list[i - 1]:
            current_tuple.extend(tuples_list[i])
        else:
            result.append(tuple(current_tuple))
            current_tuple = list(tuples_list[i])

    result.append(tuple(current_tuple))
    return result

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
    TOP=35
    GENLABEL=39
    DESTRUCTURE_ALT=40
    STOREVN=41
    ERRORK=42
    
    INIT_JQ_NEXT=97
    GET_NEXT_INPUT=98
    UPDATE_RES_STATE=99
    JQ_HALT=100

class CallType:
    def __init__(self, opcode, backtracking=0):
        self.opcode = opcode
        self.backtracking=backtracking
        
    def __str__(self):
        return f"{self.opcode}"

def jq_lower(opcodes_ptr):
    """
    Uses llvmlite C-binding feature to call opcode functions from llvmlite.
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
    # main_func.attributes.add('alwaysinline')
    main_block = main_func.append_basic_block("entry")
    builder = ir.IRBuilder(main_block)
    
    # Get jq_program arg
    _cjq, = main_func.args
    
    # Define opcode-functions
    _get_next_input = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_get_next_input')
    _get_next_input.attributes.add('alwaysinline')
    _get_next_input.attributes.add('optsize')
    
    _update_result_state = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_update_result_state')
    _update_result_state.attributes.add('alwaysinline')
    _update_result_state.attributes.add('optsize')
    
    _init_jq_next = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_init_jq_next')
    _init_jq_next.attributes.add('alwaysinline')
    _init_jq_next.attributes.add('optsize')
    
    _jq_halt = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_jq_halt')
    _jq_halt.attributes.add('alwaysinline')
    _jq_halt.attributes.add('optsize')
    
    _opcode_LOADK = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_LOADK')
    _opcode_LOADK.attributes.add('alwaysinline')
    _opcode_LOADK.attributes.add('optsize')
    
    _opcode_DUP = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_DUP')
    _opcode_DUP.attributes.add('alwaysinline')
    _opcode_DUP.attributes.add('optsize')
    
    _opcode_DUPN = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_DUPN')
    _opcode_DUPN.attributes.add('alwaysinline')
    _opcode_DUPN.attributes.add('optsize')
    
    _opcode_DUP2 = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_DUP2')
    _opcode_DUP2.attributes.add('alwaysinline')
    _opcode_DUP2.attributes.add('optsize')
    
    _opcode_PUSHK_UNDER = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_PUSHK_UNDER')
    _opcode_PUSHK_UNDER.attributes.add('alwaysinline')
    _opcode_PUSHK_UNDER.attributes.add('optsize')
    
    _opcode_POP = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_POP')
    _opcode_POP.attributes.add('alwaysinline')
    _opcode_POP.attributes.add('optsize')
    
    _opcode_LOADV = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_LOADV')
    _opcode_LOADV.attributes.add('alwaysinline')
    _opcode_LOADV.attributes.add('optsize')
    
    _opcode_LOADVN = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_LOADVN')
    _opcode_LOADVN.attributes.add('alwaysinline')
    _opcode_LOADVN.attributes.add('optsize')
    
    _opcode_STOREV = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_STOREV')
    _opcode_STOREV.attributes.add('alwaysinline')
    _opcode_STOREV.attributes.add('optsize')
    
    _opcode_STORE_GLOBAL = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_STORE_GLOBAL')
    _opcode_STORE_GLOBAL.attributes.add('alwaysinline')
    _opcode_STORE_GLOBAL.attributes.add('optsize')
    
    _opcode_INDEX = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_INDEX')
    _opcode_INDEX.attributes.add('alwaysinline')
    _opcode_INDEX.attributes.add('optsize')

    _opcode_INDEX_OPT = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_INDEX_OPT')
    _opcode_INDEX_OPT.attributes.add('alwaysinline')
    _opcode_INDEX_OPT.attributes.add('optsize')
    
    _opcode_EACH = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_EACH')
    _opcode_EACH.attributes.add('alwaysinline')
    _opcode_EACH.attributes.add('optsize')
    
    _opcode_BACKTRACK_EACH = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_BACKTRACK_EACH')
    _opcode_BACKTRACK_EACH.attributes.add('alwaysinline')
    _opcode_BACKTRACK_EACH.attributes.add('optsize')
    
    _opcode_EACH_OPT = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_EACH_OPT')
    _opcode_EACH_OPT.attributes.add('alwaysinline')
    _opcode_EACH_OPT.attributes.add('optsize')
    
    _opcode_BACKTRACK_EACH_OPT = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_BACKTRACK_EACH_OPT')
    _opcode_BACKTRACK_EACH_OPT.attributes.add('alwaysinline')
    _opcode_BACKTRACK_EACH_OPT.attributes.add('optsize')
    
    _opcode_FORK = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_FORK')
    _opcode_FORK.attributes.add('alwaysinline')
    _opcode_FORK.attributes.add('optsize')
    
    _opcode_BACKTRACK_FORK = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_BACKTRACK_FORK')
    _opcode_BACKTRACK_FORK.attributes.add('alwaysinline')
    _opcode_BACKTRACK_FORK.attributes.add('optsize')
    
    _opcode_TRY_BEGIN = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_TRY_BEGIN')
    _opcode_TRY_BEGIN.attributes.add('alwaysinline')
    _opcode_TRY_BEGIN.attributes.add('optsize')
    
    _opcode_BACKTRACK_TRY_BEGIN = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_BACKTRACK_TRY_BEGIN')
    _opcode_BACKTRACK_TRY_BEGIN.attributes.add('alwaysinline')
    _opcode_BACKTRACK_TRY_BEGIN.attributes.add('optsize')
    
    _opcode_TRY_END = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_TRY_END')
    _opcode_TRY_END.attributes.add('alwaysinline')
    _opcode_TRY_END.attributes.add('optsize')
    
    _opcode_BACKTRACK_TRY_END = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_BACKTRACK_TRY_END')
    _opcode_BACKTRACK_TRY_END.attributes.add('alwaysinline')
    _opcode_BACKTRACK_TRY_END.attributes.add('optsize')
    
    _opcode_JUMP = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_JUMP')
    _opcode_JUMP.attributes.add('alwaysinline')
    _opcode_JUMP.attributes.add('optsize')
    
    _opcode_JUMP_F = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_JUMP_F')
    _opcode_JUMP_F.attributes.add('alwaysinline')
    _opcode_JUMP_F.attributes.add('optsize')
    
    _opcode_BACKTRACK = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_BACKTRACK')
    _opcode_BACKTRACK.attributes.add('alwaysinline')
    _opcode_BACKTRACK.attributes.add('optsize')
    
    _opcode_APPEND = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_APPEND')
    _opcode_APPEND.attributes.add('alwaysinline')
    _opcode_APPEND.attributes.add('optsize')
    
    _opcode_INSERT = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_INSERT')
    _opcode_INSERT.attributes.add('alwaysinline')
    _opcode_INSERT.attributes.add('optsize')
    
    _opcode_RANGE = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_RANGE')
    _opcode_RANGE.attributes.add('alwaysinline')
    _opcode_RANGE.attributes.add('optsize')
    
    _opcode_BACKTRACK_RANGE = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_BACKTRACK_RANGE')
    _opcode_BACKTRACK_RANGE.attributes.add('alwaysinline')
    _opcode_BACKTRACK_RANGE.attributes.add('optsize')
    
    _opcode_SUBEXP_BEGIN = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_SUBEXP_BEGIN')
    _opcode_SUBEXP_BEGIN.attributes.add('alwaysinline')
    _opcode_SUBEXP_BEGIN.attributes.add('optsize')
    
    _opcode_SUBEXP_END = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_SUBEXP_END')
    _opcode_SUBEXP_END.attributes.add('alwaysinline')
    _opcode_SUBEXP_END.attributes.add('optsize')
    
    _opcode_PATH_BEGIN = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_PATH_BEGIN')
    _opcode_PATH_BEGIN.attributes.add('alwaysinline')
    _opcode_PATH_BEGIN.attributes.add('optsize')
    
    _opcode_BACKTRACK_PATH_BEGIN = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_BACKTRACK_PATH_BEGIN')
    _opcode_BACKTRACK_PATH_BEGIN.attributes.add('alwaysinline')
    _opcode_BACKTRACK_PATH_BEGIN.attributes.add('optsize')
    
    _opcode_PATH_END = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_PATH_END')
    _opcode_PATH_END.attributes.add('alwaysinline')
    _opcode_PATH_END.attributes.add('optsize')
    
    _opcode_BACKTRACK_PATH_END = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_BACKTRACK_PATH_END')
    _opcode_BACKTRACK_PATH_END.attributes.add('alwaysinline')
    _opcode_BACKTRACK_PATH_END.attributes.add('optsize')
    
    _opcode_CALL_BUILTIN = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_CALL_BUILTIN')
    _opcode_CALL_BUILTIN.attributes.add('alwaysinline')
    _opcode_CALL_BUILTIN.attributes.add('optsize')
    
    _opcode_CALL_JQ = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_CALL_JQ')
    _opcode_CALL_JQ.attributes.add('alwaysinline')
    _opcode_CALL_JQ.attributes.add('optsize')
    
    _opcode_RET = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_RET')
    _opcode_RET.attributes.add('alwaysinline')
    _opcode_RET.attributes.add('optsize')
    
    _opcode_BACKTRACK_RET = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_BACKTRACK_RET')
    _opcode_BACKTRACK_RET.attributes.add('alwaysinline')
    _opcode_BACKTRACK_RET.attributes.add('optsize')
    
    _opcode_TAIL_CALL_JQ = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_TAIL_CALL_JQ')
    _opcode_TAIL_CALL_JQ.attributes.add('alwaysinline')
    _opcode_TAIL_CALL_JQ.attributes.add('optsize')
  
    _opcode_TOP = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_TOP')
    _opcode_TOP.attributes.add('alwaysinline')
    _opcode_TOP.attributes.add('optsize')
    
    _opcode_GENLABEL = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_GENLABEL')
    _opcode_GENLABEL.attributes.add('alwaysinline')
    _opcode_GENLABEL.attributes.add('optsize')
    
    _opcode_DESTRUCTURE_ALT = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_DESTRUCTURE_ALT')
    _opcode_DESTRUCTURE_ALT.attributes.add('alwaysinline')
    _opcode_DESTRUCTURE_ALT.attributes.add('optsize')
    
    _opcode_BACKTRACK_DESTRUCTURE_ALT = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_BACKTRACK_DESTRUCTURE_ALT')
    _opcode_BACKTRACK_DESTRUCTURE_ALT.attributes.add('alwaysinline')
    _opcode_BACKTRACK_DESTRUCTURE_ALT.attributes.add('optsize')
    
    _opcode_STOREVN = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_STOREVN')
    _opcode_STOREVN.attributes.add('alwaysinline')
    _opcode_STOREVN.attributes.add('optsize')
    
    _opcode_BACKTRACK_STOREVN = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_BACKTRACK_STOREVN')
    _opcode_BACKTRACK_STOREVN.attributes.add('alwaysinline')
    _opcode_BACKTRACK_STOREVN.attributes.add('optsize')
    
    _opcode_ERRORK = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), [void_ptr_type]),
                            name='_opcode_ERRORK')
    _opcode_ERRORK.attributes.add('alwaysinline')
    _opcode_ERRORK.attributes.add('optsize')
    
    # Define argument types for C function
    jq_util_funcs._get_num_opcodes.argtypes = []
    jq_util_funcs._get_num_opcodes.restype = c_int
    # Get value of NUM_OPCODES
    num_opcodes = jq_util_funcs._get_num_opcodes()
    backtracking_opcodes = (Opcode.RANGE.value, Opcode.STOREVN.value, Opcode.PATH_BEGIN.value, Opcode.PATH_END.value, Opcode.EACH.value, Opcode.EACH_OPT.value, Opcode.TRY_BEGIN.value, Opcode.TRY_END.value, Opcode.DESTRUCTURE_ALT.value, Opcode.FORK.value, Opcode.RET.value)
    backtracking_opcodes = tuple([opcode_val + num_opcodes for opcode_val in backtracking_opcodes])
    backtracking_opcode_funcs = (_opcode_BACKTRACK_RANGE, _opcode_BACKTRACK_STOREVN, _opcode_BACKTRACK_PATH_BEGIN, _opcode_BACKTRACK_PATH_END, _opcode_BACKTRACK_EACH, _opcode_BACKTRACK_EACH_OPT, _opcode_BACKTRACK_TRY_BEGIN, _opcode_BACKTRACK_TRY_END, _opcode_BACKTRACK_DESTRUCTURE_ALT, _opcode_BACKTRACK_FORK, _opcode_BACKTRACK_RET)
    
    # Define argument types for C function
    jq_util_funcs._get_opcode_list_len.argtypes = [c_void_p]
    jq_util_funcs._get_opcode_list_len.restype = c_int
    # Get opcode_list length from cjq_state
    opcode_lis_len = jq_util_funcs._get_opcode_list_len(opcodes_ptr)
    
    # Get all opcodes from opcode_list
    jq_util_funcs._opcode_list_at.argtypes = [c_void_p, c_int]
    jq_util_funcs._opcode_list_at.restype = c_uint8
    
    # Also, get all entry points from jq_next_entry_list
    jq_util_funcs._jq_next_entry_list_at.argtypes = [c_void_p, c_int]
    jq_util_funcs._jq_next_entry_list_at.restype = c_uint16
    jq_next_entry_idx = 0
    
    # Define argument types for C function
    jq_util_funcs._get_jq_next_entry_list_len.argtypes = [c_void_p]
    jq_util_funcs._get_jq_next_entry_list_len.restype = c_int
    # Get opcode_list length from cjq
    jq_next_entry_lis_len = jq_util_funcs._get_jq_next_entry_list_len(opcodes_ptr)
    
    # Also, be able to determine when to iterate to next input
    jq_util_funcs._next_input_list_at.argtypes = [c_void_p, c_int]
    jq_util_funcs._next_input_list_at.restype = c_uint16
    next_input_idx = 0
    
    # Define argument types for C function
    jq_util_funcs._get_next_input_list_len.argtypes = [c_void_p]
    jq_util_funcs._get_next_input_list_len.restype = c_int
    # Get opcode_list length from cjq
    next_input_lis_len = jq_util_funcs._get_next_input_list_len(opcodes_ptr)
    
    # Define argument types for C function
    jq_util_funcs._get_jq_halt_loc.argtypes = [c_void_p]
    jq_util_funcs._get_jq_halt_loc.restype = c_int
    # Get opcode_list length from cjq_state
    jq_halt_loc = jq_util_funcs._get_jq_halt_loc(opcodes_ptr)
    
    
    raw_list = []
    for opcode_lis_idx in range(opcode_lis_len):
        if next_input_idx < next_input_lis_len:
            # Check if we need to iterate cjq->value to the next JSON input
            next_input_loc = jq_util_funcs._next_input_list_at(opcodes_ptr, next_input_idx)
            if opcode_lis_idx == next_input_loc:
                if opcode_lis_idx != 0:
                    raw_list.append(CallType(Opcode.UPDATE_RES_STATE.value))
                    builder.call(_update_result_state, [_cjq])
                raw_list.append(CallType(Opcode.GET_NEXT_INPUT.value))
                builder.call(_get_next_input, [_cjq])
                next_input_idx += 1
        if jq_next_entry_idx < jq_next_entry_lis_len:
            # Check if we're at a jq_next entry point
            next_entry_point = jq_util_funcs._jq_next_entry_list_at(opcodes_ptr, jq_next_entry_idx)
            if opcode_lis_idx == next_entry_point:
                raw_list.append(CallType(Opcode.INIT_JQ_NEXT.value))
                builder.call(_init_jq_next, [_cjq])
                jq_next_entry_idx += 1
        curr_opcode = jq_util_funcs._opcode_list_at(opcodes_ptr, opcode_lis_idx)
        match curr_opcode:
            case Opcode.LOADK.value:
                builder.call(_opcode_LOADK, [_cjq])
            case Opcode.DUP.value:
                builder.call(_opcode_DUP, [_cjq])
            case Opcode.DUPN.value:
                builder.call(_opcode_DUPN, [_cjq])
            case Opcode.DUP2.value:
                builder.call(_opcode_DUP2, [_cjq])
            case Opcode.PUSHK_UNDER.value:
                builder.call(_opcode_PUSHK_UNDER, [_cjq])
            case Opcode.POP.value:
                builder.call(_opcode_POP, [_cjq])
            case Opcode.LOADV.value:
                builder.call(_opcode_LOADV, [_cjq])
            case Opcode.LOADVN.value:
                builder.call(_opcode_LOADVN, [_cjq])
            case Opcode.STOREV.value:
                builder.call(_opcode_STOREV, [_cjq])
            case Opcode.STORE_GLOBAL.value:
                builder.call(_opcode_STORE_GLOBAL, [_cjq])
            case Opcode.INDEX.value:
                builder.call(_opcode_INDEX, [_cjq])
            case Opcode.INDEX_OPT.value:
                builder.call(_opcode_INDEX_OPT, [_cjq])
            case Opcode.EACH.value:
                builder.call(_opcode_EACH, [_cjq])
            case Opcode.EACH_OPT.value:
                builder.call(_opcode_EACH_OPT, [_cjq])
            case Opcode.FORK.value:
                builder.call(_opcode_FORK, [_cjq])
            case Opcode.TRY_BEGIN.value:
                builder.call(_opcode_TRY_BEGIN, [_cjq])
            case Opcode.TRY_END.value:
                builder.call(_opcode_TRY_END, [_cjq])
            case Opcode.JUMP.value:
                builder.call(_opcode_JUMP, [_cjq])
            case Opcode.JUMP_F.value:
                builder.call(_opcode_JUMP_F, [_cjq])
            case Opcode.BACKTRACK.value:
                builder.call(_opcode_BACKTRACK, [_cjq])
            case Opcode.APPEND.value:
                builder.call(_opcode_APPEND, [_cjq])
            case Opcode.INSERT.value:
                builder.call(_opcode_INSERT, [_cjq])
            case Opcode.RANGE.value:
                builder.call(_opcode_RANGE, [_cjq])
            case Opcode.SUBEXP_BEGIN.value:
                builder.call(_opcode_SUBEXP_BEGIN, [_cjq])
            case Opcode.SUBEXP_END.value:
                builder.call(_opcode_SUBEXP_END, [_cjq])
            case Opcode.PATH_BEGIN.value:
                builder.call(_opcode_PATH_BEGIN, [_cjq])
            case Opcode.PATH_END.value:
                builder.call(_opcode_PATH_END, [_cjq])
            case Opcode.CALL_BUILTIN.value:
                builder.call(_opcode_CALL_BUILTIN, [_cjq])
            case Opcode.CALL_JQ.value:
                builder.call(_opcode_CALL_JQ, [_cjq])
            case Opcode.RET.value:
                builder.call(_opcode_RET, [_cjq])
            case Opcode.TAIL_CALL_JQ.value:
                builder.call(_opcode_TAIL_CALL_JQ, [_cjq])
            case Opcode.TOP.value:
                builder.call(_opcode_TOP, [_cjq])
            case Opcode.GENLABEL.value:
                builder.call(_opcode_GENLABEL, [_cjq])
            case Opcode.DESTRUCTURE_ALT.value:
                builder.call(_opcode_DESTRUCTURE_ALT, [_cjq])
            case Opcode.STOREVN.value:
                builder.call(_opcode_STOREVN, [_cjq])
            case Opcode.ERRORK.value:
                builder.call(_opcode_ERRORK, [_cjq])
            case _:
                if curr_opcode in backtracking_opcodes:
                    opcode_idx = backtracking_opcodes.index(curr_opcode)
                    raw_list.append(CallType(opcode_idx, 1))
                    builder.call(backtracking_opcode_funcs[opcode_idx], [_cjq])
                else:
                    raise ValueError(f"Current opcode: {curr_opcode} does not match any existing opcodes")
    
    # Check if JQ program halted
    if opcode_lis_idx == jq_halt_loc:
        raw_list.append(CallType(Opcode.JQ_HALT.value))
        builder.call(_jq_halt, [_cjq])
    # Update for final state
    raw_list.append(CallType(Opcode.UPDATE_RES_STATE.value))
    builder.call(_update_result_state, [_cjq])
    # Return from main
    builder.ret_void()
    
    for op in raw_list:
        print(op)
    
    print(f"len(raw_list): {len(raw_list)}")
    subseqs = max_unique_subseq(raw_list)
    print(subseqs)
    print(coalesce_tuples(generate_subseq_lis([call.opcode for call in raw_list], subseqs)))
    return module
    
def generate_llvm_ir(opcodes_ptr):
    try:
        llvm_ir = jq_lower(opcodes_ptr)
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