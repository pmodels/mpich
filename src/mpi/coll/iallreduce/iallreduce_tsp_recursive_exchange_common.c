/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "iallreduce_tsp_recursive_exchange_common.h"

/* Routine to schedule a recursive exchange based allreduce */
/*
    MPIR_TSP_Iallreduce_sched_intra_recexch_step1 - non participating ranks
    send their data to participating ranks for allreduce in step2.

Input Parameters:
+ sendbuf - initial address of send buffer (choice)
. recvbuf - initial address of recv buffer (choice)
. count - number of elements in send buffer/recv buffer (if sendbuf is MPI_INPLACE) (nonnegative integer)
. datatype - datatype of each buffer element (handle)
. op - Type of allreduce operation (handle)
. tag - message tag (integer)
. extent - extent of datatype (integer)
. dtcopy_id - id of previous data copy from scheduler (integer)
. recv_id - array to store ids of the receives from the scheduler (integer array)
. reduce_id - array to store ids of the reduces from the scheduler (integer array)
. vtcs - array to specify dependencies of a call to the scheduler (integer array)
. step1_sendto - partner to send my data in step1 (integer)
. in_step2 - variable to check if I participate in step2 or not (bool)
. step1_nrecvs - number of partners to receive data from in step1 (integer)
. step1_recvfrom - partners to receive data from in step1 (integer array)
. per_nbr_buffer - variable to denote whether different receive buffers are
                    used for each partner and in each phase (integer)
. step1_recvbuf - address of array of receive buffers used in step1 to receive data from partners.
                  to receive data from in step1. Memory is allocated in this function and
                  address is returned (address of array of buffers)
. comm - communicator (handle)
- sched - scheduler (handle)

*/
int MPIR_TSP_Iallreduce_sched_intra_recexch_step1(const void *sendbuf,
                                                  void *recvbuf,
                                                  MPI_Aint count,
                                                  MPI_Datatype datatype,
                                                  MPI_Op op, int is_commutative, int tag,
                                                  MPI_Aint extent, int dtcopy_id, int *recv_id,
                                                  int *reduce_id, int *vtcs, int is_inplace,
                                                  int step1_sendto, bool in_step2, int step1_nrecvs,
                                                  int *step1_recvfrom, int per_nbr_buffer,
                                                  void ***step1_recvbuf_, MPIR_Comm * comm,
                                                  MPIR_TSP_sched_t sched)
{
    int mpi_errno = MPI_SUCCESS;
    int i, nvtcs, vtx_id;
    int mpi_errno_ret = MPI_SUCCESS;
    void **step1_recvbuf;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;

    MPIR_FUNC_ENTER;
    /* Step 1 */
    if (!in_step2) {
        /* non-participating rank sends the data to a participating rank */
        const void *buf_to_send;
        if (is_inplace)
            buf_to_send = (const void *) recvbuf;
        else
            buf_to_send = sendbuf;
        mpi_errno =
            MPIR_TSP_sched_isend(buf_to_send, count, datatype, step1_sendto, tag, comm, sched, 0,
                                 NULL, &vtx_id);
        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            errflag = MPIR_ERR_OTHER;
            MPIR_ERR_SET(mpi_errno, errflag, "**fail");
            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
        }
    } else {    /* Step 2 participating rank */
        step1_recvbuf = *step1_recvbuf_ =
            (void **) MPL_malloc(sizeof(void *) * step1_nrecvs, MPL_MEM_COLL);
        if (per_nbr_buffer != 1 && step1_nrecvs > 0) {
            step1_recvbuf[0] = (*step1_recvbuf_)[0] = MPIR_TSP_sched_malloc(count * extent, sched);
            MPIR_Assert(step1_recvbuf != NULL);
        }

        for (i = 0; i < step1_nrecvs; i++) {    /* participating rank gets data from non-partcipating ranks */
            if (per_nbr_buffer == 1)
                step1_recvbuf[i] = (*step1_recvbuf_)[i] =
                    MPIR_TSP_sched_malloc(count * extent, sched);
            else
                step1_recvbuf[i] = (*step1_recvbuf_)[i] = (*step1_recvbuf_)[0];

            /* recv dependencies */
            nvtcs = 0;
            if (i != 0 && per_nbr_buffer == 0 && count != 0) {
                vtcs[nvtcs++] = reduce_id[i - 1];
                MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                                (MPL_DBG_FDEST, "step1 recv depend on reduce_id[%d] %d", i - 1,
                                 reduce_id[i - 1]));
            }
            mpi_errno = MPIR_TSP_sched_irecv(step1_recvbuf[i], count, datatype,
                                             step1_recvfrom[i], tag, comm, sched, nvtcs, vtcs,
                                             &recv_id[i]);
            MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);
            if (count != 0) {   /* Reduce only if data is present */
                /* setup reduce dependencies */
                nvtcs = 1;
                vtcs[0] = recv_id[i];
                if (is_commutative) {
                    if (!is_inplace) {  /* wait for the data to be copied to recvbuf */
                        vtcs[nvtcs++] = dtcopy_id;
                    }
                } else {        /* if not commutative */
                    /* wait for datacopy to complete if i==0 && !is_inplace else wait for previous reduce */
                    if (i == 0 && !is_inplace)
                        vtcs[nvtcs++] = dtcopy_id;
                    else if (i != 0)
                        vtcs[nvtcs++] = reduce_id[i - 1];
                }
                mpi_errno = MPIR_TSP_sched_reduce_local(step1_recvbuf[i], recvbuf, count,
                                                        datatype, op, sched, nvtcs, vtcs,
                                                        &reduce_id[i]);
                MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);
            }
        }
    }
    MPIR_FUNC_EXIT;
    return mpi_errno;
}
