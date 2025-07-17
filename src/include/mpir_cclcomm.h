/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPIR_CCLCOMM_H_INCLUDED
#define MPIR_CCLCOMM_H_INCLUDED

#ifdef ENABLE_CCLCOMM

#ifdef ENABLE_NCCL
#include <nccl.h>
typedef struct MPIR_NCCLcomm {
    ncclUniqueId id;
    ncclComm_t ncclcomm;
    cudaStream_t stream;
} MPIR_NCCLcomm;
#endif /*ENABLE_NCCL */

#ifdef ENABLE_RCCL
#include <rccl.h>
typedef struct MPIR_RCCLcomm {
    ncclUniqueId id;
    ncclComm_t rcclcomm;
    hipStream_t stream;
} MPIR_RCCLcomm;
#endif /*ENABLE_RCCL */

typedef struct MPIR_CCLcomm {
    MPIR_OBJECT_HEADER;
    MPIR_Comm *comm;
#ifdef ENABLE_NCCL
    MPIR_NCCLcomm *ncclcomm;
#endif                          /*ENABLE_NCCL */
#ifdef ENABLE_RCCL
    MPIR_RCCLcomm *rcclcomm;
#endif                          /*ENABLE_RCCL */
} MPIR_CCLcomm;

int MPIR_CCL_check_both_gpu_bufs(const void *sendbuf, void *recvbuf);
int MPIR_CCLcomm_init(MPIR_Comm * comm);
int MPIR_CCLcomm_free(MPIR_Comm * comm);

#ifdef ENABLE_NCCL
int MPIR_NCCL_check_requirements_red_op(const void *sendbuf, void *recvbuf, MPI_Datatype datatype,
                                        MPI_Op op);
int MPIR_NCCL_Allreduce(const void *sendbuf, void *recvbuf, MPI_Aint count, MPI_Datatype datatype,
                        MPI_Op op, MPIR_Comm * comm_ptr, int coll_attr);
int MPIR_NCCLcomm_free(MPIR_Comm * comm);
#endif /*ENABLE_NCCL */

#ifdef ENABLE_RCCL
int MPIR_RCCL_check_requirements_red_op(const void *sendbuf, void *recvbuf, MPI_Datatype datatype,
                                        MPI_Op op);
int MPIR_RCCL_Allreduce(const void *sendbuf, void *recvbuf, MPI_Aint count, MPI_Datatype datatype,
                        MPI_Op op, MPIR_Comm * comm_ptr, int coll_attr);
int MPIR_RCCLcomm_free(MPIR_Comm * comm);
#endif /*ENABLE_RCCL */

#endif /* ENABLE_CCLCOMM */

#endif /* MPIR_CCLCOMM_H_INCLUDED */
