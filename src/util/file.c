#include "file.h"

#include <stdint.h>
#include <malloc.h>

char* file_read_string(FILE* file) {
    uint8_t len;
    fread(&len, 1, 1, file);
    char* result = malloc(len + 1);
    fread(result, len, 1, file);
    result[len] = '\0';
    return result;
}