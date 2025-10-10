/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpid_ucc.h"

#ifndef NDEBUG
/* include this file and set `verbose_debug` flag in `MPIDI_common_ucc_enable()` to get more output */
#include "mpid_ucc_debug.h"
#endif

MPIDI_common_ucc_priv_t MPIDI_common_ucc_priv = { 0 };

#define MPIDI_COMMON_UCC_VERBOSE_STRING(STRING) #STRING,
const char *MPIDI_COMMON_UCC_VERBOSE_LEVEL_TO_STRING[] = {
    MPIDI_COMMON_UCC_VERBOSE_LEVELS(MPIDI_COMMON_UCC_VERBOSE_STRING)
};

#define MPIDI_COMMON_UCC_VERBOSE_COMM_WORLD \
    comm_ptr == MPIR_Process.comm_world ? " (world)" : ""

static int mpidi_ucc_finalize(void *param ATTRIBUTE((unused)))
{
    int mpidi_ucc_err = MPIDI_COMMON_UCC_RETVAL_SUCCESS;
    MPIR_FUNC_ENTER;
    MPIDI_COMMON_UCC_DEBUG(MPIDI_COMMON_UCC_VERBOSE_LEVEL_BASIC, "entering mpidi ucc finalize");

    if (!MPIDI_common_ucc_priv.ucc_enabled || !MPIDI_common_ucc_priv.ucc_initialized) {
        goto fn_exit;
    }

    MPIDI_COMMON_UCC_VERBOSE(MPIDI_COMMON_UCC_VERBOSE_LEVEL_BASIC, "finalizing ucc library");

    MPIR_Progress_hook_deactivate(MPIDI_common_ucc_priv.progress_hook_id);
    MPIR_Progress_hook_deregister(MPIDI_common_ucc_priv.progress_hook_id);

    ucc_context_destroy(MPIDI_common_ucc_priv.ucc_context);
    if (ucc_finalize(MPIDI_common_ucc_priv.ucc_lib) != UCC_OK) {
        MPIDI_COMMON_UCC_ERROR("ucc finalize failed");
        goto fn_fail;
    }

    MPIDI_common_ucc_priv.ucc_initialized = 0;

  fn_exit:
    MPIDI_COMMON_UCC_DEBUG(MPIDI_COMMON_UCC_VERBOSE_LEVEL_BASIC, "leaving mpidi ucc finalize");
    MPIR_FUNC_EXIT;
    return mpidi_ucc_err;
  fn_fail:
    MPIDI_COMMON_UCC_WARNING("mpidi ucc finalize failed");
    goto fn_exit;
}

int MPIDI_common_ucc_enable(int verbose_level, const char *verbose_level_str, int debug_flag)
{
    /* Only initialize the basic flags here, as the actual UCC initialization happens
     * later when `MPIDI_common_ucc_comm_create_hook()` is called for the first time.
     */

    if (verbose_level) {
        MPIDI_common_ucc_priv.verbose_level = (MPIDI_common_ucc_verbose_levels_t) verbose_level;
    } else if (verbose_level_str) {
        MPIDI_COMMON_UCC_VERBOSE_STRING_TO_LEVEL(verbose_level_str,
                                                 MPIDI_common_ucc_priv.verbose_level);
    } else {
        MPIDI_common_ucc_priv.verbose_level = MPIDI_COMMON_UCC_VERBOSE_LEVEL_NONE;
    }

    MPIDI_common_ucc_priv.verbose_debug = debug_flag;

    if (!MPIDI_common_ucc_priv.ucc_enabled) {
        MPIR_Add_finalize(mpidi_ucc_finalize, NULL, MPIR_FINALIZE_CALLBACK_PRIO + 1);
        MPIDI_common_ucc_priv.ucc_enabled = 1;
    }

    MPIDI_COMMON_UCC_DEBUG(MPIDI_COMMON_UCC_VERBOSE_LEVEL_BASIC, "use of ucc generally enabled");
    return MPIDI_COMMON_UCC_RETVAL_SUCCESS;
}

