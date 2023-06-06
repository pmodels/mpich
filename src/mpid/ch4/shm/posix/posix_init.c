/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_CH4_SHM_POSIX_EAGER
      category    : CH4
      type        : string
      default     : ""
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        If non-empty, this cvar specifies which shm posix eager module to use

    - name        : MPIR_CVAR_CH4_POSIX_COLL_SELECTION_TUNING_JSON_FILE
      category    : COLLECTIVE
      type        : string
      default     : ""
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Defines the location of tuning file.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

#include "mpidimpl.h"
#include "posix_types.h"
#include "ch4_types.h"

#include "posix_eager.h"
#include "posix_noinline.h"
#include "posix_csel_container.h"
#include "mpidu_genq.h"

#include "utarray.h"
#include <strings.h>    /* for strncasecmp */

extern MPL_atomic_uint64_t *MPIDI_POSIX_shm_limit_counter;

static int choose_posix_eager(void);
static void *host_alloc(uintptr_t size);
static void host_free(void *ptr);

static void *host_alloc(uintptr_t size)
{
    return MPL_malloc(size, MPL_MEM_BUFFER);
}

static void host_free(void *ptr)
{
    MPL_free(ptr);
}


static int choose_posix_eager(void)
{
    int mpi_errno = MPI_SUCCESS;
    int i;

    MPIR_FUNC_ENTER;

    MPIR_Assert(MPIR_CVAR_CH4_SHM_POSIX_EAGER != NULL);

    if (strcmp(MPIR_CVAR_CH4_SHM_POSIX_EAGER, "") == 0) {
        /* posix_eager not specified, using the default */
        MPIDI_POSIX_eager_func = MPIDI_POSIX_eager_funcs[0];
        goto fn_exit;
    }

    for (i = 0; i < MPIDI_num_posix_eager_fabrics; ++i) {
        /* use MPL variant of strncasecmp if we get one */
        if (!strncasecmp
            (MPIR_CVAR_CH4_SHM_POSIX_EAGER, MPIDI_POSIX_eager_strings[i],
             MPIDI_MAX_POSIX_EAGER_STRING_LEN)) {
            MPIDI_POSIX_eager_func = MPIDI_POSIX_eager_funcs[i];
            goto fn_exit;
        }
    }

    MPIR_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER, "**ch4|invalid_shm_posix_eager",
                         "**ch4|invalid_shm_posix_eager %s", MPIR_CVAR_CH4_SHM_POSIX_EAGER);
  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static void *create_container(struct json_object *obj)
{
    MPIDI_POSIX_csel_container_s *cnt =
        MPL_malloc(sizeof(MPIDI_POSIX_csel_container_s), MPL_MEM_COLL);

    json_object_object_foreach(obj, key, val) {
        char *ckey = MPL_strdup_no_spaces(key);

        if (!strcmp(ckey, "algorithm=MPIDI_POSIX_mpi_bcast_release_gather"))
            cnt->id =
                MPIDI_POSIX_CSEL_CONTAINER_TYPE__ALGORITHM__MPIDI_POSIX_mpi_bcast_release_gather;
        else if (!strcmp(ckey, "algorithm=MPIDI_POSIX_mpi_barrier_release_gather"))
            cnt->id =
                MPIDI_POSIX_CSEL_CONTAINER_TYPE__ALGORITHM__MPIDI_POSIX_mpi_barrier_release_gather;
        else if (!strcmp(ckey, "algorithm=MPIDI_POSIX_mpi_allreduce_release_gather"))
            cnt->id =
                MPIDI_POSIX_CSEL_CONTAINER_TYPE__ALGORITHM__MPIDI_POSIX_mpi_allreduce_release_gather;
        else if (!strcmp(ckey, "algorithm=MPIDI_POSIX_mpi_reduce_release_gather"))
            cnt->id =
                MPIDI_POSIX_CSEL_CONTAINER_TYPE__ALGORITHM__MPIDI_POSIX_mpi_reduce_release_gather;
        else {
            fprintf(stderr, "unrecognized key %s\n", key);
            MPIR_Assert(0);
        }

        MPL_free(ckey);
    }

    return cnt;
}

