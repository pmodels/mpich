/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_utils.h"
#include "bsci.h"
#include "bscu.h"
#include "slurm.h"

int HYDT_bscd_slurm_user_node_list = 1;

static void full_str_to_groups(char *str, char **list)
{
    char *tmp;
    char new[MAX_HOSTNAME_LEN];
    int nesting, i, arg;

    tmp = str;
    i = 0;
    nesting = 0;
    arg = 0;

    while (1) {
        new[i] = *tmp;
        if (*tmp == '[')
            nesting++;
        if (*tmp == ']')
            nesting--;
        if (*tmp == ',' && nesting == 0) {
            new[i] = 0;
            list[arg++] = HYDU_strdup(new);
            i = -1;
        }
        if (*tmp == 0) {
            new[++i] = 0;
            list[arg++] = HYDU_strdup(new);
            break;
        }

        i++;
        tmp++;
    }
    list[arg++] = NULL;
}

static HYD_status group_to_individual_nodes(char *str, char **list)
{
    char *pre, *nodes, *tmp, *start_str, *end_str;
    char *node_str[HYD_NUM_TMP_STRINGS], *set[HYD_NUM_TMP_STRINGS];
    int start, end, arg, i, j;
    HYD_status status = HYD_SUCCESS;

    pre = HYDU_strdup(str);
    for (tmp = pre; *tmp != '[' && *tmp != 0; tmp++);

    if (*tmp == 0) {    /* only one node in the group */
        list[0] = pre;
        list[1] = NULL;
        goto fn_exit;
    }

    /* more than one node in the group */
    *tmp = 0;
    nodes = tmp + 1;

    for (tmp = nodes; *tmp != ']' && *tmp != 0; tmp++);
    *tmp = 0;   /* remove the closing ']' */

    for ((set[0] = strtok(nodes, ",")), i = 1; (set[i] = strtok(NULL, ",")); i++);

    arg = 0;
    for (i = 0; set[i]; i++) {
        start_str = strtok(set[i], "-");
        end_str = strtok(NULL, "-");

        start = atoi(start_str);
        end = end_str ? atoi(end_str) : start;

        for (j = start; j <= end; j++) {
            node_str[0] = HYDU_strdup(pre);
            node_str[1] = HYDU_int_to_str(j);
            node_str[2] = NULL;

            status = HYDU_str_alloc_and_join(node_str, &list[arg++]);
            HYDU_ERR_POP(status, "unable to join strings\n");

            HYDU_free_strlist(node_str);
        }
    }
    list[arg++] = NULL;

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYDT_bscd_slurm_query_node_list(struct HYD_node **node_list)
{
    char *str, *num_procs;
    char *tmp1[HYD_NUM_TMP_STRINGS], *tmp2[HYD_NUM_TMP_STRINGS];
    struct HYD_node *node, *tnode;
    int i, j;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    str = getenv("SLURM_NODELIST");
    num_procs = getenv("SLURM_JOB_CPUS_PER_NODE");

    if (str == NULL || num_procs == NULL) {
        *node_list = NULL;
    }
    else {
        full_str_to_groups(str, tmp1);
        num_procs = strtok(num_procs, "(");

        for (i = 0; tmp1[i]; i++) {
            status = group_to_individual_nodes(tmp1[i], tmp2);
            HYDU_ERR_POP(status, "unable to parse node list\n");

            for (j = 0; tmp2[j]; j++) {
                status = HYDU_alloc_node(&node);
                HYDU_ERR_POP(status, "unable to allocate note\n");

                node->hostname = HYDU_strdup(tmp2[j]);
                node->core_count = atoi(num_procs);

                if (*node_list == NULL)
                    *node_list = node;
                else {
                    for (tnode = *node_list; tnode->next; tnode = tnode->next);
                    tnode->next = node;
                }
            }
        }

        /* node list is provided by the bootstrap server */
        HYDT_bscd_slurm_user_node_list = 0;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
