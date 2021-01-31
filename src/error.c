#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "error.h"

static void
report_error(const char *call, const char *source, unsigned int line)
{
    int saved_errno = errno;

    char *func = strdup(call);
    if (!func) goto quit;

    char *p = strchr(func, '(');
    if (p) *p = '\0';

    fprintf(stderr, "%s(%s:%u) - %s\n", func, basename(source), line, strerror(saved_errno));

quit:
    exit(EXIT_FAILURE);
}

ssize_t check_retval(ssize_t retval, const char *call, const char *source, unsigned int line)
{
    if (retval < 0)
        report_error(call, source, line);

    return retval;
}

void *check_retval_p(void *retval, const char *call, const char *source, unsigned int line)
{
    if (!retval)
        report_error(call, source, line);

    return retval;
}
