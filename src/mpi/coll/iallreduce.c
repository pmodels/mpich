/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2010 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "collutil.h"

/* -- Begin Profiling Symbol Block for routine MPI_Iallreduce */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Iallreduce = PMPI_Iallreduce
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Iallreduce  MPI_Iallreduce
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Iallreduce as PMPI_Iallreduce
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Iallreduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
                   MPI_Op op, MPI_Comm comm, MPI_Request *request)
                   __attribute__((weak,alias("PMPI_Iallreduce")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Iallreduce
#define MPI_Iallreduce PMPI_Iallreduce

/* any non-MPI functions go here, especially non-static ones */

/* implements the naive intracomm allreduce, that is, reduce followed by bcast */
#undef FUNCNAME
#define FUNCNAME MPIR_Iallreduce_naive
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Iallreduce_naive(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPID_Comm *comm_ptr, MPID_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int rank;

    rank = comm_ptr->rank;

    if ((sendbuf == MPI_IN_PLACE) && (rank != 0)) {
        mpi_errno = MPIR_Ireduce_intra(recvbuf, NULL, count, datatype, op, 0, comm_ptr, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }
    else {
        mpi_errno = MPIR_Ireduce_intra(sendbuf, recvbuf, count, datatype, op, 0, comm_ptr, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }

    MPID_SCHED_BARRIER(s);

    mpi_errno = MPIR_Ibcast_intra(recvbuf, count, datatype, 0, comm_ptr, s);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

/* also known as "Rabenseifner's algorithm" */
#undef FUNCNAME
#define FUNCNAME MPIR_Iallreduce_redscat_allgather
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Iallreduce_redscat_allgather(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPID_Comm *comm_ptr, MPID_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int comm_size, rank, newrank, pof2, rem;
    int i, send_idx, recv_idx, last_idx, mask, newdst, dst, send_cnt, recv_cnt;
    MPI_Aint true_lb, true_extent, extent;
    void *tmp_buf = NULL;
    int *cnts, *disps;
    MPIR_SCHED_CHKPMEM_DECL(1);
    MPIU_CHKLMEM_DECL(2);

    /* we only support builtin datatypes for now, breaking up user types to do
     * the reduce-scatter is tricky */
    MPIU_Assert(HANDLE_GET_KIND(op) == HANDLE_KIND_BUILTIN);

    comm_size = comm_ptr->local_size;
    rank = comm_ptr->rank;

    /* need to allocate temporary buffer to store incoming data*/
    MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);
    MPID_Datatype_get_extent_macro(datatype, extent);

    MPIU_Ensure_Aint_fits_in_pointer(count * MPIR_MAX(extent, true_extent));
    MPIR_SCHED_CHKPMEM_MALLOC(tmp_buf, void *, count*(MPIR_MAX(extent,true_extent)), mpi_errno, "temporary buffer");

    /* adjust for potential negative lower bound in datatype */
    tmp_buf = (void *)((char*)tmp_buf - true_lb);

    /* copy local data into recvbuf */
    if (sendbuf != MPI_IN_PLACE) {
        mpi_errno = MPID_Sched_copy(sendbuf, count, datatype,
                                    recvbuf, count, datatype, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        MPID_SCHED_BARRIER(s);
    }

    /* find nearest power-of-two less than or equal to comm_size */
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
            mpi_errno = MPID_Sched_send(recvbuf, count, datatype, rank+1, comm_ptr, s);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            MPID_SCHED_BARRIER(s);

            /* temporarily set the rank to -1 so that this
               process does not pariticipate in recursive
               doubling */
            newrank = -1;
        }
        else { /* odd */
            mpi_errno = MPID_Sched_recv(tmp_buf, count, datatype, rank-1, comm_ptr, s);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            MPID_SCHED_BARRIER(s);

            /* do the reduction on received data. since the
               ordering is right, it doesn't matter whether
               the operation is commutative or not. */
            mpi_errno = MPID_Sched_reduce(tmp_buf, recvbuf, count, datatype, op, s);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            MPID_SCHED_BARRIER(s);

            /* change the rank */
            newrank = rank / 2;
        }
    }
    else  /* rank >= 2*rem */
        newrank = rank - rem;

    if (newrank != -1) {
        /* for the reduce-scatter, calculate the count that
           each process receives and the displacement within
           the buffer */
        /* TODO I (goodell@) believe that these counts and displacements could be
         * calculated directly during the loop, rather than requiring a less-scalable
         * "2*pof2"-sized memory allocation */

        MPIU_CHKLMEM_MALLOC(cnts, int *, pof2*sizeof(int), mpi_errno, "counts");
        MPIU_CHKLMEM_MALLOC(disps, int *, pof2*sizeof(int), mpi_errno, "displacements");

        MPIU_Assert(count >= pof2); /* the cnts calculations assume this */
        for (i=0; i<(pof2-1); i++)
            cnts[i] = count/pof2;
        cnts[pof2-1] = count - (count/pof2)*(pof2-1);

        disps[0] = 0;
        for (i=1; i<pof2; i++)
            disps[i] = disps[i-1] + cnts[i-1];

        mask = 0x1;
        send_idx = recv_idx = 0;
        last_idx = pof2;
        while (mask < pof2) {
            newdst = newrank ^ mask;
            /* find real rank of dest */
            dst = (newdst < rem) ? newdst*2 + 1 : newdst + rem;

            send_cnt = recv_cnt = 0;
            if (newrank < newdst) {
                send_idx = recv_idx + pof2/(mask*2);
                for (i=send_idx; i<last_idx; i++)
                    send_cnt += cnts[i];
                for (i=recv_idx; i<send_idx; i++)
                    recv_cnt += cnts[i];
            }
            else {
                recv_idx = send_idx + pof2/(mask*2);
                for (i=send_idx; i<recv_idx; i++)
                    send_cnt += cnts[i];
                for (i=recv_idx; i<last_idx; i++)
                    recv_cnt += cnts[i];
            }

            /* Send data from recvbuf. Recv into tmp_buf */
            mpi_errno = MPID_Sched_recv(((char *)tmp_buf + disps[recv_idx]*extent),
                                        recv_cnt, datatype, dst, comm_ptr, s);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            /* sendrecv, no barrier here */
            mpi_errno = MPID_Sched_send(((char *)recvbuf + disps[send_idx]*extent),
                                        send_cnt, datatype, dst, comm_ptr, s);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            MPID_SCHED_BARRIER(s);

            /* tmp_buf contains data received in this step.
               recvbuf contains data accumulated so far */

            /* This algorithm is used only for predefined ops
               and predefined ops are always commutative. */
            mpi_errno = MPID_Sched_reduce(((char *)tmp_buf + disps[recv_idx]*extent),
                                          ((char *)recvbuf + disps[recv_idx]*extent),
                                          recv_cnt, datatype, op, s);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            MPID_SCHED_BARRIER(s);

            /* update send_idx for next iteration */
            send_idx = recv_idx;
            mask <<= 1;

            /* update last_idx, but not in last iteration
               because the value is needed in the allgather
               step below. */
            if (mask < pof2)
                last_idx = recv_idx + pof2/mask;
        }

        /* now do the allgather */

        mask >>= 1;
        while (mask > 0) {
            newdst = newrank ^ mask;
            /* find real rank of dest */
            dst = (newdst < rem) ? newdst*2 + 1 : newdst + rem;

            send_cnt = recv_cnt = 0;
            if (newrank < newdst) {
                /* update last_idx except on first iteration */
                if (mask != pof2/2)
                    last_idx = last_idx + pof2/(mask*2);

                recv_idx = send_idx + pof2/(mask*2);
                for (i=send_idx; i<recv_idx; i++)
                    send_cnt += cnts[i];
                for (i=recv_idx; i<last_idx; i++)
                    recv_cnt += cnts[i];
            }
            else {
                recv_idx = send_idx - pof2/(mask*2);
                for (i=send_idx; i<last_idx; i++)
                    send_cnt += cnts[i];
                for (i=recv_idx; i<send_idx; i++)
                    recv_cnt += cnts[i];
            }

            mpi_errno = MPID_Sched_recv(((char *)recvbuf + disps[recv_idx]*extent),
                                        recv_cnt, datatype, dst, comm_ptr, s);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            /* sendrecv, no barrier here */
            mpi_errno = MPID_Sched_send(((char *)recvbuf + disps[send_idx]*extent),
                                        send_cnt, datatype, dst, comm_ptr, s);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            MPID_SCHED_BARRIER(s);

            if (newrank > newdst) send_idx = recv_idx;

            mask >>= 1;
        }
    }

    /* In the non-power-of-two case, all odd-numbered
       processes of rank < 2*rem send the result to
       (rank-1), the ranks who didn't participate above. */
    if (rank < 2*rem) {
        if (rank % 2) { /* odd */
            mpi_errno = MPID_Sched_send(recvbuf, count, datatype, rank-1, comm_ptr, s);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        }
        else { /* even */
            mpi_errno = MPID_Sched_recv(recvbuf, count, datatype, rank+1, comm_ptr, s);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        }
    }

    MPIR_SCHED_CHKPMEM_COMMIT(s);
fn_exit:
    MPIU_CHKLMEM_FREEALL();
    return mpi_errno;
fn_fail:
    MPIR_SCHED_CHKPMEM_REAP(s);
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Iallreduce_rec_dbl
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Iallreduce_rec_dbl(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPID_Comm *comm_ptr, MPID_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int pof2, rem, comm_size, is_commutative, rank;
    int newrank, mask, newdst, dst;
    MPI_Aint true_lb, true_extent, extent;
    void *tmp_buf = NULL;
    MPIR_SCHED_CHKPMEM_DECL(1);

    comm_size = comm_ptr->local_size;
    rank = comm_ptr->rank;

    is_commutative = MPIR_Op_is_commutative(op);

    /* need to allocate temporary buffer to store incoming data*/
    MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);
    MPID_Datatype_get_extent_macro(datatype, extent);

    MPIU_Ensure_Aint_fits_in_pointer(count * MPIR_MAX(extent, true_extent));
    MPIR_SCHED_CHKPMEM_MALLOC(tmp_buf, void *, count*(MPIR_MAX(extent,true_extent)), mpi_errno, "temporary buffer");

    /* adjust for potential negative lower bound in datatype */
    tmp_buf = (void *)((char*)tmp_buf - true_lb);

    /* copy local data into recvbuf */
    if (sendbuf != MPI_IN_PLACE) {
        mpi_errno = MPID_Sched_copy(sendbuf, count, datatype,
                                    recvbuf, count, datatype, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        MPID_SCHED_BARRIER(s);
    }

    /* find nearest power-of-two less than or equal to comm_size */
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
            mpi_errno = MPID_Sched_send(recvbuf, count, datatype, rank+1, comm_ptr, s);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            MPID_SCHED_BARRIER(s);

            /* temporarily set the rank to -1 so that this
               process does not pariticipate in recursive
               doubling */
            newrank = -1;
        }
        else { /* odd */
            mpi_errno = MPID_Sched_recv(tmp_buf, count, datatype, rank-1, comm_ptr, s);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            MPID_SCHED_BARRIER(s);

            /* do the reduction on received data. since the
               ordering is right, it doesn't matter whether
               the operation is commutative or not. */
            mpi_errno = MPID_Sched_reduce(tmp_buf, recvbuf, count, datatype, op, s);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            MPID_SCHED_BARRIER(s);

            /* change the rank */
            newrank = rank / 2;
        }
    }
    else  /* rank >= 2*rem */
        newrank = rank - rem;

    if (newrank != -1) {
        mask = 0x1;
        while (mask < pof2) {
            newdst = newrank ^ mask;
            /* find real rank of dest */
            dst = (newdst < rem) ? newdst*2 + 1 : newdst + rem;

            /* Send the most current data, which is in recvbuf. Recv
               into tmp_buf */
            mpi_errno = MPID_Sched_recv(tmp_buf, count, datatype, dst, comm_ptr, s);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            /* sendrecv, no barrier here */
            mpi_errno = MPID_Sched_send(recvbuf, count, datatype,
                                        dst, comm_ptr, s);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            MPID_SCHED_BARRIER(s);

            /* tmp_buf contains data received in this step.
               recvbuf contains data accumulated so far */

            if (is_commutative  || (dst < rank)) {
                /* op is commutative OR the order is already right */
                mpi_errno = MPID_Sched_reduce(tmp_buf, recvbuf, count, datatype, op, s);
                if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                MPID_SCHED_BARRIER(s);
            }
            else {
                /* op is noncommutative and the order is not right */
                mpi_errno = MPID_Sched_reduce(recvbuf, tmp_buf, count, datatype, op, s);
                if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                MPID_SCHED_BARRIER(s);

                /* copy result back into recvbuf */
                mpi_errno = MPID_Sched_copy(tmp_buf, count, datatype,
                                            recvbuf, count, datatype, s);
                if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                MPID_SCHED_BARRIER(s);
            }
            mask <<= 1;
        }
    }

    /* In the non-power-of-two case, all odd-numbered
       processes of rank < 2*rem send the result to
       (rank-1), the ranks who didn't participate above. */
    if (rank < 2*rem) {
        if (rank % 2) { /* odd */
            mpi_errno = MPID_Sched_send(recvbuf, count, datatype, rank-1, comm_ptr, s);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        }
        else { /* even */
            mpi_errno = MPID_Sched_recv(recvbuf, count, datatype, rank+1, comm_ptr, s);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        }
    }

    MPIR_SCHED_CHKPMEM_COMMIT(s);
fn_exit:
    return mpi_errno;
fn_fail:
    MPIR_SCHED_CHKPMEM_REAP(s);
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Iallreduce_intra
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Iallreduce_intra(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPID_Comm *comm_ptr, MPID_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int comm_size, is_homogeneous, pof2, type_size;

    MPIU_Assert(comm_ptr->comm_kind == MPID_INTRACOMM);

    is_homogeneous = TRUE;
#ifdef MPID_HAS_HETERO
    if (comm_ptr->is_hetero)
        is_homogeneous = FALSE;
#endif

    if (!is_homogeneous) {
        mpi_errno = MPIR_Iallreduce_naive(sendbuf, recvbuf, count, datatype, op, comm_ptr, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        goto fn_exit;
    }

    comm_size = comm_ptr->local_size;

    MPID_Datatype_get_size_macro(datatype, type_size);

    /* find nearest power-of-two less than or equal to comm_size */
    pof2 = 1;
    while (pof2 <= comm_size) pof2 <<= 1;
    pof2 >>=1;

    /* If op is user-defined or count is less than pof2, use
       recursive doubling algorithm. Otherwise do a reduce-scatter
       followed by allgather. (If op is user-defined,
       derived datatypes are allowed and the user could pass basic
       datatypes on one process and derived on another as long as
       the type maps are the same. Breaking up derived
       datatypes to do the reduce-scatter is tricky, therefore
       using recursive doubling in that case.) */

    if ((count*type_size <= MPIR_CVAR_ALLREDUCE_SHORT_MSG_SIZE) ||
        (HANDLE_GET_KIND(op) != HANDLE_KIND_BUILTIN) ||
        (count < pof2))
    {
        /* use recursive doubling */
        mpi_errno = MPIR_Iallreduce_rec_dbl(sendbuf, recvbuf, count, datatype, op, comm_ptr, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }
    else {
        /* do a reduce-scatter followed by allgather */
        mpi_errno = MPIR_Iallreduce_redscat_allgather(sendbuf, recvbuf, count, datatype, op, comm_ptr, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Iallreduce_inter
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Iallreduce_inter(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPID_Comm *comm_ptr, MPID_Sched_t s)
{
/* Intercommunicator Allreduce.
   We first do an intercommunicator reduce to rank 0 on left group,
   then an intercommunicator reduce to rank 0 on right group, followed
   by local intracommunicator broadcasts in each group.

   We don't do local reduces first and then intercommunicator
   broadcasts because it would require allocation of a temporary buffer.
*/
    int mpi_errno = MPI_SUCCESS;
    int rank, root;
    MPID_Comm *lcomm_ptr = NULL;

    MPIU_Assert(comm_ptr->comm_kind == MPID_INTERCOMM);

    rank = comm_ptr->rank;

    /* first do a reduce from right group to rank 0 in left group,
       then from left group to rank 0 in right group*/
    if (comm_ptr->is_low_group) {
        /* reduce from right group to rank 0*/
        root = (rank == 0) ? MPI_ROOT : MPI_PROC_NULL;
        mpi_errno = MPIR_Ireduce_inter(sendbuf, recvbuf, count, datatype, op, root, comm_ptr, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);

        /* no barrier, these reductions can be concurrent */

        /* reduce to rank 0 of right group */
        root = 0;
        mpi_errno = MPIR_Ireduce_inter(sendbuf, recvbuf, count, datatype, op, root, comm_ptr, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }
    else {
        /* reduce to rank 0 of left group */
        root = 0;
        mpi_errno = MPIR_Ireduce_inter(sendbuf, recvbuf, count, datatype, op, root, comm_ptr, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);

        /* no barrier, these reductions can be concurrent */

        /* reduce from right group to rank 0 */
        root = (rank == 0) ? MPI_ROOT : MPI_PROC_NULL;
        mpi_errno = MPIR_Ireduce_inter(sendbuf, recvbuf, count, datatype, op, root, comm_ptr, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }

    /* don't bcast until the reductions have finished */
    mpi_errno = MPID_Sched_barrier(s);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

    /* Get the local intracommunicator */
    if (!comm_ptr->local_comm) {
        MPIR_Setup_intercomm_localcomm( comm_ptr );
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }
    lcomm_ptr = comm_ptr->local_comm;

    MPIU_Assert(lcomm_ptr->coll_fns && lcomm_ptr->coll_fns->Ibcast_sched);
    mpi_errno = lcomm_ptr->coll_fns->Ibcast_sched(recvbuf, count, datatype, 0, lcomm_ptr, s);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPIR_Iallreduce_SMP
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Iallreduce_SMP(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPID_Comm *comm_ptr, MPID_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int is_commutative;
    MPID_Comm *nc;
    MPID_Comm *nrc;

    if (!MPIR_CVAR_ENABLE_SMP_COLLECTIVES || !MPIR_CVAR_ENABLE_SMP_ALLREDUCE)
        MPID_Abort(comm_ptr, MPI_ERR_OTHER, 1, "SMP collectives are disabled!");
    MPIU_Assert(MPIR_Comm_is_node_aware(comm_ptr));

    nc = comm_ptr->node_comm;
    nrc = comm_ptr->node_roots_comm;

    is_commutative = MPIR_Op_is_commutative(op);

    /* is the op commutative? We do SMP optimizations only if it is. */
    if (!is_commutative) {
        /* use flat fallback */
        mpi_errno = MPIR_Iallreduce_intra(sendbuf, recvbuf, count, datatype, op, comm_ptr, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        goto fn_exit;
    }

    /* on each node, do a reduce to the local root */
    if (nc != NULL) {
        /* take care of the MPI_IN_PLACE case. For reduce,
           MPI_IN_PLACE is specified only on the root;
           for allreduce it is specified on all processes. */
        MPIU_Assert(nc->coll_fns && nc->coll_fns->Ireduce_sched);

        if ((sendbuf == MPI_IN_PLACE) && (comm_ptr->node_comm->rank != 0)) {
            /* IN_PLACE and not root of reduce. Data supplied to this
               allreduce is in recvbuf. Pass that as the sendbuf to reduce. */
            mpi_errno = nc->coll_fns->Ireduce_sched(recvbuf, NULL, count, datatype, op, 0, nc, s);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        } else {
            mpi_errno = nc->coll_fns->Ireduce_sched(sendbuf, recvbuf, count, datatype, op, 0, nc, s);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        }
        MPID_SCHED_BARRIER(s);
    } else {
        /* only one process on the node. copy sendbuf to recvbuf */
        if (sendbuf != MPI_IN_PLACE) {
            mpi_errno = MPID_Sched_copy(sendbuf, count, datatype,
                                        recvbuf, count, datatype, s);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        }
        MPID_SCHED_BARRIER(s);
    }

    /* now do an IN_PLACE allreduce among the local roots of all nodes */
    if (nrc != NULL) {
        MPIU_Assert(nrc->coll_fns && nrc->coll_fns->Iallreduce_sched);
        mpi_errno = nrc->coll_fns->Iallreduce_sched(MPI_IN_PLACE, recvbuf, count, datatype, op, nrc, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        MPID_SCHED_BARRIER(s);
    }

    /* now broadcast the result among local processes */
    if (comm_ptr->node_comm != NULL) {
        MPIU_Assert(nc->coll_fns && nc->coll_fns->Ibcast_sched);
        mpi_errno = nc->coll_fns->Ibcast_sched(recvbuf, count, datatype, 0, nc, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        MPID_SCHED_BARRIER(s);
    }

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPIR_Iallreduce_impl
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Iallreduce_impl(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPID_Comm *comm_ptr, MPI_Request *request)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Request *reqp = NULL;
    int tag = -1;
    MPID_Sched_t s = MPID_SCHED_NULL;

    *request = MPI_REQUEST_NULL;

    MPIU_Assert(comm_ptr->coll_fns != NULL);
    if (comm_ptr->coll_fns->Iallreduce_req != NULL) {
        /* --BEGIN USEREXTENSION-- */
        mpi_errno = comm_ptr->coll_fns->Iallreduce_req(sendbuf, recvbuf, count, datatype, op, comm_ptr, &reqp);
        if (reqp) {
            *request = reqp->handle;
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            goto fn_exit;
        }
        /* --END USEREXTENSION-- */
    }

    mpi_errno = MPID_Sched_next_tag(comm_ptr, &tag);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    mpi_errno = MPID_Sched_create(&s);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

    MPIU_Assert(comm_ptr->coll_fns != NULL);
    MPIU_Assert(comm_ptr->coll_fns->Iallreduce_sched != NULL);
    mpi_errno = comm_ptr->coll_fns->Iallreduce_sched(sendbuf, recvbuf, count, datatype, op, comm_ptr, s);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

    mpi_errno = MPID_Sched_start(&s, comm_ptr, tag, &reqp);
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
#define FUNCNAME MPI_Iallreduce
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@
MPI_Iallreduce - Combines values from all processes and distributes the result
                 back to all processes in a nonblocking way

Input Parameters:
+ sendbuf - starting address of the send buffer (choice)
. count - number of elements in send buffer (non-negative integer)
. datatype - data type of elements of send buffer (handle)
. op - operation (handle)
- comm - communicator (handle)

Output Parameters:
+ recvbuf - starting address of the receive buffer (choice)
- request - communication request (handle)

.N ThreadSafe

.N Fortran

.N Errors
@*/
int MPI_Iallreduce(const void *sendbuf, void *recvbuf, int count,
                   MPI_Datatype datatype, MPI_Op op, MPI_Comm comm,
                   MPI_Request *request)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_IALLREDUCE);

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_IALLREDUCE);

    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS
        {
            MPIR_ERRTEST_DATATYPE(datatype, "datatype", mpi_errno);
            MPIR_ERRTEST_COUNT(count, mpi_errno);
            MPIR_ERRTEST_OP(op, mpi_errno);
            MPIR_ERRTEST_COMM(comm, mpi_errno);

            /* TODO more checks may be appropriate */
        }
        MPID_END_ERROR_CHECKS
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* Convert MPI object handles to object pointers */
    MPID_Comm_get_ptr(comm, comm_ptr);

    /* Validate parameters and objects (post conversion) */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS
        {
            MPID_Comm_valid_ptr( comm_ptr, mpi_errno, FALSE );
            if (HANDLE_GET_KIND(datatype) != HANDLE_KIND_BUILTIN) {
                MPID_Datatype *datatype_ptr = NULL;
                MPID_Datatype_get_ptr(datatype, datatype_ptr);
                MPID_Datatype_valid_ptr(datatype_ptr, mpi_errno);
                if (mpi_errno != MPI_SUCCESS) goto fn_fail;
                MPID_Datatype_committed_ptr(datatype_ptr, mpi_errno);
                if (mpi_errno != MPI_SUCCESS) goto fn_fail;
            }

            if (HANDLE_GET_KIND(op) != HANDLE_KIND_BUILTIN) {
                MPID_Op *op_ptr = NULL;
                MPID_Op_get_ptr(op, op_ptr);
                MPID_Op_valid_ptr(op_ptr, mpi_errno);
            }
            else if (HANDLE_GET_KIND(op) == HANDLE_KIND_BUILTIN) {
                mpi_errno = ( * MPIR_OP_HDL_TO_DTYPE_FN(op) )(datatype);
            }
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;

            if (comm_ptr->comm_kind == MPID_INTERCOMM)
                MPIR_ERRTEST_SENDBUF_INPLACE(sendbuf, count, mpi_errno);

            if (sendbuf != MPI_IN_PLACE)
                MPIR_ERRTEST_USERBUFFER(sendbuf,count,datatype,mpi_errno);

            MPIR_ERRTEST_ARGNULL(request,"request", mpi_errno);

            if (comm_ptr->comm_kind == MPID_INTRACOMM && count != 0 && sendbuf != MPI_IN_PLACE)
                MPIR_ERRTEST_ALIAS_COLL(sendbuf, recvbuf, mpi_errno);

            /* TODO more checks may be appropriate (counts, in_place, buffer aliasing, etc) */
        }
        MPID_END_ERROR_CHECKS
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    mpi_errno = MPIR_Iallreduce_impl(sendbuf, recvbuf, count, datatype, op, comm_ptr, request);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

    /* ... end of body of routine ... */

fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_IALLREDUCE);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno = MPIR_Err_create_code(
            mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
            "**mpi_iallreduce", "**mpi_iallreduce %p %p %d %D %O %C %p", sendbuf, recvbuf, count, datatype, op, comm, request);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm(comm_ptr, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
    goto fn_exit;
}
