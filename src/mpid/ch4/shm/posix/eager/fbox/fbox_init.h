/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2017 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#ifndef POSIX_EAGER_FBOX_INIT_H_INCLUDED
#define POSIX_EAGER_FBOX_INIT_H_INCLUDED

#include "fbox_types.h"
#include "fbox_impl.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_CH4_POSIX_EAGER_FBOX_POLL_CACHE_SIZE
      category    : CH4
      type        : int
      default     : 3
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        The size of the array to store expected receives to speed up fastbox polling.

    - name        : MPIR_CVAR_CH4_FBOX_MEM_BIND_TYPE
      category    : CH4
      type        : enum
      default     : default
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : |-
        Select target memory type for binding
        default   - Default memory (DDR in most systems)
        ddr       - Double data rate memory
        hbm       - High-bandwidth memory

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

extern MPIDI_POSIX_eager_fbox_control_t MPIDI_POSIX_eager_fbox_control_global;

static void get_mem_type(MPIR_hwtopo_type_e * mem_type);
static void get_proc_gids(MPIR_hwtopo_gid_t * proc_gids, bool * bind_is_valid);
static int get_unique_gids(MPIR_hwtopo_gid_t * gids, int *num_unique_gids,
                           MPIR_hwtopo_gid_t ** unique_gids);
static void get_seg_len(MPIR_hwtopo_gid_t seg_gid, MPIR_hwtopo_gid_t * proc_gids, size_t * len);
static void map_procs_to_seg(MPIR_hwtopo_gid_t seg_gid, MPIR_hwtopo_gid_t * proc_gids,
                             MPIDI_POSIX_fastbox_t * seg_ptr, MPIDI_POSIX_fastbox_t ** ptr_table);

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_eager_init(int rank, int size)
{
    int mpi_errno = MPI_SUCCESS;
    int i;
    int num_seg;
    bool bind_is_valid;
    size_t len;
    MPIR_hwtopo_gid_t *proc_gids;
    MPIR_hwtopo_type_e mem_type;
    MPIR_hwtopo_gid_t *seg_gids = NULL;
    MPIDI_POSIX_fastbox_t **ptr_table = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_EAGER_INIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_EAGER_INIT);

#ifdef MPL_USE_DBG_LOGGING
    MPIDI_CH4_SHM_POSIX_FBOX_GENERAL = MPL_dbg_class_alloc("SHM_POSIX_FBOX", "shm_posix_fbox");
