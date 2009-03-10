/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_utils.h"

/* FIXME: Here we assume that the timer is gettimeofday. */

void HYDU_Time_set(HYD_Time * time, int *val)
{
    if (val == NULL) {
        /* Set time to right now */
        gettimeofday(time, NULL);
    }
    else {
        time->tv_sec = *val;
        time->tv_usec = 0;
    }
}

int HYDU_Time_left(HYD_Time start, HYD_Time timeout)
{
    HYD_Time now;
    int time_left;

    HYDU_FUNC_ENTER();

    if (timeout.tv_sec < 0) {
        time_left = -1;
        goto fn_exit;
    }
    else {
        gettimeofday(&now, NULL);
        if (timeout.tv_sec > (now.tv_sec - start.tv_sec)) {
            time_left = (1000 * (timeout.tv_sec - now.tv_sec + start.tv_sec));
        }
        else
            time_left = 0;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return time_left;
}
