/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "bsci.h"
#include "pbs.h"

/* #define TS_PROFILE 1 */

#if defined(TS_PROFILE)
#include <sys/time.h>
double TS_Wtime( void );
double TS_Wtime( void )
{
    struct timeval  tval;
    gettimeofday( &tval, NULL );
    return ( (double) tval.tv_sec + (double) tval.tv_usec * 0.000001 );
}
#endif

HYD_status HYDT_bscd_pbs_launch_procs(char **args, struct HYD_proxy *proxy_list,
                                      int *control_fd)
{
    int proxy_count, spawned_count, args_count;
    int ierr, spawned_hostID;
    struct HYD_proxy *proxy;
    char *targs[HYD_NUM_TMP_STRINGS];
    HYD_status status = HYD_SUCCESS;

#if defined(TS_PROFILE)
    double stime, etime;
#endif

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
    HYDT_bscd_pbs_sys->size = proxy_count;

    /* Check if number of proxies > number of processes in this PBS job */
    if (proxy_count > (HYDT_bscd_pbs_sys->tm_root).tm_nnodes)
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR,
                            "Number of proxies(%d) > TM node count(%d)!\n",
                            proxy_count,
                            (HYDT_bscd_pbs_sys->tm_root).tm_nnodes);

    /* Duplicate the args in local copy, targs */
    for (args_count = 0; args[args_count]; args_count++)
        targs[args_count] = HYDU_strdup(args[args_count]);

    /* Allocate memory for taskIDs[] and events[] arrays */
    HYDU_MALLOC(HYDT_bscd_pbs_sys->taskIDs, tm_task_id *,
                HYDT_bscd_pbs_sys->size * sizeof(tm_task_id), status);
    HYDU_MALLOC(HYDT_bscd_pbs_sys->events, tm_event_t *,
                HYDT_bscd_pbs_sys->size * sizeof(tm_event_t), status);

#if defined(TS_PROFILE)
    stime = TS_Wtime();
#endif
    /* Spawn a process on each allocated node through tm_spawn() which
     * returns a taskID for the process + a eventID for the spawning.
     * The returned taskID won't be ready for access until tm_poll()
     * has returned the corresponding eventID. */
    spawned_count   = 0;
    spawned_hostID  = 0;
    for (proxy = proxy_list; proxy; proxy = proxy->next) {
        targs[args_count] = HYDU_int_to_str(proxy->proxy_id);
        ierr = tm_spawn(args_count + 1, targs, NULL, spawned_hostID,
                        HYDT_bscd_pbs_sys->taskIDs + spawned_count,
                        HYDT_bscd_pbs_sys->events + spawned_count);
        if (HYDT_bsci_info.debug)
            HYDU_dump(stdout, "PBS_DEBUG: %d, tm_spawn(hostID=%d,name=%s)\n",
                      spawned_count, spawned_hostID, proxy->node->hostname);
        if (ierr != TM_SUCCESS) {
            HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR,
                                "tm_spawn() fails with TM err %d!\n", ierr);
        }
        spawned_hostID += proxy->node->core_count;
        spawned_count++;
    }
    HYDT_bscd_pbs_sys->spawned_count = spawned_count;
#if defined(TS_PROFILE)
    etime = TS_Wtime();
    HYDU_dump(stdout, "tm_spawn() loop takes %f\n", etime-stime );
#endif

#ifdef 0
    /* Poll the TM for the spawning eventID returned by tm_spawn() to
     * determine if the spawned process has started. */
#if defined(TS_PROFILE)
    stime = TS_Wtime();
#endif
    events_count = 0;
    while (events_count < spawned_count) {
        tm_event_t event = -1;
        int poll_err;
        ierr = tm_poll(TM_NULL_EVENT, &event, 0, &poll_err);
        if (ierr != TM_SUCCESS)
            HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR,
                                "tm_poll(spawn_event) fails with TM err %d.\n",
                                ierr);
        if (event != TM_NULL_EVENT) {
            for (idx = 0; idx < spawned_count; idx++) {
                if (HYDT_bscd_pbs_sys->events[idx] == event) {
                    if (HYDT_bsci_info.debug) {
                        HYDU_dump(stdout,
                                  "PBS_DEBUG: Event %d received, task %d has started.\n",
                                  event, HYDT_bscd_pbs_sys->taskIDs[idx]);
                    }
                    events_count++;
                    break; /* break from for(idx<spawned_count) loop */
                }
            }
        }
    }
#if defined(TS_PROFILE)
    etime = TS_Wtime();
    HYDU_dump(stdout, "tm_poll(spawn_events) loop takes %f\n", etime-stime );
#endif
#endif

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
