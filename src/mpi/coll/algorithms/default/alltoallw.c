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

/* This is the default implementation of alltoallw. The algorithm is:

   Algorithm: MPI_Alltoallw

   Since each process sends/receives different amounts of data to
   every other process, we don't know the total message size for all
   processes without additional communication. Therefore we simply use
   the "middle of the road" isend/irecv algorithm that works
   reasonably well in all cases.

   We post all irecvs and isends and then do a waitall. We scatter the
   order of sources and destinations among the processes, so that all
   processes don't try to send/recv to/from the same process at the
   same time.

   *** Modification: We post only a small number of isends and irecvs
   at a time and wait on them as suggested by Tony Ladd. ***

   Possible improvements:

   End Algorithm: MPI_Alltoallw
*/

/* not declared static because a machine-specific function may call this one in some cases */
#undef FUNCNAME
#define FUNCNAME MPIC_DEFAULT_Alltoallw_intra
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIC_DEFAULT_Alltoallw_intra(const void *sendbuf, const int sendcounts[],
                                 const int sdispls[], const MPI_Datatype sendtypes[],
                                 void *recvbuf, const int recvcounts[],
                                 const int rdispls[], const MPI_Datatype recvtypes[],
                                 MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    int comm_size, i, j;
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    MPI_Status status;
    MPI_Status *starray;
    MPIR_Request **reqarray;
    int dst, rank;
    int outstanding_requests;
    int ii, ss, bblock;
    int type_size;
    MPIR_CHKLMEM_DECL(2);

    comm_size = comm_ptr->local_size;
    rank = comm_ptr->rank;

    if (sendbuf == MPI_IN_PLACE) {
        /* We use pair-wise sendrecv_replace in order to conserve memory usage,
         * which is keeping with the spirit of the MPI-2.2 Standard.  But
         * because of this approach all processes must agree on the global
         * schedule of sendrecv_replace operations to avoid deadlock.
         *
         * Note that this is not an especially efficient algorithm in terms of
         * time and there will be multiple repeated malloc/free's rather than
         * maintaining a single buffer across the whole loop.  Something like
         * MADRE is probably the best solution for the MPI_IN_PLACE scenario. */
        for (i = 0; i < comm_size; ++i) {
            /* start inner loop at i to avoid re-exchanging data */
            for (j = i; j < comm_size; ++j) {
                if (rank == i) {
                    /* also covers the (rank == i && rank == j) case */
                    mpi_errno = MPIC_Sendrecv_replace(((char *) recvbuf + rdispls[j]),
                                                      recvcounts[j], recvtypes[j],
                                                      j, MPIR_ALLTOALLW_TAG,
                                                      j, MPIR_ALLTOALLW_TAG,
                                                      comm_ptr, &status, errflag);
                    if (mpi_errno) {
                        /* for communication errors, just record the error but continue */
                        *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                        MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                        MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                    }
                } else if (rank == j) {
                    /* same as above with i/j args reversed */
                    mpi_errno = MPIC_Sendrecv_replace(((char *) recvbuf + rdispls[i]),
                                                      recvcounts[i], recvtypes[i],
                                                      i, MPIR_ALLTOALLW_TAG,
                                                      i, MPIR_ALLTOALLW_TAG,
                                                      comm_ptr, &status, errflag);
                    if (mpi_errno) {
                        /* for communication errors, just record the error but continue */
                        *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                        MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                        MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                    }
                }
            }
        }
    } else {
        bblock = MPIR_CVAR_ALLTOALL_THROTTLE;
        if (bblock == 0)
            bblock = comm_size;

        MPIR_CHKLMEM_MALLOC(starray, MPI_Status *, 2 * bblock * sizeof(MPI_Status), mpi_errno,
                            "starray");
        MPIR_CHKLMEM_MALLOC(reqarray, MPIR_Request **, 2 * bblock * sizeof(MPIR_Request *),
                            mpi_errno, "reqarray");

        /* post only bblock isends/irecvs at a time as suggested by Tony Ladd */
        for (ii = 0; ii < comm_size; ii += bblock) {
            outstanding_requests = 0;
            ss = comm_size - ii < bblock ? comm_size - ii : bblock;

            /* do the communication -- post ss sends and receives: */
            for (i = 0; i < ss; i++) {
                dst = (rank + i + ii) % comm_size;
                if (recvcounts[dst]) {
                    MPIR_Datatype_get_size_macro(recvtypes[dst], type_size);
                    if (type_size) {
                        mpi_errno = MPIC_Irecv((char *) recvbuf + rdispls[dst],
                                               recvcounts[dst], recvtypes[dst], dst,
                                               MPIR_ALLTOALLW_TAG, comm_ptr,
                                               &reqarray[outstanding_requests]);
                        if (mpi_errno) {
                            MPIR_ERR_POP(mpi_errno);
                        }

                        outstanding_requests++;
                    }
                }
            }

            for (i = 0; i < ss; i++) {
                dst = (rank - i - ii + comm_size) % comm_size;
                if (sendcounts[dst]) {
                    MPIR_Datatype_get_size_macro(sendtypes[dst], type_size);
                    if (type_size) {
                        mpi_errno = MPIC_Isend((char *) sendbuf + sdispls[dst],
                                               sendcounts[dst], sendtypes[dst], dst,
                                               MPIR_ALLTOALLW_TAG, comm_ptr,
                                               &reqarray[outstanding_requests], errflag);
                        if (mpi_errno) {
                            MPIR_ERR_POP(mpi_errno);
                        }

                        outstanding_requests++;
                    }
                }
            }

            mpi_errno = MPIC_Waitall(outstanding_requests, reqarray, starray, errflag);
            if (mpi_errno && mpi_errno != MPI_ERR_IN_STATUS)
                MPIR_ERR_POP(mpi_errno);

            /* --BEGIN ERROR HANDLING-- */
            if (mpi_errno == MPI_ERR_IN_STATUS) {
                for (i = 0; i < outstanding_requests; i++) {
                    if (starray[i].MPI_ERROR != MPI_SUCCESS) {
                        mpi_errno = starray[i].MPI_ERROR;
                        if (mpi_errno) {
                            /* for communication errors, just record the error but continue */
                            *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                            MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                        }
                    }
                }
            }
            /* --END ERROR HANDLING-- */
        }

#ifdef FOO
        /* Use pairwise exchange algorithm. */

        /* Make local copy first */
        mpi_errno = MPIR_Localcopy(((char *) sendbuf + sdispls[rank]),
                                   sendcounts[rank], sendtypes[rank],
                                   ((char *) recvbuf + rdispls[rank]),
                                   recvcounts[rank], recvtypes[rank]);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
        /* Do the pairwise exchange. */
        for (i = 1; i < comm_size; i++) {
            src = (rank - i + comm_size) % comm_size;
            dst = (rank + i) % comm_size;
            mpi_errno = MPIC_Sendrecv(((char *) sendbuf + sdispls[dst]),
                                      sendcounts[dst], sendtypes[dst], dst,
                                      MPIR_ALLTOALLW_TAG,
                                      ((char *) recvbuf + rdispls[src]),
                                      recvcounts[src], recvtypes[dst], src,
                                      MPIR_ALLTOALLW_TAG, comm_ptr, &status, errflag);
            if (mpi_errno) {
                /* for communication errors, just record the error but continue */
                *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
            }
        }
#endif
    }

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    if (mpi_errno_ret)
        mpi_errno = mpi_errno_ret;
    else if (*errflag != MPIR_ERR_NONE)
        MPIR_ERR_SET(mpi_errno, *errflag, "**coll_fail");
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}


