/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "mpidch4r.h"
#include "datatype.h"
#include "mpidu_init_shm.h"

#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

categories:
    - name        : CH4
      description : cvars that control behavior of the CH4 device

cvars:
    - name        : MPIR_CVAR_CH4_NETMOD
      category    : CH4
      type        : string
      default     : ""
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        If non-empty, this cvar specifies which network module to use

    - name        : MPIR_CVAR_CH4_SHM
      category    : CH4
      type        : string
      default     : ""
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        If non-empty, this cvar specifies which shm module to use

    - name        : MPIR_CVAR_CH4_ROOTS_ONLY_PMI
      category    : CH4
      type        : boolean
      default     : false
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        Enables an optimized business card exchange over PMI for node root processes only.

    - name        : MPIR_CVAR_CH4_RUNTIME_CONF_DEBUG
      category    : CH4
      type        : boolean
      default     : false
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        If enabled, CH4-level runtime configurations are printed out

    - name        : MPIR_CVAR_CH4_MT_MODEL
      category    : CH4
      type        : string
      default     : ""
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Specifies the CH4 multi-threading model. Possible values are:
        direct (default)
        handoff

    - name        : MPIR_CVAR_CH4_NUM_VCIS
      category    : CH4
      type        : int
      default     : 1
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        Sets the number of VCIs that user needs (should be a subset of MPIDI_CH4_MAX_VCIS).

    - name        : MPIR_CVAR_CH4_COLL_SELECTION_TUNING_JSON_FILE
      category    : COLLECTIVE
      type        : string
      default     : ""
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Defines the location of tuning file.

    - name        : MPIR_CVAR_CH4_IOV_DENSITY_MIN
      category    : CH4
      type        : int
      default     : 16384
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        Defines the threshold of high-density datatype. The
        density is calculated by (datatype_size / datatype_num_contig_blocks).
=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

static void *create_container(struct json_object *obj)
{
    MPIDI_Csel_container_s *cnt = MPL_malloc(sizeof(MPIDI_Csel_container_s), MPL_MEM_COLL);

    json_object_object_foreach(obj, key, val) {
        char *ckey = MPL_strdup_no_spaces(key);

        if (!strcmp(ckey, "composition=MPIDI_Barrier_intra_composition_alpha"))
            cnt->id = MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Barrier_intra_composition_alpha;
        else if (!strcmp(ckey, "composition=MPIDI_Barrier_intra_composition_beta"))
            cnt->id = MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Barrier_intra_composition_beta;
        else if (!strcmp(ckey, "composition=MPIDI_Bcast_intra_composition_alpha"))
            cnt->id = MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Bcast_intra_composition_alpha;
        else if (!strcmp(ckey, "composition=MPIDI_Bcast_intra_composition_beta"))
            cnt->id = MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Bcast_intra_composition_beta;
        else if (!strcmp(ckey, "composition=MPIDI_Bcast_intra_composition_gamma"))
            cnt->id = MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Bcast_intra_composition_gamma;
        else if (!strcmp(ckey, "composition=MPIDI_Allreduce_intra_composition_alpha"))
            cnt->id =
                MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Allreduce_intra_composition_alpha;
        else if (!strcmp(ckey, "composition=MPIDI_Allreduce_intra_composition_beta"))
            cnt->id =
                MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Allreduce_intra_composition_beta;
        else if (!strcmp(ckey, "composition=MPIDI_Allreduce_intra_composition_gamma"))
            cnt->id =
                MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Allreduce_intra_composition_gamma;
        else if (!strcmp(ckey, "composition=MPIDI_Reduce_intra_composition_alpha"))
            cnt->id = MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Reduce_intra_composition_alpha;
        else if (!strcmp(ckey, "composition=MPIDI_Reduce_intra_composition_beta"))
            cnt->id = MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Reduce_intra_composition_beta;
        else if (!strcmp(ckey, "composition=MPIDI_Reduce_intra_composition_gamma"))
            cnt->id = MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Reduce_intra_composition_gamma;
        else if (!strcmp(ckey, "composition=MPIDI_Alltoall_intra_composition_alpha"))
            cnt->id =
                MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Alltoall_intra_composition_alpha;
        else if (!strcmp(ckey, "composition=MPIDI_Alltoallv_intra_composition_alpha"))
            cnt->id =
                MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Alltoallv_intra_composition_alpha;
        else if (!strcmp(ckey, "composition=MPIDI_Alltoallw_intra_composition_alpha"))
            cnt->id =
                MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Alltoallw_intra_composition_alpha;
        else if (!strcmp(ckey, "composition=MPIDI_Allgather_intra_composition_alpha"))
            cnt->id =
                MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Allgather_intra_composition_alpha;
        else if (!strcmp(ckey, "composition=MPIDI_Allgatherv_intra_composition_alpha"))
            cnt->id =
                MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Allgatherv_intra_composition_alpha;
        else if (!strcmp(ckey, "composition=MPIDI_Gather_intra_composition_alpha"))
            cnt->id = MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Gather_intra_composition_alpha;
        else if (!strcmp(ckey, "composition=MPIDI_Gatherv_intra_composition_alpha"))
            cnt->id = MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Gatherv_intra_composition_alpha;
        else if (!strcmp(ckey, "composition=MPIDI_Scatter_intra_composition_alpha"))
            cnt->id = MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Scatter_intra_composition_alpha;
        else if (!strcmp(ckey, "composition=MPIDI_Scatterv_intra_composition_alpha"))
            cnt->id =
                MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Scatterv_intra_composition_alpha;
        else if (!strcmp(ckey, "composition=MPIDI_Reduce_scatter_intra_composition_alpha"))
            cnt->id =
                MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Reduce_scatter_intra_composition_alpha;
        else if (!strcmp(ckey, "composition=MPIDI_Reduce_scatter_block_intra_composition_alpha"))
            cnt->id =
                MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Reduce_scatter_block_intra_composition_alpha;
        else if (!strcmp(ckey, "composition=MPIDI_Scan_intra_composition_alpha"))
            cnt->id = MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Scan_intra_composition_alpha;
        else if (!strcmp(ckey, "composition=MPIDI_Scan_intra_composition_beta"))
            cnt->id = MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Scan_intra_composition_beta;
        else if (!strcmp(ckey, "composition=MPIDI_Exscan_intra_composition_alpha"))
            cnt->id = MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Exscan_intra_composition_alpha;
        else {
            fprintf(stderr, "unrecognized key %s\n", ckey);
            MPIR_Assert(0);
        }

        MPL_free(ckey);
    }

    return (void *) cnt;
}

