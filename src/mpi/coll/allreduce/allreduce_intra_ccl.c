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

/*
 * TODO: Decide how to handle MPIR_CVAR_ALLREDUCE_CCL_auto when multiple CCL backends are enabled.
 * We cannot support two separate CCL_auto cases if both NCCL and RCCL are built into MPICH.
 */

int MPIR_Allreduce_intra_ccl(const void *sendbuf, void *recvbuf, MPI_Aint count,
                             MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr, int ccl,
                             int coll_attr)
{
    switch (ccl) {
#ifdef ENABLE_NCCL
        case MPIR_CVAR_ALLREDUCE_CCL_auto:     // Not sure how to handle auto case yet
        case MPIR_CVAR_ALLREDUCE_CCL_nccl:
            if (MPIR_NCCL_check_requirements_red_op(sendbuf, recvbuf, datatype, op)) {
                return MPIR_NCCL_Allreduce(sendbuf, recvbuf, count, datatype, op, comm_ptr,
                                           coll_attr);
            }
            break;
#endif

#ifdef ENABLE_RCCL
        case MPIR_CVAR_ALLREDUCE_CCL_auto:     // Not sure how to handle auto case yet
        case MPIR_CVAR_ALLREDUCE_CCL_rccl:
            if (MPIR_RCCL_check_requirements_red_op(sendbuf, recvbuf, datatype, op)) {
                return MPIR_RCCL_Allreduce(sendbuf, recvbuf, count, datatype, op, comm_ptr,
                                           coll_attr);
            }
            break;
#endif

        default:
            goto fallback;

    }
  fallback:
    /* FIXME: proper error */
    return MPI_ERR_OTHER;
}