int MPIDI_POSIX_init_local(int *tag_bits /* unused */)
{
    int mpi_errno = MPI_SUCCESS;
    int i, local_rank_0 = -1;
    MPIR_CHKPMEM_DECL(MPIDI_CH4_MAX_VCIS);

    MPL_COMPILE_TIME_ASSERT(sizeof(MPIDI_POSIX_am_request_header_t)
                            < MPIDI_POSIX_AM_HDR_POOL_CELL_SIZE);

    /* Populate these values with transformation information about each rank and its original
     * information in MPI_COMM_WORLD. */

    MPIDI_POSIX_global.local_ranks = (int *) MPL_malloc(MPIR_Process.size * sizeof(int),
                                                        MPL_MEM_SHM);
    for (i = 0; i < MPIR_Process.size; ++i) {
        MPIDI_POSIX_global.local_ranks[i] = -1;
    }
    for (i = 0; i < MPIR_Process.local_size; i++) {
        MPIDI_POSIX_global.local_ranks[MPIR_Process.node_local_map[i]] = i;
    }
    local_rank_0 = MPIR_Process.node_local_map[0];
    MPIDI_POSIX_global.num_local = MPIR_Process.local_size;
    MPIDI_POSIX_global.my_local_rank = MPIR_Process.local_rank;

    MPIDI_POSIX_global.local_rank_0 = local_rank_0;

    MPIDI_POSIX_global.num_vsis = MPIDI_global.n_total_vcis;
    /* This is used to track messages that the eager submodule was not ready to send. */
    for (int vsi = 0; vsi < MPIDI_global.n_total_vcis; vsi++) {
        mpi_errno = MPIDU_genq_private_pool_create(MPIDI_POSIX_AM_HDR_POOL_CELL_SIZE,
                                                   MPIDI_POSIX_AM_HDR_POOL_NUM_CELLS_PER_CHUNK,
                                                   0 /* unlimited */ ,
                                                   host_alloc, host_free,
                                                   &MPIDI_POSIX_global.
                                                   per_vsi[vsi].am_hdr_buf_pool);
        MPIR_ERR_CHECK(mpi_errno);

        MPIDI_POSIX_global.per_vsi[vsi].postponed_queue = NULL;

        MPIR_CHKPMEM_MALLOC(MPIDI_POSIX_global.per_vsi[vsi].active_rreq, MPIR_Request **,
                            MPIR_Process.local_size * sizeof(MPIR_Request *), mpi_errno,
                            "active recv req", MPL_MEM_SHM);

        for (i = 0; i < MPIR_Process.local_size; i++) {
            MPIDI_POSIX_global.per_vsi[vsi].active_rreq[i] = NULL;
        }

    }

    choose_posix_eager();

    MPIR_CHKPMEM_COMMIT();

  fn_exit:
    return mpi_errno;
  fn_fail:
    MPIR_CHKPMEM_REAP();
    goto fn_exit;
}

static int posix_world_initialized;

/* FreeBSD work around: only destroy inter-process mutex at finalize */
struct shm_mutex_entry {
    int rank;
    MPL_proc_mutex_t *shm_mutex_ptr;
};

static UT_icd shm_mutex_icd = { sizeof(struct shm_mutex_entry), NULL, NULL, NULL };

static UT_array *shm_mutex_free_list;

void MPIDI_POSIX_delay_shm_mutex_destroy(int rank, MPL_proc_mutex_t * shm_mutex_ptr)
{
    struct shm_mutex_entry entry;
    entry.rank = rank;
    entry.shm_mutex_ptr = shm_mutex_ptr;
    utarray_push_back(shm_mutex_free_list, &entry, MPL_MEM_OTHER);
}

