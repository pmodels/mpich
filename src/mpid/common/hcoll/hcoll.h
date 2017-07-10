/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2014 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef _HCOLL_H_
#define _HCOLL_H_

#include "mpidimpl.h"
#include "hcoll/api/hcoll_api.h"
#include "hcoll/api/hcoll_constants.h"

extern int world_comm_destroying;

#if defined(MPL_USE_DBG_LOGGING)
extern MPL_dbg_class MPIR_DBG_HCOLL;
#endif /* MPL_USE_DBG_LOGGING */

int hcoll_comm_create(MPIR_Comm * comm, void *param);
int hcoll_comm_destroy(MPIR_Comm * comm, void *param);

int hcoll_Barrier(MPIR_Comm * comm_ptr, MPIR_Errflag_t *err);
int hcoll_Bcast(void *buffer, int count, MPI_Datatype datatype, int root,
                MPIR_Comm * comm_ptr, MPIR_Errflag_t *err);
int hcoll_Allgather(const void *sbuf, int scount, MPI_Datatype sdtype,
                    void *rbuf, int rcount, MPI_Datatype rdtype, MPIR_Comm * comm_ptr, MPIR_Errflag_t *err);
int hcoll_Allreduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
                    MPI_Op op, MPIR_Comm * comm_ptr, MPIR_Errflag_t *err);

#if 0
int hcoll_Ibarrier_req(MPIR_Comm * comm_ptr, MPIR_Request ** request);
int hcoll_Ibcast_req(void *buffer, int count, MPI_Datatype datatype, int root,
                     MPIR_Comm * comm_ptr, MPIR_Request ** request);
int hcoll_Iallgather_req(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                         int recvcount, MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                         MPIR_Request ** request);
int hcoll_Iallreduce_req(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
                         MPI_Op op, MPIR_Comm * comm_ptr, MPIR_Request ** request);
#endif

int hcoll_do_progress(int *made_progress);

#endif
