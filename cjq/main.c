#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "clib/lib.h"

extern char* data;
extern char x;
extern char* y;
extern void jqfunc(char* c);

bool is_json_file(const char *filename) {
    const char *ext = strrchr(filename, '.');
    return (ext && strcmp(ext, ".json") == 0);
}

int main(int argc, char *argv[]) {
    printf("BEFORE readJSON():\ndata: %s\n", data);  // Debug
    if (argc < 2) {
        printf("Usage: %s <filename1> [<filename2> ...]\n", argv[0]);
        return 1;
    }

    size_t data_size = 1;
    char *input_data = (char*)malloc(data_size);
    if (input_data == NULL) {
        fprintf(stderr, "Failed to allocate memory\n");
        return 1;
    }
    input_data[0] = '\0'; // Null-terminate the string

    for (int i = 1; i < argc; i++) {
        char *filename = argv[i];

        if (!is_json_file(filename)) {
            continue;
        }

        char *jsonData = readJSON(filename);
        if (jsonData == NULL) {
            printf("Failed to read data from file: %s\n", filename);
            continue;
        }

        size_t jsonData_len = strlen(jsonData);
        size_t new_size = data_size + jsonData_len;

        char *temp = (char*)realloc(input_data, new_size + 1);
        if (temp == NULL) {
            fprintf(stderr, "Failed to reallocate memory\n");
            free(input_data);
            free(jsonData);
            return 1;
        }
        input_data = temp;

        strcat(input_data, jsonData);
        free(jsonData); // Free jsonData here

        data_size = new_size;
    }

    printf("AFTER readJSON():\ndata: %s\n", input_data);  // Debug
    printf("Address of input_data: %i\n",&input_data[0]);
    jqfunc(input_data); // Pass data to jqfunc
    free(input_data); // Free data at the end

    return 0;
}