#endif /* MPL_USE_DBG_LOGGING */

    MPIR_CHKPMEM_DECL(4);
    MPIR_CHKLMEM_DECL(3);

    MPIR_CHKPMEM_MALLOC(MPIDI_POSIX_eager_fbox_control_global.first_poll_local_ranks,
                        int16_t *,
                        sizeof(*MPIDI_POSIX_eager_fbox_control_global.first_poll_local_ranks) *
                        (MPIR_CVAR_CH4_POSIX_EAGER_FBOX_POLL_CACHE_SIZE + 1), mpi_errno,
                        "first_poll_local_ranks", MPL_MEM_SHM);

    /* -1 means we aren't looking for anything in particular. */
    for (i = 0; i < MPIR_CVAR_CH4_POSIX_EAGER_FBOX_POLL_CACHE_SIZE; i++) {
        MPIDI_POSIX_eager_fbox_control_global.first_poll_local_ranks[i] = -1;
    }

    /* The final entry in the cache array is for tracking the fallback place to start looking for
     * messages if all other entries in the cache are empty. */
    MPIDI_POSIX_eager_fbox_control_global.first_poll_local_ranks[i] = 0;

    /* Pointer table of processes region into segment(s) */
    MPIR_CHKLMEM_MALLOC(ptr_table, MPIDI_POSIX_fastbox_t **,
                        sizeof(*ptr_table) * MPIDI_POSIX_global.num_local, mpi_errno, "ptr_table",
                        MPL_MEM_SHM);

    MPIR_CHKLMEM_MALLOC(proc_gids, MPIR_hwtopo_gid_t *,
                        sizeof(MPIR_hwtopo_gid_t) * MPIDI_POSIX_global.num_local, mpi_errno,
                        "proc_gids", MPL_MEM_OTHER);

    get_mem_type(&mem_type);
    proc_gids[MPIDI_POSIX_global.my_local_rank] = MPIR_hwtopo_get_obj_by_type(mem_type);

    /* get memory type global ids for all processes in the node */
    get_proc_gids(proc_gids, &bind_is_valid);

    /* get unique proc_gids and assign separate shared memory
     * segments to them */
    mpi_errno = get_unique_gids(proc_gids, &num_seg, &seg_gids);
    MPIR_ERR_CHECK(mpi_errno);

    MPIDI_POSIX_eager_fbox_control_global.num_seg = num_seg;

    MPIR_CHKPMEM_MALLOC(MPIDI_POSIX_eager_fbox_control_global.shm_ptr, void **,
                        sizeof(void *) * num_seg, mpi_errno, "shm_ptr", MPL_MEM_SHM);

    /* We need to split the shared memory segment by the number of
     * memory domains (num_seg) detected. The reason is that in Linux
     * shared memory policies, unlike VMA policies, apply to the
     * shared object instead of the VMA range. All processes that map
     * shared memory into their address space inherit the policy from
     * the shared object. Moreover, all the pages allocated to the
     * shared object obey the shared policy. [Reference Linux Kernel
     * documentation: `Documents/admin-guide/mm/numa_memory_policy.rst`] */
    for (i = 0; i < num_seg; i++) {
        get_seg_len(seg_gids[i], proc_gids, &len);

        /* Allocate shared memory */
        mpi_errno = MPIDU_Init_shm_alloc(len, &MPIDI_POSIX_eager_fbox_control_global.shm_ptr[i]);
        MPIR_ERR_CHECK(mpi_errno);

        map_procs_to_seg(seg_gids[i], proc_gids,
                         MPIDI_POSIX_eager_fbox_control_global.shm_ptr[i], ptr_table);

        /* Bind shm to memory device */
        if (MPIDI_POSIX_global.my_local_rank == 0 && bind_is_valid)
            MPIR_hwtopo_mem_bind(MPIDI_POSIX_eager_fbox_control_global.shm_ptr[i],
                                 len, seg_gids[i]);
    }

    /* Allocate table of pointers to fastboxes */
    MPIR_CHKPMEM_MALLOC(MPIDI_POSIX_eager_fbox_control_global.mailboxes.in,
                        MPIDI_POSIX_fastbox_t **,
                        MPIDI_POSIX_global.num_local * sizeof(MPIDI_POSIX_fastbox_t *), mpi_errno,
                        "fastboxes", MPL_MEM_SHM);
    MPIR_CHKPMEM_MALLOC(MPIDI_POSIX_eager_fbox_control_global.mailboxes.out,
                        MPIDI_POSIX_fastbox_t **,
                        MPIDI_POSIX_global.num_local * sizeof(MPIDI_POSIX_fastbox_t *), mpi_errno,
                        "fastboxes", MPL_MEM_SHM);

    /* Fill in fbox tables */
    for (i = 0; i < MPIDI_POSIX_global.num_local; i++) {
        MPIDI_POSIX_eager_fbox_control_global.mailboxes.in[i] =
            ptr_table[MPIDI_POSIX_global.my_local_rank] + i;
        MPIDI_POSIX_eager_fbox_control_global.mailboxes.out[i] =
            ptr_table[i] + MPIDI_POSIX_global.my_local_rank;

        memset(MPIDI_POSIX_eager_fbox_control_global.mailboxes.in[i], 0,
               sizeof(MPIDI_POSIX_fastbox_t));
    }

    mpi_errno = MPIDU_Init_shm_barrier();
    MPIR_ERR_CHECK(mpi_errno);

    MPIR_CHKPMEM_COMMIT();

  fn_exit:
    MPL_free(seg_gids);
    MPIR_CHKLMEM_FREEALL();
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_EAGER_INIT);
    return mpi_errno;
  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    MPIR_CHKPMEM_REAP();
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_eager_finalize()
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_EAGER_FINALIZE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_EAGER_FINALIZE);

    MPL_free(MPIDI_POSIX_eager_fbox_control_global.mailboxes.in);
    MPL_free(MPIDI_POSIX_eager_fbox_control_global.mailboxes.out);
    MPL_free(MPIDI_POSIX_eager_fbox_control_global.first_poll_local_ranks);

    for (int i = 0; i < MPIDI_POSIX_eager_fbox_control_global.num_seg; i++)
        mpi_errno = MPIDU_Init_shm_free(MPIDI_POSIX_eager_fbox_control_global.shm_ptr[i]);

    MPL_free(MPIDI_POSIX_eager_fbox_control_global.shm_ptr);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_EAGER_FINALIZE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static void get_mem_type(MPIR_hwtopo_type_e * mem_type)
{
    switch (MPIR_CVAR_CH4_FBOX_MEM_BIND_TYPE) {
        case MPIR_CVAR_CH4_FBOX_MEM_BIND_TYPE_ddr:
            *mem_type = MPIR_HWTOPO_TYPE__DDR;
            break;
        case MPIR_CVAR_CH4_FBOX_MEM_BIND_TYPE_hbm:
            *mem_type = MPIR_HWTOPO_TYPE__HBM;
            break;
        default:       /* MPIR_CVAR_CH4_FBOX_MEM_BIND_TYPE_default */
            *mem_type = MPIR_HWTOPO_TYPE__DDR;
    }
}

