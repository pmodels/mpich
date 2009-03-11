/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_utils.h"

HYD_Status HYDU_String_alloc_and_join(char **strlist, char **strjoin)
{
    int len = 0, i, count;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    for (i = 0; strlist[i] != NULL; i++)
        len += strlen(strlist[i]);

    HYDU_MALLOC(*strjoin, char *, len + 1, status);
    count = 0;
    (*strjoin)[0] = 0;

    for (i = 0; strlist[i] != NULL; i++) {
        MPIU_Snprintf(*strjoin + count, len - count + 1, "%s", strlist[i]);
        count += strlen(strlist[i]);
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


HYD_Status HYDU_String_int_to_str(int x, char **str)
{
    int len = 1, max = 10, y;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (x < 0) {
        len++;
        y = -x;
    }
    else
        y = x;

    while (y >= max) {
        len++;
        max *= 10;
    }

    *str = (char *) MPIU_Malloc(len + 1);
    if (*str == NULL) {
        HYDU_Error_printf("failed trying to allocate %d bytes\n", len + 1);
        status = HYD_NO_MEM;
        goto fn_fail;
    }

    MPIU_Snprintf(*str, len + 1, "%d", x);

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
