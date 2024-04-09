#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "clib/lib.h"

extern int jqfunc();

bool is_json_file(const char *filename) {
    const char *ext = strrchr(filename, '.');
    return (ext && strcmp(ext, ".json") == 0);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <filename1> [<filename2> ...]\n", argv[0]);
        return 1;
    }

    size_t data_size = 1;
    char *data = (char*)malloc(data_size);
    if (data == NULL) {
        fprintf(stderr, "Failed to allocate memory\n");
        return 1;
    }
    data[0] = '\0'; // Null-terminate the string

    for (int i = 1; i < argc; i++) {
        char *filename = argv[i];

        if (!is_json_file(filename)) {
            // printf("Skipping non-JSON file: %s\n", filename);
            continue;
        }

        char *jsonData = readJSON(filename);
        if (jsonData == NULL) {
            printf("Failed to read data from file: %s\n", filename);
            continue;
        }

        size_t jsonData_len = strlen(jsonData);
        size_t new_size = data_size + jsonData_len;

        char *temp = (char*)realloc(data, new_size + 1);
        if (temp == NULL) {
            fprintf(stderr, "Failed to reallocate memory\n");
            free(data);
            free(jsonData);
            return 1;
        }
        data = temp;

        strcat(data, jsonData);
        free(jsonData); // Free jsonData here

        data_size = new_size;
    }

    printf("DEBUG OUTPUT:\nConcatenated data:\n%s\n", data);  // Debug
    jqfunc(); // Now that JSON data has been read, execute JQ program

    free(data); // Free data at the end

    return 0;
}
