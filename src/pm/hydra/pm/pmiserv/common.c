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
        args[i] = MPL_strdup(args[i]);
    }

    /* Search for the PMI command in our table */
    status = HYDU_strsplit(cmd, &str1, pmi_cmd, '=');
    HYDU_ERR_POP(status, "string split returned error\n");

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

    HYDU_FUNC_ENTER();

    HYDU_MALLOC_OR_JUMP(*kvs, struct HYD_pmcd_pmi_kvs *, sizeof(struct HYD_pmcd_pmi_kvs), status);
    MPL_snprintf((*kvs)->kvsname, PMI_MAXKVSLEN, "kvs_%d_%d", (int) getpid(), pgid);
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
    }
    else {
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

HYD_status HYDU_send_start_children(int fd, char *nodes, int proxy_id)
{
    struct HYD_pmcd_hdr hdr;
    int sent, closed;
    HYD_status status = HYD_SUCCESS;
    HYDU_FUNC_ENTER();

    HYD_pmcd_init_header(&hdr);
    hdr.cmd = LAUNCH_CHILD_PROXY;
    hdr.buflen = strlen(nodes);
    hdr.pid = proxy_id;
    hdr.rank = 0;

    status = HYDU_sock_write(fd, &hdr, sizeof(hdr), &sent, &closed, HYDU_SOCK_COMM_MSGWAIT);
    HYDU_ERR_POP(status, "unable to send LAUNCH_CHILD_PROXY header to proxy\n");
    HYDU_ASSERT(!closed, status);

    status = HYDU_sock_write(fd, nodes, hdr.buflen, &sent, &closed, HYDU_SOCK_COMM_MSGWAIT);
    HYDU_ERR_POP(status, "unable to send response to command\n");
    HYDU_ASSERT(!closed, status);

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status node_list_to_strings(struct HYD_node *node_list, int size, char **node_list_str)
{
    int i, j, k;
    char *tmp[HYD_NUM_TMP_STRINGS], *foo = NULL;
    struct HYD_node *node;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    i = 0;
    j = 0;
    k = 0;
    for (node = node_list; node; node = node->next) {
        tmp[i++] = MPL_strdup(node->hostname);
        tmp[i++] = MPL_strdup(",");
        tmp[i++] = HYDU_int_to_str(node->core_count);
        tmp[i++] = MPL_strdup(",");
        tmp[i++] = HYDU_int_to_str(node->node_id);
        j++;
        if (j == size) {
            tmp[i] = NULL;
            status = HYDU_str_alloc_and_join(tmp, &foo);
            HYDU_ERR_POP(status, "error joining strings\n");
            HYDU_free_strlist(tmp);

            node_list_str[k++] = MPL_strdup(foo);
            MPL_free(foo);
            i = 0;
            j = 0;
        } else {
            if (node->next)
                tmp[i++] = MPL_strdup(",");

            /* If we used up more than half of the array elements, merge
             * what we have so far */
            if (i > (HYD_NUM_TMP_STRINGS / 2)) {
                tmp[i] = NULL;
                status = HYDU_str_alloc_and_join(tmp, &foo);
                HYDU_ERR_POP(status, "error joining strings\n");
                HYDU_free_strlist(tmp);

                i = 0;
                tmp[i++] = MPL_strdup(foo);
                MPL_free(foo);
            }
        }
    }

    if (i != 0) {
        tmp[i] = NULL;
        status = HYDU_str_alloc_and_join(tmp, &foo);
        HYDU_ERR_POP(status, "error joining strings\n");
        HYDU_free_strlist(tmp);

        node_list_str[k] = MPL_strdup(foo);
        MPL_free(foo);
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
