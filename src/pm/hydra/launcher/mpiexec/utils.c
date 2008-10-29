/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "hydra_dbg.h"
#include "hydra_mem.h"
#include "mpiexec.h"
#include "lchu.h"
#include "csi.h"

#define CHECK_LOCAL_PARAM_START(start, status) \
    { \
	if ((start)) {			 \
	    (status) = HYD_INTERNAL_ERROR;	\
	    goto fn_fail; \
	} \
    }

#define CHECK_NEXT_ARG_VALID(status) \
    { \
	--argc; ++argv; \
	if (!argc || !argv) { \
	    (status) = HYD_INTERNAL_ERROR;	\
	    goto fn_fail; \
	} \
    }

#define SET_PROP_VAL(prop, val, status)		\
    { \
	if ((prop) != HYD_LCHI_PROPAGATE_NOTSET) { \
	    (status) = HYD_INTERNAL_ERROR;	\
	    goto fn_fail; \
	} \
	(prop) = val; \
    }


#if defined FUNCNAME
#undef FUNCNAME
#endif /* FUNCNAME */
#define FUNCNAME "HYD_LCHI_Dump_params"
void HYD_LCHI_Dump_params(HYD_LCHI_Params_t params)
{
    HYD_CSI_Env_t * env;
    struct HYD_LCHI_Local * run;
    HYD_CSI_Exec_t * exec;

    HYDU_FUNC_ENTER();

    printf("\nGlobal environment:\n");
    env = params.global.added_env_list;
    while (env) {
	printf("\t%s: %s\n", env->env_name, env->env_value);
	env = env->next;
    }
    printf("Global propagation: %s\n", prop_to_str(params.global.prop));
    env = params.global.prop_env_list;
    while (env) {
	printf("\t%s: %s\n", env->env_name, env->env_value);
	env = env->next;
    }

    printf("\nLocal executables:\n");
    run = params.local;
    while (run) {
	printf("\tExecutable: ");
	exec = run->exec;
	while (exec) {
	    printf("%s ", exec->arg);
	    exec = exec->next;
	}
	printf("\n");
	printf("\tNumber of processes: %d\n", run->num_procs);
	printf("\tHostfile: %s\n", run->hostfile);
	printf("\tLocal Environment:\n");
	env = run->added_env_list;
	while (env) {
	    printf("\t\t%s: %s\n", env->env_name, env->env_value);
	    env = env->next;
	}
	printf("\tLocal propagation: %s\n", prop_to_str(run->prop));
	env = run->prop_env_list;
	while (env) {
	    printf("\t\t%s: %s\n", env->env_name, env->env_value);
	    env = env->next;
	}

	run = run->next;
	printf("\n");
    }

    goto fn_exit;

fn_exit:
    HYDU_FUNC_EXIT();
    return;
}

static HYD_CSI_Env_t * env_found_in_list(HYD_CSI_Env_t * env_list, HYD_CSI_Env_t * env)
{
    HYD_CSI_Env_t * run;

    HYDU_FUNC_ENTER();

    run = env_list;
    while (run->next) {
	if (!strcmp(run->env_name, env->env_name))
	    goto fn_exit;
	run = run->next;
    }
    run = NULL;

    goto fn_exit;

fn_exit:
    HYDU_FUNC_EXIT();
    return run;
}


#if defined FUNCNAME
#undef FUNCNAME
#endif /* FUNCNAME */
#define FUNCNAME "HYD_LCHI_Global_env_list"
HYD_Status HYD_LCHI_Global_env_list(HYD_CSI_Env_t ** env_list)
{
    HYD_CSI_Env_t * env;
    char * env_name, * env_value, * env_str;
    int i;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    *env_list = NULL;
    i = 0;
    while (environ[i]) {
	HYDU_MALLOC(env, HYD_CSI_Env_t *, sizeof(HYD_CSI_Env_t), status);

	env_str = MPIU_Strdup(environ[i]);
	env_name = strtok(env_str, "=");
	env_value = strtok(NULL, "=");
	env->env_name = MPIU_Strdup(env_name);
	env->env_value = env_value ? MPIU_Strdup(env_value) : NULL;
	env->env_type = HYD_CSI_ENV_STATIC;

	status = HYD_LCHU_Add_env_to_list(env_list, env);
	if (status != HYD_SUCCESS) {
	    HYDU_Error_printf("launcher returned error adding env to list\n");
	    goto fn_fail;
	}

	i++;
    }

fn_exit:
    HYDU_FUNC_EXIT();
    return status;

fn_fail:
    goto fn_exit;
}


