/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* All internal usage of MPIX_EQUAL should come from MPIR_Reduce_equal and MPIR_Allreduce_equal,
 * where datatype is MPI_BYTE. Data buffer is a packed buffer where the first 8 bytes holds the
 * comparison boolean results.
 */

struct equal_data {
    uint64_t is_equal;
    char data[];
};

#define EQUAL_DATA_OVERHEAD sizeof(struct equal_data)

void MPIR_EQUAL(void *invec, void *inoutvec, MPI_Aint * Len, MPI_Datatype * type)
{
    MPIR_Assert(*Len >= EQUAL_DATA_OVERHEAD);
    MPIR_Assert(*type == MPI_BYTE);

    struct equal_data *in = invec;
    struct equal_data *inout = inoutvec;

    if (in->is_equal != 1 || inout->is_equal != 1) {
        inout->is_equal = 0;
    } else {
        if (memcmp(in->data, inout->data, (*Len) - EQUAL_DATA_OVERHEAD) != 0) {
            inout->is_equal = 0;
        }
    }
}

int MPIR_EQUAL_check_dtype(MPI_Datatype type)
{
    return MPI_SUCCESS;
}

#define PREPARE_EQUAL_LOCAL_BUF \
    MPI_Aint type_sz, byte_count; \
    MPIR_Datatype_get_size_macro(datatype, type_sz); \
    byte_count = count * type_sz + EQUAL_DATA_OVERHEAD; \
    \
    struct equal_data *local_buf; \
    local_buf = MPL_malloc(byte_count, MPL_MEM_OTHER); \
    MPIR_Assert(local_buf); \
    \
    local_buf->is_equal = 1; \
    MPI_Aint actual_pack_bytes; \
    MPIR_Typerep_pack(sendbuf, count, datatype, 0, local_buf->data, count * type_sz, \
                        &actual_pack_bytes, MPIR_TYPEREP_FLAG_NONE); \
    MPIR_Assert(actual_pack_bytes == count * type_sz)

int MPIR_Reduce_equal(const void *sendbuf, MPI_Aint count, MPI_Datatype datatype,
                      int *is_equal, int root, MPIR_Comm * comm_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    PREPARE_EQUAL_LOCAL_BUF;

    /* Not all algorithm will work. In particular, we can't split the message */
    if (comm_ptr->rank == root) {
        mpi_errno = MPIR_Reduce_intra_binomial(MPI_IN_PLACE, local_buf, byte_count, MPI_BYTE,
                                               MPIX_EQUAL, root, comm_ptr, MPIR_ERR_NONE);
    } else {
        mpi_errno = MPIR_Reduce_intra_binomial(local_buf, NULL, byte_count, MPI_BYTE,
                                               MPIX_EQUAL, root, comm_ptr, MPIR_ERR_NONE);
    }
    MPIR_ERR_CHECK(mpi_errno);

    if (comm_ptr->rank == root) {
        *is_equal = local_buf->is_equal;
    }

  fn_exit:
    MPL_free(local_buf);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


int MPIR_Allreduce_equal(const void *sendbuf, MPI_Aint count, MPI_Datatype datatype,
                         int *is_equal, MPIR_Comm * comm_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    PREPARE_EQUAL_LOCAL_BUF;

    /* Not all algorithm will work. In particular, we can't split the message */
    mpi_errno = MPIR_Allreduce_intra_recursive_doubling(MPI_IN_PLACE, local_buf,
                                                        byte_count, MPI_BYTE,
                                                        MPIX_EQUAL, comm_ptr, MPIR_ERR_NONE);
    MPIR_ERR_CHECK(mpi_errno);

    *is_equal = local_buf->is_equal;

  fn_exit:
    MPL_free(local_buf);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
