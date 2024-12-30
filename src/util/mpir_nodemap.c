/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef BUILD_NODEMAP_H_INCLUDED
#define BUILD_NODEMAP_H_INCLUDED

#include "mpiimpl.h"
#include "mpir_pmi.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

categories:
    - name        : NODEMAP
      description : cvars that control behavior of nodemap

cvars:
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

/* Allocate and populate MPIR_Process.node_local_map and MPIR_Process.node_root_map */
int MPIR_build_locality(void)
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

/* MPIR_build_nodemap - build MPIR_Process.node_map, an table for rank to node_id mapping */

static int get_option_no_local(void);
static int get_option_num_cliques(void);
static int build_nodemap_nolocal(int *nodemap, int sz, int *num_nodes);
static int build_nodemap_roundrobin(int num_cliques, int *nodemap, int sz, int *num_nodes);
static int build_nodemap_byblock(int num_cliques, int *nodemap, int sz, int *num_nodes);

/* TODO: if the process manager promises persistent node_id across multiple spawns,
 *       we can use the node id to check intranode processes across comm worlds.
 *       Currently we don't do this check and all dynamic processes are treated as
 *       inter-node. When we add the optimization, we should switch off the flag
 *       when appropriate environment variable from process manager is set.
 */
static bool do_normalize_nodemap = true;

