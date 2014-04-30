/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2011 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

static int get_parent(int rank, uint32_t *bitarray)
{
    int parent = -1;
    int failed = 1;
    uint32_t mask;

    while (failed) {
        mask = 0x80000000;
        parent = rank == 0 ? -1: (rank - 1) / 2;

        /* Check whether the process is in the failed group */
        if (parent != -1) {
            mask >>= parent % (sizeof(uint32_t) * 8);
            failed = bitarray[parent / (sizeof(uint32_t) * 8)] & mask;
            if (failed) {
                rank = parent;
            }
        } else failed = 0;
    }

    return parent;
}

static void get_children(int rank, int size, uint32_t *bitarray, int *children, int *nchildren)
{
    int i;
    int child;

    for (i = 1; i <= 2; i++) {
        /* Calculate the child */
        child = 2 * rank + i;
        if (child >= size) child = -1;

        /* Check if the child is alive. If not, call get_children on the child
         * to inherit its children */
        if (child != -1) {
            if (bitarray[child / (sizeof(uint32_t) * 8)] & (0x80000000 >> (child % (sizeof(uint32_t) * 8)))) {
                get_children(child, size, bitarray, children, nchildren);
            } else {
                children[*nchildren] = child;
                (*nchildren)++;
            }
        }
    }
}

#undef FUNCNAME
#define FUNCNAME MPID_Comm_agree
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPID_Comm_agree(MPID_Comm *comm_ptr, uint32_t *bitarray, int *flag, int new_fail)
{
    int mpi_errno = MPI_SUCCESS;
    int *children, nchildren = 0, parent;
    int i;
    int errflag = new_fail;
    int tmp_flag;

    MPID_MPI_STATE_DECL(MPID_STATE_MPID_COMM_AGREE);
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPID_COMM_AGREE);

    children = (int *) MPIU_Malloc(sizeof(int) * ((comm_ptr->local_size) / 2));

    /* Calculate my parent and children */
    parent = get_parent(comm_ptr->rank, bitarray);
    get_children(comm_ptr->rank, comm_ptr->local_size, bitarray, children, &nchildren);

    /* Get a flag value from each of my children */
    for (i = 0; i < nchildren; i++) {
        if (children[i] == -1) continue;
        mpi_errno = MPIC_Recv(&tmp_flag, 1, MPI_INT, children[i], MPIR_AGREE_TAG,
                comm_ptr->handle, MPI_STATUS_IGNORE, &errflag);
        if (mpi_errno) return mpi_errno;
        if (errflag) new_fail = 1;

        *flag &= tmp_flag;
    }

    /* If I'm not the root */
    if (-1 != parent) {
        /* Send my message to my parent */
        mpi_errno = MPIC_Send(flag, 1, MPI_INT, parent, MPIR_AGREE_TAG,
                comm_ptr->handle, &errflag);
        if (mpi_errno) return mpi_errno;

        /* Receive the result from my parent */
        mpi_errno = MPIC_Recv(flag, 1, MPI_INT, parent, MPIR_AGREE_TAG,
                comm_ptr->handle, MPI_STATUS_IGNORE, &errflag);
        if (mpi_errno) return mpi_errno;
        if (errflag) new_fail = 1;
    }

    /* Send my flag value to my children */
    for (i = 0; i < nchildren; i++) {
        if (children[i] == -1) continue;
        mpi_errno = MPIC_Send(flag, 1, MPI_INT, children[i], MPIR_AGREE_TAG,
                comm_ptr->handle, &errflag);
        if (mpi_errno) return mpi_errno;
    }

    MPIU_DBG_MSG_D(CH3_OTHER, VERBOSE, "New failure: %d", new_fail);

    MPIU_ERR_CHKANDJUMP1(new_fail, mpi_errno, MPIX_ERR_PROC_FAILED, "**mpix_comm_agree", "**mpix_comm_agree %C", comm_ptr);

    MPIU_Free(children);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
