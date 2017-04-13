/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2016 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#ifndef CH4_INIT_H_INCLUDED
#define CH4_INIT_H_INCLUDED

#include "ch4_impl.h"
#include "ch4r_proc.h"
#include "ch4i_comm.h"
#include "strings.h"
#include "datatype.h"

#ifdef USE_PMI2_API
/* PMI does not specify a max size for jobid_size in PMI2_Job_GetId.
   CH3 uses jobid_size=MAX_JOBID_LEN=1024 when calling
   PMI2_Job_GetId. */
#define MPIDI_MAX_JOBID_LEN PMI2_MAX_VALLEN
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
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        If non-empty, this cvar specifies which network module to use

    - name        : MPIR_CVAR_CH4_SHM
      category    : CH4
      type        : string
      default     : ""
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        If non-empty, this cvar specifies which shm module to use

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

#undef FUNCNAME
#define FUNCNAME MPIDI_choose_netmod
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_choose_netmod(void)
{
    int i, mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CHOOSE_NETMOD);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CHOOSE_NETMOD);

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
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CHOOSE_NETMOD);
    return mpi_errno;
  fn_fail:

    goto fn_exit;
}

#if (MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY__POBJ)
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

#undef FUNCNAME
#define FUNCNAME MPID_Init
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPID_Init(int *argc,
                                        char ***argv,
                                        int requested, int *provided, int *has_args, int *has_env)
{
    int pmi_errno, mpi_errno = MPI_SUCCESS, rank, has_parent, size, appnum, thr_err;
    int avtid;
    int n_nm_vnis_provided;
#ifdef MPIDI_BUILD_CH4_SHM
    int n_shm_vnis_provided;
#endif
#ifndef USE_PMI2_API
    int max_pmi_name_length;
#endif

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_INIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_INIT);

#ifdef MPL_USE_DBG_LOGGING
    MPIDI_CH4_DBG_GENERAL = MPL_dbg_class_alloc("CH4", "ch4");
    MPIDI_CH4_DBG_MAP = MPL_dbg_class_alloc("CH4_MAP", "ch4_map");
    MPIDI_CH4_DBG_COMM = MPL_dbg_class_alloc("CH4_COMM", "ch4_comm");
    MPIDI_CH4_DBG_MEMORY = MPL_dbg_class_alloc("CH4_MEMORY", "ch4_memory");
#endif
    MPIDI_choose_netmod();
#ifdef USE_PMI2_API
    pmi_errno = PMI2_Init(&has_parent, &size, &rank, &appnum);

    if (pmi_errno != PMI_SUCCESS) {
        MPIR_ERR_SETANDJUMP1(pmi_errno, MPI_ERR_OTHER, "**pmi_init", "**pmi_init %d", pmi_errno);
    }

    MPIDI_CH4_Global.jobid = (char *) MPL_malloc(MPIDI_MAX_JOBID_LEN);
    pmi_errno = PMI2_Job_GetId(MPIDI_CH4_Global.jobid, MPIDI_MAX_JOBID_LEN);
    if (pmi_errno != PMI_SUCCESS) {
        MPIR_ERR_SETANDJUMP1(pmi_errno, MPI_ERR_OTHER, "**pmi_job_getid",
                             "**pmi_job_getid %d", pmi_errno);
    }
