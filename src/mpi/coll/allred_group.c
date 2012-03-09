/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2012 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* Map process IDs onto a binary tree.
 *
 * [in]  comm_ptr  Parent communicator
 * [in]  group_ptr Group on which to map the tree (must be a subgroup of comm_ptr's group)
 * [out] root      Process id of the root
 * [out] up        Process id of my parent
 * [out] left      Process id of my left child
 * [out] right     Process id of my right child
 */
static void MPIR_Allreduce_group_bintree(MPID_Comm *comm_ptr, MPID_Group *group_ptr, int *root,
                                         int *up, int *left, int *right)
{
    int ranks_in[4], ranks_out[4];
    int me, nproc;

    me    = group_ptr->rank;
    nproc = group_ptr->size;

    *root = 0;
    *up   = (me == 0) ? MPI_PROC_NULL : (me - 1) / 2;

    *left = 2*me + 1;
    if (*left >= nproc) *left = MPI_PROC_NULL;

    *right = 2*me + 2;
    if (*right >= nproc) *right = MPI_PROC_NULL;

    ranks_in[0] = *root;
    ranks_in[1] = *up;
    ranks_in[2] = *left;
    ranks_in[3] = *right;

    MPIR_Group_translate_ranks_impl(group_ptr, 4, ranks_in, comm_ptr->local_group, ranks_out);

    *root = ranks_out[0];
    *up   = ranks_out[1];
    *left = ranks_out[2];
    *right= ranks_out[3];
}



/* Perform group-collective allreduce (binary tree reduce-broadcast algorithm).
 *
 * [in]  sendbuf   Input buffer
 * [out] recvbuf   Output buffer
 * [in]  count     Number of data elements
 * [in]  datatype  Must be MPI_INT
 * [in]  op        One of: MPI_BAND, MPI_MAX
 * [in]  comm_ptr  Parent communicator
 * [in]  group_ptr Group of processes that will participate in the allreduce
 *                       (must be a subgroup of comm_ptr's group)
 * [in]  tag       Tag to be used for allreduce messages
 * [out] errflag   Error flag
 */
int MPIR_Allreduce_group(void *sendbuf, void *recvbuf, int count, 
                         MPI_Datatype datatype, MPI_Op op, MPID_Comm *comm_ptr,
                         MPID_Group *group_ptr, int tag, int *errflag)
{

    MPI_Status status;
    int i, bin_root, bin_parent, bin_lchild, bin_rchild;
    int *buf_left, *buf_right, *in_buf, *out_buf;

    MPIU_Assert(datatype == MPI_INT);

    buf_left  = (int*) MPIU_Malloc(sizeof(int)*count);
    buf_right = (int*) MPIU_Malloc(sizeof(int)*count);
    out_buf   = (int*) recvbuf;
    in_buf    = (int*) ((sendbuf == MPI_IN_PLACE) ? recvbuf : sendbuf);

    /* Map a binary spanning tree on members of group in comm.  Resulting
     * ranks will be MPI_PROC_NULL if, e.g., a left child does not exist, so
     * it's fine to unconditionally send/recv since these will be no-ops. */
    MPIR_Allreduce_group_bintree(comm_ptr, group_ptr, &bin_root, &bin_parent, &bin_lchild, &bin_rchild);

    /** REDUCE **/

    /* Receive from left child */
    MPIC_Recv(buf_left, count, datatype, bin_lchild, tag, comm_ptr->handle, &status);

    /* Receive from right child */
    MPIC_Recv(buf_right, count, datatype, bin_rchild, tag, comm_ptr->handle, &status);

    /* Initialize local reduction output to input values */
    if (sendbuf != MPI_IN_PLACE)
        for (i = 0; i < count; i++)
            out_buf[i] = in_buf[i];

    /* Perform local reduction on left-, and right-child values if they exist */
    if (op == MPI_BAND) {
        if (bin_lchild != MPI_PROC_NULL) {
            for (i = 0; i < count; i++)
                out_buf[i] = out_buf[i] & buf_left[i];
        }

        if (bin_rchild != MPI_PROC_NULL) {
            for (i = 0; i < count; i++)
                out_buf[i] = out_buf[i] & buf_right[i];
        }
    }
    else if (op == MPI_MAX) {
        if (bin_lchild != MPI_PROC_NULL) {
            for (i = 0; i < count; i++)
                out_buf[i] = (out_buf[i] > buf_left[i]) ? out_buf[i] : buf_left[i];
        }
        
        if (bin_rchild != MPI_PROC_NULL) {
            for (i = 0; i < count; i++)
                out_buf[i] = (out_buf[i] > buf_right[i]) ? out_buf[i] : buf_right[i];
        }
    }
    else {
        MPIU_Assert(FALSE);
    }

    /* Send to parent */
    MPIC_Send(recvbuf, count, datatype, bin_parent, tag, comm_ptr->handle);

    /** BROADCAST **/

    /* Receive from parent */
    MPIC_Recv(recvbuf, count, datatype, bin_parent, tag, comm_ptr->handle, &status);

    /* Send to left child */
    MPIC_Send(recvbuf, count, datatype, bin_lchild, tag, comm_ptr->handle);

    /* Send to right child */
    MPIC_Send(recvbuf, count, datatype, bin_rchild, tag, comm_ptr->handle);

    MPIU_Free(buf_left);
    MPIU_Free(buf_right);

    return MPI_SUCCESS;
}