static int choose_netmod(void);
static int create_init_comm(MPIR_Comm **);
static void destroy_init_comm(MPIR_Comm **);
static int init_builtin_comms(void);
static void finalize_builtin_comms(void);
static void init_av_table(void);
static void finalize_av_table(void);

static int choose_netmod(void)
{
    int i, mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CHOOSE_NETMOD);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CHOOSE_NETMOD);

    MPIR_Assert(MPIR_CVAR_CH4_NETMOD != NULL);

    if (strcmp(MPIR_CVAR_CH4_NETMOD, "") == 0) {
        /* netmod not specified, using the default */
        MPIDI_NM_func = MPIDI_NM_funcs[0];
        MPIDI_NM_native_func = MPIDI_NM_native_funcs[0];
        goto fn_exit;
    }

    for (i = 0; i < MPIDI_num_netmods; ++i) {
        /* use MPL variant of strncasecmp if we get one */
        if (!strncasecmp(MPIR_CVAR_CH4_NETMOD, MPIDI_NM_strings[i], MPIDI_MAX_NETMOD_STRING_LEN)) {
            MPIDI_NM_func = MPIDI_NM_funcs[i];
            MPIDI_NM_native_func = MPIDI_NM_native_funcs[i];
            goto fn_exit;
        }
    }

    MPIR_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER, "**invalid_netmod", "**invalid_netmod %s",
                         MPIR_CVAR_CH4_NETMOD);
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CHOOSE_NETMOD);
    return mpi_errno;
  fn_fail:

    goto fn_exit;
}

#ifdef MPIDI_CH4_USE_WORK_QUEUES
/* NOTE: MPIDI_CH4_USE_MT_RUNTIME currently are not supported. The key part is
 * guarded by "#if 0".
 */
/* TODO: move them to ch4i_workq_init.c. */

static const char *get_mt_model_name(int mt);
static void print_runtime_configurations(void);
static int parse_mt_model(const char *name);
static int set_runtime_configurations(void);

static const char *mt_model_names[MPIDI_CH4_NUM_MT_MODELS] = {
    "direct",
    "handoff",
};

static const char *get_mt_model_name(int mt)
{
    if (mt < 0 || mt >= MPIDI_CH4_NUM_MT_MODELS)
        return "(invalid)";

    return mt_model_names[mt];
}

static void print_runtime_configurations(void)
{
    printf("==== CH4 runtime configurations ====\n");
    printf("MPIDI_CH4_MT_MODEL: %d (%s)\n",
           MPIDI_CH4_MT_MODEL, get_mt_model_name(MPIDI_CH4_MT_MODEL));
    printf("================================\n");
}

static int parse_mt_model(const char *name)
{
    int i;

    if (!strcmp("", name))
        return 0;       /* default */

    for (i = 0; i < MPIDI_CH4_NUM_MT_MODELS; i++) {
        if (!strcasecmp(name, mt_model_names[i]))
            return i;
    }
    return -1;
}

static int set_runtime_configurations(void)
{
    int mpi_errno = MPI_SUCCESS;

#if 0   /* defined(MPIDI_CH4_USE_MT_RUNTIME) */
    int mt = parse_mt_model(MPIR_CVAR_CH4_MT_MODEL);
    if (mt < 0)
        MPIR_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER,
                             "**ch4|invalid_mt_model", "**ch4|invalid_mt_model %s",
                             MPIR_CVAR_CH4_MT_MODEL);
    MPIDI_global.settings.mt_model = mt;
#else
    /* Static configuration - no runtime selection */
    if (strcmp(MPIR_CVAR_CH4_MT_MODEL, "") != 0)
        printf("Warning: MPIR_CVAR_CH4_MT_MODEL will be ignored "
               "unless --enable-ch4-mt=runtime is given at the configure time.\n");
#endif /* MPIDI_CH4_USE_MT_RUNTIME */

#if 0   /* defined(MPIDI_CH4_USE_MT_RUNTIME) */
  fn_fail:
#endif
    return mpi_errno;
}

