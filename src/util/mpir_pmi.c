/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include <mpir_pmi.h>
#include <mpiimpl.h>
#include "mpir_nodemap.h"
#include "uthash.h"     /* for hash function */

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

categories:
    - name        : NODEMAP
      description : cvars that control behavior of nodemap

cvars:
    - name        : MPIR_CVAR_PMI_VERSION
      category    : NODEMAP
      type        : enum
      default     : 1
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : |-
        Variable to select runtime PMI version.
        1        - PMI (default)
        2        - PMI2
        x        - PMIx

    - name        : MPIR_CVAR_NOLOCAL
      category    : NODEMAP
      alt-env     : MPIR_CVAR_NO_LOCAL
      type        : boolean
      default     : false
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        If true, force all processes to operate as though all processes
        are located on another node.  For example, this disables shared
        memory communication hierarchical collectives.

    - name        : MPIR_CVAR_ODD_EVEN_CLIQUES
      category    : NODEMAP
      alt-env     : MPIR_CVAR_EVEN_ODD_CLIQUES
      type        : boolean
      default     : false
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        If true, odd procs on a node are seen as local to each other, and even
        procs on a node are seen as local to each other.  Used for debugging on
        a single machine. Deprecated in favor of MPIR_CVAR_NUM_CLIQUES.

    - name        : MPIR_CVAR_NUM_CLIQUES
      category    : NODEMAP
      type        : int
      default     : 1
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Specify the number of cliques that should be used to partition procs on
        a local node. Procs with the same clique number are seen as local to
        each other. Used for debugging on a single machine.

    - name        : MPIR_CVAR_CLIQUES_BY_BLOCK
      category    : NODEMAP
      type        : boolean
      default     : false
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Specify to divide processes into cliques by uniform blocks. The default
        is to divide in round-robin fashion. Used for debugging on a single machine.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

/* -- define INFO_TYPE for PMI1 and PMI2 -- */
#ifdef USE_PMI2_SLURM
/* configure will only define ENABLE_PMI2 */
#define INFO_TYPE PMI2U_Info
#define INFO_TYPE_KEY(kv) (kv).key
#define INFO_TYPE_VAL(kv) (kv).value

#else
/* PMI_keyval_t and PMI2_keyval_t are compatible */
typedef struct {
    const char *key;
    const char *val;
} INFO_TYPE;
#define INFO_TYPE_KEY(kv) (kv).key
#define INFO_TYPE_VAL(kv) (kv).val

#endif /* define INFO_TYPE */

static int mpi_to_pmi_keyvals(MPIR_Info * info_ptr, INFO_TYPE ** kv_ptr, int *nkeys_ptr);
static int get_info_kv_vectors(int count, MPIR_Info * info_ptrs[],
                               INFO_TYPE *** kv_vectors, int **kv_sizes);
static void free_pmi_keyvals(INFO_TYPE ** kv, int size, int *counts);

static int build_nodemap(int *nodemap, int sz, int *num_nodes);
static int build_locality(void);

static int pmi_version = 1;
static int pmi_subversion = 1;

static int pmi_max_key_size;
static int pmi_max_val_size;

static char *pmi_kvs_name;

static char *hwloc_topology_xmlfile;

#include "mpir_pmi1.inc"
#include "mpir_pmi2.inc"
#include "mpir_pmix.inc"

#define SWITCH_PMI(call_pmi1, call_pmi2, call_pmix) \
    switch (MPIR_CVAR_PMI_VERSION) { \
        case MPIR_CVAR_PMI_VERSION_1: \
            call_pmi1; \
            break; \
        case MPIR_CVAR_PMI_VERSION_2: \
            call_pmi2; \
            break; \
        case MPIR_CVAR_PMI_VERSION_x: \
            call_pmix; \
            break; \
        default: \
            MPIR_Assert(0); \
            break; \
    }


