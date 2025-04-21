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

    - name        : MPIR_CVAR_CH4_POSIX_COLL_SELECTION_TUNING_JSON_FILE_GPU
      category    : COLLECTIVE
      type        : string
      default     : ""
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Defines the location of tuning file for GPU.

    - name        : MPIR_CVAR_CH4_SHM_POSIX_TOPO_ENABLE
      category    : CH4
      type        : boolean
      default     : false
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Controls topology-aware communication in POSIX.

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

#include "mpir_hwtopo.h"

extern MPL_atomic_uint64_t *MPIDI_POSIX_shm_limit_counter;
#ifndef MPL_HAVE_INITSHM
static bool init_shm_initialized = false;
#endif

static int choose_posix_eager(void);
static void *host_alloc(uintptr_t size);
static void host_free(void *ptr);
static void init_topo_info(void);
static int posix_coll_init(void);
static int posix_coll_finalize(void);

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
        else if (!strcmp(ckey, "algorithm=MPIDI_POSIX_mpi_bcast_ipc_read"))
            cnt->id = MPIDI_POSIX_CSEL_CONTAINER_TYPE__ALGORITHM__MPIDI_POSIX_mpi_bcast_ipc_read;
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

int MPIDI_POSIX_init_vci(int vci)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIDU_genq_private_pool_create(MPIDI_POSIX_AM_HDR_POOL_CELL_SIZE,
                                               MPIDI_POSIX_AM_HDR_POOL_NUM_CELLS_PER_CHUNK,
                                               0 /* unlimited */ ,
                                               host_alloc, host_free,
                                               &MPIDI_POSIX_global.per_vci[vci].am_hdr_buf_pool);
    MPIR_ERR_CHECK(mpi_errno);

    MPIDI_POSIX_global.per_vci[vci].postponed_queue = NULL;

    MPIDI_POSIX_global.per_vci[vci].active_rreq =
        MPL_calloc(MPIR_Process.local_size, sizeof(MPIR_Request *), MPL_MEM_SHM);
    MPIR_ERR_CHKANDJUMP(!MPIDI_POSIX_global.per_vci[vci].active_rreq,
                        mpi_errno, MPI_ERR_OTHER, "**nomem");

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static void init_topo_info(void)
{
    MPIDI_POSIX_topo_info_t *topo = &MPIDI_POSIX_global.topo;
    MPIR_hwtopo_gid_t core_id = MPIR_hwtopo_get_obj_by_type(MPIR_HWTOPO_TYPE__CORE);
    MPIR_hwtopo_gid_t l3_cache_id = MPIR_hwtopo_get_obj_by_type(MPIR_HWTOPO_TYPE__L3CACHE);
    MPIR_hwtopo_gid_t numa_id = MPIR_hwtopo_get_obj_by_type(MPIR_HWTOPO_TYPE__DDR);

    topo->core_id = MPIR_hwtopo_get_lid(core_id);
    topo->l3_cache_id = MPIR_hwtopo_get_lid(l3_cache_id);
    topo->numa_id = MPIR_hwtopo_get_lid(numa_id);
}

