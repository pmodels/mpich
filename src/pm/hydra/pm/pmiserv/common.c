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
    buf = HYDU_strdup(obuf);
    if (buf[strlen(obuf) - 1] == '\n')
        buf[strlen(obuf) - 1] = '\0';

    if (pmi_version == 1) {
        if (!strncmp(buf, "cmd=", strlen("cmd=")))
            delim = " ";
        else
            delim = "\n";
    }
    else {      /* PMI-v2 */
        delim = ";";
    }

    cmd = strtok(buf, delim);
    for (i = 0;; i++) {
        args[i] = strtok(NULL, delim);
        if (args[i] == NULL)
            break;
        args[i] = HYDU_strdup(args[i]);
    }

    /* Search for the PMI command in our table */
    status = HYDU_strsplit(cmd, &str1, pmi_cmd, '=');
    HYDU_ERR_POP(status, "string split returned error\n");

  fn_exit:
    HYDU_FREE(buf);
    if (str1)
        HYDU_FREE(str1);
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
    HYDU_MALLOC(*tokens, struct HYD_pmcd_token *, *count * sizeof(struct HYD_pmcd_token), status);

    for (i = 0; args[i]; i++) {
        arg = HYDU_strdup(args[i]);
        (*tokens)[i].key = arg;
        for (j = 0; arg[j] && arg[j] != '='; j++);
        if (!arg[j]) {
            (*tokens)[i].val = NULL;
        }
        else {
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
        HYDU_FREE(tokens[i].key);
    HYDU_FREE(tokens);
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

    HYDU_FUNC_ENTER();

    HYDU_MALLOC(*kvs, struct HYD_pmcd_pmi_kvs *, sizeof(struct HYD_pmcd_pmi_kvs), status);
    HYDU_snprintf((*kvs)->kvsname, PMI_MAXKVSLEN, "kvs_%d_%d", (int) getpid(), pgid);
    (*kvs)->key_pair = NULL;

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
        HYDU_FREE(key_pair);
        key_pair = tmp;
    }
    HYDU_FREE(kvs_list);

    HYDU_FUNC_EXIT();
}

HYD_status HYD_pmcd_pmi_add_kvs(const char *key, char *val, struct HYD_pmcd_pmi_kvs *kvs, int *ret)
{
    struct HYD_pmcd_pmi_kvs_pair *key_pair, *run, *last;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDU_MALLOC(key_pair, struct HYD_pmcd_pmi_kvs_pair *, sizeof(struct HYD_pmcd_pmi_kvs_pair),
                status);
    HYDU_snprintf(key_pair->key, PMI_MAXKEYLEN, "%s", key);
    HYDU_snprintf(key_pair->val, PMI_MAXVALLEN, "%s", val);
    key_pair->next = NULL;

    *ret = 0;

    if (kvs->key_pair == NULL) {
        kvs->key_pair = key_pair;
    }
    else {
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
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    HYDU_FREE(key_pair);
    goto fn_exit;
}
