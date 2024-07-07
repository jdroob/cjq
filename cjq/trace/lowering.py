from llvmlite import binding as llvm
from llvmlite import ir
from ctypes import *
from enum import Enum
import os

# set of unique subsequences of calls to opcode functions
subseqs_g = set()

# map of subseq -> func (e.g. (CallType(1,0),CallType(2,0)) -> (_opcode_DUP, _opcode_DUPN))
dyn_op_subseq_to_op_func_subseq = {}

# global indices
subseq_func_idx = 0
loop_block_idx = 0
block_idx = 0
i_idx = 0

# mapping from subseq -> subsequence_func containing corresponding opcode funcs (e.g. (CallType(1,0), CallType(2,0)) -> subseq_func30; def subseq_func30: call _opcode_DUP; call _opcode_DUPN; ret void;
dyn_op_subseq_to_subseq_func = {}

# set up llvmlite
llvm.initialize()
llvm.initialize_native_target()
llvm.initialize_native_asmprinter()

target = llvm.Target.from_default_triple()
target_machine = target.create_target_machine()

data_layout = target_machine.target_data
triple = llvm.get_process_triple()

module = ir.Module()
module.data_layout = data_layout
module.triple = triple

# declare llvmlite primitives
void_ptr_type = ir.IntType(8).as_pointer()
main_func_type = ir.FunctionType(ir.VoidType(), [void_ptr_type])
main_func = ir.Function(module, main_func_type, "jq_program")
main_block = main_func.append_basic_block("entry")
builder = ir.IRBuilder(main_block)
most_recent_block = main_block

# get reference to jq_program arg
_cjq, = main_func.args

# define opcode functions
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
backtracking_opcode_funcs = (_opcode_BACKTRACK_RANGE, _opcode_BACKTRACK_STOREVN, _opcode_BACKTRACK_PATH_BEGIN, _opcode_BACKTRACK_PATH_END, _opcode_BACKTRACK_EACH, _opcode_BACKTRACK_EACH_OPT, _opcode_BACKTRACK_TRY_BEGIN, _opcode_BACKTRACK_TRY_END, _opcode_BACKTRACK_DESTRUCTURE_ALT, _opcode_BACKTRACK_FORK, _opcode_BACKTRACK_RET)


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
        self.opcode=opcode
        self.backtracking=backtracking
        
    def __str__(self):
        return f"opcode: {self.opcode}, backtracking flag: {self.backtracking}"
    
    def __ge__(self, other):
        return self.opcode > other.opcode
    
    def __le__(self, other):
        return self.opcode < other.opcode
    
    def __eq__(self, other):
        return isinstance(other, CallType) and self.opcode == other.opcode and self.backtracking == other.backtracking
    
    def __ne__(self, other):
        return self.opcode != other.opcode and self.backtracking != other.backtracking

    def __hash__(self):
        return hash((self.opcode, self.backtracking))
    
    
