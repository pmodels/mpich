/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef PMI_COMMON_H_INCLUDED
#define PMI_COMMON_H_INCLUDED

#include "hydra_base.h"
#include "hydra_utils.h"

/* Generic definitions */
#define MAXKEYLEN    64 /* max length of key in keyval space */
/* FIXME: PMI-1 uses 256, PMI-2 uses 1024; we use the MAX */
#define MAXVALLEN  1024 /* max length of value in keyval space */
#define MAXNAMELEN  256 /* max length of various names */
#define MAXKVSNAME  MAXNAMELEN  /* max length of a kvsname */

struct HYD_pmcd_pmi_kvs_pair {
    char key[MAXKEYLEN];
    char val[MAXVALLEN];
    struct HYD_pmcd_pmi_kvs_pair *next;
};

struct HYD_pmcd_pmi_kvs {
    char kvs_name[MAXNAMELEN];  /* Name of this kvs */
    struct HYD_pmcd_pmi_kvs_pair *key_pair;
};

/* The set of commands supported */
enum HYD_pmcd_pmi_cmd {
    INVALID_CMD = 0,        /* for sanity testing */

    /* UI to proxy commands */
    PROC_INFO,
    CKPOINT,
    PMI_RESPONSE,

    /* Proxy to UI commands */
    PID_LIST,
    EXIT_STATUS,
    PMI_CMD
};

struct HYD_pmcd_pmi_hdr {
    int pid;                    /* ID of the requesting process */
    int pmi_version;            /* PMI version */
    int buflen;
};

struct HYD_pmcd_token {
    char *key;
    char *val;
};

struct HYD_pmcd_stdio_hdr {
    int rank;
    int buflen;
};

HYD_status HYD_pmcd_pmi_parse_pmi_cmd(char *buf, int pmi_version, char **pmi_cmd,
                                      char *args[]);
HYD_status HYD_pmcd_pmi_args_to_tokens(char *args[], struct HYD_pmcd_token **tokens,
                                       int *count);
void HYD_pmcd_pmi_free_tokens(struct HYD_pmcd_token *tokens, int token_count);
char *HYD_pmcd_pmi_find_token_keyval(struct HYD_pmcd_token *tokens, int count,
                                     const char *key);
HYD_status HYD_pmcd_pmi_allocate_kvs(struct HYD_pmcd_pmi_kvs **kvs, int pgid);
void HYD_pmcd_free_pmi_kvs_list(struct HYD_pmcd_pmi_kvs *kvs_list);
HYD_status HYD_pmcd_pmi_add_kvs(const char *key, char *val, struct HYD_pmcd_pmi_kvs *kvs,
                                int *ret);

#endif /* PMI_COMMON_H_INCLUDED */