#else
    pmi_errno = PMI_Init(&has_parent);

    if (pmi_errno != PMI_SUCCESS) {
        MPIR_ERR_SETANDJUMP1(pmi_errno, MPI_ERR_OTHER, "**pmi_init", "**pmi_init %d", pmi_errno);
    }

    pmi_errno = PMI_Get_rank(&rank);

    if (pmi_errno != PMI_SUCCESS) {
        MPIR_ERR_SETANDJUMP1(pmi_errno, MPI_ERR_OTHER, "**pmi_get_rank",
                             "**pmi_get_rank %d", pmi_errno);
    }

    pmi_errno = PMI_Get_size(&size);

    if (pmi_errno != 0) {
        MPIR_ERR_SETANDJUMP1(pmi_errno, MPI_ERR_OTHER, "**pmi_get_size",
                             "**pmi_get_size %d", pmi_errno);
    }

    pmi_errno = PMI_Get_appnum(&appnum);

    if (pmi_errno != PMI_SUCCESS) {
        MPIR_ERR_SETANDJUMP1(pmi_errno, MPI_ERR_OTHER, "**pmi_get_appnum",
                             "**pmi_get_appnum %d", pmi_errno);
    }

    pmi_errno = PMI_KVS_Get_name_length_max(&max_pmi_name_length);
    if (pmi_errno != PMI_SUCCESS) {
        MPIR_ERR_SETANDJUMP1(pmi_errno, MPI_ERR_OTHER, "**pmi_kvs_get_name_length_max",
                             "**pmi_kvs_get_name_length_max %d", pmi_errno);
    }

    MPIDI_CH4_Global.jobid = (char *) MPL_malloc(max_pmi_name_length);
    pmi_errno = PMI_KVS_Get_my_name(MPIDI_CH4_Global.jobid, max_pmi_name_length);
    if (pmi_errno != PMI_SUCCESS) {
        MPIR_ERR_SETANDJUMP1(pmi_errno, MPI_ERR_OTHER, "**pmi_kvs_get_my_name",
                             "**pmi_kvs_get_my_name %d", pmi_errno);
    }
#endif

    MPID_Thread_mutex_create(&MPIDI_CH4I_THREAD_PROGRESS_MUTEX, &thr_err);
    MPID_Thread_mutex_create(&MPIDI_CH4I_THREAD_PROGRESS_HOOK_MUTEX, &thr_err);

    /* ---------------------------------- */
    /* Initialize MPI_COMM_SELF           */
    /* ---------------------------------- */
    MPIR_Process.comm_self->rank = 0;
    MPIR_Process.comm_self->remote_size = 1;
    MPIR_Process.comm_self->local_size = 1;

    /* ---------------------------------- */
    /* Initialize MPI_COMM_WORLD          */
    /* ---------------------------------- */
    MPIR_Process.comm_world->rank = rank;
    MPIR_Process.comm_world->remote_size = size;
    MPIR_Process.comm_world->local_size = size;

    MPIDIU_avt_init();
    MPIDIU_get_next_avtid(&avtid);
    MPIR_Assert(avtid == 0);

    MPIDI_av_table[0] = (MPIDI_av_table_t *)
        MPL_malloc(size * sizeof(MPIDI_av_entry_t)
                   + sizeof(MPIDI_av_table_t));

    MPIDI_av_table[0]->size = size;
    MPIR_Object_set_ref(MPIDI_av_table[0], 1);

    MPIDIU_alloc_globals_for_avtid(avtid);

    MPIDI_av_table0 = MPIDI_av_table[0];

    /* initialize rank_map */
    MPIDI_COMM(MPIR_Process.comm_world, map).mode = MPIDI_RANK_MAP_DIRECT_INTRA;
    MPIDI_COMM(MPIR_Process.comm_world, map).avtid = 0;
    MPIDI_COMM(MPIR_Process.comm_world, map).size = size;
    MPIDI_COMM(MPIR_Process.comm_world, local_map).mode = MPIDI_RANK_MAP_NONE;
    MPIDIU_avt_add_ref(0);

    MPIDI_COMM(MPIR_Process.comm_self, map).mode = MPIDI_RANK_MAP_OFFSET_INTRA;
    MPIDI_COMM(MPIR_Process.comm_self, map).avtid = 0;
    MPIDI_COMM(MPIR_Process.comm_self, map).size = 1;
    MPIDI_COMM(MPIR_Process.comm_self, map).reg.offset = rank;
    MPIDI_COMM(MPIR_Process.comm_self, local_map).mode = MPIDI_RANK_MAP_NONE;
    MPIDIU_avt_add_ref(0);

