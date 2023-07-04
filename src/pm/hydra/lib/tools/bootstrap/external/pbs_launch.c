/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "hydra.h"
#include "bsci.h"
#include "pbs.h"

static struct HYD_node *pbs_node_list = NULL;

static HYD_status find_pbs_node_id(const char *hostname, int *node_id)
{
    struct HYD_node *t;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    *node_id = 0;
    for (t = pbs_node_list; t; t = t->next) {
        if (!strcmp(hostname, t->hostname))
            break;
        *node_id += t->core_count;
    }

    HYDU_ERR_CHKANDJUMP(status, t == NULL, HYD_INTERNAL_ERROR,
                        "user specified host not in the PBS allocated list\n");

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

/* NOTE: tree-launch not supported, params k, myid are ignored. */
HYD_status HYDT_bscd_pbs_launch_procs(int pgid, char **args, struct HYD_host *hosts,
                                      int num_hosts, int use_rmk, int k, int myid, int *control_fd)
{
    int i, args_count, err, hostid;
    char *targs[HYD_NUM_TMP_STRINGS];
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    /* If the RMK is not PBS, query for the PBS node list, and convert
     * the user-specified node IDs to PBS node IDs */
    if (use_rmk == HYD_FALSE || strcmp(HYDT_bsci_info.rmk, "pbs")) {
        status = HYDT_bscd_pbs_query_node_list(&pbs_node_list);
        HYDU_ERR_POP(status, "error querying PBS node list\n");
    }

    /* Duplicate the args in local copy, targs */
    int idx = 0;
    int id_idx, host_idx;
    for (args_count = 0; args[args_count]; args_count++)
        targs[idx++] = MPL_strdup(args[args_count]);

    targs[idx++] = MPL_strdup("--hostname");
    targs[idx++] = NULL;
    host_idx = idx - 1;
    targs[idx++] = MPL_strdup("--proxy-id");
    targs[idx++] = NULL;
    id_idx = idx - 1;

    args_count = idx;
    targs[args_count] = NULL;

    HYDU_MALLOC_OR_JUMP(HYDT_bscd_pbs_sys->task_id, tm_task_id *, num_hosts * sizeof(tm_task_id),
                        status);
    HYDU_MALLOC_OR_JUMP(HYDT_bscd_pbs_sys->spawn_events, tm_event_t *,
                        num_hosts * sizeof(tm_event_t), status);

    /* Spawn a process on each allocated node through tm_spawn() which
     * returns a taskID for the process + a eventID for the
     * spawning. */
    hostid = 0;
    for (i = 0; i < num_hosts; i++) {
        if (pbs_node_list) {
            status = find_pbs_node_id(hosts[i].hostname, &hostid);
            HYDU_ERR_POP(status, "error finding PBS node ID for host %s\n", hosts[i].hostname);
        }

        targs[id_idx] = HYDU_int_to_str(i);

        MPL_free(targs[host_idx]);
        targs[host_idx] = MPL_strdup(hosts[i].hostname);

        /* The task_id field is not filled in during tm_spawn(). The
         * TM library just stores this address and fills it in when
         * the event is completed by a call to tm_poll(). */
        if (HYDT_bsci_info.debug) {
            HYDU_dump(stdout, "Spawn arguments (host id %d): ", hostid);

            /* NULL terminate the arguments list to pass to
             * HYDU_print_strlist() */
            HYDU_print_strlist(targs);
        }

        /* The args_count below does not include the possible NULL
         * termination, as I'm not sure how tm_spawn() handles NULL
         * arguments. Besides the last NULL string is not needed for
         * tm_spawn(). */
        err = tm_spawn(args_count, targs, NULL, hostid, &HYDT_bscd_pbs_sys->task_id[i],
                       &HYDT_bscd_pbs_sys->spawn_events[i]);
        HYDU_ERR_CHKANDJUMP(status, err != TM_SUCCESS, HYD_INTERNAL_ERROR,
                            "tm_spawn() failed with TM error %d\n", err);

        if (!pbs_node_list)
            hostid += hosts[i].core_count;
    }
    HYDT_bscd_pbs_sys->spawn_count = i;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
