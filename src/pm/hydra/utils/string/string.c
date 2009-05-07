/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_utils.h"

HYD_Status HYDU_list_append_strlist(char **src_strlist, char **dest_strlist)
{
    int i, j;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    i = HYDU_strlist_lastidx(dest_strlist);
    for (j = 0; src_strlist[j]; j++)
        dest_strlist[i++] = HYDU_strdup(src_strlist[j]);
    dest_strlist[i++] = NULL;

    HYDU_FUNC_EXIT();
    return status;
}


HYD_Status HYDU_print_strlist(char **strlist)
{
    int arg;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    for (arg = 0; strlist[arg]; arg++)
        printf("%s ", strlist[arg]);
    printf("\n");

    HYDU_FUNC_EXIT();
    return status;
}


void HYDU_free_strlist(char **strlist)
{
    int arg;

    HYDU_FUNC_ENTER();

    for (arg = 0; strlist[arg]; arg++)
        HYDU_FREE(strlist[arg]);

    HYDU_FUNC_EXIT();
}


HYD_Status HYDU_str_alloc_and_join(char **strlist, char **strjoin)
{
    int len = 0, i, count;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    for (i = 0; strlist[i] != NULL; i++) {
        len += strlen(strlist[i]);
    }

    HYDU_MALLOC(*strjoin, char *, len + 1, status);
    count = 0;
    (*strjoin)[0] = 0;

    for (i = 0; strlist[i] != NULL; i++) {
        HYDU_snprintf(*strjoin + count, len - count + 1, "%s", strlist[i]);
        count += strlen(strlist[i]);
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


HYD_Status HYDU_strsplit(char *str, char **str1, char **str2, char sep)
{
    int i;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (str == NULL)
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "");

    *str1 = HYDU_strdup(str);
    for (i = 0; (*str1)[i] && ((*str1)[i] != sep); i++);

    if ((*str1)[i] == 0)        /* End of the string */
        *str2 = NULL;
    else {
        *str2 = HYDU_strdup(&((*str1)[i + 1]));
        (*str1)[i] = 0;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


HYD_Status HYDU_strdup_list(char *src[], char **dest[])
{
    int i, count;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    count = HYDU_strlist_lastidx(src);
    HYDU_MALLOC(*dest, char **, (count + 1) * sizeof(char *), status);

    for (i = 0; i < count; i++)
        (*dest)[i] = HYDU_strdup(src[i]);
    (*dest)[i] = NULL;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


char *HYDU_int_to_str(int x)
{
    int len = 1, max = 10, y;
    char *str;
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

    HYDU_MALLOC(str, char *, len + 1, status);
    HYDU_ERR_POP(status, "unable to allocate memory\n");

    HYDU_snprintf(str, len + 1, "%d", x);

  fn_exit:
    HYDU_FUNC_EXIT();
    return str;

  fn_fail:
    goto fn_exit;
}


char *HYDU_strerror(int error)
{
    char *str;

#if defined HAVE_STRERROR
    str = strerror(error);
#else
    str = HYDU_strdup("errno: %d", error);
#endif /* HAVE_STRERROR */

    return str;
}


int HYDU_strlist_lastidx(char **strlist)
{
    int i;

    for (i = 0; strlist[i]; i++);

    return i;
}

char **HYDU_str_to_strlist(char *str)
{
    int argc = 0;
    char **strlist = NULL;
    char *p, *r;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDU_CALLOC(strlist, char **, HYD_NUM_TMP_STRINGS, sizeof(char *), status);
    if (!strlist)
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR,
                            "Unable to allocate mem for strlist\n");

    p = str;
    while (*p) {
        while (isspace(*p))
            p++;
        if (argc >= HYD_NUM_TMP_STRINGS)
            HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "too many arguments in line\n");

        /* Make a copy and NULL terminate it */
        strlist[argc] = HYDU_strdup(p);
        r = strlist[argc];
        while (*r && !isspace(*r))
            r++;
        *r = 0;

        while (*p && !isspace(*p))
            p++;
        argc++;
    }
    strlist[argc] = NULL;

  fn_exit:
    HYDU_FUNC_EXIT();
    return strlist;

  fn_fail:
    goto fn_exit;
}