int MPIDI_POSIX_init_world(void)
{
    int mpi_errno = MPI_SUCCESS;

    int rank = MPIR_Process.rank;
    int size = MPIR_Process.size;

    mpi_errno = MPIDI_POSIX_eager_init(rank, size);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIDI_POSIX_coll_init(rank, size);
    MPIR_ERR_CHECK(mpi_errno);

    utarray_new(shm_mutex_free_list, &shm_mutex_icd, MPL_MEM_OTHER);

    posix_world_initialized = 1;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_POSIX_mpi_finalize_hook(void)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    if (posix_world_initialized) {
        mpi_errno = MPIDI_POSIX_eager_finalize();
        MPIR_ERR_CHECK(mpi_errno);

        mpi_errno = MPIDI_POSIX_coll_finalize();
        MPIR_ERR_CHECK(mpi_errno);

        struct shm_mutex_entry *p;
        for (p = (struct shm_mutex_entry *) utarray_front(shm_mutex_free_list); p != NULL;
             p = (struct shm_mutex_entry *) utarray_next(shm_mutex_free_list, p)) {
            if (p->rank == 0) {
                MPIDI_POSIX_RMA_MUTEX_DESTROY(p->shm_mutex_ptr);
            }
            mpi_errno = MPIDU_shm_free(p->shm_mutex_ptr);
            MPIR_ERR_CHECK(mpi_errno);
        }
        utarray_free(shm_mutex_free_list);
    }

    for (int vsi = 0; vsi < MPIDI_global.n_total_vcis; vsi++) {
        MPIDU_genq_private_pool_destroy(MPIDI_POSIX_global.per_vsi[vsi].am_hdr_buf_pool);
        MPL_free(MPIDI_POSIX_global.per_vsi[vsi].active_rreq);
    }

    MPL_free(MPIDI_POSIX_global.local_ranks);

    posix_world_initialized = 0;

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_POSIX_coll_init(int rank, int size)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    /* Initialize collective selection */
    if (!strcmp(MPIR_CVAR_CH4_POSIX_COLL_SELECTION_TUNING_JSON_FILE, "")) {
        mpi_errno = MPIR_Csel_create_from_buf(MPIDI_POSIX_coll_generic_json,
                                              create_container, &MPIDI_global.shm.posix.csel_root);
    } else {
        mpi_errno = MPIR_Csel_create_from_file(MPIR_CVAR_CH4_POSIX_COLL_SELECTION_TUNING_JSON_FILE,
                                               create_container, &MPIDI_global.shm.posix.csel_root);
    }
    MPIR_ERR_CHECK(mpi_errno);

    /* Actually allocate the segment and assign regions to the pointers */
    mpi_errno = MPIDU_Init_shm_alloc(sizeof(int), &MPIDI_POSIX_global.shm_ptr);
    MPIR_ERR_CHECK(mpi_errno);

    MPIDI_POSIX_shm_limit_counter = (MPL_atomic_uint64_t *) MPIDI_POSIX_global.shm_ptr;

    mpi_errno = MPIDU_Init_shm_barrier();
    MPIR_ERR_CHECK(mpi_errno);

    /* Set the counter to 0 */
    MPL_atomic_relaxed_store_uint64(MPIDI_POSIX_shm_limit_counter, 0);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_POSIX_coll_finalize(void)
{
    int mpi_errno = MPI_SUCCESS;
    static MPL_atomic_uint64_t MPIDI_POSIX_dummy_shm_limit_counter;

    MPIR_FUNC_ENTER;

    /* Destroy the shared counter which was used to track the amount of shared memory created
     * per node for intra-node collectives */
    mpi_errno = MPIDU_Init_shm_free(MPIDI_POSIX_global.shm_ptr);

    /* MPIDI_POSIX_global.shm_ptr is freed but will be referenced during builtin
     * comm free; here we set MPIDI_POSIX_shm_limit_counter as dummy counter to
     * avoid segmentation fault */
    MPIDI_POSIX_shm_limit_counter = &MPIDI_POSIX_dummy_shm_limit_counter;

    if (MPIDI_global.shm.posix.csel_root) {
        mpi_errno = MPIR_Csel_free(MPIDI_global.shm.posix.csel_root);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

void *MPIDI_POSIX_mpi_alloc_mem(MPI_Aint size, MPIR_Info * info_ptr)
{
    return MPIDIG_mpi_alloc_mem(size, info_ptr);
}

int MPIDI_POSIX_mpi_free_mem(void *ptr)
{
    return MPIDIG_mpi_free_mem(ptr);
}
