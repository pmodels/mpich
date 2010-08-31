/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "smpd.h"
#ifdef HAVE_CTYPE_H
#include <ctype.h>
#endif

#undef FCNAME
#define FCNAME "smpd_isnumbers_with_colon"
SMPD_BOOL smpd_isnumbers_with_colon(const char *str)
{
    size_t i, n;
    SMPD_BOOL colon_found;

    smpd_enter_fn(FCNAME);

    n = strlen(str);
    colon_found = SMPD_FALSE;
    for (i=0; i<n; i++)
    {
	if (!isdigit(str[i]))
	{
	    if (str[i] == ':')
	    {
		if (colon_found == SMPD_TRUE)
		{
		    smpd_exit_fn(FCNAME);
		    return SMPD_FALSE;
		}
		colon_found = SMPD_TRUE;
	    }
	    else
	    {
		smpd_exit_fn(FCNAME);
		return SMPD_FALSE;
	    }
	}
    }
    smpd_exit_fn(FCNAME);
    return SMPD_TRUE;
}

#undef FCNAME
#define FCNAME "smpd_handle_spawn_command"
int smpd_handle_spawn_command(smpd_context_t *context)
{
    int result;
    smpd_command_t *cmd, *temp_cmd;
    char ctx_key[100];
    int ncmds, *maxprocs, *nkeyvals, i, j;
    smpd_launch_node_t node;
    char key[100], val[1024];
    char *iter1, *iter2;
    char maxprocs_str[1024], nkeyvals_str[1024], keyvals_str[1024];
    smpd_launch_node_t *launch_list, *launch_iter/*, *launch_temp*/;
    PMI_keyval_t *info;
    char key_temp[SMPD_MAX_NAME_LENGTH], val_temp[SMPD_MAX_VALUE_LENGTH];
    int cur_iproc;
    smpd_host_node_t *host_iter, *host_list;
    int nproc;
    char *cur_env_loc;
    int env_maxlen;
    SMPD_BOOL env_channel_specified = SMPD_FALSE;
    SMPD_BOOL env_dll_specified = SMPD_FALSE;
    SMPD_BOOL env_wrap_dll_specified = SMPD_FALSE;
    smpd_map_drive_node_t *drive_map_list = NULL;

    smpd_enter_fn(FCNAME);

    if (smpd_process.spawning == SMPD_TRUE)
    {
	result = smpd_delayed_spawn_enqueue(context);
	smpd_exit_fn(FCNAME);
	return result;
    }

    cmd = &context->read_cmd;
    smpd_process.exit_on_done = SMPD_TRUE;
    /* populate the host list */
    smpd_get_default_hosts();

    
    /* prepare the result command */


    result = smpd_create_command("result", smpd_process.id, cmd->src, SMPD_FALSE, &temp_cmd);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to create a result command for a spawn command.\n");
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    /* add the command tag for result matching */
    result = smpd_add_command_int_arg(temp_cmd, "cmd_tag", cmd->tag);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the tag to the result command for a spawn command.\n");
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    result = smpd_add_command_arg(temp_cmd, "cmd_orig", cmd->cmd_str);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add cmd_orig to the result command for a %s command\n", cmd->cmd_str);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    /* copy the ctx_key for pmi control channel lookup */
    if (MPIU_Str_get_string_arg(cmd->cmd, "ctx_key", ctx_key, 100) != MPIU_STR_SUCCESS)
    {
	smpd_err_printf("no ctx_key in the spawn command: '%s'\n", cmd->cmd);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    result = smpd_add_command_arg(temp_cmd, "ctx_key", ctx_key);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the ctx_key to the result command for spawn command '%s'.\n", cmd->cmd);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    smpd_process.spawning = SMPD_TRUE;

    /* parse the spawn command */


    if (MPIU_Str_get_int_arg(cmd->cmd, "ncmds", &ncmds) != MPIU_STR_SUCCESS)
    {
	smpd_err_printf("unable to get the ncmds parameter from the spawn command '%s'.\n", cmd->cmd);
	goto spawn_failed;
    }
    /*printf("ncmds = %d\n", ncmds);fflush(stdout);*/
    if (MPIU_Str_get_string_arg(cmd->cmd, "maxprocs", maxprocs_str, 1024) != MPIU_STR_SUCCESS)
    {
	smpd_err_printf("unable to get the maxrpocs parameter from the spawn command '%s'.\n", cmd->cmd);
	goto spawn_failed;
    }
    if (MPIU_Str_get_string_arg(cmd->cmd, "nkeyvals", nkeyvals_str, 1024) != MPIU_STR_SUCCESS)
    {
	smpd_err_printf("unable to get the nkeyvals parameter from the spawn command '%s'.\n", cmd->cmd);
	goto spawn_failed;
    }
    maxprocs = (int*)MPIU_Malloc(ncmds * sizeof(int));
    if (maxprocs == NULL)
    {
	smpd_err_printf("unable to allocate the maxprocs array.\n");
	goto spawn_failed;
    }
    nkeyvals = (int*)MPIU_Malloc(ncmds * sizeof(int));
    if (nkeyvals == NULL)
    {
	smpd_err_printf("unable to allocate the nkeyvals array.\n");
	goto spawn_failed;
    }
    iter1 = maxprocs_str;
    iter2 = nkeyvals_str;
    for (i=0; i<ncmds; i++)
    {
	result = MPIU_Str_get_string(&iter1, key, 100);
	if (result != MPIU_STR_SUCCESS)
	{
	    smpd_err_printf("unable to get the %dth string from the maxprocs parameter to the spawn command '%s'.\n", i, cmd->cmd);
	    goto spawn_failed;
	}
	maxprocs[i] = atoi(key);
	/*printf("maxprocs[%d] = %d\n", i, maxprocs[i]);fflush(stdout);*/
	result = MPIU_Str_get_string(&iter2, key, 100);
	if (result != MPIU_STR_SUCCESS)
	{
	    smpd_err_printf("unable to get the %dth string from the nkeyvals parameter to the spawn command '%s'.\n", i, cmd->cmd);
	    goto spawn_failed;
	}
	nkeyvals[i] = atoi(key);
	/*printf("nkeyvals[%d] = %d\n", i, nkeyvals[i]);fflush(stdout);*/
    }
    info = NULL;
    launch_list = NULL;
    launch_iter = NULL;
    cur_iproc = 0;
    for (i=0; i<ncmds; i++)
    {
	/* reset the node fields */
	node.appnum = -1;
	node.args[0] = '\0';
	node.clique[0] = '\0';
	node.dir[0] = '\0';
	node.env = node.env_data;
	node.env_data[0] = '\0';
	node.exe[0] = '\0';
	node.host_id = -1;
	node.hostname[0] = '\0';
        node.alt_hostname[0] = '\0';
	node.iproc = -1;
	node.map_list = NULL;
	node.next = NULL;
	node.nproc = -1;
	node.path[0] = '\0';
	node.prev = NULL;
	node.priority_class = -1;
	node.priority_thread = -1;
	cur_env_loc = node.env_data;
	env_maxlen = SMPD_MAX_ENV_LENGTH;
	drive_map_list = NULL;

	if (info != NULL)
	{
	    /* free the last round of infos */
	    for (j=0; j<nkeyvals[i-1]; j++)
	    {
		MPIU_Free(info[j].key);
		MPIU_Free(info[j].val);
	    }
	    MPIU_Free(info);
	}
	/* allocate some new infos */
	if (nkeyvals[i] > 0)
	{
	    info = (PMI_keyval_t*)MPIU_Malloc(nkeyvals[i] * sizeof(PMI_keyval_t));
	    if (info == NULL)
	    {
		smpd_err_printf("unable to allocate memory for the info keyvals (cmd %d, num_infos %d).\n", i, nkeyvals[i]);
		goto spawn_failed;
	    }
	}
	else
	{
	    info = NULL;
	}
	/* parse the keyvals into infos */
	sprintf(key, "keyvals%d", i);
	if (MPIU_Str_get_string_arg(cmd->cmd, key, keyvals_str, 1024) == MPIU_STR_SUCCESS)
	{
	    /*printf("%s = '%s'\n", key, keyvals_str);fflush(stdout);*/
	    for (j=0; j<nkeyvals[i]; j++)
	    {
		sprintf(key, "%d", j);
		if (MPIU_Str_get_string_arg(keyvals_str, key, val, 1024) != MPIU_STR_SUCCESS)
		{
		    smpd_err_printf("unable to get the %sth key from the keyval string '%s'.\n", key, keyvals_str);
		    goto spawn_failed;
		}
		/*printf("key %d = %s\n", j, val);fflush(stdout);*/
		key_temp[0] = '\0';
		val_temp[0] = '\0';
		iter1 = val;
		result = MPIU_Str_get_string(&iter1, key_temp, SMPD_MAX_NAME_LENGTH);
		if (result != MPIU_STR_SUCCESS)
		{
		    smpd_err_printf("unable to parse the key from the %dth keyval pair in the %dth keyvals string.\n", j, i);
		    goto spawn_failed;
		}
		result = MPIU_Str_get_string(&iter1, val_temp, SMPD_MAX_VALUE_LENGTH); /* eat the '=' character */
		if (result != MPIU_STR_SUCCESS)
		{
		    smpd_err_printf("unable to parse the key from the %dth keyval pair in the %dth keyvals string.\n", j, i);
		    goto spawn_failed;
		}
		result = MPIU_Str_get_string(&iter1, val_temp, SMPD_MAX_VALUE_LENGTH);
		if (result != MPIU_STR_SUCCESS)
		{
		    smpd_err_printf("unable to parse the key from the %dth keyval pair in the %dth keyvals string.\n", j, i);
		    goto spawn_failed;
		}
		info[j].key = MPIU_Strdup(key_temp);
		info[j].val = MPIU_Strdup(val_temp);
	    }
	}
	/* get the current command */
	sprintf(key, "cmd%d", i);
	if (MPIU_Str_get_string_arg(cmd->cmd, key, node.exe, SMPD_MAX_EXE_LENGTH) != MPIU_STR_SUCCESS)
	{
	    smpd_err_printf("unable to get the %s parameter from the spawn command '%s'.\n", key, cmd->cmd);
	    goto spawn_failed;
	}
#ifdef HAVE_WINDOWS_H
	if (strlen(node.exe) > 2)
	{
	    /* the Windows version handles the common unix case where executables are specified like this: ./foo */
	    /* Instead of failing, simply fix the / character */
	    if (node.exe[0] == '.' && node.exe[1] == '/')
		node.exe[1] = '\\';

	    /* If there are / characters but no \ characters should we change them all to \ ? */
	    /* my/sub/dir/app                : yes change them      - check works */
	    /* my\sub\dir\app /1 /2 /bugaloo : no don't change them - check works */
	    /* app /arg:foo                  : no don't change them - check does not work */
	}
#endif
	/*printf("%s = %s\n", key, node.exe);fflush(stdout);*/
	sprintf(key, "argv%d", i);
	if (MPIU_Str_get_string_arg(cmd->cmd, key, node.args, SMPD_MAX_EXE_LENGTH) != MPIU_STR_SUCCESS)
	{
	    node.args[0] = '\0';
	    /*
	    smpd_err_printf("unable to get the %s parameter from the spawn command '%s'.\n", key, cmd->cmd);
	    goto spawn_failed;
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	    */
	}
	/*printf("%s = %s\n", key, node.args);fflush(stdout);*/

	/* interpret the infos for this command */
	for (j=0; j<nkeyvals[i]; j++)
	{
	    /* path */
	    if (strcmp(info[j].key, "path") == 0)
	    {
		if (node.path[0] != '\0')
		{
		    /* multiple path keys */
		    /* replace old, append, error out? */
		}
		strcpy(node.path, info[j].val);
		smpd_dbg_printf("path = %s\n", info[j].val);
	    }
	    /* host */
	    if (strcmp(info[j].key, "host") == 0)
	    {
		if (node.hostname[0] != '\0')
		{
		    /* multiple host keys */
		    /* replace old, error out? */
		}
		smpd_dbg_printf("host key sent with spawn command: <%s>\n", info[j].val);
		if (smpd_get_host_id(info[j].val, &node.host_id) == SMPD_SUCCESS)
		{
		    strcpy(node.hostname, info[j].val);
		}
		else
		{
		    /* smpd_get_host_id should not modify host_id if there is a failure but just to be safe ... */
		    node.host_id = -1;
		}
	    }
	    /* hosts */
	    if (strcmp(info[j].key, "hosts") == 0)
	    {
		smpd_dbg_printf("hosts key sent with spawn command: <%s>\n", info[j].val);
		if (smpd_parse_hosts_string(info[j].val))
		{
		    /*use_machine_file = SMPD_TRUE;*/
		}
	    }
	    /* env */
	    if (strcmp(info[j].key, "env") == 0)
	    {
		char *token;
		char *env_str = MPIU_Strdup(info[j].val);

		/* This simplistic parsing code assumes that environment variables do not have spaces in them
		 * and that the variable name does not have the equals character in it.
		 */

		if (env_str == NULL)
		{
		    goto spawn_failed;
		}
		token = strtok(env_str, " ");
		while (token)
		{
		    char *env_key, *env_val;
		    env_key = MPIU_Strdup(token);
		    if (env_key == NULL)
		    {
			goto spawn_failed;
		    }
		    env_val = env_key;
		    while (*env_val != '\0' && *env_val != '=')
			env_val++;
		    if (*env_val == '=')
		    {
			*env_val = '\0';
			env_val++;
			MPIU_Str_add_string_arg(&cur_env_loc, &env_maxlen, env_key, env_val);
			/* Check for special environment variables */
			if (strcmp(env_val, "MPICH2_CHANNEL") == 0)
			{
			    env_channel_specified = SMPD_TRUE;
			}
			else if (strcmp(env_val, "MPI_DLL_NAME") == 0)
			{
			    env_dll_specified = SMPD_TRUE;
			}
			else if (strcmp(env_val, "MPI_WRAP_DLL_NAME") == 0)
			{
			    env_wrap_dll_specified = SMPD_TRUE;
			}
		    }
		    MPIU_Free(env_key);
		    token = strtok(NULL, " ");
		}
		MPIU_Free(env_str);
	    }
	    /* log */
	    if (strcmp(info[j].key, "log") == 0)
	    {
		if (smpd_is_affirmative(info[j].val) || (strcmp(info[j].val, "1") == 0))
		{
		    MPIU_Str_add_string_arg(&cur_env_loc, &env_maxlen, "MPI_WRAP_DLL_NAME", "mpe");
		    env_wrap_dll_specified = SMPD_TRUE;
		}
	    }
	    /* wdir */
	    if ((strcmp(info[j].key, "wdir") == 0) || (strcmp(info[j].key, "dir") == 0))
	    {
		strcpy(node.dir, info[j].val);
		smpd_dbg_printf("wdir = %s\n", info[j].val);
	    }
	    /* map */
	    if (strcmp(info[j].key, "map") == 0)
	    {
		if (smpd_parse_map_string(info[j].val, &drive_map_list) != SMPD_SUCCESS)
		{
		    goto spawn_failed;
		}
	    }
	    /* localonly */
	    if (strcmp(info[j].key, "localonly") == 0)
	    {
		smpd_get_hostname(node.hostname, SMPD_MAX_HOST_LENGTH);
		if (smpd_get_host_id(node.hostname, &node.host_id) != SMPD_SUCCESS)
		{
		    node.hostname[0] = '\0';
                    node.alt_hostname[0] = '\0';
		    node.host_id = -1;
		}
	    }
	    /* machinefile */
	    if (strcmp(info[j].key, "machinefile") == 0)
	    {
		if (smpd_parse_machine_file(info[j].val))
		{
		    /*use_machine_file = SMPD_TRUE;*/
		}
	    }
	    /* configfile */
	    if (strcmp(info[j].key, "configfile") == 0)
	    {
	    }
	    /* file */
	    if (strcmp(info[j].key, "file") == 0)
	    {
	    }
	    /* priority */
	    if (strcmp(info[j].key, "priority") == 0)
	    {
		if (smpd_isnumbers_with_colon(info[j].val))
		{
		    char *str;
		    node.priority_class = atoi(info[j].val); /* This assumes atoi will stop at the colon and return a number */
		    str = strchr(info[j].val, ':');
		    if (str)
		    {
			str++;
			node.priority_thread = atoi(str);
		    }
		    if (node.priority_class < 0 || node.priority_class > 4 || node.priority_thread < 0 || node.priority_thread > 5)
		    {
			smpd_err_printf("Error: priorities must be between 0-4:0-5\n");
			node.priority_class = SMPD_DEFAULT_PRIORITY_CLASS;
			node.priority_thread = SMPD_DEFAULT_PRIORITY;
		    }
		}
	    }
	    /* timeout */
	    if (strcmp(info[j].key, "timeout") == 0)
	    {
	    }
	    /* exitcodes */
	    if (strcmp(info[j].key, "exitcodes") == 0)
	    {
		/* FIXME: This will turn on exit code printing for all processes.  Implement a new mechanism for only printing codes for an individual process group. */
		if (smpd_is_affirmative(info[j].val) || (strcmp(info[j].val, "1") == 0))
		{
		    smpd_process.output_exit_codes = SMPD_TRUE;
		}
	    }
	    /* nompi */
	    if (strcmp(info[j].key, "nompi") == 0)
	    {
		/* FIXME: Tell MPICH that the spawned processes will not make any SMPD calls, including SMPD_Init - so don't to a comm_accept or it will hang! */
	    }
	    /* etc */
	}

	/* Add special environment variables */
	if (env_channel_specified == SMPD_FALSE && env_dll_specified == SMPD_FALSE)
	{
	    if (smpd_process.env_dll[0] != '\0')
	    {
		MPIU_Str_add_string_arg(&cur_env_loc, &env_maxlen, "MPI_DLL_NAME", smpd_process.env_dll);
	    }
	    else if (smpd_process.env_channel[0] != '\0')
	    {
		    MPIU_Str_add_string_arg(&cur_env_loc, &env_maxlen, "MPICH2_CHANNEL", smpd_process.env_channel);
            if(smpd_process.env_netmod[0] != '\0')
            {
                MPIU_Str_add_string_arg(&cur_env_loc, &env_maxlen, "MPICH_NEMESIS_NETMOD", smpd_process.env_netmod);
            }
	    }
	}
	if (env_wrap_dll_specified == SMPD_FALSE)
	{
	    if (smpd_process.env_wrap_dll[0] != '\0')
	    {
		MPIU_Str_add_string_arg(&cur_env_loc, &env_maxlen, "MPI_WRAP_DLL_NAME", smpd_process.env_wrap_dll);
	    }
	}

	/* create launch nodes for the current command */
	for (j=0; j<maxprocs[i]; j++)
	{
	    if (launch_list == NULL)
	    {
		launch_list = (smpd_launch_node_t*)MPIU_Malloc(sizeof(smpd_launch_node_t));
		launch_iter = launch_list;
		if (launch_iter)
		{
		    launch_iter->prev = NULL;
		}
	    }
	    else
	    {
		launch_iter->next = (smpd_launch_node_t*)MPIU_Malloc(sizeof(smpd_launch_node_t));
		if (launch_iter->next)
		{
		    launch_iter->next->prev = launch_iter;
		    launch_iter = launch_iter->next;
		}
		else
		{
		    launch_iter = NULL;
		}
	    }
	    if (launch_iter == NULL)
	    {
		smpd_err_printf("unable to allocate a launch node structure for the %dth command.\n", cur_iproc);
		goto spawn_failed;
	    }
	    launch_iter->appnum = i;
	    launch_iter->iproc = cur_iproc++;
	    launch_iter->args[0] = '\0';
	    launch_iter->clique[0] = '\0';
	    /*launch_iter->dir[0] = '\0';*/
	    strcpy(launch_iter->dir, node.dir);
	    strcpy(launch_iter->env_data, node.env_data);
	    launch_iter->env = launch_iter->env_data;
	    launch_iter->exe[0] = '\0';
	    if (node.host_id != -1)
	    {
		launch_iter->host_id = node.host_id;
		strcpy(launch_iter->hostname, node.hostname);
		strcpy(launch_iter->alt_hostname, node.alt_hostname);
	    }
	    else
	    {
		launch_iter->host_id = -1;
		launch_iter->hostname[0] = '\0';
		launch_iter->alt_hostname[0] = '\0';
	    }
	    launch_iter->map_list = drive_map_list;
	    if (drive_map_list)
	    {
		drive_map_list->ref_count++;
	    }
	    launch_iter->priority_class = node.priority_class;
	    launch_iter->priority_thread = node.priority_thread;
	    launch_iter->next = NULL;

	    strcpy(launch_iter->exe, node.exe);
	    /*strcpy(launch_iter->args, node.args);*/
	    if (strlen(node.args) > 0)
	    {
		strncat(launch_iter->exe, " ", SMPD_MAX_EXE_LENGTH);
		strncat(launch_iter->exe, node.args, SMPD_MAX_EXE_LENGTH);
	    }
	    strcpy(launch_iter->path, node.path);
	}
    }
    if (info != NULL)
    {
	/* free the last round of infos */
	for (j=0; j<nkeyvals[i-1]; j++)
	{
	    MPIU_Free(info[j].key);
	    MPIU_Free(info[j].val);
	}
	MPIU_Free(info);
    }
    info = NULL;

    /* create a spawn context to save parameters, state, etc. */
    if (context->spawn_context != NULL)
    {
	MPIU_Free(context->spawn_context);
    }
    context->spawn_context = (smpd_spawn_context_t*)MPIU_Malloc(sizeof(smpd_spawn_context_t));
    if (context->spawn_context == NULL)
    {
	smpd_err_printf("unable to create a spawn context.\n");
	goto spawn_failed;
    }
    context->spawn_context->context = context;
    context->spawn_context->kvs_name[0] = '\0';
    context->spawn_context->launch_list = NULL;
    context->spawn_context->npreput = -1;
    context->spawn_context->num_outstanding_launch_cmds = -1;
    context->spawn_context->preput[0] = '\0';
    context->spawn_context->result_cmd = NULL;

    /* Get the keyval pairs to be put in the process group keyval space before the processes are launched. */
    if (MPIU_Str_get_int_arg(cmd->cmd, "npreput", &context->spawn_context->npreput) != MPIU_STR_SUCCESS)
    {
	smpd_err_printf("unable to get the npreput parameter from the spawn command '%s'.\n", cmd->cmd);
	goto spawn_failed;
    }
    /*printf("npreput = %d\n", context->spawn_context->npreput);fflush(stdout);*/
    if (context->spawn_context->npreput > 0 && MPIU_Str_get_string_arg(cmd->cmd, "preput", context->spawn_context->preput, SMPD_MAX_CMD_LENGTH) != MPIU_STR_SUCCESS)
    {
	smpd_err_printf("unablet to get the preput parameter from the spawn command '%s'.\n", cmd->cmd);
	goto spawn_failed;
    }
    MPIU_Free(maxprocs);
    MPIU_Free(nkeyvals);


    /* do the spawn stuff */

    /* count the number of processes to spawn */
    nproc = 0;
    launch_iter = launch_list;
    while (launch_iter)
    {
	nproc++;
	launch_iter = launch_iter->next;
    }

    /* create the host list and add nproc to the launch list */
    host_list = NULL;
    launch_iter = launch_list;
    while (launch_iter)
    {
	if (launch_iter->host_id == -1)
	{
	    smpd_get_next_host(&host_list, launch_iter);
	}
	if (launch_iter->alt_hostname[0] != '\0')
	{
	    if (smpd_append_env_option(launch_iter->env_data, SMPD_MAX_ENV_LENGTH, "MPICH_INTERFACE_HOSTNAME", launch_iter->alt_hostname) != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to add the MPICH_INTERFACE_HOSTNAME to the launch node environment block\n");
		goto spawn_failed;
	    }
	}
	launch_iter->nproc = nproc;
	launch_iter = launch_iter->next;
    }
    smpd_create_cliques(launch_list);

    /* connect up the new smpd hosts */
    context = smpd_process.left_context;
    /* save the launch list to be used after the new hosts are connected */
    context->spawn_context->launch_list = launch_list;
    context->spawn_context->num_outstanding_launch_cmds = 0;/*nproc;*/ /* this assumes all launch commands will be successfully posted. */
    smpd_fix_up_host_tree(smpd_process.host_list);

    {
	SMPD_BOOL first = SMPD_TRUE;
	host_iter = smpd_process.host_list;
	while (host_iter)
	{
	    if (host_iter->connected)
	    {
		if (host_iter->left != NULL && !host_iter->left->connected)
		{
		    context->connect_to = host_iter->left;

		    /* create a connect command to be sent to the parent */
		    result = smpd_create_command("connect", 0, context->connect_to->parent, SMPD_TRUE, &cmd);
		    if (result != SMPD_SUCCESS)
		    {
			smpd_err_printf("unable to create a connect command.\n");
			goto spawn_failed;
		    }
		    context->connect_to->connect_cmd_tag = cmd->tag;
		    result = smpd_add_command_arg(cmd, "host", context->connect_to->host);
		    if (result != SMPD_SUCCESS)
		    {
			smpd_err_printf("unable to add the host parameter to the connect command for host %s\n", context->connect_to->host);
			goto spawn_failed;
		    }
		    result = smpd_add_command_int_arg(cmd, "id", context->connect_to->id);
		    if (result != SMPD_SUCCESS)
		    {
			smpd_err_printf("unable to add the id parameter to the connect command for host %s\n", context->connect_to->host);
			goto spawn_failed;
		    }
		    if (smpd_process.plaintext)
		    {
			/* propagate the plaintext option to the manager doing the connect */
			result = smpd_add_command_arg(cmd, "plaintext", "yes");
			if (result != SMPD_SUCCESS)
			{
			    smpd_err_printf("unable to add the plaintext parameter to the connect command for host %s\n", context->connect_to->host);
			    goto spawn_failed;
			}
		    }

		    smpd_dbg_printf("sending connect command to add new hosts for the spawn command.\n");
		    /*printf("sending first connect command to add new hosts for the spawn command.\n");fflush(stdout);*/
		    /* post a write of the command */
		    result = smpd_post_write_command(context, cmd);
		    if (result != SMPD_SUCCESS)
		    {
			smpd_err_printf("unable to post a write of the connect command.\n");
			goto spawn_failed;
		    }

		    if (first)
		    {
			context->spawn_context->result_cmd = temp_cmd;
			first = SMPD_FALSE;
		    }
		}
		if (host_iter->right != NULL && !host_iter->right->connected)
		{
		    context->connect_to = host_iter->right;

		    /* create a connect command to be sent to the parent */
		    result = smpd_create_command("connect", 0, context->connect_to->parent, SMPD_TRUE, &cmd);
		    if (result != SMPD_SUCCESS)
		    {
			smpd_err_printf("unable to create a connect command.\n");
			goto spawn_failed;
		    }
		    context->connect_to->connect_cmd_tag = cmd->tag;
		    result = smpd_add_command_arg(cmd, "host", context->connect_to->host);
		    if (result != SMPD_SUCCESS)
		    {
			smpd_err_printf("unable to add the host parameter to the connect command for host %s\n", context->connect_to->host);
			goto spawn_failed;
		    }
		    result = smpd_add_command_int_arg(cmd, "id", context->connect_to->id);
		    if (result != SMPD_SUCCESS)
		    {
			smpd_err_printf("unable to add the id parameter to the connect command for host %s\n", context->connect_to->host);
			goto spawn_failed;
		    }
		    if (smpd_process.plaintext)
		    {
			/* propagate the plaintext option to the manager doing the connect */
			result = smpd_add_command_arg(cmd, "plaintext", "yes");
			if (result != SMPD_SUCCESS)
			{
			    smpd_err_printf("unable to add the plaintext parameter to the connect command for host %s\n", context->connect_to->host);
			    goto spawn_failed;
			}
		    }

		    smpd_dbg_printf("sending connect command to add new hosts for the spawn command.\n");
		    /*printf("sending first connect command to add new hosts for the spawn command.\n");fflush(stdout);*/
		    /* post a write of the command */
		    result = smpd_post_write_command(context, cmd);
		    if (result != SMPD_SUCCESS)
		    {
			smpd_err_printf("unable to post a write of the connect command.\n");
			goto spawn_failed;
		    }

		    if (first)
		    {
			context->spawn_context->result_cmd = temp_cmd;
			first = SMPD_FALSE;
		    }
		}
	    }
	    host_iter = host_iter->next;
	}

	if (!first)
	{
	    /* At least one connect command was issued so return here */
	    smpd_exit_fn(FCNAME);
	    return SMPD_SUCCESS;
	}
    }

    context->spawn_context->result_cmd = temp_cmd;

    if (launch_list == NULL)
    {
	smpd_process.spawning = SMPD_FALSE;

	/* spawn command received for zero processes, return a success result immediately */
	result = smpd_add_command_arg(context->spawn_context->result_cmd, "result", SMPD_SUCCESS_STR);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to add the result string to the result command.\n");
	    goto spawn_failed;
	}
	/* send the spawn result command */
	result = smpd_post_write_command(context, context->spawn_context->result_cmd);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to post a write of the spawn result command.\n");
	    goto spawn_failed;
	}
	smpd_exit_fn(FCNAME);
	return SMPD_SUCCESS;
    }

    /* create the new kvs space */
    smpd_dbg_printf("all hosts needed for the spawn command are available, sending start_dbs command.\n");
    /*printf("all hosts needed for the spawn command are available, sending start_dbs command.\n");fflush(stdout);*/
    /* create the start_dbs command to be sent to the first host */
    result = smpd_create_command("start_dbs", 0, 1, SMPD_TRUE, &cmd);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to create a start_dbs command.\n");
	goto spawn_failed;
    }

    result = smpd_add_command_int_arg(cmd, "npreput", context->spawn_context->npreput);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the npreput value to the start_dbs command for a spawn command.\n");
	goto spawn_failed;
    }

    result = smpd_add_command_arg(cmd, "preput", context->spawn_context->preput);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the preput keyvals to the start_dbs command for a spawn command.\n");
	goto spawn_failed;
    }

    /* post a write of the command */
    result = smpd_post_write_command(context, cmd);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to post a write of the start_dbs command.\n");
	goto spawn_failed;
    }

    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
    /* send the launch commands */

