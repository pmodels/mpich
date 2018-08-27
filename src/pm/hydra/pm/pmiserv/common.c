/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2010 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "common.h"
#include "topo.h"

void HYD_pmcd_init_header(struct HYD_pmcd_hdr *hdr)
{
    hdr->cmd = INVALID_CMD;
    hdr->buflen = -1;
    hdr->pid = -1;
    hdr->pmi_version = -1;
    hdr->pgid = -1;
    hdr->proxy_id = -1;
    hdr->rank = -1;
    hdr->signum = -1;
}

HYD_status HYD_pmcd_pmi_parse_pmi_cmd(char *obuf, int pmi_version, char **pmi_cmd, char *args[])
{
    char *str1 = NULL, *cmd, *buf;
    const char *delim;
    int i;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    /* Make a copy of the original buffer */
    buf = MPL_strdup(obuf);
    HYDU_ERR_CHKANDJUMP(status, NULL == buf, HYD_INTERNAL_ERROR, "%s", "");
    if (buf[strlen(obuf) - 1] == '\n')
        buf[strlen(obuf) - 1] = '\0';

    if (pmi_version == 1) {
        if (!strncmp(buf, "cmd=", strlen("cmd=")))
            delim = " ";
        else
            delim = "\n";
    } else {    /* PMI-v2 */
        delim = ";";
    }

    cmd = strtok(buf, delim);
    for (i = 0;; i++) {
        args[i] = strtok(NULL, delim);
        if (args[i] == NULL)
            break;
        args[i] = MPL_strdup(args[i]);
    }

    /* Search for the PMI command in our table */
    status = HYDU_strsplit(cmd, &str1, pmi_cmd, '=');
    HYDU_ERR_POP(status, "string split returned error\n");
    HYDU_ERR_CHKANDJUMP(status, NULL == *pmi_cmd, HYD_INTERNAL_ERROR, "%s", "");

  fn_exit:
    MPL_free(buf);
    if (str1)
        MPL_free(str1);
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYD_pmcd_pmi_args_to_tokens(char *args[], struct HYD_pmcd_token **tokens, int *count)
{
    int i, j;
    char *arg;
    HYD_status status = HYD_SUCCESS;

    for (i = 0; args[i]; i++);
    *count = i;
    HYDU_MALLOC_OR_JUMP(*tokens, struct HYD_pmcd_token *, *count * sizeof(struct HYD_pmcd_token),
                        status);

    for (i = 0; args[i]; i++) {
        arg = MPL_strdup(args[i]);
        HYDU_ERR_CHKANDJUMP(status, NULL == arg, HYD_INTERNAL_ERROR, "strdup failed\n");
        (*tokens)[i].key = arg;
        for (j = 0; arg[j] && arg[j] != '='; j++);
        if (!arg[j]) {
            (*tokens)[i].val = NULL;
        } else {
            arg[j] = 0;
            (*tokens)[i].val = &arg[++j];
        }
    }

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

void HYD_pmcd_pmi_free_tokens(struct HYD_pmcd_token *tokens, int token_count)
{
    int i;

    for (i = 0; i < token_count; i++)
        MPL_free(tokens[i].key);
    MPL_free(tokens);
}

char *HYD_pmcd_pmi_find_token_keyval(struct HYD_pmcd_token *tokens, int count, const char *key)
{
    int i;

    for (i = 0; i < count; i++) {
        if (!strcmp(tokens[i].key, key))
            return tokens[i].val;
    }

    return NULL;
}

HYD_status HYD_pmcd_pmi_allocate_kvs(struct HYD_pmcd_pmi_kvs ** kvs, int pgid)
{
    HYD_status status = HYD_SUCCESS;
    char hostname[MAX_HOSTNAME_LEN];
    unsigned int seed;
    MPL_time_t tv;
    double secs;
    int rnd;

    HYDU_FUNC_ENTER();

    if (gethostname(hostname, MAX_HOSTNAME_LEN) < 0)
        HYDU_ERR_SETANDJUMP(status, HYD_SOCK_ERROR, "unable to get local hostname\n");

    MPL_wtime(&tv);
    MPL_wtime_todouble(&tv, &secs);
    seed = (unsigned int) (secs * 1e6);
    srand(seed);
    rnd = rand();

    HYDU_MALLOC_OR_JUMP(*kvs, struct HYD_pmcd_pmi_kvs *, sizeof(struct HYD_pmcd_pmi_kvs), status);
    MPL_snprintf((*kvs)->kvsname, PMI_MAXKVSLEN, "kvs_%d_%d_%d_%s", (int) getpid(), pgid, rnd,
                 hostname);
    (*kvs)->key_pair = NULL;
    (*kvs)->tail = NULL;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

void HYD_pmcd_free_pmi_kvs_list(struct HYD_pmcd_pmi_kvs *kvs_list)
{
    struct HYD_pmcd_pmi_kvs_pair *key_pair, *tmp;

    HYDU_FUNC_ENTER();

    key_pair = kvs_list->key_pair;
    while (key_pair) {
        tmp = key_pair->next;
        MPL_free(key_pair);
        key_pair = tmp;
    }
    MPL_free(kvs_list);

    HYDU_FUNC_EXIT();
}

HYD_status HYD_pmcd_pmi_add_kvs(const char *key, char *val, struct HYD_pmcd_pmi_kvs *kvs, int *ret)
{
    struct HYD_pmcd_pmi_kvs_pair *key_pair;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDU_MALLOC_OR_JUMP(key_pair, struct HYD_pmcd_pmi_kvs_pair *,
                        sizeof(struct HYD_pmcd_pmi_kvs_pair), status);
    MPL_snprintf(key_pair->key, PMI_MAXKEYLEN, "%s", key);
    MPL_snprintf(key_pair->val, PMI_MAXVALLEN, "%s", val);
    key_pair->next = NULL;

    *ret = 0;

    if (kvs->key_pair == NULL) {
        kvs->key_pair = key_pair;
        kvs->tail = key_pair;
    } else {
#ifdef PMI_KEY_CHECK
        struct HYD_pmcd_pmi_kvs_pair *run, *last;

        for (run = kvs->key_pair; run; run = run->next) {
            if (!strcmp(run->key, key_pair->key)) {
                /* duplicate key found */
                *ret = -1;
                goto fn_fail;
            }
            last = run;
        }
        /* Add key_pair to end of list. */
        last->next = key_pair;
#else
        kvs->tail->next = key_pair;
        kvs->tail = key_pair;
#endif
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    MPL_free(key_pair);
    goto fn_exit;
}