static int check_MPIR_CVAR_PMI_VERSION(void)
{
    /* Adjust MPIR_CVAR_PMI_VERSION if the default is disabled */
#ifndef ENABLE_PMI1
    if (MPIR_CVAR_PMI_VERSION == MPIR_CVAR_PMI_VERSION_1) {
#if defined(ENABLE_PMI2)
        MPIR_CVAR_PMI_VERSION = MPIR_CVAR_PMI_VERSION_2;
#elif defined(ENABLE_PMIX)
        MPIR_CVAR_PMI_VERSION = MPIR_CVAR_PMI_VERSION_x;
#else
        return MPI_ERR_INTERN;
#endif
    }
#endif

    /* Error if user select PMI2 but it is disabled */
#ifndef ENABLE_PMI2
    if (MPIR_CVAR_PMI_VERSION == MPIR_CVAR_PMI_VERSION_2) {
        return MPI_ERR_INTERN;
    }
#endif

    /* Error if user select PMI2 but it is disabled */
#ifndef ENABLE_PMIX
    if (MPIR_CVAR_PMI_VERSION == MPIR_CVAR_PMI_VERSION_x) {
        return MPI_ERR_INTERN;
    }
#endif

    return MPI_SUCCESS;
}

static void MPIR_pmi_finalize_on_exit(void)
{
    SWITCH_PMI(pmi1_exit(), pmi2_exit(), pmix_exit());
}

