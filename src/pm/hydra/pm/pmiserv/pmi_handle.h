/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef PMI_HANDLE_H_INCLUDED
#define PMI_HANDLE_H_INCLUDED

#include "hydra_base.h"

/* PMI-1 specific definitions */
#define PMI_V1_DELIM " "
extern struct HYD_pmcd_pmi_handle HYD_pmcd_pmi_v1;

/* PMI-2 specific definitions */
#define PMI_V2_DELIM ";"
extern struct HYD_pmcd_pmi_handle HYD_pmcd_pmi_v2;

/* Generic definitions */
#define MAXKEYLEN    64 /* max length of key in keyval space */
/* FIXME: PMI-1 uses 256, PMI-2 uses 1024; we use the MAX */
#define MAXVALLEN  1024 /* max length of value in keyval space */
#define MAXNAMELEN  256 /* max length of various names */
#define MAXKVSNAME  MAXNAMELEN  /* max length of a kvsname */

typedef struct HYD_pmcd_pmi_kvs_pair {
    char key[MAXKEYLEN];
    char val[MAXVALLEN];
    struct HYD_pmcd_pmi_kvs_pair *next;
} HYD_pmcd_pmi_kvs_pair_t;

typedef struct HYD_pmcd_pmi_kvs {
    char kvs_name[MAXNAMELEN];  /* Name of this kvs */
    HYD_pmcd_pmi_kvs_pair_t *key_pair;
} HYD_pmcd_pmi_kvs_t;

typedef struct HYD_pmcd_pmi_pg HYD_pmcd_pmi_pg_t;
typedef struct HYD_pmcd_pmi_node HYD_pmcd_pmi_node_t;
typedef struct HYD_pmcd_pmi_process HYD_pmcd_pmi_process_t;

struct HYD_pmcd_pmi_process {
    /* This is a bad design if we need to tie in an FD to a PMI
     * process. This essentially kills any chance of PMI server
     * masquerading. */
    int fd;
    int rank;                   /* COMM_WORLD rank of this process */
    int epoch;                  /* Epoch this process has reached */
    struct HYD_pmcd_pmi_node *node;     /* Back pointer to the PMI node */
    struct HYD_pmcd_pmi_process *next;
};

struct HYD_pmcd_pmi_node {
    int node_id;                /* This corresponds to the proxy ID of the
                                 * launched processes */
    struct HYD_pmcd_pmi_pg *pg; /* Back pointer to the group */
    struct HYD_pmcd_pmi_process *process_list;

    HYD_pmcd_pmi_kvs_t *kvs;    /* Node-level KVS space for node attributes */

    struct HYD_pmcd_pmi_node *next;
};

struct HYD_pmcd_pmi_pg {
    int id;

    int num_procs;              /* Number of processes in the group */
    int num_subgroups;          /* Number of subgroups */
    int *conn_procs;            /* Number of connected procs in each subgroup */

    int barrier_count;

    struct HYD_pmcd_pmi_node *node_list;
    HYD_pmcd_pmi_kvs_t *kvs;

    struct HYD_pmcd_pmi_pg *next;
};

enum HYD_pmcd_pmi_process_mapping_type {
    HYD_pmcd_pmi_vector
};

HYD_status HYD_pmcd_pmi_add_process_to_pg(HYD_pmcd_pmi_pg_t * pg, int fd, int rank);
HYD_status HYD_pmcd_pmi_id_to_rank(int id, int *rank);
HYD_pmcd_pmi_process_t *HYD_pmcd_pmi_find_process(int fd);
HYD_status HYD_pmcd_pmi_add_kvs(const char *key, char *val, HYD_pmcd_pmi_kvs_t * kvs,
                                int *ret);
HYD_status HYD_pmcd_pmi_process_mapping(HYD_pmcd_pmi_process_t * process,
                                        enum HYD_pmcd_pmi_process_mapping_type type,
                                        char **process_mapping);
HYD_status HYD_pmcd_pmi_init(void);
HYD_status HYD_pmcd_pmi_finalize(void);

extern HYD_pmcd_pmi_pg_t *HYD_pg_list;

struct HYD_pmcd_pmi_handle_fns {
    const char *cmd;
     HYD_status(*handler) (int fd, char *args[]);
};

struct HYD_pmcd_pmi_handle {
    const char *delim;
    struct HYD_pmcd_pmi_handle_fns *handle_fns;
};

extern struct HYD_pmcd_pmi_handle *HYD_pmcd_pmi_handle;

#endif /* PMI_HANDLE_H_INCLUDED */