#endif /* MPIDI_CH4_USE_WORK_QUEUES */

static int create_init_comm(MPIR_Comm ** comm)
{
    int i, mpi_errno = MPI_SUCCESS;
    int world_rank = MPIR_Process.rank;
    int node_root_rank = MPIR_Process.node_root_map[MPIR_Process.node_map[world_rank]];

    /* if the process is not a node root, exit */
    if (node_root_rank == world_rank) {
        int node_roots_comm_size = MPIR_Process.num_nodes;
        int node_roots_comm_rank = MPIR_Process.node_map[world_rank];
        MPIR_Comm *init_comm = NULL;
        MPIDI_rank_map_lut_t *lut = NULL;
        MPIR_Comm_create(&init_comm);
        init_comm->context_id = 0 << MPIR_CONTEXT_PREFIX_SHIFT;
        init_comm->recvcontext_id = 0 << MPIR_CONTEXT_PREFIX_SHIFT;
        init_comm->comm_kind = MPIR_COMM_KIND__INTRACOMM;
        init_comm->rank = node_roots_comm_rank;
        init_comm->remote_size = node_roots_comm_size;
        init_comm->local_size = node_roots_comm_size;
        init_comm->coll.pof2 = MPL_pof2(node_roots_comm_size);
        MPIDI_COMM(init_comm, map).mode = MPIDI_RANK_MAP_LUT_INTRA;
        mpi_errno = MPIDIU_alloc_lut(&lut, node_roots_comm_size);
        MPIR_ERR_CHECK(mpi_errno);
        MPIDI_COMM(init_comm, map).size = node_roots_comm_size;
        MPIDI_COMM(init_comm, map).avtid = 0;
        MPIDI_COMM(init_comm, map).irreg.lut.t = lut;
        MPIDI_COMM(init_comm, map).irreg.lut.lpid = lut->lpid;
        MPIDI_COMM(init_comm, local_map).mode = MPIDI_RANK_MAP_NONE;
        for (i = 0; i < node_roots_comm_size; ++i) {
            lut->lpid[i] = MPIR_Process.node_root_map[i];
        }
        mpi_errno = MPIDIG_init_comm(init_comm);
        MPIR_ERR_CHECK(mpi_errno);

        *comm = init_comm;
    }
  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static void destroy_init_comm(MPIR_Comm ** comm_ptr)
{
    int in_use;
    MPIR_Comm *comm = NULL;
    if (*comm_ptr != NULL) {
        comm = *comm_ptr;
        MPIDIU_release_lut(MPIDI_COMM(comm, map).irreg.lut.t);
        MPIDIG_destroy_comm(comm);
        MPIR_Object_release_ref(comm, &in_use);
        MPIR_Assert(MPIR_Object_get_ref(comm) == 0);
        MPII_COMML_FORGET(comm);
        MPIR_Handle_obj_free(&MPIR_Comm_mem, comm);
        *comm_ptr = NULL;
    }
}

static int init_builtin_comms(void)
{
    int mpi_errno = MPI_SUCCESS;

    /* ---------------------------------- */
    /* Initialize MPI_COMM_SELF           */
    /* ---------------------------------- */
    MPIR_Process.comm_self->rank = 0;
    MPIR_Process.comm_self->remote_size = 1;
    MPIR_Process.comm_self->local_size = 1;

    /* ---------------------------------- */
    /* Initialize MPI_COMM_WORLD          */
    /* ---------------------------------- */
    MPIR_Process.comm_world->rank = MPIR_Process.rank;
    MPIR_Process.comm_world->remote_size = MPIR_Process.size;
    MPIR_Process.comm_world->local_size = MPIR_Process.size;

    /* initialize rank_map */
    MPIDI_COMM(MPIR_Process.comm_world, map).mode = MPIDI_RANK_MAP_DIRECT_INTRA;
    MPIDI_COMM(MPIR_Process.comm_world, map).avtid = 0;
    MPIDI_COMM(MPIR_Process.comm_world, map).size = MPIR_Process.size;
    MPIDI_COMM(MPIR_Process.comm_world, local_map).mode = MPIDI_RANK_MAP_NONE;
    MPIDIU_avt_add_ref(0);

    MPIDI_COMM(MPIR_Process.comm_self, map).mode = MPIDI_RANK_MAP_OFFSET_INTRA;
    MPIDI_COMM(MPIR_Process.comm_self, map).avtid = 0;
    MPIDI_COMM(MPIR_Process.comm_self, map).size = 1;
    MPIDI_COMM(MPIR_Process.comm_self, map).reg.offset = MPIR_Process.rank;
    MPIDI_COMM(MPIR_Process.comm_self, local_map).mode = MPIDI_RANK_MAP_NONE;
    MPIDIU_avt_add_ref(0);

    mpi_errno = MPIR_Comm_commit(MPIR_Process.comm_self);
    MPIR_ERR_CHECK(mpi_errno);
    mpi_errno = MPIR_Comm_commit(MPIR_Process.comm_world);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static void init_av_table(void)
{
    int i;
    int size = MPIR_Process.size;
    int rank = MPIR_Process.rank;

    MPIDIU_avt_init();

    MPIDI_av_table[0] = (MPIDI_av_table_t *)
        MPL_malloc(size * sizeof(MPIDI_av_entry_t)
                   + sizeof(MPIDI_av_table_t), MPL_MEM_ADDRESS);

    MPIDI_av_table[0]->size = size;
    MPIR_Object_set_ref(MPIDI_av_table[0], 1);

    MPIDI_global.node_map[0] = MPIR_Process.node_map;

    MPIDI_av_table0 = MPIDI_av_table[0];

#ifdef MPIDI_BUILD_CH4_LOCALITY_INFO
    MPIDI_global.max_node_id = MPIR_Process.num_nodes - 1;

    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                    (MPL_DBG_FDEST, "MPIDI_global.max_node_id = %d", MPIDI_global.max_node_id));

    for (i = 0; i < size; i++) {
        MPIDI_av_table0->table[i].is_local =
            (MPIDI_global.node_map[0][i] == MPIDI_global.node_map[0][rank]) ? 1 : 0;
        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                        (MPL_DBG_FDEST, "WORLD RANK %d %s local", i,
                         MPIDI_av_table0->table[i].is_local ? "is" : "is not"));
        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                        (MPL_DBG_FDEST, "Node id (i) (me) %d %d", MPIDI_global.node_map[0][i],
                         MPIDI_global.node_map[0][rank]));
    }
