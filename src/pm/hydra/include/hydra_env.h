/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef HYDRA_ENV_H_INCLUDED
#define HYDRA_ENV_H_INCLUDED

#include "hydra.h"

typedef enum {
    HYDU_ENV_STATIC,
    HYDU_ENV_AUTOINC
} HYDU_Env_type_t;

typedef struct HYDU_Env {
    char *env_name;
    char *env_value;

    /* Auto-incrementing environment variables can only be integers */
    HYDU_Env_type_t env_type;
    int start_val;
    struct HYDU_Env *next;
} HYDU_Env_t;

HYD_Status HYDU_Global_env_list(HYDU_Env_t ** env_list);
HYDU_Env_t *HYDU_Envdup(HYDU_Env_t env);
HYDU_Env_t *HYDU_Env_found_in_list(HYDU_Env_t * env_list, HYDU_Env_t * env);
HYD_Status HYDU_Add_env_to_list(HYDU_Env_t ** env_list, HYDU_Env_t env);
HYDU_Env_t *HYDU_Envlistdup(HYDU_Env_t * env);
HYD_Status HYDU_Create_env(HYDU_Env_t ** env, char *env_name, char *env_value,
			   HYDU_Env_type_t env_type, int start);
HYD_Status HYDU_Free_env(HYDU_Env_t * env);
HYD_Status HYDU_Free_env_list(HYDU_Env_t * env);

#endif /* HYDRA_ENV_H_INCLUDED */
