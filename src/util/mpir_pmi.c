/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
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

void MPIR_pmi_abort(int exit_code, const char *error_msg)
{
#ifdef USE_PMI1_API
    PMI_Abort(exit_code, error_msg);
#elif defined(USE_PMI2_API)
    PMI2_Abort(TRUE, error_msg);
#elif defined(USE_PMIX_API)
    PMIx_Abort(exit_code, error_msg, NULL, 0);
#endif
}

/* getters for internal constants */
int MPIR_pmi_max_key_size(void)
{
    return pmi_max_key_size;
}

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
int MPIR_pmi_kvs_put(const char *key, const char *val)
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
    value.data.string = (char *) val;
    pmi_errno = PMIx_Put(PMIX_GLOBAL, key, &value);
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
int MPIR_pmi_kvs_get(int src, const char *key, char *val, int val_size)
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

int MPIR_pmi_barrier(void)
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

    /* use global wildcard proc set */
    pmi_errno = PMIx_Fence(&pmix_wcproc, 1, info, 1);
    MPIR_ERR_CHKANDJUMP1(pmi_errno != PMIX_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                         "**pmix_fence", "**pmix_fence %d", pmi_errno);
    PMIX_INFO_FREE(info, 1);
#endif

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_pmi_barrier_local(void)
{
#if defined(USE_PMIX_API)
    int mpi_errno = MPI_SUCCESS;
    int pmi_errno;
    int local_size = MPIR_Process.local_size;
    pmix_proc_t *procs = MPL_malloc(local_size * sizeof(pmix_proc_t), MPL_MEM_OTHER);
    for (int i = 0; i < local_size; i++) {
        PMIX_PROC_CONSTRUCT(&procs[i]);
        strncpy(procs[i].nspace, pmix_proc.nspace, PMIX_MAX_NSLEN);
        procs[i].rank = MPIR_Process.node_local_map[i];
    }

    pmix_info_t *info;
    int flag = 1;
    PMIX_INFO_CREATE(info, 1);
    PMIX_INFO_LOAD(info, PMIX_COLLECT_DATA, &flag, PMIX_BOOL);

    pmi_errno = PMIx_Fence(procs, local_size, info, 1);
    MPIR_ERR_CHKANDJUMP1(pmi_errno != PMIX_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**pmix_fence",
                         "**pmix_fence %d", pmi_errno);

    PMIX_INFO_FREE(info, 1);
    MPL_free(procs);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
#else
    /* If local barrier is not supported (PMI1 and PMI2), simply fallback */
    return MPIR_pmi_barrier();
#endif
}

/* declare static functions used in bcast/allgather */
static void encode(int size, const char *src, char *dest);
static void decode(int size, const char *src, char *dest);