static ucc_status_t mpidi_ucc_oob_allgather_test(void *req)
{
    MPIR_Request *request = (MPIR_Request *) req;
    int completed = 0;

    MPIDI_COMMON_UCC_DEBUG(MPIDI_COMMON_UCC_VERBOSE_LEVEL_BASIC, "entering oob allgather test");

    int mpi_errno = MPIR_Test(request, &completed, MPI_STATUS_IGNORE);

    if (mpi_errno != MPI_SUCCESS) {
        MPIDI_COMMON_UCC_WARNING("MPIR_Test() for oob allgather failed");
        return UCC_ERR_NO_MESSAGE;
    }

    if (!completed) {
        MPIDI_COMMON_UCC_DEBUG(MPIDI_COMMON_UCC_VERBOSE_LEVEL_BASIC,
                               "leaving oob allgather test (in progress)");
        return UCC_INPROGRESS;
    }

    MPIDI_COMMON_UCC_DEBUG(MPIDI_COMMON_UCC_VERBOSE_LEVEL_BASIC,
                           "leaving oob allgather test (completed)");
    return UCC_OK;
}

static ucc_status_t mpidi_ucc_oob_allgather_free(void *req)
{
    MPIR_Request *request = (MPIR_Request *) req;

    MPIR_Request_free(request);

    MPIDI_COMMON_UCC_DEBUG(MPIDI_COMMON_UCC_VERBOSE_LEVEL_BASIC, "oob allgather free called");
    return UCC_OK;
}

static ucc_status_t mpidi_ucc_oob_allgather(void *sbuf, void *rbuf, size_t msglen,
                                            void *oob_coll_ctx, void **req)
{
    MPIR_Comm *comm = (MPIR_Comm *) oob_coll_ctx;
    MPIR_Request *request = NULL;

    MPIDI_COMMON_UCC_DEBUG(MPIDI_COMMON_UCC_VERBOSE_LEVEL_BASIC, "entering oob allgather");

    int mpi_errno =
        MPIR_Iallgather_impl(sbuf, msglen, MPIR_BYTE_INTERNAL, rbuf, msglen, MPIR_BYTE_INTERNAL,
                             comm, &request);

    if (mpi_errno != MPI_SUCCESS) {
        MPIDI_COMMON_UCC_WARNING("MPIR_Iallgather_impl() for oob allgather failed");
        return UCC_ERR_NO_MESSAGE;
    }

    if (!request) {
        request = MPIR_Request_create_complete(MPIR_REQUEST_KIND__COLL);
    }

    *req = request;

    MPIDI_COMMON_UCC_DEBUG(MPIDI_COMMON_UCC_VERBOSE_LEVEL_BASIC, "oob allgather called");
    return UCC_OK;
}

static int mpidi_ucc_progress(int vci, int *made_progress)
{
    ucc_context_progress(MPIDI_common_ucc_priv.ucc_context);
    return MPI_SUCCESS;
}

int MPIDI_common_ucc_progress(int *made_progress)
{
    if (!MPIDI_common_ucc_priv.ucc_enabled || !MPIDI_common_ucc_priv.ucc_initialized)
        return MPI_SUCCESS;

    return mpidi_ucc_progress(-1, made_progress);
}

