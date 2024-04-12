#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "clib/lib.h"

extern void jqfunc(char* c);

bool isJSONFile(const char *filename) {
    const char *ext = strrchr(filename, '.');
    return (ext && strcmp(ext, ".json") == 0);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <filename1> [<filename2> ...]\n", argv[0]);
        return 1;
    }

    size_t dataSize = 1;
    char *inputData = (char*)malloc(dataSize);
    if (inputData == NULL) {
        fprintf(stderr, "Failed to allocate memory\n");
        return 1;
    }
    inputData[0] = '\0'; // Null-terminate the string

    for (int i = 1; i < argc; i++) {
        char *filename = argv[i];

        if (!isJSONFile(filename)) {
            continue;
        }

        char *jsonData = readJSON(filename);
        if (jsonData == NULL) {
            printf("Failed to read data from file: %s\n", filename);
            continue;
        }

        size_t jsonData_len = strlen(jsonData);
        size_t newSize = dataSize + jsonData_len;

        char *temp = (char*)realloc(inputData, newSize + 1);
        if (temp == NULL) {
            fprintf(stderr, "Failed to reallocate memory\n");
            free(inputData);
            free(jsonData);
            return 1;
        }
        inputData = temp;

        strcat(inputData, jsonData);
        free(jsonData); // Free jsonData here

        dataSize = newSize;
    }

    jqfunc(inputData); // Pass data to jqfunc
    free(inputData); // Free data at the end

    return 0;
}
