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
#include "frontend/src/frontend_main.h"

#define PY_SSIZE_T_CLEAN
#include <../../usr/include/python3.12/Python.h>      // TODO: yuck
#include <../../usr/include/python3.12/pyconfig.h>      // TODO: yuck

#define jq_exit(r)              exit( r > 0 ? r : 0 )

// Globals
compiled_jq_state cjq_state;
extern void jq_program();

void cleanUp(compiled_jq_state* cjq_state) {
    // Free memory allocated by frontend_main
    jq_util_input_free(&(cjq_state->input_state));
    jq_teardown(&(cjq_state->jq));
}

int get_llvm_ir() {
    // Get the value of the HOME environment variable
    char *home = getenv("HOME");
    if (home == NULL) {
        fprintf(stderr, "Failed to retrieve HOME environment variable\n");
        return 1; // Or handle the error accordingly
    }

    // Construct the full path to the cjq directory
    char path_to_cjq[256]; // Adjust the size as needed
    snprintf(path_to_cjq, sizeof(path_to_cjq), "%s/cjq/cjq", home);

    // Initialize Python Interpreter
    Py_Initialize();

    // Append paths to Python sys.path
    PyRun_SimpleString("import sys");
    PyRun_SimpleString("sys.path.append('/home/rubio/anaconda3/envs/numbaEnv/lib/python3.12/site-packages/')\n");

    // Now we can import llvmlite module
    PyObject *pModule_llvmlite = PyImport_ImportModule("llvmlite");
    if (!pModule_llvmlite) {
        PyErr_Print();
        fprintf(stderr, "Failed to import module 'llvmlite'\n");
        return 1;
    }

    // Append the cjq directory to Python path
    char python_code[512]; // Adjust the size as needed
    snprintf(python_code, sizeof(python_code), "sys.path.append(\"%s\")", path_to_cjq);

    // Now we can also import lowering module
    PyRun_SimpleString(python_code);

    // Import the required Python module
    PyObject *pModuleLowering = PyImport_ImportModule("lowering");
    if (!pModuleLowering) {
        PyErr_Print();
        fprintf(stderr, "Failed to import module 'lowering'\n");
        return 1;
    }

    // Get the Python function jq_lower from the lowering module
    PyObject *pFuncGenerateLLVMIR = PyObject_GetAttrString(pModuleLowering, "generate_llvm_ir");
    if (!pFuncGenerateLLVMIR || !PyCallable_Check(pFuncGenerateLLVMIR)) {
        if (PyErr_Occurred()) PyErr_Print();
        fprintf(stderr, "Cannot find function 'jq_lower'\n");
        return 1;
    }

    // Call the Python function
    PyObject *pResult = PyObject_CallObject(pFuncGenerateLLVMIR, NULL);
    
    // Cleanup
    Py_DECREF(pResult);
    Py_DECREF(pFuncGenerateLLVMIR);
    Py_DECREF(pModuleLowering);
    Py_DECREF(pModule_llvmlite);

    // Finalize Python
    Py_Finalize();

    // Return gracefully
    return 0;
}

int main(int argc, char *argv[]) {
    // Generate LLVM IR
    // int errorCode = get_llvm_ir();  // TODO: error handling
    
    // Get parsed bytecode, pc, additional info from frontend_main
    int error_code = frontend_main(argc, argv);
    printf("We're back babyyyy\n");
    // Execute compiled program
    jq_program();
    // Free up resources
    // cleanUp(&cjq_state);     // TODO: Fixme

    return 0;
    // jq_exit(*cjq_state.ret);
}
