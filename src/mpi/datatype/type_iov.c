/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <mpiimpl.h>
#include <mpir_dataloop.h>
#include <stdlib.h>

/*
 * MPIR_Type_to_iov_len
 *
 * Parameters:
 * datatype       - (IN)  datatype to convert to iov
 * count          - (IN)  number of datatype elements
 * offset         - (IN)  skip so many bytes before converting to iov
 * max_bytes      - (IN)  maximum number of bytes to convert to iov
 * actual_bytes   - (OUT) actual number of bytes converted to iov
 * iov_len        - (OUT) final iov length
 */
#undef FUNCNAME
#define FUNCNAME MPIR_Type_to_iov_len
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Type_to_iov_len(MPI_Datatype datatype, int count, MPI_Aint offset,
                         MPI_Aint max_bytes, MPI_Aint * actual_bytes, MPI_Aint * iov_len)
{
    MPIR_Segment *seg;
    size_t total_bytes;
    MPIR_Datatype *datatype_ptr;
    int mpi_errno = MPI_SUCCESS;

    MPIR_Assert(max_bytes > 0);
    MPIR_Assert(iov_len);

    MPIR_Datatype_get_ptr(datatype, datatype_ptr);

    if (datatype_ptr->size == 0) {
        if (actual_bytes)
            *actual_bytes = 0;
        *iov_len = 0;
    } else if (datatype_ptr->is_contig) {
        if (actual_bytes) {
        *actual_bytes = count * datatype_ptr->size;
        if (*actual_bytes > max_bytes)
            *actual_bytes = max_bytes;
        }
        *iov_len = 1;
    } else {
        seg = MPIR_Segment_alloc(NULL, count, datatype);
        MPIR_ERR_CHKANDJUMP(!seg, mpi_errno, MPI_ERR_OTHER, "**nomem");

        total_bytes = max_bytes;
        MPIR_Segment_count_contig_blocks(seg, offset, &total_bytes, iov_len);
        if (actual_bytes)
        *actual_bytes = total_bytes;

        MPIR_Segment_free(seg);
    }

  fn_exit:
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

/*
 * MPIR_Type_to_iov
 *
 * Parameters:
 * buf            - (IN)  buffer address to convert to iov
 * datatype       - (IN)  datatype to convert to iov
 * count          - (IN)  number of datatype elements
 * offset         - (IN)  skip so many bytes before converting to iov
 * max_bytes      - (IN)  maximum number of bytes to convert to iov
 * actual_bytes   - (OUT) actual number of bytes converted to iov
 * max_iov_len    - (IN)  maximum iov length
 * actual_iov_len - (OUT) actual iov length
 * iov            - (OUT) final iov
 */
#undef FUNCNAME
#define FUNCNAME MPIR_Type_to_iov
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Type_to_iov(void *buf, MPI_Datatype datatype, int count, MPI_Aint offset,
                     MPI_Aint max_bytes, MPI_Aint * actual_bytes,
                     int max_iov_len, int *actual_iov_len, MPL_IOV * iov)
{
    MPIR_Segment *seg;
    size_t last;
    int iov_len;
    MPIR_Datatype *datatype_ptr;
    int mpi_errno = MPI_SUCCESS;

    MPIR_Assert(max_bytes > 0);
    MPIR_Assert(max_iov_len > 0);
    MPIR_Assert(iov);

    MPIR_Datatype_get_ptr(datatype, datatype_ptr);

    if (datatype_ptr->size == 0) {
        if (actual_bytes)
        *actual_bytes = 0;
        if (actual_iov_len)
        *actual_iov_len = 0;
    } else if (datatype_ptr->is_contig) {
        if (actual_bytes) {
        *actual_bytes = count * datatype_ptr->size;
        if (*actual_bytes > max_bytes)
            *actual_bytes = max_bytes;
        }

        if (actual_iov_len)
        *actual_iov_len = 1;

        iov[0].MPL_IOV_BUF = buf;
        iov[0].MPL_IOV_LEN = *actual_bytes;
    } else {
        seg = MPIR_Segment_alloc(buf, count, datatype);
        MPIR_ERR_CHKANDJUMP(!seg, mpi_errno, MPI_ERR_OTHER, "**nomem");

        last = offset + max_bytes;
        iov_len = max_iov_len;

        MPIR_Segment_to_iov(seg, offset, &last, iov, &iov_len);

        if (actual_bytes)
        *actual_bytes = last - offset;
        if (actual_iov_len)
        *actual_iov_len = iov_len;

        MPIR_Segment_free(seg);
    }

  fn_exit:
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}
