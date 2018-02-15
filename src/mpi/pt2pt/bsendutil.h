/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef BSENDUTIL_H_INCLUDED
#define BSENDUTIL_H_INCLUDED

#include "mpii_bsend.h"

/* Function Prototypes for the bsend utility functions */
int MPIR_Bsend_attach(void *, int);
int MPIR_Bsend_detach(void *, int *);
int MPIR_Bsend_isend(const void *, int, MPI_Datatype, int, int, MPIR_Comm *,
                     MPII_Bsend_kind_t, MPIR_Request **);
int MPIR_Bsend_free_req_seg(MPIR_Request *);

#endif /* BSENDUTIL_H_INCLUDED */
