/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2017 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

#ifndef COLLRECEXCH_UTIL_H_INCLUDED
#define COLLRECEXCH_UTIL_H_INCLUDED

/*
This is a simple function to compare two integers.
It is used for sorting list of ranks.
*/
static inline
int intcmpfn(const void *a, const void *b)
{
    return (*(int *)a - *(int *)b);
}

#endif /* COLLRECEXCH_UTIL_H_INCLUDED */