static int mpidi_ucc_setup_lib(void)
{
    int mpidi_ucc_err = MPIDI_COMMON_UCC_RETVAL_SUCCESS;
    char str_buf[256];
    ucc_lib_config_h lib_config;
    ucc_context_config_h ctx_config;
    ucc_lib_params_t lib_params;
    ucc_context_params_t ctx_params;

    ucc_lib_h *lib_ptr = NULL;
    ucc_lib_attr_t *lib_attr_ptr = NULL;
    ucc_lib_config_h *lib_config_ptr = NULL;
    ucc_lib_params_t *lib_params_ptr = NULL;
    ucc_context_params_t *ctx_params_ptr = NULL;
    ucc_context_config_h *ctx_config_ptr = NULL;

    MPIR_FUNC_ENTER;
    MPIDI_COMMON_UCC_DEBUG(MPIDI_COMMON_UCC_VERBOSE_LEVEL_BASIC, "entering mpidi setup lib");

    if (!MPIDI_common_ucc_priv.ucc_enabled) {
        MPIDI_COMMON_UCC_ERROR("ucc is disabled explicitly");
        goto fn_fail;
    }

    if (ucc_lib_config_read(NULL, NULL, &lib_config) != UCC_OK) {
        MPIDI_COMMON_UCC_ERROR("ucc lib config read failed");
        goto fn_fail;
    }

    lib_config_ptr = &lib_config;
    lib_params_ptr = &lib_params;

#ifdef MPICH_IS_THREADED
    lib_params_ptr->thread_mode =
        MPIR_ThreadInfo.isThreaded ? UCC_THREAD_MULTIPLE : UCC_THREAD_SINGLE;
    lib_params_ptr->mask = UCC_LIB_PARAM_FIELD_THREAD_MODE;
#else
    lib_params_ptr->mask = 0;
#endif

    MPIDI_COMMON_UCC_VERBOSE(MPIDI_COMMON_UCC_VERBOSE_LEVEL_BASIC, "initializing ucc library");
    if (ucc_init(lib_params_ptr, *lib_config_ptr, &MPIDI_common_ucc_priv.ucc_lib) != UCC_OK) {
        MPIDI_COMMON_UCC_ERROR("ucc lib init failed");
        goto fn_fail;
    }

    lib_ptr = &MPIDI_common_ucc_priv.ucc_lib;
    lib_attr_ptr = &MPIDI_common_ucc_priv.ucc_lib_attr;
    lib_attr_ptr->mask = UCC_LIB_ATTR_FIELD_THREAD_MODE | UCC_LIB_ATTR_FIELD_COLL_TYPES;

    if (ucc_lib_get_attr(*lib_ptr, lib_attr_ptr)) {
        MPIDI_COMMON_UCC_ERROR("ucc get lib attr failed");
        goto fn_fail;
    }
#ifdef MPICH_IS_THREADED
    if (lib_attr_ptr->thread_mode < lib_params_ptr->thread_mode) {
        MPIDI_COMMON_UCC_ERROR("ucc library doesn't support MPI_THREAD_MULTIPLE");
        goto fn_fail;
    }
#endif

    ctx_params_ptr = &ctx_params;
    ctx_params_ptr->mask = UCC_CONTEXT_PARAM_FIELD_OOB;
    ctx_params_ptr->oob.allgather = mpidi_ucc_oob_allgather;
    ctx_params_ptr->oob.req_test = mpidi_ucc_oob_allgather_test;
    ctx_params_ptr->oob.req_free = mpidi_ucc_oob_allgather_free;

    /* TODO: For the time being, we rely on MPI_COMM_WORLD as the "substrate" for
     * all the out-of-band communication. For the future, we have to think about
     * how to realize this also for the pure session model.
     */
    ctx_params_ptr->oob.coll_info = MPIR_Process.comm_world;
    ctx_params_ptr->oob.n_oob_eps = MPIR_Process.size;
    ctx_params_ptr->oob.oob_ep = MPIR_Process.rank;

    if (ucc_context_config_read(*lib_ptr, NULL, &ctx_config) != UCC_OK) {
        MPIDI_COMMON_UCC_ERROR("ucc context config read failed");
        goto fn_fail;
    }

    ctx_config_ptr = &ctx_config;

    snprintf(str_buf, sizeof(str_buf), "%d", MPIR_Process.size);

    if (ucc_context_config_modify(*ctx_config_ptr, NULL, "ESTIMATED_NUM_EPS", str_buf) != UCC_OK) {
        MPIDI_COMMON_UCC_ERROR("ucc context config modify failed for estimated_num_eps");
        goto fn_fail;
    }

    snprintf(str_buf, sizeof(str_buf), "%d", MPIR_Process.local_size);

    if (ucc_context_config_modify(*ctx_config_ptr, NULL, "ESTIMATED_NUM_PPN", str_buf) != UCC_OK) {
        MPIDI_COMMON_UCC_ERROR("ucc context config modify failed for estimated_num_eps");
        goto fn_fail;
    }

    if (ucc_context_create(*lib_ptr, ctx_params_ptr,
                           *ctx_config_ptr, &MPIDI_common_ucc_priv.ucc_context) != UCC_OK) {
        MPIDI_COMMON_UCC_ERROR("ucc context create failed");
        goto fn_fail;
    }

    int mpi_errno = MPIR_Progress_hook_register(-1, mpidi_ucc_progress,
                                                &MPIDI_common_ucc_priv.progress_hook_id);

    if (mpi_errno != MPI_SUCCESS) {
        MPIDI_COMMON_UCC_ERROR("mpir progress hook register failed");
        goto fn_fail;
    }

    MPIR_Progress_hook_activate(MPIDI_common_ucc_priv.progress_hook_id);

    MPIDI_common_ucc_priv.ucc_initialized = 1;

  fn_exit:
    if (lib_config_ptr)
        ucc_lib_config_release(*lib_config_ptr);
    if (ctx_config_ptr)
        ucc_context_config_release(*ctx_config_ptr);
    MPIDI_COMMON_UCC_DEBUG(MPIDI_COMMON_UCC_VERBOSE_LEVEL_BASIC, "leaving mpidi setup lib");
    MPIR_FUNC_EXIT;
    return mpidi_ucc_err;
  fn_fail:
    if (lib_ptr)
        ucc_finalize(*lib_ptr);
    MPIDI_common_ucc_priv.ucc_initialized = 0;
    MPIDI_COMMON_UCC_WARNING("mpidi setup lib failed");
    goto fn_exit;
}