#endif
}

/* This local function is temporary until we decide where the
 * following init code belongs */
static int generic_init(void)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_global.is_ch4u_initialized = 0;
    MPIDIG_am_init();
    MPIDIU_map_create((void **) &(MPIDI_global.win_map), MPL_MEM_RMA);
    MPIDI_global.csel_root = NULL;
    MPIDI_global.is_ch4u_initialized = 1;
    return mpi_errno;
}

#if (MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY__VCI)
#define MAX_THREAD_MODE MPI_THREAD_MULTIPLE
#elif  (MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY__GLOBAL)
#define MAX_THREAD_MODE MPI_THREAD_MULTIPLE
#elif  (MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY__SINGLE)
#define MAX_THREAD_MODE MPI_THREAD_SERIALIZED
#elif  (MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY__LOCKFREE)
#define MAX_THREAD_MODE MPI_THREAD_SERIALIZED
#else
#error "Thread Granularity:  Invalid"
#endif

int MPID_Init(int requested, int *provided)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_INIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_INIT);

    mpi_errno = MPID_Init_local(requested, provided);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPID_Init_world();
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_INIT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPID_Init_local(int requested, int *provided)
{
    int mpi_errno = MPI_SUCCESS;
    char strerrbuf[MPIR_STRERROR_BUF_SIZE];
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_INIT_LOCAL);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_INIT_LOCAL);

    switch (requested) {
        case MPI_THREAD_SINGLE:
        case MPI_THREAD_SERIALIZED:
        case MPI_THREAD_FUNNELED:
            *provided = requested;
            break;
        case MPI_THREAD_MULTIPLE:
            *provided = MAX_THREAD_MODE;
            break;
    }

#ifdef MPL_USE_DBG_LOGGING
    MPIDI_CH4_DBG_GENERAL = MPL_dbg_class_alloc("CH4", "ch4");
    MPIDI_CH4_DBG_MAP = MPL_dbg_class_alloc("CH4_MAP", "ch4_map");
    MPIDI_CH4_DBG_COMM = MPL_dbg_class_alloc("CH4_COMM", "ch4_comm");
    MPIDI_CH4_DBG_MEMORY = MPL_dbg_class_alloc("CH4_MEMORY", "ch4_memory");
#endif

#ifdef HAVE_SIGNAL
    /* install signal handler for process failure notifications from hydra */
    MPIDI_global.sigusr1_count = 0;
    MPIDI_global.my_sigusr1_count = 0;
    MPIDI_global.prev_sighandler = signal(SIGUSR1, MPIDI_sigusr1_handler);
    MPIR_ERR_CHKANDJUMP1(MPIDI_global.prev_sighandler == SIG_ERR, mpi_errno, MPI_ERR_OTHER,
                         "**signal", "**signal %s",
                         MPIR_Strerror(errno, strerrbuf, MPIR_STRERROR_BUF_SIZE));
    if (MPIDI_global.prev_sighandler == SIG_IGN || MPIDI_global.prev_sighandler == SIG_DFL)
        MPIDI_global.prev_sighandler = NULL;
#endif

    choose_netmod();

    mpi_errno = MPIR_pmi_init();
    MPIR_ERR_CHECK(mpi_errno);

    /* Create all ch4-layer granular locks.
     * Note: some locks (e.g. MPIDIU_THREAD_HCOLL_MUTEX) may be unused due to configuration.
     * It is harmless to create them anyway rather than adding #ifdefs.
     */
    for (int i = 0; i < MAX_CH4_MUTEXES; i++) {
        int err;
        MPID_Thread_mutex_create(&MPIDI_global.m[i], &err);
        MPIR_Assert(err == 0);
    }

#ifdef MPIDI_CH4_USE_WORK_QUEUES
    mpi_errno = set_runtime_configurations();
    MPIR_ERR_CHECK(mpi_errno);

    MPIDI_workq_init(&MPIDI_global.workqueue);

    if (MPIR_CVAR_CH4_RUNTIME_CONF_DEBUG && rank == 0)
        print_runtime_configurations();
