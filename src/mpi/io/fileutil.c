/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

#ifndef MPID_FILE_PREALLOC 
#define MPID_FILE_PREALLOC 8
#endif

/* Preallocated file objects */
MPID_File MPID_File_direct[MPID_FILE_PREALLOC] = { {0} };
MPIU_Object_alloc_t MPID_File_mem = { 0, 0, 0, 0, MPID_FILE, 
				      sizeof(MPID_File), MPID_File_direct,
                                      MPID_FILE_PREALLOC};

