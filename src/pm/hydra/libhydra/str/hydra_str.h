/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2017 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef HYDRA_STR_H_INCLUDED
#define HYDRA_STR_H_INCLUDED

#include "hydra_base.h"

struct HYD_string_stash {
    char **strlist;
    int max_count;
    int cur_count;
};

#define HYD_STRING_STASH_INIT(stash)            \
    do {                                        \
        (stash).strlist = NULL;                 \
        (stash).max_count = 0;                  \
        (stash).cur_count = 0;                  \
    } while (0)

#define HYD_STRING_STASH(stash, str, status)                            \
    do {                                                                \
        if ((stash).cur_count >= (stash).max_count - 1) {               \
            HYD_REALLOC((stash).strlist, char **,              \
                                 ((stash).max_count + HYD_NUM_TMP_STRINGS) * sizeof(char *), \
                                 (status));                             \
            (stash).max_count += HYD_NUM_TMP_STRINGS;                   \
        }                                                               \
        (stash).strlist[(stash).cur_count++] = (str);                   \
        (stash).strlist[(stash).cur_count] = NULL;                      \
    } while (0)

#define HYD_STRING_SPIT(stash, str, status)                             \
    do {                                                                \
        if ((stash).cur_count == 0) {                                   \
            (str) = MPL_strdup("");                                     \
        }                                                               \
        else {                                                          \
            (status) = HYD_str_alloc_and_join((stash).strlist, &(str)); \
            HYD_ERR_POP((status), "unable to join strings\n");         \
            HYD_str_free_list((stash).strlist);                         \
            MPL_free((stash).strlist);                                  \
            HYD_STRING_STASH_INIT((stash));                             \
        }                                                               \
    } while (0)

#define HYD_STRING_STASH_FREE(stash)            \
    do {                                        \
        if ((stash).strlist == NULL)            \
            break;                              \
        HYD_str_free_list((stash).strlist);     \
        MPL_free((stash).strlist);              \
        (stash).max_count = 0;                  \
        (stash).cur_count = 0;                  \
    } while (0)

HYD_status HYD_str_print_list(char *const *const args);
void HYD_str_free_list(char **args);
HYD_status HYD_str_alloc_and_join(char **strlist, char **strjoin);
HYD_status HYD_str_split(char *str, char **str1, char **str2, char sep);
char *HYD_str_from_int(int x);
char *HYD_str_from_int_pad(int x, int maxlen);
char **HYD_str_to_strlist(char *str);

#endif /* HYDRA_STR_H_INCLUDED */
