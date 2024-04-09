#include <stdio.h>
#include <stdlib.h> // For memory allocation functions
#include <json-c/json.h>
#include <memory.h>

char* readJSON(const char* filename) {
    FILE *fp;
    char buffer[1024];
    struct json_object *parsed_json;

    fp = fopen(filename, "r");
    if (fp == NULL) {
        fprintf(stderr, "Failed to open file: %s\n", filename);
        return NULL; // Return NULL if failed to open file
    }

    fread(buffer, sizeof(buffer), 1, fp);
    fclose(fp);

    parsed_json = json_tokener_parse(buffer);
    if (parsed_json == NULL) {
        fprintf(stderr, "Failed to parse JSON from file: %s\n", filename);
        return NULL; // Return NULL if failed to parse JSON
    }

    // Convert parsed JSON object to string
    const char *jsonDataStr = json_object_to_json_string(parsed_json);
    size_t jsonDataLen = strlen(jsonDataStr);

    // Allocate memory for storing JSON data string (including null terminator)
    char *jsonData = (char*)malloc(jsonDataLen + 1);
    if (jsonData == NULL) {
        fprintf(stderr, "Failed to allocate memory\n");
        json_object_put(parsed_json); // Release parsed JSON object
        return NULL; // Return NULL if failed to allocate memory
    }

    // Copy JSON data string to the allocated memory
    memcpy(jsonData, jsonDataStr, jsonDataLen + 1);

    // Release parsed JSON object
    json_object_put(parsed_json);

    return jsonData;
}
