/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef BSCU_H_INCLUDED
#define BSCU_H_INCLUDED

#include "hydra.h"
#include "hydra_sig.h"
#include "csi.h"
#include "bsci.h"

#define HYD_BSCU_Append_exec(proc_params, client_arg, arg, num_args) \
    { \
	struct HYD_CSI_Exec * exec; \
	int count = 0; \
	exec = proc_params->exec; \
	while (exec) { \
	    client_arg[arg++] = MPIU_Strdup(exec->arg); \
	    if (++count >= num_args && num_args != -1) \
		break; \
	    exec = exec->next; \
	} \
	client_arg[arg] = NULL; \
    }

#define HYD_BSCU_Append_env(proc_params, client_env, client_arg, arg, num_args) \
    { \
	HYD_CSI_Env_t * env; \
	int count = 0; \
	while (client_env[count]) { \
	    client_arg[arg++] = MPIU_Strdup("export"); \
	    client_arg[arg++] = MPIU_Strdup(client_env[count]); \
	    client_arg[arg++] = MPIU_Strdup(";"); \
	    if (++count >= num_args && num_args != -1) \
		break; \
	} \
	client_arg[arg] = NULL; \
    }

#define HYD_BSCU_Setup_env(proc_params, client_env, process_id, status)	\
    { \
	int arg = 0, len;	  \
	char * env_value, * str;	  \
	HYD_CSI_Env_t * env; \
	env = proc_params->env_list; \
	while (env) { \
	    if (env->env_type == HYD_CSI_ENV_STATIC) \
		env_value = env->env_value; \
	    else { /* This is an auto-increment type */ \
		/* Allocate a small buffer; this is only an integer */ \
		HYDU_Int_to_str(process_id, str, status); \
		HYDU_MALLOC(env_value, char *, strlen(str) + 1, status); \
		MPIU_Snprintf(env_value, strlen(str) + 1, "%s", str); \
	    } \
				 \
	    len = strlen(env->env_name) + 2;	\
	    if (env_value) \
		len += strlen(env_value); \
								\
	    HYDU_MALLOC(client_env[arg], char *, len, status);		\
	    MPIU_Snprintf(client_env[arg++], len, "%s=%s", env->env_name, env_value); \
	    if (env->env_type == HYD_CSI_ENV_AUTOINC) \
		HYDU_FREE(env_value); \
	    env = env->next; \
	} \
	client_env[arg] = NULL; \
    }

typedef struct HYD_BSCU_Procstate {
    int   pid;
    int   exit_status;
} HYD_BSCU_Procstate_t;

extern HYD_BSCU_Procstate_t * HYD_BSCU_Procstate;
extern int HYD_BSCU_Num_procs;
extern int HYD_BSCU_Completed_procs;

HYD_Status HYD_BSCU_Init_exit_status(void);
HYD_Status HYD_BSCU_Finalize_exit_status(void);
HYD_Status HYD_BSCU_Spawn_proc(char ** client_arg, char ** client_env, int * stdin, int * stdout, int * stderr, int * pid);
HYD_Status HYD_BSCU_Wait_for_completion(void);
HYD_Status HYD_BSCU_Set_common_signals(sighandler_t handler);
void HYD_BSCU_Signal_handler(int signal);

#endif /* BSCI_H_INCLUDED */
