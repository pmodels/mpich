/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef BUILD_NODEMAP_H_INCLUDED
#define BUILD_NODEMAP_H_INCLUDED

#include "mpl.h"

/* this function is not used in pmix */
static inline int MPIR_NODEMAP_publish_node_id(int sz, int myrank)
{
    int mpi_errno = MPI_SUCCESS;
    int ret;
    char *key;
    int key_max_sz;
    char hostname[MAX_HOSTNAME_LEN];
    char strerrbuf[MPIR_STRERROR_BUF_SIZE] ATTRIBUTE((unused));
    MPIR_CHKLMEM_DECL(2);

    /* set hostname */

    ret = gethostname(hostname, MAX_HOSTNAME_LEN);
    MPIR_ERR_CHKANDJUMP2(ret == -1, mpi_errno, MPI_ERR_OTHER, "**sock_gethost",
                         "**sock_gethost %s %d",
                         MPIR_Strerror(errno, strerrbuf, MPIR_STRERROR_BUF_SIZE), errno);
    hostname[MAX_HOSTNAME_LEN - 1] = '\0';

    /* Allocate space for pmi key */
    key_max_sz = MPIR_pmi_max_key_size();
    MPIR_CHKLMEM_MALLOC(key, char *, key_max_sz, mpi_errno, "key", MPL_MEM_ADDRESS);

    /* Put my hostname id */
    if (sz > 1) {
        memset(key, 0, key_max_sz);
        MPL_snprintf(key, key_max_sz, "hostname[%d]", myrank);

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

static inline int MPIR_NODEMAP_build_nodemap_fallback(int sz, int myrank, int *out_nodemap)
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

    mpi_errno = MPIR_NODEMAP_publish_node_id(sz, myrank);
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
            MPL_snprintf(node_names[max_node_id + 1], key_max_sz, "%s", hostname);
            MPL_free(hostname);
        } else {
            memset(key, 0, key_max_sz);
            MPL_snprintf(key, key_max_sz, "hostname[%d]", i);

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

#endif /* BUILD_NODEMAP_H_INCLUDED */