#endif

    init_av_table();

    mpi_errno = generic_init();
    MPIR_ERR_CHECK(mpi_errno);

    /* Initialize multiple VCIs */
    /* TODO: add checks to ensure MPIDI_vci_t is padded or aligned to MPL_CACHELINE_SIZE */
    MPIDI_global.n_vcis = 1;
    if (MPIR_CVAR_CH4_NUM_VCIS > 1) {
        MPIDI_global.n_vcis = MPIR_CVAR_CH4_NUM_VCIS;
        /* There are configured maxes that we need observe. */
        /* TODO: check them at configure time to avoid discrepancy */
        MPIR_Assert(MPIDI_global.n_vcis <= MPIDI_CH4_MAX_VCIS);
        MPIR_Assert(MPIDI_global.n_vcis <= MPIR_REQUEST_NUM_POOLS);
    }

    for (int i = 0; i < MPIDI_global.n_vcis; i++) {
        int err;
        MPID_Thread_mutex_create(&MPIDI_VCI(i).lock, &err);
        MPIR_Assert(err == 0);

        /* NOTE: 1-1 vci-pool mapping */
        MPIR_Request_register_pool_lock(i, &MPIDI_VCI(i).lock);

        /* TODO: workq */
    }
    {
        int shm_tag_bits = MPIR_TAG_BITS_DEFAULT, nm_tag_bits = MPIR_TAG_BITS_DEFAULT;
#ifndef MPIDI_CH4_DIRECT_NETMOD
        mpi_errno = MPIDI_SHM_init_local(&shm_tag_bits);

        if (mpi_errno != MPI_SUCCESS) {
            MPIR_ERR_POPFATAL(mpi_errno);
        }
#endif

        mpi_errno = MPIDI_NM_init_local(&nm_tag_bits);
        if (mpi_errno != MPI_SUCCESS) {
            MPIR_ERR_POPFATAL(mpi_errno);
        }

        /* Use the minimum tag_bits from the netmod and shmod */
        MPIR_Process.tag_bits = MPL_MIN(shm_tag_bits, nm_tag_bits);
    }
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_INIT_LOCAL);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPID_Init_world(void)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *init_comm = NULL;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_INIT_WORLD);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_INIT_WORLD);

    /* setup receive queue statistics */
    mpi_errno = MPIDIG_recvq_init();
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = create_init_comm(&init_comm);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIDU_Init_shm_init();
    MPIR_ERR_CHECK(mpi_errno);

    {
#ifndef MPIDI_CH4_DIRECT_NETMOD
        mpi_errno = MPIDI_SHM_init_world();

        if (mpi_errno != MPI_SUCCESS) {
            MPIR_ERR_POPFATAL(mpi_errno);
        }
#endif

        mpi_errno = MPIDI_NM_init_world(init_comm);
        if (mpi_errno != MPI_SUCCESS) {
            MPIR_ERR_POPFATAL(mpi_errno);
        }
    }

    MPIDIG_am_check_init();

    /* Initialize collective selection */
    if (!strcmp(MPIR_CVAR_CH4_COLL_SELECTION_TUNING_JSON_FILE, "")) {
        mpi_errno = MPIR_Csel_create_from_buf(MPIDI_coll_generic_json,
                                              create_container, &MPIDI_global.csel_root);
    } else {
        mpi_errno = MPIR_Csel_create_from_file(MPIR_CVAR_CH4_COLL_SELECTION_TUNING_JSON_FILE,
                                               create_container, &MPIDI_global.csel_root);
    }
    MPIR_ERR_CHECK(mpi_errno);

    /* Override split_type */
    MPIDI_global.MPIR_Comm_fns_store.split_type = MPIDI_Comm_split_type;
    MPIR_Comm_fns = &MPIDI_global.MPIR_Comm_fns_store;

    MPIR_Process.attrs.appnum = MPIR_Process.appnum;
    MPIR_Process.attrs.io = MPI_ANY_SOURCE;

    destroy_init_comm(&init_comm);
    mpi_errno = init_builtin_comms();
    MPIR_ERR_CHECK(mpi_errno);

    MPIDI_global.is_initialized = 0;

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_INIT_WORLD);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPID_InitCompleted(void)
{
    int mpi_errno = MPI_SUCCESS;
    char parent_port[MPI_MAX_PORT_NAME];

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_INITCOMPLETED);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_INITCOMPLETED);

    mpi_errno = MPIDI_NM_post_init();
    MPIR_ERR_CHECK(mpi_errno);

    if (MPIR_Process.has_parent) {
        mpi_errno = MPIR_pmi_kvs_get(-1, MPIDI_PARENT_PORT_KVSKEY, parent_port, MPI_MAX_PORT_NAME);
        MPIR_ERR_CHECK(mpi_errno);
        MPID_Comm_connect(parent_port, NULL, 0, MPIR_Process.comm_world, &MPIR_Process.comm_parent);
        MPIR_Assert(MPIR_Process.comm_parent != NULL);
        MPL_strncpy(MPIR_Process.comm_parent->name, "MPI_COMM_PARENT", MPI_MAX_OBJECT_NAME);
    }

    MPIDI_global.is_initialized = 1;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_INITCOMPLETED);

  fn_exit:
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

static void finalize_builtin_comms(void)
{
    /* Release builtin comms */
    MPIR_Comm_release_always(MPIR_Process.comm_world);
    MPIR_Comm_release_always(MPIR_Process.comm_self);
}

