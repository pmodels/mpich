/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef HYDRA_ENV_H_INCLUDED
#define HYDRA_ENV_H_INCLUDED

#include "hydra.h"

typedef struct env_list {
    char            * env_name;
    char            * env_value;
    int             autoinc;
    int             start;
    struct env_list * next;
} HYD_Env_element_t;

typedef struct env_info {
    int                  propagate; /* 1 refers to propagate to all; 0 refers to none */
    HYD_Env_element_t  * env_list;
    int                  env_list_length;
} HYD_Env_info_t;

HYD_Status HYDU_Set_env(HYD_Env_info_t * env_info, char * env_name, char * env_value);
HYD_Status HYDU_Unset_all_env(char ** envp);

#endif /* HYDRA_ENV_H_INCLUDED */
