/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/*
 * This "algorithm" is the generic wrapper for
 * using a CCL (e.g., NCCL, RCCL etc.) to 
 * complete the collective operation.
 */
int MPIR_Allreduce_intra_ccl(const void *sendbuf, void *recvbuf, MPI_Aint count, MPI_Datatype datatype,
                        MPI_Op op, MPIR_Comm * comm_ptr, int ccl, MPIR_Errflag_t errflag)
{
    switch(ccl) {
        #ifdef ENABLE_NCCL
        case MPIR_CVAR_ALLREDUCE_CCL_auto: // Not sure yet how to handle "auto"
        case MPIR_CVAR_ALLREDUCE_CCL_nccl:
            if (MPIR_NCCL_check_requirements_red_op(sendbuf, recvbuf, datatype, op)) {
                return MPIR_NCCL_Allreduce(sendbuf, recvbuf, count, datatype, op, comm_ptr, errflag);
            }
        #endif
        default:
            goto fallback;
    }

  fallback:
    return MPIR_Allreduce_allcomm_auto(sendbuf, recvbuf, count, datatype,
                                op, comm_ptr, errflag);
}