static void finalize_av_table(void)
{
    int i;
    int max_n_avts;
    max_n_avts = MPIDIU_get_max_n_avts();
    for (i = 0; i < max_n_avts; i++) {
        if (MPIDI_av_table[i] != NULL) {
            MPIDIU_avt_release_ref(i);
        }
    }

    MPIDIU_avt_destroy();
}

/* This local function is temporary until we decide where the
 * following finalize code belongs */
static void generic_finalize(void)
{
    MPIDIG_am_finalize();
    MPIDI_global.is_ch4u_initialized = 0;
}

int MPID_Finalize(void)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_FINALIZE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_FINALIZE);

    mpi_errno = MPIDI_NM_mpi_finalize_hook();
    MPIR_ERR_CHECK(mpi_errno);
#ifndef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_SHM_mpi_finalize_hook();
    MPIR_ERR_CHECK(mpi_errno);
#endif

    if (MPIDI_global.csel_root) {
        mpi_errno = MPIR_Csel_free(MPIDI_global.csel_root);
        MPIR_ERR_CHECK(mpi_errno);
    }

    finalize_builtin_comms();
    generic_finalize();

    finalize_av_table();

    mpi_errno = MPIDU_Init_shm_finalize();
    MPIR_ERR_CHECK(mpi_errno);

    MPIR_pmi_finalize();

    for (int i = 0; i < MAX_CH4_MUTEXES; i++) {
        int err;
        MPID_Thread_mutex_destroy(&MPIDI_global.m[i], &err);
        MPIR_Assert(err == 0);
    }

    for (int i = 0; i < MPIDI_global.n_vcis; i++) {
        int err;
        MPID_Thread_mutex_destroy(&MPIDI_VCI(i).lock, &err);
        MPIR_Assert(err == 0);
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_FINALIZE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPID_Get_universe_size(int *universe_size)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_GET_UNIVERSE_SIZE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_GET_UNIVERSE_SIZE);

    mpi_errno = MPIR_pmi_get_universe_size(universe_size);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_GET_UNIVERSE_SIZE);
    return mpi_errno;
}

int MPID_Get_processor_name(char *name, int namelen, int *resultlen)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_GET_PROCESSOR_NAME);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_GET_PROCESSOR_NAME);

    if (!MPIDI_global.pname_set) {
#ifdef HAVE_GETHOSTNAME

        if (gethostname(MPIDI_global.pname, MPI_MAX_PROCESSOR_NAME) == 0)
            MPIDI_global.pname_len = (int) strlen(MPIDI_global.pname);

#elif defined(HAVE_SYSINFO)

        if (sysinfo(SI_HOSTNAME, MPIDI_global.pname, MPI_MAX_PROCESSOR_NAME) == 0)
            MPIDI_global.pname_len = (int) strlen(MPIDI_global.pname);

#else
        MPL_snprintf(MPIDI_global.pname, MPI_MAX_PROCESSOR_NAME, "%d",
                     MPIR_Process.comm_world->rank);
        MPIDI_global.pname_len = (int) strlen(MPIDI_global.pname);
#endif
        MPIDI_global.pname_set = 1;
    }

    MPIR_ERR_CHKANDJUMP(MPIDI_global.pname_len <= 0, mpi_errno, MPI_ERR_OTHER, "**procnamefailed");
    MPL_strncpy(name, MPIDI_global.pname, namelen);

    if (resultlen)
        *resultlen = MPIDI_global.pname_len;

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_GET_PROCESSOR_NAME);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

typedef enum {
    ALLOC_MEM_BUF_TYPE__UNSET,
    ALLOC_MEM_BUF_TYPE__DDR,
    ALLOC_MEM_BUF_TYPE__HBM,
    ALLOC_MEM_BUF_TYPE__NETMOD,
    ALLOC_MEM_BUF_TYPE__SHMMOD,
} alloc_mem_buf_type_e;

typedef struct {
    void *user_buf;

    void *real_buf;
    alloc_mem_buf_type_e buf_type;
    size_t size;

    UT_hash_handle hh;
} alloc_mem_container_s;

static alloc_mem_container_s *alloc_mem_container_list = NULL;

