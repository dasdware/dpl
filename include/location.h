#ifndef __LOCATION_H
#define __LOCATION_H

#include "nob.h"

typedef struct
{
    Nob_String_View file_name;

    size_t line;
    size_t column;

    const char *line_start;
} DPL_Location;

#define LOC_Fmt SV_Fmt ":%zu:%zu"
#define LOC_Arg(loc) SV_Arg(loc.file_name), (loc.line + 1), (loc.column + 1)

#endif // __LOCATION_H