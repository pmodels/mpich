/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_utils.h"
#include "ckpoint.h"
#include "ckpoint_blcr.h"
#include <stdio.h>
#include <libcr.h>
#include <strings.h>
#include <errno.h>

static int my_callback(void* arg)
{
    int rc;

    rc = cr_checkpoint(CR_CHECKPOINT_OMIT);

    switch(rc) {
    case -CR_ETEMPFAIL :
        /* One of the processes indicated that it couldn't take the checkpoint now.  Try again later. */
        return -1;
        break;
    case -CR_EPERMFAIL :
        /* One of the processes indicated a permanent failure */
        return -1;
        break;
    case -CR_EOMITTED :
        /* This is the expected return */
        break;
    default:
        /* Something bad happened */
        return -1;
    }
    
        
    return 0;
}

static HYD_Status create_env_file(int num_vars, const char **env_vars)
{
    HYD_Status status = HYD_SUCCESS;
    char filename[256];
    FILE *f;
    int i;
    int ret;
    
    HYDU_FUNC_ENTER();

    snprintf(filename, sizeof(filename), "/tmp/mpich-env.%d", (int)getpid());

    f = fopen(filename, "w+");
    HYDU_ERR_CHKANDJUMP1(status, f, HYD_INTERNAL_ERROR, "fopen failed: %s", strerror(errno));

    for (i = 0; i < num_vars; ++i) {
        fprintf(f, "%s\n", env_vars[i]);
    }
    
    ret = fclose(f);
    HYDU_ERR_CHKANDJUMP1(status, ret, HYD_INTERNAL_ERROR, "fclose failed: %s", strerror(errno));

fn_exit:
    HYDU_FUNC_EXIT();
    return status;
    
fn_fail:
    goto fn_exit;
}

HYD_Status HYDU_ckpoint_blcr_suspend(const char *prefix)
{
    HYD_Status status = HYD_SUCCESS;
    int ret;
    int fd;
    cr_checkpoint_args_t my_args;
    cr_checkpoint_handle_t my_handle;
    char filename[256];

    HYDU_FUNC_ENTER();

    /* build the checkpoint filename */
    snprintf(filename, sizeof(filename), "%s/context", prefix);

    /* remove existing checkpoint file, if any */
    ret = unlink(filename);
    HYDU_ERR_CHKANDJUMP1(status, ret, HYD_INTERNAL_ERROR, "unlink failed: %s", strerror(errno));
    
    /* open the checkpoint file */
    fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC /* | O_LARGEFILE */, 0600);
    HYDU_ERR_CHKANDJUMP1(status, fd < 0, HYD_INTERNAL_ERROR, "open failed: %s", strerror(errno));

    cr_initialize_checkpoint_args_t(&my_args);
    my_args.cr_fd = fd;
    my_args.cr_scope = CR_SCOPE_TREE;

    /* issue the request */
    ret = cr_request_checkpoint(&my_args, &my_handle);
    HYDU_ERR_CHKANDJUMP1(status, ret < 0, HYD_INTERNAL_ERROR, "cr_request_checkpoint failed, %s", strerror(errno));

    /* wait for the request to complete */
    while (0) {
        ret = cr_poll_checkpoint(&my_handle, NULL);
        if (ret < 0) {
            if ((ret == CR_POLL_CHKPT_ERR_POST) && (errno == CR_ERESTARTED)) {
                HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "trying to restart in a checkpoint");
            } else if (errno == EINTR) {
                /* poll was interrupted by a signal -- retry */
            } else {
                HYDU_ERR_SETANDJUMP1(status, HYD_INTERNAL_ERROR, "cr_poll_checkpoint failed: %s", strerror(errno));
            }
        } else if (ret == 0) {
            HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "cr_poll_checkpoint returned 0 unexpectedly");
        } else {
            break;
	}
    }

    ret = close(my_args.cr_fd);
    HYDU_ERR_CHKANDJUMP1(status, ret, HYD_INTERNAL_ERROR, "close failed, %s", strerror(errno));

fn_exit:
    HYDU_FUNC_EXIT();
    return status;

fn_fail:
    goto fn_exit;
}

HYD_Status HYDU_ckpoint_blcr_restart(const char *prefix, int num_vars, const char **env_vars)
{
    HYD_Status status = HYD_SUCCESS;
    pid_t mypid;
    int ret;
    int context_fd;
    cr_restart_handle_t cr_handle;
    cr_restart_args_t args;
    char filename[256];

    HYDU_FUNC_ENTER();
    mypid = getpid();

    status = create_env_file(num_vars, env_vars);
    if (status) HYDU_ERR_POP(status, "blcr restart");

    snprintf(filename, sizeof(filename), "%s/context", prefix);
    
    context_fd = open(filename, O_RDONLY /* | O_LARGEFILE */);
    HYDU_ERR_CHKANDJUMP1(status, context_fd < 0, HYD_INTERNAL_ERROR, "open failed, %s", strerror(errno));
    
    /* ... initialize the request structure */
    cr_initialize_restart_args_t(&args);
    args.cr_fd       = context_fd;
    args.cr_flags    = CR_RSTRT_RESTORE_PID;

    /* ... issue the request */
    ret = cr_request_restart(&args, &cr_handle);
    HYDU_ERR_CHKANDJUMP1(status, ret, HYD_INTERNAL_ERROR, "cr_request_restart failed, %s", strerror(errno));

    ret = close(context_fd);
    HYDU_ERR_CHKANDJUMP1(status, ret, HYD_INTERNAL_ERROR, "close failed, %s", strerror(errno));

fn_exit:
    HYDU_FUNC_EXIT();
    return status;

fn_fail:
    goto fn_exit;
}

HYD_Status HYDU_ckpoint_blcr_init(void)
{
    HYD_Status status = HYD_SUCCESS;
    int rc;
    cr_client_id_t client_id;
    cr_callback_id_t callback_id;
    
    HYDU_FUNC_ENTER();
    
    client_id = (int)cr_init();
    if (client_id < 0) goto fn_fail;

    callback_id = cr_register_callback(my_callback, &rc, CR_SIGNAL_CONTEXT);
    if (callback_id < 0) goto fn_fail;

fn_exit:
    HYDU_FUNC_EXIT();
    return status;
    
fn_fail:
    goto fn_exit;
}