void *MPID_Alloc_mem(MPI_Aint size, MPIR_Info * info_ptr)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_ALLOC_MEM);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_ALLOC_MEM);

    char val[MPI_MAX_INFO_VAL + 1];
    MPIR_hwtopo_gid_t mem_gid = MPIR_HWTOPO_GID_ROOT;
    alloc_mem_buf_type_e buf_type = ALLOC_MEM_BUF_TYPE__UNSET;
    void *user_buf = NULL;
    void *real_buf;
    int alignment = MAX_ALIGNMENT;

    MPIR_Info *curr_info;
    LL_FOREACH(info_ptr, curr_info) {
        if (curr_info->key == NULL)
            continue;

        int flag = 0;
        MPIR_Info_get_impl(info_ptr, "mpich_buf_type", MPI_MAX_INFO_VAL, val, &flag);

        if (flag) {
            if (!strcmp(val, "ddr")) {
                mem_gid = MPIR_hwtopo_get_obj_by_type(MPIR_HWTOPO_TYPE__DDR);
                if (mem_gid == MPIR_HWTOPO_GID_ROOT) {
                    buf_type = ALLOC_MEM_BUF_TYPE__UNSET;
                } else {
                    buf_type = ALLOC_MEM_BUF_TYPE__DDR;
                }
            } else if (!strcmp(val, "hbm")) {
                mem_gid = MPIR_hwtopo_get_obj_by_type(MPIR_HWTOPO_TYPE__HBM);
                if (mem_gid == MPIR_HWTOPO_GID_ROOT) {
                    /* if mem_gid = MPIR_HWTOPO_GID_ROOT and mem_type
                     * is non-default (DDR) it can mean either that
                     * the requested memory type is not available in
                     * the system or the requested memory type is
                     * available but there are many devices of such
                     * type and the process requesting memory is not
                     * bound to any of them. Regardless the reason we
                     * do not fall back to the default allocation and
                     * return a NULL pointer to the upper layer
                     * instead. */
                    goto fn_exit;
                } else {
                    buf_type = ALLOC_MEM_BUF_TYPE__HBM;
                }
            } else if (!strcmp(val, "network")) {
                buf_type = ALLOC_MEM_BUF_TYPE__NETMOD;
            } else if (!strcmp(val, "shmem")) {
                buf_type = ALLOC_MEM_BUF_TYPE__SHMMOD;
            } else {
                assert(0);
            }
        }

        MPIR_Info_get_impl(info_ptr, "mpi_minimum_memory_alignment", MPI_MAX_INFO_VAL, val, &flag);
        if (flag) {
            alignment = atoi(val);
        }
    }

    switch (buf_type) {
        case ALLOC_MEM_BUF_TYPE__HBM:
        case ALLOC_MEM_BUF_TYPE__DDR:
            /* requested memory type is available in the system and
             * process is bound to the corresponding device; allocate
             * memory and bind it to device. */
            assert(mem_gid != MPIR_HWTOPO_GID_ROOT);
            real_buf =
                MPL_mmap(NULL, size + alignment, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1,
                         0, MPL_MEM_USER);
            MPIR_hwtopo_mem_bind(real_buf, size + alignment, mem_gid);
            break;

        case ALLOC_MEM_BUF_TYPE__NETMOD:
            real_buf = MPIDI_NM_mpi_alloc_mem(size + alignment, info_ptr);
            break;

        case ALLOC_MEM_BUF_TYPE__SHMMOD:
#ifndef MPIDI_CH4_DIRECT_NETMOD
            real_buf = MPIDI_SHM_mpi_alloc_mem(size + alignment, info_ptr);
#else
            goto fn_exit;
#endif
            break;

        default:
            real_buf = MPIDIG_mpi_alloc_mem(size + alignment, info_ptr);
            break;
    }

    alloc_mem_container_s *container;
    container = (alloc_mem_container_s *) MPL_malloc(sizeof(alloc_mem_container_s), MPL_MEM_USER);

    int diff = alignment - (int) ((uintptr_t) real_buf % alignment);
    user_buf = (void *) ((uintptr_t) real_buf + diff);

    container->real_buf = real_buf;
    container->user_buf = user_buf;
    container->buf_type = buf_type;
    container->size = size + alignment;

    MPID_THREAD_CS_ENTER(VCI, MPIDIU_THREAD_ALLOC_MEM_MUTEX);
    HASH_ADD_PTR(alloc_mem_container_list, user_buf, container, MPL_MEM_USER);
    MPID_THREAD_CS_EXIT(VCI, MPIDIU_THREAD_ALLOC_MEM_MUTEX);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_ALLOC_MEM);
    return user_buf;
}

int MPID_Free_mem(void *user_buf)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_FREE_MEM);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_FREE_MEM);

    alloc_mem_container_s *container;

    MPID_THREAD_CS_ENTER(VCI, MPIDIU_THREAD_ALLOC_MEM_MUTEX);
    HASH_FIND_PTR(alloc_mem_container_list, &user_buf, container);
    assert(container);
    HASH_DEL(alloc_mem_container_list, container);
    MPID_THREAD_CS_EXIT(VCI, MPIDIU_THREAD_ALLOC_MEM_MUTEX);

    switch (container->buf_type) {
        case ALLOC_MEM_BUF_TYPE__HBM:
        case ALLOC_MEM_BUF_TYPE__DDR:
            MPL_munmap(container->real_buf, container->size, MPL_MEM_USER);
            break;

        case ALLOC_MEM_BUF_TYPE__NETMOD:
            mpi_errno = MPIDI_NM_mpi_free_mem(container->real_buf);
            break;

        case ALLOC_MEM_BUF_TYPE__SHMMOD:
#ifndef MPIDI_CH4_DIRECT_NETMOD
            mpi_errno = MPIDI_SHM_mpi_free_mem(container->real_buf);
#else
            assert(0);
#endif
            break;

        default:
            MPIDIG_mpi_free_mem(container->real_buf);
            break;
    }
    MPIR_ERR_CHECK(mpi_errno);
    MPL_free(container);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_FREE_MEM);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPID_Comm_get_lpid(MPIR_Comm * comm_ptr, int idx, int *lpid_ptr, bool is_remote)
{
    int mpi_errno = MPI_SUCCESS;
    int avtid = 0, lpid = 0;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_COMM_GET_LPID);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_COMM_GET_LPID);

    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM)
        MPIDIU_comm_rank_to_pid(comm_ptr, idx, &lpid, &avtid);
    else if (is_remote)
        MPIDIU_comm_rank_to_pid(comm_ptr, idx, &lpid, &avtid);
    else {
        MPIDIU_comm_rank_to_pid_local(comm_ptr, idx, &lpid, &avtid);
    }

    *lpid_ptr = MPIDIU_LUPID_CREATE(avtid, lpid);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_COMM_GET_LPID);
    return mpi_errno;
}