/*
    printf("host tree:\n");
    host_iter = smpd_process.host_list;
    if (!host_iter)
	printf("<none>\n");
    while (host_iter)
    {
	printf(" host: %s, parent: %d, id: %d, connected: %s\n",
	    host_iter->host,
	    host_iter->parent, host_iter->id,
	    host_iter->connected ? "yes" : "no");
	host_iter = host_iter->next;
    }

    printf("launch nodes:\n");
    launch_iter = launch_list;
    if (!launch_iter)
	printf("<none>\n");
    while (launch_iter)
    {
	printf(" launch_node:\n");
	printf("  id  : %d\n", launch_iter->host_id);
	printf("  rank: %d\n", launch_iter->iproc);
	printf("  size: %d\n", launch_iter->nproc);
	printf("  clique: %s\n", launch_iter->clique);
	printf("  exe : %s\n", launch_iter->exe);
	if (launch_iter->args[0] != '\0')
	    printf("  args: %s\n", launch_iter->args);
	if (launch_iter->path[0] != '\0')
	    printf("  path: %s\n", launch_iter->path);
	launch_temp = launch_iter;
	launch_iter = launch_iter->next;
	MPIU_Free(launch_temp);
    }
    fflush(stdout);
*/

spawn_failed:

    smpd_process.spawning = SMPD_FALSE;

    /* add the result */
    result = smpd_add_command_arg(temp_cmd, "result", SMPD_FAIL_STR);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the result string to the result command for a spawn command.\n");
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    /* send result back */
    smpd_dbg_printf("replying to spawn command: \"%s\"\n", temp_cmd->cmd);
    result = smpd_post_write_command(context, temp_cmd);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to post a write of the result command to the context.\n");
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    smpd_exit_fn(FCNAME);
    return result;
}

