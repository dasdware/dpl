#ifndef __DPL_UTILS_H
#define __DPL_UTILS_H

#define _DPL_ARG_COUNT_MATCH(_1,_2,_3,_4,_5,_6,_7,_8,_9, N, ...) N
#define DPL_ARG_COUNT(...)  _DPL_ARG_COUNT_MATCH(__VA_ARGS__, 9,8,7,6,5,4,3,2,1)

#define DPL_ARGS(...) DPL_ARG_COUNT(__VA_ARGS__), (const char*[]){__VA_ARGS__}

#endif // __DPL_UTILS_H