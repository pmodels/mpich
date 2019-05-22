/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2015 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpl.h"
#ifdef MPL_HAVE_BACKTRACE
#include <execinfo.h>

void MPL_backtrace_show(FILE * output)
{
#ifndef MPL_MAX_TRACE_DEPTH
#define MPL_MAX_TRACE_DEPTH 32
#endif
    void *trace[MPL_MAX_TRACE_DEPTH];
    char **stack_strs;
    char backtrace_buffer[MPL_BACKTRACE_BUFFER_LEN];
    int frames, i, ret, chars = 0;

    frames = backtrace(trace, MPL_MAX_TRACE_DEPTH);
    stack_strs = backtrace_symbols(trace, frames);

    for (i = 0; i < frames; i++) {
        ret = MPL_snprintf(backtrace_buffer + chars,
                           MPL_BACKTRACE_BUFFER_LEN - chars, "%s\n", stack_strs[i]);
        if (ret + chars >= MPL_BACKTRACE_BUFFER_LEN) {
            /* the extra new line will be more readable than a merely
             * truncated string */
            backtrace_buffer[MPL_BACKTRACE_BUFFER_LEN - 2] = '\n';
            backtrace_buffer[MPL_BACKTRACE_BUFFER_LEN - 1] = '\0';
            break;
        }
        chars += ret;
    }
    fprintf(output, "%s", backtrace_buffer);
    MPL_free(stack_strs);
}
#else
void MPL_backtrace_show(FILE * output)
{
    fprintf(output, "No backtrace info available\n");
}
#endif
