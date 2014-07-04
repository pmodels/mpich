/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2009 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef COMMON_H_INCLUDED
#define COMMON_H_INCLUDED

#include "hydra.h"

/* Generic definitions */
#define PMI_MAXKEYLEN    (64)   /* max length of key in keyval space */
#define PMI_MAXVALLEN    (1024) /* max length of value in keyval space */
#define PMI_MAXKVSLEN    (256)  /* max length of various names */

#define MAX_PMI_ARGS  (65536)    /* number of arguments in a PMI command */
#define MAX_PMI_INTERNAL_ARGS   (65536) /* number of arguments in internal communication */

struct HYD_pmcd_pmi_kvs_pair {
    char key[PMI_MAXKEYLEN];
    char val[PMI_MAXVALLEN];
    struct HYD_pmcd_pmi_kvs_pair *next;
};

struct HYD_pmcd_pmi_kvs {
    char kvsname[PMI_MAXKVSLEN];        /* Name of this kvs */
    struct HYD_pmcd_pmi_kvs_pair *key_pair;
};

struct HYD_pmcd_hdr {
    /* The set of commands supported */
    enum HYD_pmcd_cmd {
        INVALID_CMD = 0,        /* for sanity testing */

        /* UI to proxy commands */
        PROC_INFO,
        CKPOINT,
        PMI_RESPONSE,
        SIGNAL,
        STDIN,

        /* Proxy to UI commands */
        PID_LIST,
        EXIT_STATUS,
        PMI_CMD,
        STDOUT,
        STDERR,
        PROCESS_TERMINATED
    } cmd;

    /* Generic */
    int buflen;

    /* PMI_CMD */
    int pid;                    /* ID of the requesting process */
    int pmi_version;            /* PMI version */

    /* STDOUT/STDERR */
    int pgid;
    int proxy_id;
    int rank;

    /* SIGNAL */
    int signum;
};

struct HYD_pmcd_token {
    char *key;
    char *val;
};

void HYD_pmcd_init_header(struct HYD_pmcd_hdr *hdr);
HYD_status HYD_pmcd_pmi_parse_pmi_cmd(char *buf, int pmi_version, char **pmi_cmd, char *args[]);
HYD_status HYD_pmcd_pmi_args_to_tokens(char *args[], struct HYD_pmcd_token **tokens, int *count);
void HYD_pmcd_pmi_free_tokens(struct HYD_pmcd_token *tokens, int token_count);
char *HYD_pmcd_pmi_find_token_keyval(struct HYD_pmcd_token *tokens, int count, const char *key);
HYD_status HYD_pmcd_pmi_allocate_kvs(struct HYD_pmcd_pmi_kvs **kvs, int pgid);
void HYD_pmcd_free_pmi_kvs_list(struct HYD_pmcd_pmi_kvs *kvs_list);
HYD_status HYD_pmcd_pmi_add_kvs(const char *key, char *val, struct HYD_pmcd_pmi_kvs *kvs, int *ret);

#endif /* COMMON_H_INCLUDED */
