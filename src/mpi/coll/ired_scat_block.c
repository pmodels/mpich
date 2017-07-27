/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2010 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "collutil.h"

/* -- Begin Profiling Symbol Block for routine MPI_Ireduce_scatter_block */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Ireduce_scatter_block = PMPI_Ireduce_scatter_block
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Ireduce_scatter_block  MPI_Ireduce_scatter_block
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Ireduce_scatter_block as PMPI_Ireduce_scatter_block
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Ireduce_scatter_block(const void *sendbuf, void *recvbuf, int recvcount,
                              MPI_Datatype datatype, MPI_Op op, MPI_Comm comm,
                              MPI_Request *request)
                              __attribute__((weak,alias("PMPI_Ireduce_scatter_block")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Ireduce_scatter_block
#define MPI_Ireduce_scatter_block PMPI_Ireduce_scatter_block

/* any non-MPI functions go here, especially non-static ones */

/* A recursive halving MPI_Ireduce_scatter_block algorithm.  Requires that op is
 * commutative.  Typically yields better performance for shorter messages. */
#undef FUNCNAME
#define FUNCNAME MPIR_Ireduce_scatter_block_rec_hlv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Ireduce_scatter_block_rec_hlv(const void *sendbuf, void *recvbuf, int recvcount, MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int rank, comm_size, i;
    MPI_Aint extent, true_extent, true_lb;
    int  *disps;
    void *tmp_recvbuf, *tmp_results;
    int type_size ATTRIBUTE((unused)), total_count, dst;
    int mask;
    int *newcnts, *newdisps, rem, newdst, send_idx, recv_idx,
        last_idx, send_cnt, recv_cnt;
    int pof2, old_i, newrank;
    MPIR_SCHED_CHKPMEM_DECL(5);

    comm_size = comm_ptr->local_size;
    rank = comm_ptr->rank;

    MPIR_Datatype_get_extent_macro(datatype, extent);
    MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);

    MPIR_Assert(MPIR_Op_is_commutative(op));

    MPIR_SCHED_CHKPMEM_MALLOC(disps, int *, comm_size * sizeof(int), mpi_errno, "disps");

    total_count = 0;
    for (i=0; i<comm_size; i++) {
        disps[i] = total_count;
        total_count += recvcount;
    }

    if (total_count == 0) {
        goto fn_exit;
    }

    MPIR_Datatype_get_size_macro(datatype, type_size);

    /* allocate temp. buffer to receive incoming data */
    MPIR_SCHED_CHKPMEM_MALLOC(tmp_recvbuf, void *, total_count*(MPL_MAX(true_extent,extent)), mpi_errno, "tmp_recvbuf");
    /* adjust for potential negative lower bound in datatype */
    tmp_recvbuf = (void *)((char*)tmp_recvbuf - true_lb);

    /* need to allocate another temporary buffer to accumulate
       results because recvbuf may not be big enough */
    MPIR_SCHED_CHKPMEM_MALLOC(tmp_results, void *, total_count*(MPL_MAX(true_extent,extent)), mpi_errno, "tmp_results");
    /* adjust for potential negative lower bound in datatype */
    tmp_results = (void *)((char*)tmp_results - true_lb);

    /* copy sendbuf into tmp_results */
    if (sendbuf != MPI_IN_PLACE)
        mpi_errno = MPIR_Sched_copy(sendbuf, total_count, datatype,
                                    tmp_results, total_count, datatype, s);
    else
        mpi_errno = MPIR_Sched_copy(recvbuf, total_count, datatype,
                                    tmp_results, total_count, datatype, s);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    MPIR_SCHED_BARRIER(s);

    pof2 = 1;
    while (pof2 <= comm_size) pof2 <<= 1;
    pof2 >>=1;

    rem = comm_size - pof2;

    /* In the non-power-of-two case, all even-numbered
       processes of rank < 2*rem send their data to
       (rank+1). These even-numbered processes no longer
       participate in the algorithm until the very end. The
       remaining processes form a nice power-of-two. */

    if (rank < 2*rem) {
        if (rank % 2 == 0) { /* even */
            mpi_errno = MPIR_Sched_send(tmp_results, total_count, datatype, rank+1, comm_ptr, s);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            MPIR_SCHED_BARRIER(s);

            /* temporarily set the rank to -1 so that this
               process does not pariticipate in recursive
               doubling */
            newrank = -1;
        }
        else { /* odd */
            mpi_errno = MPIR_Sched_recv(tmp_recvbuf, total_count, datatype, rank-1, comm_ptr, s);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            MPIR_SCHED_BARRIER(s);

            /* do the reduction on received data. since the
               ordering is right, it doesn't matter whether
               the operation is commutative or not. */
            mpi_errno = MPIR_Sched_reduce(tmp_recvbuf, tmp_results, total_count, datatype, op, s);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            MPIR_SCHED_BARRIER(s);

            /* change the rank */
            newrank = rank / 2;
        }
    }
    else  /* rank >= 2*rem */
        newrank = rank - rem;

    if (newrank != -1) {
        /* recalculate the recvcnts and disps arrays because the
           even-numbered processes who no longer participate will
           have their result calculated by the process to their
           right (rank+1). */

        MPIR_SCHED_CHKPMEM_MALLOC(newcnts, int *, pof2*sizeof(int), mpi_errno, "newcnts");
        MPIR_SCHED_CHKPMEM_MALLOC(newdisps, int *, pof2*sizeof(int), mpi_errno, "newdisps");

        for (i = 0; i < pof2; i++) {
            /* what does i map to in the old ranking? */
            old_i = (i < rem) ? i*2 + 1 : i + rem;
            if (old_i < 2*rem) {
                /* This process has to also do its left neighbor's
                   work */
                newcnts[i] = 2 * recvcount;
            }
            else
                newcnts[i] = recvcount;
        }

        newdisps[0] = 0;
        for (i=1; i<pof2; i++)
            newdisps[i] = newdisps[i-1] + newcnts[i-1];

        mask = pof2 >> 1;
        send_idx = recv_idx = 0;
        last_idx = pof2;
        while (mask > 0) {
            newdst = newrank ^ mask;
            /* find real rank of dest */
            dst = (newdst < rem) ? newdst*2 + 1 : newdst + rem;

            send_cnt = recv_cnt = 0;
            if (newrank < newdst) {
                send_idx = recv_idx + mask;
                for (i=send_idx; i<last_idx; i++)
                    send_cnt += newcnts[i];
                for (i=recv_idx; i<send_idx; i++)
                    recv_cnt += newcnts[i];
            }
            else {
                recv_idx = send_idx + mask;
                for (i=send_idx; i<recv_idx; i++)
                    send_cnt += newcnts[i];
                for (i=recv_idx; i<last_idx; i++)
                    recv_cnt += newcnts[i];
            }

            /* Send data from tmp_results. Recv into tmp_recvbuf */
            {
                /* avoid sending and receiving pointless 0-byte messages */
                int send_dst = (send_cnt ? dst : MPI_PROC_NULL);
                int recv_dst = (recv_cnt ? dst : MPI_PROC_NULL);

                mpi_errno = MPIR_Sched_send(((char *)tmp_results + newdisps[send_idx]*extent),
                                            send_cnt, datatype, send_dst, comm_ptr, s);
                if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                mpi_errno = MPIR_Sched_recv(((char *) tmp_recvbuf + newdisps[recv_idx]*extent),
                                            recv_cnt, datatype, recv_dst, comm_ptr, s);
                if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                MPIR_SCHED_BARRIER(s);
            }

            /* tmp_recvbuf contains data received in this step.
               tmp_results contains data accumulated so far */
            if (recv_cnt) {
                mpi_errno = MPIR_Sched_reduce(((char *)tmp_recvbuf + newdisps[recv_idx]*extent),
                                              ((char *)tmp_results + newdisps[recv_idx]*extent),
                                              recv_cnt, datatype, op, s);
                if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                MPIR_SCHED_BARRIER(s);
            }

            /* update send_idx for next iteration */
            send_idx = recv_idx;
            last_idx = recv_idx + mask;
            mask >>= 1;
        }

        /* copy this process's result from tmp_results to recvbuf */
        mpi_errno = MPIR_Sched_copy(((char *)tmp_results + disps[rank]*extent),
                                    recvcount, datatype,
                                    recvbuf, recvcount, datatype, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        MPIR_SCHED_BARRIER(s);

    }

    /* In the non-power-of-two case, all odd-numbered
       processes of rank < 2*rem send to (rank-1) the result they
       calculated for that process */
    if (rank < 2*rem) {
        if (rank % 2) { /* odd */
            mpi_errno = MPIR_Sched_send(((char *)tmp_results + disps[rank-1]*extent),
                                        recvcount, datatype, rank-1, comm_ptr, s);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            MPIR_SCHED_BARRIER(s);
        }
        else  {   /* even */
            mpi_errno = MPIR_Sched_recv(recvbuf, recvcount, datatype, rank+1, comm_ptr, s);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            MPIR_SCHED_BARRIER(s);
        }
    }


    MPIR_SCHED_CHKPMEM_COMMIT(s);
fn_exit:
    return mpi_errno;
fn_fail:
    MPIR_SCHED_CHKPMEM_REAP(s);
    goto fn_exit;
}

/* A pairwise exchange algorithm for MPI_Ireduce_scatter_block.  Requires a
 * commutative op and is intended for use with large messages. */
#undef FUNCNAME
#define FUNCNAME MPIR_Ireduce_scatter_block_rec_pairwise
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Ireduce_scatter_block_pairwise(const void *sendbuf, void *recvbuf, int recvcount, MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int   rank, comm_size, i;
    MPI_Aint extent, true_extent, true_lb;
    int  *disps;
    void *tmp_recvbuf;
    int src, dst;
    int is_commutative;
    int total_count;
    MPIR_SCHED_CHKPMEM_DECL(2);

    comm_size = comm_ptr->local_size;
    rank = comm_ptr->rank;

    MPIR_Datatype_get_extent_macro(datatype, extent);
    MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);

    is_commutative = MPIR_Op_is_commutative(op);
    MPIR_Assert(is_commutative);

    MPIR_SCHED_CHKPMEM_MALLOC(disps, int *, comm_size * sizeof(int), mpi_errno, "disps");

    total_count = 0;
    for (i=0; i<comm_size; i++) {
        disps[i] = total_count;
        total_count += recvcount;
    }

    if (total_count == 0) {
        goto fn_exit;
    }
    /* total_count*extent eventually gets malloced. it isn't added to
     * a user-passed in buffer */
    MPIR_Ensure_Aint_fits_in_pointer(total_count * MPL_MAX(true_extent, extent));

    if (sendbuf != MPI_IN_PLACE) {
        /* copy local data into recvbuf */
        mpi_errno = MPIR_Sched_copy(((char *)sendbuf+disps[rank]*extent),
                                    recvcount, datatype,
                                    recvbuf, recvcount, datatype, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        MPIR_SCHED_BARRIER(s);
    }

    /* allocate temporary buffer to store incoming data */
    MPIR_SCHED_CHKPMEM_MALLOC(tmp_recvbuf, void *, recvcount*(MPL_MAX(true_extent,extent))+1, mpi_errno, "tmp_recvbuf");
    /* adjust for potential negative lower bound in datatype */
    tmp_recvbuf = (void *)((char*)tmp_recvbuf - true_lb);

    for (i=1; i<comm_size; i++) {
        src = (rank - i + comm_size) % comm_size;
        dst = (rank + i) % comm_size;

        /* send the data that dst needs. recv data that this process
           needs from src into tmp_recvbuf */
        if (sendbuf != MPI_IN_PLACE) {
            mpi_errno = MPIR_Sched_send(((char *)sendbuf+disps[dst]*extent),
                                        recvcount, datatype, dst, comm_ptr, s);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        }
        else {
            mpi_errno = MPIR_Sched_send(((char *)recvbuf+disps[dst]*extent),
                                        recvcount, datatype, dst, comm_ptr, s);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        }
        mpi_errno = MPIR_Sched_recv(tmp_recvbuf, recvcount, datatype, src, comm_ptr, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        MPIR_SCHED_BARRIER(s);

        /* FIXME does this algorithm actually work correctly for noncommutative ops?
         * If so, relax restriction in assert and comments... */
        if (is_commutative || (src < rank)) {
            if (sendbuf != MPI_IN_PLACE) {
                mpi_errno = MPIR_Sched_reduce(tmp_recvbuf, recvbuf, recvcount, datatype, op, s);
                if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            }
            else {
                mpi_errno = MPIR_Sched_reduce(tmp_recvbuf, ((char *)recvbuf+disps[rank]*extent),
                                              recvcount, datatype, op, s);
                if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                /* We can't store the result at the beginning of
                   recvbuf right here because there is useful data there that
                   other process/processes need.  At the end we will copy back
                   the result to the beginning of recvbuf. */
            }
            MPIR_SCHED_BARRIER(s);
        }
        else {
            if (sendbuf != MPI_IN_PLACE) {
                mpi_errno = MPIR_Sched_reduce(recvbuf, tmp_recvbuf, recvcount, datatype, op, s);
                if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                MPIR_SCHED_BARRIER(s);
                /* copy result back into recvbuf */
                mpi_errno = MPIR_Sched_copy(tmp_recvbuf, recvcount, datatype,
                                            recvbuf, recvcount, datatype, s);
                if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            }
            else {
                mpi_errno = MPIR_Sched_reduce(((char *)recvbuf+disps[rank]*extent),
                                              tmp_recvbuf, recvcount, datatype, op, s);
                if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                MPIR_SCHED_BARRIER(s);
                /* copy result back into recvbuf */
                mpi_errno = MPIR_Sched_copy(tmp_recvbuf, recvcount, datatype,
                                            ((char *)recvbuf + disps[rank]*extent),
                                            recvcount, datatype, s);
                if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            }
            MPIR_SCHED_BARRIER(s);
        }
    }

    /* if MPI_IN_PLACE, move output data to the beginning of
       recvbuf. already done for rank 0. */
    if ((sendbuf == MPI_IN_PLACE) && (rank != 0)) {
        mpi_errno = MPIR_Sched_copy(((char *)recvbuf + disps[rank]*extent),
                                    recvcount, datatype,
                                    recvbuf, recvcount, datatype, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        MPIR_SCHED_BARRIER(s);
    }

    MPIR_SCHED_CHKPMEM_COMMIT(s);
fn_exit:
    return mpi_errno;
fn_fail:
    MPIR_SCHED_CHKPMEM_REAP(s);
    goto fn_exit;
}

/* A recursive doubling algorithm for MPI_Ireduce_scatter_block, suitable for
 * noncommutative and (non-pof2 or block irregular). */
#undef FUNCNAME
#define FUNCNAME MPIR_Ireduce_scatter_block_rec_dbl
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Ireduce_scatter_block_rec_dbl(const void *sendbuf, void *recvbuf, int recvcount, MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int rank, comm_size, i;
    MPI_Aint extent, true_extent, true_lb;
    int  *disps;
    void *tmp_recvbuf, *tmp_results;
    int type_size ATTRIBUTE((unused)), dis[2], blklens[2], total_count, dst;
    int mask, dst_tree_root, my_tree_root, j, k;
    int received;
    MPI_Datatype sendtype, recvtype;
    int nprocs_completed, tmp_mask, tree_root, is_commutative;
    MPIR_SCHED_CHKPMEM_DECL(5);

    comm_size = comm_ptr->local_size;
    rank = comm_ptr->rank;

    MPIR_Datatype_get_extent_macro(datatype, extent);
    MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);
    is_commutative = MPIR_Op_is_commutative(op);

    MPIR_SCHED_CHKPMEM_MALLOC(disps, int *, comm_size * sizeof(int), mpi_errno, "disps");

    total_count = 0;
    for (i=0; i<comm_size; i++) {
        disps[i] = total_count;
        total_count += recvcount;
    }

    if (total_count == 0) {
        goto fn_exit;
    }

    MPIR_Datatype_get_size_macro(datatype, type_size);

    /* total_count*extent eventually gets malloced. it isn't added to
     * a user-passed in buffer */
    MPIR_Ensure_Aint_fits_in_pointer(total_count * MPL_MAX(true_extent, extent));


    /* need to allocate temporary buffer to receive incoming data*/
    MPIR_SCHED_CHKPMEM_MALLOC(tmp_recvbuf, void *, total_count*(MPL_MAX(true_extent,extent)), mpi_errno, "tmp_recvbuf");
    /* adjust for potential negative lower bound in datatype */
    tmp_recvbuf = (void *)((char*)tmp_recvbuf - true_lb);

    /* need to allocate another temporary buffer to accumulate
       results */
    MPIR_SCHED_CHKPMEM_MALLOC(tmp_results, void *, total_count*(MPL_MAX(true_extent,extent)), mpi_errno, "tmp_results");
    /* adjust for potential negative lower bound in datatype */
    tmp_results = (void *)((char*)tmp_results - true_lb);

    /* copy sendbuf into tmp_results */
    if (sendbuf != MPI_IN_PLACE)
        mpi_errno = MPIR_Sched_copy(sendbuf, total_count, datatype,
                                    tmp_results, total_count, datatype, s);
    else
        mpi_errno = MPIR_Sched_copy(recvbuf, total_count, datatype,
                                    tmp_results, total_count, datatype, s);

    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    MPIR_SCHED_BARRIER(s);

    mask = 0x1;
    i = 0;
    while (mask < comm_size) {
        dst = rank ^ mask;

        dst_tree_root = dst >> i;
        dst_tree_root <<= i;

        my_tree_root = rank >> i;
        my_tree_root <<= i;

        /* At step 1, processes exchange (n-n/p) amount of
           data; at step 2, (n-2n/p) amount of data; at step 3, (n-4n/p)
           amount of data, and so forth. We use derived datatypes for this.

           At each step, a process does not need to send data
           indexed from my_tree_root to
           my_tree_root+mask-1. Similarly, a process won't receive
           data indexed from dst_tree_root to dst_tree_root+mask-1. */

        /* calculate sendtype */
        blklens[0] = my_tree_root * recvcount;
        blklens[1] = (comm_size - (my_tree_root + mask)) * recvcount;
        if (blklens[1] < 0)
            blklens[1] = 0;

        dis[0] = 0;
        dis[1] = blklens[0] + recvcount * (MPL_MIN((my_tree_root + mask), comm_size) - my_tree_root);

        mpi_errno = MPIR_Type_indexed_impl(2, blklens, dis, datatype, &sendtype);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);

        mpi_errno = MPIR_Type_commit_impl(&sendtype);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);

        /* calculate recvtype */
        blklens[0] = recvcount * MPL_MIN(dst_tree_root, comm_size);
        blklens[1] = recvcount * (comm_size - (dst_tree_root + mask));
        if (blklens[1] < 0)
            blklens[1] = 0;

        dis[0] = 0;
        dis[1] = blklens[0] + recvcount * (MPL_MIN((dst_tree_root + mask), comm_size) - dst_tree_root);

        mpi_errno = MPIR_Type_indexed_impl(2, blklens, dis, datatype, &recvtype);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);

        mpi_errno = MPIR_Type_commit_impl(&recvtype);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);

        received = 0;
        if (dst < comm_size) {
            /* tmp_results contains data to be sent in each step. Data is
               received in tmp_recvbuf and then accumulated into
               tmp_results. accumulation is done later below.   */
            mpi_errno = MPIR_Sched_send(tmp_results, 1, sendtype, dst, comm_ptr, s);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            mpi_errno = MPIR_Sched_recv(tmp_recvbuf, 1, recvtype, dst, comm_ptr, s);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            MPIR_SCHED_BARRIER(s);
            received = 1;
        }

        /* if some processes in this process's subtree in this step
           did not have any destination process to communicate with
           because of non-power-of-two, we need to send them the
           result. We use a logarithmic recursive-halfing algorithm
           for this. */

        if (dst_tree_root + mask > comm_size) {
            nprocs_completed = comm_size - my_tree_root - mask;
            /* nprocs_completed is the number of processes in this
               subtree that have all the data. Send data to others
               in a tree fashion. First find root of current tree
               that is being divided into two. k is the number of
               least-significant bits in this process's rank that
               must be zeroed out to find the rank of the root */
            j = mask;
            k = 0;
            while (j) {
                j >>= 1;
                k++;
            }
            k--;

            tmp_mask = mask >> 1;
            while (tmp_mask) {
                dst = rank ^ tmp_mask;

                tree_root = rank >> k;
                tree_root <<= k;

                /* send only if this proc has data and destination
                   doesn't have data. at any step, multiple processes
                   can send if they have the data */
                if ((dst > rank) &&
                    (rank < tree_root + nprocs_completed)
                    && (dst >= tree_root + nprocs_completed))
                {
                    /* send the current result */
                    mpi_errno = MPIR_Sched_send(tmp_recvbuf, 1, recvtype, dst, comm_ptr, s);
                    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                    MPIR_SCHED_BARRIER(s);
                }
                /* recv only if this proc. doesn't have data and sender
                   has data */
                else if ((dst < rank) &&
                         (dst < tree_root + nprocs_completed) &&
                         (rank >= tree_root + nprocs_completed))
                {
                    mpi_errno = MPIR_Sched_recv(tmp_recvbuf, 1, recvtype, dst, comm_ptr, s);
                    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                    MPIR_SCHED_BARRIER(s);
                    received = 1;
                }
                tmp_mask >>= 1;
                k--;
            }
        }

        /* N.B. The following comment comes from the FT version of
         * MPI_Reduce_scatter.  It does not currently apply to this code, but
         * will in the future when we update the NBC code to be fault-tolerant
         * in roughly the same fashion. [goodell@ 2011-03-03] */
        /* The following reduction is done here instead of after
           the MPIC_Sendrecv or MPIC_Recv above. This is
           because to do it above, in the noncommutative
           case, we would need an extra temp buffer so as not to
           overwrite temp_recvbuf, because temp_recvbuf may have
           to be communicated to other processes in the
           non-power-of-two case. To avoid that extra allocation,
           we do the reduce here. */
        if (received) {
            if (is_commutative || (dst_tree_root < my_tree_root)) {
                mpi_errno = MPIR_Sched_reduce(tmp_recvbuf, tmp_results, blklens[0], datatype, op, s);
                if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                mpi_errno = MPIR_Sched_reduce(((char *)tmp_recvbuf + dis[1]*extent),
                                              ((char *)tmp_results + dis[1]*extent),
                                              blklens[1], datatype, op, s);
                if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                MPIR_SCHED_BARRIER(s);
            }
            else {
                mpi_errno = MPIR_Sched_reduce(tmp_results, tmp_recvbuf, blklens[0], datatype, op, s);
                if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                mpi_errno = MPIR_Sched_reduce(((char *)tmp_results + dis[1]*extent),
                                              ((char *)tmp_recvbuf + dis[1]*extent),
                                              blklens[1], datatype, op, s);
                if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                MPIR_SCHED_BARRIER(s);

                /* copy result back into tmp_results */
                mpi_errno = MPIR_Sched_copy(tmp_recvbuf, 1, recvtype,
                                            tmp_results, 1, recvtype, s);
                if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                MPIR_SCHED_BARRIER(s);
            }
        }

        MPIR_Type_free_impl(&sendtype);
        MPIR_Type_free_impl(&recvtype);

        mask <<= 1;
        i++;
    }

    /* now copy final results from tmp_results to recvbuf */
    mpi_errno = MPIR_Sched_copy(((char *)tmp_results+disps[rank]*extent),
                                recvcount, datatype,
                                recvbuf, recvcount, datatype, s);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    MPIR_SCHED_BARRIER(s);

    MPIR_SCHED_CHKPMEM_COMMIT(s);
