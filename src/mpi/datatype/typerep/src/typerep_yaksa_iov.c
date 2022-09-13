/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "yaksa.h"
#include "typerep_internal.h"
#include <assert.h>
#include <sys/time.h>

/* There is a mismatch between the Typerep API and the yaksa API with
 * respect to IOVs.  Typerep accepts IOV offsets in terms of bytes,
 * while yaksa accepts them in terms of IOV elements.  We try to
 * workaround this mismatch in our code, but perhaps it would be
 * cleaner if the Typerep API was modified to match what yaksa
 * provides. */
int MPIR_Typerep_to_iov(const void *buf, MPI_Aint count, MPI_Datatype datatype,
                        MPI_Aint byte_offset, struct iovec *iov, MPI_Aint max_iov_len,
                        MPI_Aint max_iov_bytes, MPI_Aint * actual_iov_len,
                        MPI_Aint * actual_iov_bytes)
{
    MPIR_FUNC_ENTER;

    int mpi_errno = MPI_SUCCESS;
    int rc;

    yaksa_type_t type = MPII_Typerep_get_yaksa_type(datatype);

    uintptr_t size;
    rc = yaksa_type_get_size(type, &size);
    MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_INTERN, "**yaksa");

    uintptr_t yaksa_max_iov_len;
    rc = yaksa_iov_len(count, type, &yaksa_max_iov_len);
    MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_INTERN, "**yaksa");

    /* fast-path: user wants the full IOV */
    if (byte_offset == 0 && max_iov_bytes >= count * size && max_iov_len == yaksa_max_iov_len) {
        uintptr_t yaksa_actual_iov_len;
        rc = yaksa_iov(buf, count, type, 0, iov, (uintptr_t) max_iov_len, &yaksa_actual_iov_len);
        MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_INTERN, "**yaksa");

        assert(max_iov_len == yaksa_actual_iov_len);

        *actual_iov_len = (MPI_Aint) yaksa_actual_iov_len;
        *actual_iov_bytes = count * size;

        goto fn_exit;
    } else if (byte_offset == 0) {
        uintptr_t yaksa_actual_iov_len;
        rc = yaksa_iov(buf, count, type, 0, iov, (uintptr_t) max_iov_len, &yaksa_actual_iov_len);
        MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_INTERN, "**yaksa");

        *actual_iov_len = (MPI_Aint) yaksa_actual_iov_len;
    } else {
        /* yaksa does not accept offsets in bytes, but in number of
         * IOV elements.  So we cannot directly pass in the
         * user-provided offset to yaksa.  Instead, create a temporary
         * IOV array and pass a subset of it to the user. */
        struct iovec *tmp_iov =
            (struct iovec *) MPL_malloc(yaksa_max_iov_len * sizeof(struct iovec),
                                        MPL_MEM_DATATYPE);

        uintptr_t yaksa_actual_iov_len;
        rc = yaksa_iov(buf, count, type, 0, tmp_iov, yaksa_max_iov_len, &yaksa_actual_iov_len);
        MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_INTERN, "**yaksa");

        uintptr_t skip_bytes = (uintptr_t) byte_offset;

        for (uintptr_t i = 0; i < yaksa_actual_iov_len; i++) {
            if (skip_bytes > tmp_iov[i].iov_len) {
                skip_bytes -= tmp_iov[i].iov_len;
            } else if (skip_bytes == tmp_iov[i].iov_len) {
                tmp_iov[i].iov_len = skip_bytes = 0;

                rc = yaksa_iov(buf, count, type, i + 1, iov, (uintptr_t) max_iov_len,
                               &yaksa_actual_iov_len);
                MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_INTERN, "**yaksa");

                break;
            } else {
                tmp_iov[i].iov_base = (void *) ((char *) tmp_iov[i].iov_base + skip_bytes);
                tmp_iov[i].iov_len -= skip_bytes;
                skip_bytes = 0;

                iov[0] = tmp_iov[i];

                rc = yaksa_iov(buf, count, type, i + 1, iov + 1, (uintptr_t) (max_iov_len - 1),
                               &yaksa_actual_iov_len);
                MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_INTERN, "**yaksa");
                yaksa_actual_iov_len++;

                break;
            }
        }
        *actual_iov_len = (MPI_Aint) yaksa_actual_iov_len;

        MPL_free(tmp_iov);
    }

    uintptr_t total_bytes = 0;
    for (int i = 0; i < *actual_iov_len; i++) {
        if (total_bytes + iov[i].iov_len < max_iov_bytes) {
            total_bytes += iov[i].iov_len;
        } else if (total_bytes + iov[i].iov_len == max_iov_bytes) {
            total_bytes += iov[i].iov_len;
            *actual_iov_len = i + 1;
            break;
        } else if (total_bytes + iov[i].iov_len > max_iov_bytes) {
            iov[i].iov_len = max_iov_bytes - total_bytes;
            total_bytes = max_iov_bytes;
            *actual_iov_len = i + 1;
            break;
        }
    }
    *actual_iov_bytes = (MPI_Aint) total_bytes;

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Typerep_to_iov_offset(const void *buf, MPI_Aint count, MPI_Datatype datatype,
                               MPI_Aint iov_offset, struct iovec *iov, MPI_Aint max_iov_len,
                               MPI_Aint * actual_iov_len)
{
    MPIR_FUNC_ENTER;

    int mpi_errno = MPI_SUCCESS;
    int rc;

    yaksa_type_t type = MPII_Typerep_get_yaksa_type(datatype);
    uintptr_t yaksa_actual_iov_len;
    rc = yaksa_iov(buf, count, type, iov_offset, iov, max_iov_len, &yaksa_actual_iov_len);
    MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_INTERN, "**yaksa");

    *actual_iov_len = (MPI_Aint) yaksa_actual_iov_len;

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Typerep_iov_len(MPI_Aint count, MPI_Datatype datatype, MPI_Aint max_iov_bytes,
                         MPI_Aint * iov_len, MPI_Aint * actual_iov_bytes)
{
    MPIR_FUNC_ENTER;

    int mpi_errno = MPI_SUCCESS;
    int rc;

    yaksa_type_t type = MPII_Typerep_get_yaksa_type(datatype);

    uintptr_t max_iov_len;
    rc = yaksa_iov_len(count, type, &max_iov_len);
    MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_INTERN, "**yaksa");

    uintptr_t size;
    rc = yaksa_type_get_size(type, &size);
    MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_INTERN, "**yaksa");

    if (max_iov_bytes == -1 || max_iov_bytes >= count * size) { /* fast path */
        *iov_len = (MPI_Aint) max_iov_len;
        if (actual_iov_bytes) {
            *actual_iov_bytes = count * size;
        }
    } else {    /* slow path */
        struct iovec *iov =
            (struct iovec *) MPL_malloc(max_iov_len * sizeof(struct iovec), MPL_MEM_DATATYPE);

        uintptr_t actual_iov_len;
        rc = yaksa_iov(NULL, count, type, 0, iov, max_iov_len, &actual_iov_len);
        MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_INTERN, "**yaksa");

        *iov_len = 0;
        *actual_iov_bytes = 0;

        *iov_len = 0;
        *actual_iov_bytes = 0;
        for (MPI_Aint i = 0; i < max_iov_len; i++) {
            if (*actual_iov_bytes + iov[i].iov_len > max_iov_bytes) {
                break;
            }
            *iov_len = i + 1;
            *actual_iov_bytes += iov[i].iov_len;
        }

        MPL_free(iov);
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
