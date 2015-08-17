/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "collutil.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_BCAST_MIN_PROCS
      category    : COLLECTIVE
      type        : int
      default     : 8
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Let's define short messages as messages with size < MPIR_CVAR_BCAST_SHORT_MSG_SIZE,
        and medium messages as messages with size >= MPIR_CVAR_BCAST_SHORT_MSG_SIZE but
        < MPIR_CVAR_BCAST_LONG_MSG_SIZE, and long messages as messages with size >=
        MPIR_CVAR_BCAST_LONG_MSG_SIZE. The broadcast algorithms selection procedure is
        as follows. For short messages or when the number of processes is <
        MPIR_CVAR_BCAST_MIN_PROCS, we do broadcast using the binomial tree algorithm.
        Otherwise, for medium messages and with a power-of-two number of processes, we do
        broadcast based on a scatter followed by a recursive doubling allgather algorithm.
        Otherwise, for long messages or with non power-of-two number of processes, we do
        broadcast based on a scatter followed by a ring allgather algorithm.
        (See also: MPIR_CVAR_BCAST_SHORT_MSG_SIZE, MPIR_CVAR_BCAST_LONG_MSG_SIZE)

    - name        : MPIR_CVAR_BCAST_SHORT_MSG_SIZE
      category    : COLLECTIVE
      type        : int
      default     : 12288
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Let's define short messages as messages with size < MPIR_CVAR_BCAST_SHORT_MSG_SIZE,
        and medium messages as messages with size >= MPIR_CVAR_BCAST_SHORT_MSG_SIZE but
        < MPIR_CVAR_BCAST_LONG_MSG_SIZE, and long messages as messages with size >=
        MPIR_CVAR_BCAST_LONG_MSG_SIZE. The broadcast algorithms selection procedure is
        as follows. For short messages or when the number of processes is <
        MPIR_CVAR_BCAST_MIN_PROCS, we do broadcast using the binomial tree algorithm.
        Otherwise, for medium messages and with a power-of-two number of processes, we do
        broadcast based on a scatter followed by a recursive doubling allgather algorithm.
        Otherwise, for long messages or with non power-of-two number of processes, we do
        broadcast based on a scatter followed by a ring allgather algorithm.
        (See also: MPIR_CVAR_BCAST_MIN_PROCS, MPIR_CVAR_BCAST_LONG_MSG_SIZE)

    - name        : MPIR_CVAR_BCAST_LONG_MSG_SIZE
      category    : COLLECTIVE
      type        : int
      default     : 524288
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Let's define short messages as messages with size < MPIR_CVAR_BCAST_SHORT_MSG_SIZE,
        and medium messages as messages with size >= MPIR_CVAR_BCAST_SHORT_MSG_SIZE but
        < MPIR_CVAR_BCAST_LONG_MSG_SIZE, and long messages as messages with size >=
        MPIR_CVAR_BCAST_LONG_MSG_SIZE. The broadcast algorithms selection procedure is
        as follows. For short messages or when the number of processes is <
        MPIR_CVAR_BCAST_MIN_PROCS, we do broadcast using the binomial tree algorithm.
        Otherwise, for medium messages and with a power-of-two number of processes, we do
        broadcast based on a scatter followed by a recursive doubling allgather algorithm.
        Otherwise, for long messages or with non power-of-two number of processes, we do
        broadcast based on a scatter followed by a ring allgather algorithm.
        (See also: MPIR_CVAR_BCAST_MIN_PROCS, MPIR_CVAR_BCAST_SHORT_MSG_SIZE)

    - name        : MPIR_CVAR_ENABLE_SMP_BCAST
      category    : COLLECTIVE
      type        : boolean
      default     : true
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : Enable SMP aware broadcast (See also: MPIR_CVAR_MAX_SMP_BCAST_MSG_SIZE)

    - name        : MPIR_CVAR_MAX_SMP_BCAST_MSG_SIZE
      category    : COLLECTIVE
      type        : int
      default     : 0
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Maximum message size for which SMP-aware broadcast is used.  A
        value of '0' uses SMP-aware broadcast for all message sizes.
        (See also: MPIR_CVAR_ENABLE_SMP_BCAST)

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

