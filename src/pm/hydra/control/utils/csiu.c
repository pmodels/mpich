/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"

HYD_Handle handle;

int HYD_CSU_Time_left(void)
{
    struct timeval now;
    int time_left;

    HYDU_FUNC_ENTER();

    if (handle.timeout.tv_sec < 0) {
        time_left = -1;
        goto fn_exit;
    }
    else {
        gettimeofday(&now, NULL);
        time_left =
            (1000 *
             (handle.timeout.tv_sec - now.tv_sec + handle.start.tv_sec));
        if (time_left < 0)
            time_left = 0;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return time_left;
}
