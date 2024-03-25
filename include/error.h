#ifndef __DPL_ERROR_H
#define __DPL_ERROR_H

void dpl_error(const char *fmt, ...) __attribute__((noreturn));

#endif // __DPL_ERROR_H