int MPIR_build_nodemap(int *nodemap, int sz, int *num_nodes)
{
    int mpi_errno = MPI_SUCCESS;

    if (sz == 1 || get_option_no_local()) {
        mpi_errno = build_nodemap_nolocal(nodemap, sz, num_nodes);
        goto fn_exit;
    }
    mpi_errno = MPIR_pmi_build_nodemap(nodemap, sz);
    MPIR_ERR_CHECK(mpi_errno);

    if (do_normalize_nodemap) {
        /* node ids from process manager may not start from 0 or has gaps.
         * Normalize it since most of the code assume a contiguous node id range */
        struct nodeid_hash {
            int old_id;
            int new_id;
            UT_hash_handle hh;
        };

        struct nodeid_hash *nodes = MPL_malloc(sz * sizeof(struct nodeid_hash), MPL_MEM_OTHER);
        MPIR_Assert(nodes);

        struct nodeid_hash *nodeid_hash = NULL;
        int next_node_id = 0;
        for (int i = 0; i < sz; i++) {
            int old_id = nodemap[i];

            struct nodeid_hash *s;
            HASH_FIND_INT(nodeid_hash, &old_id, s);
            if (s == NULL) {
                nodemap[i] = next_node_id;
                nodes[i].old_id = old_id;
                nodes[i].new_id = next_node_id;
                HASH_ADD_INT(nodeid_hash, old_id, &nodes[i], MPL_MEM_OTHER);
                next_node_id++;
            } else {
                nodemap[i] = s->new_id;
            }
        }
        *num_nodes = next_node_id;
        HASH_CLEAR(hh, nodeid_hash);
        MPL_free(nodes);
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
    return (get_option_num_cliques() > 1) || get_option_no_local();
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

int MPIR_pmi_build_nodemap_fallback(int sz, int myrank, int *out_nodemap);

/* this function is not used in pmix */
static int pmi_publish_node_id(int sz, int myrank)
{
    int mpi_errno = MPI_SUCCESS;
    int ret;
    char *key;
    int key_max_sz;
    char hostname[MAX_HOSTNAME_LEN];
    char strerrbuf[MPIR_STRERROR_BUF_SIZE] ATTRIBUTE((unused));
    MPIR_CHKLMEM_DECL();

    /* set hostname */

    ret = gethostname(hostname, MAX_HOSTNAME_LEN);
    MPIR_ERR_CHKANDJUMP2(ret == -1, mpi_errno, MPI_ERR_OTHER, "**sock_gethost",
                         "**sock_gethost %s %d",
                         MPIR_Strerror(errno, strerrbuf, MPIR_STRERROR_BUF_SIZE), errno);
    hostname[MAX_HOSTNAME_LEN - 1] = '\0';

    /* Allocate space for pmi key */
    key_max_sz = MPIR_pmi_max_key_size();
    MPIR_CHKLMEM_MALLOC(key, key_max_sz);

    /* Put my hostname id */
    if (sz > 1) {
        memset(key, 0, key_max_sz);
        snprintf(key, key_max_sz, "hostname[%d]", myrank);

        mpi_errno = MPIR_pmi_kvs_put(key, hostname);
        MPIR_ERR_CHECK(mpi_errno);

        mpi_errno = MPIR_pmi_barrier();
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* Fills in the node_id info from PMI info.  Adapted from MPIU_Get_local_procs.
   This function is collective over the entire PG because PMI_Barrier is called.

   myrank should be set to -1 if this is not the current process' PG.  This
   is currently not supported due to PMI limitations.

   Fallback Algorithm:

   Each process kvs_puts its hostname and stores the total number of
   processes (g_num_global).  Each process determines maximum node id
   and assigns a node id to each process (g_node_ids[]):

     For each hostname the process searches the list of unique nodes
     names (node_names[]) for a match.  If a match is found, the node id
     is recorded for that matching process.  Otherwise, the hostname is
     added to the list of node names.
*/

int MPIR_pmi_build_nodemap_fallback(int sz, int myrank, int *out_nodemap)
{
    int mpi_errno = MPI_SUCCESS;

    int key_max_sz = MPIR_pmi_max_key_size();
    char *key = MPL_malloc(key_max_sz, MPL_MEM_OTHER);
    char **node_names = MPL_malloc(sz * sizeof(char *), MPL_MEM_OTHER);
    char *node_name_buf = MPL_malloc(sz * key_max_sz, MPL_MEM_OTHER);
    char strerrbuf[MPIR_STRERROR_BUF_SIZE] ATTRIBUTE((unused));

    for (int i = 0; i < sz; ++i) {
        node_names[i] = &node_name_buf[i * key_max_sz];
        node_names[i][0] = '\0';
    }

    mpi_errno = pmi_publish_node_id(sz, myrank);
    MPIR_ERR_CHECK(mpi_errno);

    int max_node_id = -1;

    for (int i = 0; i < sz; ++i) {
        MPIR_Assert(max_node_id < sz);
        if (i == myrank) {
            /* This is us, no need to perform a get */
            int ret;
            char *hostname = (char *) MPL_malloc(sizeof(char) * MAX_HOSTNAME_LEN, MPL_MEM_ADDRESS);
            ret = gethostname(hostname, MAX_HOSTNAME_LEN);
            MPIR_ERR_CHKANDJUMP2(ret == -1, mpi_errno, MPI_ERR_OTHER, "**sock_gethost",
                                 "**sock_gethost %s %d",
                                 MPIR_Strerror(errno, strerrbuf, MPIR_STRERROR_BUF_SIZE), errno);
            hostname[MAX_HOSTNAME_LEN - 1] = '\0';
            snprintf(node_names[max_node_id + 1], key_max_sz, "%s", hostname);
            MPL_free(hostname);
        } else {
            memset(key, 0, key_max_sz);
            snprintf(key, key_max_sz, "hostname[%d]", i);

            mpi_errno = MPIR_pmi_kvs_get(i, key, node_names[max_node_id + 1], key_max_sz);
            MPIR_ERR_CHECK(mpi_errno);
        }

        /* Find the node_id for this process, or create a new one */
        /* FIXME:need a better algorithm -- this one does O(N^2) strncmp()s! */
        /* The right fix is to get all this information from the process
         * manager, rather than bother with this hostname hack at all. */
        int j;
        for (j = 0; j < max_node_id + 1; ++j) {
            if (strncmp(node_names[j], node_names[max_node_id + 1], key_max_sz) == 0)
                break;
        }
        if (j == max_node_id + 1)
            ++max_node_id;
        else
            node_names[max_node_id + 1][0] = '\0';
        out_nodemap[i] = j;
    }

  fn_exit:
    MPL_free(key);
    MPL_free(node_names);
    MPL_free(node_name_buf);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* nodeid utilities to support dynamic process node id */
UT_icd hostname_icd = { MAX_HOSTNAME_LEN, NULL, NULL, NULL };

/* Lookup node_id in MPIR_Process.node_hostnames, insert and return next_node_id if it's new hostname */
int MPIR_nodeid_lookup(const char *hostname, int *node_id)
{
    if (MPIR_pmi_has_local_cliques()) {
        *node_id = -1;
        goto fn_exit;
    }

    int len = utarray_len(MPIR_Process.node_hostnames);
    for (int i = 0; i < len; i++) {
        char *s = utarray_eltptr(MPIR_Process.node_hostnames, i);
        if (strcmp(hostname, s) == 0) {
            *node_id = i;
            goto fn_exit;
        }
    }

    /* append as new entry */
    utarray_extend_back(MPIR_Process.node_hostnames, MPL_MEM_OTHER);
    char *buf = utarray_back(MPIR_Process.node_hostnames);
    strcpy(buf, hostname);
    *node_id = len;

  fn_exit:
    return MPI_SUCCESS;
}


/* Initialize MPIR_Process.node_hostnames after comm_world is initialized */
int MPIR_nodeid_init(void)
{
    int mpi_errno = MPI_SUCCESS;

    if (MPIR_pmi_has_local_cliques()) {
        goto fn_exit;
    }

    utarray_new(MPIR_Process.node_hostnames, &hostname_icd, MPL_MEM_OTHER);
    utarray_resize(MPIR_Process.node_hostnames, MPIR_Process.num_nodes, MPL_MEM_OTHER);
    char *allhostnames = (char *) utarray_eltptr(MPIR_Process.node_hostnames, 0);

    if (MPIR_Process.local_rank == 0) {
        MPIR_Comm *node_roots_comm = MPIR_Process.comm_world->node_roots_comm;
        if (node_roots_comm == NULL) {
            /* num_external == comm->remote_size */
            node_roots_comm = MPIR_Process.comm_world;
        }

        char *my_hostname = allhostnames + MAX_HOSTNAME_LEN * node_roots_comm->rank;
        int ret = gethostname(my_hostname, MAX_HOSTNAME_LEN);
        char strerrbuf[MPIR_STRERROR_BUF_SIZE] ATTRIBUTE((unused));
        MPIR_ERR_CHKANDJUMP2(ret == -1, mpi_errno, MPI_ERR_OTHER,
                             "**sock_gethost", "**sock_gethost %s %d",
                             MPIR_Strerror(errno, strerrbuf, MPIR_STRERROR_BUF_SIZE), errno);
        my_hostname[MAX_HOSTNAME_LEN - 1] = '\0';

        mpi_errno = MPIR_Allgather_impl(MPI_IN_PLACE, MAX_HOSTNAME_LEN, MPI_CHAR,
                                        allhostnames, MAX_HOSTNAME_LEN, MPI_CHAR,
                                        node_roots_comm, MPIR_ERR_NONE);
        MPIR_ERR_CHECK(mpi_errno);
    }

    MPIR_Comm *node_comm = MPIR_Process.comm_world->node_comm;
    if (node_comm) {
        mpi_errno = MPIR_Bcast_impl(allhostnames, MAX_HOSTNAME_LEN * MPIR_Process.num_nodes,
                                    MPI_CHAR, 0, node_comm, MPIR_ERR_NONE);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_nodeid_free(void)
{
    if (MPIR_pmi_has_local_cliques()) {
        goto fn_exit;
    }

    utarray_free(MPIR_Process.node_hostnames);

  fn_exit:
    return MPI_SUCCESS;
}

#endif /* BUILD_NODEMAP_H_INCLUDED */
