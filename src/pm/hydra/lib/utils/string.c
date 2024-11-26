/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "hydra.h"

HYD_status HYDU_list_append_strlist(char **src_strlist, char **dest_strlist)
{
    int i, j;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    i = HYDU_strlist_lastidx(dest_strlist);
    for (j = 0; src_strlist[j]; j++)
        dest_strlist[i++] = MPL_strdup(src_strlist[j]);
    dest_strlist[i++] = NULL;

    HYDU_FUNC_EXIT();
    return status;
}


HYD_status HYDU_print_strlist(char **strlist)
{
    int arg;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    for (arg = 0; strlist[arg]; arg++)
        HYDU_dump_noprefix(stdout, "%s ", strlist[arg]);
    HYDU_dump_noprefix(stdout, "\n");

    HYDU_FUNC_EXIT();
    return status;
}


void HYDU_free_strlist(char **strlist)
{
    int arg;

    HYDU_FUNC_ENTER();

    for (arg = 0; strlist[arg]; arg++)
        MPL_free(strlist[arg]);

    HYDU_FUNC_EXIT();
}


HYD_status HYDU_str_alloc_and_join(char **strlist, char **strjoin)
{
    int len = 0, i, count;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    for (i = 0; strlist[i] != NULL; i++) {
        len += (int) strlen(strlist[i]);
    }

    HYDU_MALLOC_OR_JUMP(*strjoin, char *, len + 1, status);
    count = 0;
    (*strjoin)[0] = 0;

    for (i = 0; strlist[i] != NULL; i++) {
        snprintf(*strjoin + count, len - count + 1, "%s", strlist[i]);
        count += (int) strlen(strlist[i]);
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


HYD_status HYDU_strsplit(char *str, char **str1, char **str2, char sep)
{
    int i;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (str == NULL)
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "%s", "");

    *str1 = MPL_strdup(str);
    HYDU_ERR_CHKANDJUMP(status, NULL == *str1, HYD_INTERNAL_ERROR, "%s", "");
    for (i = 0; (*str1)[i] && ((*str1)[i] != sep); i++);

    if ((*str1)[i] == 0)        /* End of the string */
        *str2 = NULL;
    else {
        *str2 = MPL_strdup(&((*str1)[i + 1]));
        (*str1)[i] = 0;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


HYD_status HYDU_strdup_list(char *src[], char **dest[])
{
    int i, count;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    count = HYDU_strlist_lastidx(src);
    HYDU_MALLOC_OR_JUMP(*dest, char **, (count + 1) * sizeof(char *), status);

    for (i = 0; i < count; i++)
        (*dest)[i] = MPL_strdup(src[i]);
    (*dest)[i] = NULL;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


char *HYDU_size_t_to_str(size_t x)
{
    int len = 1, i;
    size_t max = 10;
    char *str = NULL;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    while (x >= max) {
        len++;
        max *= 10;
    }
    len++;

    HYDU_MALLOC_OR_JUMP(str, char *, len, status);
    HYDU_ERR_POP(status, "unable to allocate memory\n");

    for (i = 0; i < len; i++)
        str[i] = '0';

    snprintf(str, len, "%llu", (unsigned long long) x);

  fn_exit:
    HYDU_FUNC_EXIT();
    return str;

  fn_fail:
    goto fn_exit;
}


char *HYDU_int_to_str(int x)
{
    return HYDU_int_to_str_pad(x, 0);
}


char *HYDU_int_to_str_pad(int x, int maxlen)
{
    int len = 1, max = 10, y;
    int actual_len, i;
    char *str = NULL;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (x < 0) {
        len++;
        y = -x;
    } else
        y = x;

    while (y >= max) {
        len++;
        max *= 10;
    }

    if (len > maxlen)
        actual_len = len + 1;
    else
        actual_len = maxlen + 1;

    HYDU_MALLOC_OR_JUMP(str, char *, actual_len, status);
    HYDU_ERR_POP(status, "unable to allocate memory\n");

    for (i = 0; i < actual_len; i++)
        str[i] = '0';

    snprintf(str + actual_len - len - 1, len + 1, "%d", x);

  fn_exit:
    HYDU_FUNC_EXIT();
    return str;

  fn_fail:
    goto fn_exit;
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
    int capacity = 0;
    char **strlist = NULL;
    char *p;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    capacity = 10;
    strlist = MPL_malloc(capacity * sizeof(char *), MPL_MEM_OTHER);
    if (!strlist)
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "Unable to allocate mem for strlist\n");

    p = str;
    while (*p) {
        while (isspace(*p))
            p++;

        /* Copy till you hit a space */
        char *start = p;
        while (*p && !isspace(*p)) {
            p++;
        }
        int len = p - start;
        if (len > 0) {
            char *s;
            HYDU_MALLOC_OR_JUMP(s, char *, len + 1, status);
            MPL_strncpy(s, start, len + 1);

            strlist[argc] = s;
            argc++;
            if (argc == capacity) {
                capacity *= 2;
                strlist = MPL_realloc(strlist, capacity * sizeof(char *), MPL_MEM_OTHER);
                if (!strlist) {
                    HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR,
                                        "Unable to allocate mem for strlist\n");
                }
            }
        } else {
            /* happens when line has trailing spaces (including \n) */
            break;
        }
    }
    strlist[argc] = NULL;

  fn_exit:
    HYDU_FUNC_EXIT();
    return strlist;

  fn_fail:
    goto fn_exit;
}
