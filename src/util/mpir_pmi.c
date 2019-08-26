/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2019 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <mpir_pmi.h>
#include <mpiimpl.h>
#include "mpir_nodemap.h"

static int build_nodemap(int *nodemap, int sz, int *p_max_node_id);
static int build_locality(void);

static int pmi_version = 1;
static int pmi_subversion = 1;

static int pmi_max_key_size;
static int pmi_max_val_size;

#ifdef USE_PMI1_API
static int pmi_max_kvs_name_length;
static char *pmi_kvs_name;
#elif defined USE_PMI2_API
static char *pmi_jobid;
#elif defined USE_PMIX_API
static pmix_proc_t pmix_proc;
static pmix_proc_t pmix_wcproc;
#endif

int MPIR_pmi_init(void)
{
    int mpi_errno = MPI_SUCCESS;
    int pmi_errno;

    /* See if the user wants to override our default values */
    MPL_env2int("PMI_VERSION", &pmi_version);
    MPL_env2int("PMI_SUBVERSION", &pmi_subversion);

    int has_parent, rank, size, appnum;
#ifdef USE_PMI1_API
    pmi_errno = PMI_Init(&has_parent);
    MPIR_ERR_CHKANDJUMP1(pmi_errno != PMI_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                         "**pmi_init", "**pmi_init %d", pmi_errno);
    pmi_errno = PMI_Get_rank(&rank);
    MPIR_ERR_CHKANDJUMP1(pmi_errno != PMI_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                         "**pmi_get_rank", "**pmi_get_rank %d", pmi_errno);
    pmi_errno = PMI_Get_size(&size);
    MPIR_ERR_CHKANDJUMP1(pmi_errno != PMI_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                         "**pmi_get_size", "**pmi_get_size %d", pmi_errno);
    pmi_errno = PMI_Get_appnum(&appnum);
    MPIR_ERR_CHKANDJUMP1(pmi_errno != PMI_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                         "**pmi_get_appnum", "**pmi_get_appnum %d", pmi_errno);

    pmi_errno = PMI_KVS_Get_name_length_max(&pmi_max_kvs_name_length);
    MPIR_ERR_CHKANDJUMP1(pmi_errno != PMI_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                         "**pmi_kvs_get_name_length_max",
                         "**pmi_kvs_get_name_length_max %d", pmi_errno);
    pmi_kvs_name = (char *) MPL_malloc(pmi_max_kvs_name_length, MPL_MEM_OTHER);
    pmi_errno = PMI_KVS_Get_my_name(pmi_kvs_name, pmi_max_kvs_name_length);
    MPIR_ERR_CHKANDJUMP1(pmi_errno != PMI_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                         "**pmi_kvs_get_my_name", "**pmi_kvs_get_my_name %d", pmi_errno);

    pmi_errno = PMI_KVS_Get_key_length_max(&pmi_max_key_size);
    MPIR_ERR_CHKANDJUMP1(pmi_errno != PMI_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                         "**pmi_kvs_get_key_length_max",
                         "**pmi_kvs_get_key_length_max %d", pmi_errno);
    pmi_errno = PMI_KVS_Get_value_length_max(&pmi_max_val_size);
    MPIR_ERR_CHKANDJUMP1(pmi_errno != PMI_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                         "**pmi_kvs_get_value_length_max",
                         "**pmi_kvs_get_value_length_max %d", pmi_errno);

#elif defined USE_PMI2_API
    pmi_max_key_size = PMI2_MAX_KEYLEN;
    pmi_max_val_size = PMI2_MAX_VALLEN;

    pmi_errno = PMI2_Init(&has_parent, &size, &rank, &appnum);
    MPIR_ERR_CHKANDJUMP1(pmi_errno != PMI2_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                         "**pmi_init", "**pmi_init %d", pmi_errno);

    pmi_jobid = (char *) MPL_malloc(PMI2_MAX_VALLEN, MPL_MEM_OTHER);
    pmi_errno = PMI2_Job_GetId(pmi_jobid, PMI2_MAX_VALLEN);
    MPIR_ERR_CHKANDJUMP1(pmi_errno != PMI2_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                         "**pmi_job_getid", "**pmi_job_getid %d", pmi_errno);

#elif defined USE_PMIX_API
    pmi_max_key_size = PMIX_MAX_KEYLEN;
    pmi_max_val_size = 1024;    /* this is what PMI2_MAX_VALLEN currently set to */

    pmix_value_t *pvalue = NULL;

    pmi_errno = PMIx_Init(&pmix_proc, NULL, 0);
    MPIR_ERR_CHKANDJUMP1(pmi_errno != PMIX_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                         "**pmix_init", "**pmix_init %d", pmi_errno);

    rank = pmix_proc.rank;
    PMIX_PROC_CONSTRUCT(&pmix_wcproc);
    MPL_strncpy(pmix_wcproc.nspace, pmix_proc.nspace, PMIX_MAX_NSLEN);
    pmix_wcproc.rank = PMIX_RANK_WILDCARD;

    pmi_errno = PMIx_Get(&pmix_wcproc, PMIX_JOB_SIZE, NULL, 0, &pvalue);
    MPIR_ERR_CHKANDJUMP1(pmi_errno != PMIX_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                         "**pmix_get", "**pmix_get %d", pmi_errno);
    size = pvalue->data.uint32;
    PMIX_VALUE_RELEASE(pvalue);

    /* appnum, has_parent is not set for now */
    appnum = 0;
    has_parent = 0;

    MPIR_Process.pmix_proc = pmix_proc;
    MPIR_Process.pmix_wcproc = pmix_wcproc;

#endif
    MPIR_Process.has_parent = has_parent;
    MPIR_Process.rank = rank;
    MPIR_Process.size = size;
    MPIR_Process.appnum = appnum;

    static int g_max_node_id = -1;
    MPIR_Process.node_map = (int *) MPL_malloc(size * sizeof(int), MPL_MEM_ADDRESS);

    mpi_errno = build_nodemap(MPIR_Process.node_map, size, &g_max_node_id);
    MPIR_ERR_CHECK(mpi_errno);
    MPIR_Process.num_nodes = g_max_node_id + 1;

    /* allocate and populate MPIR_Process.node_local_map and MPIR_Process.node_root_map */
    mpi_errno = build_locality();

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_pmi_max_val_size(void)
{
    return pmi_max_val_size;
}

void MPIR_pmi_finalize(void)
{
#ifdef USE_PMI1_API
    PMI_Finalize();
    MPL_free(pmi_kvs_name);
#elif defined(USE_PMI2_API)
    PMI2_Finalize();
    MPL_free(pmi_jobid);
#elif defined(USE_PMIX_API)
    PMIx_Finalize(NULL, 0);
    /* pmix_proc does not need free */
#endif

    MPL_free(MPIR_Process.node_map);
    MPL_free(MPIR_Process.node_root_map);
    MPL_free(MPIR_Process.node_local_map);
}

/* getters for internal constants */
int MPIR_pmi_max_val_size(void)
{
    return pmi_max_val_size;
}

const char *MPIR_pmi_job_id(void)
{
#ifdef USE_PMI1_API
    return (const char *) pmi_kvs_name;
#elif defined USE_PMI2_API
    return (const char *) pmi_jobid;
#elif defined USE_PMIX_API
    return (const char *) pmix_proc.nspace;
#endif
}

/* wrapper functions */
int MPIR_pmi_kvs_put(char *key, char *val)
{
    int mpi_errno = MPI_SUCCESS;
    int pmi_errno;

#ifdef USE_PMI1_API
    pmi_errno = PMI_KVS_Put(pmi_kvs_name, key, val);
    MPIR_ERR_CHKANDJUMP1(pmi_errno != PMI_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                         "**pmi_kvs_put", "**pmi_kvs_put %d", pmi_errno);
    pmi_errno = PMI_KVS_Commit(pmi_kvs_name);
    MPIR_ERR_CHKANDJUMP1(pmi_errno != PMI_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                         "**pmi_kvs_commit", "**pmi_kvs_commit %d", pmi_errno);
#elif defined(USE_PMI2_API)
    pmi_errno = PMI2_KVS_Put(key, val);
    MPIR_ERR_CHKANDJUMP1(pmi_errno != PMI2_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                         "**pmi_kvsput", "**pmi_kvsput %d", pmi_errno);
#elif defined(USE_PMIX_API)
    pmix_value_t value;
    value.type = PMIX_STRING;
    value.data.string = val;
    pmi_errno = PMIx_Put(PMIX_LOCAL, key, &value);
    MPIR_ERR_CHKANDJUMP1(pmi_errno != PMIX_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                         "**pmix_put", "**pmix_put %d", pmi_errno);
    pmi_errno = PMIx_Commit();
    MPIR_ERR_CHKANDJUMP1(pmi_errno != PMIX_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                         "**pmix_commit", "**pmix_commit %d", pmi_errno);
#endif

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* NOTE: src is a hint, use src = -1 if not known */
int MPIR_pmi_kvs_get(int src, char *key, char *val, int val_size)
{
    int mpi_errno = MPI_SUCCESS;
    int pmi_errno;

#ifdef USE_PMI1_API
    /* src is not used in PMI1 */
    pmi_errno = PMI_KVS_Get(pmi_kvs_name, key, val, val_size);
    MPIR_ERR_CHKANDJUMP1(pmi_errno != PMI_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                         "**pmi_kvs_get", "**pmi_kvs_get %d", pmi_errno);
#elif defined(USE_PMI2_API)
    if (src < 0)
        src = PMI2_ID_NULL;
    int out_len;
    pmi_errno = PMI2_KVS_Get(pmi_jobid, src, key, val, val_size, &out_len);
    MPIR_ERR_CHKANDJUMP1(pmi_errno != PMI2_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                         "**pmi_kvsget", "**pmi_kvsget %d", pmi_errno);
#elif defined(USE_PMIX_API)
    pmix_value_t *pvalue;
    if (src < 0) {
        pmi_errno = PMIx_Get(NULL, key, NULL, 0, &pvalue);
    } else {
        pmix_proc_t proc;
        PMIX_PROC_CONSTRUCT(&proc);
        proc.rank = src;

        pmi_errno = PMIx_Get(&proc, key, NULL, 0, &pvalue);
    }
    MPIR_ERR_CHKANDJUMP1(pmi_errno != PMIX_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                         "**pmix_get", "**pmix_get %d", pmi_errno);
    strncpy(val, pvalue->data.string, val_size);
    PMIX_VALUE_RELEASE(pvalue);
#endif

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* ---- utils functions ---- */

int MPIR_pmi_barrier(MPIR_PMI_DOMAIN domain)
{
    int mpi_errno = MPI_SUCCESS;
    int pmi_errno;

#ifdef USE_PMI1_API
    pmi_errno = PMI_Barrier();
    MPIR_ERR_CHKANDJUMP1(pmi_errno != PMI_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                         "**pmi_barrier", "**pmi_barrier %d", pmi_errno);
#elif defined(USE_PMI2_API)
    pmi_errno = PMI2_KVS_Fence();
    MPIR_ERR_CHKANDJUMP1(pmi_errno != PMI2_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                         "**pmi_kvsfence", "**pmi_kvsfence %d", pmi_errno);
#elif defined(USE_PMIX_API)
    pmix_info_t *info;
    PMIX_INFO_CREATE(info, 1);
    int flag = 1;
    PMIX_INFO_LOAD(info, PMIX_COLLECT_DATA, &flag, PMIX_BOOL);

    if (domain == MPIR_PMI_DOMAIN_LOCAL) {
        /* use local proc set */
        /* FIXME: we could simply construct the proc set from MPIR_Process.node_local_map */
        pmix_proc_t *procs;
        size_t nprocs;
        pmix_value_t *pvalue = NULL;

        pmi_errno = PMIx_Get(&pmix_proc, PMIX_HOSTNAME, NULL, 0, &pvalue);
        MPIR_ERR_CHKANDJUMP1(pmi_errno != PMIX_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**pmix_get",
                             "**pmix_get %d", pmi_errno);
        const char *nodename = (const char *) pvalue->data.string;

        pmi_errno = PMIx_Resolve_peers(nodename, pmix_proc.nspace, &procs, &nprocs);
        MPIR_ERR_CHKANDJUMP1(pmi_errno != PMIX_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                             "**pmix_resolve_peers", "**pmix_resolve_peers %d", pmi_errno);

        pmi_errno = PMIx_Fence(procs, nprocs, info, 1);
        MPIR_ERR_CHKANDJUMP1(pmi_errno != PMIX_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**pmix_fence",
                             "**pmix_fence %d", pmi_errno);

        PMIX_VALUE_RELEASE(pvalue);
        PMIX_PROC_FREE(procs, nprocs);

    } else {
        /* use global wildcard proc set */
        pmi_errno = PMIx_Fence(&pmix_wcproc, 1, info, 1);
        MPIR_ERR_CHKANDJUMP1(pmi_errno != PMIX_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                             "**pmix_fence", "**pmix_fence %d", pmi_errno);
    }
    PMIX_INFO_FREE(info, 1);
#endif

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* ---- static functions ---- */

/* The following static function declares are only for build_nodemap() */
static int get_option_no_local(void);
static int get_option_num_cliques(void);
static int build_nodemap_nolocal(int *nodemap, int sz, int *p_max_node_id);
static int build_nodemap_roundrobin(int num_cliques, int *nodemap, int sz, int *p_max_node_id);

#ifdef USE_PMI1_API
static int build_nodemap_pmi1(int *nodemap, int sz, int *p_max_node_id);
static int build_nodemap_fallback(int *nodemap, int sz, int *p_max_node_id);
#elif defined(USE_PMI2_API)
static int build_nodemap_pmi2(int *nodemap, int sz, int *p_max_node_id);
#elif defined(USE_PMIX_API)
static int build_nodemap_pmix(int *nodemap, int sz, int *p_max_node_id);
#endif

static int build_nodemap(int *nodemap, int sz, int *p_max_node_id)
{
    int mpi_errno = MPI_SUCCESS;

    if (sz == 1 || get_option_no_local()) {
        mpi_errno = build_nodemap_nolocal(nodemap, sz, p_max_node_id);
        goto fn_exit;
    }
#ifdef USE_PMI1_API
    mpi_errno = build_nodemap_pmi1(nodemap, sz, p_max_node_id);
#elif defined(USE_PMI2_API)
    mpi_errno = build_nodemap_pmi2(nodemap, sz, p_max_node_id);
#elif defined(USE_PMIX_API)
    mpi_errno = build_nodemap_pmix(nodemap, sz, p_max_node_id);
#endif
    MPIR_ERR_CHECK(mpi_errno);

    int num_cliques = get_option_num_cliques();
    if (num_cliques > sz) {
        num_cliques = sz;
    }
    if (*p_max_node_id == 0 && num_cliques > 1) {
        mpi_errno = build_nodemap_roundrobin(num_cliques, nodemap, sz, p_max_node_id);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int get_option_no_local(void)
{
    /* Used for debugging only.  This disables communication over shared memory */
#ifdef ENABLED_NO_LOCAL
    return 1;
#else
    return MPIR_CVAR_NOLOCAL;
#endif
}

static int get_option_num_cliques(void)
{
    /* Used for debugging on a single machine: split procs into num_cliques nodes.
     * If ODD_EVEN_CLIQUES were enabled, split procs into 2 nodes.
     */
    if (MPIR_CVAR_NUM_CLIQUES > 1) {
        return MPIR_CVAR_NUM_CLIQUES;
    } else {
#ifdef ENABLED_ODD_EVEN_CLIQUES
        return 2;
#else
        return MPIR_CVAR_ODD_EVEN_CLIQUES ? 2 : 1;
#endif
    }
}

/* one process per node */
int build_nodemap_nolocal(int *nodemap, int sz, int *p_max_node_id)
{
    int i;
    for (i = 0; i < sz; ++i) {
        nodemap[i] = i;
    }
    *p_max_node_id = sz - 1;
    return MPI_SUCCESS;
}

/* assign processes to num_cliques nodes in a round-robin fashion */
static int build_nodemap_roundrobin(int num_cliques, int *nodemap, int sz, int *p_max_node_id)
{
    int i;
    for (i = 0; i < sz; ++i) {
        nodemap[i] = i % num_cliques;
    }
    *p_max_node_id = num_cliques - 1;
    return MPI_SUCCESS;
}

#ifdef USE_PMI1_API

/* build nodemap based on allgather hostnames */
/* FIXME: migrate the function */
static int build_nodemap_fallback(int *nodemap, int sz, int *p_max_node_id)
{
    return MPIR_NODEMAP_build_nodemap(sz, MPIR_Process.rank, nodemap, p_max_node_id);
}

/* build nodemap using PMI1 process_mapping or fallback with hostnames */
static int build_nodemap_pmi1(int *nodemap, int sz, int *p_max_node_id)
{
    int mpi_errno = MPI_SUCCESS;
    int pmi_errno;
    int did_map = 0;
    if (pmi_version == 1 && pmi_subversion == 1) {
        char *process_mapping = MPL_malloc(pmi_max_val_size, MPL_MEM_ADDRESS);
        pmi_errno = PMI_KVS_Get(pmi_kvs_name, "PMI_process_mapping",
                                process_mapping, pmi_max_val_size);
        if (pmi_errno == PMI_SUCCESS) {
            mpi_errno = MPIR_NODEMAP_populate_ids_from_mapping(process_mapping, sz, nodemap,
                                                               p_max_node_id, &did_map);
            MPIR_ERR_CHECK(mpi_errno);
            MPIR_ERR_CHKINTERNAL(!did_map, mpi_errno,
                                 "unable to populate node ids from PMI_process_mapping");
        }
        MPL_free(process_mapping);
    }
    if (!did_map) {
        mpi_errno = build_nodemap_fallback(nodemap, sz, p_max_node_id);
    }
  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#elif defined USE_PMI2_API

/* build nodemap using PMI2 process_mapping or error */
static int build_nodemap_pmi2(int *nodemap, int sz, int *p_max_node_id)
{
    int mpi_errno = MPI_SUCCESS;
    int pmi_errno;
    char process_mapping[PMI2_MAX_VALLEN];
    int found;

    pmi_errno = PMI2_Info_GetJobAttr("PMI_process_mapping", process_mapping, PMI2_MAX_VALLEN,
                                     &found);
    MPIR_ERR_CHKANDJUMP1(pmi_errno != PMI2_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                         "**pmi2_info_getjobattr", "**pmi2_info_getjobattr %d", pmi_errno);
    MPIR_ERR_CHKINTERNAL(!found, mpi_errno, "PMI_process_mapping attribute not found");

    int did_map;
    mpi_errno = MPIR_NODEMAP_populate_ids_from_mapping(process_mapping, sz, nodemap,
                                                       p_max_node_id, &did_map);
    MPIR_ERR_CHECK(mpi_errno);
    MPIR_ERR_CHKINTERNAL(!did_map, mpi_errno,
                         "unable to populate node ids from PMI_process_mapping");
  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#elif defined USE_PMIX_API

/* build nodemap using PMIx_Resolve_nodes */
int build_nodemap_pmix(int *nodemap, int sz, int *p_max_node_id)
{
    int mpi_errno = MPI_SUCCESS;
    int pmi_errno;
    char *nodelist = NULL, *node = NULL;
    pmix_proc_t *procs = NULL;
    size_t nprocs, node_id = 0;
    int i;

    pmi_errno = PMIx_Resolve_nodes(pmix_proc.nspace, &nodelist);
    MPIR_ERR_CHKANDJUMP1(pmi_errno != PMIX_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                         "**pmix_resolve_nodes", "**pmix_resolve_nodes %d", pmi_errno);
    MPIR_Assert(nodelist);

    node = strtok(nodelist, ",");
    while (node) {
        pmi_errno = PMIx_Resolve_peers(node, pmix_proc.nspace, &procs, &nprocs);
        MPIR_ERR_CHKANDJUMP1(pmi_errno != PMIX_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                             "**pmix_resolve_peers", "**pmix_resolve_peers %d", pmi_errno);
        for (i = 0; i < nprocs; i++) {
            nodemap[procs[i].rank] = node_id;
        }
        node_id++;
        node = strtok(NULL, ",");
    }
    *p_max_node_id = node_id - 1;
    /* PMIx latest adds pmix_free. We should switch to that at some point */
    MPL_external_free(nodelist);
    PMIX_PROC_FREE(procs, nprocs);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif

/* allocate and populate MPIR_Process.node_local_map and MPIR_Process.node_root_map */
static int build_locality(void)
{
    int i;
    int local_rank = -1;
    int local_size = 0;
    int *node_root_map, *node_local_map;

    int rank = MPIR_Process.rank;
    int size = MPIR_Process.size;
    int *node_map = MPIR_Process.node_map;
    int num_nodes = MPIR_Process.num_nodes;
    int local_node_id = node_map[rank];

    node_root_map = MPL_malloc(num_nodes * sizeof(int), MPL_MEM_ADDRESS);
    for (i = 0; i < num_nodes; i++) {
        node_root_map[i] = -1;
    }

    for (i = 0; i < size; i++) {
        int node_id = node_map[i];
        if (node_root_map[node_id] < 0) {
            node_root_map[node_id] = i;
        }
        if (node_id == local_node_id) {
            local_size++;
        }
    }

    node_local_map = MPL_malloc(local_size * sizeof(int), MPL_MEM_ADDRESS);
    int j = 0;
    for (i = 0; i < size; i++) {
        int node_id = node_map[i];
        if (node_id == local_node_id) {
            node_local_map[j] = i;
            if (i == rank) {
                local_rank = j;
            }
            j++;
        }
    }

    MPIR_Process.node_root_map = node_root_map;
    MPIR_Process.node_local_map = node_local_map;
    MPIR_Process.local_size = local_size;
    MPIR_Process.local_rank = local_rank;

    return MPI_SUCCESS;
}

#define _PMI_VAL_INIT \
    char *val = MPL_calloc(pmi_max_val_size, 1, MPL_MEM_OTHER)

#define _PMI_VAL_ENCODE(_buf, _sz) \
    do { \
        char *pval = val; \
        int rem = pmi_max_val_size; \
        int rc; \
        rc = MPL_str_add_binary_arg(&pval, &rem, "mpi", (char *) (_buf), _sz); \
        MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_OTHER, "**buscard"); \
        MPIR_Assert(rem >= 0); \
    } while (0)

#define _PMI_VAL_DECODE(_buf, _sz) \
    do { \
        int rc, out_len; \
        rc = MPL_str_get_binary_arg(val, "mpi", (char *) (_buf), _sz, &out_len); \
        MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_OTHER, "**buscard"); \
    } while (0)

#define _PMI_VAL_FREE \
    MPL_free(val)

int MPIR_pmi_bcast(void *buf, int bufsize, PMI_DOMAIN domain)
{
    int mpi_errno = MPI_SUCCESS;

    static int bcast_seq = 0;
    bcast_seq++;

    int local_node_id = MPIR_Process.node_map[MPIR_Process.rank];
    int is_node_root = (MPIR_Process.node_root_map[local_node_id] == MPIR_Process.rank);
    int in_domain = 1;
    if (domain == PMI_DOMAIN_NODE_ROOTS && !is_node_root) {
        in_domain = 0;
    }

    char key[50];
    sprintf(key, "_bcast-%d", bcast_seq);

    _PMI_VAL_INIT;
    if (in_domain && MPIR_Process.rank == 0) {
        _PMI_VAL_ENCODE(buf, bufsize);
        mpi_errno = MPIR_pmi_kvs_put(key, val);
        MPIR_ERR_CHECK(mpi_errno);
    }
#ifndef USE_PMIX_API
    /* PMIx will wait, so barrier unnecessary */
    mpi_errno = MPIR_pmi_barrier();
    MPIR_ERR_CHECK(mpi_errno);
#endif

    if (in_domain && MPIR_Process.rank != 0) {
        mpi_errno = MPIR_pmi_kvs_get(0, key, val, pmi_max_val_size);
        MPIR_ERR_CHECK(mpi_errno);
        _PMI_VAL_DECODE(buf, bufsize);
    }

  fn_exit:
    _PMI_VAL_FREE;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_pmi_allgather(const void *sendbuf, int sendsize, void *recvbuf, int recvsize,
                       PMI_DOMAIN domain)
{
    int mpi_errno = MPI_SUCCESS;

    static int allgather_seq = 0;
    allgather_seq++;

    int local_node_id = MPIR_Process.node_map[MPIR_Process.rank];
    int is_node_root = (MPIR_Process.node_root_map[local_node_id] == MPIR_Process.rank);
    int in_domain = 1;
    if (domain == PMI_DOMAIN_NODE_ROOTS && !is_node_root) {
        in_domain = 0;
    }

    char key[50];
    if (domain == PMI_DOMAIN_ALL) {
        sprintf(key, "_allgather-%d-%d", allgather_seq, MPIR_Process.rank);
    } else {
        sprintf(key, "_allgather-%d-%d", allgather_seq, local_node_id);
    }

    _PMI_VAL_INIT;
    if (in_domain) {
        _PMI_VAL_ENCODE(sendbuf, sendsize);
        mpi_errno = MPIR_pmi_kvs_put(key, val);
        MPIR_ERR_CHECK(mpi_errno);
    }
#ifndef USE_PMIX_API
    /* PMIx will wait, so barrier unnecessary */
    mpi_errno = MPIR_pmi_barrier();
    MPIR_ERR_CHECK(mpi_errno);
#endif

    if (in_domain) {
        int domain_size = (domain == PMI_DOMAIN_ALL ? MPIR_Process.size : MPIR_Process.num_nodes);
        int i;
        for (i = 0; i < domain_size; i++) {
            sprintf(key, "_allgather-%d-%d", allgather_seq, i);
            int rank = (domain == PMI_DOMAIN_ALL ? i : MPIR_Process.node_root_map[i]);
            mpi_errno = MPIR_pmi_kvs_get(MPIR_Process.node_root_map[i], key, val, pmi_max_val_size);
            MPIR_ERR_CHECK(mpi_errno);

            int rc, out_len;
            rc = MPL_str_get_binary_arg(val, "mpi", (char *) (recvbuf + i * size), size, &out_len);
            MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_OTHER, "**buscard");
        }
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* A special function used to bcast serialized shm segment handle */
int MPIR_pmi_bcast_local(char *val, int val_size)
{
    int mpi_errno = MPI_SUCCESS;
    int pmi_errno;

    MPIR_Assert(val_size <= pmi_max_val_size);

    int rank = MPIR_Process.rank;
    int local_node_id = MPIR_Process.node_map[rank];
    int local_size = MPIR_Process.local_size;
    int local_node_root = MPIR_Process.node_root_map[local_node_id];
    int is_node_root = (rank == local_node_root);

    char key[50];
#ifdef USE_PMI1_API
    if (is_node_root && local_size > 1) {
        sprintf(key, "_bcast_local-%d", rank);
        mpi_errno = MPIR_pmi_kvs_put(key, val);
        MPIR_ERR_CHECK(mpi_errno);
    }
    mpi_errno = MPIR_pmi_barrier();
    MPIR_ERR_CHECK(mpi_errno);
    if (!is_node_root) {
        sprintf(key, "_bcast_local-%d", local_node_root);
        mpi_errno = MPIR_pmi_kvs_get(local_node_root, key, val, pmi_max_val_size);
        MPIR_ERR_CHECK(mpi_errno);
    }
#elif defined USE_PMI2_API
    static int bcast_local_seq = 0;
    bcast_local_seq++;
    sprintf(key, "_bcast_local-%d", bcast_local_seq);
    if (local_size == 1) {
        /* nothing to do */
    } else if (is_node_root) {
        pmi_errno = PMI2_Info_PutNodeAttr(key, val);
        MPIR_ERR_CHKANDJUMP(pmi_errno != PMI2_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                            "**pmi_putnodeattr");
    } else {
        int found = 0;
        /* set last param, waitfor, to TRUE */
        pmi_errno = PMI2_Info_GetNodeAttr(key, val, PMI2_MAX_VALLEN, &found, TRUE);
        MPIR_ERR_CHKANDJUMP(pmi_errno != PMI2_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                            "**pmi_getnodeattr");
        MPIR_ERR_CHKINTERNAL(!found, mpi_errno, "nodeattr not found");
    }
#elif defined USE_PMIX_API
    static int bcast_local_seq = 0;
    bcast_local_seq++;
    sprintf(key, "_bcast_local-%d", bcast_local_seq);
    if (local_size == 1) {
        /* nothing to do */
    } else if (is_node_root) {
        pmix_value_t value;
        value.type = PMIX_STRING;
        value.data.string = val;
        pmi_errno = PMIx_Put(PMIX_LOCAL, key, &value);
        MPIR_ERR_CHKANDJUMP1(pmi_errno != PMIX_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                             "**pmix_put", "**pmix_put %d", pmi_errno);
        pmi_errno = PMIx_Commit();
        MPIR_ERR_CHKANDJUMP1(pmi_errno != PMIX_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                             "**pmix_commit", "**pmix_commit %d", pmi_errno);
    } else {
        pmix_proc_t proc;
        PMIX_PROC_CONSTRUCT(&proc);
        proc.rank = local_node_root;

        pmix_value_t *pvalue;
        /* blocking, waits until the value arrives */
        pmi_errno = PMIx_Get(&proc, key, NULL, 0, &pvalue);
        MPIR_ERR_CHKANDJUMP1(pmi_errno != PMIX_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                             "**pmix_get", "**pmix_get %d", pmi_errno);
        strncpy(val, pvalue->data.string, val_size);
        PMIX_VALUE_RELEASE(pvalue);
    }
#endif
  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* ---- static functions ---- */
/* FIXME: add parameters */
static int get_no_local(void);
static int get_num_cliques(void);
static int build_nodemap_nolocal(int *nodemap, int sz, int *p_max_node_id);
static int build_nodemap_roundrobin(int num_cliques, int *nodemap, int sz, int *p_max_node_id);
static int build_nodemap_fallback(int *nodemap, int sz, int *p_max_node_id);
static int build_nodemap_pmi1(int *nodemap, int sz, int *p_max_node_id);
static int build_nodemap_pmi2(int *nodemap, int sz, int *p_max_node_id);
static int build_nodemap_pmix(int *nodemap, int sz, int *p_max_node_id);

static int build_nodemap(int *nodemap, int sz, int *p_max_node_id)
{
    int mpi_errno = MPI_SUCCESS;
    int pmi_errno;

    if (sz == 1 || get_no_local()) {
        mpi_errno = build_nodemap_nolocal(nodemap, sz, p_max_node_id);
        goto fn_exit;
    }
#ifdef USE_PMI1_API
    mpi_errno = build_nodemap_pmi1(nodemap, sz, p_max_node_id);
#elif defined(USE_PMI2_API)
    mpi_errno = build_nodemap_pmi2(nodemap, sz, p_max_node_id);
#elif defined(USE_PMIX_API)
    mpi_errno = build_nodemap_pmix(nodemap, sz, p_max_node_id);
#endif
    MPIR_ERR_CHECK(mpi_errno);

    int num_cliques = get_num_cliques();
    if (num_cliques > sz) {
        num_cliques = sz;
    }
    if (*p_max_node_id == 0 && num_cliques > 1) {
        mpi_errno = build_nodemap_roundrobin(num_cliques, nodemap, sz, p_max_node_id);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int get_no_local(void)
{
    /* Used for debugging only.  This disables communication over shared memory */
#ifdef ENABLED_NO_LOCAL
    return 1;
#else
    return MPIR_CVAR_NOLOCAL;
#endif
}

static int get_num_cliques(void)
{
    /* Used for debugging on a single machine: split procs into num_cliques nodes.
     * If ODD_EVEN_CLIQUES were enabled, split procs into 2 nodes.
     */
    if (MPIR_CVAR_NUM_CLIQUES > 1) {
        return MPIR_CVAR_NUM_CLIQUES;
    } else {
#ifdef ENABLED_ODD_EVEN_CLIQUES
        return 2;
#else
        return MPIR_CVAR_NUM_CLIQUES ? 2 : 1;
#endif
    }
}

int build_nodemap_nolocal(int *nodemap, int sz, int *p_max_node_id)
{
    int i;
    for (i = 0; i < sz; ++i) {
        nodemap[i] = i;
    }
    *p_max_node_id = sz - 1;
    return MPI_SUCCESS;
}

static int build_nodemap_roundrobin(int num_cliques, int *nodemap, int sz, int *p_max_node_id)
{
    int i;
    for (i = 0; i < sz; ++i) {
        nodemap[i] = i % num_cliques;
    }
    *p_max_node_id = num_cliques - 1;
    return MPI_SUCCESS;
}

int build_nodemap_fallback(int *nodemap, int sz, int *p_max_node_id)
{
    int mpi_errno = MPI_SUCCESS;
    int pmi_errno;
    int i, j;

    /* fallback algorithm: O(N^2) PMI_KVS_Gets */

    /* set hostname */
    char hostname[MAX_HOSTNAME_LEN];
    int rc;
    rc = gethostname(hostname, MAX_HOSTNAME_LEN);
    MPIR_ERR_CHKANDJUMP2(rc == -1, mpi_errno, MPI_ERR_OTHER, "**sock_gethost",
                         "**sock_gethost %s %d", MPIR_Strerror(errno), errno);
    hostname[MAX_HOSTNAME_LEN - 1] = '\0';

    int bufsize = MAX_HOSTNAME_LEN;
    /* Allocate temporary structures.  These would need to be persistent if
     * we somehow were able to support dynamic processes via this method. */
    MPIR_Assert(bufsize <= pmi_max_val_size);
    char **node_names = NULL;
    char *node_name_buf = NULL;
    node_names = (char **) MPL_malloc(sz * sizeof(char *), MPL_MEM_ADDRESS);
    node_name_buf = (char *) MPL_malloc(sz * bufsize, MPL_MEM_ADDRESS);

    /* Gather hostnames */
    for (i = 0; i < sz; ++i) {
        node_names[i] = &node_name_buf[i * bufsize];
        node_names[i][0] = '\0';
    }

    mpi_errno = MPIR_pmi_allgather(hostname, bufsize, node_name_buf, PMI_DOMAIN_ALL);

    int max_node_id = -1;       /* defensive */
    for (i = 0; i < sz; ++i) {
        MPIR_Assert(max_node_id < sz);
        for (j = 0; j < max_node_id + 1; ++j)
            if (!MPL_strncmp(node_names[j], node_names[max_node_id + 1], pmi_max_key_size))
                break;
        if (j == max_node_id + 1)
            ++max_node_id;
        else
            node_names[max_node_id + 1][0] = '\0';
        nodemap[i] = j;
    }
    *p_max_node_id = max_node_id;

  fn_exit:
    if (node_names) {
        MPL_free(node_names);
        MPL_free(node_name_buf);
    }
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int build_nodemap_pmi1(int *nodemap, int sz, int *p_max_node_id)
{
    int mpi_errno = MPI_SUCCESS;
    int pmi_errno;
#ifdef USE_PMI1_API
    int did_map = 0;
    if (pmi_version == 1 && pmi_subversion == 1) {
        char *process_mapping = MPL_malloc(pmi_max_val_size, MPL_MEM_ADDRESS);
        pmi_errno = PMI_KVS_Get(pmi_kvs_name, "PMI_process_mapping",
                                process_mapping, pmi_max_val_size);
        if (pmi_errno == PMI_SUCCESS) {
            mpi_errno = MPIR_NODEMAP_populate_ids_from_mapping(process_mapping, sz, nodemap,
                                                               p_max_node_id, &did_map);
            MPIR_ERR_CHECK(mpi_errno);
            MPIR_ERR_CHKINTERNAL(!did_map, mpi_errno,
                                 "unable to populate node ids from PMI_process_mapping");
        }
        MPL_free(process_mapping);
    }
    if (!did_map) {
        mpi_errno = build_nodemap_fallback(nodemap, sz, p_max_node_id);
    }
#endif
  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int build_nodemap_pmi2(int *nodemap, int sz, int *p_max_node_id)
{
    int mpi_errno = MPI_SUCCESS;
    int pmi_errno;
#ifdef USE_PMI2_API
    char process_mapping[PMI2_MAX_VALLEN];
    pmi_errno =
        PMI2_Info_GetJobAttr("PMI_process_mapping", process_mapping, PMI2_MAX_VALLEN, &found);
    MPIR_ERR_CHKANDJUMP1(pmi_errno != PMI2_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                         "**pmi2_info_getjobattr", "**pmi2_info_getjobattr %d", pmi_errno);
    MPIR_ERR_CHKINTERNAL(!found, mpi_errno, "PMI_process_mapping attribute not found");

    int did_map;
    mpi_errno = MPIR_NODEMAP_populate_ids_from_mapping(process_mapping, sz, nodemap,
                                                       p_max_node_id, &did_map);
    MPIR_ERR_CHECK(mpi_errno);
    MPIR_ERR_CHKINTERNAL(!did_map, mpi_errno,
                         "unable to populate node ids from PMI_process_mapping");
#endif
  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int build_nodemap_pmix(int *nodemap, int sz, int *p_max_node_id)
{
    int mpi_errno = MPI_SUCCESS;
    int pmi_errno;
#ifdef USE_PMIX_API
    char *nodelist = NULL, *node = NULL;
    pmix_proc_t *procs = NULL;
    size_t nprocs, node_id = 0;
    int i;

    pmi_errno = PMIx_Resolve_nodes(pmix_proc.nspace, &nodelist);
    MPIR_ERR_CHKANDJUMP1(pmi_errno != PMIX_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                         "**pmix_resolve_nodes", "**pmix_resolve_nodes %d", pmi_errno);
    MPIR_Assert(nodelist);

    node = strtok(nodelist, ",");
    while (node) {
        pmi_errno = PMIx_Resolve_peers(node, pmix_proc.nspace, &procs, &nprocs);
        MPIR_ERR_CHKANDJUMP1(pmi_errno != PMIX_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                             "**pmix_resolve_peers", "**pmix_resolve_peers %d", pmi_errno);
        for (i = 0; i < nprocs; i++) {
            nodemap[procs[i].rank] = node_id;
        }
        node_id++;
        node = strtok(NULL, ",");
    }
    *p_max_node_id = node_id - 1;
    /* PMIx latest adds pmix_free. We should switch to that at some point */
    MPL_external_free(nodelist);
    PMIX_PROC_FREE(procs, nprocs);
#endif
  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int build_locality(void)
{
    /* builds node_root_map and node_local_map as well as local_size
     * based on node_map.
     */
    int i;
    int local_rank = -1;
    int local_size = 0;
    int *node_root_map, *node_local_map;

    int *node_map = MPIR_Process.node_map;
    int num_nodes = MPIR_Process.num_nodes;
    int local_node_id = node_map[MPIR_Process.rank];

    node_root_map = MPL_malloc(num_nodes * sizeof(int), MPL_MEM_ADDRESS);
    for (i = 0; i < num_nodes; i++) {
        node_root_map[i] = -1;
    }

    for (i = 0; i < MPIR_Process.size; i++) {
        int node_id = node_map[i];
        if (node_root_map[node_id] < 0) {
            node_root_map[node_id] = i;
        }
        if (node_id == local_node_id) {
            local_size++;
        }
    }

    node_local_map = MPL_malloc(local_size * sizeof(int), MPL_MEM_ADDRESS);
    int j = 0;
    for (i = 0; i < MPIR_Process.size; i++) {
        int node_id = node_map[i];
        if (node_id == local_node_id) {
            node_local_map[j] = i;
            if (i == rank) {
                local_rank = j;
            }
            j++;
        }
    }

    MPIR_Process.node_root_map = node_root_map;
    MPIR_Process.node_local_map = node_local_map;
    MPIR_Process.local_size = local_size;
    MPIR_Process.local_rank = local_rank;

    return MPI_SUCCESS;
}
