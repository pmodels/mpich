/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPIR_CCLCOMM_H_INCLUDED
#define MPIR_CCLCOMM_H_INCLUDED

#include <nccl.h>

#define ENABLE_CCLCOMM 1 //Temporary, needs to get put in configure

#ifdef ENABLE_CCLCOMM
    typedef struct MPIR_CCLcomm {
        MPIR_OBJECT_HEADER;
        MPIR_Comm *comm;
        ncclUniqueId id;
        ncclComm_t ncclcomm;
        cudaStream_t stream;
    } MPIR_CCLcomm;

    int MPIR_CCL_red_op_is_supported(MPI_Op op);

    int MPIR_CCL_datatype_is_supported(MPI_Datatype datatype);
#endif /* ENABLE_CCLCOMM */

#endif /* MPIR_CCLCOMM_H_INCLUDED */