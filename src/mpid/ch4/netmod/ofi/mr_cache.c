/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include <mpidimpl.h>
#include "ofi_impl.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===
cvars:
    - name        : MPIR_CVAR_CH4_OFI_MR_CACHE_SIZE
      category    : CH4_OFI
      type        : int
      default     : 16
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        The maximum number of entries to hold in the HMEM MR cache. When an entry
        is evicted, the corresponding MR is unregistered.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

/* Use a simple array to manage HMEM mr cache. This is similar to the one used in IPC handle cache.
 *   1. observes MPIR_CVAR_CH4_OFI_MR_CACHE_SIZE to avoid hoarding memory (an issue for Intel ZE).
 *   2. checks for stale entries by checking for overlapping address ranges.
 */

#define MR_CACHE_MAX 1024
/* assume stale entries (user frees the buffer but the mr still in cache) are harmless.
 * otherwise, define MR_CACHE_CHECK_STALE_ENTRY.
 */
/* #define MR_CACHE_CHECK_STALE_ENTRY */

struct mr_cache_entry {
    const void *base_addr;
    MPI_Aint len;
    MPL_gpu_buffer_id_t buffer_id;
    int ctx_idx;

    int in_use;
    unsigned long long usage_stamp;

    struct fid_mr *mr;
};

static struct mr_cache_entry mr_cache[MR_CACHE_MAX];
static int mr_cache_count = 0;

static unsigned long long mr_cache_usage_counter;       /* for tracking LRU (Least Recently Used) */

/* ipc handle cache utilities */
static int mr_cache_free(int idx);
static int mr_cache_delete(int idx);
static struct mr_cache_entry *mr_cache_search(const void *addr, MPI_Aint len,
                                              MPL_gpu_buffer_id_t buffer_id, int ctx_idx);
static int mr_cache_check_limit(void);
static int mr_cache_insert(const void *addr, MPI_Aint len,
                           MPL_gpu_buffer_id_t buffer_id, int ctx_idx, struct fid_mr *mr);


/* free the cache entry without updating mr_cache array */
static int mr_cache_free(int idx)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Assert(mr_cache[idx].in_use == 0);
    MPIDI_OFI_CALL(fi_close(&mr_cache[idx].mr->fid), mr_unreg);

    /* The caller will take care of shifting mr_cache and updating mr_cache_count */

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* free the cache entry and shift mr_cache array */
static int mr_cache_delete(int idx)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Assert(idx < mr_cache_count);

    mpi_errno = mr_cache_free(idx);
    MPIR_ERR_CHECK(mpi_errno);

    /* shift cache array and update mr_cache_count */
    if (mr_cache_count - 1 > idx) {
        memmove(mr_cache + idx, mr_cache + idx + 1,
                (mr_cache_count - idx - 1) * sizeof(mr_cache[0]));
    }
    mr_cache_count -= 1;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static struct mr_cache_entry *mr_cache_search(const void *addr, MPI_Aint len,
                                              MPL_gpu_buffer_id_t buffer_id, int ctx_idx)
{
    struct mr_cache_entry *found = NULL;

    for (int i = 0; i < mr_cache_count; i++) {
        struct mr_cache_entry *entry = &mr_cache[i];
        if (entry->base_addr == addr && entry->ctx_idx == ctx_idx && entry->buffer_id == buffer_id) {
            found = entry;
            goto fn_exit;
        }
#ifdef MR_CACHE_CHECK_STALE_ENTRY
        /* check potential stale entry.
         * Overlap condition between [a1, b1) and [a2, b2) is a1<b2 && b1>a2 */
        if ((uintptr_t) entry->base_addr < (uintptr_t) addr + len &&
            (uintptr_t) entry->base_addr + entry->len > (uintptr_t) addr) {
            mr_cache_delete(i);
            i--;        /* the cache array has been shifted up */
            continue;
        }
#endif
    }

  fn_exit:
    if (found) {
        /* update usage_stamp for LRU */
        found->usage_stamp = ++mr_cache_usage_counter;
        found->in_use++;
    }
    return found;
}

