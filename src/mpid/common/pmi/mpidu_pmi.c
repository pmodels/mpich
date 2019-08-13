/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2019 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

#if !defined USE_PMI1_API && !defined USE_PMI2_API && !defined USE_PMIX_API
#define USE_PMI1_API
#endif

typedef enum PMI_DOMAIN_t {
    PMI_DOMAIN_ALL = 0;
    PMI_DOMAIN_NODE_ROOTS = 1;
} PMI_DOMAIN;

static char* pmi_kvs_name;
static int pmi_key_size, pmi_val_size;

static int MPIDU_pmi_build_node_map();

/**** init/finalize ******************************/

/* rank, size, kvs_name */
int MPIDU_pmi_init(void)
{
    int mpi_errno = MPI_SUCCESS;
    int pmi_errno;
    int rank, size, appnum;
    char *kvsname = NULL;

#if defined(USE_PMI1_API)
    pmi_errno = PMI_Init(&has_parent);
    MPIR_ERR_CHKANDJUMP1(pmi_errno, mpi_errno, MPI_ERR_OTHER, "**pmi_init",
                         "**pmi_init %d", pmi_errno);

    pmi_errno = PMI_Get_rank(&rank);
    MPIR_ERR_CHKANDJUMP1(pmi_errno, mpi_errno, MPI_ERR_OTHER, "**pmi_get_rank",
                         "**pmi_get_rank %d", pmi_errno);

    pmi_errno = PMI_Get_size(&size);
    MPIR_ERR_CHKANDJUMP1(pmi_errno, mpi_errno, MPI_ERR_OTHER, "**pmi_get_size",
                         "**pmi_get_size %d", pmi_errno);
    }

    pmi_errno = PMI_Get_appnum(&appnum);
    MPIR_ERR_CHKANDJUMP1(pmi_errno, mpi_errno, MPI_ERR_OTHER, "**pmi_get_appnum",
                         "**pmi_get_appnum %d", pmi_errno);

    int max_pmi_name_length;
    pmi_errno = PMI_KVS_Get_name_length_max(&max_pmi_name_length);
    MPIR_ERR_SETANDJUMP1(pmi_errno, mpi_errno, MPI_ERR_OTHER, "**pmi_kvs_get_name_length_max",
                         "**pmi_kvs_get_name_length_max %d", pmi_errno);

    kvs_name = (char *) MPL_malloc(max_pmi_name_length, MPL_MEM_OTHER);
    pmi_errno = PMI_KVS_Get_my_name(kvs_name, max_pmi_name_length);
    MPIR_ERR_CHKANDJUMP1(pmi_errno, mpi_errno, MPI_ERR_OTHER, "**pmi_kvs_get_my_name",
                         "**pmi_kvs_get_my_name %d", pmi_errno);

    pmi_errno = PMI_KVS_Get_key_length_max(&pmi_key_size);
    MPIR_ERR_CHKANDJUMP1(pmi_errno, mpi_errno, MPI_ERR_OTHER, "**fail", "**fail %d", pmi_errno);

    pmi_errno = PMI_KVS_Get_value_length_max(&pmi_val_size);
    MPIR_ERR_CHKANDJUMP1(pmi_errno, mpi_errno, MPI_ERR_OTHER, "**fail", "**fail %d", pmi_errno);

#elif defined(USE_PMI2_API)
    pmi_errno = PMI2_Init(&has_parent, &size, &rank, &appnum);
    MPIR_ERR_CHKANDJUMP1(pmi_errno, mpi_errno, MPI_ERR_OTHER, "**pmi_init",
                         "**pmi_init %d", pmi_errno);

    kvs_name = (char *) MPL_malloc(PMI2_MAX_VALLEN, MPL_MEM_OTHER);
    pmi_errno = PMI2_Job_GetId(kvs_name, PMI2_MAX_VALLEN);
    MPIR_ERR_CHKANDJUMP1(pmi_errno, mpi_errno, MPI_ERR_OTHER, "**pmi_job_getid",
                         "**pmi_job_getid %d", pmi_errno);

    pmi_val_size = PMI2_MAX_VALLEN;

