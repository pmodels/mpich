/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2017 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_fs.h"
#include "hydra_err.h"
#include "hydra_mem.h"
#include "hydra_str.h"

static int exists(char *filename)
{
    struct stat file_stat;

    if ((stat(filename, &file_stat) < 0) || !(S_ISREG(file_stat.st_mode))) {
        return 0;       /* no such file, or not a regular file */
    }

    return 1;
}

static HYD_status get_abs_wd(const char *wd, char **abs_wd)
{
    int ret;
    char *cwd;
    HYD_status status = HYD_SUCCESS;

    if (wd == NULL) {
        *abs_wd = NULL;
        goto fn_exit;
    }

    if (wd[0] != '.') {
        *abs_wd = (char *) wd;
        goto fn_exit;
    }

    cwd = HYD_getcwd();
    ret = chdir(wd);
    if (ret < 0)
        HYD_ERR_POP(status, "error calling chdir\n");

    *abs_wd = HYD_getcwd();
    ret = chdir(cwd);
    if (ret < 0)
        HYD_ERR_POP(status, "error calling chdir\n");

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYD_find_in_path(const char *execname, char **path)
{
    char *tmp[HYD_NUM_TMP_STRINGS], *path_loc = NULL, *test_loc, *user_path;
    HYD_status status = HYD_SUCCESS;

    HYD_FUNC_ENTER();

    /* The executable is somewhere in the user's path. Find it. */
    if (MPL_env2str("PATH", (const char **) &user_path))
        user_path = MPL_strdup(user_path);

    if (user_path) {    /* If the PATH environment exists */
        status = get_abs_wd(strtok(user_path, ";:"), &test_loc);
        HYD_ERR_POP(status, "error getting absolute working dir\n");
        do {
            tmp[0] = MPL_strdup(test_loc);
            tmp[1] = MPL_strdup("/");
            tmp[2] = MPL_strdup(execname);
            tmp[3] = NULL;

            status = HYD_str_alloc_and_join(tmp, &path_loc);
            HYD_ERR_POP(status, "unable to join strings\n");
            HYD_str_free_list(tmp);

            if (exists(path_loc)) {
                tmp[0] = MPL_strdup(test_loc);
                tmp[1] = MPL_strdup("/");
                tmp[2] = NULL;

                status = HYD_str_alloc_and_join(tmp, path);
                HYD_ERR_POP(status, "unable to join strings\n");
                HYD_str_free_list(tmp);

                goto fn_exit;   /* We are done */
            }

            MPL_free(path_loc);
            path_loc = NULL;

            status = get_abs_wd(strtok(NULL, ";:"), &test_loc);
            HYD_ERR_POP(status, "error getting absolute working dir\n");
        } while (test_loc);
    }

    /* There is either no PATH environment or we could not find the
     * file in the PATH. Just return an empty path */
    *path = MPL_strdup("");

  fn_exit:
    if (user_path)
        MPL_free(user_path);
    if (path_loc)
        MPL_free(path_loc);
    HYD_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

char *HYD_getcwd(void)
{
    char *cwdval, *retval = NULL;
    const char *pwdval;
    HYD_status status = HYD_SUCCESS;
#if defined HAVE_STAT
    struct stat spwd, scwd;
#endif /* HAVE_STAT */

    if (MPL_env2str("PWD", &pwdval) == 0)
        pwdval = NULL;
    HYD_MALLOC(cwdval, char *, HYDRA_MAX_PATH, status);
    if (getcwd(cwdval, HYDRA_MAX_PATH) == NULL)
        HYD_ERR_SETANDJUMP(status, HYD_ERR_INTERNAL,
                           "allocated space is too small for absolute path\n");

#if defined HAVE_STAT
    if (pwdval && stat(pwdval, &spwd) != -1 && stat(cwdval, &scwd) != -1 &&
        spwd.st_dev == scwd.st_dev && spwd.st_ino == scwd.st_ino) {
        /* PWD and getcwd() match; use the PWD value */
        retval = MPL_strdup(pwdval);
        MPL_free(cwdval);
    }
    else
#endif /* HAVE_STAT */
    {
        /* PWD and getcwd() don't match; use the getcwd value and hope
         * for the best. */
        retval = cwdval;
    }

  fn_exit:
    return retval;

  fn_fail:
    goto fn_exit;
}

char *HYD_find_full_path(const char *execname)
{
    char *tmp[HYD_NUM_TMP_STRINGS] = { NULL }, *path = NULL, *test_path = NULL;
    HYD_status status = HYD_SUCCESS;

    HYD_FUNC_ENTER();

    status = HYD_find_in_path(execname, &test_path);
    HYD_ERR_POP(status, "error while searching for executable in user path\n");

    if (test_path) {
        tmp[0] = MPL_strdup(test_path);
        tmp[1] = MPL_strdup(execname);
        tmp[2] = NULL;

        status = HYD_str_alloc_and_join(tmp, &path);
        HYD_ERR_POP(status, "error joining strings\n");
    }

  fn_exit:
    HYD_str_free_list(tmp);
    if (test_path)
        MPL_free(test_path);
    HYD_FUNC_EXIT();
    return path;

  fn_fail:
    goto fn_exit;
}
