/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2016 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef BUILD_NODEMAP_H_INCLUDED
#define BUILD_NODEMAP_H_INCLUDED

#include "mpl.h"

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
        a single machine.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

#if !defined(USE_PMI2_API)
/* this function is not used in pmi2 */
#undef FUNCNAME
#define FUNCNAME MPIR_NODEMAP_publish_node_id
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIR_NODEMAP_publish_node_id(int sz, int myrank)
{
    int mpi_errno = MPI_SUCCESS;
    int pmi_errno;
    int ret;
    char *key;
    int key_max_sz;
    char *kvs_name;
    char hostname[MAX_HOSTNAME_LEN];
    MPIR_CHKLMEM_DECL(2);

    /* set hostname */

    ret = gethostname(hostname, MAX_HOSTNAME_LEN);
    MPIR_ERR_CHKANDJUMP2(ret == -1, mpi_errno, MPI_ERR_OTHER, "**sock_gethost", "**sock_gethost %s %d", MPIR_Strerror(errno), errno);
    hostname[MAX_HOSTNAME_LEN-1] = '\0';

    /* Allocate space for pmi key */
    pmi_errno = PMI_KVS_Get_key_length_max(&key_max_sz);
    MPIR_ERR_CHKANDJUMP1(pmi_errno, mpi_errno, MPI_ERR_OTHER, "**fail", "**fail %d", pmi_errno);

    MPIR_CHKLMEM_MALLOC(key, char *, key_max_sz, mpi_errno, "key");

    MPIR_CHKLMEM_MALLOC(kvs_name, char *, 256, mpi_errno, "kvs_name");
    pmi_errno = PMI_KVS_Get_my_name(kvs_name, 256);
    MPIR_ERR_CHKANDJUMP1(pmi_errno, mpi_errno, MPI_ERR_OTHER, "**fail", "**fail %d", pmi_errno);
    /* Put my hostname id */
    if (sz > 1)
    {
        memset(key, 0, key_max_sz);
        MPL_snprintf(key, key_max_sz, "hostname[%d]", myrank);

        pmi_errno = PMI_KVS_Put(kvs_name, key, hostname);
        MPIR_ERR_CHKANDJUMP1(pmi_errno != PMI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**pmi_kvs_put", "**pmi_kvs_put %d", pmi_errno);

        pmi_errno = PMI_KVS_Commit(kvs_name);
        MPIR_ERR_CHKANDJUMP1(pmi_errno != PMI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**pmi_kvs_commit", "**pmi_kvs_commit %d", pmi_errno);

        pmi_errno = PMI_Barrier();
        MPIR_ERR_CHKANDJUMP1(pmi_errno != PMI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**pmi_barrier", "**pmi_barrier %d", pmi_errno);
    }

fn_exit:
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno;
fn_fail:
    goto fn_exit;
}
#endif


#define MPIR_NODEMAP_PARSE_ERROR() MPIR_ERR_INTERNALANDJUMP(mpi_errno, "parse error")
/* advance _c until we find a non whitespace character */
#define MPIR_NODEMAP_SKIP_SPACE(_c) while (isspace(*(_c))) ++(_c)
/* return true iff _c points to a character valid as an indentifier, i.e., [-_a-zA-Z0-9] */
#define MPIR_NODEMAP_ISIDENT(_c) (isalnum(_c) || (_c) == '-' || (_c) == '_')

/* give an error iff *_c != _e */
#define MPIR_NODEMAP_EXPECT_C(_c, _e) do { if (*(_c) != _e) MPIR_NODEMAP_PARSE_ERROR(); } while (0)
#define MPIR_NODEMAP_EXPECT_AND_SKIP_C(_c, _e) do { MPIR_NODEMAP_EXPECT_C(_c, _e); ++c; } while (0)
/* give an error iff the first |_m| characters of the string _s are equal to _e */
#define MPIR_NODEMAP_EXPECT_S(_s, _e) (MPL_strncmp(_s, _e, strlen(_e)) == 0 && !MPIR_NODEMAP_ISIDENT((_s)[strlen(_e)]))

typedef enum {
    MPIR_NODEMAP_UNKNOWN_MAPPING = -1,
    MPIR_NODEMAP_NULL_MAPPING = 0,
    MPIR_NODEMAP_VECTOR_MAPPING
} MPIR_NODEMAP_mapping_type_t;

#define MPIR_NODEMAP_VECTOR "vector"

typedef struct map_block
{
    int start_id;
    int count;
    int size;
} MPIR_NODEMAP_map_block_t;

#undef FUNCNAME
#define FUNCNAME MPIR_NODEMAP_parse_mapping
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIR_NODEMAP_parse_mapping(char *map_str,
                                             MPIR_NODEMAP_mapping_type_t *type,
                                             MPIR_NODEMAP_map_block_t **map,
                                             int *nblocks)
{
    int mpi_errno = MPI_SUCCESS;
    char *c = map_str, *d;
    int num_blocks = 0;
    int i;
    MPIR_CHKPMEM_DECL(1);

    /* parse string of the form:
       '(' <format> ',' '(' <num> ',' <num> ',' <num> ')' {',' '(' <num> ',' <num> ',' <num> ')'} ')'

       the values of each 3-tuple have the following meaning (X,Y,Z):
         X - node id start value
         Y - number of nodes with size Z
         Z - number of processes assigned to each node
     */

    if (!strlen(map_str)) {
        /* An empty-string indicates an inability to determine or express the
         * process layout on the part of the process manager.  Consider this a
         * non-fatal error case. */
        *type = MPIR_NODEMAP_NULL_MAPPING;
        *map = NULL;
        *nblocks = 0;
        goto fn_exit;
    }

    MPIR_NODEMAP_SKIP_SPACE(c);
    MPIR_NODEMAP_EXPECT_AND_SKIP_C(c, '(');
    MPIR_NODEMAP_SKIP_SPACE(c);

    d = c;
    if (MPIR_NODEMAP_EXPECT_S(d, MPIR_NODEMAP_VECTOR))
        *type = MPIR_NODEMAP_VECTOR_MAPPING;
    else
        MPIR_NODEMAP_PARSE_ERROR();
    c += strlen(MPIR_NODEMAP_VECTOR);
    MPIR_NODEMAP_SKIP_SPACE(c);

    /* first count the number of block descriptors */
    d = c;
    while (*d) {
        if (*d == '(')
            ++num_blocks;
        ++d;
    }

    MPIR_CHKPMEM_MALLOC(*map, MPIR_NODEMAP_map_block_t *, sizeof(MPIR_NODEMAP_map_block_t) * num_blocks, mpi_errno, "map");

    /* parse block descriptors */
    for (i = 0; i < num_blocks; ++i) {
        MPIR_NODEMAP_EXPECT_AND_SKIP_C(c, ',');
        MPIR_NODEMAP_SKIP_SPACE(c);

        MPIR_NODEMAP_EXPECT_AND_SKIP_C(c, '(');
        MPIR_NODEMAP_SKIP_SPACE(c);

        if (!isdigit(*c))
            MPIR_NODEMAP_PARSE_ERROR();
        (*map)[i].start_id = (int)strtol(c, &c, 0);
        MPIR_NODEMAP_SKIP_SPACE(c);

        MPIR_NODEMAP_EXPECT_AND_SKIP_C(c, ',');
        MPIR_NODEMAP_SKIP_SPACE(c);

        if (!isdigit(*c))
            MPIR_NODEMAP_PARSE_ERROR();
        (*map)[i].count = (int)strtol(c, &c, 0);
        MPIR_NODEMAP_SKIP_SPACE(c);

        MPIR_NODEMAP_EXPECT_AND_SKIP_C(c, ',');
        MPIR_NODEMAP_SKIP_SPACE(c);

        if (!isdigit(*c))
            MPIR_NODEMAP_PARSE_ERROR();
        (*map)[i].size = (int)strtol(c, &c, 0);

        MPIR_NODEMAP_EXPECT_AND_SKIP_C(c, ')');
        MPIR_NODEMAP_SKIP_SPACE(c);
    }

    MPIR_NODEMAP_EXPECT_AND_SKIP_C(c, ')');

    *nblocks = num_blocks;
    MPIR_CHKPMEM_COMMIT();
fn_exit:
    return mpi_errno;
fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    MPIR_CHKPMEM_REAP();
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

#undef FUNCNAME
#define FUNCNAME MPIR_NODEMAP_populate_ids_from_mapping
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIR_NODEMAP_populate_ids_from_mapping(char *mapping,
                                                         int sz,
                                                         MPID_Node_id_t *out_nodemap,
                                                         MPID_Node_id_t *out_max_node_id,
                                                         int *did_map)
{
    int mpi_errno = MPI_SUCCESS;
    /* PMI_process_mapping is available */
    MPIR_NODEMAP_mapping_type_t mt = MPIR_NODEMAP_UNKNOWN_MAPPING;
    MPIR_NODEMAP_map_block_t *mb = NULL;
    int nblocks = 0;
    int rank;
    int block, block_node, node_proc;
    int i;
    int found_wrap;
    int local_max_node_id = -1;

    *did_map = 1; /* reset upon failure */

    mpi_errno = MPIR_NODEMAP_parse_mapping(mapping, &mt, &mb, &nblocks);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

    if (MPIR_NODEMAP_NULL_MAPPING == mt) goto fn_fail;
    MPIR_ERR_CHKINTERNAL(mt != MPIR_NODEMAP_VECTOR_MAPPING, mpi_errno, "unsupported mapping type");

    /* allocate nodes to ranks */
    found_wrap = 0;
    for (rank = 0;;) {
        /* FIXME: The patch is hacky because it assumes that seeing a
         * start node ID of 0 means a wrap around.  This is not
         * necessarily true.  A user-defined node list can, in theory,
         * use the node ID 0 without actually creating a wrap around.
         * The reason this patch still works in this case is because
         * Hydra creates a new node list starting from node ID 0 for
         * user-specified nodes during MPI_Comm_spawn{_multiple}.  If
         * a different process manager searches for allocated nodes in
         * the user-specified list, this patch will break. */

        /* If we found that the blocks wrap around, repeat loops
         * should only start at node id 0 */
        for (block = 0; found_wrap && mb[block].start_id; block++);

        for (; block < nblocks; block++) {
            if (mb[block].start_id == 0)
                found_wrap = 1;
            for (block_node = 0; block_node < mb[block].count; block_node++) {
                for (node_proc = 0; node_proc < mb[block].size; node_proc++) {
                    out_nodemap[rank]= mb[block].start_id + block_node;
                    if (++rank == sz)
                        goto break_out;
                }
            }
        }
    }

 break_out:
    /* identify maximum node id */
    for (i = 0; i < sz; i++)
        if (out_nodemap[i] + 1 > local_max_node_id)
            local_max_node_id = out_nodemap[i];

    *out_max_node_id = local_max_node_id;

fn_exit:
    MPL_free(mb);
    return mpi_errno;
fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    *did_map = 0;
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

/* Fills in the node_id info from PMI info.  Adapted from MPIU_Get_local_procs.
   This function is collective over the entire PG because PMI_Barrier is called.

   myrank should be set to -1 if this is not the current process' PG.  This
   is currently not supported due to PMI limitations.

   Fallback Algorithm:

   Each process kvs_puts its hostname and stores the total number of
   processes (g_num_global).  Each process determines maximum node id
   (g_max_node_id) and assigns a node id to each process (g_node_ids[]):

     For each hostname the process seaches the list of unique nodes
     names (node_names[]) for a match.  If a match is found, the node id
     is recorded for that matching process.  Otherwise, the hostname is
     added to the list of node names.
*/
#undef FUNCNAME
#define FUNCNAME MPIR_NODEMAP_build_nodemap
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIR_NODEMAP_build_nodemap(int sz,
                                             int myrank,
                                             MPID_Node_id_t *out_nodemap,
                                             MPID_Node_id_t *out_max_node_id)
{
    static int g_max_node_id = -1;
    int mpi_errno = MPI_SUCCESS;
    int pmi_errno;
    int i, j;
    char *key;
    char *value;
    int key_max_sz;
    int val_max_sz;
    char *kvs_name;
    char **node_names;
    char *node_name_buf;
    int no_local = 0;
    int odd_even_cliques = 0;
    int pmi_version = 1;
    int pmi_subversion = 1;
    MPIR_CHKLMEM_DECL(5);

    /* See if the user wants to override our default values */
    MPL_env2int("PMI_VERSION", &pmi_version);
    MPL_env2int("PMI_SUBVERSION", &pmi_subversion);

    if (sz == 1) {
        out_nodemap[0] = 0;
        *out_max_node_id = 0;
        goto fn_exit;
    }

    /* Used for debugging only.  This disables communication over shared memory */
#ifdef ENABLED_NO_LOCAL
    no_local = 1;
#else
    no_local = MPIR_CVAR_NOLOCAL;
#endif

    /* Used for debugging on a single machine: Odd procs on a node are
       seen as local to each other, and even procs on a node are seen
       as local to each other. */
#ifdef ENABLED_ODD_EVEN_CLIQUES
    odd_even_cliques = 1;
#else
    odd_even_cliques = MPIR_CVAR_ODD_EVEN_CLIQUES;
#endif

    if (no_local) {
        /* just assign 0 to n-1 as node ids and bail */
        for (i = 0; i < sz; ++i) {
            out_nodemap[i] = ++g_max_node_id;
        }
        *out_max_node_id = g_max_node_id;
        goto fn_exit;
    }

#ifdef USE_PMI2_API
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

        mpi_errno = PMI2_Info_GetJobAttr("PMI_process_mapping", process_mapping, sizeof(process_mapping), &found);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        MPIR_ERR_CHKINTERNAL(!found, mpi_errno, "PMI_process_mapping attribute not found");
        /* this code currently assumes pg is comm_world */
        mpi_errno = MPIR_NODEMAP_populate_ids_from_mapping(process_mapping, sz, out_nodemap, out_max_node_id, &did_map);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        MPIR_ERR_CHKINTERNAL(!did_map, mpi_errno, "unable to populate node ids from PMI_process_mapping");
    }
#else /* USE_PMI2_API */
    if (myrank == -1) {
        /* fixme this routine can't handle the dynamic process case at this
           time.  this will require more support from the process manager. */
        MPIR_Assert(0);
    }

    /* Allocate space for pmi key and value */
    pmi_errno = PMI_KVS_Get_key_length_max(&key_max_sz);
    MPIR_ERR_CHKANDJUMP1(pmi_errno, mpi_errno, MPI_ERR_OTHER, "**fail", "**fail %d", pmi_errno);
    MPIR_CHKLMEM_MALLOC(key, char *, key_max_sz, mpi_errno, "key");

    pmi_errno = PMI_KVS_Get_value_length_max(&val_max_sz);
    MPIR_ERR_CHKANDJUMP1(pmi_errno, mpi_errno, MPI_ERR_OTHER, "**fail", "**fail %d", pmi_errno);
    MPIR_CHKLMEM_MALLOC(value, char *, val_max_sz, mpi_errno, "value");

    MPIR_CHKLMEM_MALLOC(kvs_name, char *, 256, mpi_errno, "kvs_name");
    pmi_errno = PMI_KVS_Get_my_name(kvs_name, 256);
    MPIR_ERR_CHKANDJUMP1(pmi_errno, mpi_errno, MPI_ERR_OTHER, "**fail", "**fail %d", pmi_errno);
    /* See if process manager supports PMI_process_mapping keyval */

    if (pmi_version == 1 && pmi_subversion == 1) {
        pmi_errno = PMI_KVS_Get(kvs_name, "PMI_process_mapping", value, val_max_sz);
        if (pmi_errno == 0) {
            int did_map = 0;
            /* this code currently assumes pg is comm_world */
            mpi_errno = MPIR_NODEMAP_populate_ids_from_mapping(value, sz, out_nodemap, out_max_node_id, &did_map);
            g_max_node_id = *out_max_node_id;
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            if (did_map) {
                goto odd_even_cliques;
            }
            /* else fall through to O(N^2) PMI_KVS_Gets version */
        }
    }

    /* fallback algorithm */
    mpi_errno = MPIR_NODEMAP_publish_node_id(sz, myrank);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

    /* Allocate temporary structures.  These would need to be persistent if
       we somehow were able to support dynamic processes via this method. */
    MPIR_CHKLMEM_MALLOC(node_names, char **, sz * sizeof(char*), mpi_errno, "node_names");
    MPIR_CHKLMEM_MALLOC(node_name_buf, char *, sz * key_max_sz * sizeof(char), mpi_errno, "node_name_buf");

    /* Gather hostnames */
    for (i = 0; i < sz; ++i)
    {
        node_names[i] = &node_name_buf[i * key_max_sz];
        node_names[i][0] = '\0';
    }

    g_max_node_id = -1; /* defensive */

    for (i = 0; i < sz; ++i)
    {
        MPIR_Assert(g_max_node_id < sz);
        if (i == myrank)
        {
            /* This is us, no need to perform a get */
            int ret;
            char *hostname = (char*) MPL_malloc(sizeof(char) * MAX_HOSTNAME_LEN);
            ret = gethostname(hostname, MAX_HOSTNAME_LEN);
            MPIR_ERR_CHKANDJUMP2(ret == -1, mpi_errno, MPI_ERR_OTHER, "**sock_gethost", "**sock_gethost %s %d", MPIR_Strerror(errno), errno);
            hostname[MAX_HOSTNAME_LEN-1] = '\0';
            MPL_snprintf(node_names[g_max_node_id+1], key_max_sz, "%s", hostname);
            MPL_free(hostname);
        }
        else
        {
            memset(key, 0, key_max_sz);
            MPL_snprintf(key, key_max_sz, "hostname[%d]", i);

            pmi_errno = PMI_KVS_Get(kvs_name, key, node_names[g_max_node_id+1], key_max_sz);
            MPIR_ERR_CHKANDJUMP1(pmi_errno != PMI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**pmi_kvs_get", "**pmi_kvs_get %d", pmi_errno);
        }

        /* Find the node_id for this process, or create a new one */
        /* FIXME:need a better algorithm -- this one does O(N^2) strncmp()s! */
        /* The right fix is to get all this information from the process
           manager, rather than bother with this hostname hack at all. */
        for (j = 0; j < g_max_node_id + 1; ++j)
            if (!MPL_strncmp(node_names[j], node_names[g_max_node_id+1], key_max_sz))
                break;
        if (j == g_max_node_id + 1)
            ++g_max_node_id;
        else
            node_names[g_max_node_id+1][0] = '\0';
        out_nodemap[i] = j;
    }

odd_even_cliques:
    if (odd_even_cliques)
    {
        /* Create new processes for all odd numbered processes. This
           may leave nodes ids with no processes assigned to them, but
           I think this is OK */
        for (i = 0; i < sz; ++i)
            if (i & 0x1)
                out_nodemap[i] += g_max_node_id + 1;
        g_max_node_id = g_max_node_id * 2 + 1;
    }
    *out_max_node_id = g_max_node_id;
#endif

fn_exit:
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#endif /* ifndef BUILD_NODEMAP_H_INCLUDED */
