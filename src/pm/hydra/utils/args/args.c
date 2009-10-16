/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_utils.h"

static int exists(char *filename)
{
    struct stat file_stat;

    if ((stat(filename, &file_stat) < 0) || !(S_ISREG(file_stat.st_mode))) {
        return 0;       /* no such file, or not a regular file */
    }

    return 1;
}

HYD_status HYDU_find_in_path(const char *execname, char **path)
{
    char *user_path = NULL, *tmp[HYD_NUM_TMP_STRINGS], *path_loc = NULL, *test_loc;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    /* The executable is somewhere in the user's path. We need to find
     * it. */
    if (getenv("PATH")) {       /* If the PATH environment exists */
        user_path = HYDU_strdup(getenv("PATH"));
        test_loc = strtok(user_path, ";:");
        do {
            tmp[0] = HYDU_strdup(test_loc);
            tmp[1] = HYDU_strdup("/");
            tmp[2] = HYDU_strdup(execname);
            tmp[3] = NULL;

            status = HYDU_str_alloc_and_join(tmp, &path_loc);
            HYDU_ERR_POP(status, "unable to join strings\n");
            HYDU_free_strlist(tmp);

            if (exists(path_loc)) {
                tmp[0] = HYDU_strdup(test_loc);
                tmp[1] = HYDU_strdup("/");
                tmp[2] = NULL;

                status = HYDU_str_alloc_and_join(tmp, path);
                HYDU_ERR_POP(status, "unable to join strings\n");
                HYDU_free_strlist(tmp);

                goto fn_exit;   /* We are done */
            }

            HYDU_FREE(path_loc);
            path_loc = NULL;
        } while ((test_loc = strtok(NULL, ";:")));
    }

    /* There is either no PATH environment or we could not find the
     * file in the PATH. Just return an empty path */
    *path = HYDU_strdup("");

  fn_exit:
    if (user_path)
        HYDU_FREE(user_path);
    if (path_loc)
        HYDU_FREE(path_loc);
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYDU_get_base_path(const char *execname, char *wdir, char **path)
{
    char *loc, *post;
    char *tmp[HYD_NUM_TMP_STRINGS];
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    /* Find the last '/' in the executable name */
    post = HYDU_strdup(execname);
    loc = strrchr(post, '/');
    if (!loc) { /* If there is no path */
        *path = NULL;
        status = HYDU_find_in_path(execname, path);
        HYDU_ERR_POP(status, "error while searching for executable in the user path\n");
    }
    else {      /* There is a path */
        *(++loc) = 0;

        /* Check if its absolute or relative */
        if (post[0] != '/') {   /* relative */
            tmp[0] = HYDU_strdup(wdir);
            tmp[1] = HYDU_strdup("/");
            tmp[2] = HYDU_strdup(post);
            tmp[3] = NULL;
            status = HYDU_str_alloc_and_join(tmp, path);
            HYDU_ERR_POP(status, "unable to join strings\n");
            HYDU_free_strlist(tmp);
        }
        else {  /* absolute */
            *path = HYDU_strdup(post);
        }
    }

  fn_exit:
    if (post)
        HYDU_FREE(post);
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


char *HYDU_getcwd(void)
{
    char *pwdval, *cwdval, *retval = NULL;
    HYD_status status = HYD_SUCCESS;
#if defined HAVE_STAT
    struct stat spwd, scwd;
#endif /* HAVE_STAT */

    pwdval = getenv("PWD");
    HYDU_MALLOC(cwdval, char *, HYDRA_MAX_PATH, status);
    if (getcwd(cwdval, HYDRA_MAX_PATH) == NULL)
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR,
                            "allocated space is too small for absolute path\n");

#if defined HAVE_STAT
    if (pwdval && stat(pwdval, &spwd) != -1 && stat(cwdval, &scwd) != -1 &&
        spwd.st_dev == scwd.st_dev && spwd.st_ino == scwd.st_ino) {
        /* PWD and getcwd() match; use the PWD value */
        retval = HYDU_strdup(pwdval);
        HYDU_free(cwdval);
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


HYD_status HYDU_parse_hostfile(char *hostfile,
                               HYD_status(*process_token) (char *token, int newline))
{
    char line[HYD_TMP_STRLEN], **tokens;
    FILE *fp;
    int i;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if ((fp = fopen(hostfile, "r")) == NULL)
        HYDU_ERR_SETANDJUMP1(status, HYD_INTERNAL_ERROR,
                             "unable to open host file: %s\n", hostfile);

    while (fgets(line, HYD_TMP_STRLEN, fp)) {
        char *linep = NULL;

        linep = line;

        strtok(linep, "#");
        while (isspace(*linep))
            linep++;

        /* Ignore blank lines & comments */
        if ((*linep == '#') || (*linep == '\0'))
            continue;

        tokens = HYDU_str_to_strlist(linep);
        if (!tokens)
            HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR,
                                "Unable to convert host file entry to strlist\n");

        for (i = 0; tokens[i]; i++) {
            status = process_token(tokens[i], !i);
            HYDU_ERR_POP(status, "unable to process token\n");
        }
    }

    fclose(fp);


  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
