/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2019 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "atomic_test.h"

int main()
{
    MPL_atomic_write_barrier();
    MPL_atomic_read_barrier();
    MPL_atomic_read_write_barrier();
    return 0;
}