static uint64_t mpidi_ucc_rank_map_cb(uint64_t ep, void *cb_ctx)
{
    MPIR_Comm *comm_ptr = cb_ctx;
    MPIR_Lpid lpid;
    int rank = (int) ep;

    lpid = MPIR_comm_rank_to_lpid(comm_ptr, rank);

    return (uint64_t) lpid;
}

static int mpidi_ucc_setup_comm_team(MPIR_Comm * comm_ptr)
{
    int mpidi_ucc_err = MPIDI_COMMON_UCC_RETVAL_SUCCESS;
    ucc_status_t status;
    MPIDI_COMMON_UCC_DEBUG(MPIDI_COMMON_UCC_VERBOSE_LEVEL_COMM, "entering mpidi setup comm team");

    ucc_team_params_t team_params = {
        .mask = UCC_TEAM_PARAM_FIELD_EP_MAP |
            UCC_TEAM_PARAM_FIELD_EP | UCC_TEAM_PARAM_FIELD_EP_RANGE | UCC_TEAM_PARAM_FIELD_ID,
        .ep_map = {
                   .type = (comm_ptr == MPIR_Process.comm_world) ? UCC_EP_MAP_FULL : UCC_EP_MAP_CB,
                   .ep_num = MPIR_Comm_size(comm_ptr),
                   .cb.cb = mpidi_ucc_rank_map_cb,
                   .cb.cb_ctx = (void *) comm_ptr},
        .ep = MPIR_Comm_rank(comm_ptr),
        .ep_range = UCC_COLLECTIVE_EP_RANGE_CONTIG,
        .id = comm_ptr->context_id
    };

    MPIDI_COMMON_UCC_VERBOSE(MPIDI_COMMON_UCC_VERBOSE_LEVEL_COMM,
                             "creating ucc_team for comm %p, comm_id %llu, comm_size %d",
                             (void *) comm_ptr, (long long unsigned) team_params.id,
                             MPIR_Comm_size(comm_ptr));

    if (ucc_team_create_post
        (&MPIDI_common_ucc_priv.ucc_context, 1, &team_params,
         &comm_ptr->ucc_priv.ucc_team) != UCC_OK) {
        MPIDI_COMMON_UCC_ERROR("ucc team create post failed");
        goto fn_fail;
    }
    while ((status = ucc_team_create_test(comm_ptr->ucc_priv.ucc_team)) == UCC_INPROGRESS) {
        MPID_Progress_test(NULL);
    }
    if (status != UCC_OK) {
        MPIDI_COMMON_UCC_ERROR("ucc team create test failed");
        goto fn_fail;
    }

    MPIDI_COMMON_UCC_VERBOSE(MPIDI_COMMON_UCC_VERBOSE_LEVEL_COMM,
                             "team created for comm %p, comm_id %llu, comm_size %d",
                             (void *) comm_ptr, (long long unsigned) team_params.id,
                             MPIR_Comm_size(comm_ptr));

  fn_exit:
    MPIDI_COMMON_UCC_DEBUG(MPIDI_COMMON_UCC_VERBOSE_LEVEL_COMM, "leaving mpidi setup comm team");
    return mpidi_ucc_err;
  fn_fail:
    comm_ptr->ucc_priv.ucc_team = NULL;
    comm_ptr->ucc_priv.ucc_initialized = 0;
    MPIDI_COMMON_UCC_WARNING("mpidi setup comm team failed");
    goto fn_exit;
}