#elif defined(USE_PMIX_API)
    pmix_value_t *pvalue = NULL;

    int pmi_errno = PMIx_Init(&MPIR_Process.pmix_proc, NULL, 0);
    if (pmi_errno != PMIX_SUCCESS) {
        MPIR_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER, "**pmix_init", "**pmix_init %d",
                                pmi_errno);
    }
    rank = MPIR_Process.pmix_proc.rank;

    PMIX_PROC_CONSTRUCT(&MPIR_Process.pmix_wcproc);
    MPL_strncpy(MPIR_Process.pmix_wcproc.nspace, MPIR_Process.pmix_proc.nspace, PMIX_MAX_NSLEN);
    MPIR_Process.pmix_wcproc.rank = PMIX_RANK_WILDCARD;

    pmi_errno = PMIx_Get(&MPIR_Process.pmix_wcproc, PMIX_JOB_SIZE, NULL, 0, &pvalue);
    if (pmi_errno != PMIX_SUCCESS) {
        MPIR_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER, "**pmix_get", "**pmix_get %d",
                                pmi_errno);
    }
    size = pvalue->data.uint32;
    PMIX_VALUE_RELEASE(pvalue);

    /* appnum, has_parent is not set for now */
    appnum = 0;
    has_parent = 0;
#endif

    MPIR_Process.rank = rank;
    MPIR_Process.size = size;
    MPIR_Process.attrs.appnum = appnum;
    /* MPIDI_global.jobid = kvs_name; */
    pmi_kvs_name = kvs_name;

    MPIDU_pmi_build_node_map();
  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

void MPIDU_pmi_finalize(void)
{
#if defined USE_PMI1_API
    PMI_Finalize();
#elif defined USE_PMI2_API
    PMI2_Finalize();
#elif defined USE_PMIX_API
    PMIx_Finalize(NULL, 0);
#endif
    if (pmi_kvs_name)
        MPL_free(pmi_kvs_name);
}

/**** pmi allgather *************/

static int pmi_algather_index;
int MPIDU_pmi_allgather(const void *sendbuf, int bufsize, void *recvbuf, PMI_DOMAIN domain)
{
    int mpi_errno = MPI_SUCCESS;
    int rc, pmi_errno;

    int myrank = MPIR_Process.rank;
    int sz = MPIR_Process.size;
    int *node_map = MPIR_Process.node_map;

    int local_size, local_rank, local_leader;
    MPIR_NODEMAP_get_local_info(myrank, sz, node_map, &local_size, &local_rank, &local_leader);

    if (domain == PMI_DOMAIN_NODE_ROOTS && myrank != local_leader) {
        /* not a node root, just participate in the barrier, unless we have pmi root-only barrier */
        mpi_errno = MPIDU_pmi_barrier();
        MPIR_ERR_CHECK(mpi_errno);
        goto fn_exit;
    }

    if (domain == PMI_DOMAIN_NODE_ROOTS) {
        sz = MPIR_Process.max_node_id + 1;
    }

    /**** Prepare key/val **********/
    char key[50];
    pmi_algather_index++;
#if !defined USE_PMIX_API
    if (domain == PMI_DOMAIN_ALL) {
        sprintf(key, "allgather-%d-%d", pmi_algather_index, myrank);
    } else if (domain == PMI_DOMAIN_NODE_ROOTS) {
        sprintf(key, "allgather-%d-%d", pmi_algather_index, node_map[myrank]);
    }
#else
    sprintf(key, "allgather-%d", pmi_algather_index);
#endif 

    char *val = MPL_malloc(pmi_val_size, MPL_MEM_ADDRESS);
    memset(val, 0, pmi_val_size);
    char *val_p = val;
    int rem = pmi_val_size;
    rc = MPL_str_add_binary_arg(&val_p, &rem, "mpi", (char *) sendbuf, bufsize);
    MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_OTHER, "**buscard");
    MPIR_Assert(rem >= 0);

    /**** Put key/val **********/