def match_on_opcode(lis, curr_opcode, backtracking_opcodes):
    match curr_opcode:
            case Opcode.LOADK.value:
                lis.append(CallType(Opcode.LOADK.value))
            case Opcode.DUP.value:
                lis.append(CallType(Opcode.DUP.value))
            case Opcode.DUPN.value:
                lis.append(CallType(Opcode.DUPN.value))
            case Opcode.DUP2.value:
                lis.append(CallType(Opcode.DUP2.value))
            case Opcode.PUSHK_UNDER.value:
                lis.append(CallType(Opcode.PUSHK_UNDER.value))
            case Opcode.POP.value:
                lis.append(CallType(Opcode.POP.value))
            case Opcode.LOADV.value:
                lis.append(CallType(Opcode.LOADV.value))
            case Opcode.LOADVN.value:
                lis.append(CallType(Opcode.LOADVN.value))
            case Opcode.STOREV.value:
                lis.append(CallType(Opcode.STOREV.value))
            case Opcode.STORE_GLOBAL.value:
                lis.append(CallType(Opcode.STORE_GLOBAL.value))
            case Opcode.INDEX.value:
                lis.append(CallType(Opcode.INDEX.value))
            case Opcode.INDEX_OPT.value:
                lis.append(CallType(Opcode.INDEX_OPT.value))
            case Opcode.EACH.value:
                lis.append(CallType(Opcode.EACH.value))
            case Opcode.EACH_OPT.value:
                lis.append(CallType(Opcode.EACH_OPT.value))
            case Opcode.FORK.value:
                lis.append(CallType(Opcode.FORK.value))
            case Opcode.TRY_BEGIN.value:
                lis.append(CallType(Opcode.TRY_BEGIN.value))
            case Opcode.TRY_END.value:
                lis.append(CallType(Opcode.TRY_END.value))
            case Opcode.JUMP.value:
                lis.append(CallType(Opcode.JUMP.value))
            case Opcode.JUMP_F.value:
                lis.append(CallType(Opcode.JUMP_F.value))
            case Opcode.BACKTRACK.value:
                lis.append(CallType(Opcode.BACKTRACK.value))
            case Opcode.APPEND.value:
                lis.append(CallType(Opcode.APPEND.value))
            case Opcode.INSERT.value:
                lis.append(CallType(Opcode.INSERT.value))
            case Opcode.RANGE.value:
                lis.append(CallType(Opcode.RANGE.value))
            case Opcode.SUBEXP_BEGIN.value:
                lis.append(CallType(Opcode.SUBEXP_BEGIN.value))
            case Opcode.SUBEXP_END.value:
                lis.append(CallType(Opcode.SUBEXP_END.value))
            case Opcode.PATH_BEGIN.value:
                lis.append(CallType(Opcode.PATH_BEGIN.value))
            case Opcode.PATH_END.value:
                lis.append(CallType(Opcode.PATH_END.value))
            case Opcode.CALL_BUILTIN.value:
                lis.append(CallType(Opcode.CALL_BUILTIN.value))
            case Opcode.CALL_JQ.value:
                lis.append(CallType(Opcode.CALL_JQ.value))
            case Opcode.RET.value:
                lis.append(CallType(Opcode.RET.value))
            case Opcode.TAIL_CALL_JQ.value:
                lis.append(CallType(Opcode.TAIL_CALL_JQ.value))
            case Opcode.TOP.value:
                lis.append(CallType(Opcode.TOP.value))
            case Opcode.GENLABEL.value:
                lis.append(CallType(Opcode.GENLABEL.value))
            case Opcode.DESTRUCTURE_ALT.value:
                lis.append(CallType(Opcode.DESTRUCTURE_ALT.value))
            case Opcode.STOREVN.value:
                lis.append(CallType(Opcode.STOREVN.value))
            case Opcode.ERRORK.value:
                lis.append(CallType(Opcode.ERRORK.value))
            case _:
                if curr_opcode in backtracking_opcodes:
                    opcode_idx = backtracking_opcodes.index(curr_opcode)
                    lis.append(CallType(opcode_idx, 1))
                else:
                    raise ValueError(f"Current opcode: {curr_opcode} does not match any existing opcodes")

