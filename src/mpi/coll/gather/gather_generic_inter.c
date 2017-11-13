/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

#undef FUNCNAME
#define FUNCNAME MPIR_Gather_generic_inter
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Gather_generic_inter(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                      int recvcount, MPI_Datatype recvtype, int root, MPIR_Comm *comm_ptr,
                      MPIR_Errflag_t *errflag)
{
    /*  Intercommunicator gather.
        For short messages, remote group does a local intracommunicator
        gather to rank 0. Rank 0 then sends data to root.

Cost: (lgp+1).alpha + n.((p-1)/p).beta + n.beta

For long messages, we use linear gather to avoid the extra n.beta.

Cost: p.alpha + n.beta
*/

    int rank, local_size, remote_size, mpi_errno=MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    int i;
    MPI_Aint nbytes, sendtype_size, recvtype_size;
    MPI_Status status;
    MPI_Aint extent, true_extent, true_lb = 0;
    void *tmp_buf=NULL;
    MPIR_Comm *newcomm_ptr = NULL;
    MPIR_CHKLMEM_DECL(1);

    if (root == MPI_PROC_NULL)
    {
        /* local processes other than root do nothing */
        return MPI_SUCCESS;
    }

    remote_size = comm_ptr->remote_size; 
    local_size = comm_ptr->local_size; 

    if (root == MPI_ROOT)
    {
        MPIR_Datatype_get_size_macro(recvtype, recvtype_size);
        nbytes = recvtype_size * recvcount * remote_size;
    }
    else
    {
        /* remote side */
        MPIR_Datatype_get_size_macro(sendtype, sendtype_size);
        nbytes = sendtype_size * sendcount * local_size;
    }

    if (nbytes < MPIR_CVAR_GATHER_INTER_SHORT_MSG_SIZE)
    {
        if (root == MPI_ROOT)
        {
            /* root receives data from rank 0 on remote group */
            mpi_errno = MPIC_Recv(recvbuf, recvcount*remote_size,
                    recvtype, 0, MPIR_GATHER_TAG, comm_ptr,
                    &status, errflag);
            if (mpi_errno) {
                /* for communication errors, just record the error but continue */
                *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
            }
        }
        else
        {
            /* remote group. Rank 0 allocates temporary buffer, does
               local intracommunicator gather, and then sends the data
               to root. */

            rank = comm_ptr->rank;

            if (rank == 0)
            {
                MPIR_Type_get_true_extent_impl(sendtype, &true_lb, &true_extent);
                MPIR_Datatype_get_extent_macro(sendtype, extent);

                MPIR_Ensure_Aint_fits_in_pointer(sendcount*local_size*
                        (MPL_MAX(extent, true_extent)));
                MPIR_CHKLMEM_MALLOC(tmp_buf, void *, sendcount*local_size*(MPL_MAX(extent,true_extent)), mpi_errno, "tmp_buf", MPL_MEM_BUFFER);
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
            mpi_errno = MPID_Gather(sendbuf, sendcount, sendtype,
                    tmp_buf, sendcount, sendtype, 0,
                    newcomm_ptr, errflag);
            if (mpi_errno) {
                /* for communication errors, just record the error but continue */
                *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
            }

            if (rank == 0)
            {
                mpi_errno = MPIC_Send(tmp_buf, sendcount*local_size,
                        sendtype, root,
                        MPIR_GATHER_TAG, comm_ptr, errflag);
                if (mpi_errno) {
                    /* for communication errors, just record the error but continue */
                    *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                    MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                    MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                }
            }
        }
    }
    else
    {
        /* long message. use linear algorithm. */
        if (root == MPI_ROOT)
        {
            MPIR_Datatype_get_extent_macro(recvtype, extent);
            MPIR_Ensure_Aint_fits_in_pointer(MPIR_VOID_PTR_CAST_TO_MPI_AINT recvbuf +
                    (recvcount*remote_size*extent));

            for (i=0; i<remote_size; i++)
            {
                mpi_errno = MPIC_Recv(((char *)recvbuf+recvcount*i*extent),
                        recvcount, recvtype, i,
                        MPIR_GATHER_TAG, comm_ptr, &status, errflag);
                if (mpi_errno) {
                    /* for communication errors, just record the error but continue */
                    *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                    MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                    MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                }
            }
        }
        else
        {
            mpi_errno = MPIC_Send(sendbuf,sendcount,sendtype,root,
                    MPIR_GATHER_TAG,comm_ptr, errflag);
            if (mpi_errno) {
                /* for communication errors, just record the error but continue */
                *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
            }
        }
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
