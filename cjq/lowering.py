from llvmlite import binding as llvm
from llvmlite import ir
import ctypes


def jq_lower():
    """
    Uses llvmlite C-binding feature to call cjq_execute from llvmlite.
    """
    # print("Calling cjq.lowering.jq_lower() function from cjq/main.c")
   
    # Create LLVM module
    module = ir.Module()
    module.triple = llvm.get_process_triple()
    
    # Define entry point
    main_func_type = ir.FunctionType(ir.VoidType(), [])
    main_func = ir.Function(module, main_func_type, "jq_program")
    main_block = main_func.append_basic_block("entry")
    builder = ir.IRBuilder(main_block)
    
    # Declare _cjq_execute - this links this identifier to C function
    _cjq_execute = ir.Function(module,
                            ir.FunctionType(ir.VoidType(), []),
                            name='_cjq_execute')
    
    # Call _cjq_execute
    builder.call(_cjq_execute, []) 
    
    # Return from main
    builder.ret_void()
    
    return module
    
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