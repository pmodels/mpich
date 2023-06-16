/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "mpidch4r.h"
#include "datatype.h"
#include "mpidu_init_shm.h"
#include "stream_workq.h"

#include <strings.h>    /* for strncasecmp */
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
        lockless

    - name        : MPIR_CVAR_CH4_NUM_VCIS
      category    : CH4
      type        : int
      default     : 1
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        Sets the number of VCIs to be implicitly used (should be a subset of MPIDI_CH4_MAX_VCIS).

    - name        : MPIR_CVAR_CH4_RESERVE_VCIS
      category    : CH4
      type        : int
      default     : 0
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        Sets the number of VCIs that user can explicitly allocate (should be a subset of MPIDI_CH4_MAX_VCIS).

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

    - name        : MPIR_CVAR_CH4_PACK_BUFFER_SIZE
      category    : CH4
      type        : int
      default     : 16384
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        Specifies the number of buffers for packing/unpacking active messages in
        each block of the pool. The size here should be greater or equal to the
        max of the eager buffer limit of SHM and NETMOD.

    - name        : MPIR_CVAR_CH4_NUM_PACK_BUFFERS_PER_CHUNK
      category    : CH4
      type        : int
      default     : 64
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        Specifies the number of buffers for packing/unpacking active messages in
        each block of the pool.

    - name        : MPIR_CVAR_CH4_MAX_NUM_PACK_BUFFERS
      category    : CH4
      type        : int
      default     : 0
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        Specifies the max number of buffers for packing/unpacking buffers in the
        pool. Use 0 for unlimited.

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
        else if (!strcmp(ckey, "composition=MPIDI_Alltoall_intra_composition_beta"))
            cnt->id = MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Alltoall_intra_composition_beta;
        else if (!strcmp(ckey, "composition=MPIDI_Alltoallv_intra_composition_alpha"))
            cnt->id =
                MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Alltoallv_intra_composition_alpha;
        else if (!strcmp(ckey, "composition=MPIDI_Alltoallw_intra_composition_alpha"))
            cnt->id =
                MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Alltoallw_intra_composition_alpha;
        else if (!strcmp(ckey, "composition=MPIDI_Allgather_intra_composition_alpha"))
            cnt->id =
                MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Allgather_intra_composition_alpha;
        else if (!strcmp(ckey, "composition=MPIDI_Allgather_intra_composition_beta"))
            cnt->id =
                MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Allgather_intra_composition_beta;
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

static int choose_netmod(void)
{
    int i, mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

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
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:

    goto fn_exit;
}

#ifdef MPIDI_CH4_USE_MT_RUNTIME
static const char *get_mt_model_name(int mt);
static void print_runtime_configurations(void);
static int parse_mt_model(const char *name);
static int set_runtime_configurations(void);