static void get_proc_gids(MPIR_hwtopo_gid_t * proc_gids, bool * bind_is_valid)
{
    *bind_is_valid = true;
    MPIDU_Init_shm_put(&proc_gids[MPIDI_POSIX_global.my_local_rank], sizeof(MPIR_hwtopo_gid_t));
    MPIDU_Init_shm_barrier();
    for (int i = 0; i < MPIDI_POSIX_global.num_local; i++) {
        MPIDU_Init_shm_get(i, sizeof(MPIR_hwtopo_gid_t), &proc_gids[i]);
        if (proc_gids[i] == MPIR_HWTOPO_GID_ROOT && *bind_is_valid) {
            *bind_is_valid = false;
        }
    }
    MPIDU_Init_shm_barrier();
}

static int compare_gid(const void *a, const void *b)
{
    return *(int *) a - *(int *) b;
}

static int get_unique_gids(MPIR_hwtopo_gid_t * gids, int *num_unique_gids,
                           MPIR_hwtopo_gid_t ** unique_gids)
{
    int mpi_errno = MPI_SUCCESS;
    int count = 0;
    int i, j;
    MPIR_CHKLMEM_DECL(1);
    MPIR_CHKPMEM_DECL(1);

    /* Sort node global ids */
    MPIR_hwtopo_gid_t *sorted_gids;
    MPIR_CHKLMEM_MALLOC(sorted_gids, MPIR_hwtopo_gid_t *,
                        sizeof(MPIR_hwtopo_gid_t) * MPIDI_POSIX_global.num_local, mpi_errno,
                        "sorted_gids", MPL_MEM_OTHER);
    memcpy(sorted_gids, gids, sizeof(MPIR_hwtopo_gid_t) * MPIDI_POSIX_global.num_local);

    qsort(sorted_gids, MPIDI_POSIX_global.num_local, sizeof(MPIR_hwtopo_gid_t), compare_gid);

    /* Count global ids */
    MPIR_hwtopo_gid_t curr_gid = -1;
    for (i = 0; i < MPIDI_POSIX_global.num_local; i++) {
        if (curr_gid != sorted_gids[i]) {
            count++;
            curr_gid = sorted_gids[i];
        }
    }

    /* Remove duplicates from sorted_gids */
    MPIR_CHKPMEM_MALLOC(*unique_gids, MPIR_hwtopo_gid_t *,
                        sizeof(MPIR_hwtopo_gid_t) * count, mpi_errno, "unique_gids", MPL_MEM_OTHER);
    curr_gid = -1;
    for (i = 0, j = 0; i < MPIDI_POSIX_global.num_local; i++) {
        if (curr_gid != sorted_gids[i]) {
            curr_gid = sorted_gids[i];
            (*unique_gids)[j++] = curr_gid;
        }
    }

    MPIR_CHKPMEM_COMMIT();

  fn_exit:
    *num_unique_gids = count;
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno;
  fn_fail:
    MPIR_CHKPMEM_REAP();
    *unique_gids = NULL;
    goto fn_exit;
}

static void get_seg_len(MPIR_hwtopo_gid_t seg_gid, MPIR_hwtopo_gid_t * proc_gids, size_t * len)
{
    *len = 0;
    for (int i = 0; i < MPIDI_POSIX_global.num_local; i++)
        if (seg_gid == proc_gids[i])
            *len += (MPIDI_POSIX_global.num_local * sizeof(MPIDI_POSIX_fastbox_t));
}

static void map_procs_to_seg(MPIR_hwtopo_gid_t seg_gid, MPIR_hwtopo_gid_t * proc_gids,
                             MPIDI_POSIX_fastbox_t * seg_ptr, MPIDI_POSIX_fastbox_t ** ptr_table)
{
    for (int i = 0, j = 0; i < MPIDI_POSIX_global.num_local; i++)
        if (seg_gid == proc_gids[i])
            ptr_table[i] = seg_ptr + MPIDI_POSIX_global.num_local * j++;
}


#endif /* POSIX_EAGER_FBOX_INIT_H_INCLUDED */
