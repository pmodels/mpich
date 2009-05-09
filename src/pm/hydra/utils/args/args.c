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
        return 0; /* no such file, or not a regular file */
    }

    return 1;
}

HYD_Status HYDU_get_base_path(char *execname, char *wdir, char **path)
{
    char *loc, *post;
    char *user_path = NULL, *tmp[HYD_NUM_TMP_STRINGS], *path_loc = NULL, *test_loc;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    /* Find the last '/' in the executable name */
    post = HYDU_strdup(execname);
    loc = strrchr(post, '/');
    if (!loc) { /* If there is no path */
        /* The executable is somewhere in the user's path. We need to
         * find it. */
        if (getenv("PATH")) { /* If the PATH environment exists */
            user_path = HYDU_strdup(getenv("PATH"));
            test_loc = strtok(user_path, ";:");
            do {
                tmp[0] = test_loc;
                tmp[1] = "/";
                tmp[2] = execname;
                tmp[3] = NULL;

                status = HYDU_str_alloc_and_join(tmp, &path_loc);
                HYDU_ERR_POP(status, "unable to join strings\n");

                if (exists(path_loc)) {
                    tmp[0] = test_loc;
                    tmp[1] = "/";
                    tmp[2] = NULL;

                    status = HYDU_str_alloc_and_join(tmp, path);
                    HYDU_ERR_POP(status, "unable to join strings\n");

                    goto fn_exit; /* We are done */
                }
            } while ((test_loc = strtok(NULL, ";:")));
        }

        /* There is either no PATH environment or we could not find
         * the file in the PATH. Just return an empty path */
        *path = HYDU_strdup("");
    }
    else {      /* There is a path */
        *(++loc) = 0;

        /* Check if its absolute or relative */
        if (post[0] != '/') {   /* relative */
            tmp[0] = wdir;
            tmp[1] = "/";
            tmp[2] = post;
            tmp[3] = NULL;
            status = HYDU_str_alloc_and_join(tmp, path);
            HYDU_ERR_POP(status, "unable to join strings\n");
        }
        else {  /* absolute */
            *path = HYDU_strdup(post);
        }
    }

  fn_exit:
    if (post)
        HYDU_FREE(post);
    if (user_path)
        HYDU_FREE(user_path);
    if (path_loc)
        HYDU_FREE(path_loc);
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