fn_exit:
    return mpi_errno;
fn_fail:
    MPIR_SCHED_CHKPMEM_REAP(s);
    goto fn_exit;
}

/* Implements the reduce-scatter butterfly algorithm described in J. L. Traff's
 * "An Improved Algorithm for (Non-commutative) Reduce-Scatter with an Application"
 * from EuroPVM/MPI 2005.  This function currently only implements support for
 * the power-of-2 case. */
#undef FUNCNAME
#define FUNCNAME MPIR_Reduce_scatter_block_noncomm
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Ireduce_scatter_block_noncomm(const void *sendbuf, void *recvbuf, int recvcount, MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int comm_size = comm_ptr->local_size;
    int rank = comm_ptr->rank;
    int pof2;
    int log2_comm_size;
    int i, k;
    int recv_offset, send_offset;
    int block_size, total_count, size;
    MPI_Aint true_extent, true_lb;
    int buf0_was_inout;
    void *tmp_buf0;
    void *tmp_buf1;
    void *result_ptr;
    MPIR_SCHED_CHKPMEM_DECL(2);

    MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);

    pof2 = 1;
    log2_comm_size = 0;
    while (pof2 < comm_size) {
        pof2 <<= 1;
        ++log2_comm_size;
    }

    /* begin error checking */
    MPIR_Assert(pof2 == comm_size); /* FIXME this version only works for power of 2 procs */
    /* end error checking */

    /* size of a block (count of datatype per block, NOT bytes per block) */
    block_size = recvcount;
    total_count = block_size * comm_size;

    MPIR_SCHED_CHKPMEM_MALLOC(tmp_buf0, void *, true_extent * total_count, mpi_errno, "tmp_buf0");
    MPIR_SCHED_CHKPMEM_MALLOC(tmp_buf1, void *, true_extent * total_count, mpi_errno, "tmp_buf1");
    /* adjust for potential negative lower bound in datatype */
    tmp_buf0 = (void *)((char*)tmp_buf0 - true_lb);
    tmp_buf1 = (void *)((char*)tmp_buf1 - true_lb);

    /* Copy our send data to tmp_buf0.  We do this one block at a time and
       permute the blocks as we go according to the mirror permutation. */
    for (i = 0; i < comm_size; ++i) {
        mpi_errno = MPIR_Sched_copy(((char *)(sendbuf == MPI_IN_PLACE ? (const void *)recvbuf : sendbuf) + (i * true_extent * block_size)),
                                    block_size, datatype,
                                    ((char *)tmp_buf0 + (MPIU_Mirror_permutation(i, log2_comm_size) * true_extent * block_size)),
                                     block_size, datatype, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }
    MPIR_SCHED_BARRIER(s);
    buf0_was_inout = 1;

    send_offset = 0;
    recv_offset = 0;
    size = total_count;
    for (k = 0; k < log2_comm_size; ++k) {
        /* use a double-buffering scheme to avoid local copies */
        char *incoming_data = (buf0_was_inout ? tmp_buf1 : tmp_buf0);
        char *outgoing_data = (buf0_was_inout ? tmp_buf0 : tmp_buf1);
        int peer = rank ^ (0x1 << k);
        size /= 2;

        if (rank > peer) {
            /* we have the higher rank: send top half, recv bottom half */
            recv_offset += size;
        }
        else {
            /* we have the lower rank: recv top half, send bottom half */
            send_offset += size;
        }

        mpi_errno = MPIR_Sched_send((outgoing_data + send_offset*true_extent),
                                    size, datatype, peer, comm_ptr, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        mpi_errno = MPIR_Sched_recv((incoming_data + recv_offset*true_extent),
                                    size, datatype, peer, comm_ptr, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        MPIR_SCHED_BARRIER(s);

        /* always perform the reduction at recv_offset, the data at send_offset
           is now our peer's responsibility */
        if (rank > peer) {
            /* higher ranked value so need to call op(received_data, my_data) */
            mpi_errno = MPIR_Sched_reduce((incoming_data + recv_offset*true_extent),
                                          (outgoing_data + recv_offset*true_extent),
                                          size, datatype, op, s);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        }
        else {
            /* lower ranked value so need to call op(my_data, received_data) */
            mpi_errno = MPIR_Sched_reduce((outgoing_data + recv_offset*true_extent),
                                          (incoming_data + recv_offset*true_extent),
                                          size, datatype, op, s);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            buf0_was_inout = !buf0_was_inout;
        }
        MPIR_SCHED_BARRIER(s);

        /* the next round of send/recv needs to happen within the block (of size
           "size") that we just received and reduced */
        send_offset = recv_offset;
    }

    MPIR_Assert(size == recvcount);

    /* copy the reduced data to the recvbuf */
    result_ptr = (char *)(buf0_was_inout ? tmp_buf0 : tmp_buf1) + recv_offset * true_extent;
    mpi_errno = MPIR_Sched_copy(result_ptr, size, datatype,
                                recvbuf, size, datatype, s);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

    MPIR_SCHED_CHKPMEM_COMMIT(s);
fn_exit:
    return mpi_errno;
fn_fail:
    MPIR_SCHED_CHKPMEM_REAP(s);
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Ireduce_scatter_block_intra
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Ireduce_scatter_block_intra(const void *sendbuf, void *recvbuf, int recvcount, MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int is_commutative;
    int total_count, type_size, nbytes;
    int comm_size;

    is_commutative = MPIR_Op_is_commutative(op);

    comm_size = comm_ptr->local_size;
    total_count = recvcount * comm_size;
    if (total_count == 0) {
        goto fn_exit;
    }
    MPIR_Datatype_get_size_macro(datatype, type_size);
    nbytes = total_count * type_size;

    /* select an appropriate algorithm based on commutivity and message size */
    if (is_commutative && (nbytes < MPIR_CVAR_REDSCAT_COMMUTATIVE_LONG_MSG_SIZE)) {
        mpi_errno = MPIR_Ireduce_scatter_block_rec_hlv(sendbuf, recvbuf, recvcount, datatype, op, comm_ptr, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }
    else if (is_commutative && (nbytes >= MPIR_CVAR_REDSCAT_COMMUTATIVE_LONG_MSG_SIZE)) {
        mpi_errno = MPIR_Ireduce_scatter_block_pairwise(sendbuf, recvbuf, recvcount, datatype, op, comm_ptr, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }
    else /* (!is_commutative) */ {
        if (MPIU_is_pof2(comm_size, NULL)) {
            /* noncommutative, pof2 size */
            mpi_errno = MPIR_Ireduce_scatter_block_noncomm(sendbuf, recvbuf, recvcount, datatype, op, comm_ptr, s);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        }
        else {
            /* noncommutative and non-pof2, use recursive doubling. */
            mpi_errno = MPIR_Ireduce_scatter_block_rec_dbl(sendbuf, recvbuf, recvcount, datatype, op, comm_ptr, s);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        }
    }

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPIR_Ireduce_scatter_block_inter
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Ireduce_scatter_block_inter(const void *sendbuf, void *recvbuf, int recvcount, MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Sched_t s)
{
/* Intercommunicator Ireduce_scatter_block.
   We first do an intercommunicator reduce to rank 0 on left group,
   then an intercommunicator reduce to rank 0 on right group, followed
   by local intracommunicator scattervs in each group.
*/
    int mpi_errno = MPI_SUCCESS;
    int rank, root, local_size, total_count;
    MPI_Aint true_extent, true_lb = 0, extent;
    void *tmp_buf = NULL;
    MPIR_Comm *newcomm_ptr = NULL;
    MPIR_SCHED_CHKPMEM_DECL(1);

    rank = comm_ptr->rank;
    local_size = comm_ptr->local_size;

    total_count = recvcount * local_size;

    if (rank == 0) {
        /* In each group, rank 0 allocates a temp. buffer for the
           reduce */
        MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);
        MPIR_Datatype_get_extent_macro(datatype, extent);

        MPIR_SCHED_CHKPMEM_MALLOC(tmp_buf, void *, total_count*(MPL_MAX(extent,true_extent)), mpi_errno, "tmp_buf");

        /* adjust for potential negative lower bound in datatype */
        tmp_buf = (void *)((char*)tmp_buf - true_lb);
    }

    /* first do a reduce from right group to rank 0 in left group,
       then from left group to rank 0 in right group*/
    MPIR_Assert(comm_ptr->coll_fns && comm_ptr->coll_fns->Ireduce_sched);
    if (comm_ptr->is_low_group) {
        /* reduce from right group to rank 0*/
        root = (rank == 0) ? MPI_ROOT : MPI_PROC_NULL;
        mpi_errno = comm_ptr->coll_fns->Ireduce_sched(sendbuf, tmp_buf, total_count,
                                                datatype, op, root, comm_ptr, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);

        /* sched barrier intentionally omitted here to allow both reductions to
         * proceed in parallel */

        /* reduce to rank 0 of right group */
        root = 0;
        mpi_errno = comm_ptr->coll_fns->Ireduce_sched(sendbuf, tmp_buf, total_count,
                                                datatype, op, root, comm_ptr, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }
    else {
        /* reduce to rank 0 of right group */
        root = 0;
        mpi_errno = comm_ptr->coll_fns->Ireduce_sched(sendbuf, tmp_buf, total_count,
                                                datatype, op, root, comm_ptr, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);

        /* sched barrier intentionally omitted here to allow both reductions to
         * proceed in parallel */

        /* reduce from right group to rank 0*/
        root = (rank == 0) ? MPI_ROOT : MPI_PROC_NULL;
        mpi_errno = comm_ptr->coll_fns->Ireduce_sched(sendbuf, tmp_buf, total_count,
                                                datatype, op, root, comm_ptr, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }
    MPIR_SCHED_BARRIER(s);

    /* Get the local intracommunicator */
    if (!comm_ptr->local_comm) {
        mpi_errno = MPII_Setup_intercomm_localcomm(comm_ptr);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }

    newcomm_ptr = comm_ptr->local_comm;

    MPIR_Assert(newcomm_ptr->coll_fns && newcomm_ptr->coll_fns->Iscatter_sched);
    mpi_errno = newcomm_ptr->coll_fns->Iscatter_sched(tmp_buf, recvcount, datatype,
                                                recvbuf, recvcount, datatype, 0,
                                                newcomm_ptr, s);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

    MPIR_SCHED_CHKPMEM_COMMIT(s);
fn_exit:
    return mpi_errno;
fn_fail:
    MPIR_SCHED_CHKPMEM_REAP(s);
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Ireduce_scatter_block_impl
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Ireduce_scatter_block_impl(const void *sendbuf, void *recvbuf, int recvcount, MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPI_Request *request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *reqp = NULL;
    int tag = -1;
    MPIR_Sched_t s = MPIR_SCHED_NULL;

    *request = MPI_REQUEST_NULL;

    mpi_errno = MPIR_Sched_next_tag(comm_ptr, &tag);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    mpi_errno = MPIR_Sched_create(&s);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

    MPIR_Assert(comm_ptr->coll_fns != NULL);
    MPIR_Assert(comm_ptr->coll_fns->Ireduce_scatter_block_sched != NULL);
    mpi_errno = comm_ptr->coll_fns->Ireduce_scatter_block_sched(sendbuf, recvbuf, recvcount, datatype, op, comm_ptr, s);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

    mpi_errno = MPIR_Sched_start(&s, comm_ptr, tag, &reqp);
    if (reqp)
        *request = reqp->handle;
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#endif /* MPICH_MPI_FROM_PMPI */

#undef FUNCNAME
#define FUNCNAME MPI_Ireduce_scatter_block
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@
MPI_Ireduce_scatter_block - Combines values and scatters the results in
                            a nonblocking way

Input Parameters:
+ sendbuf - starting address of the send buffer (choice)
. recvcount - element count per block (non-negative integer)
. datatype - data type of elements of input buffer (handle)
. op - operation (handle)
- comm - communicator (handle)

Output Parameters:
+ recvbuf - starting address of the receive buffer (choice)
- request - communication request (handle)

.N ThreadSafe

.N Fortran

.N Errors
@*/
int MPI_Ireduce_scatter_block(const void *sendbuf, void *recvbuf,
                              int recvcount,
                              MPI_Datatype datatype, MPI_Op op, MPI_Comm comm,
                              MPI_Request *request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *comm_ptr = NULL;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_IREDUCE_SCATTER_BLOCK);

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_IREDUCE_SCATTER_BLOCK);

    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS
        {
            MPIR_ERRTEST_DATATYPE(datatype, "datatype", mpi_errno);
            MPIR_ERRTEST_OP(op, mpi_errno);
            MPIR_ERRTEST_COMM(comm, mpi_errno);

            /* TODO more checks may be appropriate */
        }
        MPID_END_ERROR_CHECKS
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* Convert MPI object handles to object pointers */
    MPIR_Comm_get_ptr(comm, comm_ptr);

    /* Validate parameters and objects (post conversion) */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS
        {
            MPIR_Comm_valid_ptr( comm_ptr, mpi_errno, FALSE );
            if (HANDLE_GET_KIND(datatype) != HANDLE_KIND_BUILTIN) {
                MPIR_Datatype *datatype_ptr = NULL;
                MPIR_Datatype_get_ptr(datatype, datatype_ptr);
                MPIR_Datatype_valid_ptr(datatype_ptr, mpi_errno);
                if (mpi_errno != MPI_SUCCESS) goto fn_fail;
                MPIR_Datatype_committed_ptr(datatype_ptr, mpi_errno);
                if (mpi_errno != MPI_SUCCESS) goto fn_fail;
            }

            if (HANDLE_GET_KIND(op) != HANDLE_KIND_BUILTIN) {
                MPIR_Op *op_ptr = NULL;
                MPIR_Op_get_ptr(op, op_ptr);
                MPIR_Op_valid_ptr(op_ptr, mpi_errno);
            }
            else if (HANDLE_GET_KIND(op) == HANDLE_KIND_BUILTIN) {
                mpi_errno = ( * MPIR_OP_HDL_TO_DTYPE_FN(op) )(datatype);
            }
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;

            MPIR_ERRTEST_ARGNULL(request,"request", mpi_errno);

            if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM && sendbuf != MPI_IN_PLACE && recvcount != 0)
                MPIR_ERRTEST_ALIAS_COLL(sendbuf, recvbuf, mpi_errno)
            /* TODO more checks may be appropriate (counts, in_place, etc) */
        }
        MPID_END_ERROR_CHECKS
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    mpi_errno = MPID_Ireduce_scatter_block(sendbuf, recvbuf, recvcount, datatype, op, comm_ptr, request);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

    /* ... end of body of routine ... */

fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_IREDUCE_SCATTER_BLOCK);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno = MPIR_Err_create_code(
            mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
            "**mpi_ireduce_scatter_block", "**mpi_ireduce_scatter_block %p %p %d %D %O %C %p", sendbuf, recvbuf, recvcount, datatype, op, comm, request);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm(comm_ptr, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
