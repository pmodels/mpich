/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include <stdarg.h>

char unit_last_error[1024];

void unit_set_error(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(unit_last_error, sizeof(unit_last_error), fmt, ap);
    va_end(ap);
}
