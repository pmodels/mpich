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

HYD_Status HYDU_Env_global_list(HYDU_Env_t ** env_list);
HYDU_Env_t *HYDU_Env_dup(HYDU_Env_t env);
HYDU_Env_t *HYDU_Env_found_in_list(HYDU_Env_t * env_list, HYDU_Env_t * env);
HYD_Status HYDU_Env_add_to_list(HYDU_Env_t ** env_list, HYDU_Env_t env);
HYDU_Env_t *HYDU_Env_listdup(HYDU_Env_t * env);
HYD_Status HYDU_Env_create(HYDU_Env_t ** env, char *env_name, char *env_value,
			   HYDU_Env_type_t env_type, int start);
HYD_Status HYDU_Env_free(HYDU_Env_t * env);
HYD_Status HYDU_Env_free_list(HYDU_Env_t * env);

#endif /* HYDRA_ENV_H_INCLUDED */