#ifdef MPL_USE_DBG_LOGGING
    int counter_;
    if (size < 16) {
        for (counter_ = 0; counter_ < size; ++counter_) {
            MPIDIU_comm_rank_to_av(MPIR_Process.comm_world, counter_);
        }
    }
#endif

    MPIR_Process.attrs.tag_ub = (1ULL << MPIDI_CH4U_TAG_SHIFT) - 1;
    /* discuss */

    /* Call any and all MPID_Init type functions */
    MPIR_Err_init();
    MPIR_Datatype_init();
    MPIR_Group_init();

#ifdef MPIDI_BUILD_CH4_LOCALITY_INFO
    int i;
    for (i = 0; i < MPIR_Process.comm_world->local_size; i++) {
        MPIDI_av_table0->table[i].is_local = 0;
    }
    mpi_errno = MPIDI_CH4U_build_nodemap(MPIR_Process.comm_world->rank,
                                         MPIR_Process.comm_world,
                                         MPIR_Process.comm_world->local_size,
                                         MPIDI_CH4_Global.node_map[0], &MPIDI_CH4_Global.max_node_id);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                    (MPL_DBG_FDEST, "MPIDI_CH4_Global.max_node_id = %d",
                     MPIDI_CH4_Global.max_node_id));

    for (i = 0; i < MPIR_Process.comm_world->local_size; i++) {
        MPIDI_av_table0->table[i].is_local =
            (MPIDI_CH4_Global.node_map[0][i] ==
             MPIDI_CH4_Global.node_map[0][MPIR_Process.comm_world->rank]) ? 1 : 0;
        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                        (MPL_DBG_FDEST, "WORLD RANK %d %s local", i,
                         MPIDI_av_table0->table[i].is_local ? "is" : "is not"));
        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                        (MPL_DBG_FDEST, "Node id (i) (me) %d %d", MPIDI_CH4_Global.node_map[0][i],
                         MPIDI_CH4_Global.node_map[0][MPIR_Process.comm_world->rank]));
    }
#endif

#ifdef MPIDI_BUILD_CH4_SHM
    mpi_errno = MPIDI_SHM_mpi_init_hook(rank, size, &n_shm_vnis_provided);

    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POPFATAL(mpi_errno);
    }
