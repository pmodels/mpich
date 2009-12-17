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

struct HYD_pmcd_token {
    char *key;
    char *val;
};

struct HYD_pmcd_pmi_kvs_pair {
    char key[MAXKEYLEN];
    char val[MAXVALLEN];
    struct HYD_pmcd_pmi_kvs_pair *next;
};

struct HYD_pmcd_pmi_kvs {
    char kvs_name[MAXNAMELEN];  /* Name of this kvs */
    struct HYD_pmcd_pmi_kvs_pair *key_pair;
};

struct HYD_pmcd_pmi_process {
    /* This is a bad design if we need to tie in an FD to a PMI
     * process. This essentially kills any chance of PMI server
     * masquerading. */
    int fd;
    int rank;                   /* COMM_WORLD rank of this process */
    int epoch;                  /* Epoch this process has reached */
    struct HYD_proxy *proxy;     /* Back pointer to the proxy */
    struct HYD_pmcd_pmi_process *next;
};

struct HYD_pmcd_pmi_proxy_scratch {
    struct HYD_pmcd_pmi_process *process_list;
    struct HYD_pmcd_pmi_kvs *kvs;    /* Node-level KVS space for node attributes */
};

struct HYD_pmcd_pmi_pg_scratch {
    int num_subgroups;          /* Number of subgroups */
    int *conn_procs;            /* Number of connected procs in each subgroup */

    int barrier_count;

    struct HYD_pmcd_pmi_kvs *kvs;
};

HYD_status HYD_pmcd_args_to_tokens(char *args[], struct HYD_pmcd_token **tokens, int *count);
char *HYD_pmcd_find_token_keyval(struct HYD_pmcd_token *tokens, int count, const char *key);
HYD_status HYD_pmcd_pmi_add_process_to_pg(struct HYD_pg * pg, int fd, int rank);
HYD_status HYD_pmcd_pmi_id_to_rank(int id, int pgid, int *rank);
struct HYD_pmcd_pmi_process *HYD_pmcd_pmi_find_process(int fd);
HYD_status HYD_pmcd_pmi_add_kvs(const char *key, char *val, struct HYD_pmcd_pmi_kvs * kvs,
                                int *ret);
HYD_status HYD_pmcd_pmi_process_mapping(struct HYD_pmcd_pmi_process * process,
                                        char **process_mapping);
HYD_status HYD_pmcd_pmi_init(void);
HYD_status HYD_pmcd_pmi_finalize(void);

struct HYD_pmcd_pmi_handle_fns {
    const char *cmd;
    HYD_status(*handler) (int fd, int pgid, char *args[]);
};

struct HYD_pmcd_pmi_handle {
    const char *delim;
    struct HYD_pmcd_pmi_handle_fns *handle_fns;
};

extern struct HYD_pmcd_pmi_handle *HYD_pmcd_pmi_handle;

#endif /* PMI_HANDLE_H_INCLUDED */
