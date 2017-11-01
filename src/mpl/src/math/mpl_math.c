/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2017 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

#include "mpl.h"

/* Calculates a^b */
int MPL_ipow(int a, int b)
{
    int i, n=1;
    for (i=0; i<b; i++) {
        n *= a;
    }

    return n;
}