#if defined USE_PMI1_API
    pmi_errno = PMI_KVS_Put(pmi_kvs_name, key, sendbuf);
    MPIR_ERR_CHKANDJUMP(pmi_errno, mpi_errno, MPI_ERR_OTHER, "**pmi_kvs_put");
    pmi_errno = PMI_KVS_Commit(pmi_kvs_name);
    MPIR_ERR_CHKANDJUMP(pmi_errno, mpi_errno, MPI_ERR_OTHER, "**pmi_kvs_commit");

#elif defined USE_PMI2_API
    pmi_errno = PMI2_KVS_Put(key, val);
    MPIR_ERR_CHKANDJUMP(pmi_errno, mpi_errno, MPI_ERR_OTHER, "**pmi_kvsput");

#elif defined USE_PMIX_API
    pmix_value_t value, *pvalue;
    pmix_info_t *info;
    pmix_proc_t proc;

    value.type = PMIX_STRING;
    value.data.string = val;
    pmi_errno = PMIx_Put(PMIX_LOCAL, "");
#endif

    /**** Barrier **********/
    mpi_errno = MPIDU_pmi_barrier();
    MPIR_ERR_CHECK(mpi_errno);

    /**** Retrieve key/val **********/
    int i;
    for (i = 0; i < sz; i++) {
#if defined USE_PMI1_API
        sprintf(key, "allgather-%d-%d", pmi_algather_index, i);
        pmi_errno = PMI_KVS_Get(pmi_kvs_name, key, val, pmi_val_size);
        MPIR_ERR_CHKANDJUMP(pmi_errno, mpi_errno, MPI_ERR_OTHER, "**pmi_kvs_get");
        int out_len;
        rc = MPL_str_get_binary_arg(val, "mpi", (char *)recvbuf + (i * bufsize), bufsize, &out_len);
        MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_OTHER, "**argstr_missinghost");
#elif defined USE_PMI2_API
        sprintf(key, "allgather-%d-%d", pmi_algather_index, i);
        int out_len;
        pmi_errno = PMI2_KVS_Get(NULL, -1, key, val, PMI2_MAX_VALLEN, &out_len);
        MPIR_ERR_CHKANDJUMP(pmi_errno, mpi_errno, MPI_ERR_OTHER, "**pmi_kvsget");
        rc = MPL_str_get_binary_arg(val, "mpi", (char *)recvbuf + (i * bufsize), bufsize, &out_len);
        MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_OTHER, "**argstr_missinghost");
#elif defined USE_PMIX_API
        pmix_proc_t proc;
        PMIX_PROC_CONSTRUCT(&proc);
        if (domain == PMI_DOMAIN_ALL) {
            proc.rank = i;
        } else if (domain == PMI_DOMAIN_NODE_ROOTS) {
            proc.rank = MPIR_Process.node_leader_map[i];
            MPIR_Assert(proc.rank >= 0);
        }
        pmi_errno = PMIx_Get(&proc, key, NULL, 0, &pvalue);
        MPIR_ERR_CHKANDJUMP(pmi_errno, mpi_errno, MPI_ERR_OTHER, "**pmix_get");
        rc = MPL_str_get_binary_arg(pval->data.string, "mpi", (char *)recvbuf + (i * bufsize), bufsize, &out_len);
        MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_OTHER, "**argstr_missinghost");
        PMIX_VALUE_RELEASE(pvalue);
#endif
    }

  fn_exit:
    MPL_free(val);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/**** run-time PMI utilities *************/

int MPIDU_pmi_get_universe_size(int *universe_size)
{
    int mpi_errno = MPI_SUCCESS;
    int pmi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_PMI_GET_UNIVERSE_SIZE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_PMI_GET_UNIVERSE_SIZE);

