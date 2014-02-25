/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2009 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "bsci.h"
#include "bscu.h"
#include "slurm.h"

static int *tasks_per_node = NULL;
static struct HYD_node *global_node_list = NULL;

static HYD_status group_to_nodes(char *str)
{
    char *nodes, *tnodes, *tmp, *start_str, *end_str, **set;
    int start, end, i, j, k = 0;
    HYD_status status = HYD_SUCCESS;

    for (tmp = str; *tmp != '[' && *tmp != 0; tmp++);

    if (*tmp == 0) {    /* only one node in the group */
        status = HYDU_add_to_node_list(str, tasks_per_node[k++], &global_node_list);
        HYDU_ERR_POP(status, "unable to add to node list\n");

        goto fn_exit;
    }

    /* more than one node in the group */
    *tmp = 0;
    nodes = tmp + 1;

    for (tmp = nodes; *tmp != ']' && *tmp != 0; tmp++);
    *tmp = 0;   /* remove the closing ']' */

    /* Find the number of sets */
    tnodes = HYDU_strdup(nodes);
    tmp = strtok(tnodes, ",");
    for (i = 1; tmp; i++)
        tmp = strtok(NULL, ",");

    HYDU_MALLOC(set, char **, i * sizeof(char *), status);

    /* Find the actual node sets */
    set[0] = strtok(nodes, ",");
    for (i = 1; set[i - 1]; i++)
        set[i] = strtok(NULL, ",");

    for (i = 0; set[i]; i++) {
        start_str = strtok(set[i], "-");
        if ((end_str = strtok(NULL, "-")) == NULL)
            end_str = start_str;

        start = atoi(start_str);
        end = atoi(end_str);

        for (j = start; j <= end; j++) {
            char *node_str[HYD_NUM_TMP_STRINGS];

            node_str[0] = HYDU_strdup(str);
            node_str[1] = HYDU_int_to_str_pad(j, strlen(start_str));
            node_str[2] = NULL;

            status = HYDU_str_alloc_and_join(node_str, &tmp);
            HYDU_ERR_POP(status, "unable to join strings\n");

            HYDU_free_strlist(node_str);

            status = HYDU_add_to_node_list(tmp, tasks_per_node[k++], &global_node_list);
            HYDU_ERR_POP(status, "unable to add to node list\n");
        }
    }

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

/* List is a comma separated collection of groups, where each group is
 * of the form "[host001-host012]", "host001-host012", "host009",
 * "[host001-host012,host014-host020]", etc. */
static HYD_status list_to_groups(char *str)
{
    char *tmp, group[3 * MAX_HOSTNAME_LEN];     /* maximum group size */
    int nesting, i;
    HYD_status status = HYD_SUCCESS;

    tmp = str;
    i = 0;
    nesting = 0;

    while (1) {
        group[i] = *tmp;
        if (*tmp == '[')
            nesting++;
        if (*tmp == ']')
            nesting--;
        if (*tmp == ',' && nesting == 0) {
            group[i] = 0;

            status = group_to_nodes(group);
            HYDU_ERR_POP(status, "unable to split group to nodes\n");

            i = -1;
        }
        if (*tmp == 0) {
            group[++i] = 0;

            status = group_to_nodes(group);
            HYDU_ERR_POP(status, "unable to split group to nodes\n");

            break;
        }

        i++;
        tmp++;
    }

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status extract_tasks_per_node(int nnodes, char *task_list)
{
    char *task_set, **tmp_core_list = NULL;
    char *nodes, *cores;
    int i, j, k, p, count = 0;
    HYD_status status = HYD_SUCCESS;

    HYDU_MALLOC(tasks_per_node, int *, nnodes * sizeof(int), status);
    HYDU_MALLOC(tmp_core_list, char **, nnodes * sizeof(char *), status);

    task_set = strtok(task_list, ",");
    i = 0;
    do {
        HYDU_MALLOC(tmp_core_list[i], char *, strlen(task_set) + 1, status);
        MPL_snprintf(tmp_core_list[i], strlen(task_set) + 1, "%s", task_set);
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
            tasks_per_node[p++] = atoi(cores);
    }

  fn_exit:
    for (i = 0; i < count; i++)
        HYDU_FREE(tmp_core_list[i]);
    HYDU_FREE(tmp_core_list);
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYDT_bscd_slurm_query_node_list(struct HYD_node **node_list)
{
    char *list, *dummy, *task_list;
    int nnodes;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (MPL_env2str("SLURM_NODELIST", (const char **) &list) == 0) {
        *node_list = NULL;
        goto fn_exit;
    }

    if (MPL_env2str("SLURM_NNODES", (const char **) &dummy) == 0) {
        *node_list = NULL;
        goto fn_exit;
    }
    nnodes = atoi(dummy);

    if (MPL_env2str("SLURM_TASKS_PER_NODE", (const char **) &task_list) == 0) {
        *node_list = NULL;
        goto fn_exit;
    }

    status = extract_tasks_per_node(nnodes, task_list);
    HYDU_ERR_POP(status, "unable to extract the number of tasks per node\n");

    list_to_groups(list);
    *node_list = global_node_list;

  fn_exit:
    HYDU_FREE(tasks_per_node);
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
