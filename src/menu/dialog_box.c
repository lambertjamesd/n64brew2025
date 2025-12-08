#include "dialog_box.h" 

#include <stdio.h>

void dialog_box_format_string(char* into, char* format, int* args) {
    while (*format) {
        if (*format == '%') {
            format += 1;

            int arg = *args;
            args += 1;

            if (*format == 's') {
                char* as_str = (char*)arg;
                while (*as_str) *into++ = *as_str++;
            } else if (*format == 'd') {
                into += sprintf(into, "%d", arg);
            } else if (*format == '%') {
                *into++ = '%';
            }
        } else {
            *into++ = *format;
        }

        format += 1;
    }

    *into++ = '\0';
}