int MPIDI_POSIX_init_local(int *tag_bits /* unused */)
{
    int mpi_errno = MPI_SUCCESS;
    int i;

    MPL_COMPILE_TIME_ASSERT(sizeof(MPIDI_POSIX_am_request_header_t)
                            < MPIDI_POSIX_AM_HDR_POOL_CELL_SIZE);

    MPIDI_POSIX_global.num_vcis = 1;
    MPIDI_POSIX_global.shm_slab = NULL;
    MPIDI_POSIX_global.shm_vci_slab = NULL;

    /* hwloc getting topo info */
    MPIDI_POSIX_global.topo.core_id = -1;
    MPIDI_POSIX_global.topo.l3_cache_id = -1;
    MPIDI_POSIX_global.topo.numa_id = -1;
    MPIDI_POSIX_global.local_rank_dist = (int *) MPL_malloc(MPIR_Process.local_size * sizeof(int),
                                                            MPL_MEM_SHM);
    for (i = 0; i < MPIR_Process.local_size; i++) {
        MPIDI_POSIX_global.local_rank_dist[i] = MPIDI_POSIX_DIST__LOCAL;
    }
    if (MPIR_CVAR_CH4_SHM_POSIX_TOPO_ENABLE) {
        init_topo_info();
    }

    choose_posix_eager();

    mpi_errno = posix_coll_init();
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

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

static int calc_head_shm_size(int local_size)
{
    int head_shm_size = sizeof(MPIDI_POSIX_shm_t) + local_size * sizeof(int);
    if (head_shm_size % MPL_CACHELINE_SIZE) {
        head_shm_size += MPL_CACHELINE_SIZE - head_shm_size % MPL_CACHELINE_SIZE;
    }
    return head_shm_size;
}

int MPIDI_POSIX_comm_bootstrap(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Comm *node_comm = MPIR_Comm_get_node_comm(comm);
    int local_size = node_comm->local_size;
    int local_rank = node_comm->rank;

    if (local_size == 1) {
        goto fn_exit;
    }

    MPIDI_POSIX_shm_t *slab = MPIDI_POSIX_global.shm_slab;
    if (MPIDI_POSIX_global.shm_slab == NULL) {
        mpi_errno = MPIDI_POSIX_init_vci(0);
        MPIR_ERR_CHECK(mpi_errno);

        int eager_shm_size = MPIDI_POSIX_eager_shm_size(MPIR_Process.local_size);
        int head_shm_size = calc_head_shm_size(MPIR_Process.local_size);
        int slab_size = head_shm_size + eager_shm_size;

#ifdef MPL_HAVE_INITSHM
        /* MPICH supports local cliques, so we need node_id in shm name to avoid collision */
        unsigned world_id = MPIR_Process.world_id;
        int node_id = MPIR_Process.node_map[MPIR_Process.rank];
        sprintf(MPIDI_POSIX_global.shm_name, "mpich_shm_%x_%d", world_id, node_id);
        sprintf(MPIDI_POSIX_global.shm_vci_name, "mpich_vci_%x_%d", world_id, node_id);

        bool is_root;
        slab = MPL_initshm_open(MPIDI_POSIX_global.shm_name, slab_size, &is_root);
        MPIR_ERR_CHKANDJUMP(!slab, mpi_errno, MPI_ERR_OTHER, "**nomem");

        if (is_root) {
            memset(slab, 0, sizeof(MPIDI_POSIX_shm_t));
            MPL_atomic_relaxed_store_uint64(&slab->shm_limit_counter, 0);
            MPL_atomic_store_int(&slab->root_ready, MPIDI_POSIX_READY_FLAG);
        } else {
            while (MPL_atomic_load_int(&slab->root_ready) != MPIDI_POSIX_READY_FLAG) {
                MPID_Thread_yield();
            }
        }
#else
        /* Init_shm requires collective over node */
        MPIR_Assert(node_comm->local_size == MPIR_Process.local_size);

        if (!init_shm_initialized) {
            mpi_errno = MPIDU_Init_shm_init();
            MPIR_ERR_CHECK(mpi_errno);
        }

        int slab_size = MPIDI_POSIX_eager_shm_size(size);
        mpi_errno = MPIDU_Init_shm_alloc(slab_size, (void *) &slab);
        MPIR_ERR_CHECK(mpi_errno);
#endif
        MPIDI_POSIX_global.shm_slab = slab;

        mpi_errno = MPIDI_POSIX_eager_init((char *) slab + head_shm_size,
                                           MPIR_Process.local_rank, MPIR_Process.local_size);
        MPIR_ERR_CHECK(mpi_errno);

        MPL_atomic_store_int(&slab->eager_ready[MPIR_Process.local_rank], MPIDI_POSIX_READY_FLAG);

        MPIDI_POSIX_shm_limit_counter = &slab->shm_limit_counter;

        utarray_new(shm_mutex_free_list, &shm_mutex_icd, MPL_MEM_OTHER);
    }

    /* wait for every other process in the node_comm */
    for (int i = 0; i < local_size; i++) {
        if (i == local_rank) {
            continue;
        }
        int r = MPIDI_SHM_global.local_ranks[MPIDIU_get_grank(i, node_comm)];
        while (MPL_atomic_load_int(&slab->eager_ready[r]) != MPIDI_POSIX_READY_FLAG) {
            MPID_Thread_yield();
        }
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_POSIX_mpi_finalize_hook(void)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    if (MPIDI_POSIX_global.shm_slab) {
        mpi_errno = MPIDI_POSIX_eager_finalize();
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

#ifdef MPL_HAVE_INITSHM
        MPL_initshm_freeall();
#else
        mpi_errno = MPIDU_Init_shm_free(MPIDI_POSIX_global.shm_slab);
        MPIR_ERR_CHECK(mpi_errno);
        MPIDI_POSIX_global.shm_slab = NULL;

        if (MPIDI_POSIX_global.shm_vci_slab) {
            mpi_errno = MPIDU_Init_shm_free(MPIDI_POSIX_global.shm_vci_slab);
            MPIR_ERR_CHECK(mpi_errno);
            MPIDI_POSIX_global.shm_vci_slab = NULL;
        }

        if (init_shm_initialized) {
            mpi_errno = MPIDU_Init_shm_finalize();
            MPIR_ERR_CHECK(mpi_errno);
        }
#endif

        for (int vci = 0; vci < MPIDI_POSIX_global.num_vcis; vci++) {
            MPIDU_genq_private_pool_destroy(MPIDI_POSIX_global.per_vci[vci].am_hdr_buf_pool);
            MPL_free(MPIDI_POSIX_global.per_vci[vci].active_rreq);
        }
    }

    mpi_errno = posix_coll_finalize();
    MPIR_ERR_CHECK(mpi_errno);

    MPL_free(MPIDI_POSIX_global.local_rank_dist);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int posix_coll_init(void)
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

    /* Initialize collective selection for gpu */
    if (!strcmp(MPIR_CVAR_CH4_POSIX_COLL_SELECTION_TUNING_JSON_FILE_GPU, "")) {
        mpi_errno = MPIR_Csel_create_from_buf(MPIDI_POSIX_coll_generic_json,
                                              create_container,
                                              &MPIDI_global.shm.posix.csel_root_gpu);
    } else {
        mpi_errno =
            MPIR_Csel_create_from_file(MPIR_CVAR_CH4_POSIX_COLL_SELECTION_TUNING_JSON_FILE_GPU,
                                       create_container, &MPIDI_global.shm.posix.csel_root_gpu);
    }
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int posix_coll_finalize(void)
{
    int mpi_errno = MPI_SUCCESS;
    static MPL_atomic_uint64_t MPIDI_POSIX_dummy_shm_limit_counter;

    MPIR_FUNC_ENTER;

    /* MPIDI_POSIX_global.shm_ptr is freed but will be referenced during builtin
     * comm free; here we set MPIDI_POSIX_shm_limit_counter as dummy counter to
     * avoid segmentation fault */
    MPIDI_POSIX_shm_limit_counter = &MPIDI_POSIX_dummy_shm_limit_counter;

    if (MPIDI_global.shm.posix.csel_root) {
        mpi_errno = MPIR_Csel_free(MPIDI_global.shm.posix.csel_root);
        MPIR_ERR_CHECK(mpi_errno);
    }

    if (MPIDI_global.shm.posix.csel_root_gpu) {
        mpi_errno = MPIR_Csel_free(MPIDI_global.shm.posix.csel_root_gpu);
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
