/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2017 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_hostfile.h"
#include "hydra_err.h"
#include "hydra_str.h"
#include "hydra_mem.h"

HYD_status HYD_hostfile_process_tokens(char **tokens, struct HYD_node *node)
{
    char *procs, *tmp;
    int i;
    HYD_status status = HYD_SUCCESS;

    MPL_VG_MEM_INIT(node, sizeof(struct HYD_node));

    /* tokens[0] is the hostname and core count */
    MPL_strncpy(node->hostname, strtok(tokens[0], ":"), HYD_MAX_HOSTNAME_LEN);

    procs = strtok(NULL, ":");
    node->core_count = procs ? atoi(procs) : 1;

    /* parse the rest of the tokens */
    for (i = 1; tokens[i]; i++) {
        tmp = strtok(tokens[i], "=");
        if (!strcmp(tmp, "user")) {
            MPL_strncpy(node->username, strtok(NULL, "="), HYD_MAX_USERNAME_LEN);
        }
        else {
            HYD_ERR_SETANDJUMP(status, HYD_ERR_INTERNAL, "token %s not supported at this time\n",
                               tokens[i]);
        }
    }

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYD_hostfile_parse(const char *hostfile, int *node_count, struct HYD_node **nodes,
                              HYD_status(*process_tokens) (char **tokens, struct HYD_node * node))
{
    char line[HYD_TMP_STRLEN], **tokens;
    FILE *fp;
    int nodes_alloc = 1;
    struct HYD_node node;
    HYD_status status = HYD_SUCCESS;

    HYD_FUNC_ENTER();

    if ((fp = fopen(hostfile, "r")) == NULL)
        HYD_ERR_SETANDJUMP(status, HYD_ERR_INTERNAL, "unable to open host file: %s\n", hostfile);

    HYD_MALLOC(*nodes, struct HYD_node *, nodes_alloc * sizeof(struct HYD_node), status);

    *node_count = 0;
    while (fgets(line, HYD_TMP_STRLEN, fp)) {
        char *linep = NULL;

        linep = line;

        strtok(linep, "#");
        while (isspace(*linep))
            linep++;

        /* Ignore blank lines & comments */
        if ((*linep == '#') || (*linep == '\0'))
            continue;

        tokens = HYD_str_to_strlist(linep);
        if (!tokens)
            HYD_ERR_SETANDJUMP(status, HYD_ERR_INTERNAL,
                               "Unable to convert host file entry to strlist\n");

        status = process_tokens(tokens, &node);
        HYD_ERR_POP(status, "unable to process token\n");

        /* if the hostname is the same as the last one in our list,
         * simply append it */
        if (*node_count && !strcmp(node.hostname, (*nodes)[(*node_count) - 1].hostname))
            (*nodes)[*node_count - 1].core_count += node.core_count;
        else {
            if (*node_count == nodes_alloc) {
                HYD_REALLOC(*nodes, struct HYD_node *, nodes_alloc * 2 * sizeof(struct HYD_node),
                            status);
                nodes_alloc *= 2;
            }

            if (*node_count == 0)
                node.node_id = 0;
            else
                node.node_id = (*nodes)[(*node_count) - 1].node_id + 1;

            memcpy(&(*nodes)[*node_count], &node, sizeof(struct HYD_node));

            (*node_count)++;
        }

        HYD_str_free_list(tokens);
        MPL_free(tokens);
    }

    fclose(fp);

  fn_exit:
    HYD_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