#ifdef USE_PMIX_API
    {
        pmix_value_t *pvalue = NULL;

        pmi_errno = PMIx_Get(&MPIR_Process.pmix_wcproc, PMIX_UNIV_SIZE, NULL, 0, &pvalue);
        if (pmi_errno != PMIX_SUCCESS)
            MPIR_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER,
                                 "**pmix_get", "**pmix_get %d", pmi_errno);
        *universe_size = pvalue->data.uint32;
        PMIX_VALUE_RELEASE(pvalue);
    }
#elif defined(USE_PMI2_API)
    {
        char val[PMI2_MAX_VALLEN];
        int found = 0;
        char *endptr;

        pmi_errno = PMI2_Info_GetJobAttr("universeSize", val, sizeof(val), &found);
        if (pmi_errno)
            MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**pmi_getjobattr");

        if (!found) {
            *universe_size = MPIR_UNIVERSE_SIZE_NOT_AVAILABLE;
        } else {
            *universe_size = strtol(val, &endptr, 0);
            MPIR_ERR_CHKINTERNAL(endptr - val != strlen(val), mpi_errno,
                                 "can't parse universe size");
        }
    }
#else
    pmi_errno = PMI_Get_universe_size(universe_size);

    if (pmi_errno)
        MPIR_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER,
                             "**pmi_get_universe_size", "**pmi_get_universe_size %d", pmi_errno);
#endif

    if (*universe_size < 0)
        *universe_size = MPIR_UNIVERSE_SIZE_NOT_AVAILABLE;

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_PMI_GET_UNIVERSE_SIZE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

void MPIDU_pmi_check_for_failed_procs(char **failed_procs_string_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    int pmi_errno;
    int len;
    char *kvsname = MPIDI_global.jobid;
    /* FIXME: Currently this only handles failed processes in
     * comm_world.  We need to fix hydra to include the pgid along
     * with the rank, then we need to create the failed group from
     * something bigger than comm_world. */
    *failed_procs_string_ptr = NULL;
#if defined USE_PMI1_API
    pmi_errno = PMI_KVS_Get_value_length_max(&len);
    MPIR_ERR_CHKANDJUMP(pmi_errno, mpi_errno, MPI_ERR_OTHER, "**pmi_kvs_get_value_length_max");
    *failed_procs_string_ptr = MPL_malloc(len, MPL_MEM_OTHER);
    MPIR_Assert(*failed_procs_string_ptr);
    pmi_errno = PMI_KVS_Get(kvsname, "PMI_dead_processes", *failed_procs_string_ptr, len);
    MPIR_ERR_CHKANDJUMP(pmi_errno, mpi_errno, MPI_ERR_OTHER, "**pmi_kvs_get");
#elif defined(USE_PMI2_API)
    int vallen = 0;
    len = PMI2_MAX_VALLEN;
    *failed_procs_string_ptr = MPL_malloc(len, MPL_MEM_OTHER);
    MPIR_Assert(*failed_procs_string_ptr);
    pmi_errno = PMI2_KVS_Get(kvsname, PMI2_ID_NULL, "PMI_dead_processes", *failed_procs_string_ptr,
                             len, &vallen);
    MPIR_ERR_CHKANDJUMP(pmi_errno, mpi_errno, MPI_ERR_OTHER, "**pmi_kvs_get");
#elif defined USE_PMIX_API
    MPIR_Assert(0);
#endif
  fn_exit:
    return mpi_errno;
  fn_fail:
    if (*failed_procs_string_ptr) {
        free(*failed_procs_string_ptr);
        *failed_procs_string_ptr = NULL;
    }
    goto fn_exit;
}