#endif

    mpi_errno = MPIDI_NM_mpi_init_hook(rank, size, appnum, &MPIR_Process.attrs.tag_ub,
                                       MPIR_Process.comm_world,
                                       MPIR_Process.comm_self, has_parent,
                                       &n_nm_vnis_provided);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POPFATAL(mpi_errno);
    }

    MPIR_Process.attrs.appnum = appnum;
    MPIR_Process.attrs.wtime_is_global = 1;
    MPIR_Process.attrs.io = MPI_ANY_SOURCE;

    MPIR_Comm_commit(MPIR_Process.comm_self);
    MPIR_Comm_commit(MPIR_Process.comm_world);

    /* -------------------------------- */
    /* Return MPICH Parameters          */
    /* -------------------------------- */
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

    *has_args = TRUE;
    *has_env = TRUE;
    MPIDI_CH4_Global.is_initialized = 0;

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_INIT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_InitCompleted
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPID_InitCompleted(void)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_INITCOMPLETED);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_INITCOMPLETED);
    MPIDI_CH4_Global.is_initialized = 1;
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_INITCOMPLETED);
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPID_Finalize
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPID_Finalize(void)
{
    int mpi_errno, thr_err;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_FINALIZE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_FINALIZE);

    mpi_errno = MPIDI_NM_mpi_finalize_hook();
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);
#ifdef MPIDI_BUILD_CH4_SHM
    mpi_errno = MPIDI_SHM_mpi_finalize_hook();
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);
#endif

    int i;
    int max_n_avts;
    max_n_avts = MPIDIU_get_max_n_avts();
    for (i = 0; i < max_n_avts; i++) {
        if (MPIDI_av_table[i] != NULL) {
            MPIDIU_avt_release_ref(i);
        }
    }

    MPIDIU_avt_destroy();
    MPL_free(MPIDI_CH4_Global.jobid);

    MPID_Thread_mutex_destroy(&MPIDI_CH4I_THREAD_PROGRESS_MUTEX, &thr_err);
    MPID_Thread_mutex_destroy(&MPIDI_CH4I_THREAD_PROGRESS_HOOK_MUTEX, &thr_err);
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_FINALIZE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_Get_universe_size
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPID_Get_universe_size(int *universe_size)
{
    int mpi_errno = MPI_SUCCESS;
    int pmi_errno = PMI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_GET_UNIVERSE_SIZE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_GET_UNIVERSE_SIZE);


    pmi_errno = PMI_Get_universe_size(universe_size);

    if (pmi_errno != PMI_SUCCESS)
        MPIR_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER,
                             "**pmi_get_universe_size", "**pmi_get_universe_size %d", pmi_errno);

    if (*universe_size < 0)
        *universe_size = MPIR_UNIVERSE_SIZE_NOT_AVAILABLE;

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_GET_UNIVERSE_SIZE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_Get_processor_name
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPID_Get_processor_name(char *name, int namelen, int *resultlen)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_GET_PROCESSOR_NAME);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_GET_PROCESSOR_NAME);

    if (!MPIDI_CH4_Global.pname_set) {
#ifdef HAVE_GETHOSTNAME

        if (gethostname(MPIDI_CH4_Global.pname, MPI_MAX_PROCESSOR_NAME) == 0)
            MPIDI_CH4_Global.pname_len = (int) strlen(MPIDI_CH4_Global.pname);

#elif defined(HAVE_SYSINFO)

        if (sysinfo(SI_HOSTNAME, MPIDI_CH4_Global.pname, MPI_MAX_PROCESSOR_NAME) == 0)
            MPIDI_CH4_Global.pname_len = (int) strlen(MPIDI_CH4_Global.pname);

#else
        MPL_snprintf(MPIDI_CH4_Global.pname, MPI_MAX_PROCESSOR_NAME, "%d",
                     MPIR_Process.comm_world->rank);
        MPIDI_CH4_Global.pname_len = (int) strlen(MPIDI_CH4_Global.pname);
#endif
        MPIDI_CH4_Global.pname_set = 1;
    }

    MPIR_ERR_CHKANDJUMP(MPIDI_CH4_Global.pname_len <= 0,
                        mpi_errno, MPI_ERR_OTHER, "**procnamefailed");
    MPL_strncpy(name, MPIDI_CH4_Global.pname, namelen);

    if (resultlen)
        *resultlen = MPIDI_CH4_Global.pname_len;

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_GET_PROCESSOR_NAME);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_Alloc_mem
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX void *MPID_Alloc_mem(size_t size, MPIR_Info * info_ptr)
{
    void *p;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_ALLOC_MEM);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_ALLOC_MEM);

    p = MPIDI_NM_mpi_alloc_mem(size, info_ptr);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_ALLOC_MEM);
    return p;
}

#undef FUNCNAME
#define FUNCNAME MPID_Free_mem
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPID_Free_mem(void *ptr)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_FREE_MEM);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_FREE_MEM);
    mpi_errno = MPIDI_NM_mpi_free_mem(ptr);

    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_FREE_MEM);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_Comm_get_lpid
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPID_Comm_get_lpid(MPIR_Comm * comm_ptr,
                                                 int idx, int *lpid_ptr, MPL_bool is_remote)
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