/* not declared static because a machine-specific function may call this one in some cases */
#undef FUNCNAME
#define FUNCNAME MPIC_DEFAULT_Alltoallw_inter
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIC_DEFAULT_Alltoallw_inter(const void *sendbuf, const int sendcounts[],
                                 const int sdispls[], const MPI_Datatype sendtypes[],
                                 void *recvbuf, const int recvcounts[],
                                 const int rdispls[], const MPI_Datatype recvtypes[],
                                 MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
/* Intercommunicator alltoallw. We use a pairwise exchange algorithm
   similar to the one used in intracommunicator alltoallw. Since the
   local and remote groups can be of different
   sizes, we first compute the max of local_group_size,
   remote_group_size. At step i, 0 <= i < max_size, each process
   receives from src = (rank - i + max_size) % max_size if src <
   remote_size, and sends to dst = (rank + i) % max_size if dst <
   remote_size.

   FIXME: change algorithm to match intracommunicator alltoallv
*/
    int local_size, remote_size, max_size, i;
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    MPI_Status status;
    int src, dst, rank, sendcount, recvcount;
    char *sendaddr, *recvaddr;
    MPI_Datatype sendtype, recvtype;

    local_size = comm_ptr->local_size;
    remote_size = comm_ptr->remote_size;
    rank = comm_ptr->rank;

    /* Use pairwise exchange algorithm. */
    max_size = MPL_MAX(local_size, remote_size);
    for (i = 0; i < max_size; i++) {
        src = (rank - i + max_size) % max_size;
        dst = (rank + i) % max_size;
        if (src >= remote_size) {
            src = MPI_PROC_NULL;
            recvaddr = NULL;
            recvcount = 0;
            recvtype = MPI_DATATYPE_NULL;
        } else {
            recvaddr = (char *) recvbuf + rdispls[src];
            recvcount = recvcounts[src];
            recvtype = recvtypes[src];
        }
        if (dst >= remote_size) {
            dst = MPI_PROC_NULL;
            sendaddr = NULL;
            sendcount = 0;
            sendtype = MPI_DATATYPE_NULL;
        } else {
            sendaddr = (char *) sendbuf + sdispls[dst];
            sendcount = sendcounts[dst];
            sendtype = sendtypes[dst];
        }

        mpi_errno = MPIC_Sendrecv(sendaddr, sendcount, sendtype,
                                  dst, MPIR_ALLTOALLW_TAG, recvaddr,
                                  recvcount, recvtype, src,
                                  MPIR_ALLTOALLW_TAG, comm_ptr, &status, errflag);
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
#define FUNCNAME MPIC_DEFAULT_Alltoallw
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIC_DEFAULT_Alltoallw(const void *sendbuf, const int sendcounts[],
                           const int sdispls[], const MPI_Datatype sendtypes[],
                           void *recvbuf, const int recvcounts[], const int rdispls[],
                           const MPI_Datatype recvtypes[], MPIR_Comm * comm_ptr,
                           MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        /* intracommunicator */
        mpi_errno = MPIC_DEFAULT_Alltoallw_intra(sendbuf, sendcounts, sdispls,
                                                 sendtypes, recvbuf, recvcounts,
                                                 rdispls, recvtypes, comm_ptr, errflag);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    } else {
        /* intercommunicator */
        mpi_errno = MPIC_DEFAULT_Alltoallw_inter(sendbuf, sendcounts, sdispls,
                                                 sendtypes, recvbuf, recvcounts,
                                                 rdispls, recvtypes, comm_ptr, errflag);
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
