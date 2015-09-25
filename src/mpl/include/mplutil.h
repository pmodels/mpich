/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2015 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */


#ifndef MPLUTIL_H_INCLUDED
#define MPLUTIL_H_INCLUDED

#include "mplconfig.h"
#if defined(__cplusplus)
extern "C" {
#endif

#define MPL_BACKTRACE_BUFFER_LEN 1024
#define MPL_MAX_TRACE_DEPTH 32
void MPL_backtrace_show(FILE *output);

#if defined(__cplusplus)

}
#endif

#endif