/* -- Begin Profiling Symbol Block for routine MPI_Bcast */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Bcast = PMPI_Bcast
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Bcast  MPI_Bcast
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Bcast as PMPI_Bcast
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Bcast(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm)
              __attribute__((weak,alias("PMPI_Bcast")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Bcast
#define MPI_Bcast PMPI_Bcast

/* A binomial tree broadcast algorithm.  Good for short messages, 
   Cost = lgp.alpha + n.lgp.beta */
#undef FUNCNAME
#define FUNCNAME MPIR_Bcast_binomial
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int MPIR_Bcast_binomial(
    void *buffer, 
    int count, 
    MPI_Datatype datatype, 
    int root, 
    MPID_Comm *comm_ptr,
    MPIR_Errflag_t *errflag)
{
    int        rank, comm_size, src, dst;
    int        relative_rank, mask;
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    int nbytes=0;
    int recvd_size;
    MPI_Status status;
    int is_contig, is_homogeneous;
    MPI_Aint type_size;
    MPI_Aint position;
    void *tmp_buf=NULL;
    MPID_Datatype *dtp;
    MPIU_CHKLMEM_DECL(1);

    comm_size = comm_ptr->local_size;
    rank = comm_ptr->rank;

    /* If there is only one process, return */
    if (comm_size == 1) goto fn_exit;

    if (HANDLE_GET_KIND(datatype) == HANDLE_KIND_BUILTIN)
        is_contig = 1;
    else {
        MPID_Datatype_get_ptr(datatype, dtp); 
        is_contig = dtp->is_contig;
    }

    is_homogeneous = 1;
#ifdef MPID_HAS_HETERO
    if (comm_ptr->is_hetero)
        is_homogeneous = 0;
#endif

    /* MPI_Type_size() might not give the accurate size of the packed
     * datatype for heterogeneous systems (because of padding, encoding,
     * etc). On the other hand, MPI_Pack_size() can become very
     * expensive, depending on the implementation, especially for
     * heterogeneous systems. We want to use MPI_Type_size() wherever
     * possible, and MPI_Pack_size() in other places.
     */
    if (is_homogeneous)
        MPID_Datatype_get_size_macro(datatype, type_size);
    else
	/* --BEGIN HETEROGENEOUS-- */
        MPIR_Pack_size_impl(1, datatype, &type_size);
        /* --END HETEROGENEOUS-- */

    nbytes = type_size * count;
    if (nbytes == 0)
        goto fn_exit; /* nothing to do */

    if (!is_contig || !is_homogeneous)
    {
        MPIU_CHKLMEM_MALLOC(tmp_buf, void *, nbytes, mpi_errno, "tmp_buf");

        /* TODO: Pipeline the packing and communication */
        position = 0;
        if (rank == root) {
            mpi_errno = MPIR_Pack_impl(buffer, count, datatype, tmp_buf, nbytes,
                                       &position);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        }
    }

    relative_rank = (rank >= root) ? rank - root : rank - root + comm_size;

    /* Use short message algorithm, namely, binomial tree */

    /* Algorithm:
       This uses a fairly basic recursive subdivision algorithm.
       The root sends to the process comm_size/2 away; the receiver becomes
       a root for a subtree and applies the same process. 

       So that the new root can easily identify the size of its
       subtree, the (subtree) roots are all powers of two (relative
       to the root) If m = the first power of 2 such that 2^m >= the
       size of the communicator, then the subtree at root at 2^(m-k)
       has size 2^k (with special handling for subtrees that aren't
       a power of two in size).

       Do subdivision.  There are two phases:
       1. Wait for arrival of data.  Because of the power of two nature
       of the subtree roots, the source of this message is alwyas the
       process whose relative rank has the least significant 1 bit CLEARED.
       That is, process 4 (100) receives from process 0, process 7 (111) 
       from process 6 (110), etc.   
       2. Forward to my subtree

       Note that the process that is the tree root is handled automatically
       by this code, since it has no bits set.  */

    mask = 0x1;
    while (mask < comm_size)
    {
        if (relative_rank & mask)
        {
            src = rank - mask; 
            if (src < 0) src += comm_size;
            if (!is_contig || !is_homogeneous)
                mpi_errno = MPIC_Recv(tmp_buf,nbytes,MPI_BYTE,src,
                                         MPIR_BCAST_TAG,comm_ptr, &status, errflag);
            else
                mpi_errno = MPIC_Recv(buffer,count,datatype,src,
                                         MPIR_BCAST_TAG,comm_ptr, &status, errflag);
            if (mpi_errno) {
                /* for communication errors, just record the error but continue */
                *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
            }

            /* check that we received as much as we expected */
            MPIR_Get_count_impl(&status, MPI_BYTE, &recvd_size);
            /* recvd_size may not be accurate for packed heterogeneous data */
            if (is_homogeneous && recvd_size != nbytes) {
                if (*errflag == MPIR_ERR_NONE) *errflag = MPIR_ERR_OTHER;
                MPIR_ERR_SET2(mpi_errno, MPI_ERR_OTHER,
		      "**collective_size_mismatch",
		      "**collective_size_mismatch %d %d", recvd_size, nbytes );
                MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
            }
            break;
        }
        mask <<= 1;
    }

    /* This process is responsible for all processes that have bits
       set from the LSB upto (but not including) mask.  Because of
       the "not including", we start by shifting mask back down one.

       We can easily change to a different algorithm at any power of two
       by changing the test (mask > 1) to (mask > block_size) 

       One such version would use non-blocking operations for the last 2-4
       steps (this also bounds the number of MPI_Requests that would
       be needed).  */

    mask >>= 1;
    while (mask > 0)
    {
        if (relative_rank + mask < comm_size)
        {
            dst = rank + mask;
            if (dst >= comm_size) dst -= comm_size;
            if (!is_contig || !is_homogeneous)
                mpi_errno = MPIC_Send(tmp_buf,nbytes,MPI_BYTE,dst,
                                         MPIR_BCAST_TAG,comm_ptr, errflag);
            else
                mpi_errno = MPIC_Send(buffer,count,datatype,dst,
                                         MPIR_BCAST_TAG,comm_ptr, errflag);
            if (mpi_errno) {
                /* for communication errors, just record the error but continue */
                *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
            }
        }
        mask >>= 1;
    }

    if (!is_contig || !is_homogeneous)
    {
        if (rank != root)
        {
            position = 0;
            mpi_errno = MPIR_Unpack_impl(tmp_buf, nbytes, &position, buffer,
                                         count, datatype);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            
        }
    }

fn_exit:
    MPIU_CHKLMEM_FREEALL();
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno_ret)
        mpi_errno = mpi_errno_ret;
    else if (*errflag != MPIR_ERR_NONE)
        MPIR_ERR_SET(mpi_errno, *errflag, "**coll_fail");
    /* --END ERROR HANDLING-- */
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

/* FIXME it would be nice if we could refactor things to minimize
   duplication between this and MPIR_Scatter_intra and friends.  We can't use
   MPIR_Scatter_intra as is without inducing an extra copy in the noncontig case. */
/* There are additional arguments included here that are unused because we
   always assume that the noncontig case has been packed into a contig case by
   the caller for now.  Once we start handling noncontig data at the upper level
   we can start handling it here.
   
   At the moment this function always scatters a buffer of nbytes starting at
   tmp_buf address. */
#undef FUNCNAME
#define FUNCNAME scatter_for_bcast
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int scatter_for_bcast(
    void *buffer ATTRIBUTE((unused)),
    int count ATTRIBUTE((unused)), 
    MPI_Datatype datatype ATTRIBUTE((unused)),
    int root,
    MPID_Comm *comm_ptr,
    int nbytes,
    void *tmp_buf,
    int is_contig,
    int is_homogeneous,
    MPIR_Errflag_t *errflag)
{
    MPI_Status status;
    int        rank, comm_size, src, dst;
    int        relative_rank, mask;
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    int scatter_size, curr_size, recv_size = 0, send_size;

    comm_size = comm_ptr->local_size;
    rank = comm_ptr->rank;
    relative_rank = (rank >= root) ? rank - root : rank - root + comm_size;

    /* use long message algorithm: binomial tree scatter followed by an allgather */

    /* The scatter algorithm divides the buffer into nprocs pieces and
       scatters them among the processes. Root gets the first piece,
       root+1 gets the second piece, and so forth. Uses the same binomial
       tree algorithm as above. Ceiling division
       is used to compute the size of each piece. This means some
       processes may not get any data. For example if bufsize = 97 and
       nprocs = 16, ranks 15 and 16 will get 0 data. On each process, the
       scattered data is stored at the same offset in the buffer as it is
       on the root process. */ 

    scatter_size = (nbytes + comm_size - 1)/comm_size; /* ceiling division */
    curr_size = (rank == root) ? nbytes : 0; /* root starts with all the
                                                data */

    mask = 0x1;
    while (mask < comm_size)
    {
        if (relative_rank & mask)
        {
            src = rank - mask; 
            if (src < 0) src += comm_size;
            recv_size = nbytes - relative_rank*scatter_size;
            /* recv_size is larger than what might actually be sent by the
               sender. We don't need compute the exact value because MPI
               allows you to post a larger recv.*/ 
            if (recv_size <= 0)
            {
                curr_size = 0; /* this process doesn't receive any data
                                  because of uneven division */
            }
            else
            {
                mpi_errno = MPIC_Recv(((char *)tmp_buf +
                                          relative_rank*scatter_size),
                                         recv_size, MPI_BYTE, src,
                                         MPIR_BCAST_TAG, comm_ptr, &status, errflag);
                if (mpi_errno) {
                    /* for communication errors, just record the error but continue */
                    *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                    MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                    MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                    curr_size = 0;
                } else
                    /* query actual size of data received */
                    MPIR_Get_count_impl(&status, MPI_BYTE, &curr_size);
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
    while (mask > 0)
    {
        if (relative_rank + mask < comm_size)
        {
            send_size = curr_size - scatter_size * mask; 
            /* mask is also the size of this process's subtree */

            if (send_size > 0)
            {
                dst = rank + mask;
                if (dst >= comm_size) dst -= comm_size;
                mpi_errno = MPIC_Send(((char *)tmp_buf +
                                          scatter_size*(relative_rank+mask)),
                                         send_size, MPI_BYTE, dst,
                                         MPIR_BCAST_TAG, comm_ptr, errflag);
                if (mpi_errno) {
                    /* for communication errors, just record the error but continue */
                    *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                    MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                    MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                }

                curr_size -= send_size;
            }
        }
        mask >>= 1;
    }

fn_exit:
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno_ret)
        mpi_errno = mpi_errno_ret;
    else if (*errflag != MPIR_ERR_NONE)
        MPIR_ERR_SET(mpi_errno, *errflag, "**coll_fail");
    /* --END ERROR HANDLING-- */
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

/*
   Broadcast based on a scatter followed by an allgather.

   We first scatter the buffer using a binomial tree algorithm. This costs
   lgp.alpha + n.((p-1)/p).beta
   If the datatype is contiguous and the communicator is homogeneous,
   we treat the data as bytes and divide (scatter) it among processes
   by using ceiling division. For the noncontiguous or heterogeneous
   cases, we first pack the data into a temporary buffer by using
   MPI_Pack, scatter it as bytes, and unpack it after the allgather.

   For the allgather, we use a recursive doubling algorithm for 
   medium-size messages and power-of-two number of processes. This
   takes lgp steps. In each step pairs of processes exchange all the
   data they have (we take care of non-power-of-two situations). This
   costs approximately lgp.alpha + n.((p-1)/p).beta. (Approximately
   because it may be slightly more in the non-power-of-two case, but
   it's still a logarithmic algorithm.) Therefore, for long messages
   Total Cost = 2.lgp.alpha + 2.n.((p-1)/p).beta
*/
#undef FUNCNAME
#define FUNCNAME MPIR_Bcast_scatter_doubling_allgather
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int MPIR_Bcast_scatter_doubling_allgather(
    void *buffer, 
    int count, 
    MPI_Datatype datatype, 
    int root, 
    MPID_Comm *comm_ptr,
    MPIR_Errflag_t *errflag)
{
    MPI_Status status;
    int rank, comm_size, dst;
    int relative_rank, mask;
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    int scatter_size, curr_size, recv_size = 0;
    int j, k, i, tmp_mask, is_contig, is_homogeneous;
    MPI_Aint type_size, nbytes = 0;
    int relative_dst, dst_tree_root, my_tree_root, send_offset;
    int recv_offset, tree_root, nprocs_completed, offset;
    MPI_Aint position;
    MPIU_CHKLMEM_DECL(1);
    MPID_Datatype *dtp;
    MPI_Aint true_extent, true_lb;
    void *tmp_buf;

    comm_size = comm_ptr->local_size;
    rank = comm_ptr->rank;
    relative_rank = (rank >= root) ? rank - root : rank - root + comm_size;

    /* If there is only one process, return */
    if (comm_size == 1) goto fn_exit;

    if (HANDLE_GET_KIND(datatype) == HANDLE_KIND_BUILTIN)
        is_contig = 1;
    else {
        MPID_Datatype_get_ptr(datatype, dtp); 
        is_contig = dtp->is_contig;
    }

    is_homogeneous = 1;
#ifdef MPID_HAS_HETERO
    if (comm_ptr->is_hetero)
        is_homogeneous = 0;
#endif

    /* MPI_Type_size() might not give the accurate size of the packed
     * datatype for heterogeneous systems (because of padding, encoding,
     * etc). On the other hand, MPI_Pack_size() can become very
     * expensive, depending on the implementation, especially for
     * heterogeneous systems. We want to use MPI_Type_size() wherever
     * possible, and MPI_Pack_size() in other places.
     */
    if (is_homogeneous)
        MPID_Datatype_get_size_macro(datatype, type_size);
    else
        /* --BEGIN HETEROGENEOUS-- */
        MPIR_Pack_size_impl(1, datatype, &type_size);
        /* --END HETEROGENEOUS-- */

    nbytes = type_size * count;
    if (nbytes == 0)
        goto fn_exit; /* nothing to do */

    if (is_contig && is_homogeneous)
    {
        /* contiguous and homogeneous. no need to pack. */
        MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);

        tmp_buf = (char *) buffer + true_lb;
    }
    else
    {
        MPIU_CHKLMEM_MALLOC(tmp_buf, void *, nbytes, mpi_errno, "tmp_buf");

        /* TODO: Pipeline the packing and communication */
        position = 0;
        if (rank == root) {
            mpi_errno = MPIR_Pack_impl(buffer, count, datatype, tmp_buf, nbytes,
                                       &position);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        }
    }


    scatter_size = (nbytes + comm_size - 1)/comm_size; /* ceiling division */

    mpi_errno = scatter_for_bcast(buffer, count, datatype, root, comm_ptr,
                                  nbytes, tmp_buf, is_contig, is_homogeneous, errflag);
    if (mpi_errno) {
        /* for communication errors, just record the error but continue */
        *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
        MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
        MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
    }

    /* curr_size is the amount of data that this process now has stored in
     * buffer at byte offset (relative_rank*scatter_size) */
    curr_size = MPIU_MIN(scatter_size, (nbytes - (relative_rank * scatter_size)));
    if (curr_size < 0)
        curr_size = 0;

    /* medium size allgather and pof2 comm_size. use recurive doubling. */

    mask = 0x1;
    i = 0;
    while (mask < comm_size)
    {
        relative_dst = relative_rank ^ mask;

        dst = (relative_dst + root) % comm_size; 

        /* find offset into send and recv buffers.
           zero out the least significant "i" bits of relative_rank and
           relative_dst to find root of src and dst
           subtrees. Use ranks of roots as index to send from
           and recv into  buffer */ 

        dst_tree_root = relative_dst >> i;
        dst_tree_root <<= i;

        my_tree_root = relative_rank >> i;
        my_tree_root <<= i;

        send_offset = my_tree_root * scatter_size;
        recv_offset = dst_tree_root * scatter_size;

        if (relative_dst < comm_size)
        {
            mpi_errno = MPIC_Sendrecv(((char *)tmp_buf + send_offset),
                                         curr_size, MPI_BYTE, dst, MPIR_BCAST_TAG, 
                                         ((char *)tmp_buf + recv_offset),
                                         (nbytes-recv_offset < 0 ? 0 : nbytes-recv_offset), 
                                         MPI_BYTE, dst, MPIR_BCAST_TAG, comm_ptr, &status, errflag);
            if (mpi_errno) {
		/* --BEGIN ERROR HANDLING-- */
                /* for communication errors, just record the error but continue */
                *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                recv_size = 0;
		/* --END ERROR HANDLING-- */
            } else
                MPIR_Get_count_impl(&status, MPI_BYTE, &recv_size);
            curr_size += recv_size;
        }

        /* if some processes in this process's subtree in this step
           did not have any destination process to communicate with
           because of non-power-of-two, we need to send them the
           data that they would normally have received from those
           processes. That is, the haves in this subtree must send to
           the havenots. We use a logarithmic recursive-halfing algorithm
           for this. */

        /* This part of the code will not currently be
           executed because we are not using recursive
           doubling for non power of two. Mark it as experimental
           so that it doesn't show up as red in the coverage tests. */  

        /* --BEGIN EXPERIMENTAL-- */
        if (dst_tree_root + mask > comm_size)
        {
            nprocs_completed = comm_size - my_tree_root - mask;
            /* nprocs_completed is the number of processes in this
               subtree that have all the data. Send data to others
               in a tree fashion. First find root of current tree
               that is being divided into two. k is the number of
               least-significant bits in this process's rank that
               must be zeroed out to find the rank of the root */ 
            j = mask;
            k = 0;
            while (j)
            {
                j >>= 1;
                k++;
            }
            k--;

            offset = scatter_size * (my_tree_root + mask);
            tmp_mask = mask >> 1;

            while (tmp_mask)
            {
                relative_dst = relative_rank ^ tmp_mask;
                dst = (relative_dst + root) % comm_size; 

                tree_root = relative_rank >> k;
                tree_root <<= k;

                /* send only if this proc has data and destination
                   doesn't have data. */

                /* if (rank == 3) { 
                   printf("rank %d, dst %d, root %d, nprocs_completed %d\n", relative_rank, relative_dst, tree_root, nprocs_completed);
                   fflush(stdout);
                   }*/

                if ((relative_dst > relative_rank) && 
                    (relative_rank < tree_root + nprocs_completed)
                    && (relative_dst >= tree_root + nprocs_completed))
                {

                    /* printf("Rank %d, send to %d, offset %d, size %d\n", rank, dst, offset, recv_size);
                       fflush(stdout); */
                    mpi_errno = MPIC_Send(((char *)tmp_buf + offset),
                                             recv_size, MPI_BYTE, dst,
                                             MPIR_BCAST_TAG, comm_ptr, errflag);
                    /* recv_size was set in the previous
                       receive. that's the amount of data to be
                       sent now. */
                    if (mpi_errno) {
                        /* for communication errors, just record the error but continue */
                        *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                        MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                        MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                    }
                }
                /* recv only if this proc. doesn't have data and sender
                   has data */
                else if ((relative_dst < relative_rank) && 
                         (relative_dst < tree_root + nprocs_completed) &&
                         (relative_rank >= tree_root + nprocs_completed))
                {
                    /* printf("Rank %d waiting to recv from rank %d\n",
                       relative_rank, dst); */
                    mpi_errno = MPIC_Recv(((char *)tmp_buf + offset),
                                             nbytes - offset, 
                                             MPI_BYTE, dst, MPIR_BCAST_TAG,
                                             comm_ptr, &status, errflag);
                    /* nprocs_completed is also equal to the no. of processes
                       whose data we don't have */
                    if (mpi_errno) {
			/* --BEGIN ERROR HANDLING-- */
                        /* for communication errors, just record the error but continue */
                        *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                        MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                        MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                        recv_size = 0;
			/* --END ERROR HANDLING-- */
                    } else
                        MPIR_Get_count_impl(&status, MPI_BYTE, &recv_size);
                    curr_size += recv_size;
                    /* printf("Rank %d, recv from %d, offset %d, size %d\n", rank, dst, offset, recv_size);
                       fflush(stdout);*/
                }
                tmp_mask >>= 1;
                k--;
            }
        }
        /* --END EXPERIMENTAL-- */

        mask <<= 1;
        i++;
    }

    /* check that we received as much as we expected */
    /* recvd_size may not be accurate for packed heterogeneous data */
    if (is_homogeneous && curr_size != nbytes) {
        if (*errflag == MPIR_ERR_NONE) *errflag = MPIR_ERR_OTHER;
        MPIR_ERR_SET2(mpi_errno, MPI_ERR_OTHER,
		      "**collective_size_mismatch",
		      "**collective_size_mismatch %d %d", curr_size, nbytes );
        MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
    }

    if (!is_contig || !is_homogeneous)
    {
        if (rank != root)
        {
            position = 0;
            mpi_errno = MPIR_Unpack_impl(tmp_buf, nbytes, &position, buffer,
                                         count, datatype);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        }
    }

fn_exit:
    MPIU_CHKLMEM_FREEALL();
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno_ret)
        mpi_errno = mpi_errno_ret;
    else if (*errflag != MPIR_ERR_NONE)
        MPIR_ERR_SET(mpi_errno, *errflag, "**coll_fail");
    /* --END ERROR HANDLING-- */
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

/*
   Broadcast based on a scatter followed by an allgather.

   We first scatter the buffer using a binomial tree algorithm. This costs
   lgp.alpha + n.((p-1)/p).beta
   If the datatype is contiguous and the communicator is homogeneous,
   we treat the data as bytes and divide (scatter) it among processes
   by using ceiling division. For the noncontiguous or heterogeneous
   cases, we first pack the data into a temporary buffer by using
   MPI_Pack, scatter it as bytes, and unpack it after the allgather.

   We use a ring algorithm for the allgather, which takes p-1 steps.
   This may perform better than recursive doubling for long messages and
   medium-sized non-power-of-two messages.
   Total Cost = (lgp+p-1).alpha + 2.n.((p-1)/p).beta
*/
#undef FUNCNAME
#define FUNCNAME MPIR_Bcast_scatter_ring_allgather
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int MPIR_Bcast_scatter_ring_allgather(
    void *buffer, 
    int count, 
    MPI_Datatype datatype, 
    int root, 
    MPID_Comm *comm_ptr,
    MPIR_Errflag_t *errflag)
{
    int rank, comm_size;
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    int scatter_size;
    int j, i, is_contig, is_homogeneous;
    MPI_Aint nbytes, type_size, position;
    int left, right, jnext;
    int curr_size = 0;
    void *tmp_buf;
    int recvd_size;
    MPI_Status status;
    MPID_Datatype *dtp;
    MPI_Aint true_extent, true_lb;
    MPIU_CHKLMEM_DECL(1);

    comm_size = comm_ptr->local_size;
    rank = comm_ptr->rank;

    /* If there is only one process, return */
    if (comm_size == 1) goto fn_exit;

    if (HANDLE_GET_KIND(datatype) == HANDLE_KIND_BUILTIN)
        is_contig = 1;
    else {
        MPID_Datatype_get_ptr(datatype, dtp); 
        is_contig = dtp->is_contig;
    }

    is_homogeneous = 1;
#ifdef MPID_HAS_HETERO
    if (comm_ptr->is_hetero)
        is_homogeneous = 0;
#endif

    /* MPI_Type_size() might not give the accurate size of the packed
     * datatype for heterogeneous systems (because of padding, encoding,
     * etc). On the other hand, MPI_Pack_size() can become very
     * expensive, depending on the implementation, especially for
     * heterogeneous systems. We want to use MPI_Type_size() wherever
     * possible, and MPI_Pack_size() in other places.
     */
    if (is_homogeneous)
        MPID_Datatype_get_size_macro(datatype, type_size);
    else
	/* --BEGIN HETEROGENEOUS-- */
        MPIR_Pack_size_impl(1, datatype, &type_size);
        /* --END HETEROGENEOUS-- */

    nbytes = type_size * count;
    if (nbytes == 0)
        goto fn_exit; /* nothing to do */

    if (is_contig && is_homogeneous)
    {
        /* contiguous and homogeneous. no need to pack. */
        MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);

        tmp_buf = (char *) buffer + true_lb;
    }
    else
    {
        MPIU_CHKLMEM_MALLOC(tmp_buf, void *, nbytes, mpi_errno, "tmp_buf");

        /* TODO: Pipeline the packing and communication */
        position = 0;
        if (rank == root) {
            mpi_errno = MPIR_Pack_impl(buffer, count, datatype, tmp_buf, nbytes,
                                       &position);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        }
    }

    scatter_size = (nbytes + comm_size - 1)/comm_size; /* ceiling division */

    mpi_errno = scatter_for_bcast(buffer, count, datatype, root, comm_ptr,
                                  nbytes, tmp_buf, is_contig, is_homogeneous, errflag);
    if (mpi_errno) {
        /* for communication errors, just record the error but continue */
        *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
        MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
        MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
    }

    /* long-message allgather or medium-size but non-power-of-two. use ring algorithm. */ 

    /* Calculate how much data we already have */
    curr_size = MPIR_MIN(scatter_size,
                         nbytes - ((rank - root + comm_size) % comm_size) * scatter_size);
    if (curr_size < 0)
        curr_size = 0;

    left  = (comm_size + rank - 1) % comm_size;
    right = (rank + 1) % comm_size;
    j     = rank;
    jnext = left;
    for (i=1; i<comm_size; i++)
    {
        int left_count, right_count, left_disp, right_disp, rel_j, rel_jnext;

        rel_j     = (j     - root + comm_size) % comm_size;
        rel_jnext = (jnext - root + comm_size) % comm_size;
        left_count = MPIR_MIN(scatter_size, (nbytes - rel_jnext * scatter_size));
        if (left_count < 0)
            left_count = 0;
        left_disp = rel_jnext * scatter_size;
        right_count = MPIR_MIN(scatter_size, (nbytes - rel_j * scatter_size));
        if (right_count < 0)
            right_count = 0;
        right_disp = rel_j * scatter_size;

        mpi_errno = MPIC_Sendrecv((char *)tmp_buf + right_disp, right_count,
                                     MPI_BYTE, right, MPIR_BCAST_TAG,
                                     (char *)tmp_buf + left_disp, left_count,
                                     MPI_BYTE, left, MPIR_BCAST_TAG,
                                     comm_ptr, &status, errflag);
        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
            MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
        }
        MPIR_Get_count_impl(&status, MPI_BYTE, &recvd_size);
        curr_size += recvd_size;
        j     = jnext;
        jnext = (comm_size + jnext - 1) % comm_size;
    }

    /* check that we received as much as we expected */
    /* recvd_size may not be accurate for packed heterogeneous data */
    if (is_homogeneous && curr_size != nbytes) {
        if (*errflag == MPIR_ERR_NONE) *errflag = MPIR_ERR_OTHER;
        MPIR_ERR_SET2(mpi_errno, MPI_ERR_OTHER,
		      "**collective_size_mismatch",
		      "**collective_size_mismatch %d %d", curr_size, nbytes );
        MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
    }

    if (!is_contig || !is_homogeneous)
    {
        if (rank != root)
        {
            position = 0;
            mpi_errno = MPIR_Unpack_impl(tmp_buf, nbytes, &position, buffer,
                                         count, datatype);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        }
    }

fn_exit:
    MPIU_CHKLMEM_FREEALL();
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno_ret)
        mpi_errno = mpi_errno_ret;
    else if (*errflag != MPIR_ERR_NONE)
        MPIR_ERR_SET(mpi_errno, *errflag, "**coll_fail");
    /* --END ERROR HANDLING-- */
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

/* A convenience function to de-clutter code and minimize copy-paste bugs.
   Invokes the coll_fns->Bcast override for the given communicator if non-null.
   Otherwise it invokes bcast_fn_ with the given args.
   
   NOTE: calls MPIR_ERR_POP on any failure, so a fn_fail label is needed. */
#define MPIR_Bcast_fn_or_override(bcast_fn_,mpi_errno_ret_,buffer_,count_,datatype_,root_,comm_ptr_,errflag_) \
    do {                                                                                         \
        int mpi_errno_ = MPI_SUCCESS;                                                            \
        if (comm_ptr_->coll_fns != NULL && comm_ptr_->coll_fns->Bcast != NULL)                   \
        {                                                                                        \
            /* --BEGIN USEREXTENSION-- */                                                        \
            mpi_errno_ = comm_ptr_->coll_fns->Bcast(buffer_, count_,                              \
                                                   datatype_, root_, comm_ptr_, errflag_);       \
            /* --END USEREXTENSION-- */                                                          \
        }                                                                                        \
        else                                                                                     \
        {                                                                                        \
            mpi_errno_ = bcast_fn_(buffer_, count_, datatype_, root_, comm_ptr_, errflag_);      \
        }                                                                                        \
        if (mpi_errno_) {                                                                        \
            /* for communication errors, just record the error but continue */                   \
            *(errflag_) = MPIR_ERR_GET_CLASS(mpi_errno_);                                        \
            MPIR_ERR_SET(mpi_errno_, *(errflag_), "**fail");                                     \
            MPIR_ERR_ADD(mpi_errno_ret_, mpi_errno_);                                            \
        }                                                                                        \
    } while (0)

/* FIXME This function uses some heuristsics based off of some testing on a
 * cluster at Argonne.  We need a better system for detrmining and controlling
 * the cutoff points for these algorithms.  If I've done this right, you should
 * be able to make changes along these lines almost exclusively in this function
 * and some new functions. [goodell@ 2008/01/07] */

#undef FUNCNAME
#define FUNCNAME MPIR_SMP_Bcast
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int MPIR_SMP_Bcast(
        void *buffer, 
        int count, 
        MPI_Datatype datatype, 
        int root, 
        MPID_Comm *comm_ptr,
        MPIR_Errflag_t *errflag)
{
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    int is_homogeneous;
    MPI_Aint type_size, nbytes=0;
    MPI_Status status;
    int recvd_size;

    if (!MPIR_CVAR_ENABLE_SMP_COLLECTIVES || !MPIR_CVAR_ENABLE_SMP_BCAST) {
        MPIU_Assert(0);
    }
    MPIU_Assert(MPIR_Comm_is_node_aware(comm_ptr));

    is_homogeneous = 1;
#ifdef MPID_HAS_HETERO
    if (comm_ptr->is_hetero)
        is_homogeneous = 0;
#endif

    /* MPI_Type_size() might not give the accurate size of the packed
     * datatype for heterogeneous systems (because of padding, encoding,
     * etc). On the other hand, MPI_Pack_size() can become very
     * expensive, depending on the implementation, especially for
     * heterogeneous systems. We want to use MPI_Type_size() wherever
     * possible, and MPI_Pack_size() in other places.
     */
    if (is_homogeneous)
        MPID_Datatype_get_size_macro(datatype, type_size);
    else
        /* --BEGIN HETEROGENEOUS-- */
        MPIR_Pack_size_impl(1, datatype, &type_size);
        /* --END HETEROGENEOUS-- */

    nbytes = type_size * count;
    if (nbytes == 0)
        goto fn_exit; /* nothing to do */

    if ((nbytes < MPIR_CVAR_BCAST_SHORT_MSG_SIZE) || (comm_ptr->local_size < MPIR_CVAR_BCAST_MIN_PROCS))
    {
        /* send to intranode-rank 0 on the root's node */
        if (comm_ptr->node_comm != NULL &&
            MPIU_Get_intranode_rank(comm_ptr, root) > 0) /* is not the node root (0) */ 
        {                                                /* and is on our node (!-1) */
            if (root == comm_ptr->rank) {
                mpi_errno = MPIC_Send(buffer,count,datatype,0,
                                         MPIR_BCAST_TAG,comm_ptr->node_comm, errflag);
                if (mpi_errno) {
                    /* for communication errors, just record the error but continue */
                    *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                    MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                    MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                }
            }
            else if (0 == comm_ptr->node_comm->rank) {
                mpi_errno = MPIC_Recv(buffer,count,datatype,MPIU_Get_intranode_rank(comm_ptr, root),
                                         MPIR_BCAST_TAG,comm_ptr->node_comm, &status, errflag);
                if (mpi_errno) {
                    /* for communication errors, just record the error but continue */
                    *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                    MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                    MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                }
                /* check that we received as much as we expected */
                MPIR_Get_count_impl(&status, MPI_BYTE, &recvd_size);
                /* recvd_size may not be accurate for packed heterogeneous data */
                if (is_homogeneous && recvd_size != nbytes) {
                    if (*errflag == MPIR_ERR_NONE) *errflag = MPIR_ERR_OTHER;
                    MPIR_ERR_SET2(mpi_errno, MPI_ERR_OTHER,
				  "**collective_size_mismatch",
				  "**collective_size_mismatch %d %d", 
				  recvd_size, nbytes );
                    MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                }
            }
            
        }

        /* perform the internode broadcast */
        if (comm_ptr->node_roots_comm != NULL)
        {
            MPIR_Bcast_fn_or_override(MPIR_Bcast_binomial, mpi_errno_ret,
                                      buffer, count, datatype,
                                      MPIU_Get_internode_rank(comm_ptr, root),
                                      comm_ptr->node_roots_comm, errflag);
        }

        /* perform the intranode broadcast on all except for the root's node */
        if (comm_ptr->node_comm != NULL)
        {
            MPIR_Bcast_fn_or_override(MPIR_Bcast_binomial, mpi_errno_ret,
                                      buffer, count, datatype, 0, comm_ptr->node_comm, errflag);
        }
    }
    else /* (nbytes > MPIR_CVAR_BCAST_SHORT_MSG_SIZE) && (comm_ptr->size >= MPIR_CVAR_BCAST_MIN_PROCS) */
    {
        /* supposedly...
           smp+doubling good for pof2
           reg+ring better for non-pof2 */
        if (nbytes < MPIR_CVAR_BCAST_LONG_MSG_SIZE && MPIU_is_pof2(comm_ptr->local_size, NULL))
        {
            /* medium-sized msg and pof2 np */

            /* perform the intranode broadcast on the root's node */
            if (comm_ptr->node_comm != NULL &&
                MPIU_Get_intranode_rank(comm_ptr, root) > 0) /* is not the node root (0) */ 
            {                                                /* and is on our node (!-1) */
                /* FIXME binomial may not be the best algorithm for on-node
                   bcast.  We need a more comprehensive system for selecting the
                   right algorithms here. */
                MPIR_Bcast_fn_or_override(MPIR_Bcast_binomial, mpi_errno_ret,
                                          buffer, count, datatype,
                                          MPIU_Get_intranode_rank(comm_ptr, root),
                                          comm_ptr->node_comm, errflag);
            }

            /* perform the internode broadcast */
            if (comm_ptr->node_roots_comm != NULL)
            {
                if (MPIU_is_pof2(comm_ptr->node_roots_comm->local_size, NULL))
                {
                    MPIR_Bcast_fn_or_override(MPIR_Bcast_scatter_doubling_allgather, mpi_errno_ret,
                                              buffer, count, datatype,
                                              MPIU_Get_internode_rank(comm_ptr, root),
                                              comm_ptr->node_roots_comm, errflag);
                }
                else
                {
                    MPIR_Bcast_fn_or_override(MPIR_Bcast_scatter_ring_allgather, mpi_errno_ret,
                                              buffer, count, datatype,
                                              MPIU_Get_internode_rank(comm_ptr, root),
                                              comm_ptr->node_roots_comm, errflag);
                }
            }

            /* perform the intranode broadcast on all except for the root's node */
            if (comm_ptr->node_comm != NULL &&
                MPIU_Get_intranode_rank(comm_ptr, root) <= 0) /* 0 if root was local root too */
            {                                                 /* -1 if different node than root */
                /* FIXME binomial may not be the best algorithm for on-node
                   bcast.  We need a more comprehensive system for selecting the
                   right algorithms here. */
                MPIR_Bcast_fn_or_override(MPIR_Bcast_binomial, mpi_errno_ret,
                                          buffer, count, datatype, 0, comm_ptr->node_comm, errflag);
            }
        }
        else /* large msg or non-pof2 */
        {
            /* FIXME It would be good to have an SMP-aware version of this
               algorithm that (at least approximately) minimized internode
               communication. */
            mpi_errno = MPIR_Bcast_scatter_ring_allgather(buffer, count, datatype, root, comm_ptr, errflag);
            if (mpi_errno) {
                /* for communication errors, just record the error but continue */
                *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
            }
        }
    }

fn_exit:
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno_ret)
        mpi_errno = mpi_errno_ret;
    else if (*errflag != MPIR_ERR_NONE)
        MPIR_ERR_SET(mpi_errno, *errflag, "**coll_fail");
    /* --END ERROR HANDLING-- */
    return mpi_errno;
fn_fail:
    goto fn_exit;
}


/* This is the default implementation of broadcast. The algorithm is:
   
   Algorithm: MPI_Bcast

   For short messages, we use a binomial tree algorithm. 
   Cost = lgp.alpha + n.lgp.beta

   For long messages, we do a scatter followed by an allgather. 
   We first scatter the buffer using a binomial tree algorithm. This costs
   lgp.alpha + n.((p-1)/p).beta
   If the datatype is contiguous and the communicator is homogeneous,
   we treat the data as bytes and divide (scatter) it among processes
   by using ceiling division. For the noncontiguous or heterogeneous
   cases, we first pack the data into a temporary buffer by using
   MPI_Pack, scatter it as bytes, and unpack it after the allgather.

   For the allgather, we use a recursive doubling algorithm for 
   medium-size messages and power-of-two number of processes. This
   takes lgp steps. In each step pairs of processes exchange all the
   data they have (we take care of non-power-of-two situations). This
   costs approximately lgp.alpha + n.((p-1)/p).beta. (Approximately
   because it may be slightly more in the non-power-of-two case, but
   it's still a logarithmic algorithm.) Therefore, for long messages
   Total Cost = 2.lgp.alpha + 2.n.((p-1)/p).beta

   Note that this algorithm has twice the latency as the tree algorithm
   we use for short messages, but requires lower bandwidth: 2.n.beta
   versus n.lgp.beta. Therefore, for long messages and when lgp > 2,
   this algorithm will perform better.

   For long messages and for medium-size messages and non-power-of-two 
   processes, we use a ring algorithm for the allgather, which 
   takes p-1 steps, because it performs better than recursive doubling.
   Total Cost = (lgp+p-1).alpha + 2.n.((p-1)/p).beta

   Possible improvements: 
   For clusters of SMPs, we may want to do something differently to
   take advantage of shared memory on each node.

   End Algorithm: MPI_Bcast
*/

#undef FUNCNAME
#define FUNCNAME MPIR_Bcast_intra
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/* not declared static because it is called in intercomm. allgatherv */
int MPIR_Bcast_intra ( 
        void *buffer, 
        int count, 
        MPI_Datatype datatype, 
        int root, 
        MPID_Comm *comm_ptr,
        MPIR_Errflag_t *errflag )
{
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    int comm_size;
    int nbytes=0;
    int is_homogeneous;
    MPI_Aint type_size;
    MPID_MPI_STATE_DECL(MPID_STATE_MPIR_BCAST);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPIR_BCAST);

    /* check if multiple threads are calling this collective function */
    MPIDU_ERR_CHECK_MULTIPLE_THREADS_ENTER( comm_ptr );

    if (count == 0) goto fn_exit;

    MPID_Datatype_get_size_macro(datatype, type_size);
    nbytes = MPIR_CVAR_MAX_SMP_BCAST_MSG_SIZE ? type_size*count : 0;
    if (MPIR_CVAR_ENABLE_SMP_COLLECTIVES && MPIR_CVAR_ENABLE_SMP_BCAST &&
        nbytes <= MPIR_CVAR_MAX_SMP_BCAST_MSG_SIZE && MPIR_Comm_is_node_aware(comm_ptr)) {
        mpi_errno = MPIR_SMP_Bcast(buffer, count, datatype, root, comm_ptr, errflag);
        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
            MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
        }
        goto fn_exit;
    }

    comm_size = comm_ptr->local_size;

    is_homogeneous = 1;