#if defined USE_PMI1_API
/* this function is not used in pmi2 or pmix */
static int pmi_build_nodemap_fallback(int *out_nodemap, int *out_max_node_id)
{
    mpi_errno = MPI_SUCCESS;
    int i, j;

    char hostname[MAX_HOSTNAME_LEN];
    ret = gethostname(hostname, MAX_HOSTNAME_LEN);
    MPIR_ERR_CHKANDJUMP2(ret == -1, mpi_errno, MPI_ERR_OTHER, "**sock_gethost",
                         "**sock_gethost %s %d", MPIR_Strerror(errno), errno);
    hostname[MAX_HOSTNAME_LEN - 1] = '\0';

    int sz = MPIR_Process.size;
    char *node_name_buf = MPL_malloc(MAX_HOSTNAME_LEN * sz, MPL_MEM_STRINGS);
    char **node_names = MPL_malloc(sizeof(char *) * sz, MPL_MEM_STRINGS);
    for (i = 0; i < sz; ++i) {
        node_names[i] = &node_name_buf[i * MAX_HOSTNAME_LEN];
        node_names[i][0] = '\0';
    }

    mpi_errno = MPIDU_pmi_allgather(hostname, MAX_HOSTNAME_LEN, node_name_buf, PMI_DOMAIN_ALL);
    MPIR_ERR_CHECK(mpi_errno);

    int max_node_id = -1;
    for (i = 0; i < sz; ++i) {
        MPIR_Assert(max_node_id < sz);
        for (j = 0; j <= max_node_id; ++j)
            if (!MPL_strncmp(node_names[j], node_names[max_node_id + 1], MAX_HOSTNAME_LEN))
                break;
        if (j == max_node_id + 1)
            ++max_node_id;
        else
            node_names[max_node_id + 1][0] = '\0';
        out_nodemap[i] = j;
    }

    *out_max_node_id = max_node_id;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif

/**** PMI wrappers **********/

void MPIDU_pmi_abort(int exit_code, const char *error_msg)
{
#if defined USE_PMI1_API
    PMI_Abort(exit_code, error_msg);
#elif defined(USE_PMI2_API)
    PMI2_Abort(TRUE, error_msg);
#elif defined USE_PMIX_API
    PMIx_Abort(exit_code, error_msg, NULL, 0);
#endif
}

int MPIDU_pmi_barrier()
{
    int mpi_errno = MPI_SUCCESS;

#if defined USE_PMI1_API
    int pmi_errno = PMI_Barrier();
    if (pmi_errno != PMI2_SUCCESS) {
        MPIR_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER, "**pmi_barrier", "**pmi_barrier %d");
    }
#elif defined(USE_PMI2_API)
    int pmi_errno = PMI2_KVS_Fence();
    if (pmi_errno != PMI2_SUCCESS) {
        MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**pmi_kvsfence");
    }
#elif defined USE_PMIX_API
    pmix_value_t value;
    pmix_info_t *info;
    int flag = 1;

    PMIX_INFO_CREATE(info, 1);
    PMIX_INFO_LOAD(info, PMIX_COLLECT_DATA, &flag, PMIX_BOOL);
    int pmi_errno = PMIx_Fence(&MPIR_Process.pmix_wcproc, 1, info, 1);
    if (pmi_errno != PMIX_SUCCESS) {
        MPIR_ERR_SETANDJUMP1(pmi_errno, MPI_ERR_OTHER, "**pmix_fence", "**pmix_fence %d",
                             pmi_errno);
    }
    PMIX_INFO_FREE(info, 1);
#endif

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/**** static functions **********/

static void build_node_leader_map(int *node_map, int *node_leader_map, int num_ranks, int num_nodes);

static int MPIDU_pmi_build_node_map()
{
    int myrank = MPIR_Process.rank;
    int sz = MPIR_Process.size;
    int *out_nodemap = MPL_malloc(sizeof(int) * sz, MPL_MEM_GROUP);
    int max_node_id = 0;

    /* See if the user wants to override our default values */
    MPL_env2int("PMI_VERSION", &pmi_version);
    MPL_env2int("PMI_SUBVERSION", &pmi_subversion);

#if defined USE_PMI1_API
    if (myrank == -1) {
        MPIR_Assert(0);
    }

    /* See if process manager supports PMI_process_mapping keyval */
    if (pmi_version == 1 && pmi_subversion == 1) {
        char *process_mapping = MPL_malloc(pmi_val_size, MPL_MEM_STRINGS);
        pmi_errno = PMI_KVS_Get(kvs_name, "PMI_process_mapping", process_mapping, pmi_val_size);
        if (pmi_errno == PMI_SUCCESS) {
            mpi_errno = MPIR_NODEMAP_populate_ids_from_mapping(process_mapping, sz, out_nodemap,
                                                   &max_node_id, &did_map);
            MPIR_ERR_CHECK(mpi_errno);
            MPIR_ERR_CHKINTERNAL(!did_map, mpi_errno,
                                 "unable to populate node ids from PMI_process_mapping");
        } else {
            mpi_errno = pmi_build_nodemap_fallback(out_nodemap, &max_node_id);
            MPIR_ERR_CHECK(mpi_errno);
        }
        MPL_free(process_mapping);
    }
#elif defined USE_PMI2_API
    {
        char process_mapping[PMI2_MAX_VALLEN];
        int outlen;
        int found = FALSE;
        int i;
        MPIR_NODEMAP_map_block_t *mb;
        int nblocks;
        int rank;
        int block, block_node, node_proc;
        int did_map = 0;

        mpi_errno =
            PMI2_Info_GetJobAttr("PMI_process_mapping", process_mapping, sizeof(process_mapping),
                                 &found);
        MPIR_ERR_CHECK(mpi_errno);
        MPIR_ERR_CHKINTERNAL(!found, mpi_errno, "PMI_process_mapping attribute not found");
        /* this code currently assumes pg is comm_world */
        mpi_errno =
            MPIR_NODEMAP_populate_ids_from_mapping(process_mapping, sz, out_nodemap,
                                                   &max_node_id, &did_map);
        MPIR_ERR_CHECK(mpi_errno);
        MPIR_ERR_CHKINTERNAL(!did_map, mpi_errno,
                             "unable to populate node ids from PMI_process_mapping");
    }
#elif defined(USE_PMIX_API)
    {
        char *nodelist = NULL, *node = NULL;
        pmix_proc_t *procs = NULL;
        size_t nprocs, node_id = 0;
        int i;

        pmi_errno = PMIx_Resolve_nodes(MPIR_Process.pmix_proc.nspace, &nodelist);
        MPIR_ERR_CHKANDJUMP1(pmi_errno != PMIX_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                             "**pmix_resolve_nodes", "**pmix_resolve_nodes %d", pmi_errno);
        MPIR_Assert(nodelist);

        node = strtok(nodelist, ",");
        while (node) {
            pmi_errno = PMIx_Resolve_peers(node, MPIR_Process.pmix_proc.nspace, &procs, &nprocs);
            MPIR_ERR_CHKANDJUMP1(pmi_errno != PMIX_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                                 "**pmix_resolve_peers", "**pmix_resolve_peers %d", pmi_errno);
            for (i = 0; i < nprocs; i++) {
                out_nodemap[procs[i].rank] = node_id;
            }
            node_id++;
            node = strtok(NULL, ",");
        }
        max_node_id = node_id - 1;
        /* PMIx latest adds pmix_free. We should switch to that at some point */
        MPL_external_free(nodelist);
        PMIX_PROC_FREE(procs, nprocs);
    }
#endif
    
    MPIR_NODEMAP_update_cliques(sz, out_nodemap, &max_node_id);
    MPIR_Process.node_map = out_nodemap;
    MPIR_Process.max_node_id = max_node_id;
    int num_nodes = max_node_id + 1;
    MPIR_Process.node_leader_map = MPL_malloc(sizeof(int) * num_nodes, MPL_MEM_ADDRESS);
    build_node_leader_map(MPIR_Process.node_map, MPIR_Process.node_leader_map, sz, num_nodes);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static void build_node_leader_map(int *node_map, int *node_leader_map, int num_ranks, int num_nodes)
{
    int i, j;
    for (i = 0; i < num_nodes; i++) {
        MPIR_Process.node_leader_map[i] = -1;
        for (j = 0; j < sz; j++) {
            if (MPIR_Process.node_map[j] == i) {
                MPIR_Process.node_leader_map[i] = j;
                break;
            }
        }
    }
}
