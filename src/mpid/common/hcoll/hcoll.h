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

extern int MPIDI_HCOLL_state_world_comm_destroying;

#if defined(MPL_USE_DBG_LOGGING)
extern MPL_dbg_class MPIR_DBG_HCOLL;
#endif /* MPL_USE_DBG_LOGGING */

int MPIDI_HCOLL_state_initialize(void);

int MPIDI_HCOLL_mpi_comm_create(MPIR_Comm * comm, void *param);
int MPIDI_HCOLL_mpi_comm_destroy(MPIR_Comm * comm, void *param);

int MPIDI_HCOLL_coll_barrier(MPIR_Comm * comm_ptr, MPIR_Errflag_t * err);
int MPIDI_HCOLL_coll_bcast(void *buffer, int count, MPI_Datatype datatype, int root,
                           MPIR_Comm * comm_ptr, MPIR_Errflag_t * err);
int MPIDI_HCOLL_coll_reduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
                            MPI_Op op, int root, MPIR_Comm * comm_ptr, MPIR_Errflag_t * err);
int MPIDI_HCOLL_coll_allgather(const void *sbuf, int scount, MPI_Datatype sdtype, void *rbuf,
                               int rcount, MPI_Datatype rdtype, MPIR_Comm * comm_ptr,
                               MPIR_Errflag_t * err);
int MPIDI_HCOLL_coll_allreduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
                               MPI_Op op, MPIR_Comm * comm_ptr, MPIR_Errflag_t * err);
int MPIDI_HCOLL_coll_alltoall(const void *sbuf, int scount, MPI_Datatype sdtype, void *rbuf,
                              int rcount, MPI_Datatype rdtype, MPIR_Comm * comm_ptr,
                              MPIR_Errflag_t * err);
int MPIDI_HCOLL_coll_alltoallv(const void *sbuf, const int *scounts, const int *sdispls,
                               MPI_Datatype sdtype, void *rbuf, const int *rcounts,
                               const int *rdispls, MPI_Datatype rdtype, MPIR_Comm * comm_ptr,
                               MPIR_Errflag_t * err);

int MPIDI_HCOLL_state_progress(int *made_progress);

#endif /* HCOLL_H_INCLUDED */
