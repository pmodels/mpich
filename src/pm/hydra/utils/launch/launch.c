/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_utils.h"

HYD_Status HYDU_create_process(char **client_arg, HYD_Env_t * env_list,
                               int *in, int *out, int *err, int *pid, int core)
{
    int inpipe[2], outpipe[2], errpipe[2], tpid;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (in && (pipe(inpipe) < 0))
        HYDU_ERR_SETANDJUMP1(status, HYD_SOCK_ERROR, "pipe error (%s)\n",
                             HYDU_strerror(errno));

    if (out && (pipe(outpipe) < 0))
        HYDU_ERR_SETANDJUMP1(status, HYD_SOCK_ERROR, "pipe error (%s)\n",
                             HYDU_strerror(errno));

    if (err && (pipe(errpipe) < 0))
        HYDU_ERR_SETANDJUMP1(status, HYD_SOCK_ERROR, "pipe error (%s)\n",
                             HYDU_strerror(errno));

    /* Fork off the process */
    tpid = fork();
    if (tpid == 0) {    /* Child process */
        close(1);
        if (out) {
            close(outpipe[0]);
            if (dup2(outpipe[1], 1) < 0)
                HYDU_ERR_SETANDJUMP1(status, HYD_SOCK_ERROR, "dup2 error (%s)\n",
                                     HYDU_strerror(errno));
        }

        close(2);
        if (err) {
            close(errpipe[0]);
            if (dup2(errpipe[1], 2) < 0)
                HYDU_ERR_SETANDJUMP1(status, HYD_SOCK_ERROR, "dup2 error (%s)\n",
                                     HYDU_strerror(errno));
        }

        close(0);
        if (in) {
            close(inpipe[1]);
            if (dup2(inpipe[0], 0) < 0)
                HYDU_ERR_SETANDJUMP1(status, HYD_SOCK_ERROR, "dup2 error (%s)\n",
                                     HYDU_strerror(errno));
        }

        if (env_list) {
            status = HYDU_putenv_list(env_list);
            HYDU_ERR_POP(status, "unable to putenv\n");
        }

        if (core >= 0) {
            status = HYDU_bind_process(core);
            HYDU_ERR_POP(status, "bind process failed\n");
        }

        if (execvp(client_arg[0], client_arg) < 0) {
            HYDU_ERR_SETANDJUMP1(status, HYD_INTERNAL_ERROR, "execvp error (%s)\n",
                                 HYDU_strerror(errno));
        }
    }
    else {      /* Parent process */
        if (out)
            close(outpipe[1]);
        if (err)
            close(errpipe[1]);
        if (in) {
            close(inpipe[0]);
            *in = inpipe[1];
        }
        if (out)
            *out = outpipe[0];
        if (err)
            *err = errpipe[0];
    }

    if (pid)
        *pid = tpid;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


HYD_Status HYDU_fork_and_exit(int core)
{
    pid_t tpid;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    /* Fork off the process */
    tpid = fork();
    if (tpid == 0) {    /* Child process */
        close(0);
        close(1);
        close(2);

        if (core >= 0) {
            status = HYDU_bind_process(core);
            HYDU_ERR_POP(status, "bind process failed\n");
        }
    }
    else {      /* Parent process */
        exit(0);
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

#if defined HAVE_THREAD_SUPPORT
HYD_Status HYDU_create_thread(void *(*func) (void *), void *args,
                              struct HYD_Thread_context *ctxt)
{
    int ret;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    ret = pthread_create(&ctxt->thread, NULL, func, args);
    if (ret < 0)
        HYDU_ERR_SETANDJUMP1(status, HYD_INTERNAL_ERROR, "pthread create failed (%s)\n",
                             HYDU_strerror(errno));

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_Status HYDU_join_thread(struct HYD_Thread_context ctxt)
{
    int ret;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    ret = pthread_join(ctxt.thread, NULL);
    if (ret < 0)
        HYDU_ERR_SETANDJUMP1(status, HYD_INTERNAL_ERROR, "pthread join failed (%s)\n",
                             HYDU_strerror(errno));

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
#endif /* HAVE_THREAD_SUPPORT */

int HYDU_local_to_global_id(int local_id, int partition_core_count,
                            struct HYD_Partition_segment *segment_list, int global_core_count)
{
    int global_id, rem;
    struct HYD_Partition_segment *segment;

    global_id = ((local_id / partition_core_count) * global_core_count);
    rem = (local_id % partition_core_count);

    for (segment = segment_list; segment; segment = segment->next) {
        if (rem >= segment->proc_count)
            rem -= segment->proc_count;
        else {
            global_id += segment->start_pid + rem;
            break;
        }
    }

    return global_id;
}
