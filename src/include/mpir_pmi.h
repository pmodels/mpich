/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPIR_PMI_H_INCLUDED
#define MPIR_PMI_H_INCLUDED

#include "mpichconf.h"

#ifdef ENABLE_PMI1
#if defined(USE_PMI1_SLURM)
#include <slurm/pmi.h>
#else
#include <pmi.h>
#endif
#endif

#ifdef ENABLE_PMI2
#if defined(USE_PMI2_SLURM)
#include <slurm/pmi2.h>
#else
#include <pmi2.h>
#endif
#endif

#ifdef ENABLE_PMIX
#include <pmix.h>
#endif

/* Domain options for init-time collectives */
typedef enum {
    MPIR_PMI_DOMAIN_ALL = 0,
    MPIR_PMI_DOMAIN_LOCAL = 1,
    MPIR_PMI_DOMAIN_NODE_ROOTS = 2
} MPIR_PMI_DOMAIN;

/* key/val pair struct to abstract PMI key/val pair */
typedef struct MPIR_PMI_KEYVAL {
    const char *key;
    char *val;
} MPIR_PMI_KEYVAL_t;

/* PMI init / finalize */
int MPIR_pmi_init(void);
void MPIR_pmi_finalize(void);
void MPIR_pmi_abort(int exit_code, const char *error_msg);
int MPIR_pmi_set_threaded(int is_threaded);

/* PMI getters for private fields */
int MPIR_pmi_max_key_size(void);
int MPIR_pmi_max_val_size(void);
const char *MPIR_pmi_job_id(void);
const char *MPIR_pmi_hostname(void);
char *MPIR_pmi_get_jobattr(const char *key);    /* key must use "PMI_" prefix */

/* PMI wrapper utilities */

/* * barrier or kvs fence. */
int MPIR_pmi_barrier(void);
int MPIR_pmi_barrier_only(void);
/* * barrier over local set. More efficient for PMIx. Same as MPIR_pmi_barrier for PMI1/2. */
int MPIR_pmi_barrier_local(void);
/* * put, to global domain */
int MPIR_pmi_kvs_put(const char *key, const char *val);
/* * get. src in [0..size-1] or -1 for anysrc. val_size <= MPIR_pmi_max_val_size(). */
int MPIR_pmi_kvs_get(int src, const char *key, char *val, int val_size);
/* get from parent process */
int MPIR_pmi_kvs_parent_get(const char *key, char *val, int val_size);

/* * bcast from rank 0 to ALL or NODE_ROOTS processes. Both are collective over ALL */
int MPIR_pmi_bcast(void *buf, int size, MPIR_PMI_DOMAIN domain);

/* * allgather over either ALL or NODE_ROOTS processes. Both are collective over ALL.
 *   recvsize <= MPIR_pmi_max_val_size().
 *   recvbuf of size either "size x recvsize" or "num_nodes x recvsize".
 */
int MPIR_pmi_allgather(const void *sendbuf, int sendsize, void *recvbuf, int recvsize,
                       MPIR_PMI_DOMAIN domain);

/* * allgather utilizing shared memory. shm_buf is assumed to be a shared-memory.
 *   all processes in ALL or NODE_ROOTS will first publish using put. Then
 *   all processes will cooperatively gather using get.
 */
int MPIR_pmi_allgather_shm(const void *sendbuf, int sendsize, void *shm_buf, int recvsize,
                           MPIR_PMI_DOMAIN domain);

/* * bcast_local: all processes will participate.
 *   Each local leader bcast to each local proc (within a node).
 */
int MPIR_pmi_bcast_local(char *val, int val_size);

/* name service functions */
int MPIR_pmi_publish(const char name[], const char port[]);
int MPIR_pmi_lookup(const char name[], char port[], int portlen);
int MPIR_pmi_unpublish(const char name[]);

/* Other misc functions */
int MPIR_pmi_get_universe_size(int *universe_size);

struct MPIR_Info;               /* forward declare (mpir_info.h) */
int MPIR_pmi_spawn_multiple(int count, char *commands[], char **argvs[],
                            const int maxprocs[], struct MPIR_Info *info_ptrs[],
                            int num_preput_keyval, struct MPIR_PMI_KEYVAL *preput_keyvals,
                            int *pmi_errcodes);
int MPIR_pmi_has_local_cliques(void);
int MPIR_pmi_build_nodemap(int *nodemap, int sz);
int MPIR_pmi_build_nodemap_fallback(int sz, int myrank, int *out_nodemap);

#ifdef HAVE_HWLOC
/* A fallback for PMIx_Load_topology. */
typedef struct MPIR_pmi_topology {
    const char *source;
    void *topology;  /* assume hwloc_topology_t is a pointer */
} MPIR_pmi_topology_t;

int MPIR_pmi_load_hwloc_topology(MPIR_pmi_topology_t *topo);
#endif

#endif /* MPIR_PMI_H_INCLUDED */
