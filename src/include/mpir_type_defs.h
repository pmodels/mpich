/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#if !defined(MPIR_TYPE_DEFS_H_INCLUDED)
#define MPIR_TYPE_DEFS_H_INCLUDED

#include "mpichconf.h"

/* Define a typedef for the errflag value used by many internal functions.
 * If an error needs to be returned, these values can be used to signal such.
 * More details can be found further down in the code with the bitmasking logic */
typedef enum {
    MPIR_ERR_NONE = MPI_SUCCESS,
    MPIR_ERR_PROC_FAILED = MPIX_ERR_PROC_FAILED,
    MPIR_ERR_OTHER = MPI_ERR_OTHER
} MPIR_Errflag_t;

#endif /* !defined(MPIR_TYPE_DEFS_H_INCLUDED) */