#if defined FUNCNAME
#undef FUNCNAME
#endif /* FUNCNAME */
#define FUNCNAME "HYD_LCHI_Env_copy"
void HYD_LCHI_Env_copy(HYD_CSI_Env_t * dest, HYD_CSI_Env_t src)
{
    HYDU_FUNC_ENTER();

    memcpy(dest, &src, sizeof(HYD_CSI_Env_t));
    dest->next = NULL;

    goto fn_exit;

fn_exit:
    HYDU_FUNC_EXIT();
    return;
}


#if defined FUNCNAME
#undef FUNCNAME
#endif /* FUNCNAME */
#define FUNCNAME "HYD_LCHI_Create_env_list"
HYD_Status HYD_LCHI_Create_env_list(HYD_LCHI_Propagate_t prop, HYD_CSI_Env_t * added_env_list,
				    HYD_CSI_Env_t * prop_env_list, HYD_CSI_Env_t ** env_list)
{
    HYD_CSI_Env_t * env, * tenv;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    *env_list = NULL;
    if (prop == HYD_LCHI_PROPAGATE_ALL) {
	/* First add all environment variables, then override with the
	 * explicitly added ones. */

	env = added_env_list;
	while (env) {
	    HYDU_MALLOC(tenv, HYD_CSI_Env_t *, sizeof(HYD_CSI_Env_t), status);
	    HYD_LCHI_Env_copy(tenv, *env);
	    status = HYD_LCHU_Add_env_to_list(env_list, tenv);
	    if (status != HYD_SUCCESS) {
		HYDU_Error_printf("launcher returned error adding env to list\n");
		goto fn_fail;
	    }
	    env = env->next;
	}
    }
    else if (prop == HYD_LCHI_PROPAGATE_LIST) {
	/* This is a more complicated case. First, we add all
	 * environment variables from the list to be propagated. Then
	 * we try to find it value from the global environment
	 * variables. Finally, we override these values with the
	 * explicitly provided environment variables. */

	env = prop_env_list;
	while (env) {
	    HYDU_MALLOC(tenv, HYD_CSI_Env_t *, sizeof(HYD_CSI_Env_t), status);
	    HYD_LCHI_Env_copy(tenv, *env);
	    HYD_LCHU_Add_env_to_list(env_list, tenv);
	    env = env->next;
	}

	status = HYD_LCHI_Global_env_list(&env);
	if (status != HYD_SUCCESS) {
	    HYDU_Error_printf("global env list creation returned an error\n");
	    goto fn_fail;
	}

	while (env) {
	    if (env_found_in_list(prop_env_list, env)) {
		HYDU_MALLOC(tenv, HYD_CSI_Env_t *, sizeof(HYD_CSI_Env_t), status);
		HYD_LCHI_Env_copy(tenv, *env);
		status = HYD_LCHU_Add_env_to_list(env_list, tenv);
		if (status != HYD_SUCCESS) {
		    HYDU_Error_printf("launcher returned error adding env to list\n");
		    goto fn_fail;
		}
	    }
	    env = env->next;
	}

	env = added_env_list;
	while (env) {
	    if (env_found_in_list(prop_env_list, env)) {
		HYDU_MALLOC(tenv, HYD_CSI_Env_t *, sizeof(HYD_CSI_Env_t), status);
		HYD_LCHI_Env_copy(tenv, *env);
		status = HYD_LCHU_Add_env_to_list(env_list, tenv);
		if (status != HYD_SUCCESS) {
		    HYDU_Error_printf("launcher returned error adding env to list\n");
		    goto fn_fail;
		}
	    }
	    env = env->next;
	}
    }

fn_exit:
    HYDU_FUNC_EXIT();
    return status;

fn_fail:
    goto fn_exit;
}

