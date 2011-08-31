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
double TS_Wtime( void );
#endif

/* integer comparison function for tm_event_t[] */
static int cmp_pbsevent( const void *p1, const void *p2 )
{
    return (*(tm_event_t*)p1) - (*(tm_event_t*)p2);
}

HYD_status HYDT_bscd_pbs_wait_for_completion(int timeout)
{
    tm_event_t     *pollevents = NULL;
    int             time_elapsed;
    int             events_count, spawned_count;
    int             mismatch, idx, ierr;
    struct timeval  start_tval, curr_tval;
    HYD_status      status = HYD_SUCCESS;

#if defined(TS_PROFILE)
    double stime, etime;
#endif

    HYDU_FUNC_ENTER();

    /* Allocate memory for taskobits[] and pollevents[]. */
    HYDU_MALLOC(HYDT_bscd_pbs_sys->taskobits, int *,
                HYDT_bscd_pbs_sys->size * sizeof(int), status);
    spawned_count = HYDT_bscd_pbs_sys->spawned_count;
    HYDU_MALLOC(pollevents, tm_event_t *,
                spawned_count * sizeof(tm_event_t), status);

    /*
     * FIXME: We rely on gettimeofday here. This needs to detect the
     * timer type available and use that. Probably more of an MPL
     * functionality than Hydra's.
     */
    gettimeofday(&start_tval, NULL);

    /* Poll the TM for the spawning eventID returned by tm_spawn() to
     * determine if the spawned process has started. */
#if defined(TS_PROFILE)
    stime = TS_Wtime();
#endif
    events_count = 0;
    while (events_count < spawned_count) {
        int poll_err;
        ierr = tm_poll(TM_NULL_EVENT, &pollevents[events_count], 0, &poll_err);
        if (ierr != TM_SUCCESS)
            HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR,
                                "tm_poll(spawn_event) fails with TM err %d.\n",
                                ierr);
        if (pollevents[events_count] != TM_NULL_EVENT) {
            events_count++;
        }
    }

    /* Sort HYDT_bscd_pbs_sys->events[] and pollevents[] */
    qsort(HYDT_bscd_pbs_sys->events, spawned_count,
          sizeof(tm_event_t), cmp_pbsevent);
    qsort(pollevents, spawned_count, sizeof(tm_event_t), cmp_pbsevent);
    mismatch = 0;
    for ( idx = 0; idx < spawned_count; idx++ ) {
        if ( pollevents[idx] != HYDT_bscd_pbs_sys->events[idx] ) {
             mismatch = 1;
             break;
        }
    }
    if ( mismatch ) {
        /* Pavan: what is the correct HYDU macro to do this kind of fprintf ? */
        fprintf(stderr, "tm_spawn()'s events[] = ");
        for ( idx = 0; idx < spawned_count; idx++ )
             fprintf(stderr, "%d ", HYDT_bscd_pbs_sys->events[idx]);
        fprintf(stderr, "\n");
        fprintf(stderr, "pollevents[] = ");
        for ( idx = 0; idx < spawned_count; idx++ )
             fprintf(stderr, "%d ", pollevents[idx]);
        fprintf(stderr, "\n");
        fflush(stderr);
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR,
                            "Inconsistent pollevents[] & tm_spawn()'s events[]:\n");
    }
#if defined(TS_PROFILE)
    etime = TS_Wtime();
    HYDU_dump(stdout, "tm_poll(spawn_events) loop takes %f\n", etime-stime );
#endif

#if defined(TS_PROFILE)
    stime = TS_Wtime();
#endif
    /* Register with TM to be notified the obituary of the spawning process. */
    for (idx = 0; idx < spawned_count; idx++) {
        /*
         * Get a TM event which will be returned by tm_poll() when
         * the process labelled by taskID dies
         */
        ierr = tm_obit(HYDT_bscd_pbs_sys->taskIDs[idx],
                       HYDT_bscd_pbs_sys->taskobits + idx,
                       HYDT_bscd_pbs_sys->events + idx);
        if (ierr != TM_SUCCESS)
            HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR,
                                "tm_obit() fails with TM err=%d.\n", ierr);
        if (HYDT_bscd_pbs_sys->events[idx] == TM_ERROR_EVENT)
            HYDU_error_printf("tm_obit(Task %d) returns error.\n",
                              HYDT_bscd_pbs_sys->taskIDs[idx]);
        if (HYDT_bscd_pbs_sys->events[idx] == TM_NULL_EVENT)
            HYDU_error_printf("Task %d already exits with status %d\n",
                              HYDT_bscd_pbs_sys->taskIDs[idx],
                              HYDT_bscd_pbs_sys->taskobits[idx]);
    }
#if defined(TS_PROFILE)
    etime = TS_Wtime();
    HYDU_dump(stdout, "tm_obit() loop takes %f\n", etime-stime );
#endif

#if defined(TS_PROFILE)
    stime = TS_Wtime();
#endif
    /* Sort HYDT_bscd_pbs_sys->events[] */
    qsort(HYDT_bscd_pbs_sys->events, spawned_count,
          sizeof(tm_event_t), cmp_pbsevent);
    /* Poll if the spawned process has exited */
    events_count = 0;
    /* Substract all the processes that have already exited */
    for (idx = 0; idx < spawned_count; idx++) {
        if (HYDT_bscd_pbs_sys->events[idx] == TM_NULL_EVENT) {
            pollevents[events_count] = TM_NULL_EVENT;
            events_count++;
        }
    }
    /* Polling for the remaining alive processes till they all exit */
    while (events_count < spawned_count) {
        int poll_err;
        ierr = tm_poll(TM_NULL_EVENT, &pollevents[events_count], 0, &poll_err);
        if (ierr != TM_SUCCESS)
            HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR,
                                "tm_poll(obit_event) fails with err=%d.\n",
                                ierr);
        if (pollevents[events_count] != TM_NULL_EVENT) {
            events_count++;
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
    }  /* while (events_count < spawned_count) */

    /* Sort pollevents[] in sync with HYDT_bscd_pbs_sys->events[] */
    qsort(pollevents, spawned_count, sizeof(tm_event_t), cmp_pbsevent);
    mismatch = 0;
    for ( idx = 0; idx < spawned_count; idx++ ) {
        if ( pollevents[idx] != HYDT_bscd_pbs_sys->events[idx] ) {
             mismatch = 1;
             break;
        }
    }
    if ( mismatch ) {
        /* Pavan: what is the correct HYDU macro to do this kind of fprintf ? */
        fprintf(stderr, "tm_obit()'s events[] = ");
        for ( idx = 0; idx < spawned_count; idx++ )
            fprintf(stderr, "%d ", HYDT_bscd_pbs_sys->events[idx]);
        fprintf(stderr, "\n");
        fprintf(stderr, "pollevents[] = ");
        for ( idx = 0; idx < spawned_count; idx++ )
            fprintf(stderr, "%d ", pollevents[idx]);
        fprintf(stderr, "\n");
        fflush(stderr);
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR,
                            "Inconsistent pollevents[] & tm_obit()'s events[]:\n");
    }
#if defined(TS_PROFILE)
    etime = TS_Wtime();
    HYDU_dump(stdout, "tm_poll(obit_events) loop takes %f\n", etime-stime );
#endif

    if (HYDT_bsci_info.debug) {
        HYDU_dump(stdout, "\nPBS_DEBUG: Done with polling obit events!\n");
    }

    if (pollevents)
        HYDU_FREE(pollevents);

    /* Loop till all sockets have closed */
  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
