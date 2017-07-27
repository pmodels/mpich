/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2017 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_str.h"
#include "hydra_err.h"
#include "hydra_mem.h"

HYD_status HYD_str_print_list(char *const *const strlist)
{
    int arg;
    HYD_status status = HYD_SUCCESS;

    HYD_FUNC_ENTER();

    for (arg = 0; strlist[arg]; arg++)
        HYD_PRINT_NOPREFIX(stdout, "%s ", strlist[arg]);
    HYD_PRINT_NOPREFIX(stdout, "\n");

    HYD_FUNC_EXIT();
    return status;
}


void HYD_str_free_list(char **strlist)
{
    int arg;

    HYD_FUNC_ENTER();

    for (arg = 0; strlist[arg]; arg++)
        MPL_free(strlist[arg]);

    HYD_FUNC_EXIT();
}


HYD_status HYD_str_alloc_and_join(char **strlist, char **strjoin)
{
    int len = 0, i, count;
    HYD_status status = HYD_SUCCESS;

    HYD_FUNC_ENTER();

    for (i = 0; strlist[i] != NULL; i++) {
        len += strlen(strlist[i]);
    }

    HYD_MALLOC(*strjoin, char *, len + 1, status);
    count = 0;
    (*strjoin)[0] = 0;

    for (i = 0; strlist[i] != NULL; i++) {
        MPL_snprintf(*strjoin + count, len - count + 1, "%s", strlist[i]);
        count += strlen(strlist[i]);
    }

  fn_exit:
    HYD_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


HYD_status HYD_str_split(char *str, char **str1, char **str2, char sep)
{
    int i;
    HYD_status status = HYD_SUCCESS;

    HYD_FUNC_ENTER();

    if (str == NULL)
        HYD_ERR_SETANDJUMP(status, HYD_ERR_INTERNAL, "%s", "");

    *str1 = MPL_strdup(str);
    for (i = 0; (*str1)[i] && ((*str1)[i] != sep); i++);

    if ((*str1)[i] == 0)        /* End of the string */
        *str2 = NULL;
    else {
        *str2 = MPL_strdup(&((*str1)[i + 1]));
        (*str1)[i] = 0;
    }

  fn_exit:
    HYD_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


char *HYD_str_from_int(int x)
{
    return HYD_str_from_int_pad(x, 0);
}


char *HYD_str_from_int_pad(int x, int maxlen)
{
    int len = 1, max = 10, y;
    int actual_len, i;
    char *str = NULL;
    HYD_status status = HYD_SUCCESS;

    HYD_FUNC_ENTER();

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

    if (len > maxlen)
        actual_len = len + 1;
    else
        actual_len = maxlen + 1;

    HYD_MALLOC(str, char *, actual_len, status);
    HYD_ERR_POP(status, "unable to allocate memory\n");

    for (i = 0; i < actual_len; i++)
        str[i] = '0';

    MPL_snprintf(str + actual_len - len - 1, len + 1, "%d", x);

  fn_exit:
    HYD_FUNC_EXIT();
    return str;

  fn_fail:
    goto fn_exit;
}

char **HYD_str_to_strlist(char *str)
{
    int argc = 0, i;
    char **strlist = NULL;
    char *p;
    HYD_status status = HYD_SUCCESS;

    HYD_FUNC_ENTER();

    HYD_MALLOC(strlist, char **, HYD_NUM_TMP_STRINGS * sizeof(char *), status);
    if (!strlist)
        HYD_ERR_SETANDJUMP(status, HYD_ERR_INTERNAL, "Unable to allocate mem for strlist\n");

    for (i = 0; i < HYD_NUM_TMP_STRINGS; i++)
        strlist[i] = NULL;

    p = str;
    while (*p) {
        while (isspace(*p))
            p++;

        if (argc >= HYD_NUM_TMP_STRINGS)
            HYD_ERR_SETANDJUMP(status, HYD_ERR_INTERNAL, "too many arguments in line\n");

        HYD_MALLOC(strlist[argc], char *, HYD_TMP_STRLEN, status);

        /* Copy till you hit a space */
        i = 0;
        while (*p && !isspace(*p)) {
            strlist[argc][i] = *p;
            i++;
            p++;
        }
        if (i) {
            strlist[argc][i] = 0;
            argc++;
        }
    }
    if (strlist[argc])
        MPL_free(strlist[argc]);
    strlist[argc] = NULL;

  fn_exit:
    HYD_FUNC_EXIT();
    return strlist;

  fn_fail:
    goto fn_exit;
}
