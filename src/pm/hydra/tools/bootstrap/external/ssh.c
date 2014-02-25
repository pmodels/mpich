/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2010 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "bsci.h"
#include "bscu.h"
#include "ssh.h"

struct HYDT_bscd_ssh_time *HYDT_bscd_ssh_time = NULL;

static HYD_status create_element(char *hostname, struct HYDT_bscd_ssh_time **e)
{
    int i;
    struct HYDT_bscd_ssh_time *tmp;
    HYD_status status = HYD_SUCCESS;

    /* FIXME: These are never getting freed */
    HYDU_MALLOC((*e), struct HYDT_bscd_ssh_time *, sizeof(struct HYDT_bscd_ssh_time), status);
    HYDU_MALLOC((*e)->init_time, struct timeval *,
                HYDT_bscd_ssh_limit_time * sizeof(struct timeval), status);

    (*e)->hostname = HYDU_strdup(hostname);
    for (i = 0; i < HYDT_bscd_ssh_limit; i++) {
        (*e)->init_time[i].tv_sec = 0;
        (*e)->init_time[i].tv_usec = 0;
    }
    (*e)->next = NULL;

    if (HYDT_bscd_ssh_time == NULL)
        HYDT_bscd_ssh_time = (*e);
    else {
        for (tmp = HYDT_bscd_ssh_time; tmp->next; tmp = tmp->next);
        tmp->next = (*e);
    }

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYDTI_bscd_ssh_store_launch_time(char *hostname)
{
    int i, oldest, time_left;
    struct timeval now;
    struct HYDT_bscd_ssh_time *e;
    HYD_status status = HYD_SUCCESS;

    for (e = HYDT_bscd_ssh_time; e; e = e->next)
        if (!strcmp(hostname, e->hostname))
            break;

    if (e == NULL) {    /* Couldn't find an element for this host */
        status = create_element(hostname, &e);
        HYDU_ERR_POP(status, "unable to create ssh time element\n");
    }

    /* Search for an unset element to store the current time */
    for (i = 0; i < HYDT_bscd_ssh_limit; i++) {
        if (e->init_time[i].tv_sec == 0 && e->init_time[i].tv_usec == 0) {
            gettimeofday(&e->init_time[i], NULL);
            goto fn_exit;
        }
    }

    /* No free element found; wait for the oldest element to turn
     * older */
    oldest = 0;
    for (i = 0; i < HYDT_bscd_ssh_limit; i++)
        if (older(e->init_time[i], e->init_time[oldest]))
            oldest = i;

    gettimeofday(&now, NULL);
    time_left = HYDT_bscd_ssh_limit_time - now.tv_sec + e->init_time[oldest].tv_sec;

    /* A better approach will be to make progress here, but that would
     * mean that we need to deal with nested calls to the demux engine
     * and process launches. */
    if (time_left > 0) {
        if (HYDT_bscd_ssh_warnings)
            HYDU_dump(stdout, "WARNING: too many ssh connections to %s; waiting %d seconds\n",
                      hostname, time_left);
        sleep(time_left);
    }

    /* Store the current time in the oldest element */
    gettimeofday(&e->init_time[oldest], NULL);

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}
