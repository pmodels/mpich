/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "dataloop_internal.h"

/* Recursively find number of contig segments. Assuming -
 *     non-contig, dlp not NULL
 *     *rem_iov_bytes < type size
 *     iov_len contains previous count, not resetting
 */
int MPIR_Dataloop_iov_len(void *dataloop, MPI_Aint * rem_iov_bytes, MPI_Aint * iov_len)
{
    int mpi_errno = MPI_SUCCESS;
    MPII_Dataloop *dlp = dataloop;

    int dlp_kind = dlp->kind & MPII_DATALOOP_KIND_MASK;

    int child_is_contig;
    MPI_Aint child_size, child_num_contig;

    MPIR_Assert(dlp_kind != MPII_DATALOOP_KIND_STRUCT); /* should be all converted away */

    MPII_Dataloop *child_dlp = dlp->loop_params.cm_t.dataloop;
    if (!child_dlp) {
        child_is_contig = 1;
        child_num_contig = 1;
    } else {
        child_is_contig = child_dlp->is_contig;
        child_num_contig = child_dlp->num_contig;
    }
    child_size = dlp->el_size;

    if (child_is_contig) {
        switch (dlp_kind) {
            case MPII_DATALOOP_KIND_VECTOR:
            case MPII_DATALOOP_KIND_BLOCKINDEXED:
                {
                    MPI_Aint sub_size;
                    if (dlp_kind == MPII_DATALOOP_KIND_VECTOR) {
                        sub_size = dlp->loop_params.v_t.blocksize * child_size;
                    } else {
                        sub_size = dlp->loop_params.bi_t.blocksize * child_size;
                    }
                    MPI_Aint n = (*rem_iov_bytes) / sub_size;
                    *rem_iov_bytes -= n * sub_size;
                    *iov_len += n;
                    goto fn_exit;
                }
                break;
            case MPII_DATALOOP_KIND_INDEXED:
                for (MPI_Aint i = 0; i < dlp->loop_params.i_t.count; i++) {
                    MPI_Aint sub_size = dlp->loop_params.i_t.blocksize_array[i] * child_size;
                    if (*rem_iov_bytes < sub_size) {
                        goto fn_exit;
                    }
                    *rem_iov_bytes -= sub_size;
                    *iov_len += 1;
                }
                break;
            default:
                MPIR_Assert(0);
        }
    } else {
        /* child is non-contig */
        MPI_Aint n = (*rem_iov_bytes) / child_size;
        *rem_iov_bytes -= n * child_size;
        *iov_len += n * child_num_contig;
        if (child_num_contig > 1) {
            mpi_errno = MPIR_Dataloop_iov_len(child_dlp, rem_iov_bytes, iov_len);
            MPIR_ERR_CHECK(mpi_errno);
        }
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static void fill_iov_vector(void *buf, MPI_Aint cnt, MPI_Aint blklen, MPI_Aint stride,
                            MPII_Dataloop * child_dlp,
                            MPI_Aint child_extent, MPI_Aint child_size,
                            MPI_Aint skip_iov_len, struct iovec *iov, MPI_Aint max_iov_len,
                            MPI_Aint * actual_iov_len);
static void fill_iov_blockindexed(void *buf, MPI_Aint cnt, MPI_Aint blklen, MPI_Aint * offset_array,
                                  MPII_Dataloop * child_dlp,
                                  MPI_Aint child_extent, MPI_Aint child_size,
                                  MPI_Aint skip_iov_len, struct iovec *iov, MPI_Aint max_iov_len,
                                  MPI_Aint * actual_iov_len);
static void fill_iov_indexed(void *buf, MPI_Aint cnt, MPI_Aint * blklen_array,
                             MPI_Aint * offset_array,
                             MPII_Dataloop * child_dlp,
                             MPI_Aint child_extent, MPI_Aint child_size,
                             MPI_Aint skip_iov_len, struct iovec *iov, MPI_Aint max_iov_len,
                             MPI_Aint * actual_iov_len);

int MPIR_Dataloop_iov(const void *buf, MPI_Aint count, void *dataloop, MPI_Aint dt_extent,
                      MPI_Aint skip_iov_len, struct iovec *iov, MPI_Aint max_iov_len,
                      MPI_Aint * actual_iov_len)
{
    int mpi_errno = MPI_SUCCESS;
    MPII_Dataloop *dlp = dataloop;

    MPI_Aint idx = 0;
    char *addr = (char *) buf;
    MPI_Aint rem_skip = skip_iov_len;
    MPI_Aint rem_count = count;

    if (rem_skip >= dlp->num_contig) {
        MPI_Aint n = rem_skip / dlp->num_contig;
        if (n >= rem_count) {
            n = rem_count;
        }

        rem_skip -= n * dlp->num_contig;
        addr += n * dt_extent;
        rem_count -= n;
    }

    /* Try fill the iov, exit when we fill max_iov_len */
    while (rem_count > 0 && idx < max_iov_len) {
        /* we can extend for more count if we got the iovs of one count */
        MPI_Aint got_1st_iov_idx = -1;
        if (rem_skip == 0) {
            got_1st_iov_idx = idx;
        }

        /* recursively fill one count */
        if (dlp->is_contig) {
            MPI_Aint cnt = dlp->loop_params.cm_t.count;

            /* should have been all optimized into a contig loop */
            MPIR_Assert((dlp->kind & MPII_DATALOOP_KIND_MASK) == MPII_DATALOOP_KIND_CONTIG);
            /* skipped above already */
            MPIR_Assert(rem_skip == 0);

            MPI_Aint sub_size = cnt * dlp->el_size;

            iov[idx].iov_base = addr;
            iov[idx].iov_len = sub_size;

            addr += iov[idx].iov_len;
            idx++;
        } else {
            MPI_Aint cnt = dlp->loop_params.cm_t.count;
            MPII_Dataloop *child_dlp = dlp->loop_params.cm_t.dataloop;

            MPI_Aint tmp_len = 0;
            switch (dlp->kind & MPII_DATALOOP_KIND_MASK) {
                case MPII_DATALOOP_KIND_CONTIG:
                    /* contig is just a special vector */
                    fill_iov_vector(addr, 1, dlp->loop_params.c_t.count,
                                    dlp->el_extent, child_dlp, dlp->el_extent, dlp->el_size,
                                    rem_skip, iov + idx, max_iov_len - idx, &tmp_len);
                    addr += dlp->el_extent * cnt;
                    break;
                case MPII_DATALOOP_KIND_VECTOR:
                    fill_iov_vector(addr, cnt, dlp->loop_params.v_t.blocksize,
                                    dlp->loop_params.v_t.stride,
                                    child_dlp, dlp->el_extent, dlp->el_size,
                                    rem_skip, iov + idx, max_iov_len - idx, &tmp_len);
                    addr = (void *) ((intptr_t) addr +
                                     dlp->loop_params.v_t.stride * (cnt - 1) +
                                     dlp->loop_params.v_t.blocksize * dlp->el_extent);
                    break;
                case MPII_DATALOOP_KIND_BLOCKINDEXED:
                    fill_iov_blockindexed(addr, cnt, dlp->loop_params.bi_t.blocksize,
                                          dlp->loop_params.bi_t.offset_array,
                                          child_dlp, dlp->el_extent, dlp->el_size,
                                          rem_skip, iov + idx, max_iov_len - idx, &tmp_len);
                    addr = (void *) ((intptr_t) addr +
                                     dlp->loop_params.bi_t.offset_array[cnt - 1] +
                                     dlp->loop_params.bi_t.blocksize * dlp->el_extent);
                    break;
                case MPII_DATALOOP_KIND_INDEXED:
                    fill_iov_indexed(addr, cnt, dlp->loop_params.i_t.blocksize_array,
                                     dlp->loop_params.i_t.offset_array,
                                     child_dlp, dlp->el_extent, dlp->el_size,
                                     rem_skip, iov + idx, max_iov_len - idx, &tmp_len);
                    addr = (void *) ((intptr_t) addr +
                                     dlp->loop_params.i_t.offset_array[cnt - 1] +
                                     dlp->loop_params.i_t.blocksize_array[cnt - 1] *
                                     dlp->el_extent);
                    break;
                default:
                    MPIR_Assert(0);
            }
            idx += tmp_len;
            rem_skip = 0;
        }
        rem_count--;

        /* If we filled one entire dataloop of iovs, extend the rest */
        if (got_1st_iov_idx != -1 && rem_count > 0 && idx < max_iov_len) {
            MPIR_Assert(idx - got_1st_iov_idx == dlp->num_contig);
            struct iovec *iov_from = iov + got_1st_iov_idx;
            MPI_Aint shift = dt_extent;
            while (rem_count > 0) {
                for (MPI_Aint i = 0; i < dlp->num_contig; i++) {
                    iov[idx].iov_base = (void *) ((intptr_t) iov_from[i].iov_base + shift);
                    iov[idx].iov_len = iov_from[i].iov_len;
                    idx++;
                    if (idx >= max_iov_len) {
                        goto fn_exit;
                    }
                }
                rem_count--;
                shift += dt_extent;
            }
        }
    }

  fn_exit:
    *actual_iov_len = idx;
    return mpi_errno;
}

/* routine to fill iovs from one count of datatype */

static void fill_iov_vector(void *buf, MPI_Aint cnt, MPI_Aint blklen, MPI_Aint stride,
                            MPII_Dataloop * child_dlp,
                            MPI_Aint child_extent, MPI_Aint child_size,
                            MPI_Aint skip_iov_len, struct iovec *iov, MPI_Aint max_iov_len,
                            MPI_Aint * actual_iov_len)
{
    char *addr = buf;
    MPI_Aint idx = 0;
    MPI_Aint rem_skip = skip_iov_len;

    int child_is_contig;
    MPI_Aint child_num_contig;
    if (!child_dlp) {
        child_is_contig = 1;
        child_num_contig = 1;
    } else {
        child_is_contig = child_dlp->is_contig;
        child_num_contig = child_dlp->num_contig;
    }

    MPI_Aint i_start;
    if (child_is_contig) {
        i_start = rem_skip;
        rem_skip = 0;
    } else {
        i_start = rem_skip / (blklen * child_num_contig);
        rem_skip -= i_start * (blklen * child_num_contig);
    }

    for (MPI_Aint i = i_start; i < cnt; i++) {
        if (child_is_contig) {
            /* rem_skip is 0 */
            iov[idx].iov_base = (void *) ((intptr_t) addr + i * stride);
            iov[idx].iov_len = blklen * child_size;
            idx++;
        } else {
            MPI_Aint tmp_len;
            void *ptr = (void *) ((intptr_t) addr + i * stride);
            MPIR_Dataloop_iov(ptr, blklen, child_dlp, child_extent,
                              rem_skip, iov + idx, max_iov_len - idx, &tmp_len);
            idx += tmp_len;
        }
        if (idx >= max_iov_len) {
            goto fn_exit;
        }
    }
  fn_exit:
    *actual_iov_len = idx;
}

static void fill_iov_blockindexed(void *buf, MPI_Aint cnt, MPI_Aint blklen, MPI_Aint * offset_array,
                                  MPII_Dataloop * child_dlp,
                                  MPI_Aint child_extent, MPI_Aint child_size,
                                  MPI_Aint skip_iov_len, struct iovec *iov, MPI_Aint max_iov_len,
                                  MPI_Aint * actual_iov_len)
{
    char *addr = buf;
    MPI_Aint idx = 0;
    MPI_Aint rem_skip = skip_iov_len;

    int child_is_contig;
    MPI_Aint child_num_contig;
    if (!child_dlp) {
        child_is_contig = 1;
        child_num_contig = 1;
    } else {
        child_is_contig = child_dlp->is_contig;
        child_num_contig = child_dlp->num_contig;
    }

    MPI_Aint i_start;
    if (child_is_contig) {
        i_start = rem_skip;
        rem_skip = 0;
    } else {
        i_start = rem_skip / (blklen * child_num_contig);
        rem_skip -= i_start * (blklen * child_num_contig);
    }

    for (MPI_Aint i = i_start; i < cnt; i++) {
        if (child_is_contig) {
            /* rem_skip is 0 */
            iov[idx].iov_base = (void *) ((intptr_t) addr + offset_array[i]);
            iov[idx].iov_len = blklen * child_size;
            idx++;
        } else {
            MPI_Aint tmp_len;
            MPIR_Dataloop_iov(addr + offset_array[i], blklen, child_dlp, child_extent,
                              rem_skip, iov + idx, max_iov_len - idx, &tmp_len);
            idx += tmp_len;
        }
        if (idx >= max_iov_len) {
            goto fn_exit;
        }
    }
  fn_exit:
    *actual_iov_len = idx;
}

static void fill_iov_indexed(void *buf, MPI_Aint cnt, MPI_Aint * blklen_array,
                             MPI_Aint * offset_array,
                             MPII_Dataloop * child_dlp,
                             MPI_Aint child_extent, MPI_Aint child_size,
                             MPI_Aint skip_iov_len, struct iovec *iov, MPI_Aint max_iov_len,
                             MPI_Aint * actual_iov_len)
{
    char *addr = buf;
    MPI_Aint idx = 0;
    MPI_Aint rem_skip = skip_iov_len;

    int child_is_contig;
    MPI_Aint child_num_contig;
    if (!child_dlp) {
        child_is_contig = 1;
        child_num_contig = 1;
    } else {
        child_is_contig = child_dlp->is_contig;
        child_num_contig = child_dlp->num_contig;
    }

    for (MPI_Aint i = 0; i < cnt; i++) {
        if (child_is_contig) {
            if (rem_skip >= 1) {
                rem_skip--;
                continue;
            }
        } else {
            if (rem_skip >= blklen_array[i] * child_num_contig) {
                rem_skip -= blklen_array[i] * child_num_contig;
                continue;
            }
        }
        if (child_is_contig) {
            /* rem_skip is 0 */
            iov[idx].iov_base = (void *) ((intptr_t) addr + offset_array[i]);
            iov[idx].iov_len = blklen_array[i] * child_size;
            idx++;
        } else {
            MPI_Aint tmp_len;
            MPIR_Dataloop_iov(addr + offset_array[i], blklen_array[i], child_dlp, child_extent,
                              rem_skip, iov + idx, max_iov_len - idx, &tmp_len);
            idx += tmp_len;
        }
        if (idx >= max_iov_len) {
            goto fn_exit;
        }
    }
  fn_exit:
    *actual_iov_len = idx;
}