#ifdef MPID_HAS_HETERO
    if (comm_ptr->is_hetero)
        is_homogeneous = 0;
#endif

    /* MPI_Type_size() might not give the accurate size of the packed
     * datatype for heterogeneous systems (because of padding, encoding,
     * etc). On the other hand, MPI_Pack_size() can become very
     * expensive, depending on the implementation, especially for
     * heterogeneous systems. We want to use MPI_Type_size() wherever
     * possible, and MPI_Pack_size() in other places.
     */
    if (is_homogeneous)
        MPID_Datatype_get_size_macro(datatype, type_size);
    else
	/* --BEGIN HETEROGENEOUS-- */
        MPIR_Pack_size_impl(1, datatype, &type_size);
        /* --END HETEROGENEOUS-- */

    nbytes = type_size * count;
    if (nbytes == 0)
        goto fn_exit; /* nothing to do */

    if ((nbytes < MPIR_CVAR_BCAST_SHORT_MSG_SIZE) || (comm_size < MPIR_CVAR_BCAST_MIN_PROCS))
    {
        mpi_errno = MPIR_Bcast_binomial(buffer, count, datatype, root, comm_ptr, errflag);
        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
            MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
        }
    }
    else /* (nbytes >= MPIR_CVAR_BCAST_SHORT_MSG_SIZE) && (comm_size >= MPIR_CVAR_BCAST_MIN_PROCS) */
    {
        if ((nbytes < MPIR_CVAR_BCAST_LONG_MSG_SIZE) && (MPIU_is_pof2(comm_size, NULL)))
        {
            mpi_errno = MPIR_Bcast_scatter_doubling_allgather(buffer, count, datatype, root, comm_ptr, errflag);
            if (mpi_errno) {
                /* for communication errors, just record the error but continue */
                *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
            }
        }
        else /* (nbytes >= MPIR_CVAR_BCAST_LONG_MSG_SIZE) || !(comm_size_is_pof2) */
        {
            /* We want the ring algorithm whether or not we have a
               topologically aware communicator.  Doing inter/intra-node
               communication phases breaks the pipelining of the algorithm.  */
            mpi_errno = MPIR_Bcast_scatter_ring_allgather(buffer, count, datatype, root, comm_ptr, errflag);
            if (mpi_errno) {
                /* for communication errors, just record the error but continue */
                *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
            }
        }
    }