int MPIR_pmi_init(void)
{
    int mpi_errno = MPI_SUCCESS;
    static bool pmi_connected = false;

    mpi_errno = check_MPIR_CVAR_PMI_VERSION();
    MPIR_ERR_CHECK(mpi_errno);

    /* See if the user wants to override our default values */
    MPL_env2int("PMI_VERSION", &pmi_version);
    MPL_env2int("PMI_SUBVERSION", &pmi_subversion);

    int has_parent, rank, size, appnum;
    SWITCH_PMI(mpi_errno = pmi1_init(&has_parent, &rank, &size, &appnum),
               mpi_errno = pmi2_init(&has_parent, &rank, &size, &appnum),
               mpi_errno = pmix_init(&has_parent, &rank, &size, &appnum));
    MPIR_ERR_CHECK(mpi_errno);

    unsigned world_id = 0;
    if (pmi_kvs_name) {
        HASH_FNV(pmi_kvs_name, strlen(pmi_kvs_name), world_id);
    }

    if (!pmi_connected) {
        /* Register finalization of PM connection in exit handler */
        mpi_errno = atexit(MPIR_pmi_finalize_on_exit);
        MPIR_ERR_CHKANDJUMP1(mpi_errno != 0, mpi_errno, MPI_ERR_OTHER,
                             "**atexit_pmi_finalize", "**atexit_pmi_finalize %d", mpi_errno);

        pmi_connected = true;
    }

    MPIR_Process.has_parent = has_parent;
    MPIR_Process.rank = rank;
    MPIR_Process.size = size;
    MPIR_Process.appnum = appnum;
    MPIR_Process.world_id = world_id;

    MPIR_Process.node_map = (int *) MPL_malloc(size * sizeof(int), MPL_MEM_ADDRESS);

    mpi_errno = build_nodemap(MPIR_Process.node_map, size, &MPIR_Process.num_nodes);
    MPIR_ERR_CHECK(mpi_errno);

    /* allocate and populate MPIR_Process.node_local_map and MPIR_Process.node_root_map */
    mpi_errno = build_locality();

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

void MPIR_pmi_finalize(void)
{
    /* Finalize of PM interface happens in exit handler,
     * here: free allocated memory */
    MPL_free(pmi_kvs_name);

    MPL_free(MPIR_Process.node_map);
    MPL_free(MPIR_Process.node_root_map);
    MPL_free(MPIR_Process.node_local_map);

    MPL_free(hwloc_topology_xmlfile);
    hwloc_topology_xmlfile = NULL;
}

void MPIR_pmi_abort(int exit_code, const char *error_msg)
{
    SWITCH_PMI(pmi1_abort(exit_code, error_msg),
               pmi2_abort(exit_code, error_msg), pmix_abort(exit_code, error_msg));
}

/* This function is currently unused in MPICH because we always call
 * PMI functions from a single thread or within a critical section.
 */
int MPIR_pmi_set_threaded(int is_threaded)
{
    if (MPIR_CVAR_PMI_VERSION == MPIR_CVAR_PMI_VERSION_2) {
#if ENABLE_PMI2
        PMI2_Set_threaded(is_threaded);
#endif
    }
    return MPI_SUCCESS;
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
    return (const char *) pmi_kvs_name;
}

/* wrapper functions */
int MPIR_pmi_kvs_put(const char *key, const char *val)
{
    int mpi_errno = MPI_SUCCESS;

    SWITCH_PMI(mpi_errno = pmi1_put(key, val),
               mpi_errno = pmi2_put(key, val), mpi_errno = pmix_put(key, val));
    return mpi_errno;
}

/* NOTE: src is a hint, use src = -1 if not known */
int MPIR_pmi_kvs_get(int src, const char *key, char *val, int val_size)
{
    int mpi_errno = MPI_SUCCESS;

    SWITCH_PMI(mpi_errno = pmi1_get(src, key, val, val_size),
               mpi_errno = pmi2_get(src, key, val, val_size),
               mpi_errno = pmix_get(src, key, val, val_size));
    return mpi_errno;
}

char *MPIR_pmi_get_jobattr(const char *key)
{
    char *valbuf = NULL;
    valbuf = MPL_malloc(pmi_max_val_size, MPL_MEM_OTHER);
    if (!valbuf) {
        goto fn_exit;
    }

    bool found = false;
    SWITCH_PMI(found = pmi1_get_jobattr(key, valbuf),
               found = pmi2_get_jobattr(key, valbuf), found = pmix_get_jobattr(key, valbuf));

    if (!found) {
        MPL_free(valbuf);
        valbuf = NULL;
    }

  fn_exit:
    return valbuf;
}

/* ---- utils functions ---- */

int MPIR_pmi_barrier(void)
{
    int mpi_errno = MPI_SUCCESS;
    SWITCH_PMI(mpi_errno = pmi1_barrier(), mpi_errno = pmi2_barrier(), mpi_errno = pmix_barrier());
    return mpi_errno;
}

int MPIR_pmi_barrier_local(void)
{
    int mpi_errno = MPI_SUCCESS;
    SWITCH_PMI(mpi_errno = pmi1_barrier_local(),
               mpi_errno = pmi2_barrier_local(), mpi_errno = pmix_barrier_local());
    return mpi_errno;
}

/* declare static functions used in bcast/allgather */
static void encode(int size, const char *src, char *dest);
static void decode(int size, const char *src, char *dest);

/* is_local is a hint that we optimize for node local access when we can */
static int optimized_put(const char *key, const char *val, int is_local)
{
    int mpi_errno = MPI_SUCCESS;
    SWITCH_PMI(mpi_errno = pmi1_optimized_put(key, val, is_local),
               mpi_errno = pmi2_optimized_put(key, val, is_local),
               mpi_errno = pmix_optimized_put(key, val, is_local));
    return mpi_errno;
}

static int optimized_get(int src, const char *key, char *val, int valsize, int is_local)
{
    int mpi_errno = MPI_SUCCESS;
    SWITCH_PMI(mpi_errno = pmi1_optimized_get(src, key, val, valsize, is_local),
               mpi_errno = pmi2_optimized_get(src, key, val, valsize, is_local),
               mpi_errno = pmix_optimized_get(src, key, val, valsize, is_local));
    return mpi_errno;
}

/* higher-level binary put/get:
 * 1. binary encoding/decoding
 * 2. chops long values into multiple segments
 * 3. uses optimized_put/get for the case of node-level access
 */
static int put_ex_segs(const char *key, const void *buf, int bufsize, int is_local)
{
    int mpi_errno = MPI_SUCCESS;
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
        snprintf(val, pmi_max_val_size, "segments=%d", num_segs);
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

  fn_exit:
    MPL_free(val);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int get_ex_segs(int src, const char *key, void *buf, int *p_size, int is_local)
{
    int mpi_errno = MPI_SUCCESS;

    int bufsize = *p_size;
    char *val = MPL_malloc(pmi_max_val_size, MPL_MEM_OTHER);
    int segsize = (pmi_max_val_size - 1) / 2;

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

static int put_ex(const char *key, const void *buf, int bufsize, int is_local)
{
    int mpi_errno = MPI_SUCCESS;
    SWITCH_PMI(mpi_errno = put_ex_segs(key, buf, bufsize, is_local),
               mpi_errno = put_ex_segs(key, buf, bufsize, is_local),
               mpi_errno = pmix_put_binary(key, buf, bufsize, is_local));
    return mpi_errno;
}

static int get_ex(int src, const char *key, void *buf, int *p_size, int is_local)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Assert(p_size);
    MPIR_Assert(*p_size > 0);
    SWITCH_PMI(mpi_errno = get_ex_segs(src, key, buf, p_size, is_local),
               mpi_errno = get_ex_segs(src, key, buf, p_size, is_local),
               mpi_errno = pmix_get_binary(src, key, buf, p_size, is_local));
    return mpi_errno;
}

static int optional_bcast_barrier(MPIR_PMI_DOMAIN domain)
{
    /* unless bcast is skipped altogether */
    if (domain == MPIR_PMI_DOMAIN_ALL && MPIR_Process.size == 1) {
        return MPI_SUCCESS;
    } else if (domain == MPIR_PMI_DOMAIN_NODE_ROOTS && MPIR_Process.num_nodes == 1) {
        return MPI_SUCCESS;
    } else if (domain == MPIR_PMI_DOMAIN_LOCAL && MPIR_Process.size == MPIR_Process.num_nodes) {
        return MPI_SUCCESS;
    }

    int mpi_errno = MPI_SUCCESS;
    SWITCH_PMI(mpi_errno = pmi1_optional_bcast_barrier(domain),
               mpi_errno = pmi2_optional_bcast_barrier(domain),
               mpi_errno = pmix_optional_bcast_barrier(domain));
    return mpi_errno;
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
    if (MPIR_CVAR_PMI_VERSION != MPIR_CVAR_PMI_VERSION_x) {
        /* PMIx will wait, so barrier unnecessary */
        mpi_errno = MPIR_pmi_barrier();
        MPIR_ERR_CHECK(mpi_errno);
    }
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
    SWITCH_PMI(mpi_errno = pmi1_get_universe_size(universe_size),
               mpi_errno = pmi2_get_universe_size(universe_size),
               mpi_errno = pmix_get_universe_size(universe_size));
    return mpi_errno;
}

/* NOTE: MPIR_pmi_spawn_multiple is to be called by a single root spawning process */
int MPIR_pmi_spawn_multiple(int count, char *commands[], char **argvs[],
                            const int maxprocs[], MPIR_Info * info_ptrs[],
                            int num_preput_keyval, struct MPIR_PMI_KEYVAL *preput_keyvals,
                            int *pmi_errcodes)
{
    int mpi_errno = MPI_SUCCESS;
#ifdef NO_PMI_SPAWN_MULTIPLE
    /* legacy bgq system does not have PMI_Spawn_multiple */
    MPIR_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER,
                         "**pmi_spawn_multiple", "**pmi_spawn_multiple %d", 0);
#elif defined(USE_PMI2_SLURM)
    mpi_errno = pmi2_spawn_slurm(count, commands, argvs, maxprocs, info_ptrs,
                                 num_preput_keyval, preput_keyvals, pmi_errcodes);
#else
    SWITCH_PMI(mpi_errno = pmi1_spawn(count, commands, argvs, maxprocs, info_ptrs,
                                      num_preput_keyval, preput_keyvals, pmi_errcodes),
               mpi_errno = pmi2_spawn(count, commands, argvs, maxprocs, info_ptrs,
                                      num_preput_keyval, preput_keyvals, pmi_errcodes),
               mpi_errno = pmix_spawn(count, commands, argvs, maxprocs, info_ptrs,
                                      num_preput_keyval, preput_keyvals, pmi_errcodes));
#endif
    return mpi_errno;
}

int MPIR_pmi_publish(const char name[], const char port[])
{
    int mpi_errno = MPI_SUCCESS;
    SWITCH_PMI(mpi_errno = pmi1_publish(name, port),
               mpi_errno = pmi2_publish(name, port), mpi_errno = pmix_publish(name, port));
    return mpi_errno;
}

int MPIR_pmi_lookup(const char name[], char port[])
{
    int mpi_errno = MPI_SUCCESS;
    SWITCH_PMI(mpi_errno = pmi1_lookup(name, port),
               mpi_errno = pmi2_lookup(name, port), mpi_errno = pmix_lookup(name, port));
    return mpi_errno;
}

int MPIR_pmi_unpublish(const char name[])
{
    int mpi_errno = MPI_SUCCESS;
    SWITCH_PMI(mpi_errno = pmi1_unpublish(name),
               mpi_errno = pmi2_unpublish(name), mpi_errno = pmix_unpublish(name));
    return mpi_errno;
}

/* ---- static functions ---- */

/* The following static function declares are only for build_nodemap() */
static int get_option_no_local(void);
static int get_option_num_cliques(void);
static int build_nodemap_nolocal(int *nodemap, int sz, int *num_nodes);
static int build_nodemap_roundrobin(int num_cliques, int *nodemap, int sz, int *num_nodes);
static int build_nodemap_byblock(int num_cliques, int *nodemap, int sz, int *num_nodes);

static int pmi_build_nodemap(int *nodemap, int sz);

/* TODO: if the process manager promises persistent node_id across multiple spawns,
 *       we can use the node id to check intranode processes across comm worlds.
 *       Currently we don't do this check and all dynamic processes are treated as
 *       inter-node. When we add the optimization, we should switch off the flag
 *       when appropriate environment variable from process manager is set.
 */
static bool do_normalize_nodemap = true;

static int build_nodemap(int *nodemap, int sz, int *num_nodes)
{
    int mpi_errno = MPI_SUCCESS;

    if (sz == 1 || get_option_no_local()) {
        mpi_errno = build_nodemap_nolocal(nodemap, sz, num_nodes);
        goto fn_exit;
    }
    mpi_errno = pmi_build_nodemap(nodemap, sz);
    MPIR_ERR_CHECK(mpi_errno);

    if (do_normalize_nodemap) {
        /* node ids from process manager may not start from 0 or has gaps.
         * Normalize it since most of the code assume a contiguous node id range */
        int max_id = -1;
        for (int i = 0; i < sz; i++) {
            if (max_id < nodemap[i]) {
                max_id = nodemap[i];
            }
        }
        int *nodeids = MPL_malloc((max_id + 1) * sizeof(int), MPL_MEM_OTHER);
        for (int i = 0; i < max_id + 1; i++) {
            nodeids[i] = -1;
        }
        int next_node_id = 0;
        for (int i = 0; i < sz; i++) {
            int old_id = nodemap[i];
            if (nodeids[old_id] == -1) {
                nodeids[old_id] = next_node_id;
                next_node_id++;
            }
            nodemap[i] = nodeids[old_id];
        }
        *num_nodes = next_node_id;
        MPL_free(nodeids);
    }

    /* local cliques */
    int num_cliques = get_option_num_cliques();
    if (num_cliques > sz) {
        num_cliques = sz;
    }
    if (*num_nodes == 1 && num_cliques > 1) {
        if (MPIR_CVAR_CLIQUES_BY_BLOCK) {
            mpi_errno = build_nodemap_byblock(num_cliques, nodemap, sz, num_nodes);
        } else {
            mpi_errno = build_nodemap_roundrobin(num_cliques, nodemap, sz, num_nodes);
        }
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

int MPIR_pmi_has_local_cliques(void)
{
    return (get_option_num_cliques() > 1);
}

/* one process per node */
int build_nodemap_nolocal(int *nodemap, int sz, int *num_nodes)
{
    for (int i = 0; i < sz; ++i) {
        nodemap[i] = i;
    }
    *num_nodes = sz;
    return MPI_SUCCESS;
}

/* assign processes to num_cliques nodes in a round-robin fashion */
static int build_nodemap_roundrobin(int num_cliques, int *nodemap, int sz, int *num_nodes)
{
    for (int i = 0; i < sz; ++i) {
        nodemap[i] = i % num_cliques;
    }
    *num_nodes = (sz >= num_cliques) ? num_cliques : sz;
    return MPI_SUCCESS;
}

/* assign processes to num_cliques nodes by uniform block */
static int build_nodemap_byblock(int num_cliques, int *nodemap, int sz, int *num_nodes)
{
    int block_size = sz / num_cliques;
    int remainder = sz % num_cliques;
    /* The first `remainder` cliques have size `block_size + 1` */
    int middle = (block_size + 1) * remainder;
    for (int i = 0; i < sz; ++i) {
        if (i < middle) {
            nodemap[i] = i / (block_size + 1);
        } else {
            nodemap[i] = (i - remainder) / block_size;
        }
    }
    *num_nodes = (sz >= num_cliques) ? num_cliques : sz;
    return MPI_SUCCESS;
}

static int pmi_build_nodemap(int *nodemap, int sz)
{
    int mpi_errno = MPI_SUCCESS;
    if (MPIR_CVAR_PMI_VERSION == MPIR_CVAR_PMI_VERSION_x) {
        mpi_errno = pmix_build_nodemap(nodemap, sz);
    } else {
        char *process_mapping = MPIR_pmi_get_jobattr("PMI_process_mapping");
        if (process_mapping) {
            int mpl_err = MPL_rankmap_str_to_array(process_mapping, sz, nodemap);
            MPIR_ERR_CHKINTERNAL(mpl_err, mpi_errno,
                                 "unable to populate node ids from PMI_process_mapping");
            MPL_free(process_mapping);
        } else {
            /* build nodemap based on allgather hostnames */
            /* FIXME: migrate the function */
            mpi_errno = MPIR_NODEMAP_build_nodemap_fallback(sz, MPIR_Process.rank, nodemap);
        }
    }
  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

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
        snprintf(dest, 3, "%02X", (unsigned char) *src);
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

static int mpi_to_pmi_keyvals(MPIR_Info * info_ptr, INFO_TYPE ** kv_ptr, int *nkeys_ptr)
{
    char key[MPI_MAX_INFO_KEY];
    INFO_TYPE *kv = 0;
    int nkeys = 0, vallen, flag, mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    if (!info_ptr || info_ptr->handle == MPI_INFO_NULL)
        goto fn_exit;

    MPIR_Info_get_nkeys_impl(info_ptr, &nkeys);

    if (nkeys == 0)
        goto fn_exit;

    kv = (INFO_TYPE *) MPL_malloc(nkeys * sizeof(INFO_TYPE), MPL_MEM_BUFFER);

    for (int i = 0; i < nkeys; i++) {
        mpi_errno = MPIR_Info_get_nthkey_impl(info_ptr, i, key);
        MPIR_ERR_CHECK(mpi_errno);
        MPIR_Info_get_valuelen_impl(info_ptr, key, &vallen, &flag);

        char *s_val;
        s_val = (char *) MPL_malloc(vallen + 1, MPL_MEM_BUFFER);
        MPIR_Info_get_impl(info_ptr, key, vallen + 1, s_val, &flag);

        INFO_TYPE_KEY(kv[i]) = MPL_strdup(key);
        INFO_TYPE_VAL(kv[i]) = s_val;
    }

  fn_exit:
    *kv_ptr = kv;
    *nkeys_ptr = nkeys;
    MPIR_FUNC_EXIT;
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

static int get_info_kv_vectors(int count, MPIR_Info * info_ptrs[],
                               INFO_TYPE *** kv_vectors, int **kv_sizes)
{
    int mpi_errno = MPI_SUCCESS;

    (*kv_sizes) = (int *) MPL_malloc(count * sizeof(int), MPL_MEM_BUFFER);
    MPIR_ERR_CHKANDJUMP(!(*kv_sizes), mpi_errno, MPI_ERR_OTHER, "**nomem");

    (*kv_vectors) = (INFO_TYPE **) MPL_malloc(count * sizeof(INFO_TYPE *), MPL_MEM_BUFFER);
    MPIR_ERR_CHKANDJUMP(!(*kv_vectors), mpi_errno, MPI_ERR_OTHER, "**nomem");

    if (!info_ptrs) {
        for (int i = 0; i < count; i++) {
            (*kv_vectors)[i] = 0;
            (*kv_sizes)[i] = 0;
        }
    } else {
        for (int i = 0; i < count; i++) {
            mpi_errno = mpi_to_pmi_keyvals(info_ptrs[i], &(*kv_vectors)[i], &(*kv_sizes)[i]);
            MPIR_ERR_CHECK(mpi_errno);
        }
    }
  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static void free_pmi_keyvals(INFO_TYPE ** kv, int size, int *counts)
{
    if (kv) {
        for (int i = 0; i < size; i++) {
            for (int j = 0; j < counts[i]; j++) {
                /* cast the "const" away */
                MPL_free((char *) INFO_TYPE_KEY(kv[i][j]));
                MPL_free((char *) INFO_TYPE_VAL(kv[i][j]));
            }
            MPL_free(kv[i]);
        }
        MPL_free(kv);
        MPL_free(counts);
    }
}
