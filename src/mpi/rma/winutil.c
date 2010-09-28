/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* FIXME: Move this into wincreate (having a separate file is unneccessary,
   and it leads to a false "missing coverage" report) */
#ifndef MPID_WIN_PREALLOC 
#define MPID_WIN_PREALLOC 8
#endif

/* Preallocated window objects */
MPID_Win MPID_Win_direct[MPID_WIN_PREALLOC] = { {0} };
MPIU_Object_alloc_t MPID_Win_mem = { 0, 0, 0, 0, MPID_WIN, 
				      sizeof(MPID_Win), MPID_Win_direct,
                                      MPID_WIN_PREALLOC};
