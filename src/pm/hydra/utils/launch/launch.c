/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "bind.h"

HYD_status HYDU_create_process(char **client_arg, struct HYD_env *env_list,
                               int *in, int *out, int *err, int *pid,
                               struct HYDT_bind_cpuset_t cpuset)
{
    int inpipe[2], outpipe[2], errpipe[2], tpid, i, j, k, has_space, num_args, ret;
    char *path = NULL, **args;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (in && (pipe(inpipe) < 0))
        HYDU_ERR_SETANDJUMP(status, HYD_SOCK_ERROR, "pipe error (%s)\n", HYDU_strerror(errno));

    if (out && (pipe(outpipe) < 0))
        HYDU_ERR_SETANDJUMP(status, HYD_SOCK_ERROR, "pipe error (%s)\n", HYDU_strerror(errno));

    if (err && (pipe(errpipe) < 0))
        HYDU_ERR_SETANDJUMP(status, HYD_SOCK_ERROR, "pipe error (%s)\n", HYDU_strerror(errno));

    /* Fork off the process */
    tpid = fork();
    if (tpid == 0) {    /* Child process */
        close(STDIN_FILENO);
        if (in) {
            close(inpipe[1]);
            if (dup2(inpipe[0], STDIN_FILENO) < 0)
                HYDU_ERR_SETANDJUMP(status, HYD_SOCK_ERROR, "dup2 error (%s)\n",
                                    HYDU_strerror(errno));
        }

        close(STDOUT_FILENO);
        if (out) {
            close(outpipe[0]);
            if (dup2(outpipe[1], STDOUT_FILENO) < 0)
                HYDU_ERR_SETANDJUMP(status, HYD_SOCK_ERROR, "dup2 error (%s)\n",
                                    HYDU_strerror(errno));
        }

        close(STDERR_FILENO);
        if (err) {
            close(errpipe[0]);
            if (dup2(errpipe[1], STDERR_FILENO) < 0)
                HYDU_ERR_SETANDJUMP(status, HYD_SOCK_ERROR, "dup2 error (%s)\n",
                                    HYDU_strerror(errno));
        }

        /* Forced environment overwrites existing environment */
        if (env_list) {
            status = HYDU_putenv_list(env_list, HYD_ENV_OVERWRITE_TRUE);
            HYDU_ERR_POP(status, "unable to putenv\n");
        }

        status = HYDT_bind_process(cpuset);
        HYDU_ERR_POP(status, "bind process failed\n");

        status = HYDU_strdup_list(client_arg, &args);
        HYDU_ERR_POP(status, "unable to dup argument list\n");

        num_args = HYDU_strlist_lastidx(client_arg);

        for (j = 0; j < num_args; j++) {
            has_space = 0;
            for (i = 0; args[j][i]; i++) {
                if (args[j][i] == ' ') {
                    has_space = 1;
                    break;
                }
            }

            if (has_space) {
                /* executable string has space */
                HYDU_FREE(args[j]);

                if (j == 0) {
                    path = HYDU_strdup(client_arg[j]);
                    k = 0;
                    for (i = 0; path[i]; i++)
                        if (path[i] == '/')
                            k = i + 1;
                    path[k] = 0;

                    if (path[0]) {
                        ret = chdir(path);
                        HYDU_ASSERT(!ret, status);
                    }

                    args[j] = HYDU_strdup(&client_arg[j][k]);
                }
                else {
                    HYDU_MALLOC(args[j], char *, strlen(client_arg[j]) + 3, status);
                    MPL_snprintf(args[j], strlen(client_arg[j]) + 5, "'%s'", client_arg[j]);
                }
            }
        }

        if (execvp(args[0], args) < 0) {
            /* The child process should never get back to the proxy
             * code; if there is an error, just throw it here and
             * exit. */
            HYDU_error_printf("execvp error on file %s (%s)\n", client_arg[0],
                              HYDU_strerror(errno));
            exit(-1);
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