fn_exit:
    /* check if multiple threads are calling this collective function */
    MPIDU_ERR_CHECK_MULTIPLE_THREADS_EXIT( comm_ptr );

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPIR_BCAST);

    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno_ret)
        mpi_errno = mpi_errno_ret;
    else if (*errflag != MPIR_ERR_NONE)
        MPIR_ERR_SET(mpi_errno, *errflag, "**coll_fail");
    /* --END ERROR HANDLING-- */
    return mpi_errno;
fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPIR_Bcast_inter
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)

/* Not PMPI_LOCAL because it is called in intercomm allgather */
int MPIR_Bcast_inter ( 
    void *buffer, 
    int count, 
    MPI_Datatype datatype, 
    int root, 
    MPID_Comm *comm_ptr,
    MPIR_Errflag_t *errflag)
{
/*  Intercommunicator broadcast.
    Root sends to rank 0 in remote group. Remote group does local
    intracommunicator broadcast.
*/
    int rank, mpi_errno;
    int mpi_errno_ret = MPI_SUCCESS;
    MPI_Status status;
    MPID_Comm *newcomm_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPIR_BCAST_INTER);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPIR_BCAST_INTER);


    if (root == MPI_PROC_NULL)
    {
        /* local processes other than root do nothing */
        mpi_errno = MPI_SUCCESS;
    }
    else if (root == MPI_ROOT)
    {
        /* root sends to rank 0 on remote group and returns */
        MPIDU_ERR_CHECK_MULTIPLE_THREADS_ENTER( comm_ptr );
        mpi_errno =  MPIC_Send(buffer, count, datatype, 0,
                                  MPIR_BCAST_TAG, comm_ptr, errflag);
        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
            MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
        }
        MPIDU_ERR_CHECK_MULTIPLE_THREADS_EXIT( comm_ptr );
    }
    else
    {
        /* remote group. rank 0 on remote group receives from root */
        
        rank = comm_ptr->rank;
        
        if (rank == 0)
        {
            mpi_errno = MPIC_Recv(buffer, count, datatype, root,
                                     MPIR_BCAST_TAG, comm_ptr, &status, errflag);
            if (mpi_errno) {
                /* for communication errors, just record the error but continue */
                *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
            }
        }
        
        /* Get the local intracommunicator */
        if (!comm_ptr->local_comm)
            MPIR_Setup_intercomm_localcomm( comm_ptr );

        newcomm_ptr = comm_ptr->local_comm;

        /* now do the usual broadcast on this intracommunicator
           with rank 0 as root. */
        mpi_errno = MPIR_Bcast_intra(buffer, count, datatype, 0, newcomm_ptr, errflag);
        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
            MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
        }
    }

