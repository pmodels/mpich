/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_utils.h"

HYD_Status HYDU_get_base_path(char *execname, char *wdir, char **path)
{
    char *loc, *pre, *post;
    char *path_str[HYDU_NUM_JOIN_STR];
    int i;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    /* Find the last '/' in the executable name */
    post = MPIU_Strdup(execname);
    loc = strrchr(post, '/');
    if (!loc) { /* If there is no path */
        *path = MPIU_Strdup("");
    }
    else { /* There is a path */
        *(++loc) = 0;

        /* Check if its absolute or relative */
        if (post[0] != '/') { /* relative */
            path_str[0] = wdir;
            path_str[1] = "/";
            path_str[2] = post;
            path_str[3] = NULL;
            status = HYDU_str_alloc_and_join(path_str, path);
            HYDU_ERR_POP(status, "unable to join strings\n");
        }
        else { /* absolute */
            *path = MPIU_Strdup(post);
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
