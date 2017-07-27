/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2009 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_rmk_slurm.h"
#include "hydra_err.h"
#include "hydra_mem.h"
#include "hydra_str.h"

static int *tasks_per_node = NULL;
static struct HYD_node *global_node_list = NULL;
static int ncount = 0;
static int max_ncount = 0;

static HYD_status group_to_nodes(char *str)
{
    char *nodes, *tnodes, *tmp, *start_str, *end_str, **set;
    int start, end, i, j, k = 0;
    HYD_status status = HYD_SUCCESS;

    for (tmp = str; *tmp != '[' && *tmp != 0; tmp++);

    if (*tmp == 0) {    /* only one node in the group */
        status =
            HYD_node_list_append(str, tasks_per_node[k++], &global_node_list, &ncount, &max_ncount);
        HYD_ERR_POP(status, "unable to add to node list\n");

        goto fn_exit;
    }

    /* more than one node in the group */
    *tmp = 0;
    nodes = tmp + 1;

    for (tmp = nodes; *tmp != ']' && *tmp != 0; tmp++);
    *tmp = 0;   /* remove the closing ']' */

    /* Find the number of sets */
    tnodes = MPL_strdup(nodes);
    tmp = strtok(tnodes, ",");
    for (i = 1; tmp; i++)
        tmp = strtok(NULL, ",");

    HYD_MALLOC(set, char **, i * sizeof(char *), status);

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

            node_str[0] = MPL_strdup(str);
            node_str[1] = HYD_str_from_int_pad(j, strlen(start_str));
            node_str[2] = NULL;

            status = HYD_str_alloc_and_join(node_str, &tmp);
            HYD_ERR_POP(status, "unable to join strings\n");

            HYD_str_free_list(node_str);

            status =
                HYD_node_list_append(tmp, tasks_per_node[k++], &global_node_list, &ncount,
                                     &max_ncount);
            HYD_ERR_POP(status, "unable to add to node list\n");
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
    char *tmp, group[3 * HYD_MAX_HOSTNAME_LEN]; /* maximum group size */
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
            HYD_ERR_POP(status, "unable to split group to nodes\n");

            i = -1;
        }
        if (*tmp == 0) {
            group[++i] = 0;

            status = group_to_nodes(group);
            HYD_ERR_POP(status, "unable to split group to nodes\n");

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

    HYD_MALLOC(tasks_per_node, int *, nnodes * sizeof(int), status);
    HYD_MALLOC(tmp_core_list, char **, nnodes * sizeof(char *), status);

    task_set = strtok(task_list, ",");
    i = 0;
    do {
        HYD_MALLOC(tmp_core_list[i], char *, strlen(task_set) + 1, status);
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
        MPL_free(tmp_core_list[i]);
    MPL_free(tmp_core_list);
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYDI_rmk_slurm_query_node_list(int *node_count, struct HYD_node **nodes)
{
    char *list, *task_list;
    int nnodes;
    HYD_status status = HYD_SUCCESS;

    HYD_FUNC_ENTER();

    MPL_env2str("SLURM_NODELIST", (const char **) &list);
    MPL_env2str("SLURM_TASKS_PER_NODE", (const char **) &task_list);
    MPL_env2int("SLURM_NNODES", &nnodes);

    status = extract_tasks_per_node(nnodes, task_list);
    HYD_ERR_POP(status, "unable to extract the number of tasks per node\n");

    list_to_groups(list);
    *nodes = global_node_list;
    *node_count = ncount;

  fn_exit:
    MPL_free(tasks_per_node);
    HYD_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
