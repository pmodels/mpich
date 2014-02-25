/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2011 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "bsci.h"
#include "pbs.h"

HYD_status HYDT_bscd_pbs_wait_for_completion(int timeout)
{
    int time_elapsed, events_count, spawn_count, idx, err, *taskobits, poll_err;
    struct timeval start_tval, curr_tval;
    tm_event_t e;
    tm_event_t *obit_events;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDU_MALLOC(taskobits, int *, HYDT_bscd_pbs_sys->spawn_count * sizeof(int), status);
    HYDU_MALLOC(obit_events, tm_event_t *, HYDT_bscd_pbs_sys->spawn_count * sizeof(tm_event_t),
                status);

    for (idx = 0; idx < HYDT_bscd_pbs_sys->spawn_count; idx++)
        obit_events[idx] = TM_NULL_EVENT;

    /* FIXME: We rely on gettimeofday here. This needs to detect the
     * timer type available and use that. Probably more of an MPL
     * functionality than Hydra's. */
    gettimeofday(&start_tval, NULL);

    spawn_count = HYDT_bscd_pbs_sys->spawn_count;
    while (spawn_count) {
        /* For each task, we need to first wait for it to be spawned,
         * and then register its termination event */
        err = tm_poll(TM_NULL_EVENT, &e, 1, &poll_err);
        HYDU_ERR_CHKANDJUMP(status, err != TM_SUCCESS, HYD_INTERNAL_ERROR,
                            "tm_poll(obit_event) failed with TM error %d\n", err);

        for (idx = 0; idx < HYDT_bscd_pbs_sys->spawn_count; idx++) {
            if (HYDT_bscd_pbs_sys->spawn_events[idx] == e) {
                /* got a spawn event (task_id[idx] is now valid);
                 * register this task for a termination event */
                err = tm_obit(HYDT_bscd_pbs_sys->task_id[idx], &taskobits[idx], &obit_events[idx]);
                HYDU_ERR_CHKANDJUMP(status, err != TM_SUCCESS, HYD_INTERNAL_ERROR,
                                    "tm_obit() failed with TM error %d\n", err);
                spawn_count--;
                break;
            }
            else if (obit_events[idx] == e) {
                /* got a task termination event */
                obit_events[idx] = TM_NULL_EVENT;
            }
        }
    }

    /* All the tasks have been spawned (and some of them have terminated) */

    /* Wait for all processes to terminate */
    for (events_count = 0, idx = 0; idx < HYDT_bscd_pbs_sys->spawn_count; idx++)
        if (obit_events[idx] != TM_NULL_EVENT)
            events_count++;
    while (events_count) {
        err = tm_poll(TM_NULL_EVENT, &e, 0, &poll_err);
        HYDU_ERR_CHKANDJUMP(status, err != TM_SUCCESS, HYD_INTERNAL_ERROR,
                            "tm_poll(obit_event) failed with TM error %d\n", err);

        if (e != TM_NULL_EVENT) {
            /* got some event; check if it is what we want */
            for (idx = 0; idx < HYDT_bscd_pbs_sys->spawn_count; idx++) {
                if (obit_events[idx] == e) {
                    events_count--;
                    break;
                }
            }
        }

        /* Check if time is up */
        if (timeout > 0) {
            gettimeofday(&curr_tval, NULL);
            time_elapsed = curr_tval.tv_sec - start_tval.tv_sec;
            if (time_elapsed > timeout) {
                status = HYD_TIMED_OUT;
                goto fn_exit;
            }
        }
    }

    if (HYDT_bsci_info.debug)
        HYDU_dump(stdout, "\nPBS_DEBUG: Done with polling obit events\n");

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