int MPID_Get_node_id(MPIR_Comm * comm, int rank, int *id_p)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_GET_NODE_ID);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_GET_NODE_ID);

    MPIDIU_get_node_id(comm, rank, id_p);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_GET_NODE_ID);
    return mpi_errno;
}

int MPID_Get_max_node_id(MPIR_Comm * comm, int *max_id_p)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_GET_MAX_NODE_ID);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_GET_MAX_NODE_ID);

    MPIDIU_get_max_node_id(comm, max_id_p);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_GET_MAX_NODE_ID);
    return mpi_errno;
}

int MPID_Create_intercomm_from_lpids(MPIR_Comm * newcomm_ptr, int size, const int lpids[])
{
    int mpi_errno = MPI_SUCCESS, i;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_CREATE_INTERCOMM_FROM_LPIDS);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_CREATE_INTERCOMM_FROM_LPIDS);

    MPIDI_rank_map_mlut_t *mlut = NULL;
    MPIDI_COMM(newcomm_ptr, map).mode = MPIDI_RANK_MAP_MLUT;
    MPIDI_COMM(newcomm_ptr, map).avtid = -1;
    mpi_errno = MPIDIU_alloc_mlut(&mlut, size);
    MPIR_ERR_CHECK(mpi_errno);
    MPIDI_COMM(newcomm_ptr, map).size = size;
    MPIDI_COMM(newcomm_ptr, map).irreg.mlut.t = mlut;
    MPIDI_COMM(newcomm_ptr, map).irreg.mlut.gpid = mlut->gpid;

    for (i = 0; i < size; i++) {
        MPIDI_COMM(newcomm_ptr, map).irreg.mlut.gpid[i].avtid = MPIDIU_LUPID_GET_AVTID(lpids[i]);
        MPIDI_COMM(newcomm_ptr, map).irreg.mlut.gpid[i].lpid = MPIDIU_LUPID_GET_LPID(lpids[i]);
        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                        (MPL_DBG_FDEST, " remote rank=%d, avtid=%d, lpid=%d", i,
                         MPIDI_COMM(newcomm_ptr, map).irreg.mlut.gpid[i].avtid,
                         MPIDI_COMM(newcomm_ptr, map).irreg.mlut.gpid[i].lpid));
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_CREATE_INTERCOMM_FROM_LPIDS);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPI_Aint MPID_Aint_add(MPI_Aint base, MPI_Aint disp)
{
    MPI_Aint result;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_AINT_ADD);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_AINT_ADD);
    result = (MPI_Aint) ((char *) base + disp);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_AINT_ADD);
    return result;
}

MPI_Aint MPID_Aint_diff(MPI_Aint addr1, MPI_Aint addr2)
{
    MPI_Aint result;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_AINT_DIFF);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_AINT_DIFF);

    result = (MPI_Aint) ((char *) addr1 - (char *) addr2);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_AINT_DIFF);
    return result;
}

int MPID_Type_commit_hook(MPIR_Datatype * type)
{
    int mpi_errno;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_TYPE_COMMIT_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_TYPE_COMMIT_HOOK);

    mpi_errno = MPIDI_NM_mpi_type_commit_hook(type);
    MPIR_ERR_CHECK(mpi_errno);
#ifndef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_SHM_mpi_type_commit_hook(type);
    MPIR_ERR_CHECK(mpi_errno);
#endif

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_TYPE_COMMIT_HOOK);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPID_Type_free_hook(MPIR_Datatype * type)
{
    int mpi_errno;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_TYPE_FREE_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_TYPE_FREE_HOOK);

    mpi_errno = MPIDI_NM_mpi_type_free_hook(type);
    MPIR_ERR_CHECK(mpi_errno);
#ifndef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_SHM_mpi_type_free_hook(type);
    MPIR_ERR_CHECK(mpi_errno);
#endif

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_TYPE_FREE_HOOK);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPID_Op_commit_hook(MPIR_Op * op)
{
    int mpi_errno;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_OP_COMMIT_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_OP_COMMIT_HOOK);

    mpi_errno = MPIDI_NM_mpi_op_commit_hook(op);
    MPIR_ERR_CHECK(mpi_errno);
#ifndef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_SHM_mpi_op_commit_hook(op);
    MPIR_ERR_CHECK(mpi_errno);
#endif

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_OP_COMMIT_HOOK);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPID_Op_free_hook(MPIR_Op * op)
{
    int mpi_errno;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_OP_FREE_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_OP_FREE_HOOK);

    mpi_errno = MPIDI_NM_mpi_op_free_hook(op);
    MPIR_ERR_CHECK(mpi_errno);
#ifndef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_SHM_mpi_op_free_hook(op);
    MPIR_ERR_CHECK(mpi_errno);
#endif

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_OP_FREE_HOOK);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
