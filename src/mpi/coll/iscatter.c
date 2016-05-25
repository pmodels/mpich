/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2010 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Iscatter */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Iscatter = PMPI_Iscatter
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Iscatter  MPI_Iscatter
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Iscatter as PMPI_Iscatter
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Iscatter(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                 int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm,
                 MPI_Request *request)
                 __attribute__((weak,alias("PMPI_Iscatter")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Iscatter
#define MPI_Iscatter PMPI_Iscatter

/* helper callbacks and associated state structures */
struct shared_state {
    int sendcount;
    int curr_count;
    MPI_Aint send_subtree_count;
    int nbytes;
    MPI_Status status;
};
static int get_count(MPIR_Comm *comm, int tag, void *state)
{
    struct shared_state *ss = state;
    MPIR_Get_count_impl(&ss->status, MPI_BYTE, &ss->curr_count);
    return MPI_SUCCESS;
}
static int calc_send_count_root(MPIR_Comm *comm, int tag, void *state, void *state2)
{
    struct shared_state *ss = state;
    int mask = (int)(size_t)state2;
    ss->send_subtree_count = ss->curr_count - ss->sendcount * mask;
    return MPI_SUCCESS;
}
static int calc_send_count_non_root(MPIR_Comm *comm, int tag, void *state, void *state2)
{
    struct shared_state *ss = state;
    int mask = (int)(size_t)state2;
    ss->send_subtree_count = ss->curr_count - ss->nbytes * mask;
    return MPI_SUCCESS;
}
static int calc_curr_count(MPIR_Comm *comm, int tag, void *state)
{
    struct shared_state *ss = state;
    ss->curr_count -= ss->send_subtree_count;
    return MPI_SUCCESS;
}

/* any non-MPI functions go here, especially non-static ones */

/* This is the default implementation of scatter. The algorithm is:

   Algorithm: MPI_Scatter

   We use a binomial tree algorithm for both short and
   long messages. At nodes other than leaf nodes we need to allocate
   a temporary buffer to store the incoming message. If the root is
   not rank 0, we reorder the sendbuf in order of relative ranks by
   copying it into a temporary buffer, so that all the sends from the
   root are contiguous and in the right order. In the heterogeneous
   case, we first pack the buffer by using MPI_Pack and then do the
   scatter.

   Cost = lgp.alpha + n.((p-1)/p).beta
   where n is the total size of the data to be scattered from the root.

   Possible improvements:

   End Algorithm: MPI_Scatter
*/
#undef FUNCNAME
#define FUNCNAME MPIR_Iscatter_intra
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Iscatter_intra(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                        void *recvbuf, int recvcount, MPI_Datatype recvtype,
                        int root, MPIR_Comm *comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    MPI_Aint extent = 0;
    int rank, comm_size, is_homogeneous, sendtype_size;
    int relative_rank;
    int mask, recvtype_size=0, src, dst;
    int tmp_buf_size = 0;
    void *tmp_buf = NULL;
    struct shared_state *ss = NULL;
    MPIR_SCHED_CHKPMEM_DECL(4);

    comm_size = comm_ptr->local_size;
    rank = comm_ptr->rank;

    if (((rank == root) && (sendcount == 0)) || ((rank != root) && (recvcount == 0)))
        goto fn_exit;

    is_homogeneous = 1;
#ifdef MPID_HAS_HETERO
    if (comm_ptr->is_hetero)
        is_homogeneous = 0;
#endif

/* Use binomial tree algorithm */

    MPIR_SCHED_CHKPMEM_MALLOC(ss, struct shared_state *, sizeof(struct shared_state), mpi_errno, "shared_state");
    ss->sendcount = sendcount;

    if (rank == root)
        MPIR_Datatype_get_extent_macro(sendtype, extent);

    relative_rank = (rank >= root) ? rank - root : rank - root + comm_size;

    if (is_homogeneous) {
        /* communicator is homogeneous */
        if (rank == root) {
            /* We separate the two cases (root and non-root) because
               in the event of recvbuf=MPI_IN_PLACE on the root,
               recvcount and recvtype are not valid */
            MPIR_Datatype_get_size_macro(sendtype, sendtype_size);
            MPIR_Ensure_Aint_fits_in_pointer(MPIR_VOID_PTR_CAST_TO_MPI_AINT sendbuf +
                                             extent*sendcount*comm_size);

            ss->nbytes = sendtype_size * sendcount;
        }
        else {
            MPIR_Datatype_get_size_macro(recvtype, recvtype_size);
            MPIR_Ensure_Aint_fits_in_pointer(extent*recvcount*comm_size);
            ss->nbytes = recvtype_size * recvcount;
        }

        ss->curr_count = 0;

        /* all even nodes other than root need a temporary buffer to
           receive data of max size (ss->nbytes*comm_size)/2 */
        if (relative_rank && !(relative_rank % 2)) {
            tmp_buf_size = (ss->nbytes*comm_size)/2;
            MPIR_SCHED_CHKPMEM_MALLOC(tmp_buf, void *, tmp_buf_size, mpi_errno, "tmp_buf");
        }

        /* if the root is not rank 0, we reorder the sendbuf in order of
           relative ranks and copy it into a temporary buffer, so that
           all the sends from the root are contiguous and in the right
           order. */
        if (rank == root) {
            if (root != 0) {
                tmp_buf_size = ss->nbytes*comm_size;
                MPIR_SCHED_CHKPMEM_MALLOC(tmp_buf, void *, tmp_buf_size, mpi_errno, "tmp_buf");

                if (recvbuf != MPI_IN_PLACE)
                    mpi_errno = MPIR_Sched_copy(((char *) sendbuf + extent*sendcount*rank),
                                                sendcount*(comm_size-rank), sendtype,
                                                tmp_buf, ss->nbytes*(comm_size-rank), MPI_BYTE, s);
                else
                    mpi_errno = MPIR_Sched_copy(((char *) sendbuf + extent*sendcount*(rank+1)),
                                                sendcount*(comm_size-rank-1), sendtype,
                                                ((char *)tmp_buf + ss->nbytes),
                                                ss->nbytes*(comm_size-rank-1), MPI_BYTE, s);
                if (mpi_errno) MPIR_ERR_POP(mpi_errno);

                mpi_errno = MPIR_Sched_copy(sendbuf, sendcount*rank, sendtype,
                                            ((char *) tmp_buf + ss->nbytes*(comm_size-rank)),
                                            ss->nbytes*rank, MPI_BYTE, s);
                if (mpi_errno) MPIR_ERR_POP(mpi_errno);

                MPIR_SCHED_BARRIER(s);
                ss->curr_count = ss->nbytes*comm_size;
            }
            else
                ss->curr_count = sendcount*comm_size;
        }

        /* root has all the data; others have zero so far */

        mask = 0x1;
        while (mask < comm_size) {
            if (relative_rank & mask) {
                src = rank - mask;
                if (src < 0) src += comm_size;

                /* The leaf nodes receive directly into recvbuf because
                   they don't have to forward data to anyone. Others
                   receive data into a temporary buffer. */
                if (relative_rank % 2) {
                    mpi_errno = MPIR_Sched_recv(recvbuf, recvcount, recvtype, src, comm_ptr, s);
                    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                    MPIR_SCHED_BARRIER(s);
                }
                else {

                    /* the recv size is larger than what may be sent in
                       some cases. query amount of data actually received */
                    mpi_errno = MPIR_Sched_recv_status(tmp_buf, tmp_buf_size, MPI_BYTE, src, comm_ptr, &ss->status, s);
                    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                    MPIR_SCHED_BARRIER(s);
                    mpi_errno = MPIR_Sched_cb(&get_count, ss, s);
                    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                    MPIR_SCHED_BARRIER(s);
                }
                break;
            }
            mask <<= 1;
        }

        /* This process is responsible for all processes that have bits
           set from the LSB upto (but not including) mask.  Because of
           the "not including", we start by shifting mask back down
           one. */

        mask >>= 1;
        while (mask > 0) {
            if (relative_rank + mask < comm_size) {
                dst = rank + mask;
                if (dst >= comm_size) dst -= comm_size;

                if ((rank == root) && (root == 0))
                {
#if 0
                    /* FIXME how can this be right? shouldn't (sendcount*mask)
                     * be the amount sent and curr_cnt be reduced by that?  Or
                     * is it always true the (curr_cnt/2==sendcount*mask)? */
                    send_subtree_cnt = curr_cnt - sendcount * mask;
#endif
                    mpi_errno = MPIR_Sched_cb2(&calc_send_count_root, ss, ((void *)(size_t)mask), s);
                    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                    MPIR_SCHED_BARRIER(s);

                    /* mask is also the size of this process's subtree */
                    mpi_errno = MPIR_Sched_send_defer(((char *)sendbuf + extent*sendcount*mask),
                                                      &ss->send_subtree_count, sendtype, dst,
                                                      comm_ptr, s);
                    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                    MPIR_SCHED_BARRIER(s);
                }
                else
                {
                    /* non-zero root and others */
                    mpi_errno = MPIR_Sched_cb2(&calc_send_count_non_root, ss, ((void *)(size_t)mask), s);
                    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                    MPIR_SCHED_BARRIER(s);

                    /* mask is also the size of this process's subtree */
                    mpi_errno = MPIR_Sched_send_defer(((char *)tmp_buf + ss->nbytes*mask),
                                                      &ss->send_subtree_count, MPI_BYTE, dst,
                                                      comm_ptr, s);
                    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                    MPIR_SCHED_BARRIER(s);
                }
                mpi_errno = MPIR_Sched_cb(&calc_curr_count, ss, s);
                if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                MPIR_SCHED_BARRIER(s);
            }
            mask >>= 1;
        }

        if ((rank == root) && (root == 0) && (recvbuf != MPI_IN_PLACE)) {
            /* for root=0, put root's data in recvbuf if not MPI_IN_PLACE */
            mpi_errno = MPIR_Sched_copy(sendbuf, sendcount, sendtype,
                                        recvbuf, recvcount, recvtype, s);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            MPIR_SCHED_BARRIER(s);
        }
        else if (!(relative_rank % 2) && (recvbuf != MPI_IN_PLACE)) {
            /* for non-zero root and non-leaf nodes, copy from tmp_buf
               into recvbuf */
            mpi_errno = MPIR_Sched_copy(tmp_buf, ss->nbytes, MPI_BYTE,
                                        recvbuf, recvcount, recvtype, s);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            MPIR_SCHED_BARRIER(s);
        }

    }
#ifdef MPID_HAS_HETERO
    else { /* communicator is heterogeneous */
        int position;
        MPIR_Assertp(FALSE); /* hetero case not yet implemented */

        if (rank == root) {
            MPIR_Pack_size_impl(sendcount*comm_size, sendtype, &tmp_buf_size);

            MPIR_CHKLMEM_MALLOC(tmp_buf, void *, tmp_buf_size, mpi_errno, "tmp_buf");

          /* calculate the value of nbytes, the number of bytes in packed
             representation that each process receives. We can't
             accurately calculate that from tmp_buf_size because
             MPI_Pack_size returns an upper bound on the amount of memory
             required. (For example, for a single integer, MPICH-1 returns
             pack_size=12.) Therefore, we actually pack some data into
             tmp_buf and see by how much 'position' is incremented. */

            position = 0;
            mpi_errno = MPIR_Pack_impl(sendbuf, 1, sendtype, tmp_buf, tmp_buf_size, &position);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);

            nbytes = position*sendcount;

            curr_cnt = nbytes*comm_size;

            if (root == 0) {
                if (recvbuf != MPI_IN_PLACE) {
                    position = 0;
                    mpi_errno = MPIR_Pack_impl(sendbuf, sendcount*comm_size, sendtype, tmp_buf,
                                               tmp_buf_size, &position);
                    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                }
                else {
                    position = nbytes;
                    mpi_errno = MPIR_Pack_impl(((char *) sendbuf + extent*sendcount),
                                               sendcount*(comm_size-1), sendtype, tmp_buf,
                                               tmp_buf_size, &position);
                    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                }
            }
            else {
                if (recvbuf != MPI_IN_PLACE) {
                    position = 0;
                    mpi_errno = MPIR_Pack_impl(((char *) sendbuf + extent*sendcount*rank),
                                               sendcount*(comm_size-rank), sendtype, tmp_buf,
                                               tmp_buf_size, &position);
                    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                }
                else {
                    position = nbytes;
                    mpi_errno = MPIR_Pack_impl(((char *) sendbuf + extent*sendcount*(rank+1)),
                                               sendcount*(comm_size-rank-1), sendtype, tmp_buf,
                                               tmp_buf_size, &position);
                    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                }
                mpi_errno = MPIR_Pack_impl(sendbuf, sendcount*rank, sendtype, tmp_buf,
                                           tmp_buf_size, &position);
                if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            }
        }
        else {
            MPIR_Pack_size_impl(recvcount*(comm_size/2), recvtype, &tmp_buf_size);
            MPIR_CHKLMEM_MALLOC(tmp_buf, void *, tmp_buf_size, mpi_errno, "tmp_buf");

            /* calculate nbytes */
            position = 0;
            mpi_errno = MPIR_Pack_impl(recvbuf, 1, recvtype, tmp_buf, tmp_buf_size, &position);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            nbytes = position*recvcount;

            curr_cnt = 0;
        }

        mask = 0x1;
        while (mask < comm_size) {
            if (relative_rank & mask) {
                src = rank - mask;
                if (src < 0) src += comm_size;

                mpi_errno = MPIC_Recv(tmp_buf, tmp_buf_size, MPI_BYTE, src,
                                         MPIR_SCATTER_TAG, comm_ptr, &status, errflag);
                if (mpi_errno) {
                    /* for communication errors, just record the error but continue */
                    *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                    MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                    MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                    curr_cnt = 0;
                } else
                    /* the recv size is larger than what may be sent in
                       some cases. query amount of data actually received */
                    MPIR_Get_count_impl(&status, MPI_BYTE, &curr_cnt);
                break;
            }
            mask <<= 1;
        }

        /* This process is responsible for all processes that have bits
           set from the LSB upto (but not including) mask.  Because of
           the "not including", we start by shifting mask back down
           one. */

        mask >>= 1;
        while (mask > 0) {
            if (relative_rank + mask < comm_size) {
                dst = rank + mask;
                if (dst >= comm_size) dst -= comm_size;

                send_subtree_cnt = curr_cnt - nbytes * mask;
                /* mask is also the size of this process's subtree */
                mpi_errno = MPIC_Send(((char *)tmp_buf + nbytes*mask),
                                         send_subtree_cnt, MPI_BYTE, dst,
                                         MPIR_SCATTER_TAG, comm_ptr, errflag);
                if (mpi_errno) {
                    /* for communication errors, just record the error but continue */
                    *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                    MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                    MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                }
                curr_cnt -= send_subtree_cnt;
            }
            mask >>= 1;
        }

        /* copy local data into recvbuf */
        position = 0;
        if (recvbuf != MPI_IN_PLACE) {
            mpi_errno = MPIR_Unpack_impl(tmp_buf, tmp_buf_size, &position, recvbuf,
                                         recvcount, recvtype);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        }
    }
#endif /* MPID_HAS_HETERO */


    MPIR_SCHED_CHKPMEM_COMMIT(s);
 fn_exit:
    return mpi_errno;
 fn_fail:
    MPIR_SCHED_CHKPMEM_REAP(s);
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Iscatter_inter
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Iscatter_inter(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                        void *recvbuf, int recvcount, MPI_Datatype recvtype,
                        int root, MPIR_Comm *comm_ptr, MPIR_Sched_t s)
{
/*  Intercommunicator scatter.
    For short messages, root sends to rank 0 in remote group. rank 0
    does local intracommunicator scatter (binomial tree).
    Cost: (lgp+1).alpha + n.((p-1)/p).beta + n.beta

    For long messages, we use linear scatter to avoid the extra n.beta.
    Cost: p.alpha + n.beta
*/
    int mpi_errno = MPI_SUCCESS;
    int rank, local_size, remote_size;
    int i, nbytes, sendtype_size, recvtype_size;
    MPI_Aint extent, true_extent, true_lb = 0;
    void *tmp_buf = NULL;
    MPIR_Comm *newcomm_ptr = NULL;
    MPIR_SCHED_CHKPMEM_DECL(1);

    if (root == MPI_PROC_NULL) {
        /* local processes other than root do nothing */
        goto fn_exit;
    }

    remote_size = comm_ptr->remote_size;
    local_size  = comm_ptr->local_size;

    if (root == MPI_ROOT) {
        MPIR_Datatype_get_size_macro(sendtype, sendtype_size);
        nbytes = sendtype_size * sendcount * remote_size;
    }
    else {
        /* remote side */
        MPIR_Datatype_get_size_macro(recvtype, recvtype_size);
        nbytes = recvtype_size * recvcount * local_size;
    }

    if (nbytes < MPIR_CVAR_SCATTER_INTER_SHORT_MSG_SIZE) {
        if (root == MPI_ROOT) {
            /* root sends all data to rank 0 on remote group and returns */
            mpi_errno = MPIR_Sched_send(sendbuf, sendcount*remote_size, sendtype, 0, comm_ptr, s);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            MPIR_SCHED_BARRIER(s);
            goto fn_exit;
        }
        else {
            /* remote group. rank 0 receives data from root. need to
               allocate temporary buffer to store this data. */
            rank = comm_ptr->rank;

            if (rank == 0) {
                MPIR_Type_get_true_extent_impl(recvtype, &true_lb, &true_extent);

                MPIR_Datatype_get_extent_macro(recvtype, extent);
                MPIR_Ensure_Aint_fits_in_pointer(extent*recvcount*local_size);
                MPIR_Ensure_Aint_fits_in_pointer(MPIR_VOID_PTR_CAST_TO_MPI_AINT sendbuf +
                                                 sendcount*remote_size*extent);

                MPIR_SCHED_CHKPMEM_MALLOC(tmp_buf, void *, recvcount*local_size*(MPL_MAX(extent,true_extent)),
                                          mpi_errno, "tmp_buf");

                /* adjust for potential negative lower bound in datatype */
                tmp_buf = (void *)((char*)tmp_buf - true_lb);

                mpi_errno = MPIR_Sched_recv(tmp_buf, recvcount*local_size, recvtype, root, comm_ptr, s);
                if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                MPIR_SCHED_BARRIER(s);
            }

            /* Get the local intracommunicator */
            if (!comm_ptr->local_comm)
                MPII_Setup_intercomm_localcomm(comm_ptr);

            newcomm_ptr = comm_ptr->local_comm;

            /* now do the usual scatter on this intracommunicator */
            MPIR_Assert(newcomm_ptr->coll_fns != NULL);
            MPIR_Assert(newcomm_ptr->coll_fns->Iscatter_sched != NULL);
            mpi_errno = newcomm_ptr->coll_fns->Iscatter_sched(tmp_buf, recvcount, recvtype,
                                                        recvbuf, recvcount, recvtype,
                                                        0, newcomm_ptr, s);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            MPIR_SCHED_BARRIER(s);
        }
    }
    else {
        /* long message. use linear algorithm. */
        if (root == MPI_ROOT) {
            MPIR_Datatype_get_extent_macro(sendtype, extent);
            for (i = 0; i < remote_size; i++) {
                mpi_errno = MPIR_Sched_send(((char *)sendbuf+sendcount*i*extent),
                                            sendcount, sendtype, i, comm_ptr, s);
                if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            }
            MPIR_SCHED_BARRIER(s);
        }
        else {
            mpi_errno = MPIR_Sched_recv(recvbuf, recvcount, recvtype, root, comm_ptr, s);
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

#undef FUNCNAME
#define FUNCNAME MPIR_Iscatter_impl
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Iscatter_impl(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPIR_Comm *comm_ptr, MPI_Request *request)
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

    MPIR_Assert(comm_ptr->coll_fns->Iscatter_sched != NULL);
    mpi_errno = comm_ptr->coll_fns->Iscatter_sched(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm_ptr, s);
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
#define FUNCNAME MPI_Iscatter
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@
MPI_Iscatter - Sends data from one process to all other processes in a
               communicator in a nonblocking way

Input Parameters:
+ sendbuf - address of send buffer (significant only at root) (choice)
. sendcount - number of elements sent to each process (significant only at root) (non-negative integer)
. sendtype - data type of send buffer elements (significant only at root) (handle)
. recvcount - number of elements in receive buffer (non-negative integer)
. recvtype - data type of receive buffer elements (handle)
. root - rank of sending process (integer)
- comm - communicator (handle)

Output Parameters:
+ recvbuf - starting address of the receive buffer (choice)
- request - communication request (handle)

.N ThreadSafe

.N Fortran

.N Errors
@*/
int MPI_Iscatter(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                 void *recvbuf, int recvcount, MPI_Datatype recvtype, int root,
                 MPI_Comm comm, MPI_Request *request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *comm_ptr = NULL;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_ISCATTER);

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_ISCATTER);

    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS
        {
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
            MPIR_Datatype *sendtype_ptr, *recvtype_ptr;
            MPIR_Comm_valid_ptr( comm_ptr, mpi_errno, FALSE );
            if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
                MPIR_ERRTEST_INTRA_ROOT(comm_ptr, root, mpi_errno);

                if (comm_ptr->rank == root) {
                    MPIR_ERRTEST_COUNT(sendcount, mpi_errno);
                    MPIR_ERRTEST_DATATYPE(sendtype, "sendtype", mpi_errno);
                    if (HANDLE_GET_KIND(sendtype) != HANDLE_KIND_BUILTIN) {
                        MPIR_Datatype_get_ptr(sendtype, sendtype_ptr);
                        MPIR_Datatype_valid_ptr(sendtype_ptr, mpi_errno);
                        if (mpi_errno != MPI_SUCCESS) goto fn_fail;
                        MPIR_Datatype_committed_ptr(sendtype_ptr, mpi_errno);
                        if (mpi_errno != MPI_SUCCESS) goto fn_fail;
                    }
                    MPIR_ERRTEST_USERBUFFER(sendbuf,sendcount,sendtype,mpi_errno);
                    MPIR_ERRTEST_SENDBUF_INPLACE(sendbuf, sendcount, mpi_errno);

                    /* catch common aliasing cases */
                    if (recvbuf != MPI_IN_PLACE && sendtype == recvtype && sendcount == recvcount && recvcount != 0) {
                        int sendtype_size;
                        MPIR_Datatype_get_size_macro(sendtype, sendtype_size);
                        MPIR_ERRTEST_ALIAS_COLL(recvbuf, (char*)sendbuf + comm_ptr->rank*sendcount*sendtype_size, mpi_errno);
                    }
                }
                else
                    MPIR_ERRTEST_RECVBUF_INPLACE(recvbuf, recvcount, mpi_errno);

                if (recvbuf != MPI_IN_PLACE) {
                    MPIR_ERRTEST_COUNT(recvcount, mpi_errno);
                    MPIR_ERRTEST_DATATYPE(recvtype, "recvtype", mpi_errno);
                    if (HANDLE_GET_KIND(recvtype) != HANDLE_KIND_BUILTIN) {
                        MPIR_Datatype_get_ptr(recvtype, recvtype_ptr);
                        MPIR_Datatype_valid_ptr(recvtype_ptr, mpi_errno);
                        if (mpi_errno != MPI_SUCCESS) goto fn_fail;
                        MPIR_Datatype_committed_ptr(recvtype_ptr, mpi_errno);
                        if (mpi_errno != MPI_SUCCESS) goto fn_fail;
                    }
                    MPIR_ERRTEST_USERBUFFER(recvbuf,recvcount,recvtype,mpi_errno);
                }
            }

            if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTERCOMM) {
                MPIR_ERRTEST_INTER_ROOT(comm_ptr, root, mpi_errno);

                if (root == MPI_ROOT) {
                    MPIR_ERRTEST_COUNT(sendcount, mpi_errno);
                    MPIR_ERRTEST_DATATYPE(sendtype, "sendtype", mpi_errno);
                    if (HANDLE_GET_KIND(sendtype) != HANDLE_KIND_BUILTIN) {
                        MPIR_Datatype_get_ptr(sendtype, sendtype_ptr);
                        MPIR_Datatype_valid_ptr(sendtype_ptr, mpi_errno);
                        if (mpi_errno != MPI_SUCCESS) goto fn_fail;
                        MPIR_Datatype_committed_ptr(sendtype_ptr, mpi_errno);
                        if (mpi_errno != MPI_SUCCESS) goto fn_fail;
                    }
                    MPIR_ERRTEST_SENDBUF_INPLACE(sendbuf, sendcount, mpi_errno);
                    MPIR_ERRTEST_USERBUFFER(sendbuf,sendcount,sendtype,mpi_errno);
                }
                else if (root != MPI_PROC_NULL) {
                    MPIR_ERRTEST_COUNT(recvcount, mpi_errno);
                    MPIR_ERRTEST_DATATYPE(recvtype, "recvtype", mpi_errno);
                    if (HANDLE_GET_KIND(recvtype) != HANDLE_KIND_BUILTIN) {
                        MPIR_Datatype_get_ptr(recvtype, recvtype_ptr);
                        MPIR_Datatype_valid_ptr(recvtype_ptr, mpi_errno);
                        if (mpi_errno != MPI_SUCCESS) goto fn_fail;
                        MPIR_Datatype_committed_ptr(recvtype_ptr, mpi_errno);
                        if (mpi_errno != MPI_SUCCESS) goto fn_fail;
                    }
                    MPIR_ERRTEST_RECVBUF_INPLACE(recvbuf, recvcount, mpi_errno);
                    MPIR_ERRTEST_USERBUFFER(recvbuf,recvcount,recvtype,mpi_errno);
                }
            }
        }
        MPID_END_ERROR_CHECKS
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    mpi_errno = MPID_Iscatter(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm_ptr, request);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

    /* ... end of body of routine ... */

fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_ISCATTER);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno = MPIR_Err_create_code(
            mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
            "**mpi_iscatter", "**mpi_iscatter %p %d %D %p %d %D %d %C %p", sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, request);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm(comm_ptr, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
