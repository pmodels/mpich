/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef HYDRA_ENV_H_INCLUDED
#define HYDRA_ENV_H_INCLUDED

#include "hydra.h"

HYD_Status HYDU_Env_global_list(HYD_Env_t ** env_list);
char *HYDU_Env_type_str(HYD_Env_type_t type);
HYD_Env_t *HYDU_Env_dup(HYD_Env_t env);
HYD_Env_t *HYDU_Env_found_in_list(HYD_Env_t * env_list, HYD_Env_t * env);
HYD_Status HYDU_Env_add_to_list(HYD_Env_t ** env_list, HYD_Env_t env);
HYD_Env_t *HYDU_Env_listdup(HYD_Env_t * env);
HYD_Status HYDU_Env_create(HYD_Env_t ** env, char *env_name, char *env_value,
                           HYD_Env_type_t env_type, int start);
HYD_Status HYDU_Env_free(HYD_Env_t * env);
HYD_Status HYDU_Env_free_list(HYD_Env_t * env);

#endif /* HYDRA_ENV_H_INCLUDED */
