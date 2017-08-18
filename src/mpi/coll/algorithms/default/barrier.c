/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2017 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

#include "mpiimpl.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_ENABLE_SMP_BARRIER
      category    : COLLECTIVE
      type        : boolean
      default     : true
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : Enable SMP aware barrier.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/


/* This is the default implementation of the barrier operation.  The
   algorithm is:

   Algorithm: MPI_Barrier

   We use the dissemination algorithm described in:
   Debra Hensgen, Raphael Finkel, and Udi Manbet, "Two Algorithms for
   Barrier Synchronization," International Journal of Parallel
   Programming, 17(1):1-17, 1988.

   It uses ceiling(lgp) steps. In step k, 0 <= k <= (ceiling(lgp)-1),
   process i sends to process (i + 2^k) % p and receives from process
   (i - 2^k + p) % p.

   Possible improvements:

   End Algorithm: MPI_Barrier

   This is an intracommunicator barrier only!
*/

#undef FUNCNAME
#define FUNCNAME barrier_smp_intra
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int barrier_smp_intra(MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;

    MPIR_Assert(MPIR_CVAR_ENABLE_SMP_COLLECTIVES && MPIR_CVAR_ENABLE_SMP_BARRIER &&
                MPIR_Comm_is_node_aware(comm_ptr));

    /* do the intranode barrier on all nodes */
    if (comm_ptr->node_comm != NULL) {
        mpi_errno = MPID_Barrier(comm_ptr->node_comm, errflag);
        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
            MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
        }
    }

    /* do the barrier across roots of all nodes */
    if (comm_ptr->node_roots_comm != NULL) {
        mpi_errno = MPID_Barrier(comm_ptr->node_roots_comm, errflag);
        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
            MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
        }
    }

    /* release the local processes on each node with a 1-byte
     * broadcast (0-byte broadcast just returns without doing
     * anything) */
    if (comm_ptr->node_comm != NULL) {
        int i = 0;
        mpi_errno = MPID_Bcast(&i, 1, MPI_BYTE, 0, comm_ptr->node_comm, errflag);
        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
            MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
        }
    }

    if (mpi_errno_ret)
        mpi_errno = mpi_errno_ret;
    else if (*errflag != MPIR_ERR_NONE)
        MPIR_ERR_SET(mpi_errno, *errflag, "**coll_fail");
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIC_DEFAULT_Barrier_recursive_doubling
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIC_DEFAULT_Barrier_recursive_doubling(MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag)
{
    int size, rank, src, dst, mask, mpi_errno=MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;

    size = comm_ptr->local_size;

    /* Trivial barriers return immediately */
    if (size == 1) goto fn_exit;

    rank = comm_ptr->rank;

    mask = 0x1;
    while (mask < size) {
        dst = (rank + mask) % size;
        src = (rank - mask + size) % size;
        mpi_errno = MPIC_Sendrecv(NULL, 0, MPI_BYTE, dst,
                                    MPIR_BARRIER_TAG, NULL, 0, MPI_BYTE,
                                    src, MPIR_BARRIER_TAG, comm_ptr,
                                    MPI_STATUS_IGNORE, errflag);
        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
            MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
       }
       mask <<= 1;

    }
fn_exit:
    if (mpi_errno_ret)
        mpi_errno = mpi_errno_ret;
    else if (*errflag != MPIR_ERR_NONE)
        MPIR_ERR_SET(mpi_errno, *errflag, "**coll_fail");
    return mpi_errno;
}

