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


HYD_Status HYDU_String_break(char *str, char **str1, char **str2)
{
    int i;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (str == NULL)
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "");

    *str1 = MPIU_Strdup(str);
    for (i = 0; (*str1)[i] && ((*str1)[i] != '='); i++);

    if ((*str1)[i] == 0) /* End of the string */
        *str2 = NULL;
    else {
        *str2 = &((*str1)[i+1]);
        (*str1)[i] = 0;
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
    if (*str == NULL)
        HYDU_ERR_SETANDJUMP1(status, HYD_NO_MEM, "unable to allocate %d bytes\n", len + 1);

    MPIU_Snprintf(*str, len + 1, "%d", x);

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


char *HYDU_String_error(int error)
{
    char *str;

#if defined HAVE_STRERROR
    str = strerror(error);
#else
    str = MPIU_Strdup("errno: %d", error);
#endif /* HAVE_STRERROR */

    return str;
}
