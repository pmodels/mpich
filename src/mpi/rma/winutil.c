/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* FIXME: Move this into wincreate (having a separate file is unnecessary,
   and it leads to a false "missing coverage" report) */

/* Preallocated window objects */
MPIR_Win MPIR_Win_direct[MPIR_WIN_PREALLOC];

MPIR_Object_alloc_t MPIR_Win_mem = { 0, 0, 0, 0, 0, 0, MPIR_WIN,
    sizeof(MPIR_Win), MPIR_Win_direct,
    MPIR_WIN_PREALLOC,
    NULL, {0}
};