fn_fail:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPIR_BCAST_INTER);
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno_ret)
        mpi_errno = mpi_errno_ret;
    else if (*errflag != MPIR_ERR_NONE)
        MPIR_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**coll_fail");
    /* --END ERROR HANDLING-- */
    return mpi_errno;
}


/* MPIR_Bcast_impl should be called by any internal component that
   would otherwise call MPI_Bcast.  This differs from MPIR_Bcast in
   that this will call the coll_fns version if it exists and will use
   the SMP-aware bcast algorithm. */
#undef FUNCNAME
#define FUNCNAME MPIR_Bcast_impl
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Bcast_impl(void *buffer, int count, MPI_Datatype datatype, int root, MPID_Comm *comm_ptr, MPIR_Errflag_t *errflag)
{
    int mpi_errno = MPI_SUCCESS;

    if (comm_ptr->coll_fns != NULL && comm_ptr->coll_fns->Bcast != NULL)
    {
	/* --BEGIN USEREXTENSION-- */
	mpi_errno = comm_ptr->coll_fns->Bcast(buffer, count,
                                              datatype, root, comm_ptr, errflag);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
	/* --END USEREXTENSION-- */
    }
    else
    {
        mpi_errno = MPIR_Bcast(buffer, count, datatype, root, comm_ptr, errflag);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }


 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

/* MPIR_Bcast performs an broadcast using point-to-point messages.
   This is intended to be used by device-specific implementations of
   broadcast.  In all other cases MPIR_Bcast_impl should be used. */
#undef FUNCNAME
#define FUNCNAME MPIR_Bcast
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
inline int MPIR_Bcast(void *buffer, int count, MPI_Datatype datatype, int root, MPID_Comm *comm_ptr, MPIR_Errflag_t *errflag)
{
    int mpi_errno = MPI_SUCCESS;

    if (comm_ptr->comm_kind == MPID_INTRACOMM) {
        /* intracommunicator */
        mpi_errno = MPIR_Bcast_intra( buffer, count, datatype, root, comm_ptr, errflag );
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        
    } else {
        /* intercommunicator */
        mpi_errno = MPIR_Bcast_inter( buffer, count, datatype, root, comm_ptr, errflag );
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


#endif /* MPICH_MPI_FROM_PMPI */

#undef FUNCNAME
#define FUNCNAME MPI_Bcast
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)

/*@
MPI_Bcast - Broadcasts a message from the process with rank "root" to
            all other processes of the communicator

Input/Output Parameters:
. buffer - starting address of buffer (choice) 

Input Parameters:
+ count - number of entries in buffer (integer) 
. datatype - data type of buffer (handle) 
. root - rank of broadcast root (integer) 
- comm - communicator (handle) 

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_COMM
.N MPI_ERR_COUNT
.N MPI_ERR_TYPE
.N MPI_ERR_BUFFER
.N MPI_ERR_ROOT
@*/
int MPI_Bcast( void *buffer, int count, MPI_Datatype datatype, int root, 
               MPI_Comm comm )
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_BCAST);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPID_MPI_COLL_FUNC_ENTER(MPID_STATE_MPI_BCAST);

    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_COMM(comm, mpi_errno);
	}
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* Convert MPI object handles to object pointers */
    MPID_Comm_get_ptr( comm, comm_ptr );

    /* Validate parameters and objects (post conversion) */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPID_Datatype *datatype_ptr = NULL;
	    
            MPID_Comm_valid_ptr( comm_ptr, mpi_errno, FALSE );
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;
	    MPIR_ERRTEST_COUNT(count, mpi_errno);
	    MPIR_ERRTEST_DATATYPE(datatype, "datatype", mpi_errno);
	    if (comm_ptr->comm_kind == MPID_INTRACOMM) {
		MPIR_ERRTEST_INTRA_ROOT(comm_ptr, root, mpi_errno);
	    }
	    else {
		MPIR_ERRTEST_INTER_ROOT(comm_ptr, root, mpi_errno);
	    }
	    
            if (HANDLE_GET_KIND(datatype) != HANDLE_KIND_BUILTIN) {
                MPID_Datatype_get_ptr(datatype, datatype_ptr);
                MPID_Datatype_valid_ptr( datatype_ptr, mpi_errno );
                if (mpi_errno != MPI_SUCCESS) goto fn_fail;
                MPID_Datatype_committed_ptr( datatype_ptr, mpi_errno );
                if (mpi_errno != MPI_SUCCESS) goto fn_fail;
            }

            MPIR_ERRTEST_BUF_INPLACE(buffer, count, mpi_errno);
            MPIR_ERRTEST_USERBUFFER(buffer,count,datatype,mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    
    mpi_errno = MPIR_Bcast_impl( buffer, count, datatype, root, comm_ptr, &errflag );
    if (mpi_errno) goto fn_fail;

    /* ... end of body of routine ... */
    
  fn_exit:
    MPID_MPI_COLL_FUNC_EXIT(MPID_STATE_MPI_BCAST);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, 
	    "**mpi_bcast",
	    "**mpi_bcast %p %d %D %d %C", buffer, count, datatype, root, comm);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
