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

#include <../../usr/include/python3.12/Python.h>      // TODO: yuck
#include <../../usr/include/python3.12/pyconfig.h>      // TODO: yuck

void cleanUp(compiled_jq_state* cjq_state) {
    // Free memory allocated by frontend_main
    jq_util_input_free(&cjq_state->input_state);
    jq_teardown(&cjq_state->jq);
}

int call_jq_lower(compiled_jq_state* cjq_state) {
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
    // TODO: yuck - find way to generalize this
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
    PyObject *pFuncJqLower = PyObject_GetAttrString(pModuleLowering, "jq_lower");
    if (!pFuncJqLower || !PyCallable_Check(pFuncJqLower)) {
        if (PyErr_Occurred()) PyErr_Print();
        fprintf(stderr, "Cannot find function 'jq_lower'\n");
        return 1;
    }

    // Create a Python capsule object to pass the compiled_jq_state
    PyObject *pArgsJqLower = PyTuple_Pack(1, PyCapsule_New(&cjq_state, NULL, NULL));

    // Call the Python function with the compiled_jq_state argument
    PyObject *pResult = PyObject_CallObject(pFuncJqLower, pArgsJqLower);
    if (!pResult) {
        PyErr_Print();
        fprintf(stderr, "Call to function 'jq_lower' failed\n");
        return 1;
    }

    // Cleanup
    Py_DECREF(pResult);
    Py_DECREF(pArgsJqLower);
    Py_DECREF(pFuncJqLower);
    Py_DECREF(pModuleLowering);
    Py_DECREF(pModule_llvmlite);

    // Finalize Python
    Py_Finalize();

    // Return gracefully
    return 0;
}

int main(int argc, char *argv[]) {
    // Get parsed bytecode, pc, additional info from frontend_main
    compiled_jq_state cjq_state = frontend_main(argc, argv);

    // Generate LLVM IR
    int errorCode = call_jq_lower(&cjq_state);  // TODO: error handling

    // Free up resources
    cleanUp(&cjq_state);

    return 0;
}
