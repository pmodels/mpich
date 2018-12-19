/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2009 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "bsci.h"
#include "bscu.h"
#include "slurm.h"

#if defined(HAVE_SLURM_SLURM_H)
#include <slurm/slurm.h>        /* for slurm_hostlist_create */
#elif defined(HAVE_POSIX_REGCOMP)
#include <regex.h>      /* for POSIX regular expressions */

#define MAX_GMATCH 5    /* max number of atoms in group matches + 1 */
#define MAX_RMATCH 5    /* max number of atoms in range matches + 1 */
#define MAX_EMATCH 2    /* max number of atoms in element matches + 1 */
#define MAX_NNODES_STRLEN 8     /* max string len for node # incl. '\0' */
#endif

static int *tasks_per_node = NULL;
static struct HYD_node *global_node_list = NULL;

#if defined(HAVE_LIBSLURM)
static HYD_status list_to_nodes(char *str)
{
    hostlist_t hostlist;
    char *host;
    int k = 0;
    HYD_status status = HYD_SUCCESS;

    if ((hostlist = slurm_hostlist_create(str)) == NULL) {
        status = HYD_FAILURE;
        goto fn_fail;
    }

    for (host = slurm_hostlist_shift(hostlist); host; host = slurm_hostlist_shift(hostlist)) {
        status = HYDU_add_to_node_list(host, tasks_per_node[k++], &global_node_list);
        HYDU_ERR_POP(status, "unable to add to node list\n");
    }

    slurm_hostlist_destroy(hostlist);

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}
#elif defined(HAVE_POSIX_REGCOMP)
static HYD_status list_to_nodes(char *str)
{
    regex_t gmatch_old[2];
    regex_t gmatch_new[2];
    regex_t rmatch_old;
    regex_t rmatch_new;
    regex_t ematch_old;
    regex_t ematch_new;
    regmatch_t gmatch[2][MAX_GMATCH];
    regmatch_t rmatch[MAX_RMATCH];
    regmatch_t ematch[MAX_EMATCH];
    char hostname[MAX_HOSTNAME_LEN];
    char basename[MAX_HOSTNAME_LEN];
    char rbegin[MAX_NNODES_STRLEN];
    char rend[MAX_NNODES_STRLEN];
    char *gpattern[2];
    char *rpattern;
    char *epattern;
    char *string;
    char tmp[2];
    int j, begin, end, k = 0;
    HYD_status status = HYD_SUCCESS;

    string = MPL_strdup(str);

    /* compile regex patterns for nodelist matching */

    /* compile group-0 regex for old format: "[h00-h12,h14] | h00-h12 | h14" */
    regcomp(&gmatch_old[0],
            "(,|^)(\\[[-,a-zA-Z0-9]+\\]|[a-zA-Z]+[0-9]+-[a-zA-Z]+[0-9]+|[a-zA-Z]+[0-9]+)(,|$)",
            REG_EXTENDED);

    /* compile group-1 regex for old format: "h00-h12 | h14" */
    regcomp(&gmatch_old[1],
            "([[,]|^)([a-zA-Z]+[0-9]+-[a-zA-Z]+[0-9]+|[a-zA-Z]+[0-9]+)([],]|$)", REG_EXTENDED);

    /* compile range regex for old format: "h00-h12" */
    regcomp(&rmatch_old, "([a-zA-Z]+)([0-9]+)-([a-zA-Z]+)([0-9]+)", REG_EXTENDED);

    /* compile element regex for old format: "h14" */
    regcomp(&ematch_old, "([a-zA-Z]+[0-9]+)", REG_EXTENDED);

    /* compile group-0 regex for new format: "h-[00-12,14] | h-14" */
    regcomp(&gmatch_new[0], "(,|^)([a-zA-Z]+[0-9]*-)(\\[[-,0-9]+\\]|[0-9]+)(,|$)", REG_EXTENDED);

    /* compile group-1 regex for new format: "00-12 | 14" */
    regcomp(&gmatch_new[1], "([[,]|^)([0-9]+-[0-9]+|[0-9]+)([],]|$)", REG_EXTENDED);

    /* compile range regex for new format: "00-12" */
    regcomp(&rmatch_new, "([0-9]+)-([0-9]+)", REG_EXTENDED);

    /* compile element regex for new format: "14" */
    regcomp(&ematch_new, "([0-9]+)", REG_EXTENDED);

    gpattern[0] = string;

    /* match old group-0 pattern: (,|^)([h00-h12,h14] | h00-h12 | h14)(,|$) */
    while (*gpattern[0] && regexec(&gmatch_old[0], gpattern[0], MAX_GMATCH, gmatch[0], 0) == 0) {
        /* bound group-0 for group-1 matching: [h00-h12,h14],... -> [h00-h12,h14]\0... */
        tmp[0] = *(gpattern[0] + gmatch[0][0].rm_eo);
        *(gpattern[0] + gmatch[0][0].rm_eo) = 0;

        /* select second atom in group-0 */
        gpattern[1] = gpattern[0] + gmatch[0][2].rm_so;

        /* match group-1 pattern: ([|,|^)(h00-h12 | h14)(]|,|$) */
        while (*gpattern[1] && regexec(&gmatch_old[1], gpattern[1], MAX_GMATCH, gmatch[1], 0) == 0) {
            /* bound group-1 for range/element matching: h00-h12, -> h00-h12\0 | h14] -> h14\0 */
            tmp[1] = *(gpattern[1] + gmatch[1][0].rm_eo);
            *(gpattern[1] + gmatch[1][0].rm_eo) = 0;

            /* select second atom in group-1 */
            rpattern = gpattern[1] + gmatch[1][2].rm_so;
            epattern = rpattern;

            if (regexec(&rmatch_old, rpattern, MAX_RMATCH, rmatch, 0) == 0) {
                /* matched range: (h)(00)-(h)(12) */
                snprintf(basename, MAX_HOSTNAME_LEN, "%.*s%c",
                         (int) (rmatch[1].rm_eo - rmatch[1].rm_so), rpattern + rmatch[1].rm_so,
                         '\0');
                snprintf(rbegin, MAX_NNODES_STRLEN, "%.*s%c",
                         (int) (rmatch[2].rm_eo - rmatch[2].rm_so), rpattern + rmatch[2].rm_so,
                         '\0');
                snprintf(rend, MAX_NNODES_STRLEN, "%.*s%c",
                         (int) (rmatch[4].rm_eo - rmatch[4].rm_so), rpattern + rmatch[4].rm_so,
                         '\0');
                begin = atoi(rbegin);
                end = atoi(rend);

                /* expand range and add nodes to global node list */
                for (j = begin; j <= end; j++) {
                    snprintf(hostname, MAX_HOSTNAME_LEN, "%s%.*d%c",
                             basename, (int) (rmatch[2].rm_eo - rmatch[2].rm_so), j, '\0');
                    status =
                        HYDU_add_to_node_list(hostname, tasks_per_node[k++], &global_node_list);
                    HYDU_ERR_POP(status, "unable to add to node list\n");
                }
            } else if (regexec(&ematch_old, epattern, MAX_EMATCH, ematch, 0) == 0) {
                /* matched element: (h14) */
                snprintf(hostname, MAX_HOSTNAME_LEN, "%.*s%c",
                         (int) (ematch[1].rm_eo - ematch[1].rm_so), epattern + ematch[1].rm_so,
                         '\0');
                status = HYDU_add_to_node_list(hostname, tasks_per_node[k++], &global_node_list);
                HYDU_ERR_POP(status, "unable to add to node list\n");
            }

            /* unbound group-1 and move to next group-1: h00-h12\0 -> h00-h12, | h14\0 -> h14] */
            *(gpattern[1] + gmatch[1][0].rm_eo) = tmp[1];
            gpattern[1] += gmatch[1][0].rm_eo;
        }

        /* unbound group-0 and move to next group-0: [h00-h12,h14]\0... -> [h00-h12,h14],... */
        *(gpattern[0] + gmatch[0][0].rm_eo) = tmp[0];
        gpattern[0] += gmatch[0][0].rm_eo;
    }

    /* match new group-0 pattern: (,|^)(h-)([00-12,14] | 00-12 | 14)(,|$) */
    while (*gpattern[0] && regexec(&gmatch_new[0], gpattern[0], MAX_GMATCH, gmatch[0], 0) == 0) {
        /* bound group-0 for group-1 matching: h-[00-h12,14],... -> h-[00-12,14]\0... */
        tmp[0] = *(gpattern[0] + gmatch[0][0].rm_eo);
        *(gpattern[0] + gmatch[0][0].rm_eo) = 0;

        /* extranct basename from atom 2 in group-0 */
        sprintf(basename, "%.*s%c",
                (int) (gmatch[0][2].rm_eo - gmatch[0][2].rm_so), gpattern[0] + gmatch[0][2].rm_so,
                '\0');

        /* select third atom in group-0 */
        gpattern[1] = gpattern[0] + gmatch[0][3].rm_so;

        /* match new group-1 pattern: ([|,|^)(00-12 | 14)(]|,|$) */
        while (*gpattern[1] && regexec(&gmatch_new[1], gpattern[1], MAX_GMATCH, gmatch[1], 0) == 0) {
            /* bound group-1 for range/element matching: 00-12, -> 00-12\0 | 14] -> 14\0 */
            tmp[1] = *(gpattern[1] + gmatch[1][0].rm_eo);
            *(gpattern[1] + gmatch[1][0].rm_eo) = 0;

            /* select second atom in group-1 */
            rpattern = gpattern[1] + gmatch[1][2].rm_so;
            epattern = rpattern;

            if (regexec(&rmatch_new, rpattern, MAX_RMATCH, rmatch, 0) == 0) {
                /* matched range: (00)-(10) */
                snprintf(rbegin, MAX_NNODES_STRLEN, "%.*s%c",
                         (int) (rmatch[1].rm_eo - rmatch[1].rm_so), rpattern + rmatch[1].rm_so,
                         '\0');
                snprintf(rend, MAX_NNODES_STRLEN, "%.*s%c",
                         (int) (rmatch[2].rm_eo - rmatch[2].rm_so), rpattern + rmatch[2].rm_so,
                         '\0');
                begin = atoi(rbegin);
                end = atoi(rend);

                /* expand range and add nodes to global node list */
                for (j = begin; j <= end; j++) {
                    sprintf(hostname, "%s%.*d%c",
                            basename, (int) (rmatch[1].rm_eo - rmatch[1].rm_so), j, '\0');
                    status =
                        HYDU_add_to_node_list(hostname, tasks_per_node[k++], &global_node_list);
                    HYDU_ERR_POP(status, "unable to add to node list\n");
                }
            } else if (regexec(&ematch_new, epattern, MAX_EMATCH, ematch, 0) == 0) {
                /* matched element: (14) */
                sprintf(rbegin, "%.*s%c",
                        (int) (ematch[1].rm_eo - ematch[1].rm_so), epattern + ematch[1].rm_so,
                        '\0');
                sprintf(hostname, "%s%s%c", basename, rbegin, '\0');
                status = HYDU_add_to_node_list(hostname, tasks_per_node[k++], &global_node_list);
                HYDU_ERR_POP(status, "unable to add to node list\n");
            }

            /* unbound group-1 and move to next group-1: 00-10\0 -> 00-10, | 14\0 -> 14] */
            *(gpattern[1] + gmatch[1][0].rm_eo) = tmp[1];
            gpattern[1] += gmatch[1][0].rm_eo;
        }

        /* unbound group-0 and move to next group-0: h-[00-12,14]\0... -> h-[00-12,14],... */
        *(gpattern[0] + gmatch[0][0].rm_eo) = tmp[0];
        gpattern[0] += gmatch[0][0].rm_eo;
    }

    /* clean up match patterns */
    regfree(&gmatch_old[0]);
    regfree(&gmatch_new[0]);
    regfree(&gmatch_old[1]);
    regfree(&gmatch_new[1]);
    regfree(&rmatch_old);
    regfree(&rmatch_new);
    regfree(&ematch_old);
    regfree(&ematch_new);

    /* free local nodelist */
    MPL_free(string);

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}
#else
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
    tnodes = MPL_strdup(nodes);
    tmp = strtok(tnodes, ",");
    for (i = 1; tmp; i++)
        tmp = strtok(NULL, ",");

    HYDU_MALLOC_OR_JUMP(set, char **, i * sizeof(char *), status);

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
static HYD_status list_to_nodes(char *str)
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
#endif

static HYD_status extract_tasks_per_node(int nnodes, char *task_list)
{
    char *task_set, **tmp_core_list = NULL;
    char *nodes, *cores;
    int i, j, k, p, count = 0;
    HYD_status status = HYD_SUCCESS;

    HYDU_MALLOC_OR_JUMP(tasks_per_node, int *, nnodes * sizeof(int), status);
    HYDU_MALLOC_OR_JUMP(tmp_core_list, char **, nnodes * sizeof(char *), status);

    task_set = strtok(task_list, ",");
    HYDU_ASSERT(task_set, status);
    i = 0;
    do {
        HYDU_MALLOC_OR_JUMP(tmp_core_list[i], char *, strlen(task_set) + 1, status);
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
        } else
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

    list_to_nodes(list);
    *node_list = global_node_list;

  fn_exit:
    MPL_free(tasks_per_node);
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