static int mr_cache_check_limit(void)
{
    int mpi_errno = MPI_SUCCESS;

    int cache_limit = MPL_MIN(MPIR_CVAR_CH4_OFI_MR_CACHE_SIZE, MR_CACHE_MAX);
    if (mr_cache_count >= cache_limit) {
        /* find LRU slot */
        int min_idx = -1;
        unsigned long long min_stamp = 0;
        for (int i = 0; i < mr_cache_count; i++) {
            if (mr_cache[i].in_use) {
                continue;
            }
            if (min_idx == -1 || mr_cache[i].usage_stamp < min_stamp) {
                min_stamp = mr_cache[i].usage_stamp;
                min_idx = i;
            }
        }

        if (min_idx != -1) {
            mpi_errno = mr_cache_delete(min_idx);
        }
    }

    return mpi_errno;
}

static int mr_cache_insert(const void *addr, MPI_Aint len, MPL_gpu_buffer_id_t buffer_id,
                           int ctx_idx, struct fid_mr *mr)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = mr_cache_check_limit();
    MPIR_ERR_CHECK(mpi_errno);

    int cache_limit = MPL_MIN(MPIR_CVAR_CH4_OFI_MR_CACHE_SIZE, MR_CACHE_MAX);
    if (mr_cache_count < cache_limit) {
        struct mr_cache_entry *entry = &mr_cache[mr_cache_count];
        memset(entry, 0, sizeof(*entry));
        entry->base_addr = addr;
        entry->len = len;
        entry->buffer_id = buffer_id;
        entry->ctx_idx = ctx_idx;
        entry->mr = mr;
        entry->usage_stamp = ++mr_cache_usage_counter;
        mr_cache_count++;

        entry->in_use = 1;
    }
    /* NOTE: if we didn't cache due to cache limit, mr will be unregistered in MPIDI_OFI_mr_complete */

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* ---------------------------------------- */

int MPIDI_OFI_register_memory_and_bind(char *buf, size_t data_sz,
                                       MPL_pointer_attr_t * attr, int ctx_idx, struct fid_mr **mr)
{
    int mpi_errno = MPI_SUCCESS;

    void *bounds_base;
    uintptr_t bounds_len;
    MPL_gpu_buffer_id_t buffer_id;
    if (MPL_gpu_attr_is_dev(attr)) {
        int mpl_err = MPL_gpu_get_buffer_bounds(buf, &bounds_base, &bounds_len);
        MPIR_ERR_CHKANDJUMP(mpl_err != MPL_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                            "**gpu_get_buffer_info");
        buffer_id = MPL_gpu_get_buffer_id(bounds_base);
    } else {
        bounds_base = buf;
        bounds_len = data_sz;
        buffer_id = 0;
    }

    struct mr_cache_entry *entry = mr_cache_search(bounds_base, bounds_len, buffer_id, ctx_idx);
    if (entry) {
        *mr = entry->mr;
    } else {
        mpi_errno = MPIDI_OFI_register_memory(bounds_base, bounds_len, attr, ctx_idx, 0, mr);
        MPIR_ERR_CHECK(mpi_errno);

        if (*mr) {
            mpi_errno = MPIDI_OFI_mr_bind(MPIDI_OFI_global.prov_use[0], *mr,
                                          MPIDI_OFI_global.ctx[ctx_idx].ep, NULL);
            MPIR_ERR_CHECK(mpi_errno);

            mpi_errno = mr_cache_insert(bounds_base, bounds_len, buffer_id, ctx_idx, *mr);
            MPIR_ERR_CHECK(mpi_errno);
        }
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


int MPIDI_OFI_mr_complete(struct fid_mr *mr, bool keep_cache)
{
    int mpi_errno = MPI_SUCCESS;

    bool found_in_cache = false;
    for (int i = 0; i < mr_cache_count; i++) {
        if (mr_cache[i].mr == mr) {
            mr_cache[i].in_use--;
            if (mr_cache[i].in_use <= 0) {
                if (keep_cache) {
                    mpi_errno = mr_cache_check_limit();
                } else {
                    mpi_errno = mr_cache_delete(i);
                }
                MPIR_ERR_CHECK(mpi_errno);
            }

            found_in_cache = true;
            break;
        }
    }

    if (!found_in_cache) {
        /* unregister directly */
        MPIDI_OFI_CALL(fi_close(&mr->fid), mr_unreg);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_OFI_mr_cache_finalize(void)
{
    int mpi_errno = MPI_SUCCESS;

    for (int i = 0; i < mr_cache_count; i++) {
        mpi_errno = mr_cache_free(i);
        MPIR_Assert(mpi_errno == MPI_SUCCESS);
    }
    mr_cache_count = 0;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