static HYD_Status allocate_local(struct HYD_LCHI_Local ** local)
{
    HYD_Status status = HYD_SUCCESS;

    HYDU_MALLOC(*local, struct HYD_LCHI_Local *, sizeof(struct HYD_LCHI_Local), status);
    (*local)->exec = NULL;
    (*local)->num_procs = -1;
    (*local)->hostfile = NULL;
    (*local)->added_env_list = NULL;
    (*local)->prop = HYD_LCHI_PROPAGATE_NOTSET;
    (*local)->prop_env_list = NULL;
    (*local)->next = NULL;

fn_exit:
    return status;

fn_fail:
    goto fn_exit;
}

static HYD_Status get_current_local(struct HYD_LCHI_Local ** local, struct HYD_LCHI_Local ** nlocal)
{
    HYD_Status status = HYD_SUCCESS;

    if (*local == NULL) {
	status = allocate_local(local);
	if (status != HYD_SUCCESS) {
	    HYDU_Error_printf("allocate_local returned error\n");
	    goto fn_fail;
	}
    }

    *nlocal = *local;
    while ((*nlocal)->next)
	*nlocal = (*nlocal)->next;

fn_exit:
    return status;

fn_fail:
    goto fn_exit;
}


#if defined FUNCNAME
#undef FUNCNAME
#endif /* FUNCNAME */
#define FUNCNAME "HYD_LCHI_Get_parameters"
HYD_Status HYD_LCHI_Get_parameters(int t_argc, char ** t_argv, HYD_LCHI_Params_t * params)
{
    int argc = t_argc;
    char ** argv = t_argv;
    int local_params_started;
    HYD_CSI_Env_t * env;
    char * envstr, * arg;
    struct HYD_LCHI_Local * local;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    params->debug_level = -1;
    params->global.added_env_list = NULL;
    params->global.prop = HYD_LCHI_PROPAGATE_NOTSET;
    params->global.prop_env_list = NULL;
    params->global.wdir = NULL;
    params->local = NULL;

    local_params_started = 0;
    while (--argc && ++argv) {

	if (!strcmp(*argv, "--debug-level")) {
	    CHECK_LOCAL_PARAM_START(local_params_started, status);
	    CHECK_NEXT_ARG_VALID(status);

	    /* Debug level already set */
	    if (params->debug_level != -1) {
		HYDU_Error_printf("Duplicate debug level setting; previously set to %d\n", params->debug_level);
		status = HYD_INTERNAL_ERROR;
		goto fn_fail;
	    }

	    params->debug_level = atoi(*argv);
	    continue;
	}

	if (!strcmp(*argv, "-genv")) {
	    CHECK_LOCAL_PARAM_START(local_params_started, status);

	    HYDU_MALLOC(env, HYD_CSI_Env_t *, sizeof(HYD_CSI_Env_t), status);

	    CHECK_NEXT_ARG_VALID(status);
	    env->env_name = MPIU_Strdup(*argv);

	    CHECK_NEXT_ARG_VALID(status);
	    env->env_value = MPIU_Strdup(*argv);

	    env->env_type = HYD_CSI_ENV_STATIC;

	    HYD_LCHU_Add_env_to_list(&params->global.added_env_list, env);
	    continue;
	}

	if (!strcmp(*argv, "-genvnone")) {
	    CHECK_LOCAL_PARAM_START(local_params_started, status);
	    SET_PROP_VAL(params->global.prop, HYD_LCHI_PROPAGATE_NONE, status);
	    continue;
	}

	if (!strcmp(*argv, "-genvall")) {
	    CHECK_LOCAL_PARAM_START(local_params_started, status);
	    SET_PROP_VAL(params->global.prop, HYD_LCHI_PROPAGATE_ALL, status);
	    continue;
	}

	if (!strcmp(*argv, "-genvlist")) {
	    CHECK_LOCAL_PARAM_START(local_params_started, status);
	    SET_PROP_VAL(params->global.prop, HYD_LCHI_PROPAGATE_LIST, status);
	    CHECK_NEXT_ARG_VALID(status);

	    arg = *argv;
	    do {
		envstr = strtok(arg, ",");
		arg = NULL;

		if (!envstr)
		    break;

		/* We don't need to set the value or type of the
		 * environment in this list. */
		HYDU_MALLOC(env, HYD_CSI_Env_t *, sizeof(HYD_CSI_Env_t), status);
		env->env_name = MPIU_Strdup(envstr);

		HYD_LCHU_Add_env_to_list(&params->global.prop_env_list, env);
	    } while (envstr);

	    continue;
	}

	if (!strcmp(*argv, "-wdir")) {
	    CHECK_LOCAL_PARAM_START(local_params_started, status);
	    CHECK_NEXT_ARG_VALID(status);
	    params->global.wdir = MPIU_Strdup(*argv);
	}

	if (!strcmp(*argv, "-n")) {
	    local_params_started = 1;

	    CHECK_NEXT_ARG_VALID(status);
	    status = get_current_local(&params->local, &local);
	    if (status != HYD_SUCCESS) {
		HYDU_Error_printf("get_current_local returned error\n");
		goto fn_fail;
	    }

	    /* Num_procs already set */
	    if (local->num_procs != -1) {
		HYDU_Error_printf("Duplicate setting for number of processes; previously set to %d\n", local->num_procs);
		status = HYD_INTERNAL_ERROR;
		goto fn_fail;
	    }

	    local->num_procs = atoi(*argv);

	    continue;
	}

	if (!strcmp(*argv, "-f")) {
	    local_params_started = 1;

	    CHECK_NEXT_ARG_VALID(status);

	    status = get_current_local(&params->local, &local);
	    if (status != HYD_SUCCESS) {
		HYDU_Error_printf("get_current_local returned error\n");
		goto fn_fail;
	    }

	    /* Hostfile already set */
	    if (local->hostfile) {
		HYDU_Error_printf("Duplicate hostfile setting; previously set to %s\n", local->hostfile);
		status = HYD_INTERNAL_ERROR;
		goto fn_fail;
	    }

	    local->hostfile = MPIU_Strdup(*argv);
	    continue;
	}

	if (!strcmp(*argv, "-env")) {
	    local_params_started = 1;

	    HYDU_MALLOC(env, HYD_CSI_Env_t *, sizeof(HYD_CSI_Env_t), status);

	    CHECK_NEXT_ARG_VALID(status);
	    env->env_name = MPIU_Strdup(*argv);

	    CHECK_NEXT_ARG_VALID(status);
	    env->env_value = MPIU_Strdup(*argv);

	    env->env_type = HYD_CSI_ENV_STATIC;

	    status = get_current_local(&params->local, &local);
	    if (status != HYD_SUCCESS) {
		HYDU_Error_printf("get_current_local returned error\n");
		goto fn_fail;
	    }

	    HYD_LCHU_Add_env_to_list(&local->added_env_list, env);
	    continue;
	}

	if (!strcmp(*argv, "-envnone")) {
	    local_params_started = 1;
	    status = get_current_local(&params->local, &local);
	    if (status != HYD_SUCCESS) {
		HYDU_Error_printf("get_current_local returned error\n");
		goto fn_fail;
	    }
	    SET_PROP_VAL(local->prop, HYD_LCHI_PROPAGATE_NONE, status);
	    continue;
	}

	if (!strcmp(*argv, "-envall")) {
	    local_params_started = 1;
	    status = get_current_local(&params->local, &local);
	    if (status != HYD_SUCCESS) {
		HYDU_Error_printf("get_current_local returned error\n");
		goto fn_fail;
	    }
	    SET_PROP_VAL(local->prop, HYD_LCHI_PROPAGATE_ALL, status);
	    continue;
	}

	if (!strcmp(*argv, "-envlist")) {
	    char * envstr;

	    local_params_started = 1;

	    status = get_current_local(&params->local, &local);
	    if (status != HYD_SUCCESS) {
		HYDU_Error_printf("get_current_local returned error\n");
		goto fn_fail;
	    }
	    SET_PROP_VAL(local->prop, HYD_LCHI_PROPAGATE_LIST, status);

	    CHECK_NEXT_ARG_VALID(status);

	    arg = *argv;
	    do {
		envstr = strtok(arg, ",");
		arg = NULL;

		if (!envstr)
		    break;

		/* We don't need to set the value or type of the
		 * environment in this list. */
		HYDU_MALLOC(env, HYD_CSI_Env_t *, sizeof(HYD_CSI_Env_t), status);
		env->env_name = MPIU_Strdup(envstr);

	        HYD_LCHU_Add_env_to_list(&local->prop_env_list, env);
	    } while (envstr);

	    continue;
	}

	/* This is to catch everything that fell through */
	if (1) {
	    HYD_CSI_Exec_t * exec;

	    local_params_started = 1;
	    status = get_current_local(&params->local, &local);
	    if (status != HYD_SUCCESS) {
		HYDU_Error_printf("get_current_local returned error\n");
		goto fn_fail;
	    }

	    exec = NULL;
	    do {
		if (!strcmp(*argv, ":")) { /* Move to the next local executable */
		    status = allocate_local(&local->next);
		    if (status != HYD_SUCCESS) {
			HYDU_Error_printf("allocate_local returned error\n");
			goto fn_fail;
		    }
		    break;
		}

		if (!exec) { /* First element */
		    HYDU_MALLOC(local->exec, HYD_CSI_Exec_t *, sizeof(HYD_CSI_Exec_t), status);
		    exec = local->exec;
		    exec->next = NULL;
		}
		else {
		    HYDU_MALLOC(exec->next, HYD_CSI_Exec_t *, sizeof(HYD_CSI_Exec_t), status);
		    exec->next->next = NULL;
		    exec = exec->next;
		}

		exec->arg = MPIU_Strdup(*argv);
	    } while (--argc && ++argv);

	    /* No more parameters, we are done */
	    if (!argc || !argv)
		break;

	    continue;
	}
    }

    /* Check on what's set and what's not */
    if (params->debug_level == -1)
	params->debug_level = 0; /* Default debug level */

    /* We need at least one local exec */
    if (params->local == NULL) {
	HYDU_Error_printf("No local options set\n");
	status = HYD_INTERNAL_ERROR;
	goto fn_fail;
    }

    /* If wdir is not set, use the current one */
    if (params->global.wdir == NULL) {
	params->global.wdir = MPIU_Strdup(getcwd(NULL, 0));
    }

    local = params->local;
    while (local) {
	if (local->prop == HYD_LCHI_PROPAGATE_NOTSET &&
	    params->global.prop == HYD_LCHI_PROPAGATE_NOTSET)
	    local->prop = HYD_LCHI_PROPAGATE_ALL;

	if (local->num_procs == -1)
	    local->num_procs = 1;

	if ((local->prop != HYD_LCHI_PROPAGATE_NOTSET &&
	    params->global.prop != HYD_LCHI_PROPAGATE_NOTSET) ||
	    (local->added_env_list != NULL &&
	     params->global.prop != HYD_LCHI_PROPAGATE_NOTSET) ||
	    (local->exec == NULL)) {
	    HYDU_Error_printf("Mismatch in environment propagation settings\n");
	    status = HYD_INTERNAL_ERROR;
	    goto fn_fail;
	}

	if (local->hostfile == NULL && getenv("HYDRA_HOST_FILE")) {
	    local->hostfile = MPIU_Strdup(getenv("HYDRA_HOST_FILE"));
	}
	if (local->hostfile == NULL) {
	    HYDU_Error_printf("Host file not specified\n");
	    status = HYD_INTERNAL_ERROR;
	    goto fn_fail;
	}

	local = local->next;
    }

fn_exit:
    HYDU_FUNC_EXIT();
    return status;

fn_fail:
    goto fn_exit;
}