/* not declared static because it is called in ch3_comm_connect/accept */
#undef FUNCNAME
#define FUNCNAME MPIC_DEFAULT_Barrier_intra
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIC_DEFAULT_Barrier_intra(MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    int size, mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;

    size = comm_ptr->local_size;
    /* Trivial barriers return immediately */
    if (size == 1)
        goto fn_exit;

    if (MPIR_CVAR_ENABLE_SMP_COLLECTIVES && MPIR_CVAR_ENABLE_SMP_BARRIER &&
        MPIR_Comm_is_node_aware(comm_ptr)) {
        mpi_errno = barrier_smp_intra(comm_ptr, errflag);
        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
            MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
        }
        goto fn_exit;
    }

    mpi_errno = MPIC_DEFAULT_Barrier_recursive_doubling(comm_ptr, errflag);
    if (mpi_errno) {
        /* for communication errors, just record the error but continue */
        *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
        MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
        MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
    }

  fn_exit:
    if (mpi_errno_ret)
        mpi_errno = mpi_errno_ret;
    else if (*errflag != MPIR_ERR_NONE)
        MPIR_ERR_SET(mpi_errno, *errflag, "**coll_fail");
    return mpi_errno;
}

/* not declared static because a machine-specific function may call this one
   in some cases */
#undef FUNCNAME
#define FUNCNAME MPIC_DEFAULT_Barrier_inter
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIC_DEFAULT_Barrier_inter(MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    int rank, mpi_errno = MPI_SUCCESS, root;
    int mpi_errno_ret = MPI_SUCCESS;
    int i = 0;
    MPIR_Comm *newcomm_ptr = NULL;

    rank = comm_ptr->rank;

    /* Get the local intracommunicator */
    if (!comm_ptr->local_comm) {
        mpi_errno = MPII_Setup_intercomm_localcomm(comm_ptr);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    }

    newcomm_ptr = comm_ptr->local_comm;

    /* do a barrier on the local intracommunicator */
    mpi_errno = MPIR_Barrier(newcomm_ptr, errflag);
    if (mpi_errno) {
        /* for communication errors, just record the error but continue */
        *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
        MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
        MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
    }

    /* rank 0 on each group does an intercommunicator broadcast to the
     * remote group to indicate that all processes in the local group
     * have reached the barrier. We do a 1-byte bcast because a 0-byte
     * bcast will just return without doing anything. */

    /* first broadcast from left to right group, then from right to
     * left group */
    if (comm_ptr->is_low_group) {
        /* bcast to right */
        root = (rank == 0) ? MPI_ROOT : MPI_PROC_NULL;
        mpi_errno = MPIR_Bcast(&i, 1, MPI_BYTE, root, comm_ptr, errflag);
        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
            MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
        }

        /* receive bcast from right */
        root = 0;
        mpi_errno = MPIR_Bcast(&i, 1, MPI_BYTE, root, comm_ptr, errflag);
        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
            MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
        }
    } else {
        /* receive bcast from left */
        root = 0;
        mpi_errno = MPIR_Bcast(&i, 1, MPI_BYTE, root, comm_ptr, errflag);
        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
            MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
        }

        /* bcast to left */
        root = (rank == 0) ? MPI_ROOT : MPI_PROC_NULL;
        mpi_errno = MPIR_Bcast(&i, 1, MPI_BYTE, root, comm_ptr, errflag);
        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
            MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
        }
    }
  fn_exit:
    if (mpi_errno_ret)
        mpi_errno = mpi_errno_ret;
    else if (*errflag != MPIR_ERR_NONE)
        MPIR_ERR_SET(mpi_errno, *errflag, "**coll_fail");
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* MPIR_Barrier performs an barrier using point-to-point messages.
   This is intended to be used by device-specific implementations of
   barrier.  In all other cases MPIR_Barrier_impl should be used. */
#undef FUNCNAME
#define FUNCNAME MPIC_DEFAULT_Barrier
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIC_DEFAULT_Barrier(MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        /* intracommunicator */
        mpi_errno = MPIC_DEFAULT_Barrier_intra(comm_ptr, errflag);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    } else {
        /* intercommunicator */
        mpi_errno = MPIC_DEFAULT_Barrier_inter(comm_ptr, errflag);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FCNAME
#undef FUNCNAME
