/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "smpd.h"

#undef FCNAME
#define FCNAME "smpd_handle_barrier_command"
int smpd_handle_barrier_command(smpd_context_t *context)
{
    int result;
    smpd_command_t *cmd, *temp_cmd;
    char name[SMPD_MAX_DBS_NAME_LEN];
    int count;
    char ctx_key[100];
    char value[100];
    smpd_barrier_node_t *iter, *trailer;
    int i;

    smpd_enter_fn(FCNAME);

    cmd = &context->read_cmd;

    if (MPIU_Str_get_string_arg(cmd->cmd, "name", name, SMPD_MAX_DBS_NAME_LEN) != MPIU_STR_SUCCESS)
    {
	smpd_err_printf("no name in the barrier command: '%s'\n", cmd->cmd);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    if (MPIU_Str_get_string_arg(cmd->cmd, "value", value, 100) != MPIU_STR_SUCCESS)
    {
	smpd_err_printf("no count in the barrier command: '%s'\n", cmd->cmd);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    count = atoi(value);
    if (count < 1)
    {
	smpd_err_printf("invalid count in the barrier command: '%s'\n", cmd->cmd);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    if (MPIU_Str_get_string_arg(cmd->cmd, "ctx_key", ctx_key, 100) != MPIU_STR_SUCCESS)
    {
	smpd_err_printf("no ctx_key in the barrier command: '%s'\n", cmd->cmd);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    if (count == 1)
    {
	/* send a success result immediately */
    }

    trailer = iter = smpd_process.barrier_list;
    while (iter)
    {
	if (strcmp(name, iter->name) == 0)
	{
	    iter->in_array[iter->in].context = context;
	    iter->in_array[iter->in].dest = cmd->src;
	    iter->in_array[iter->in].cmd_tag = cmd->tag;
	    strcpy(iter->in_array[iter->in].ctx_key, ctx_key);

	    smpd_dbg_printf("incrementing barrier(%s) incount from %d to %d out of %d\n", iter->name, iter->in, iter->in+1, iter->count);
	    iter->in++;
	    if (iter->in >= iter->count)
	    {
		smpd_dbg_printf("all in barrier, sending result back to all participators.\n");
		/* send all the results */
		for (i=0; i<iter->count; i++)
		{
		    /* prepare the result command */
		    result = smpd_create_command("result", smpd_process.id, iter->in_array[i].dest, SMPD_FALSE, &temp_cmd);
		    if (result != SMPD_SUCCESS)
		    {
			smpd_err_printf("unable to create a result command for the barrier command.\n");
			smpd_exit_fn(FCNAME);
			return SMPD_FAIL;
		    }
		    /* add the command tag for result matching */
		    result = smpd_add_command_int_arg(temp_cmd, "cmd_tag", iter->in_array[i].cmd_tag);
		    if (result != SMPD_SUCCESS)
		    {
			smpd_err_printf("unable to add the tag to the result command for barrier command\n");
			smpd_exit_fn(FCNAME);
			return SMPD_FAIL;
		    }
		    result = smpd_add_command_arg(temp_cmd, "cmd_orig", "barrier");
		    if (result != SMPD_SUCCESS)
		    {
			smpd_err_printf("unable to add cmd_orig to the result command for a barrier command\n");
			smpd_exit_fn(FCNAME);
			return SMPD_FAIL;
		    }
		    /* add the ctx_key for control channel matching */
		    result = smpd_add_command_arg(temp_cmd, "ctx_key", iter->in_array[i].ctx_key);
		    if (result != SMPD_SUCCESS)
		    {
			smpd_err_printf("unable to add the ctx_key to the result command for dbs command.\n");
			smpd_exit_fn(FCNAME);
			return SMPD_FAIL;
		    }
		    /* add the result string */
		    smpd_dbg_printf("sending reply to barrier command '%s'.\n", iter->name);
		    result = smpd_add_command_arg(temp_cmd, "result", DBS_SUCCESS_STR);
		    if (result != SMPD_SUCCESS)
		    {
			smpd_err_printf("unable to add the result string to the result command for barrier command '%s'.\n", iter->name);
			smpd_exit_fn(FCNAME);
			return SMPD_FAIL;
		    }
		    smpd_dbg_printf("sending result command to %s context: \"%s\"\n", smpd_get_context_str(context), temp_cmd->cmd);
		    result = smpd_post_write_command(iter->in_array[i].context, temp_cmd);
		    if (result != SMPD_SUCCESS)
		    {
			smpd_err_printf("unable to post a write of the result command to the %s context: cmd '%s', barrier '%s'",
			    smpd_get_context_str(iter->in_array[i].context), temp_cmd->cmd, iter->name);
			smpd_exit_fn(FCNAME);
			return SMPD_FAIL;
		    }
		}

		/* free the barrier node */
		if (iter == smpd_process.barrier_list)
		{
		    smpd_process.barrier_list = smpd_process.barrier_list->next;
		}
		else
		{
		    trailer->next = iter->next;
		}
		/* zero out to help catch access after free errors */
		memset(iter->in_array, 0, sizeof(smpd_barrier_in_t) * iter->count);
		memset(iter, 0, sizeof(smpd_barrier_node_t));
		MPIU_Free(iter->in_array);
		MPIU_Free(iter);
	    }
	    smpd_exit_fn(FCNAME);
	    return SMPD_SUCCESS;
	}
	if (trailer != iter)
	    trailer = trailer->next;
	iter = iter->next;
    }

    /* this is the first guy in so create a new barrier structure and add it to the list */
    smpd_dbg_printf("initializing barrier(%s): in=1 size=%d\n", name, count);
    iter = (smpd_barrier_node_t*)MPIU_Malloc(sizeof(smpd_barrier_node_t));
    if (iter == NULL)
    {
	smpd_err_printf("unable to allocate a barrier node.\n");
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    strcpy(iter->name, name);
    iter->count = count;
    iter->in = 1;
    iter->in_array = (smpd_barrier_in_t*)MPIU_Malloc(count * sizeof(smpd_barrier_in_t));
    if (iter->in_array == NULL)
    {
	MPIU_Free(iter);
	smpd_err_printf("unable to allocate a barrier in array of size %d\n", count);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    iter->in_array[0].context = context;
    iter->in_array[0].dest = cmd->src;
    iter->in_array[0].cmd_tag = cmd->tag;
    strcpy(iter->in_array[0].ctx_key, ctx_key);

    iter->next = smpd_process.barrier_list;
    smpd_process.barrier_list = iter;

    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}