int MPIDI_common_ucc_comm_create_hook(MPIR_Comm * comm_ptr)
{
    int mpidi_ucc_err = MPIDI_COMMON_UCC_RETVAL_SUCCESS;
    MPIR_FUNC_ENTER;
    MPIDI_COMMON_UCC_DEBUG(MPIDI_COMMON_UCC_VERBOSE_LEVEL_COMM,
                           "entering mpidi comm create hook for comm_id %d", comm_ptr->context_id);

    /* TODO: Later, this could be decided on a per comm basis (e.g., based on an Info key) */
    comm_ptr->ucc_priv.ucc_enabled = MPIDI_common_ucc_priv.ucc_enabled;

    comm_ptr->ucc_priv.ucc_initialized = 0;

    if (!MPIDI_common_ucc_priv.ucc_enabled || !comm_ptr->ucc_priv.ucc_enabled) {
        goto fn_exit;
    }

    if (!MPIDI_common_ucc_priv.comm_world_initialized && (comm_ptr != MPIR_Process.comm_world)) {
        /* TODO: In the case of the pure session model (i.e., without MPI_COMM_WORLD), we always
         * get here and fall back. Therefore, as a task for future improvements, we will need to
         * consider something else for this case, not relying on MPI_COMM_WORLD and its ranks
         * for the out-of-band communication. (See also mpidi_ucc_setup_lib() above.)
         */
        goto fn_exit;
    }

    if ((MPIDI_COMMON_UCC_COMM_EXCL_COND_TYPE) || (MPIDI_COMMON_UCC_COMM_EXCL_COND_SIZE) ||
        (MPIDI_COMMON_UCC_COMM_EXCL_COND_DEV)) {
#ifdef MPIDI_COMMON_UCC_OUTPUT_COMM_EXCL_COND
        MPIDI_COMMON_UCC_OUTPUT_COMM_EXCL_COND;
#endif
        goto disabled;
    }

    MPIDI_COMMON_UCC_VERBOSE(MPIDI_COMMON_UCC_VERBOSE_LEVEL_COMM,
                             "create hook for comm %p, comm_id %d%s", (void *) comm_ptr,
                             comm_ptr->context_id, MPIDI_COMMON_UCC_VERBOSE_COMM_WORLD);

    if (!MPIDI_common_ucc_priv.ucc_initialized) {
        mpidi_ucc_err = mpidi_ucc_setup_lib();
        if (mpidi_ucc_err != MPIDI_COMMON_UCC_RETVAL_SUCCESS)
            goto fn_fail;
    }

    MPIR_Assert(MPIDI_common_ucc_priv.ucc_initialized);

    mpidi_ucc_err = mpidi_ucc_setup_comm_team(comm_ptr);
    if (mpidi_ucc_err != MPIDI_COMMON_UCC_RETVAL_SUCCESS)
        goto fn_fail;

    if (comm_ptr == MPIR_Process.comm_world) {
        MPIDI_common_ucc_priv.comm_world_initialized = 1;
    }

    comm_ptr->ucc_priv.ucc_initialized = 1;

  fn_exit:
    MPIDI_COMMON_UCC_DEBUG(MPIDI_COMMON_UCC_VERBOSE_LEVEL_BASIC, "leaving mpidi comm create hook");
    MPIR_FUNC_EXIT;
    return mpidi_ucc_err;
  fn_fail:
    MPIDI_COMMON_UCC_WARNING("comm create hook failed");
    goto fn_exit;
  disabled:
    MPIDI_COMMON_UCC_VERBOSE(MPIDI_COMMON_UCC_VERBOSE_LEVEL_COMM,
                             "create hook for comm %p, comm_id %d%s is disabled",
                             (void *) comm_ptr, comm_ptr->context_id,
                             MPIDI_COMMON_UCC_VERBOSE_COMM_WORLD);
    goto fn_exit;
}

