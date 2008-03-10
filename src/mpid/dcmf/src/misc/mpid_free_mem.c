/*  (C)Copyright IBM Corp.  2007, 2008  */
/**
 * \file src/misc/mpid_free_mem.c
 * \brief ???
 */
/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

int MPID_Free_mem( void *ptr )
{
    int mpi_errno = MPI_SUCCESS;
    MPIU_Free(ptr);
    return mpi_errno;
}
