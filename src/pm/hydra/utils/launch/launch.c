/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "topo.h"

#ifdef HAVE_PIP

#include <pip.h>

static HYD_status HYDU_pip_envv(struct HYD_env *env_list, char ***envv_ptr)
{
    struct HYD_env *env;
    char **envv = NULL;
    int i, envc;
    HYD_status status = HYD_SUCCESS;

    for (envc = 1, env = env_list; env != NULL; env = env->next)
        envc++;

    HYDU_MALLOC_OR_JUMP(envv, char **, sizeof(char *) * envc, status);

    if (envv != NULL) {
        for (i = 0, env = env_list; env; env = env->next) {
            char *envstr;
            size_t envstr_sz = 0;

            /* FIXME: any better way to get the length of formatted string ? */
            envstr_sz = strlen(env->env_name) + strlen(env->env_value) + 4;

            HYDU_MALLOC_OR_JUMP(envstr, char *, envstr_sz, status);
            sprintf(envstr, "%s=%s", env->env_name, env->env_value);
            envv[i++] = envstr;
        }
        envv[i] = NULL;
    }

    *envv_ptr = envv;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

struct pip_task_fds {
    int inpipe[2];
    int outpipe[2];
    int errpipe[2];
    int idx;
    char **envv;
};

static int HYDU_pip_before(struct pip_task_fds *fds)
{
    close(STDIN_FILENO);
    if (fds->inpipe[0] >= 0) {
        close(fds->inpipe[1]);
        if (fds->inpipe[0] != STDIN_FILENO) {
            while (1) {
                if (dup2(fds->inpipe[0], STDIN_FILENO) >= 0)
                    break;
                if (errno != EBUSY)
                    return errno;
            }
        }
    }
    close(STDOUT_FILENO);
    if (fds->outpipe[0] >= 0) {
        close(fds->outpipe[0]);
        if (fds->outpipe[1] != STDOUT_FILENO) {
            while (1) {
                if (dup2(fds->outpipe[1], STDOUT_FILENO) >= 0)
                    break;
                if (errno != EBUSY)
                    return errno;
            }
        }
    }
    close(STDERR_FILENO);
    if (fds->errpipe[0] >= 0) {
        close(fds->errpipe[0]);
        if (fds->errpipe[1] != STDERR_FILENO) {
            while (1) {
                if (dup2(fds->errpipe[1], STDERR_FILENO) >= 0)
                    break;
                if (errno != EBUSY)
                    return errno;
            }
        }
    }
    if (fds->idx >= 0) {
        if (HYDT_topo_bind(fds->idx) != HYD_SUCCESS) {
            fprintf(stderr, "bind process failed\n");
        }
    }
    return 0;
}

static int HYDU_pip_after(struct pip_task_fds *fds)
{
    int i;

    for (i = 0; fds->envv[i] != NULL; i++)
        MPL_free(fds->envv[i]);
    MPL_free(fds->envv);
    MPL_free(fds);

    return 0;
}

HYD_status HYDU_spawn_pip_tasks(char **client_arg, struct HYD_env * env_list,
                                int *in, int *out, int *err, int *pid, int idx)
{
    static pipid = 0;
    struct pip_task_fds *fds;
    int tpid;
    int pip_err;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDU_MALLOC_OR_JUMP(fds, struct pip_task_fds *, sizeof(struct pip_task_fds), status);
    fds->inpipe[0] = -1;
    fds->inpipe[1] = -1;
    fds->outpipe[0] = -1;
    fds->outpipe[1] = -1;
    fds->errpipe[0] = -1;
    fds->errpipe[1] = -1;
    fds->idx = idx;
    HYDU_pip_envv(env_list, &fds->envv);


    if (in && (pipe(fds->inpipe) < 0))
        HYDU_ERR_SETANDJUMP(status, HYD_SOCK_ERROR, "pipe error (%s)\n", MPL_strerror(errno));

    if (out && (pipe(fds->outpipe) < 0))
        HYDU_ERR_SETANDJUMP(status, HYD_SOCK_ERROR, "pipe error (%s)\n", MPL_strerror(errno));

    if (err && (pipe(fds->errpipe) < 0))
        HYDU_ERR_SETANDJUMP(status, HYD_SOCK_ERROR, "pipe error (%s)\n", MPL_strerror(errno));

    if ((pip_err = pip_spawn(client_arg[0],
                             client_arg,
                             fds->envv,
                             PIP_CPUCORE_ASIS,
                             &pipid,
                             (pip_spawnhook_t) HYDU_pip_before,
                             (pip_spawnhook_t) HYDU_pip_after, (void *) fds)) != 0) {
        HYDU_ERR_SETANDJUMP(status, HYD_FAILURE, "pip_spawn(): %s\n", MPL_strerror(pip_err));
    }
    pip_get_pid(pipid, &tpid);
    pipid++;

    if (out)
        close(fds->outpipe[1]);
    if (err)
        close(fds->errpipe[1]);
    if (in) {
        close(fds->inpipe[0]);
        *in = fds->inpipe[1];

        status = HYDU_sock_cloexec(*in);
        HYDU_ERR_POP(status, "unable to set close on exec\n");
    }
    if (out)
        *out = fds->outpipe[0];
    if (err)
        *err = fds->errpipe[0];
    if (pid)
        *pid = tpid;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
#endif

HYD_status HYDU_create_process(char **client_arg, struct HYD_env *env_list,
                               int *in, int *out, int *err, int *pid, int idx)
{
    int inpipe[2], outpipe[2], errpipe[2], tpid;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (in && (pipe(inpipe) < 0))
        HYDU_ERR_SETANDJUMP(status, HYD_SOCK_ERROR, "pipe error (%s)\n", MPL_strerror(errno));

    if (out && (pipe(outpipe) < 0))
        HYDU_ERR_SETANDJUMP(status, HYD_SOCK_ERROR, "pipe error (%s)\n", MPL_strerror(errno));

    if (err && (pipe(errpipe) < 0))
        HYDU_ERR_SETANDJUMP(status, HYD_SOCK_ERROR, "pipe error (%s)\n", MPL_strerror(errno));

    /* Fork off the process */
    tpid = fork();
    if (tpid == 0) {    /* Child process */

#if defined HAVE_SETSID
        setsid();
#endif /* HAVE_SETSID */

        close(STDIN_FILENO);
        if (in) {
            close(inpipe[1]);
            if (inpipe[0] != STDIN_FILENO && dup2(inpipe[0], STDIN_FILENO) < 0)
                HYDU_ERR_SETANDJUMP(status, HYD_SOCK_ERROR, "dup2 error (%s)\n",
                                    MPL_strerror(errno));
        }

        close(STDOUT_FILENO);
        if (out) {
            close(outpipe[0]);
            if (outpipe[1] != STDOUT_FILENO && dup2(outpipe[1], STDOUT_FILENO) < 0)
                HYDU_ERR_SETANDJUMP(status, HYD_SOCK_ERROR, "dup2 error (%s)\n",
                                    MPL_strerror(errno));
        }

        close(STDERR_FILENO);
        if (err) {
            close(errpipe[0]);
            if (errpipe[1] != STDERR_FILENO && dup2(errpipe[1], STDERR_FILENO) < 0)
                HYDU_ERR_SETANDJUMP(status, HYD_SOCK_ERROR, "dup2 error (%s)\n",
                                    MPL_strerror(errno));
        }

        /* Forced environment overwrites existing environment */
        if (env_list) {
            status = HYDU_putenv_list(env_list, HYD_ENV_OVERWRITE_TRUE);
            HYDU_ERR_POP(status, "unable to putenv\n");
        }

        if (idx >= 0) {
            status = HYDT_topo_bind(idx);
            HYDU_ERR_POP(status, "bind process failed\n");
        }

        if (execvp(client_arg[0], client_arg) < 0) {
            /* The child process should never get back to the proxy
             * code; if there is an error, just throw it here and
             * exit. */
            HYDU_error_printf("execvp error on file %s (%s)\n", client_arg[0], MPL_strerror(errno));
            exit(-1);
        }
    } else {    /* Parent process */
        if (out)
            close(outpipe[1]);
        if (err)
            close(errpipe[1]);
        if (in) {
            close(inpipe[0]);
            *in = inpipe[1];

            status = HYDU_sock_cloexec(*in);
            HYDU_ERR_POP(status, "unable to set close on exec\n");
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