int MPIDI_common_ucc_comm_destroy_hook(MPIR_Comm * comm_ptr)
{
    int mpidi_ucc_err = MPIDI_COMMON_UCC_RETVAL_SUCCESS;
    ucc_status_t status;
    MPIR_FUNC_ENTER;
    MPIDI_COMMON_UCC_DEBUG(MPIDI_COMMON_UCC_VERBOSE_LEVEL_BASIC,
                           "entering mpidi comm destroy hook");

    if (!MPIDI_common_ucc_priv.ucc_enabled || !MPIDI_common_ucc_priv.ucc_initialized) {
        goto fn_exit;
    }

    if (!comm_ptr->ucc_priv.ucc_initialized || (comm_ptr->comm_kind != MPIR_COMM_KIND__INTRACOMM)) {
        goto disabled;
    }

    MPIDI_COMMON_UCC_VERBOSE(MPIDI_COMMON_UCC_VERBOSE_LEVEL_COMM,
                             "destroy hook for comm %p, comm_id %d%s", (void *) comm_ptr,
                             comm_ptr->context_id, MPIDI_COMMON_UCC_VERBOSE_COMM_WORLD);

    while ((status = ucc_team_destroy(comm_ptr->ucc_priv.ucc_team)) == UCC_INPROGRESS) {
        /* Wait until the team has been destroyed (or there is an error) */
        MPID_Progress_test(NULL);
    }

    if (status != UCC_OK) {
        MPIDI_COMMON_UCC_ERROR("ucc team destroy failed");
        goto fn_fail;
    }

    comm_ptr->ucc_priv.ucc_initialized = 0;

  fn_exit:
    MPIDI_COMMON_UCC_DEBUG(MPIDI_COMMON_UCC_VERBOSE_LEVEL_BASIC, "leaving mpidi comm destroy hook");
    MPIR_FUNC_EXIT;
    return mpidi_ucc_err;
  fn_fail:
    MPIDI_COMMON_UCC_WARNING("comm destroy hook failed");
    goto fn_exit;
  disabled:
    MPIDI_COMMON_UCC_VERBOSE(MPIDI_COMMON_UCC_VERBOSE_LEVEL_COMM,
                             "destroy hook for comm %p, comm_id %d%s is disabled",
                             (void *) comm_ptr, comm_ptr->context_id,
                             MPIDI_COMMON_UCC_VERBOSE_COMM_WORLD);
    goto fn_exit;
}
