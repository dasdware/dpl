#include "error.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

void dpl_error(const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);

    exit(1);
}