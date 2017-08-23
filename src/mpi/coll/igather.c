/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2010 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Igather */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Igather = PMPI_Igather
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Igather  MPI_Igather
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Igather as PMPI_Igather
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Igather(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm,
                MPI_Request *request)
                __attribute__((weak,alias("PMPI_Igather")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Igather
#define MPI_Igather PMPI_Igather

/* any non-MPI functions go here, especially non-static ones */

/* This is the default implementation of igather. The algorithm is:

   Algorithm: MPI_Igather

   We use a binomial tree algorithm for both short and long
   messages. At nodes other than leaf nodes we need to allocate a
   temporary buffer to store the incoming message. If the root is not
   rank 0, for very small messages, we pack it into a temporary
   contiguous buffer and reorder it to be placed in the right
   order. For small (but not very small) messages, we use a derived
   datatype to unpack the incoming data into non-contiguous buffers in
   the right order. In the heterogeneous case we first pack the
   buffers by using MPI_Pack and then do the gather.

   Cost = lgp.alpha + n.((p-1)/p).beta
   where n is the total size of the data gathered at the root.

   Possible improvements:

   End Algorithm: MPI_Gather
*/
#undef FUNCNAME
#define FUNCNAME MPIR_Igather_binomial
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Igather_binomial(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPIR_Comm *comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int comm_size, rank;
    int relative_rank, is_homogeneous;
    int mask, src, dst, relative_src;
    MPI_Aint recvtype_size, sendtype_size, curr_cnt=0, nbytes;
    int recvblks;
    int tmp_buf_size, missing;
    void *tmp_buf = NULL;
    int blocks[2];
    int displs[2];
    MPI_Aint struct_displs[2];
    MPI_Aint extent=0;
    int copy_offset = 0, copy_blks = 0;
    MPI_Datatype types[2], tmp_type;
    MPIR_SCHED_CHKPMEM_DECL(1);

    comm_size = comm_ptr->local_size;
    rank = comm_ptr->rank;

    if (((rank == root) && (recvcount == 0)) || ((rank != root) && (sendcount == 0)))
        goto fn_exit;

    is_homogeneous = TRUE;
#ifdef MPID_HAS_HETERO
    is_homogeneous = !comm_ptr->is_hetero;
#endif

    MPIR_Assert(comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM);

    /* Use binomial tree algorithm. */

    relative_rank = (rank >= root) ? rank - root : rank - root + comm_size;

    if (rank == root)
    {
        MPIR_Datatype_get_extent_macro(recvtype, extent);
        MPIR_Ensure_Aint_fits_in_pointer(MPIR_VOID_PTR_CAST_TO_MPI_AINT recvbuf+
                                         (extent*recvcount*comm_size));
    }

    if (is_homogeneous)
    {
        /* communicator is homogeneous. no need to pack buffer. */
        if (rank == root)
        {
            MPIR_Datatype_get_size_macro(recvtype, recvtype_size);
            nbytes = recvtype_size * recvcount;
        }
        else
        {
            MPIR_Datatype_get_size_macro(sendtype, sendtype_size);
            nbytes = sendtype_size * sendcount;
        }

        /* Find the number of missing nodes in my sub-tree compared to
         * a balanced tree */
        for (mask = 1; mask < comm_size; mask <<= 1);
        --mask;
        while (relative_rank & mask) mask >>= 1;
        missing = (relative_rank | mask) - comm_size + 1;
        if (missing < 0) missing = 0;
        tmp_buf_size = (mask - missing);

        /* If the message is smaller than the threshold, we will copy
         * our message in there too */
        if (nbytes < MPIR_CVAR_GATHER_VSMALL_MSG_SIZE) tmp_buf_size++;

        tmp_buf_size *= nbytes;

        /* For zero-ranked root, we don't need any temporary buffer */
        if ((rank == root) && (!root || (nbytes >= MPIR_CVAR_GATHER_VSMALL_MSG_SIZE)))
            tmp_buf_size = 0;

        if (tmp_buf_size) {
            MPIR_SCHED_CHKPMEM_MALLOC(tmp_buf, void *, tmp_buf_size, mpi_errno, "tmp_buf");
        }

        if (rank == root) {
            if (sendbuf != MPI_IN_PLACE) {
                mpi_errno = MPIR_Localcopy(sendbuf, sendcount, sendtype,
                                           ((char *) recvbuf + extent*recvcount*rank), recvcount, recvtype);
                if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            }
        }
        else if (tmp_buf_size && (nbytes < MPIR_CVAR_GATHER_VSMALL_MSG_SIZE)) {
            /* copy from sendbuf into tmp_buf */
            mpi_errno = MPIR_Localcopy(sendbuf, sendcount, sendtype,
                                       tmp_buf, nbytes, MPI_BYTE);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        }
        curr_cnt = nbytes;

        mask = 0x1;
        while (mask < comm_size) {
            if ((mask & relative_rank) == 0) {
                src = relative_rank | mask;
                if (src < comm_size) {
                    src = (src + root) % comm_size;

                    if (rank == root) {
                        recvblks = mask;
                        if ((2 * recvblks) > comm_size)
                            recvblks = comm_size - recvblks;

                        if ((rank + mask + recvblks == comm_size) ||
                            (((rank + mask) % comm_size) < ((rank + mask + recvblks) % comm_size)))
                        {
                            /* If the data contiguously fits into the
                             * receive buffer, place it directly. This
                             * should cover the case where the root is
                             * rank 0. */
                            char *rp = (char *)recvbuf + (((rank + mask) % comm_size)*recvcount*extent);
                            mpi_errno = MPIR_Sched_recv(rp, (recvblks * recvcount), recvtype, src, comm_ptr, s);
                            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                            mpi_errno = MPIR_Sched_barrier(s);
                            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                        }
                        else if (nbytes < MPIR_CVAR_GATHER_VSMALL_MSG_SIZE) {
                            mpi_errno = MPIR_Sched_recv(tmp_buf, (recvblks * nbytes), MPI_BYTE, src, comm_ptr, s);
                            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                            mpi_errno = MPIR_Sched_barrier(s);
                            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                            copy_offset = rank + mask;
                            copy_blks = recvblks;
                        }
                        else {
                            blocks[0] = recvcount * (comm_size - root - mask);
                            displs[0] = recvcount * (root + mask);
                            blocks[1] = (recvcount * recvblks) - blocks[0];
                            displs[1] = 0;

                            mpi_errno = MPIR_Type_indexed_impl(2, blocks, displs, recvtype, &tmp_type);
                            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                            mpi_errno = MPIR_Type_commit_impl(&tmp_type);
                            if (mpi_errno) MPIR_ERR_POP(mpi_errno);

                            mpi_errno = MPIR_Sched_recv(recvbuf, 1, tmp_type, src, comm_ptr, s);
                            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                            mpi_errno = MPIR_Sched_barrier(s);
                            if (mpi_errno) MPIR_ERR_POP(mpi_errno);

                            /* this "premature" free is safe b/c the sched holds an actual ref to keep it alive */
                            MPIR_Type_free_impl(&tmp_type);
                        }
                    }
                    else { /* Intermediate nodes store in temporary buffer */
                        MPI_Aint offset;

                        /* Estimate the amount of data that is going to come in */
                        recvblks = mask;
                        relative_src = ((src - root) < 0) ? (src - root + comm_size) : (src - root);
                        if (relative_src + mask > comm_size)
                            recvblks -= (relative_src + mask - comm_size);

                        if (nbytes < MPIR_CVAR_GATHER_VSMALL_MSG_SIZE)
                            offset = mask * nbytes;
                        else
                            offset = (mask - 1) * nbytes;
                        mpi_errno = MPIR_Sched_recv(((char *)tmp_buf + offset), (recvblks * nbytes),
                                                    MPI_BYTE, src, comm_ptr, s);
                        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                        mpi_errno = MPIR_Sched_barrier(s);
                        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                        curr_cnt += (recvblks * nbytes);
                    }
                }
            }
            else {
                dst = relative_rank ^ mask;
                dst = (dst + root) % comm_size;

                if (!tmp_buf_size) {
                    /* leaf nodes send directly from sendbuf */
                    mpi_errno = MPIR_Sched_send(sendbuf, sendcount, sendtype, dst, comm_ptr, s);
                    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                    mpi_errno = MPIR_Sched_barrier(s);
                    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                }
                else if (nbytes < MPIR_CVAR_GATHER_VSMALL_MSG_SIZE) {
                    mpi_errno = MPIR_Sched_send(tmp_buf, curr_cnt, MPI_BYTE, dst, comm_ptr, s);
                    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                    mpi_errno = MPIR_Sched_barrier(s);
                    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                }
                else {
                    blocks[0] = sendcount;
                    struct_displs[0] = MPIR_VOID_PTR_CAST_TO_MPI_AINT sendbuf;
                    types[0] = sendtype;
		    /* check for overflow.  work around int limits if needed*/
		    if (curr_cnt - nbytes != (int)(curr_cnt-nbytes)) {
			blocks[1] = 1;
			MPIR_Type_contiguous_x_impl(curr_cnt - nbytes,
				MPI_BYTE, &(types[1]));
		    } else {
			MPIR_Assign_trunc(blocks[1], curr_cnt - nbytes, int);
			types[1] = MPI_BYTE;
		    }
                    struct_displs[1] = MPIR_VOID_PTR_CAST_TO_MPI_AINT tmp_buf;

                    mpi_errno = MPIR_Type_create_struct_impl(2, blocks, struct_displs, types, &tmp_type);
                    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                    mpi_errno = MPIR_Type_commit_impl(&tmp_type);
                    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

                    mpi_errno = MPIR_Sched_send(MPI_BOTTOM, 1, tmp_type, dst, comm_ptr, s);
                    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                    MPIR_SCHED_BARRIER(s);

                    /* this "premature" free is safe b/c the sched holds an actual ref to keep it alive */
                    MPIR_Type_free_impl(&tmp_type);
                }

                break;
            }
            mask <<= 1;
        }

        if ((rank == root) && root && (nbytes < MPIR_CVAR_GATHER_VSMALL_MSG_SIZE) && copy_blks) {
            /* reorder and copy from tmp_buf into recvbuf */
            /* FIXME why are there two copies here? */
            mpi_errno = MPIR_Sched_copy(tmp_buf, nbytes * (comm_size - copy_offset), MPI_BYTE,
                                       ((char *)recvbuf + extent * recvcount * copy_offset),
                                       recvcount * (comm_size - copy_offset), recvtype, s);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            mpi_errno = MPIR_Sched_copy((char *)tmp_buf + nbytes * (comm_size - copy_offset),
                                        nbytes * (copy_blks - comm_size + copy_offset), MPI_BYTE,
                                        recvbuf, recvcount * (copy_blks - comm_size + copy_offset),
                                        recvtype, s);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        }
    }
#ifdef MPID_HAS_HETERO
    else {
        /* communicator is heterogeneous. pack data into tmp_buf. */

        /* currently unimplemented & untested */
        MPIR_Assertp(FALSE);

#if 0
        if (rank == root)
            MPIR_Pack_size_impl(recvcount*comm_size, recvtype, &tmp_buf_size);
        else
            MPIR_Pack_size_impl(sendcount*(comm_size/2), sendtype, &tmp_buf_size);

        MPIR_CHKPMEM_MALLOC(tmp_buf, void *, tmp_buf_size, mpi_errno, "tmp_buf");

        position = 0;
        if (sendbuf != MPI_IN_PLACE) {
            mpi_errno = MPIR_Pack_impl(sendbuf, sendcount, sendtype, tmp_buf,
                                       tmp_buf_size, &position);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            nbytes = position;
        }
        else {
            /* do a dummy pack just to calculate nbytes */
            mpi_errno = MPIR_Pack_impl(recvbuf, 1, recvtype, tmp_buf,
                                       tmp_buf_size, &position);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            nbytes = position*recvcount;
        }

        curr_cnt = nbytes;

        mask = 0x1;
        while (mask < comm_size) {
            if ((mask & relative_rank) == 0) {
                src = relative_rank | mask;
                if (src < comm_size) {
                    src = (src + root) % comm_size;
                    mpi_errno = MPIC_Recv(((char *)tmp_buf + curr_cnt),
                                             tmp_buf_size-curr_cnt, MPI_BYTE, src,
                                             MPIR_GATHER_TAG, comm_ptr,
                                             &status, errflag);
                    /* the recv size is larger than what may be sent in
                       some cases. query amount of data actually received */
                    recv_size = 0;
                    MPIR_Get_count_impl(&status, MPI_BYTE, &recv_size);
                    curr_cnt += recv_size;
                }
            }
            else
            {
                dst = relative_rank ^ mask;
                dst = (dst + root) % comm_size;
                mpi_errno = MPIC_Send(tmp_buf, curr_cnt, MPI_BYTE, dst,
                                         MPIR_GATHER_TAG, comm_ptr, errflag);
                if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                break;
            }
            mask <<= 1;
        }

        if (rank == root)
        {
            /* reorder and copy from tmp_buf into recvbuf */
            if (sendbuf != MPI_IN_PLACE) {
                position = 0;
                mpi_errno = MPIR_Unpack_impl(tmp_buf, tmp_buf_size, &position,
                                             ((char *) recvbuf + extent*recvcount*rank),
                                             recvcount*(comm_size-rank), recvtype);
                if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            }
            else {
                position = nbytes;
                mpi_errno = MPIR_Unpack_impl(tmp_buf, tmp_buf_size, &position,
                                             ((char *) recvbuf + extent*recvcount*(rank+1)),
                                             recvcount*(comm_size-rank-1), recvtype);
                if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            }
            if (root != 0) {
                mpi_errno = MPIR_Unpack_impl(tmp_buf, tmp_buf_size, &position, recvbuf,
                                             recvcount*rank, recvtype);
                if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            }
        }

    }
#endif
#endif /* MPID_HAS_HETERO */

    MPIR_SCHED_CHKPMEM_COMMIT(s);
fn_exit:
    return mpi_errno;
fn_fail:
    MPIR_SCHED_CHKPMEM_REAP(s);
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Igather_intra
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Igather_intra(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPIR_Comm *comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Igather_binomial(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm_ptr, s);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Igather_inter
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Igather_inter(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPIR_Comm *comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int rank;
    MPI_Aint local_size, remote_size;
    int i;
    MPI_Aint recvtype_size, sendtype_size, nbytes;
    MPI_Aint extent, true_extent, true_lb = 0;
    void *tmp_buf = NULL;
    MPIR_Comm *newcomm_ptr = NULL;
    MPIR_SCHED_CHKPMEM_DECL(1);

/*  Intercommunicator gather.
    For short messages, remote group does a local intracommunicator
    gather to rank 0. Rank 0 then sends data to root.

    Cost: (lgp+1).alpha + n.((p-1)/p).beta + n.beta

    For long messages, we use linear gather to avoid the extra n.beta.

    Cost: p.alpha + n.beta
*/

    if (root == MPI_PROC_NULL) {
        /* local processes other than root do nothing */
        goto fn_exit;
    }

    remote_size = comm_ptr->remote_size;
    local_size = comm_ptr->local_size;

    if (root == MPI_ROOT) {
        MPIR_Datatype_get_size_macro(recvtype, recvtype_size);
        nbytes = recvtype_size * recvcount * remote_size;
    }
    else {
        /* remote side */
        MPIR_Datatype_get_size_macro(sendtype, sendtype_size);
        nbytes = sendtype_size * sendcount * local_size;
    }

    if (nbytes < MPIR_CVAR_GATHER_INTER_SHORT_MSG_SIZE) {
        if (root == MPI_ROOT) {
            /* root receives data from rank 0 on remote group */
            mpi_errno = MPIR_Sched_recv(recvbuf, recvcount*remote_size, recvtype, 0, comm_ptr, s);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        }
        else {
            /* remote group. Rank 0 allocates temporary buffer, does
               local intracommunicator gather, and then sends the data
               to root. */

            rank = comm_ptr->rank;

            if (rank == 0) {
                MPIR_Type_get_true_extent_impl(sendtype, &true_lb, &true_extent);
                MPIR_Datatype_get_extent_macro(sendtype, extent);

                MPIR_Ensure_Aint_fits_in_pointer(sendcount*local_size*(MPL_MAX(extent, true_extent)));
                MPIR_SCHED_CHKPMEM_MALLOC(tmp_buf, void *, sendcount*local_size*(MPL_MAX(extent,true_extent)),
                                          mpi_errno, "tmp_buf");
                /* adjust for potential negative lower bound in datatype */
                tmp_buf = (void *)((char*)tmp_buf - true_lb);
            }

            /* all processes in remote group form new intracommunicator */
            if (!comm_ptr->local_comm) {
                mpi_errno = MPII_Setup_intercomm_localcomm( comm_ptr );
                if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            }

            newcomm_ptr = comm_ptr->local_comm;

            /* now do the a local gather on this intracommunicator */
            MPIR_Assert(newcomm_ptr->coll_fns && newcomm_ptr->coll_fns->Igather_sched);
            mpi_errno = newcomm_ptr->coll_fns->Igather_sched(sendbuf, sendcount, sendtype,
                                                       tmp_buf, sendcount, sendtype, 0,
                                                       newcomm_ptr, s);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);

            if (rank == 0) {
                mpi_errno = MPIR_Sched_send(tmp_buf, sendcount*local_size, sendtype, root, comm_ptr, s);
                if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            }
        }
    }
    else {
        /* long message. use linear algorithm. */
        if (root == MPI_ROOT) {
            MPIR_Datatype_get_extent_macro(recvtype, extent);
            MPIR_Ensure_Aint_fits_in_pointer(MPIR_VOID_PTR_CAST_TO_MPI_AINT recvbuf +
                                             (recvcount*remote_size*extent));

            for (i=0; i<remote_size; i++) {
                mpi_errno = MPIR_Sched_recv(((char *)recvbuf+recvcount*i*extent),
                                            recvcount, recvtype, i, comm_ptr, s);
                if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            }
        }
        else {
            mpi_errno = MPIR_Sched_send(sendbuf, sendcount, sendtype, root, comm_ptr, s);
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
#define FUNCNAME MPIR_Igather_impl
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Igather_impl(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPIR_Comm *comm_ptr, MPI_Request *request)
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
    MPIR_Assert(comm_ptr->coll_fns->Igather_sched != NULL);
    mpi_errno = comm_ptr->coll_fns->Igather_sched(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm_ptr, s);
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
#define FUNCNAME MPI_Igather
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@
MPI_Igather - Gathers together values from a group of processes in
              a nonblocking way

Input Parameters:
+ sendbuf - starting address of the send buffer (choice)
. sendcount - number of elements in send buffer (non-negative integer)
. sendtype - data type of send buffer elements (handle)
. recvcount - number of elements in receive buffer (significant only at root) (non-negative integer)
. recvtype - data type of receive buffer elements (significant only at root) (handle)
. root - rank of receiving process (integer)
- comm - communicator (handle)

Output Parameters:
+ recvbuf - starting address of the receive buffer (significant only at root) (choice)
- request - communication request (handle)

.N ThreadSafe

.N Fortran

.N Errors
@*/
int MPI_Igather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                void *recvbuf, int recvcount, MPI_Datatype recvtype,
                int root, MPI_Comm comm, MPI_Request *request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *comm_ptr = NULL;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_IGATHER);

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_IGATHER);

    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS
        {
            MPIR_ERRTEST_COMM(comm, mpi_errno);
            if (sendbuf != MPI_IN_PLACE) {
                MPIR_ERRTEST_COUNT(sendcount, mpi_errno);
                MPIR_ERRTEST_DATATYPE(sendtype, "sendtype", mpi_errno);
            }
            MPIR_ERRTEST_COUNT(recvcount, mpi_errno);
            MPIR_ERRTEST_DATATYPE(recvtype, "recvtype", mpi_errno);

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
            MPIR_Datatype *sendtype_ptr=NULL, *recvtype_ptr=NULL;
            int rank;

            MPIR_Comm_valid_ptr( comm_ptr, mpi_errno, FALSE );
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;

            if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
                MPIR_ERRTEST_INTRA_ROOT(comm_ptr, root, mpi_errno);

                if (sendbuf != MPI_IN_PLACE) {
                    MPIR_ERRTEST_COUNT(sendcount, mpi_errno);
                    MPIR_ERRTEST_DATATYPE(sendtype, "sendtype", mpi_errno);
                    if (HANDLE_GET_KIND(sendtype) != HANDLE_KIND_BUILTIN) {
                        MPIR_Datatype_get_ptr(sendtype, sendtype_ptr);
                        MPIR_Datatype_valid_ptr( sendtype_ptr, mpi_errno );
                        if (mpi_errno != MPI_SUCCESS) goto fn_fail;
                        MPIR_Datatype_committed_ptr( sendtype_ptr, mpi_errno );
                        if (mpi_errno != MPI_SUCCESS) goto fn_fail;
                    }
                    MPIR_ERRTEST_USERBUFFER(sendbuf,sendcount,sendtype,mpi_errno);
                }

                rank = comm_ptr->rank;
                if (rank == root) {
                    MPIR_ERRTEST_COUNT(recvcount, mpi_errno);
                    MPIR_ERRTEST_DATATYPE(recvtype, "recvtype", mpi_errno);
                    if (HANDLE_GET_KIND(recvtype) != HANDLE_KIND_BUILTIN) {
                        MPIR_Datatype_get_ptr(recvtype, recvtype_ptr);
                        MPIR_Datatype_valid_ptr( recvtype_ptr, mpi_errno );
                        if (mpi_errno != MPI_SUCCESS) goto fn_fail;
                        MPIR_Datatype_committed_ptr( recvtype_ptr, mpi_errno );
                        if (mpi_errno != MPI_SUCCESS) goto fn_fail;
                    }
                    MPIR_ERRTEST_RECVBUF_INPLACE(recvbuf, recvcount, mpi_errno);
                    MPIR_ERRTEST_USERBUFFER(recvbuf,recvcount,recvtype,mpi_errno);

                    /* catch common aliasing cases */
                    if (recvbuf != MPI_IN_PLACE && sendtype == recvtype && sendcount == recvcount && sendcount != 0) {
                        MPI_Aint recvtype_size;
                        MPIR_Datatype_get_size_macro(recvtype, recvtype_size);
                        MPIR_ERRTEST_ALIAS_COLL(sendbuf, (char*)recvbuf + comm_ptr->rank*recvcount*recvtype_size, mpi_errno);
                    }
                }
                else
                    MPIR_ERRTEST_SENDBUF_INPLACE(sendbuf, sendcount, mpi_errno);
            }

            if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTERCOMM) {
                MPIR_ERRTEST_INTER_ROOT(comm_ptr, root, mpi_errno);

                if (root == MPI_ROOT) {
                    MPIR_ERRTEST_COUNT(recvcount, mpi_errno);
                    MPIR_ERRTEST_DATATYPE(recvtype, "recvtype", mpi_errno);
                    if (HANDLE_GET_KIND(recvtype) != HANDLE_KIND_BUILTIN) {
                        MPIR_Datatype_get_ptr(recvtype, recvtype_ptr);
                        MPIR_Datatype_valid_ptr( recvtype_ptr, mpi_errno );
                        if (mpi_errno != MPI_SUCCESS) goto fn_fail;
                        MPIR_Datatype_committed_ptr( recvtype_ptr, mpi_errno );
                        if (mpi_errno != MPI_SUCCESS) goto fn_fail;
                    }
                    MPIR_ERRTEST_RECVBUF_INPLACE(recvbuf, recvcount, mpi_errno);
                    MPIR_ERRTEST_USERBUFFER(recvbuf,recvcount,recvtype,mpi_errno);
                }

                else if (root != MPI_PROC_NULL) {
                    MPIR_ERRTEST_COUNT(sendcount, mpi_errno);
                    MPIR_ERRTEST_DATATYPE(sendtype, "sendtype", mpi_errno);
                    if (HANDLE_GET_KIND(sendtype) != HANDLE_KIND_BUILTIN) {
                        MPIR_Datatype_get_ptr(sendtype, sendtype_ptr);
                        MPIR_Datatype_valid_ptr( sendtype_ptr, mpi_errno );
                        if (mpi_errno != MPI_SUCCESS) goto fn_fail;
                        MPIR_Datatype_committed_ptr( sendtype_ptr, mpi_errno );
                        if (mpi_errno != MPI_SUCCESS) goto fn_fail;
                    }
                    MPIR_ERRTEST_SENDBUF_INPLACE(sendbuf, sendcount, mpi_errno);
                    MPIR_ERRTEST_USERBUFFER(sendbuf,sendcount,sendtype,mpi_errno);
                }
            }
        }
        MPID_END_ERROR_CHECKS
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    mpi_errno = MPID_Igather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm_ptr, request);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

    /* ... end of body of routine ... */

fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_IGATHER);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno = MPIR_Err_create_code(
            mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
            "**mpi_igather", "**mpi_igather %p %d %D %p %d %D %d %C %p", sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, request);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm(comm_ptr, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