#undef FCNAME
#define FCNAME "smpd_delayed_spawn_enqueue"
int smpd_delayed_spawn_enqueue(smpd_context_t *context)
{
    smpd_delayed_spawn_node_t *iter, *node;

    smpd_enter_fn(FCNAME);

    node = (smpd_delayed_spawn_node_t*)MPIU_Malloc(sizeof(smpd_delayed_spawn_node_t));
    if (node == NULL)
    {
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    node->next = NULL;
    node->context = context;
    node->cmd = context->read_cmd;
 
    iter = smpd_process.delayed_spawn_queue;
    if (iter == NULL)
    {
	smpd_process.delayed_spawn_queue = node;
	smpd_exit_fn(FCNAME);
	return SMPD_SUCCESS;
    }
    while (iter->next != NULL)
    {
	iter = iter->next;
    }
    iter->next = node;
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_delayed_spawn_dequeue"
int smpd_delayed_spawn_dequeue(smpd_context_t **context_pptr)
{
    smpd_delayed_spawn_node_t *node;

    smpd_enter_fn(FCNAME);
    if (smpd_process.delayed_spawn_queue != NULL)
    {
	*context_pptr = smpd_process.delayed_spawn_queue->context;
	/* Copy the command in the queue to the context read command
	 * restoring the context to the state it was when it was enqueued.
	 */
	(*context_pptr)->read_cmd = smpd_process.delayed_spawn_queue->cmd;
	node = smpd_process.delayed_spawn_queue;
	smpd_process.delayed_spawn_queue = smpd_process.delayed_spawn_queue->next;
	MPIU_Free(node);
    }
    else
    {
	*context_pptr = NULL;
    }
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_handle_delayed_spawn_command"
int smpd_handle_delayed_spawn_command(void)
{
    smpd_context_t *context = NULL;
    int result = SMPD_SUCCESS;

    smpd_enter_fn(FCNAME);

    /* Handle delayed spawn commands until a spawn is in progress or the queue is empty */
    while (smpd_process.spawning == SMPD_FALSE && smpd_process.delayed_spawn_queue != NULL)
    {
	result = smpd_delayed_spawn_dequeue(&context);
	if (result != SMPD_SUCCESS)
	{
	    smpd_exit_fn(FCNAME);
	    return result;
	}
	if (context != NULL)
	{
	    result = smpd_handle_spawn_command(context);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_exit_fn(FCNAME);
		return result;
	    }
	}
    }

    smpd_exit_fn(FCNAME);
    return result;
}
