#ifndef __UTIL_FILE_H__
#define __UTIL_FILE_H__

#include <stdio.h>

// caller is responsible for freeing result
char* file_read_string(FILE* file);

#endif