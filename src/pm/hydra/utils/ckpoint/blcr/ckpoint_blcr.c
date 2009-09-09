/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_utils.h"
#include "ckpoint.h"
#include "ckpoint_blcr.h"
#include <libcr.h>

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

static HYD_Status create_fifo(const char *fname_template, int rank, int *fd)
{
    HYD_Status status = HYD_SUCCESS;
    char filename[256];
    int ret;

    snprintf(filename, sizeof(filename), fname_template, (int)getpid(), rank);
    
    ret = mkfifo(filename, 0600);
    HYDU_ERR_CHKANDJUMP1(status, ret, HYD_INTERNAL_ERROR, "mkfifo failed: %s\n", strerror(errno));
        
    *fd = open(filename, O_RDWR);
    HYDU_ERR_CHKANDJUMP1(status, *fd < 0, HYD_INTERNAL_ERROR, "open failed: %s\n", strerror(errno));

fn_exit:
    HYDU_FUNC_EXIT();
    return status;
    
fn_fail:
    goto fn_exit;
}


static HYD_Status create_stdinouterr_fifos(int num_ranks, int ranks[], int *in, int *out, int *err)
{
    HYD_Status status = HYD_SUCCESS;
    int r;

    for (r = 0; r < num_ranks; ++r) {
        if (in && ranks[r] == 0) {
            status = create_fifo("/tmp/hydra-in-%d:%d", ranks[r], in);
            if (status) HYDU_ERR_POP(status, "create in fifo\n");
        }
        if (out) {
            status = create_fifo("/tmp/hydra-out-%d:%d", ranks[r], &out[r]);
            if (status) HYDU_ERR_POP(status, "create out fifo\n");
        }
        if (err) {
            status = create_fifo("/tmp/hydra-err-%d:%d", ranks[r], &err[r]);
            if (status) HYDU_ERR_POP(status, "create err fifo\n");
        }
    }
    
fn_exit:
    HYDU_FUNC_EXIT();
    return status;
    
fn_fail:
    goto fn_exit;
}


static HYD_Status create_env_file(const HYD_Env_t *envlist, int num_ranks, int *ranks)
{
    HYD_Status status = HYD_SUCCESS;
    char filename[256];
    FILE *f;
    const HYD_Env_t *e;
    int ret;
    int r;
    
    HYDU_FUNC_ENTER();

    for (r = 0; r < num_ranks; ++r) {
        snprintf(filename, sizeof(filename), "/tmp/hydra-env-file-%d:%d", (int)getpid(), ranks[r]);
        
        f = fopen(filename, "w");
        HYDU_ERR_CHKANDJUMP1(status, f == NULL, HYD_INTERNAL_ERROR, "fopen failed: %s\n", strerror(errno));
        
        for (e = envlist; e; e = e->next) {
            fprintf(f, "%s=%s\n", e->env_name, e->env_value);
        }
        
        ret = fclose(f);
        HYDU_ERR_CHKANDJUMP1(status, ret, HYD_INTERNAL_ERROR, "fclose failed: %s\n", strerror(errno));
    }
    
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
    (void)unlink(filename);
    
    /* open the checkpoint file */
    fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC /* | O_LARGEFILE */, 0600);
    HYDU_ERR_CHKANDJUMP1(status, fd < 0, HYD_INTERNAL_ERROR, "open failed: %s\n", strerror(errno));

    cr_initialize_checkpoint_args_t(&my_args);
    my_args.cr_fd = fd;
    my_args.cr_scope = CR_SCOPE_TREE;

    /* issue the request */
    ret = cr_request_checkpoint(&my_args, &my_handle);
    if (ret < 0) {
        HYDU_ERR_CHKANDJUMP1(status, errno == CR_ENOSUPPORT, HYD_INTERNAL_ERROR, "cr_request_checkpoint failed, %s\n", strerror(errno));
        HYDU_ERR_SETANDJUMP1(status, HYD_INTERNAL_ERROR, "cr_request_checkpoint failed, %s\n", strerror(errno));
    }
    /* wait for the request to complete */
    while (0) {
        ret = cr_poll_checkpoint(&my_handle, NULL);
        if (ret < 0) {
            if ((ret == CR_POLL_CHKPT_ERR_POST) && (errno == CR_ERESTARTED)) {
                HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "trying to restart in a checkpoint\n");
            } else if (errno == EINTR) {
                /* poll was interrupted by a signal -- retry */
            } else {
                HYDU_ERR_SETANDJUMP1(status, HYD_INTERNAL_ERROR, "cr_poll_checkpoint failed: %s\n", strerror(errno));
            }
        } else if (ret == 0) {
            HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "cr_poll_checkpoint returned 0 unexpectedly\n");
        } else {
            break;
	}
    }

    ret = close(my_args.cr_fd);
    HYDU_ERR_CHKANDJUMP1(status, ret, HYD_INTERNAL_ERROR, "close failed, %s\n", strerror(errno));

fn_exit:
    HYDU_FUNC_EXIT();
    return status;

fn_fail:
    goto fn_exit;
}

HYD_Status HYDU_ckpoint_blcr_restart(const char *prefix, HYD_Env_t *envlist, int num_ranks, int ranks[], int *in, int *out, int *err)
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

    status = create_env_file(envlist, num_ranks, ranks);
    if (status) HYDU_ERR_POP(status, "blcr restart\n");

    status = create_stdinouterr_fifos(num_ranks, ranks, in, out, err);
    if (status) HYDU_ERR_POP(status, "blcr restart\n");

    snprintf(filename, sizeof(filename), "%s/context", prefix);
    
    context_fd = open(filename, O_RDONLY /* | O_LARGEFILE */);
    HYDU_ERR_CHKANDJUMP1(status, context_fd < 0, HYD_INTERNAL_ERROR, "open failed, %s\n", strerror(errno));
    
    /* ... initialize the request structure */
    cr_initialize_restart_args_t(&args);
    args.cr_fd       = context_fd;
    args.cr_flags    = CR_RSTRT_RESTORE_PID;

    /* ... issue the request */
    ret = cr_request_restart(&args, &cr_handle);
    HYDU_ERR_CHKANDJUMP1(status, ret, HYD_INTERNAL_ERROR, "cr_request_restart failed, %s\n", strerror(errno));

    ret = close(context_fd);
    HYDU_ERR_CHKANDJUMP1(status, ret, HYD_INTERNAL_ERROR, "close failed, %s\n", strerror(errno));

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

