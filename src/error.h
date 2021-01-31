#pragma once

#include <stdio.h>

ssize_t check_retval(ssize_t retval, const char *call, const char *source, unsigned int line);

void *check_retval_p(void *retval, const char *call, const char *source, unsigned int line);

/*
 * Check the return value of a posix API (int version)
 */
#define CHK(expr) check_retval(expr, #expr, __FILE__, __LINE__)

/*
 * Check the return value of a posix API (pointer version)
 */
#define CHK_P(expr) check_retval_p(expr, #expr, __FILE__, __LINE__)