/* is_local is a hint that we optimize for node local access when we can */
static int optimized_put(const char *key, const char *val, int is_local)
{
    int mpi_errno = MPI_SUCCESS;
#if defined(USE_PMI1_API)
    mpi_errno = MPIR_pmi_kvs_put(key, val);
    MPIR_ERR_CHECK(mpi_errno);
#elif defined(USE_PMI2_API)
    if (!is_local) {
        mpi_errno = MPIR_pmi_kvs_put(key, val);
    } else {
        int pmi_errno = PMI2_Info_PutNodeAttr(key, val);
        MPIR_ERR_CHKANDJUMP(pmi_errno != PMI2_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                            "**pmi_putnodeattr");
    }
#elif defined(USE_PMIX_API)
    int pmi_errno;
    pmix_value_t value;
    value.type = PMIX_STRING;
    value.data.string = (char *) val;
    pmi_errno = PMIx_Put(is_local ? PMIX_LOCAL : PMIX_GLOBAL, key, &value);
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

static int optimized_get(int src, const char *key, char *val, int valsize, int is_local)
{
#if defined(USE_PMI1_API)
    return MPIR_pmi_kvs_get(src, key, val, valsize);
#elif defined(USE_PMI2_API)
    if (is_local) {
        int mpi_errno = MPI_SUCCESS;
        int found;
        int pmi_errno = PMI2_Info_GetNodeAttr(key, val, valsize, &found, TRUE);
        if (pmi_errno != PMI2_SUCCESS) {
            MPIR_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**pmi_getnodeattr");
        } else if (!found) {
            MPIR_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**pmi_getnodeattr");
        }
        return mpi_errno;
    } else {
        return MPIR_pmi_kvs_get(src, key, val, valsize);
    }
#else
    return MPIR_pmi_kvs_get(src, key, val, valsize);
#endif
}

/* higher-level binary put/get:
 * 1. binary encoding/decoding
 * 2. chops long values into multiple segments
 * 3. uses optimized_put/get for the case of node-level access
 */
static int put_ex(const char *key, const void *buf, int bufsize, int is_local)
{
    int mpi_errno = MPI_SUCCESS;
#if defined(USE_PMI1_API) || defined(USE_PMI2_API)
    char *val = MPL_malloc(pmi_max_val_size, MPL_MEM_OTHER);
    /* reserve some spaces for '\0' and maybe newlines
     * (depends on pmi implementations, and may not be sufficient) */
    int segsize = (pmi_max_val_size - 2) / 2;
    if (bufsize < segsize) {
        encode(bufsize, buf, val);
        mpi_errno = optimized_put(key, val, is_local);
        MPIR_ERR_CHECK(mpi_errno);
    } else {
        int num_segs = bufsize / segsize;
        if (bufsize % segsize > 0) {
            num_segs++;
        }
        MPL_snprintf(val, pmi_max_val_size, "segments=%d", num_segs);
        mpi_errno = MPIR_pmi_kvs_put(key, val);
        MPIR_ERR_CHECK(mpi_errno);
        for (int i = 0; i < num_segs; i++) {
            char seg_key[50];
            sprintf(seg_key, "%s-seg-%d/%d", key, i + 1, num_segs);
            int n = segsize;
            if (i == num_segs - 1) {
                n = bufsize - segsize * (num_segs - 1);
            }
            encode(n, (char *) buf + i * segsize, val);
            mpi_errno = optimized_put(seg_key, val, is_local);
            MPIR_ERR_CHECK(mpi_errno);
        }
    }
#elif defined(USE_PMIX_API)
    int n = bufsize * 2 + 1;
    char *val = MPL_malloc(n, MPL_MEM_OTHER);
    encode(bufsize, buf, val);
    mpi_errno = optimized_put(key, val, is_local);
    MPIR_ERR_CHECK(mpi_errno);
#endif
  fn_exit:
    MPL_free(val);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int get_ex(int src, const char *key, void *buf, int *p_size, int is_local)
{
    int mpi_errno = MPI_SUCCESS;
    char *val = MPL_malloc(pmi_max_val_size, MPL_MEM_OTHER);
    int segsize = (pmi_max_val_size - 1) / 2;

    MPIR_Assert(p_size);
    MPIR_Assert(*p_size > 0);
    int bufsize = *p_size;
    int got_size;

    mpi_errno = optimized_get(src, key, val, pmi_max_val_size, is_local);
    MPIR_ERR_CHECK(mpi_errno);
    if (strncmp(val, "segments=", 9) == 0) {
        int num_segs = atoi(val + 9);
        got_size = 0;
        for (int i = 0; i < num_segs; i++) {
            char seg_key[50];
            sprintf(seg_key, "%s-seg-%d/%d", key, i + 1, num_segs);
            mpi_errno = optimized_get(src, seg_key, val, pmi_max_val_size, is_local);
            MPIR_ERR_CHECK(mpi_errno);
            int n = strlen(val) / 2;    /* 2-to-1 decode */
            if (i < num_segs - 1) {
                MPIR_Assert(n == segsize);
            } else {
                MPIR_Assert(n <= segsize);
            }
            decode(n, val, (char *) buf + i * segsize);
            got_size += n;
        }
    } else {
        int n = strlen(val) / 2;        /* 2-to-1 decode */
        decode(n, val, (char *) buf);
        got_size = n;
    }
    MPIR_Assert(got_size <= bufsize);
    if (got_size < bufsize) {
        ((char *) buf)[got_size] = '\0';
    }

    *p_size = got_size;

  fn_exit:
    MPL_free(val);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int optional_bcast_barrier(MPIR_PMI_DOMAIN domain)
{
#if defined(USE_PMI1_API)
    /* unless bcast is skipped altogether */
    if (domain == MPIR_PMI_DOMAIN_ALL && MPIR_Process.size == 1) {
        return MPI_SUCCESS;
    } else if (domain == MPIR_PMI_DOMAIN_NODE_ROOTS && MPIR_Process.num_nodes == 1) {
        return MPI_SUCCESS;
    } else if (domain == MPIR_PMI_DOMAIN_LOCAL && MPIR_Process.size == MPIR_Process.num_nodes) {
        return MPI_SUCCESS;
    }
#elif defined(USE_PMI2_API)
    if (domain == MPIR_PMI_DOMAIN_ALL && MPIR_Process.size == 1) {
        return MPI_SUCCESS;
    } else if (domain == MPIR_PMI_DOMAIN_NODE_ROOTS && MPIR_Process.num_nodes == 1) {
        return MPI_SUCCESS;
    } else if (domain == MPIR_PMI_DOMAIN_LOCAL) {
        /* PMI2 local uses Put/GetNodeAttr, no need for barrier */
        return MPI_SUCCESS;
    }
#elif defined(USE_PMIx_API)
    /* PMIx will block/wait, so barrier unnecessary */
    return MPI_SUCCESS;
#endif
    return MPIR_pmi_barrier();
}

int MPIR_pmi_bcast(void *buf, int bufsize, MPIR_PMI_DOMAIN domain)
{
    int mpi_errno = MPI_SUCCESS;

    int rank = MPIR_Process.rank;
    int local_node_id = MPIR_Process.node_map[rank];
    int node_root = MPIR_Process.node_root_map[local_node_id];
    int is_node_root = (node_root == rank);

    int in_domain, is_root, is_local, bcast_size;
    if (domain == MPIR_PMI_DOMAIN_NODE_ROOTS && !is_node_root) {
        in_domain = 0;
    } else {
        in_domain = 1;
    }
    if (rank == 0 || (domain == MPIR_PMI_DOMAIN_LOCAL && is_node_root)) {
        is_root = 1;
    } else {
        is_root = 0;
    }
    is_local = (domain == MPIR_PMI_DOMAIN_LOCAL);

    bcast_size = MPIR_Process.size;
    if (domain == MPIR_PMI_DOMAIN_NODE_ROOTS) {
        bcast_size = MPIR_Process.num_nodes;
    } else if (domain == MPIR_PMI_DOMAIN_LOCAL) {
        bcast_size = MPIR_Process.local_size;
    }
    if (bcast_size == 1) {
        in_domain = 0;
    }

    char key[50];
    int root;
    static int bcast_seq = 0;

    if (!in_domain) {
        /* PMI_Barrier may require all process to participate */
        mpi_errno = optional_bcast_barrier(domain);
        MPIR_ERR_CHECK(mpi_errno);
    } else {
        MPIR_Assert(buf);
        MPIR_Assert(bufsize > 0);

        bcast_seq++;

        root = 0;
        if (domain == MPIR_PMI_DOMAIN_LOCAL) {
            root = node_root;
        }
        /* add root to the key since potentially we may have multiple root(s)
         * on a single node due to odd-even-cliques */
        sprintf(key, "-bcast-%d-%d", bcast_seq, root);

        if (is_root) {
            mpi_errno = put_ex(key, buf, bufsize, is_local);
            MPIR_ERR_CHECK(mpi_errno);
        }

        mpi_errno = optional_bcast_barrier(domain);
        MPIR_ERR_CHECK(mpi_errno);

        if (!is_root) {
            int got_size = bufsize;
            mpi_errno = get_ex(root, key, buf, &got_size, is_local);
            MPIR_ERR_CHECK(mpi_errno);
        }
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_pmi_allgather(const void *sendbuf, int sendsize, void *recvbuf, int recvsize,
                       MPIR_PMI_DOMAIN domain)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Assert(domain != MPIR_PMI_DOMAIN_LOCAL);

    int local_node_id = MPIR_Process.node_map[MPIR_Process.rank];
    int is_node_root = (MPIR_Process.node_root_map[local_node_id] == MPIR_Process.rank);
    int in_domain = 1;
    if (domain == MPIR_PMI_DOMAIN_NODE_ROOTS && !is_node_root) {
        in_domain = 0;
    }

    static int allgather_seq = 0;
    allgather_seq++;

    char key[50];
    sprintf(key, "-allgather-%d-%d", allgather_seq, MPIR_Process.rank);

    if (in_domain) {
        mpi_errno = put_ex(key, sendbuf, sendsize, 0);
        MPIR_ERR_CHECK(mpi_errno);
    }
#ifndef USE_PMIX_API
    /* PMIx will wait, so barrier unnecessary */
    mpi_errno = MPIR_pmi_barrier();
    MPIR_ERR_CHECK(mpi_errno);
#endif

    if (in_domain) {
        int domain_size = MPIR_Process.size;
        if (domain == MPIR_PMI_DOMAIN_NODE_ROOTS) {
            domain_size = MPIR_Process.num_nodes;
        }
        for (int i = 0; i < domain_size; i++) {
            int rank = i;
            if (domain == MPIR_PMI_DOMAIN_NODE_ROOTS) {
                rank = MPIR_Process.node_root_map[i];
            }
            sprintf(key, "-allgather-%d-%d", allgather_seq, rank);
            int got_size = recvsize;
            mpi_errno = get_ex(rank, key, (unsigned char *) recvbuf + i * recvsize, &got_size, 0);
            MPIR_ERR_CHECK(mpi_errno);
        }
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* This version assumes shm_buf is shared across local procs. Each process
 * participate in the gather part by distributing the task over local procs.
 *
 * NOTE: the behavior is different with MPIR_pmi_allgather when domain is
 * MPIR_PMI_DOMAIN_NODE_ROOTS. With MPIR_pmi_allgather, only the root_nodes participate.
 */
int MPIR_pmi_allgather_shm(const void *sendbuf, int sendsize, void *shm_buf, int recvsize,
                           MPIR_PMI_DOMAIN domain)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Assert(domain != MPIR_PMI_DOMAIN_LOCAL);

    int rank = MPIR_Process.rank;
    int size = MPIR_Process.size;
    int local_size = MPIR_Process.local_size;
    int local_rank = MPIR_Process.local_rank;
    int local_node_id = MPIR_Process.node_map[rank];
    int node_root = MPIR_Process.node_root_map[local_node_id];
    int is_node_root = (node_root == MPIR_Process.rank);

    static int allgather_shm_seq = 0;
    allgather_shm_seq++;

    char key[50];
    sprintf(key, "-allgather-shm-%d-%d", allgather_shm_seq, rank);

    /* in roots-only, non-roots would skip the put */
    if (domain != MPIR_PMI_DOMAIN_NODE_ROOTS || is_node_root) {
        mpi_errno = put_ex(key, (unsigned char *) sendbuf, sendsize, 0);
        MPIR_ERR_CHECK(mpi_errno);
    }

    mpi_errno = MPIR_pmi_barrier();
    MPIR_ERR_CHECK(mpi_errno);

    /* Each rank need get val from "size" ranks, divide the task evenly over local ranks */
    if (domain == MPIR_PMI_DOMAIN_NODE_ROOTS) {
        size = MPIR_Process.num_nodes;
    }
    int per_local_rank = size / local_size;
    if (per_local_rank * local_size < size) {
        per_local_rank++;
    }
    int start = local_rank * per_local_rank;
    int end = start + per_local_rank;
    if (end > size) {
        end = size;
    }
    for (int i = start; i < end; i++) {
        int src = i;
        if (domain == MPIR_PMI_DOMAIN_NODE_ROOTS) {
            src = MPIR_Process.node_root_map[i];
        }
        sprintf(key, "-allgather-shm-%d-%d", allgather_shm_seq, src);
        int got_size = recvsize;
        mpi_errno = get_ex(src, key, (unsigned char *) shm_buf + i * recvsize, &got_size, 0);
        MPIR_ERR_CHECK(mpi_errno);
        MPIR_Assert(got_size <= recvsize);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_pmi_get_universe_size(int *universe_size)
{
    int mpi_errno = MPI_SUCCESS;
    int pmi_errno;

#ifdef USE_PMI1_API
    pmi_errno = PMI_Get_universe_size(universe_size);
    MPIR_ERR_CHKANDJUMP1(pmi_errno != PMI_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                         "**pmi_get_universe_size", "**pmi_get_universe_size %d", pmi_errno);
#elif defined(USE_PMI2_API)
    char val[PMI2_MAX_VALLEN];
    int found = 0;
    char *endptr;

    pmi_errno = PMI2_Info_GetJobAttr("universeSize", val, sizeof(val), &found);
    MPIR_ERR_CHKANDJUMP1(pmi_errno != PMI2_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                         "**pmi_getjobattr", "**pmi_getjobattr %d", pmi_errno);
    if (!found) {
        *universe_size = MPIR_UNIVERSE_SIZE_NOT_AVAILABLE;
    } else {
        *universe_size = strtol(val, &endptr, 0);
        MPIR_ERR_CHKINTERNAL(endptr - val != strlen(val), mpi_errno, "can't parse universe size");
    }
#elif defined(USE_PMIX_API)
    pmix_value_t *pvalue = NULL;

    pmi_errno = PMIx_Get(&pmix_wcproc, PMIX_UNIV_SIZE, NULL, 0, &pvalue);
    MPIR_ERR_CHKANDJUMP1(pmi_errno != PMIX_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                         "**pmix_get", "**pmix_get %d", pmi_errno);
    *universe_size = pvalue->data.uint32;
    PMIX_VALUE_RELEASE(pvalue);
#endif
  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

char *MPIR_pmi_get_failed_procs(void)
{
    int pmi_errno;
    char *failed_procs_string = NULL;

    failed_procs_string = MPL_malloc(pmi_max_val_size, MPL_MEM_OTHER);
    MPIR_Assert(failed_procs_string);
#ifdef USE_PMI1_API
    pmi_errno = PMI_KVS_Get(pmi_kvs_name, "PMI_dead_processes",
                            failed_procs_string, pmi_max_val_size);
    if (pmi_errno != PMI_SUCCESS)
        goto fn_fail;
#elif defined(USE_PMI2_API)
    int out_len;
    pmi_errno = PMI2_KVS_Get(pmi_jobid, PMI2_ID_NULL, "PMI_dead_processes",
                             failed_procs_string, pmi_max_val_size, &out_len);
    if (pmi_errno != PMI2_SUCCESS)
        goto fn_fail;
#elif defined(USE_PMIX_API)
    goto fn_fail;
#endif

  fn_exit:
    return failed_procs_string;
  fn_fail:
    /* FIXME: approprate error messages here? */
    MPL_free(failed_procs_string);
    failed_procs_string = NULL;
    goto fn_exit;
}

/* static functions only for MPIR_pmi_spawn_multiple */
#if defined(USE_PMI1_API) || defined(USE_PMI2_API)
/* PMI_keyval_t is only defined in PMI1 or PMI2 */
static int mpi_to_pmi_keyvals(MPIR_Info * info_ptr, PMI_keyval_t ** kv_ptr, int *nkeys_ptr);
static void free_pmi_keyvals(PMI_keyval_t ** kv, int size, int *counts);
#endif

/* NOTE: MPIR_pmi_spawn_multiple is to be called by a single root spawning process */
int MPIR_pmi_spawn_multiple(int count, char *commands[], char **argvs[],
                            const int maxprocs[], MPIR_Info * info_ptrs[],
                            int num_preput_keyval, struct MPIR_PMI_KEYVAL *preput_keyvals,
                            int *pmi_errcodes)
{
    int mpi_errno = MPI_SUCCESS;
    int pmi_errno;

#ifdef USE_PMI1_API
    int *info_keyval_sizes = NULL;
    PMI_keyval_t **info_keyval_vectors = NULL;

    info_keyval_sizes = (int *) MPL_malloc(count * sizeof(int), MPL_MEM_BUFFER);
    MPIR_ERR_CHKANDJUMP(!info_keyval_sizes, mpi_errno, MPI_ERR_OTHER, "**nomem");

    info_keyval_vectors =
        (PMI_keyval_t **) MPL_malloc(count * sizeof(PMI_keyval_t *), MPL_MEM_BUFFER);
    MPIR_ERR_CHKANDJUMP(!info_keyval_vectors, mpi_errno, MPI_ERR_OTHER, "**nomem");

    if (!info_ptrs) {
        for (int i = 0; i < count; i++) {
            info_keyval_vectors[i] = 0;
            info_keyval_sizes[i] = 0;
        }
    } else {
        for (int i = 0; i < count; i++) {
            mpi_errno = mpi_to_pmi_keyvals(info_ptrs[i], &info_keyval_vectors[i],
                                           &info_keyval_sizes[i]);
            MPIR_ERR_CHECK(mpi_errno);
        }
    }

    pmi_errno = PMI_Spawn_multiple(count, (const char **) commands, (const char ***) argvs,
                                   maxprocs,
                                   info_keyval_sizes,
                                   (const PMI_keyval_t **) info_keyval_vectors,
                                   num_preput_keyval, (PMI_keyval_t *) preput_keyvals,
                                   pmi_errcodes);

    MPIR_ERR_CHKANDJUMP1(pmi_errno != PMI_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                         "**pmi_spawn_multiple", "**pmi_spawn_multiple %d", pmi_errno);
#elif defined(USE_PMI2_API)
    /* not supported yet */
    MPIR_Assert(0);
#elif defined(USE_PMIX_API)
    /* not supported yet */
    MPIR_Assert(0);
#endif

  fn_exit:
#ifdef USE_PMI1_API
    if (info_keyval_vectors) {
        free_pmi_keyvals(info_keyval_vectors, count, info_keyval_sizes);
        MPL_free(info_keyval_vectors);
    }

    MPL_free(info_keyval_sizes);
#endif

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
#ifdef ENABLE_NO_LOCAL
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
        return MPIR_CVAR_ODD_EVEN_CLIQUES ? 2 : 1;
    }
}

/* one process per node */
int build_nodemap_nolocal(int *nodemap, int sz, int *p_max_node_id)
{
    for (int i = 0; i < sz; ++i) {
        nodemap[i] = i;
    }
    *p_max_node_id = sz - 1;
    return MPI_SUCCESS;
}

/* assign processes to num_cliques nodes in a round-robin fashion */
static int build_nodemap_roundrobin(int num_cliques, int *nodemap, int sz, int *p_max_node_id)
{
    for (int i = 0; i < sz; ++i) {
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
    return MPIR_NODEMAP_build_nodemap_fallback(sz, MPIR_Process.rank, nodemap, p_max_node_id);
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

    pmi_errno = PMIx_Resolve_nodes(pmix_proc.nspace, &nodelist);
    MPIR_ERR_CHKANDJUMP1(pmi_errno != PMIX_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                         "**pmix_resolve_nodes", "**pmix_resolve_nodes %d", pmi_errno);
    MPIR_Assert(nodelist);

    node = strtok(nodelist, ",");
    while (node) {
        pmi_errno = PMIx_Resolve_peers(node, pmix_proc.nspace, &procs, &nprocs);
        MPIR_ERR_CHKANDJUMP1(pmi_errno != PMIX_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                             "**pmix_resolve_peers", "**pmix_resolve_peers %d", pmi_errno);
        for (int i = 0; i < nprocs; i++) {
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
    int local_rank = -1;
    int local_size = 0;
    int *node_root_map, *node_local_map;

    int rank = MPIR_Process.rank;
    int size = MPIR_Process.size;
    int *node_map = MPIR_Process.node_map;
    int num_nodes = MPIR_Process.num_nodes;
    int local_node_id = node_map[rank];

    node_root_map = MPL_malloc(num_nodes * sizeof(int), MPL_MEM_ADDRESS);
    for (int i = 0; i < num_nodes; i++) {
        node_root_map[i] = -1;
    }

    for (int i = 0; i < size; i++) {
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
    for (int i = 0; i < size; i++) {
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

/* similar to functions in mpl/src/str/mpl_argstr.c, but much simpler */
static int hex(unsigned char c)
{
    if (c >= '0' && c <= '9') {
        return c - '0';
    } else if (c >= 'a' && c <= 'f') {
        return 10 + c - 'a';
    } else if (c >= 'A' && c <= 'F') {
        return 10 + c - 'A';
    } else {
        MPIR_Assert(0);
        return -1;
    }
}

static void encode(int size, const char *src, char *dest)
{
    for (int i = 0; i < size; i++) {
        MPL_snprintf(dest, 3, "%02X", (unsigned char) *src);
        src++;
        dest += 2;
    }
}

static void decode(int size, const char *src, char *dest)
{
    for (int i = 0; i < size; i++) {
        *dest = (char) (hex(src[0]) << 4) + hex(src[1]);
        src += 2;
        dest++;
    }
}

/* static functions used in MPIR_pmi_spawn_multiple */
#if defined(USE_PMI1_API) || defined(USE_PMI2_API)
static int mpi_to_pmi_keyvals(MPIR_Info * info_ptr, PMI_keyval_t ** kv_ptr, int *nkeys_ptr)
{
    char key[MPI_MAX_INFO_KEY];
    PMI_keyval_t *kv = 0;
    int nkeys = 0, vallen, flag, mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPI_TO_PMI_KEYVALS);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPI_TO_PMI_KEYVALS);

    if (!info_ptr || info_ptr->handle == MPI_INFO_NULL)
        goto fn_exit;

    MPIR_Info_get_nkeys_impl(info_ptr, &nkeys);

    if (nkeys == 0)
        goto fn_exit;

    kv = (PMI_keyval_t *) MPL_malloc(nkeys * sizeof(PMI_keyval_t), MPL_MEM_BUFFER);

    for (int i = 0; i < nkeys; i++) {
        mpi_errno = MPIR_Info_get_nthkey_impl(info_ptr, i, key);
        MPIR_ERR_CHECK(mpi_errno);
        MPIR_Info_get_valuelen_impl(info_ptr, key, &vallen, &flag);
        kv[i].key = MPL_strdup(key);
        kv[i].val = (char *) MPL_malloc(vallen + 1, MPL_MEM_BUFFER);
        MPIR_Info_get_impl(info_ptr, key, vallen + 1, kv[i].val, &flag);
    }

  fn_exit:
    *kv_ptr = kv;
    *nkeys_ptr = nkeys;
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPI_TO_PMI_KEYVALS);
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

static void free_pmi_keyvals(PMI_keyval_t ** kv, int size, int *counts)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_FREE_PMI_KEYVALS);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_FREE_PMI_KEYVALS);

    for (int i = 0; i < size; i++) {
        for (int j = 0; j < counts[i]; j++) {
            MPL_free((char *) kv[i][j].key);
            MPL_free(kv[i][j].val);
        }
        MPL_free(kv[i]);
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_FREE_PMI_KEYVALS);
}
#endif /* USE_PMI1_API or USE_PMI2_API */
