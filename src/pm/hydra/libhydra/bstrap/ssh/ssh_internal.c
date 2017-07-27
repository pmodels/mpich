/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2010 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "ssh_internal.h"
#include "hydra_err.h"
#include "hydra_mem.h"

struct ssh_time_hash {
    char hostname[HYD_MAX_HOSTNAME_LEN];
    struct timeval *init_time;
    MPL_UT_hash_handle hh;
};

static int vars_initialized = 0;
static int ssh_limit;
static int ssh_limit_time;
static int ssh_warnings;
static struct ssh_time_hash *ssh_time_hash = NULL;

static HYD_status create_element(const char *hostname, struct ssh_time_hash **e)
{
    int i;
    HYD_status status = HYD_SUCCESS;

    HYD_MALLOC((*e), struct ssh_time_hash *, sizeof(struct ssh_time_hash), status);

    MPL_strncpy((*e)->hostname, hostname, HYD_MAX_HOSTNAME_LEN);

    HYD_MALLOC((*e)->init_time, struct timeval *, ssh_limit * sizeof(struct timeval), status);
    for (i = 0; i < ssh_limit; i++) {
        (*e)->init_time[i].tv_sec = 0;
        (*e)->init_time[i].tv_usec = 0;
    }

    MPL_HASH_ADD_STR(ssh_time_hash, hostname, (*e));

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYDI_ssh_store_launch_time(const char *hostname)
{
    int i, oldest, time_left;
    struct timeval now;
    struct ssh_time_hash *e;
    HYD_status status = HYD_SUCCESS;

    if (!vars_initialized) {
        int rc;

        rc = MPL_env2int("HYDRA_BSTRAP_SSH_LIMIT", &ssh_limit);
        if (rc == 0)
            ssh_limit = HYDI_BSTRAP_SSH_DEFAULT_LIMIT;

        rc = MPL_env2int("HYDRA_BSTRAP_SSH_LIMIT_TIME", &ssh_limit_time);
        if (rc == 0)
            ssh_limit_time = HYDI_BSTRAP_SSH_DEFAULT_LIMIT_TIME;

        rc = MPL_env2bool("HYDRA_BSTRAP_SSH_ENABLE_WARNINGS", &ssh_warnings);
        if (rc == 0)
            ssh_warnings = 1;
    }

    MPL_HASH_FIND_STR(ssh_time_hash, hostname, e);

    if (e == NULL) {    /* Couldn't find an element for this host */
        status = create_element(hostname, &e);
        HYD_ERR_POP(status, "unable to create ssh time element\n");
    }

    /* Search for an unset element to store the current time */
    for (i = 0; i < ssh_limit; i++) {
        if (e->init_time[i].tv_sec == 0 && e->init_time[i].tv_usec == 0) {
            gettimeofday(&e->init_time[i], NULL);
            goto fn_exit;
        }
    }

    /* No free element found; wait for the oldest element to turn
     * older */
    oldest = 0;
    for (i = 0; i < ssh_limit; i++)
        if (older(e->init_time[i], e->init_time[oldest]))
            oldest = i;

    gettimeofday(&now, NULL);
    time_left = ssh_limit_time - now.tv_sec + e->init_time[oldest].tv_sec;

    /* A better approach will be to make progress here, but that would
     * mean that we need to deal with nested calls to the demux engine
     * and process launches. */
    if (time_left > 0) {
        if (ssh_warnings)
            HYD_PRINT(stdout, "WARNING: too many ssh connections to %s; waiting %d seconds\n",
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

HYD_status HYDI_ssh_free_launch_elements(void)
{
    struct ssh_time_hash *e, *tmp;
    HYD_status status = HYD_SUCCESS;

    MPL_HASH_ITER(hh, ssh_time_hash, e, tmp) {
        MPL_HASH_DEL(ssh_time_hash, e);
        MPL_free(e->init_time);
        MPL_free(e);
    }

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}
