/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
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

HYD_status HYD_pmcd_pmi_parse_pmi_cmd(char *obuf, int pmi_version, char **pmi_cmd,
                                      char *args[])
{
    char *tbuf = NULL, *seg, *str1 = NULL, *cmd;
    char *buf;
    char *tmp[HYD_NUM_TMP_STRINGS], *targs[HYD_NUM_TMP_STRINGS];
    const char *delim;
    int i, j, k;
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

        /* Here we only get PMI-1 commands or backward compatible
         * PMI-2 commands, so we always explicitly use the PMI-1
         * delimiter. This allows us to get backward-compatible PMI-2
         * commands interleaved with regular PMI-2 commands. */
        tbuf = HYDU_strdup(buf);
        cmd = strtok(tbuf, delim);
        for (i = 0; i < HYD_NUM_TMP_STRINGS; i++) {
            targs[i] = strtok(NULL, delim);
            if (targs[i] == NULL)
                break;
        }

        /* Make a pass through targs and merge space separated
         * arguments which are actually part of the same key */
        k = 0;
        for (i = 0; targs[i]; i++) {
            if (!strrchr(targs[i], ' ')) {
                /* no spaces */
                args[k++] = HYDU_strdup(targs[i]);
            }
            else {
                /* space in the argument; each segment is either a new
                 * key, or a space-separated part of the previous
                 * key */
                j = 0;
                seg = strtok(targs[i], " ");
                while (1) {
                    if (!seg || strrchr(seg, '=')) {
                        /* segment has an '='; it's a start of a new key */
                        if (j) {
                            tmp[j++] = NULL;
                            status = HYDU_str_alloc_and_join(tmp, &args[k++]);
                            HYDU_ERR_POP(status, "error while joining strings\n");
                            HYDU_free_strlist(tmp);
                        }
                        j = 0;

                        if (!seg)
                            break;
                    }
                    else {
                        /* no '='; part of the previous key */
                        tmp[j++] = HYDU_strdup(" ");
                    }
                    tmp[j++] = HYDU_strdup(seg);

                    seg = strtok(NULL, " ");
                }
            }
        }
        args[k++] = NULL;
    }
    else {      /* PMI-v2 */
        delim = ";";

        tbuf = HYDU_strdup(buf);
        cmd = strtok(tbuf, delim);
        for (i = 0; i < HYD_NUM_TMP_STRINGS; i++) {
            args[i] = strtok(NULL, delim);
            if (args[i] == NULL)
                break;
            args[i] = HYDU_strdup(args[i]);
        }
    }

    /* Search for the PMI command in our table */
    status = HYDU_strsplit(cmd, &str1, pmi_cmd, '=');
    HYDU_ERR_POP(status, "string split returned error\n");

  fn_exit:
    HYDU_FREE(buf);
    if (tbuf)
        HYDU_FREE(tbuf);
    if (str1)
        HYDU_FREE(str1);
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYD_pmcd_pmi_args_to_tokens(char *args[], struct HYD_pmcd_token **tokens,
                                       int *count)
{
    int i, j;
    char *arg;
    HYD_status status = HYD_SUCCESS;

    for (i = 0; args[i]; i++);
    *count = i;
    HYDU_MALLOC(*tokens, struct HYD_pmcd_token *, *count * sizeof(struct HYD_pmcd_token),
                status);

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
    HYDU_snprintf((*kvs)->kvs_name, PMI_MAXKVSLEN, "kvs_%d_%d", (int) getpid(), pgid);
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

HYD_status HYD_pmcd_pmi_add_kvs(const char *key, char *val, struct HYD_pmcd_pmi_kvs *kvs,
                                int *ret)
{
    struct HYD_pmcd_pmi_kvs_pair *key_pair, *run;
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
        run = kvs->key_pair;
        while (run->next) {
            if (!strcmp(run->key, key_pair->key)) {
                /* duplicate key found */
                *ret = -1;
                break;
            }
            run = run->next;
        }
        run->next = key_pair;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
