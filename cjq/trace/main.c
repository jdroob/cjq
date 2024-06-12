#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../jq/src/compile.h"
#include "../jq/src/jv.h"
#include "../jq/src/jq.h"
#include "../jq/src/jv_alloc.h"
#include "../jq/src/util.h"
#include "../jq/src/version.h"
#include "cjq_trace.h"

#define PY_SSIZE_T_CLEAN
#include <Python.h>


int init_cpython(const char *path_to_cjq, PyObject **pModule_llvmlite, PyObject **pModuleLowering) {
    Py_Initialize();
    
    PyRun_SimpleString("import sys");
    char python_code[512];
    snprintf(python_code, sizeof(python_code), "sys.path.append(\"%s\")", path_to_cjq);
    PyRun_SimpleString(python_code);

    const char *python_path = getenv("PYTHONPATH");
    if (python_path) {
        snprintf(python_code, sizeof(python_code), "sys.path.append(\"%s\")", python_path);
        PyRun_SimpleString(python_code);
    }

    *pModule_llvmlite = PyImport_ImportModule("llvmlite");
    if (!(*pModule_llvmlite)) {
        PyErr_Print();
        fprintf(stderr, "Failed to import module 'llvmlite'\n");
        return 1;
    }

    snprintf(python_code, sizeof(python_code), "sys.path.append(\"%s\")", path_to_cjq);
    PyRun_SimpleString(python_code);
    *pModuleLowering = PyImport_ImportModule("lowering");
    if (!(*pModuleLowering)) {
        PyErr_Print();
        fprintf(stderr, "Failed to import module 'lowering'\n");
        return 1;
    }

    return 0;
}

int get_llvm_ir(trace* opcode_trace, PyObject* pModule_llvmlite, PyObject* pModuleLowering) {
    // Get generate_llvm_ir function from lowering module
    PyObject *pFuncGenerateLLVMIR = PyObject_GetAttrString(pModuleLowering, "generate_llvm_ir");

    if (!pFuncGenerateLLVMIR || !PyCallable_Check(pFuncGenerateLLVMIR)) {
        if (PyErr_Occurred()) PyErr_Print();
        fprintf(stderr, "Cannot find function 'generate_llvm_ir'\n");
        return 1;
    }
    
    PyObject* opcode_trace_ptr = PyLong_FromVoidPtr((void*)opcode_trace);
    PyObject* pResult = PyObject_CallFunctionObjArgs(pFuncGenerateLLVMIR, opcode_trace_ptr, NULL);

    // Clean up
    Py_DECREF(pResult);
    Py_DECREF(pFuncGenerateLLVMIR);
    Py_DECREF(pModuleLowering);
    Py_DECREF(pModule_llvmlite);
    Py_Finalize();

    return 0;
}

int main(int argc, char *argv[]) {
    char *home = getenv("HOME");
    if (home == NULL) {
        fprintf(stderr, "Failed to retrieve HOME environment variable\n");
        return 1;
    }

    char path_to_cjq[256]; 
    snprintf(path_to_cjq, sizeof(path_to_cjq), "%s/cjq/cjq/trace", home);

    PyObject* pModule_llvmlite = NULL;
    PyObject* pModuleLowering = NULL;

    if (init_cpython(path_to_cjq, &pModule_llvmlite, &pModuleLowering) != 0) {
        // Error occurred during Python initialization
        return 1;
    }

    trace* opcode_trace = init_trace();
    int trace_error = cjq_trace(argc, argv, opcode_trace);
    int gen_ir_error = get_llvm_ir(opcode_trace, pModule_llvmlite, pModuleLowering);
    free_trace(opcode_trace);

    return 0;
}
