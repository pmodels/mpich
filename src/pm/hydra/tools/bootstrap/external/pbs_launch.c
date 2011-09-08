/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "bsci.h"
#include "pbs.h"

HYD_status HYDT_bscd_pbs_launch_procs(char **args, struct HYD_proxy *proxy_list,
                                      int *control_fd)
{
    int proxy_count, i, args_count, events_count, err, idx, hostid;
    struct HYD_proxy *proxy;
    char *targs[HYD_NUM_TMP_STRINGS];
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    /* If the RMK is not PBS, error out for the time being. This needs
     * to be modified to reparse the host file and find the spawn IDs
     * separately. */
    if (strcmp(HYDT_bsci_info.rmk, "pbs"))
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR,
                            "using a non-PBS RMK with the PBS launcher is not supported\n");

    proxy_count = 0;
    for (proxy = proxy_list; proxy; proxy = proxy->next)
        proxy_count++;

    /* Duplicate the args in local copy, targs */
    for (args_count = 0; args[args_count]; args_count++)
        targs[args_count] = HYDU_strdup(args[args_count]);

    HYDU_MALLOC(HYDT_bscd_pbs_sys->task_id, tm_task_id *, proxy_count * sizeof(tm_task_id),
                status);
    HYDU_MALLOC(HYDT_bscd_pbs_sys->spawn_events, tm_event_t *,
                proxy_count * sizeof(tm_event_t), status);

    /* Spawn a process on each allocated node through tm_spawn() which
     * returns a taskID for the process + a eventID for the
     * spawning. */
    hostid = 0;
    for (i = 0, proxy = proxy_list; proxy; proxy = proxy->next, i++) {
        targs[args_count] = HYDU_int_to_str(i);

        /* The task_id field is not filled in during tm_spawn(). The
         * TM library just stores this address and fills it in when
         * the event is completed by a call to tm_poll(). */
        err = tm_spawn(args_count + 1, targs, NULL, hostid, &HYDT_bscd_pbs_sys->task_id[i],
                       &HYDT_bscd_pbs_sys->spawn_events[i]);
        HYDU_ERR_CHKANDJUMP(status, err != TM_SUCCESS, HYD_INTERNAL_ERROR,
                            "tm_spawn() failed with TM error %d\n", err);

        hostid += proxy->node->core_count;
    }
    HYDT_bscd_pbs_sys->spawn_count = i;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
