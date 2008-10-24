/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef PMCU_PMI_H_INCLUDED
#define PMCU_PMI_H_INCLUDED

#include "hydra.h"
#include "hydra_mem.h"

#define MAXKEYLEN    64         /* max length of key in keyval space */
#define MAXVALLEN   256         /* max length of value in keyval space */
#define MAXNAMELEN  256         /* max length of various names */
#define MAXKVSNAME  MAXNAMELEN  /* max length of a kvsname */

#define HYD_PMCU_NUM_STR 100

typedef struct HYD_PMCU_pmi_kvs_pair {
    char   key[MAXKEYLEN];
    char   val[MAXVALLEN];
    struct HYD_PMCU_pmi_kvs_pair * next;
} HYD_PMCU_pmi_kvs_pair_t;

typedef struct HYD_PMCU_pmi_kvs {
    char                      kvs_name[MAXNAMELEN]; /* Name of this kvs */
    HYD_PMCU_pmi_kvs_pair_t * key_pair;
} HYD_PMCU_pmi_kvs_t;

typedef struct HYD_PMCU_pmi_pg HYD_PMCU_pmi_pg_t;
typedef struct HYD_PMCU_pmi_process HYD_PMCU_pmi_process_t;

struct HYD_PMCU_pmi_process {
    /* This is a bad design if we need to tie in an FD to a PMI
     * process. This essentially kills any chance of PMI server
     * masquerading. However, PMI v1 requires this state to be
     * maintained. */
    int                            fd;
    struct HYD_PMCU_pmi_pg       * pg;
    struct HYD_PMCU_pmi_process  * next;
};

struct HYD_PMCU_pmi_pg {
    int                      id;

    int                      num_procs; /* Number of processes in the group */
    int                      barrier_count;

    struct HYD_PMCU_pmi_process * process;
    HYD_PMCU_pmi_kvs_t          * kvs;

    struct HYD_PMCU_pmi_pg * next;
};

HYD_Status HYD_PMCU_Create_pg(void);
HYD_Status HYD_PMCU_pmi_initack(int fd, char * args[]);
HYD_Status HYD_PMCU_pmi_init(int fd, char * args[]);
HYD_Status HYD_PMCU_pmi_get_maxes(int fd, char * args[]);
HYD_Status HYD_PMCU_pmi_get_my_kvsname(int fd, char * args[]);
HYD_Status HYD_PMCU_pmi_barrier_in(int fd, char * args[]);
HYD_Status HYD_PMCU_pmi_put(int fd, char * args[]);
HYD_Status HYD_PMCU_pmi_get(int fd, char * args[]);
HYD_Status HYD_PMCU_pmi_finalize(int fd, char * args[]);

#endif /* PMCU_PMI_H_INCLUDED */
