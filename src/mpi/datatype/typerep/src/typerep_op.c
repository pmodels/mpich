/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "typerep_internal.h"

/* MPII_Typerep_op_fallback - accumulate source_buf onto target_buf with op.
 * Assume (we may relax and extend in the future)
 *   - source datatype is predefined (basic or pairtype)
 *   - target datatype may be derived with the same basic type as source
 *   - op is builtin op
 */
static int typerep_op_fallback(void *source_buf, MPI_Aint source_count, MPI_Datatype source_dtp,
                               void *target_buf, MPI_Aint target_count, MPI_Datatype target_dtp,
                               MPI_Op op);

int MPII_Typerep_op_fallback(void *source_buf, MPI_Aint source_count, MPI_Datatype source_dtp,
                             void *target_buf, MPI_Aint target_count, MPI_Datatype target_dtp,
                             MPI_Op op, bool source_is_packed)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = (*MPIR_OP_HDL_TO_DTYPE_FN(op)) (source_dtp);
    MPIR_ERR_CHECK(mpi_errno);

    /* unpack source buffer if necessary */
    bool source_unpacked = false;
    if (source_is_packed) {
        MPI_Aint source_dtp_size, source_dtp_extent;
        MPIR_Datatype_get_size_macro(source_dtp, source_dtp_size);
        MPIR_Datatype_get_extent_macro(source_dtp, source_dtp_extent);
        if (source_dtp_size != source_dtp_extent) {
            /* assume source_dtp is a pairtype */
            MPIR_Assert(MPIR_DATATYPE_IS_PREDEFINED(source_dtp));
            void *src_ptr = MPL_malloc(source_dtp_extent * source_count, MPL_MEM_OTHER);
            MPI_Aint unpack_size;
            MPIR_Typerep_unpack(source_buf, source_dtp_size * source_count, src_ptr,
                                source_count, source_dtp, 0, &unpack_size, MPIR_TYPEREP_REQ_NULL);
            source_buf = src_ptr;
            source_unpacked = true;
        }
    }
    /* swap host buffers if necessary */
    void *save_targetbuf = NULL;
    void *host = MPIR_gpu_host_swap(target_buf, target_count, target_dtp);
    if (host) {
        save_targetbuf = target_buf;
        target_buf = host;
    }

    mpi_errno = typerep_op_fallback(source_buf, source_count, source_dtp,
                                    target_buf, target_count, target_dtp, op);

    if (save_targetbuf) {
        MPIR_gpu_swap_back(target_buf, save_targetbuf, target_count, target_dtp);
        target_buf = save_targetbuf;
    }
    if (source_unpacked) {
        MPL_free(source_buf);
    }
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int typerep_op_fallback(void *source_buf, MPI_Aint source_count, MPI_Datatype source_dtp,
                               void *target_buf, MPI_Aint target_count, MPI_Datatype target_dtp,
                               MPI_Op op)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_op_function *uop;
    uop = MPIR_OP_HDL_TO_FN(op);

    if (HANDLE_IS_BUILTIN(target_dtp)) {
        MPIR_Assert(source_dtp == target_dtp);
        MPIR_Assert(source_count == target_count);

        (*uop) (source_buf, target_buf, &target_count, &target_dtp);
    } else {
        /* check source type */
        MPI_Aint source_dtp_size, source_dtp_extent;
        MPIR_Datatype_get_size_macro(source_dtp, source_dtp_size);
        MPIR_Datatype_get_extent_macro(source_dtp, source_dtp_extent);
        bool is_pairtype = (source_dtp_size < source_dtp_extent);

        /* get target iov */
        MPI_Aint vec_len;
        struct iovec *typerep_vec = NULL;
        {
            MPIR_Datatype *dtp ATTRIBUTE((unused));
            MPIR_Datatype_get_ptr(target_dtp, dtp);
            MPIR_Assert(dtp != NULL);
            MPIR_Assert(dtp->basic_type == source_dtp);
            MPIR_Assert(dtp->basic_type != MPI_DATATYPE_NULL);

            mpi_errno = MPIR_Typerep_get_iov_len(target_count, target_dtp, &vec_len);
            MPIR_ERR_CHECK(mpi_errno);
            typerep_vec = (struct iovec *)
                MPL_malloc(vec_len * sizeof(struct iovec), MPL_MEM_OTHER);
            MPIR_ERR_CHKANDJUMP(!typerep_vec, mpi_errno, MPI_ERR_OTHER, "**nomem");

            MPI_Aint actual_iov_len;
            MPIR_Typerep_to_iov_offset(NULL, target_count, target_dtp, 0, typerep_vec, vec_len,
                                       &actual_iov_len);
            vec_len = actual_iov_len;
            MPIR_Assert(vec_len <= INT_MAX);
        }

        /* iterate iov */
        void *source_ptr = source_buf;
        void *target_ptr = NULL;
        MPI_Aint curr_len = 0;  /* current target buffer space discounting gaps */
        for (int i = 0; i < vec_len; i++) {
            if (is_pairtype) {
                if (curr_len == 0) {
                    target_ptr = (char *) target_buf + MPIR_Ptr_to_aint(typerep_vec[i].iov_base);
                }
                curr_len += typerep_vec[i].iov_len;
                if (curr_len < source_dtp_size) {
                    continue;
                }
            } else {
                target_ptr = (char *) target_buf + MPIR_Ptr_to_aint(typerep_vec[i].iov_base);
                curr_len = typerep_vec[i].iov_len;
            }

            MPI_Aint count = curr_len / source_dtp_size;
            MPI_Aint data_sz = count * source_dtp_size;
            MPI_Aint stride = count * source_dtp_extent;

            (*uop) (source_ptr, target_ptr, &count, &source_dtp);

            source_ptr = (char *) source_ptr + stride;
            if (is_pairtype) {
                curr_len -= data_sz;
                if (curr_len > 0) {
                    target_ptr =
                        (char *) target_buf + MPIR_Ptr_to_aint(typerep_vec[i].iov_base) +
                        (typerep_vec[i].iov_len - curr_len);
                }
            } else {
                MPIR_Assert(curr_len == data_sz);
            }
        }

        MPL_free(typerep_vec);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
