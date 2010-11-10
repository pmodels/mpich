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
        if ((end_str = strtok(NULL, "-")) == NULL)
            end_str = start_str;

        start = atoi(start_str);
        end = atoi(end_str);

        for (j = start; j <= end; j++) {
            node_str[0] = HYDU_strdup(pre);
            node_str[1] = HYDU_int_to_str_pad(j, strlen(start_str));
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

static HYD_status extract_tasks_per_node(int **tasks_per_node, int *nnodes, int *valid)
{
    char *task_list, *task_set, **tmp_core_list;
    char *dummy, *nodes, *cores;
    int i, j, k, p, count;
    HYD_status status = HYD_SUCCESS;

    *valid = 1;

    if (MPL_env2str("SLURM_NNODES", (const char **) &dummy) == 0) {
        *valid = 0;
        goto fn_exit;
    }
    *nnodes = atoi(dummy);

    if (MPL_env2str("SLURM_TASKS_PER_NODE", (const char **) &task_list) == 0) {
        *valid = 0;
        goto fn_exit;
    }
    task_list = HYDU_strdup(task_list);

    HYDU_MALLOC(*tasks_per_node, int *, *nnodes * sizeof(int), status);
    HYDU_MALLOC(tmp_core_list, char **, *nnodes * sizeof(char *), status);

    task_set = strtok(task_list, ",");
    i = 0;
    do {
        HYDU_MALLOC(tmp_core_list[i], char *, sizeof(task_set) + 1, status);
        MPL_snprintf(tmp_core_list[i], sizeof(task_set) + 1, "%s", task_set);
        i++;
        task_set = strtok(NULL, ",");
    } while (task_set);
    count = i;

    p = 0;
    for (i = 0; i < count; i++) {
        cores = strtok(tmp_core_list[i], "(");
        nodes = strtok(NULL, "(");
        if (nodes) {
            nodes[strlen(nodes) - 1] = 0;
            nodes++;
            j = atoi(nodes);
        }
        else
            j = 1;

        for (k = 0; k < j; k++)
            (*tasks_per_node)[p++] = atoi(cores);
    }

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYDT_bscd_slurm_query_node_list(struct HYD_node **node_list)
{
    char *str, *tstr = NULL;
    char *tmp1[HYD_NUM_TMP_STRINGS], *tmp2[HYD_NUM_TMP_STRINGS];
    struct HYD_node *node, *tnode;
    int i, j, k;
    int *tasks_per_node = NULL, nnodes, valid;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (MPL_env2str("SLURM_NODELIST", (const char **) &str) == 0)
        str = NULL;

    status = extract_tasks_per_node(&tasks_per_node, &nnodes, &valid);
    HYDU_ERR_POP(status, "unable to extract the number of tasks per node\n");

    if (str == NULL || !valid) {
        *node_list = NULL;
    }
    else {
        tstr = HYDU_strdup(str);
        full_str_to_groups(str, tmp1);

        k = 0;
        for (i = 0; tmp1[i]; i++) {
            status = group_to_individual_nodes(tmp1[i], tmp2);
            HYDU_ERR_POP(status, "unable to parse node list\n");

            for (j = 0; tmp2[j]; j++) {
                status = HYDU_alloc_node(&node);
                HYDU_ERR_POP(status, "unable to allocate note\n");

                node->hostname = HYDU_strdup(tmp2[j]);
                node->core_count = tasks_per_node[k++];

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

        if (tstr)
            HYDU_FREE(tstr);
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
