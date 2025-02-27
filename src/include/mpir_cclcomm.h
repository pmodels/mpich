/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPIR_CCLCOMM_H_INCLUDED
#define MPIR_CCLCOMM_H_INCLUDED



#define ENABLE_CCLCOMM 1 // Temporary, needs to get put in configure
#define ENABLE_NCCL 1 // Temporary, needs to get put in configure

#ifdef ENABLE_CCLCOMM

#include <nccl.h>

#ifdef ENABLE_NCCL
typedef struct MPIR_NCCLcomm {
    ncclUniqueId id;
    ncclComm_t ncclcomm;
    cudaStream_t stream;
} MPIR_NCCLcomm;
#endif /*ENABLE_NCCL */

typedef struct MPIR_CCLcomm {
    MPIR_OBJECT_HEADER;
    MPIR_Comm *comm;
    #ifdef ENABLE_NCCL
    MPIR_NCCLcomm *ncclcomm;
    #endif /*ENABLE_NCCL */
} MPIR_CCLcomm;

int MPIR_CCL_check_both_gpu_bufs(const void *sendbuf, void *recvbuf);

int get_ccl_from_string(const char *ccl_str);

int MPIR_CCLcomm_init(MPIR_Comm * comm);

int MPIR_CCLcomm_free(MPIR_Comm * comm);

#ifdef ENABLE_NCCL
int MPIR_NCCL_check_requirements_red_op(const void *sendbuf, void *recvbuf, MPI_Datatype datatype, 
                        MPI_Op op);
int MPIR_NCCL_Allreduce(const void *sendbuf, void *recvbuf, MPI_Aint count, MPI_Datatype datatype,
                        MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Errflag_t errflag);
int MPIR_NCCLcomm_free(MPIR_Comm *comm);
#endif /*ENABLE_NCCL */

#endif /* ENABLE_CCLCOMM */

#endif /* MPIR_CCLCOMM_H_INCLUDED */