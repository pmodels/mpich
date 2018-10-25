/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2014 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef HCOLL_H_INCLUDED
#define HCOLL_H_INCLUDED

#include "hcoll/api/hcoll_api.h"
#ifndef HCOLL_VERSION
#define HCOLL_VERSION(major, minor) (((major)<<HCOLL_MAJOR_BIT)|((minor)<<HCOLL_MINOR_BIT))
#endif
#include "hcoll_dtypes.h"

extern int world_comm_destroying;

#if defined(MPL_USE_DBG_LOGGING)
extern MPL_dbg_class MPIR_DBG_HCOLL;
#endif /* MPL_USE_DBG_LOGGING */

int hcoll_initialize(void);

int hcoll_comm_create(MPIR_Comm * comm, void *param);
int hcoll_comm_destroy(MPIR_Comm * comm, void *param);

int hcoll_Barrier(MPIR_Comm * comm_ptr, MPIR_Errflag_t * err);
int hcoll_Bcast(void *buffer, int count, MPI_Datatype datatype, int root,
                MPIR_Comm * comm_ptr, MPIR_Errflag_t * err);
int hcoll_Allgather(const void *sbuf, int scount, MPI_Datatype sdtype,
                    void *rbuf, int rcount, MPI_Datatype rdtype, MPIR_Comm * comm_ptr,
                    MPIR_Errflag_t * err);
int hcoll_Allreduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op,
                    MPIR_Comm * comm_ptr, MPIR_Errflag_t * err);

int hcoll_do_progress(int *made_progress);

#endif /* HCOLL_H_INCLUDED */
