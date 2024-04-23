#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "clib/lib.h"

#include "frontend/src/compile.h"
#include "frontend/src/jv.h"
#include "frontend/src/jq.h"
#include "frontend/src/jv_alloc.h"
#include "frontend/src/util.h"
#include "frontend/src/version.h"
#include "frontend/src/cjq_frontend.h"

#define PY_SSIZE_T_CLEAN
#include <../../usr/include/python3.12/Python.h>      // TODO: yuck
#include <../../usr/include/python3.12/pyconfig.h>      // TODO: yuck

#define jq_exit(r)              exit( r > 0 ? r : 0 )

// Globals
compiled_jq_state cjq_state;
extern void jq_program();

void clean_up(compiled_jq_state* cjq_state) {
    // Free memory allocated by cjq_frontend
    jq_util_input_free(&(cjq_state->input_state));
    jq_teardown(&(cjq_state->jq));
}

int get_llvm_ir() {
    // Get value of HOME environment variable
    char *home = getenv("HOME");
    if (home == NULL) {
        fprintf(stderr, "Failed to retrieve HOME environment variable\n");
        return 1;
    }

    // Construct full path to the cjq directory
    char path_to_cjq[256]; 
    snprintf(path_to_cjq, sizeof(path_to_cjq), "%s/cjq/cjq", home);

    // Initialize CPython
    Py_Initialize();

    // Append paths to Python sys.path
    PyRun_SimpleString("import sys");
    PyRun_SimpleString("sys.path.append('/home/rubio/anaconda3/envs/numbaEnv/lib/python3.12/site-packages/')\n");

    // Import llvmlite module
    PyObject *pModule_llvmlite = PyImport_ImportModule("llvmlite");
    if (!pModule_llvmlite) {
        PyErr_Print();
        fprintf(stderr, "Failed to import module 'llvmlite'\n");
        return 1;
    }

    // Append cjq directory to Python path
    char python_code[512];
    snprintf(python_code, sizeof(python_code), "sys.path.append(\"%s\")", path_to_cjq);

    // Import lowering module
    PyRun_SimpleString(python_code);
    PyObject *pModuleLowering = PyImport_ImportModule("lowering");
    if (!pModuleLowering) {
        PyErr_Print();
        fprintf(stderr, "Failed to import module 'lowering'\n");
        return 1;
    }

    // Get jq_lower function from lowering module
    PyObject *pFuncGenerateLLVMIR = PyObject_GetAttrString(pModuleLowering, "generate_llvm_ir");
    if (!pFuncGenerateLLVMIR || !PyCallable_Check(pFuncGenerateLLVMIR)) {
        if (PyErr_Occurred()) PyErr_Print();
        fprintf(stderr, "Cannot find function 'jq_lower'\n");
        return 1;
    }

    // Call jq_lower
    PyObject *pResult = PyObject_CallObject(pFuncGenerateLLVMIR, NULL);
    
    // Cleanup
    Py_DECREF(pResult);
    Py_DECREF(pFuncGenerateLLVMIR);
    Py_DECREF(pModuleLowering);
    Py_DECREF(pModule_llvmlite);
    Py_Finalize();

    // Return gracefully
    return 0;
}

int main(int argc, char *argv[]) {
    // Generate LLVM IR
    // int errorCode = get_llvm_ir();  // TODO: refactor + error handling
    
    // Parse source code & prepare setup for lowering
    int error_code = cjq_parse(argc, argv);

    // Execute compiled program
    jq_program();

    // Free up resources
    // clean_up(&cjq_state);     // TODO: Fixme

    return 0;
}