#undef FUNCNAME
#define FUNCNAME MPID_Get_node_id
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPID_Get_node_id(MPIR_Comm * comm, int rank, MPID_Node_id_t * id_p)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_GET_NODE_ID);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_GET_NODE_ID);

    MPIDI_CH4U_get_node_id(comm, rank, id_p);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_GET_NODE_ID);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPID_Get_max_node_id
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPID_Get_max_node_id(MPIR_Comm * comm, MPID_Node_id_t * max_id_p)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_GET_MAX_NODE_ID);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_GET_MAX_NODE_ID);

    MPIDI_CH4U_get_max_node_id(comm, max_id_p);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_GET_MAX_NODE_ID);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPID_Create_intercomm_from_lpids
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPID_Create_intercomm_from_lpids(MPIR_Comm * newcomm_ptr,
                                                               int size, const int lpids[])
{
    int mpi_errno = MPI_SUCCESS, i;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_CREATE_INTERCOMM_FROM_LPIDS);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_CREATE_INTERCOMM_FROM_LPIDS);

    MPIDI_rank_map_mlut_t *mlut = NULL;
    MPIDI_COMM(newcomm_ptr, map).mode = MPIDI_RANK_MAP_MLUT;
    MPIDI_COMM(newcomm_ptr, map).avtid = -1;
    mpi_errno = MPIDIU_alloc_mlut(&mlut, size);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
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


#undef FUNCNAME
#define FUNCNAME MPID_Aint_add
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX MPI_Aint MPID_Aint_add(MPI_Aint base, MPI_Aint disp)
{
    MPI_Aint result;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_AINT_ADD);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_AINT_ADD);
    result = MPIR_VOID_PTR_CAST_TO_MPI_AINT((char *) MPIR_AINT_CAST_TO_VOID_PTR(base) + disp);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_AINT_ADD);
    return result;
}

#undef FUNCNAME
#define FUNCNAME MPID_Aint_diff
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX MPI_Aint MPID_Aint_diff(MPI_Aint addr1, MPI_Aint addr2)
{
    MPI_Aint result;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_AINT_DIFF);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_AINT_DIFF);

    result = MPIR_PTR_DISP_CAST_TO_MPI_AINT((char *) MPIR_AINT_CAST_TO_VOID_PTR(addr1)
                                            - (char *) MPIR_AINT_CAST_TO_VOID_PTR(addr2));
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_AINT_DIFF);
    return result;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_Type_commit_hook
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_Type_commit_hook(MPIR_Datatype * type)
{
    int mpi_errno;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_TYPE_CREATE_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_TYPE_CREATE_HOOK);

    mpi_errno = MPIDI_NM_mpi_type_commit_hook(type);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }

#ifdef MPIDI_BUILD_CH4_SHM
    mpi_errno = MPIDI_SHM_mpi_type_commit_hook(type);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
#endif

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_TYPE_CREATE_HOOK);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_Type_free_hook
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_Type_free_hook(MPIR_Datatype * type)
{
    int mpi_errno;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_TYPE_FREE_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_TYPE_FREE_HOOK);

    mpi_errno = MPIDI_NM_mpi_type_free_hook(type);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }

#ifdef MPIDI_BUILD_CH4_SHM
    mpi_errno = MPIDI_SHM_mpi_type_free_hook(type);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
#endif

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_TYPE_FREE_HOOK);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_Op_commit_hook
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_Op_commit_hook(MPIR_Op * op)
{
    int mpi_errno;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OP_CREATE_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OP_CREATE_HOOK);

    mpi_errno = MPIDI_NM_mpi_op_commit_hook(op);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }

#ifdef MPIDI_BUILD_CH4_SHM
    mpi_errno = MPIDI_SHM_mpi_op_commit_hook(op);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
#endif

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OP_CREATE_HOOK);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_Op_free_hook
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_Op_free_hook(MPIR_Op * op)
{
    int mpi_errno;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OP_FREE_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OP_FREE_HOOK);

    mpi_errno = MPIDI_NM_mpi_op_free_hook(op);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }

#ifdef MPIDI_BUILD_CH4_SHM
    mpi_errno = MPIDI_SHM_mpi_op_free_hook(op);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
#endif

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OP_FREE_HOOK);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif /* CH4_INIT_H_INCLUDED */
