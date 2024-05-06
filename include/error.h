#ifndef __DW_ERROR_H
#define __DW_ERROR_H

#include <stdio.h>

#ifndef DW_ERROR_STREAM
#define DW_ERROR_STREAM stderr
#endif

#ifndef DW_ERROR_EXIT_CODE
#define DW_ERROR_EXIT_CODE 1
#endif

#define DW_ERROR_MSG(format, ...)                        \
    do                                                   \
    {                                                    \
        fprintf(DW_ERROR_STREAM, format, ##__VA_ARGS__); \
    } while (false)

#define DW_ERROR_MSGLN(format, ...)                      \
    do                                                   \
    {                                                    \
        fprintf(DW_ERROR_STREAM, format, ##__VA_ARGS__); \
        fprintf(DW_ERROR_STREAM, "\n");                  \
    } while (false)

#define DW_ERROR(format, ...)                            \
    do                                                   \
    {                                                    \
        DW_ERROR_MSGLN(format, ##__VA_ARGS__);           \
        exit(DW_ERROR_EXIT_CODE);                        \
    } while (false)

#define DW_UNIMPLEMENTED                                 \
    DW_ERROR("%s:%d: UNIMPLEMENTED (function %s)", __FILE__, __LINE__, __FUNCTION__)

#define DW_UNIMPLEMENTED_MSG(format, args)               \
    DW_ERROR("%s:%d: UNIMPLEMENTED (function %s): " format, __FILE__, __LINE__, __FUNCTION__, args)


#endif // __DW_ERROR_H