def find_shortest_repeating_subsequence(sequence):
    n = len(sequence)
    for length in range(1, n // 2 + 1):
        subseq = sequence[:length]
        count = 0
        pos = 0

        while pos + length <= n and sequence[pos:pos+length] == subseq:
            count += 1
            pos += length

        if count > 1 and pos == n:
            return subseq, count

    return None, 0

def compress_op_lis(op_lis):
    n = len(op_lis)
    encoding = []
    i = 0

    while i < n:
        max_length = float('inf')
        best_subseq = []
        max_count = 0

        # check for repeating sequences
        for j in range(i + 1, n + 1):
            subseq = op_lis[i:j]
            sub_length = len(subseq)
            count = 0
            pos = i

            # count number of times the sequence repeats consecutively
            while pos + sub_length <= n and op_lis[pos:pos+sub_length] == subseq:
                count += 1
                pos += sub_length

            # update the best (shortest) repeating sequence found
            if count > 1 and (sub_length < max_length or (sub_length == max_length and count > max_count)):
                max_length = sub_length
                best_subseq = subseq
                max_count = count

        if best_subseq:
            encoding.append((best_subseq, max_count))
            i += max_length * max_count
        else:
            encoding.append(([op_lis[i]], 1))
            i += 1

    # compress each found sequence if it has internal repetitions
    compressed_op_lis = []
    for sequence, count in encoding:
        if len(sequence) > 1:
            sub_seq, sub_count = find_shortest_repeating_subsequence(sequence)
            if sub_seq:
                compressed_op_lis.append((sub_seq, sub_count * count))
            else:
                compressed_op_lis.append((sequence, count))
        else:
            compressed_op_lis.append((sequence, count))

    return compressed_op_lis

def gen_unique_subseq_lis(sequence):
    subsequences = set()
    current_subsequence = []
    CHUNK_SIZE = 10

    for i, call in enumerate(sequence):
        current_subsequence.append(call)

        # check if we have formed a subsequence of size CHUNK_SIZE
        if len(current_subsequence) == CHUNK_SIZE:
            subsequences.add(tuple(current_subsequence))
            current_subsequence = []

    # add any remaining elements as the final subsequence
    if current_subsequence:
        subsequences.add(tuple(current_subsequence))

    return sorted(subsequences, key=len, reverse=True)
         
def gen_dyn_op_lis(buffer, buffer_subseqs):
    dyn_op_lis = []
    i = 0
       
    while i < len(buffer):
        matched = False
        for subseq in buffer_subseqs:
            subseq_len = len(subseq)
            buffer_slice = tuple(buffer[i:i+subseq_len])
            if buffer_slice == subseq:
                dyn_op_lis.append(subseq)
                i += subseq_len
                matched = True
                break
        if not matched:
            buffer_subseqs.add(tuple([buffer[i]]))
            for _subseq in buffer_subseqs:
                if _subseq == tuple([buffer[i]]):
                    dyn_op_lis.append(_subseq) 
            i += 1 
    return dyn_op_lis

def save_trace(opcodes_ptr):
    global subseqs
    global dyn_op_subseq_to_op_func_subseq
    global subseq_func_idx
    global loop_block_idx
    global block_idx
    global i_idx
    global dyn_op_subseq_to_subseq_func
    global main_block
    global most_recent_block
    global _cjq
    home_dir = os.path.expanduser("~")
    so_file = os.path.join(home_dir, "cjq/jq_util.so")
    
    # need this to call C functions from Python
    jq_util_funcs = CDLL(so_file)
    
    # get current trace state
    jq_util_funcs._get_opcode_list_len.argtypes = [c_void_p]
    jq_util_funcs._get_opcode_list_len.restype = c_uint64
    opcode_lis_len = jq_util_funcs._get_opcode_list_len(opcodes_ptr)
    
    jq_util_funcs._get_next_input_list_len.argtypes = [c_void_p]
    jq_util_funcs._get_next_input_list_len.restype = c_uint64
    next_input_lis_len = jq_util_funcs._get_next_input_list_len(opcodes_ptr)
    
    jq_util_funcs._opcode_list_at.argtypes = [c_void_p, c_uint64]
    jq_util_funcs._opcode_list_at.restype = c_uint8
    
    jq_util_funcs._jq_next_entry_list_at.argtypes = [c_void_p, c_uint64]
    jq_util_funcs._jq_next_entry_list_at.restype = c_uint64
    jq_next_entry_idx = 0
    
    jq_util_funcs._get_jq_next_entry_list_len.argtypes = [c_void_p]
    jq_util_funcs._get_jq_next_entry_list_len.restype = c_uint64
    jq_next_entry_lis_len = jq_util_funcs._get_jq_next_entry_list_len(opcodes_ptr)
    
    jq_util_funcs._next_input_list_at.argtypes = [c_void_p, c_uint64]
    jq_util_funcs._next_input_list_at.restype = c_uint64
    next_input_idx = 0
    
    jq_util_funcs._get_jq_halt_loc.argtypes = [c_void_p]
    jq_util_funcs._get_jq_halt_loc.restype = c_uint64
    jq_halt_loc = jq_util_funcs._get_jq_halt_loc(opcodes_ptr)
    
    jq_util_funcs._get_num_opcodes.argtypes = []
    jq_util_funcs._get_num_opcodes.restype = c_int
    
    # get value of NUM_OPCODES
    # TODO: Make this global?
    num_opcodes = jq_util_funcs._get_num_opcodes()
    
    # trick to easily look up reference to backtracking opcode function given opcode
    backtracking_opcodes = (Opcode.RANGE.value, Opcode.STOREVN.value, Opcode.PATH_BEGIN.value, Opcode.PATH_END.value, Opcode.EACH.value, Opcode.EACH_OPT.value, Opcode.TRY_BEGIN.value, Opcode.TRY_END.value, Opcode.DESTRUCTURE_ALT.value, Opcode.FORK.value, Opcode.RET.value)
    backtracking_opcodes = tuple([opcode_val + num_opcodes for opcode_val in backtracking_opcodes])
    
    # 1. record this buffer of dynamic opcodes as a list of CallType object
    buffer = []     # list of CallType objects
    for opcode_lis_idx in range(opcode_lis_len):
        if next_input_idx < next_input_lis_len:
            # check if we need to iterate cjq->value to the next JSON input
            next_input_loc = jq_util_funcs._next_input_list_at(opcodes_ptr, next_input_idx)
            if opcode_lis_idx == next_input_loc:
                if opcode_lis_idx != 0:
                    buffer.append(CallType(Opcode.UPDATE_RES_STATE.value))
                buffer.append(CallType(Opcode.GET_NEXT_INPUT.value))
                next_input_idx += 1
        if jq_next_entry_idx < jq_next_entry_lis_len:
            # check if we're at a jq_next entry point
            next_entry_point = jq_util_funcs._jq_next_entry_list_at(opcodes_ptr, jq_next_entry_idx)
            if opcode_lis_idx == next_entry_point:
                buffer.append(CallType(Opcode.INIT_JQ_NEXT.value))
                jq_next_entry_idx += 1
        curr_opcode = jq_util_funcs._opcode_list_at(opcodes_ptr, opcode_lis_idx)
        match_on_opcode(buffer, curr_opcode, backtracking_opcodes)
    # check if program halted
    if opcode_lis_len-1 == jq_halt_loc:
        buffer.append(CallType(Opcode.JQ_HALT.value))

    # 2. generate set of subsequences that appear in buffer
    buffer_subseqs = set(gen_unique_subseq_lis(buffer))
    
    # 3. generate a list of references to subsequences that matches 
    #    order of dynamic opcodes from buffer
    dyn_op_lis = gen_dyn_op_lis(buffer, buffer_subseqs)
    
    # 4. subseqs_g := subseqs_g UNION buffer_subseqs
    subseqs_g.update(buffer_subseqs)
    
    # 5. update map of CallType object subsequences to corresponding subsequences of LLVM function calls
    for subseq in subseqs_g:
        func = []
        for dyn_op in subseq:
            if not dyn_op.backtracking:
                match dyn_op.opcode:
                    case Opcode.LOADK.value:
                        func.append(_opcode_LOADK)
                    case Opcode.DUP.value:
                        func.append(_opcode_DUP)
                    case Opcode.DUPN.value:
                        func.append(_opcode_DUPN)
                    case Opcode.DUP2.value:
                        func.append(_opcode_DUP2)
                    case Opcode.PUSHK_UNDER.value:
                        func.append(_opcode_PUSHK_UNDER)
                    case Opcode.POP.value:
                        func.append(_opcode_POP)
                    case Opcode.LOADV.value:
                        func.append(_opcode_LOADV)
                    case Opcode.LOADVN.value:
                        func.append(_opcode_LOADVN)
                    case Opcode.STOREV.value:
                        func.append(_opcode_STOREV)
                    case Opcode.STORE_GLOBAL.value:
                        func.append(_opcode_STORE_GLOBAL)
                    case Opcode.INDEX.value:
                        func.append(_opcode_INDEX)
                    case Opcode.INDEX_OPT.value:
                        func.append(_opcode_INDEX_OPT)
                    case Opcode.EACH.value:
                        func.append(_opcode_EACH)
                    case Opcode.EACH_OPT.value:
                        func.append(_opcode_EACH_OPT)
                    case Opcode.FORK.value:
                        func.append(_opcode_FORK)
                    case Opcode.TRY_BEGIN.value:
                        func.append(_opcode_TRY_BEGIN)
                    case Opcode.TRY_END.value:
                        func.append(_opcode_TRY_END)
                    case Opcode.JUMP.value:
                        func.append(_opcode_JUMP)
                    case Opcode.JUMP_F.value:
                        func.append(_opcode_JUMP_F)
                    case Opcode.BACKTRACK.value:
                        func.append(_opcode_BACKTRACK)
                    case Opcode.APPEND.value:
                        func.append(_opcode_APPEND)
                    case Opcode.INSERT.value:
                        func.append(_opcode_INSERT)
                    case Opcode.RANGE.value:
                        func.append(_opcode_RANGE)
                    case Opcode.SUBEXP_BEGIN.value:
                        func.append(_opcode_SUBEXP_BEGIN)
                    case Opcode.SUBEXP_END.value:
                        func.append(_opcode_SUBEXP_END)
                    case Opcode.PATH_BEGIN.value:
                        func.append(_opcode_PATH_BEGIN)
                    case Opcode.PATH_END.value:
                        func.append(_opcode_PATH_END)
                    case Opcode.CALL_BUILTIN.value:
                        func.append(_opcode_CALL_BUILTIN)
                    case Opcode.CALL_JQ.value:
                        func.append(_opcode_CALL_JQ)
                    case Opcode.RET.value:
                        func.append(_opcode_RET)
                    case Opcode.TAIL_CALL_JQ.value:
                        func.append(_opcode_TAIL_CALL_JQ)
                    case Opcode.TOP.value:
                        func.append(_opcode_TOP)
                    case Opcode.GENLABEL.value:
                        func.append(_opcode_GENLABEL)
                    case Opcode.DESTRUCTURE_ALT.value:
                        func.append(_opcode_DESTRUCTURE_ALT)
                    case Opcode.STOREVN.value:
                        func.append(_opcode_STOREVN)
                    case Opcode.ERRORK.value:
                        func.append(_opcode_ERRORK)
                    case Opcode.INIT_JQ_NEXT.value:
                        func.append(_init_jq_next)
                    case Opcode.GET_NEXT_INPUT.value:
                        func.append(_get_next_input)
                    case Opcode.UPDATE_RES_STATE.value:
                        func.append(_update_result_state)
                    case Opcode.JQ_HALT.value:
                        func.append(_jq_halt)
                    case _:
                        raise ValueError(f"Current opcode: {dyn_op.opcode} does not match any existing opcodes")
            else:
                func.append(backtracking_opcode_funcs[dyn_op.opcode])   # index of backtracking func in backtracking_opcode_funcs
        if not subseq in dyn_op_subseq_to_op_func_subseq:
            dyn_op_subseq_to_op_func_subseq[subseq] = tuple(func)
    
    # 6. create map of subsequences of CallType objects to subsequence functions (LLVM functions that call a specific subsequence of opcode functions)
    subseq_func_type = main_func_type
    for subseq in subseqs_g:
        if not subseq in dyn_op_subseq_to_subseq_func:
            dyn_op_subseq_to_subseq_func[subseq] = ir.Function(module, subseq_func_type, "_subsequence_func"+str(subseq_func_idx))
            dyn_op_subseq_to_subseq_func[subseq].attributes.add('noinline')
            subseq_block = dyn_op_subseq_to_subseq_func[subseq].append_basic_block("subsequence_"+str(subseq_func_idx))
            builder = ir.IRBuilder(subseq_block)
            for opcode_func in dyn_op_subseq_to_op_func_subseq[subseq]:
                builder.call(opcode_func, [_cjq])
            builder.ret_void()
            subseq_func_idx += 1
    # 7. generate compressed representation of dynamic opcode sequence.
    #    this is required for loop construction - compressed representation is variant of run-length encoding (RLE)
    #       e.g. (subseq3, subseq4, subseq5) *10, (subseq6) *1, (subseq7,subseq8) * 50, ...
    compressed_op_lis = compress_op_lis(dyn_op_lis)
    # formatted_output = ', '.join([f"{''.join([str(ct)+" " for ct in sequence])} * {count}\n" for sequence, count in compressed_op_lis])
            
    builder = ir.IRBuilder(most_recent_block)
    for sequence, n_iterations in compressed_op_lis:
        if n_iterations == 1:
            builder.call(dyn_op_subseq_to_subseq_func[sequence[0]], [_cjq])
        else:   # build a loop
            i = builder.alloca(ir.IntType(32), name="i."+str(i_idx))
            i_idx += 1
            builder.store(ir.Constant(ir.IntType(32), 0), i)
            
            loop_cond_block = main_func.append_basic_block(name="loop_cond"+str(loop_block_idx))
            loop_body_block = main_func.append_basic_block(name="loop_body"+str(loop_block_idx))
            loop_end_block = main_func.append_basic_block(name="loop_end"+str(loop_block_idx))
            loop_block_idx += 1
            builder.branch(loop_cond_block)

            builder.position_at_end(loop_cond_block)
            i_val = builder.load(i, name="i_val")
            condition = builder.icmp_signed('<', i_val, ir.Constant(ir.IntType(32), n_iterations), name="loop_cond_var"+str(loop_block_idx))
            builder.cbranch(condition, loop_body_block, loop_end_block)

            builder.position_at_end(loop_body_block)
            for subseq in sequence:
                builder.call(dyn_op_subseq_to_subseq_func[subseq], [_cjq])
            i_val = builder.load(i, name="i_val")
            incremented = builder.add(i_val, ir.Constant(ir.IntType(32), 1), name="incremented")
            builder.store(incremented, i)
            builder.branch(loop_cond_block)

            builder.position_at_end(loop_end_block)
            
        block = main_func.append_basic_block(name="block_"+str(block_idx))
        builder.branch(block)
        builder.position_at_end(block)
        most_recent_block = block
        block_idx += 1
    
def finalize_ir():
    # add in final instructions
    builder = ir.IRBuilder(most_recent_block)
    builder.call(_update_result_state, [_cjq])
    builder.ret_void()
    return module      

def generate_llvm_ir():
    try:
        llvm_ir = finalize_ir()
        mod = llvm.parse_assembly(str(llvm_ir))
        mod.verify()
        
        with open("ir.ll", "w") as file:
            file.write(str(mod))
            
    except Exception as e:
        print("Error generating LLVM IR:", e)
        exit(1)