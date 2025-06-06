#include "mpiimpl.h"
#include <math.h>

int MPIR_Allreduce_intra_circ_vring(const void* sendbuf,
                                    void* recvbuf,
                                    MPI_Aint count,
                                    MPI_Datatype datatype,
                                    MPI_Op op,
                                    MPIR_Comm* comm,
                                    MPIR_Errflag_t errflag)
{
    int mpi_errno = MPI_SUCCESS;

    int comm_size, rank;
    comm_size = comm->local_size;
    rank = comm->rank;

    MPIR_CHKLMEM_DECL();

    // I just yoinked this so I hope it does what I expect
    MPI_Aint true_extent, true_lb, extent;
    MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);
    MPIR_Datatype_get_extent_macro(datatype, extent);
    MPI_Aint single_size = extent * count - (extent - true_extent);
    if (extent > true_extent) {
        single_size -= (extent - true_extent);
        if (single_size % MAX_ALIGNMENT) {
            single_size += MAX_ALIGNMENT - single_size % MAX_ALIGNMENT;
        }
    }

    if (comm_size == 1) {
        if (sendbuf != MPI_IN_PLACE) {
            mpi_errno = MPIR_Localcopy(sendbuf, count, datatype, recvbuf, count, datatype);
            MPIR_ERR_CHECK(mpi_errno);
        }
        goto fn_exit;
    }

    void* tmp_buf;
    MPIR_CHKLMEM_MALLOC(tmp_buf, count * single_size);
    tmp_buf = (void*) ((char*) tmp_buf - true_lb);
    
    void* modified_send_buf;
    MPIR_CHKLMEM_MALLOC(modified_send_buf, count * single_size);
    modified_send_buf = (void*) ((char*) modified_send_buf - true_lb);
    
    MPIR_Request* requests[2];

    int current_skip = (comm_size / 2) + (comm_size & 0x1);

    int to = (rank - current_skip + comm_size) % comm_size;
    int from = (rank + current_skip) % comm_size;
    mpi_errno = MPIC_Irecv(recvbuf, count, datatype, from, MPIR_ALLREDUCE_TAG, comm, &requests[0]);
    MPIR_ERR_CHECK(mpi_errno);
    mpi_errno = MPIC_Isend(sendbuf, count, datatype, to, MPIR_ALLREDUCE_TAG, comm, &requests[1], errflag);
    MPIR_ERR_CHECK(mpi_errno);
    if (current_skip == 1) {
        mpi_errno = MPIC_Waitall(2, requests, MPI_STATUSES_IGNORE);
        MPIR_ERR_CHECK(mpi_errno);
        goto fn_exit;
    } else {
        mpi_errno = MPIR_Localcopy(sendbuf, count, datatype, tmp_buf, count, datatype);
        MPIR_ERR_CHECK(mpi_errno);
        mpi_errno = MPIC_Waitall(2, requests, MPI_STATUSES_IGNORE);
        MPIR_ERR_CHECK(mpi_errno);
    }

    do {
        int adjust = current_skip & 0x1;
        current_skip = (current_skip / 2) + adjust;
        int to = (rank - (current_skip - adjust) + comm_size) % comm_size;
        int from = (rank + (current_skip - adjust)) % comm_size;
        mpi_errno = MPIC_Irecv(tmp_buf, count, datatype, from, MPIR_ALLREDUCE_TAG, comm, &requests[0]);
        MPIR_ERR_CHECK(mpi_errno);
        if (adjust) {
            mpi_errno = MPIC_Isend(recvbuf, count, datatype, to, MPIR_ALLREDUCE_TAG, comm, &requests[1], errflag);
            MPIR_ERR_CHECK(mpi_errno);
        } else {
            mpi_errno = MPIR_Localcopy(sendbuf, count, datatype, modified_send_buf, count, datatype);
            MPIR_ERR_CHECK(mpi_errno);
            mpi_errno = MPIR_Reduce_local(tmp_buf, modified_send_buf, count, datatype, op);
            MPIR_ERR_CHECK(mpi_errno);
            mpi_errno = MPIC_Isend(recvbuf, count, datatype, to, MPIR_ALLREDUCE_TAG, comm, &requests[1], errflag);
            MPIR_ERR_CHECK(mpi_errno);
        }
        mpi_errno = MPIC_Waitall(2, requests, MPI_STATUSES_IGNORE);
        MPIR_ERR_CHECK(mpi_errno);

        mpi_errno = MPIR_Reduce_local(tmp_buf, recvbuf, count, datatype, op);
        MPIR_ERR_CHECK(mpi_errno);
    } while (current_skip != 1);
    
    mpi_errno = MPIR_Reduce_local(sendbuf, recvbuf, count, datatype, op);
    MPIR_ERR_CHECK(mpi_errno);

fn_exit:
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno;
fn_fail:
    goto fn_exit;
}