static const char *mt_model_names[MPIDI_CH4_NUM_MT_MODELS] = {
    "direct",
    "lockless",
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
#endif /* #ifdef MPIDI_CH4_USE_MT_RUNTIME */

static int set_runtime_configurations(void)
{
    int mpi_errno = MPI_SUCCESS;

#ifdef MPIDI_CH4_USE_MT_RUNTIME
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

#ifdef MPIDI_CH4_USE_MT_RUNTIME
  fn_fail:
#endif
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

static void *host_alloc_registered(uintptr_t size)
{
    void *ptr = MPL_malloc(size, MPL_MEM_BUFFER);
    MPIR_Assert(ptr);
    MPIR_gpu_register_host(ptr, size);
    return ptr;
}

static void host_free_registered(void *ptr)
{
    MPIR_gpu_unregister_host(ptr);
    MPL_free(ptr);
}

/* Register CH4-specific hints */
static void register_comm_hints(void)
{
    /* Non-standard hints for VCI selection. */
    MPIR_Comm_register_hint(MPIR_COMM_HINT_SENDER_VCI, "sender_vci",
                            MPIDI_set_comm_hint_sender_vci, MPIR_COMM_HINT_TYPE_INT, 0,
                            MPIDI_VCI_INVALID);
    MPIR_Comm_register_hint(MPIR_COMM_HINT_RECEIVER_VCI, "receiver_vci",
                            MPIDI_set_comm_hint_receiver_vci, MPIR_COMM_HINT_TYPE_INT, 0,
                            MPIDI_VCI_INVALID);
    MPIR_Comm_register_hint(MPIR_COMM_HINT_VCI, "vci", MPIDI_set_comm_hint_vci,
                            MPIR_COMM_HINT_TYPE_INT, 0, MPIDI_VCI_INVALID);
}

int MPID_Init(int requested, int *provided)
{
    int mpi_errno = MPI_SUCCESS;
    char strerrbuf[MPIR_STRERROR_BUF_SIZE] ATTRIBUTE((unused));

    MPIR_FUNC_ENTER;

    MPIDI_global.is_initialized = 0;

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
    if (MPIDI_global.prev_sighandler == SIG_IGN || MPIDI_global.prev_sighandler == SIG_DFL ||
        MPIDI_global.prev_sighandler == MPIDI_sigusr1_handler) {
        MPIDI_global.prev_sighandler = NULL;
    }
#endif

    mpi_errno = MPIDI_Self_init();
    MPIR_ERR_CHECK(mpi_errno);

    choose_netmod();

    /* Create all ch4-layer granular locks.
     * Note: some locks (e.g. MPIDIU_THREAD_HCOLL_MUTEX) may be unused due to configuration.
     * It is harmless to create them anyway rather than adding #ifdefs.
     */
    for (int i = 0; i < MAX_CH4_MUTEXES; i++) {
        int err;
        MPID_Thread_mutex_create(&MPIDI_global.m[i], &err);
        MPIR_Assert(err == 0);
    }

    mpi_errno = set_runtime_configurations();
    if (mpi_errno != MPI_SUCCESS)
        return mpi_errno;

    if (MPIR_CVAR_DEBUG_SUMMARY && MPIR_Process.rank == 0) {
#ifdef MPIDI_CH4_USE_MT_RUNTIME
        print_runtime_configurations();
#endif
        fprintf(stdout, "==== Various sizes and limits ====\n");
        fprintf(stdout, "sizeof(MPIDI_per_vci_t): %d\n", (int) sizeof(MPIDI_per_vci_t));
    }

    /* These mutex are used for the lockless MT model. */
    if (MPIDI_CH4_MT_MODEL == MPIDI_CH4_MT_LOCKLESS) {
        for (int i = 0; i < MPIR_REQUEST_NUM_POOLS; i++) {
            int err;
            MPID_Thread_mutex_create(&(MPIR_THREAD_VCI_HANDLE_POOL_MUTEXES[i]), &err);
            MPIR_Assert(err == 0);
        }
    }

    MPIDIU_avt_init();

    MPIDIU_map_create((void **) &(MPIDI_global.win_map), MPL_MEM_RMA);
    MPIDI_global.csel_root = NULL;

    /* Initialize multiple VCIs */
    /* TODO: add checks to ensure MPIDI_vci_t is padded or aligned to MPL_CACHELINE_SIZE */
    MPIR_Assert(MPIR_CVAR_CH4_NUM_VCIS >= 1);   /* number of vcis used in implicit vci hashing */
    MPIR_Assert(MPIR_CVAR_CH4_RESERVE_VCIS >= 0);       /* maximum number of vcis can be reserved */

    MPIDI_global.n_vcis = MPIR_CVAR_CH4_NUM_VCIS;
    MPIDI_global.n_total_vcis = MPIDI_global.n_vcis + MPIR_CVAR_CH4_RESERVE_VCIS;
    MPIDI_global.n_reserved_vcis = 0;
    MPIDI_global.share_reserved_vcis = false;

    MPIR_Assert(MPIDI_global.n_total_vcis <= MPIDI_CH4_MAX_VCIS);
    MPIR_Assert(MPIDI_global.n_total_vcis <= MPIR_REQUEST_NUM_POOLS);

    for (int i = 0; i < MPIDI_global.n_total_vcis; i++) {
        int err;
        MPID_Thread_mutex_create(&MPIDI_VCI(i).lock, &err);
        MPIR_Assert(err == 0);

        /* NOTE: 1-1 vci-pool mapping */
        /* For lockless, use a separate set of mutexes */
        if (MPIDI_CH4_MT_MODEL == MPIDI_CH4_MT_LOCKLESS)
            MPIR_Request_register_pool_lock(i, &MPIR_THREAD_VCI_HANDLE_POOL_MUTEXES[i]);
        else
            MPIR_Request_register_pool_lock(i, &MPIDI_VCI(i).lock);

        /* Initialize registered host buffer pool to be used as temporary unpack buffers */
        mpi_errno = MPIDU_genq_private_pool_create(MPIR_CVAR_CH4_PACK_BUFFER_SIZE,
                                                   MPIR_CVAR_CH4_NUM_PACK_BUFFERS_PER_CHUNK,
                                                   MPIR_CVAR_CH4_MAX_NUM_PACK_BUFFERS,
                                                   host_alloc_registered,
                                                   host_free_registered,
                                                   &MPIDI_global.per_vci[i].pack_buf_pool);
        MPIR_ERR_CHECK(mpi_errno);

    }


    /* internally does per-vci am initialization */
    MPIDIG_am_init();

    /* **** Call NM/SHM init_local **** */
    int shm_tag_bits, nm_tag_bits;
    shm_tag_bits = MPIR_TAG_BITS_DEFAULT;
    nm_tag_bits = MPIR_TAG_BITS_DEFAULT;
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

    /* setup receive queue statistics */
    mpi_errno = MPIDIG_recvq_init();
    MPIR_ERR_CHECK(mpi_errno);

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

    register_comm_hints();

    mpi_errno = MPIDU_stream_workq_init();
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPID_InitCompleted(void)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    if (MPIR_Process.has_parent) {
        char parent_port[MPI_MAX_PORT_NAME];
        mpi_errno = MPIR_pmi_kvs_get(-1, MPIDI_PARENT_PORT_KVSKEY, parent_port, MPI_MAX_PORT_NAME);
        MPIR_ERR_CHECK(mpi_errno);
        MPID_Comm_connect(parent_port, NULL, 0, MPIR_Process.comm_world, &MPIR_Process.comm_parent);
        MPIR_Assert(MPIR_Process.comm_parent != NULL);
        MPL_strncpy(MPIR_Process.comm_parent->name, "MPI_COMM_PARENT", MPI_MAX_OBJECT_NAME);
    }

    MPIR_FUNC_EXIT;

  fn_exit:
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

int MPID_Allocate_vci(int *vci, bool is_shared)
{
    int mpi_errno = MPI_SUCCESS;

    *vci = 0;
#if MPIDI_CH4_MAX_VCIS == 1
    MPIR_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**ch4nostream");
#else

    if (MPIDI_global.n_vcis + MPIDI_global.n_reserved_vcis >= MPIDI_global.n_total_vcis) {
        MPIR_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**outofstream");
    } else {
        MPIDI_global.n_reserved_vcis++;
        for (int i = MPIDI_global.n_vcis; i < MPIDI_global.n_total_vcis; i++) {
            if (!MPIDI_VCI(i).allocated) {
                MPIDI_VCI(i).allocated = true;
                *vci = i;
                break;
            }
        }
    }
#endif
    if (is_shared) {
        MPIDI_global.share_reserved_vcis = true;
    }
    return mpi_errno;
}

int MPID_Deallocate_vci(int vci)
{
    MPIR_Assert(vci < MPIDI_global.n_total_vcis && vci >= MPIDI_global.n_vcis);
    MPIR_Assert(MPIDI_VCI(vci).allocated);
    MPIDI_VCI(vci).allocated = false;
    MPIDI_global.n_reserved_vcis--;
    return MPI_SUCCESS;
}

int MPID_Stream_create_hook(MPIR_Stream * stream)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    if (stream->type == MPIR_STREAM_GPU) {
        mpi_errno = MPIDU_stream_workq_alloc(&stream->dev.workq, stream->vci);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPID_Stream_free_hook(MPIR_Stream * stream)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    if (stream->dev.workq) {
        mpi_errno = MPIDU_stream_workq_dealloc(stream->dev.workq);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPID_Finalize(void)
{
    int mpi_errno;
    MPIR_FUNC_ENTER;

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

    MPIDIG_am_finalize();

    MPIDIU_avt_destroy();

    mpi_errno = MPIDU_Init_shm_finalize();
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIDU_stream_workq_finalize();
    MPIR_ERR_CHECK(mpi_errno);

    MPIR_pmi_finalize();

    for (int i = 0; i < MAX_CH4_MUTEXES; i++) {
        int err;
        MPID_Thread_mutex_destroy(&MPIDI_global.m[i], &err);
        MPIR_Assert(err == 0);
    }

    for (int i = 0; i < MPIDI_global.n_total_vcis; i++) {
        MPIDU_genq_private_pool_destroy(MPIDI_global.per_vci[i].pack_buf_pool);

        int err;
        MPID_Thread_mutex_destroy(&MPIDI_VCI(i).lock, &err);
        MPIR_Assert(err == 0);
    }

    memset(&MPIDI_global, 0, sizeof(MPIDI_global));

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPID_Get_universe_size(int *universe_size)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    mpi_errno = MPIR_pmi_get_universe_size(universe_size);

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

int MPID_Get_processor_name(char *name, int namelen, int *resultlen)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    if (!MPIDI_global.pname_set) {
#ifdef HAVE_GETHOSTNAME

        if (gethostname(MPIDI_global.pname, MPI_MAX_PROCESSOR_NAME) == 0)
            MPIDI_global.pname_len = (int) strlen(MPIDI_global.pname);

#elif defined(HAVE_SYSINFO)

        if (sysinfo(SI_HOSTNAME, MPIDI_global.pname, MPI_MAX_PROCESSOR_NAME) == 0)
            MPIDI_global.pname_len = (int) strlen(MPIDI_global.pname);

#else
        snprintf(MPIDI_global.pname, MPI_MAX_PROCESSOR_NAME, "%d", MPIR_Process.rank);
        MPIDI_global.pname_len = (int) strlen(MPIDI_global.pname);
#endif
        MPIDI_global.pname_set = 1;
    }

    MPIR_ERR_CHKANDJUMP(MPIDI_global.pname_len <= 0, mpi_errno, MPI_ERR_OTHER, "**procnamefailed");
    MPL_strncpy(name, MPIDI_global.pname, namelen);

    if (resultlen)
        *resultlen = MPIDI_global.pname_len;

  fn_exit:
    MPIR_FUNC_EXIT;
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
    MPIR_FUNC_ENTER;

    MPIR_hwtopo_gid_t mem_gid = MPIR_HWTOPO_GID_ROOT;
    alloc_mem_buf_type_e buf_type = ALLOC_MEM_BUF_TYPE__UNSET;
    void *user_buf = NULL;
    void *real_buf;
    int alignment = MAX_ALIGNMENT;

    const char *val;
    val = MPIR_Info_lookup(info_ptr, "mpich_buf_type");
    if (val) {
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

    val = MPIR_Info_lookup(info_ptr, "mpi_minimum_memory_alignment");
    if (val) {
        alignment = atoi(val);
    }

    switch (buf_type) {
        case ALLOC_MEM_BUF_TYPE__HBM:
        case ALLOC_MEM_BUF_TYPE__DDR:
            /* requested memory type is available in the system and
             * process is bound to the corresponding device; allocate
             * memory and bind it to device. */
            assert(mem_gid != MPIR_HWTOPO_GID_ROOT);
#ifdef MAP_ANON
            real_buf = MPL_mmap(NULL, size + alignment, PROT_READ | PROT_WRITE,
                                MAP_ANON | MAP_PRIVATE, -1, 0, MPL_MEM_USER);
#else
            real_buf = MPL_malloc(size + alignment, MPL_MEM_USER);
#endif
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
    MPIR_FUNC_EXIT;
    return user_buf;
}

int MPID_Free_mem(void *user_buf)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    alloc_mem_container_s *container;

    MPID_THREAD_CS_ENTER(VCI, MPIDIU_THREAD_ALLOC_MEM_MUTEX);
    HASH_FIND_PTR(alloc_mem_container_list, &user_buf, container);
    assert(container);
    HASH_DEL(alloc_mem_container_list, container);
    MPID_THREAD_CS_EXIT(VCI, MPIDIU_THREAD_ALLOC_MEM_MUTEX);

    switch (container->buf_type) {
        case ALLOC_MEM_BUF_TYPE__HBM:
        case ALLOC_MEM_BUF_TYPE__DDR:
#ifdef MAP_ANON
            MPL_munmap(container->real_buf, container->size, MPL_MEM_USER);
#else
            MPL_free(container->real_buf);
#endif
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
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPID_Comm_get_lpid(MPIR_Comm * comm_ptr, int idx, uint64_t * lpid_ptr, bool is_remote)
{
    int mpi_errno = MPI_SUCCESS;
    int avtid = 0, lpid = 0;
    MPIR_FUNC_ENTER;

    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM)
        MPIDIU_comm_rank_to_pid(comm_ptr, idx, &lpid, &avtid);
    else if (is_remote)
        MPIDIU_comm_rank_to_pid(comm_ptr, idx, &lpid, &avtid);
    else {
        MPIDIU_comm_rank_to_pid_local(comm_ptr, idx, &lpid, &avtid);
    }

    *lpid_ptr = MPIDIU_GPID_CREATE(avtid, lpid);

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

int MPID_Get_node_id(MPIR_Comm * comm, int rank, int *id_p)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    MPIDIU_get_node_id(comm, rank, id_p);

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

int MPID_Get_max_node_id(MPIR_Comm * comm, int *max_id_p)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    *max_id_p = MPIR_Process.num_nodes - 1;

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

MPI_Aint MPID_Aint_add(MPI_Aint base, MPI_Aint disp)
{
    MPI_Aint result;
    MPIR_FUNC_ENTER;
    result = (MPI_Aint) ((char *) base + disp);
    MPIR_FUNC_EXIT;
    return result;
}

MPI_Aint MPID_Aint_diff(MPI_Aint addr1, MPI_Aint addr2)
{
    MPI_Aint result;
    MPIR_FUNC_ENTER;

    result = (MPI_Aint) ((char *) addr1 - (char *) addr2);
    MPIR_FUNC_EXIT;
    return result;
}

int MPID_Type_commit_hook(MPIR_Datatype * type)
{
    int mpi_errno;

    MPIR_FUNC_ENTER;

    mpi_errno = MPIDI_NM_mpi_type_commit_hook(type);
    MPIR_ERR_CHECK(mpi_errno);
#ifndef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_SHM_mpi_type_commit_hook(type);
    MPIR_ERR_CHECK(mpi_errno);
#endif

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPID_Type_free_hook(MPIR_Datatype * type)
{
    int mpi_errno;

    MPIR_FUNC_ENTER;

    mpi_errno = MPIDI_NM_mpi_type_free_hook(type);
    MPIR_ERR_CHECK(mpi_errno);
#ifndef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_SHM_mpi_type_free_hook(type);
    MPIR_ERR_CHECK(mpi_errno);
#endif

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPID_Op_commit_hook(MPIR_Op * op)
{
    int mpi_errno;

    MPIR_FUNC_ENTER;

    mpi_errno = MPIDI_NM_mpi_op_commit_hook(op);
    MPIR_ERR_CHECK(mpi_errno);
#ifndef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_SHM_mpi_op_commit_hook(op);
    MPIR_ERR_CHECK(mpi_errno);
#endif

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPID_Op_free_hook(MPIR_Op * op)
{
    int mpi_errno;

    MPIR_FUNC_ENTER;

    mpi_errno = MPIDI_NM_mpi_op_free_hook(op);
    MPIR_ERR_CHECK(mpi_errno);
#ifndef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_SHM_mpi_op_free_hook(op);
    MPIR_ERR_CHECK(mpi_errno);
#endif

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
