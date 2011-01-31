/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "smpd.h"
#ifdef HAVE_STDARG_H
#include <stdarg.h>
#endif
#ifdef HAVE_WINDOWS_H
#include "smpd_service.h"
#include <Ntdsapi.h>
#endif

/* prototype local functions */
static int write_to_stdout(const char *buffer, size_t num_bytes);
static int write_to_stderr(const char *buffer, size_t num_bytes);
static int smpd_do_abort_job(char *name, int rank, char *error_str, int exit_code);
static int create_process_group(int nproc, char *kvs_name, smpd_process_group_t **pg_pptr);
static int get_name_key_value(char *str, char *name, char *key, char *value);


/* FIXME: smpd_get_hostname() does the same thing */
#undef FCNAME
#define FCNAME "get_hostname"
static int get_hostname(char *host, int hostlen){
    int retval = SMPD_SUCCESS;
#ifdef HAVE_WINDOWS_H	
    DWORD len = hostlen;
#endif	
    smpd_enter_fn(FCNAME);
    if(host == NULL){
        smpd_err_printf("host == NULL\n");
        smpd_exit_fn(FCNAME);
        return SMPD_FAIL;        
    }
#ifdef HAVE_WINDOWS_H
    if(!GetComputerNameEx(ComputerNameDnsFullyQualified, host, &len)){
        smpd_err_printf("GetComputerNameEx failed, error = %d\n", GetLastError());
        smpd_exit_fn(FCNAME);
        return SMPD_FAIL;
    }
#else
    if(gethostname(host, hostlen)){
        smpd_err_printf("gethostname failed, error = %d\n", errno);
        smpd_exit_fn(FCNAME);
        return SMPD_FAIL;
    }
#endif    
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_do_abort_job"
static int smpd_do_abort_job(char *name, int rank, char *error_str, int exit_code)
{
    int result;
    smpd_command_t *cmd_ptr;
    smpd_process_group_t *pg;
    int i;

    smpd_enter_fn(FCNAME);
    pg = smpd_process.pg_list;
    while (pg)
    {
	if (strcmp(pg->kvs, name) == 0)
	{
	    if (rank >= pg->num_procs)
	    {
		smpd_err_printf("invalid abort_job command - rank %d out of range, number of processes = %d", rank, pg->num_procs);
		smpd_exit_fn(FCNAME);
		return SMPD_FAIL;
	    }
	    break;
	}
	pg = pg->next;
    }
    if (pg == NULL)
    {
	smpd_err_printf("no process group structure found to match the abort_job command: pg <%s>\n", name);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    /* save the abort message */
    pg->processes[rank].errmsg = MPIU_Strdup(error_str);
    if (pg->aborted == SMPD_TRUE)
    {
	smpd_dbg_printf("job already aborted.\n");
	smpd_exit_fn(FCNAME);
	return SMPD_SUCCESS;
    }

    smpd_process.use_abort_exit_code = SMPD_TRUE;
    smpd_process.abort_exit_code = exit_code;

    pg->aborted = SMPD_TRUE;
    pg->num_pending_suspends = 0;
    for (i=0; i<pg->num_procs; i++)
    {
	if (!pg->processes[i].exited)
	{
	    /* create the suspend command */
	    result = smpd_create_command("suspend", 0, pg->processes[i].node_id, SMPD_TRUE, &cmd_ptr);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to create a suspend command.\n");
		goto do_abort_failure;
	    }
	    result = smpd_add_command_arg(cmd_ptr, "name", name);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to add the kvs name to the suspend command: '%s'\n", name);
		goto do_abort_failure;
	    }
	    result = smpd_add_command_int_arg(cmd_ptr, "rank", i);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to add the rank %d to the suspend command\n", i);
		goto do_abort_failure;
	    }
	    result = smpd_add_command_int_arg(cmd_ptr, "exit_code", exit_code);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to add the exit_code %d to the suspend command\n", exit_code);
		goto do_abort_failure;
	    }
	    if (pg->processes[i].ctx_key[0] != '\0')
	    {
		result = smpd_add_command_arg(cmd_ptr, "ctx_key", pg->processes[i].ctx_key);
		if (result != SMPD_SUCCESS)
		{
		    smpd_err_printf("unable to add the ctx_key to the suspend command: '%s'\n", pg->processes[i].ctx_key);
		    goto do_abort_failure;
		}

		/* send the suspend command */
		result = smpd_post_write_command(smpd_process.left_context, cmd_ptr);
		if (result != SMPD_SUCCESS)
		{
		    smpd_err_printf("unable to post a write for the suspend command: rank %d\n", rank);
		    goto do_abort_failure;
		}
	    }
	    else
	    {
		pg->processes[i].suspend_cmd = cmd_ptr;
	    }
	    pg->num_pending_suspends++;
	}
    }

    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
do_abort_failure:
    smpd_exit_fn(FCNAME);
    return SMPD_FAIL;
}

#undef FCNAME
#define FCNAME "smpd_abort_job"
int smpd_abort_job(char *name, int rank, char *fmt, ...)
{
    int result;
    char error_str[2048] = "";
    smpd_command_t *cmd_ptr;
    smpd_context_t *context;
    va_list list;

    smpd_enter_fn(FCNAME);

    va_start(list, fmt);
    vsnprintf(error_str, 2048, fmt, list);
    va_end(list);

    smpd_command_destination(0, &context);
    if (context == NULL)
    {
	result = smpd_do_abort_job(name, rank, error_str, 123);
	smpd_exit_fn(FCNAME);
	return result;
    }

    result = smpd_create_command("abort_job", smpd_process.id, 0, SMPD_FALSE, &cmd_ptr);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to create an abort_job command.\n");
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    result = smpd_add_command_arg(cmd_ptr, "name", name);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("Unable to add the job name to the abort_job command.\n");
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    result = smpd_add_command_int_arg(cmd_ptr, "rank", rank);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("Unable to add the rank to the abort_job command.\n");
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    result = smpd_add_command_arg(cmd_ptr, "error", error_str);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("Unable to add the error string to the abort_job command.\n");
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    smpd_dbg_printf("sending abort_job command to %s context: \"%s\"\n", smpd_get_context_str(context), cmd_ptr->cmd);
    result = smpd_post_write_command(context, cmd_ptr);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to post a write of the abort_job command to the %s context.\n", smpd_get_context_str(context));
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_handle_stdin_command"
int smpd_handle_stdin_command(smpd_context_t *context)
{
    char data[SMPD_MAX_STDOUT_LENGTH];
    smpd_command_t *cmd;
    smpd_process_t *piter;
    smpd_stdin_write_node_t *node, *iter;
    int result;
    SMPDU_Size_t num_written, num_decoded;
    int nd;

    smpd_enter_fn(FCNAME);

    cmd = &context->read_cmd;
    if (MPIU_Str_get_string_arg(cmd->cmd, "data", data, SMPD_MAX_STDOUT_LENGTH) == MPIU_STR_SUCCESS)
    {
	smpd_decode_buffer(data, data, SMPD_MAX_STDOUT_LENGTH, &nd);
	num_decoded = nd;
	/*printf("[%d]", rank);*/
	piter = smpd_process.process_list;
	while (piter)
	{
	    if (piter->rank == 0 || smpd_process.stdin_toall)
	    {
		if (piter->stdin_write_list != NULL)
		{
		    node = (smpd_stdin_write_node_t*)MPIU_Malloc(sizeof(smpd_stdin_write_node_t));
		    if (node == NULL)
			smpd_write(piter->in->sock, data, num_decoded);
		    else
		    {
			node->buffer = (char*)MPIU_Malloc(num_decoded);
			if (node->buffer == NULL)
			{
			    MPIU_Free(node);
			    smpd_write(piter->in->sock, data, num_decoded);
			}
			else
			{
			    /* add the node to the end of the write list */
			    node->length = num_decoded;
			    memcpy(node->buffer, data, num_decoded);
			    node->next = NULL;
			    iter = piter->stdin_write_list;
			    while (iter->next != NULL)
				iter = iter->next;
			    iter->next = node;
			}
		    }
		}
		else
		{
		    /* attempt to write the data immediately */
		    num_written = 0;
		    result = SMPDU_Sock_write(piter->in->sock, data, num_decoded, &num_written);
		    if (result != SMPD_SUCCESS)
		    {
			smpd_err_printf("unable to write data to the stdin context of process %d\n", piter->rank);
		    }
		    else
		    {
			if (num_written != num_decoded)
			{
			    /* If not all the data is written, copy it to a temp buffer and post a write for the remaining data */
			    node = (smpd_stdin_write_node_t*)MPIU_Malloc(sizeof(smpd_stdin_write_node_t));
			    if (node == NULL)
				smpd_write(piter->in->sock, &data[num_written], num_decoded-num_written);
			    else
			    {
				node->buffer = (char*)MPIU_Malloc(num_decoded-num_written);
				if (node->buffer == NULL)
				{
				    MPIU_Free(node);
				    smpd_write(piter->in->sock, &data[num_written], num_decoded-num_written);
				}
				else
				{
				    /* add the node to write list */
				    node->length = num_decoded - num_written;
				    memcpy(node->buffer, &data[num_written], num_decoded-num_written);
				    node->next = NULL;
				    piter->stdin_write_list = node;
				    piter->in->write_state = SMPD_WRITING_DATA_TO_STDIN;
				    result = SMPDU_Sock_post_write(piter->in->sock, node->buffer, node->length, node->length, NULL);
				    if (result != SMPD_SUCCESS)
				    {
					smpd_err_printf("unable to post a write of %d bytes to stdin for rank %d\n",
					    node->length, piter->rank);
				    }
				}
			    }
			}
		    }
		}
	    }
	    piter = piter->next;
	}
    }
    else
    {
	smpd_err_printf("unable to get the data from the stdin command: '%s'\n", cmd->cmd);
    }

    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_handle_close_stdin_command"
int smpd_handle_close_stdin_command(smpd_context_t *context)
{
    smpd_process_t *piter;
    int result;

    smpd_enter_fn(FCNAME);
    SMPD_UNREFERENCED_ARG(context);

    piter = smpd_process.process_list;
    while (piter)
    {
	if ((piter->rank == 0 || smpd_process.stdin_toall) && piter->in != NULL)
	{
	    piter->in->state = SMPD_CLOSING;
	    piter->in->process = NULL; /* NULL this out so the closing of the stdin context won't access it after it has been freed */
	    result = SMPDU_Sock_post_close(piter->in->sock);
	    if (result == SMPD_SUCCESS)
	    {
		piter->in = NULL;
	    }
	}
	piter = piter->next;
    }

    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "write_to_stdout"
static int write_to_stdout(const char *buffer, size_t num_bytes)
{
#ifdef HAVE_WINDOWS_H
    HANDLE hStdout;
    DWORD num_written;

    smpd_enter_fn(FCNAME);
    /* MT - This should acquire the hLaunchProcessMutex so that it doesn't acquire a redirected handle. */
    /* But since the code is not multi-threaded yet this doesn't matter */
    hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
    WriteFile(hStdout, buffer, num_bytes, &num_written, NULL);
#else
    smpd_enter_fn(FCNAME);
    fwrite(buffer, 1, num_bytes, stdout);
    fflush(stdout);
#endif
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_handle_stdout_command"
int smpd_handle_stdout_command(smpd_context_t *context)
{
    int rank;
    char data[SMPD_MAX_STDOUT_LENGTH];
    smpd_command_t *cmd;
    int num_decoded = 0;
    int first;
    char prefix[20];
    size_t prefix_length;
    char *token;
    SMPD_BOOL ends_in_cr = SMPD_FALSE;

    smpd_enter_fn(FCNAME);

    cmd = &context->read_cmd;
    if (MPIU_Str_get_int_arg(cmd->cmd, "rank", &rank) != MPIU_STR_SUCCESS)
    {
	rank = -1;
	smpd_err_printf("no rank in the stdout command: '%s'\n", cmd->cmd);
    }
    if (MPIU_Str_get_int_arg(cmd->cmd, "first", &first) != MPIU_STR_SUCCESS)
    {
	first = 0;
	smpd_err_printf("no first flag in the stdout command: '%s'\n", cmd->cmd);
    }
    if (MPIU_Str_get_string_arg(cmd->cmd, "data", data, SMPD_MAX_STDOUT_LENGTH) == MPIU_STR_SUCCESS)
    {
	smpd_decode_buffer(data, data, SMPD_MAX_STDOUT_LENGTH, &num_decoded);
	data[num_decoded] = '\0';
	if (data[num_decoded-1] == '\n')
	{
	    ends_in_cr = SMPD_TRUE;
	}
	/*printf("[%d]", rank);*/
	if (smpd_process.prefix_output == SMPD_TRUE)
	{
	    if (first)
	    {
		MPIU_Snprintf(prefix, 20, "[%d]", rank);
		prefix_length = strlen(prefix);
		write_to_stdout(prefix, prefix_length);
	    }
	    MPIU_Snprintf(prefix, 20, "\n[%d]", rank);
	    prefix_length = strlen(prefix);
	    token = strtok(data, "\r\n");
	    while (token != NULL)
	    {
		write_to_stdout(token, strlen(token));
		token = strtok(NULL, "\r\n");
		if (token != NULL)
		{
		    write_to_stdout(prefix, prefix_length);
		}
		else
		{
		    if (ends_in_cr == SMPD_TRUE)
		    {
			write_to_stdout("\n", 1);
		    }
		}
	    }
	}
	else
	{
	    write_to_stdout(data, num_decoded);
	}
    }
    else
    {
	smpd_err_printf("unable to get the data from the stdout command: '%s'\n", cmd->cmd);
    }

    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "write_to_stderr"
static int write_to_stderr(const char *buffer, size_t num_bytes)
{
#ifdef HAVE_WINDOWS_H
    HANDLE hStderr;
    DWORD num_written;

    smpd_enter_fn(FCNAME);
    /* MT - This should acquire the hLaunchProcessMutex so that it doesn't acquire a redirected handle. */
    /* But since the code is not multi-threaded yet this doesn't matter */
    hStderr = GetStdHandle(STD_ERROR_HANDLE);
    WriteFile(hStderr, buffer, num_bytes, &num_written, NULL);
#else
    smpd_enter_fn(FCNAME);
    fwrite(buffer, 1, num_bytes, stderr);
    fflush(stdout);
#endif
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_handle_stderr_command"
int smpd_handle_stderr_command(smpd_context_t *context)
{
    int rank;
    char data[SMPD_MAX_STDOUT_LENGTH];
    smpd_command_t *cmd;
    int num_decoded = 0;
    int first;
    char prefix[20];
    size_t prefix_length;
    char *token;
    SMPD_BOOL ends_in_cr = SMPD_FALSE;

    smpd_enter_fn(FCNAME);

    cmd = &context->read_cmd;
    if (MPIU_Str_get_int_arg(cmd->cmd, "rank", &rank) != MPIU_STR_SUCCESS)
    {
	rank = -1;
	smpd_err_printf("no rank in the stderr command: '%s'\n", cmd->cmd);
    }
    if (MPIU_Str_get_int_arg(cmd->cmd, "first", &first) != MPIU_STR_SUCCESS)
    {
	first = 0;
	smpd_err_printf("no first flag in the stderr command: '%s'\n", cmd->cmd);
    }
    if (MPIU_Str_get_string_arg(cmd->cmd, "data", data, SMPD_MAX_STDOUT_LENGTH) == MPIU_STR_SUCCESS)
    {
	smpd_decode_buffer(data, data, SMPD_MAX_STDOUT_LENGTH, &num_decoded);
	data[num_decoded] = '\0';
	if (data[num_decoded-1] == '\n')
	{
	    ends_in_cr = SMPD_TRUE;
	}
	/*fprintf(stderr, "[%d]", rank);*/
	if (smpd_process.prefix_output == SMPD_TRUE)
	{
	    if (first)
	    {
		MPIU_Snprintf(prefix, 20, "[%d]", rank);
		prefix_length = strlen(prefix);
		write_to_stderr(prefix, prefix_length);
	    }
	    MPIU_Snprintf(prefix, 20, "\n[%d]", rank);
	    prefix_length = strlen(prefix);
	    token = strtok(data, "\r\n");
	    while (token != NULL)
	    {
		write_to_stderr(token, strlen(token));
		token = strtok(NULL, "\r\n");
		if (token != NULL)
		{
		    write_to_stderr(prefix, prefix_length);
		}
		else
		{
		    write_to_stderr("\n", 1);
		}
	    }
	}
	else
	{
	    write_to_stderr(data, num_decoded);
	}
    }
    else
    {
	smpd_err_printf("unable to get the data from the stderr command: '%s'\n", cmd->cmd);
    }

    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "create_process_group"
static int create_process_group(int nproc, char *kvs_name, smpd_process_group_t **pg_pptr)
{
    int i;
    smpd_process_group_t *pg;

    smpd_enter_fn(FCNAME);

    /* initialize a new process group structure */
    pg = (smpd_process_group_t*)MPIU_Malloc(sizeof(smpd_process_group_t));
    if (pg == NULL)
    {
	smpd_err_printf("unable to allocate memory for a process group structure.\n");
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    pg->aborted = SMPD_FALSE;
    pg->any_init_received = SMPD_FALSE;
    pg->any_noinit_process_exited = SMPD_FALSE;
    strncpy(pg->kvs, kvs_name, SMPD_MAX_DBS_NAME_LEN);
    pg->num_procs = nproc;
    pg->processes = (smpd_exit_process_t*)MPIU_Malloc(nproc * sizeof(smpd_exit_process_t));
    if (pg->processes == NULL)
    {
	smpd_err_printf("unable to allocate an array of %d process exit structures.\n", nproc);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    for (i=0; i<nproc; i++)
    {
	pg->processes[i].ctx_key[0] = '\0';
	pg->processes[i].errmsg = NULL;
	pg->processes[i].exitcode = -1;
	pg->processes[i].exited = SMPD_FALSE;
	pg->processes[i].finalize_called = SMPD_FALSE;
	pg->processes[i].init_called = SMPD_FALSE;
	pg->processes[i].suspended = SMPD_FALSE;
	pg->processes[i].suspend_cmd = NULL;
    }
    /* add the process group to the global list */
    pg->next = smpd_process.pg_list;
    smpd_process.pg_list = pg;
    *pg_pptr = pg;
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_launch_processes"
int smpd_launch_processes(smpd_launch_node_t *launch_list, char *kvs_name, char *domain_name, smpd_spawn_context_t *spawn_context)
{
    int result;
    smpd_command_t *cmd_ptr;
    smpd_launch_node_t *launch_node_ptr, *launch_iter;
    smpd_map_drive_node_t *map_iter;
    char drive_arg_str[20];
    char drive_map_str[SMPD_MAX_EXE_LENGTH];
    smpd_process_group_t *pg;
    int i;

    smpd_enter_fn(FCNAME);

    launch_node_ptr = launch_list;
    smpd_dbg_printf("creating a process group of size %d on node %d called %s\n", launch_node_ptr->nproc, smpd_process.id, kvs_name);
    result = create_process_group(launch_list->nproc, kvs_name, &pg);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to create a process group.\n");
	goto launch_failure;
    }
    launch_iter = launch_node_ptr;
    for (i=0; i<launch_node_ptr->nproc; i++)
    {
	if (launch_iter == NULL)
	{
	    smpd_err_printf("number of launch nodes does not match number of processes: %d < %d\n", i, launch_node_ptr->nproc);
	    goto launch_failure;
	} 
	pg->processes[launch_iter->iproc].node_id = launch_iter->host_id;
	MPIU_Strncpy(pg->processes[launch_iter->iproc].host, launch_iter->hostname, SMPD_MAX_HOST_LENGTH);
	launch_iter = launch_iter->next;
    }

    /* launch the processes */
    smpd_dbg_printf("launching the processes.\n");
    launch_node_ptr = launch_list;
    while (launch_node_ptr)
    {
	/* create the launch command */
	result = smpd_create_command("launch", 0, 
	    launch_node_ptr->host_id,
	    SMPD_TRUE, &cmd_ptr);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to create a launch command.\n");
	    goto launch_failure;
	}
	result = smpd_add_command_arg(cmd_ptr, "c", launch_node_ptr->exe);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to add the command line to the launch command: '%s'\n", launch_node_ptr->exe);
	    goto launch_failure;
	}
	result = smpd_add_command_int_arg(cmd_ptr, "s", spawn_context ? 1 : 0);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to add the spawn flag to the launch command: '%s'\n", launch_node_ptr->exe);
	    goto launch_failure;
	}
	result = smpd_add_command_int_arg(cmd_ptr, "a", launch_node_ptr->appnum);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to add the application number to the launch command: '%s'\n", launch_node_ptr->exe);
	    goto launch_failure;
	}
	if (launch_node_ptr->env[0] != '\0')
	{
	    result = smpd_add_command_arg(cmd_ptr, "e", launch_node_ptr->env);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to add the environment variables to the launch command: '%s'\n", launch_node_ptr->env);
		goto launch_failure;
	    }
	}
	if (launch_node_ptr->dir[0] != '\0')
	{
	    result = smpd_add_command_arg(cmd_ptr, "d", launch_node_ptr->dir);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to add the working directory to the launch command: '%s'\n", launch_node_ptr->dir);
		goto launch_failure;
	    }
	}
	if (launch_node_ptr->path[0] != '\0')
	{
	    result = smpd_add_command_arg(cmd_ptr, "p", launch_node_ptr->path);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to add the search path to the launch command: '%s'\n", launch_node_ptr->path);
		goto launch_failure;
	    }
	}
	if (launch_node_ptr->clique[0] != '\0')
	{
	    result = smpd_add_command_arg(cmd_ptr, "q", launch_node_ptr->clique);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to add the clique string to the launch command: '%s'\n", launch_node_ptr->clique);
		goto launch_failure;
	    }
	}
	/*printf("creating launch command for rank %d\n", launch_node_ptr->iproc);*/
	result = smpd_add_command_int_arg(cmd_ptr, "i", launch_node_ptr->iproc);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to add the rank to the launch command: %d\n", launch_node_ptr->iproc);
	    goto launch_failure;
	}
	result = smpd_add_command_int_arg(cmd_ptr, "n", launch_node_ptr->nproc);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to add the nproc field to the launch command: %d\n", launch_node_ptr->nproc);
	    goto launch_failure;
	}
	result = smpd_add_command_arg(cmd_ptr, "k", kvs_name);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to add the kvs name('%s') to the launch command\n", kvs_name);
	    goto launch_failure;
	}
	result = smpd_add_command_arg(cmd_ptr, "kd", domain_name);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to add the domain name('%s') to the launch command\n", domain_name);
	    goto launch_failure;
	}
#ifdef HAVE_WINDOWS_H
    if(smpd_process.set_affinity)
    {
        result = smpd_add_command_int_arg(cmd_ptr, "af", 1);
        if(result != SMPD_SUCCESS)
        {
            smpd_err_printf("Unable to add the affinity flag to the launch command\n");
            goto launch_failure;
        }
        if(launch_node_ptr->binding_proc != -1){
            result = smpd_add_command_int_arg(cmd_ptr, "afp", launch_node_ptr->binding_proc);
            if(result != SMPD_SUCCESS)
            {
                smpd_err_printf("Unable to add the binding proc to the launch command\n");
                goto launch_failure;
            }
        }
    }
#endif
	if (launch_node_ptr->priority_class != SMPD_DEFAULT_PRIORITY_CLASS)
	{
	    result = smpd_add_command_int_arg(cmd_ptr, "pc", launch_node_ptr->priority_class);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to add the priority class field to the launch command: %d\n", launch_node_ptr->priority_class);
		goto launch_failure;
	    }
	}
	if (launch_node_ptr->priority_thread != SMPD_DEFAULT_PRIORITY)
	{
	    result = smpd_add_command_int_arg(cmd_ptr, "pt", launch_node_ptr->priority_thread);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to add the priority field to the launch command: %d\n", launch_node_ptr->priority_thread);
		goto launch_failure;
	    }
	}
	map_iter = launch_node_ptr->map_list;
	i = 0;
	while (map_iter)
	{
	    i++;
	    map_iter = map_iter->next;
	}
	if (i > 0)
	{
	    result = smpd_add_command_int_arg(cmd_ptr, "mn", i);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to add the drive mapping arg to the launch command: 'mn=%d'\n", i);
		goto launch_failure;
	    }
	}
	map_iter = launch_node_ptr->map_list;
	i = 0;
	while (map_iter)
	{
	    sprintf(drive_arg_str, "m%d", i);
	    i++;
	    sprintf(drive_map_str, "%c:%s", map_iter->drive, map_iter->share);
	    result = smpd_add_command_arg(cmd_ptr, drive_arg_str, drive_map_str);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to add the drive mapping to the launch command: '%s'\n", drive_map_str);
		goto launch_failure;
	    }
	    map_iter = map_iter->next;
	}

	/* send the launch command */
	result = smpd_post_write_command(smpd_process.left_context, cmd_ptr);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to post a write for the launch command:\n id: %d\n rank: %d\n cmd: '%s'\n",
		launch_node_ptr->host_id, launch_node_ptr->iproc, launch_node_ptr->exe);
	    goto launch_failure;
	}

	/* increment the number of launched processes */
	smpd_process.nproc++;
	if (spawn_context)
	    spawn_context->num_outstanding_launch_cmds++;

	/* move to the next node */
	launch_node_ptr = launch_node_ptr->next;
    }
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
launch_failure:
    smpd_exit_fn(FCNAME);
    return SMPD_FAIL;
}

#ifdef HAVE_WINDOWS_H
#undef FCNAME
#define FCNAME "smpd_add_pmi_env_to_procs"
int smpd_add_pmi_env_to_procs(smpd_launch_node_t *head, char *hostname, int port, char *kvs_name, char *domain_name)
{
    int len, result;
    errno_t ret_errno;
    char *env_str, env_int_str[SMPD_MAX_INT_LENGTH], port_str[SMPD_MAX_INT_LENGTH];
    smpd_enter_fn(FCNAME);
    if((head == NULL) || (hostname == NULL) || (port <= 0) || (kvs_name == NULL) || (domain_name == NULL)){
        smpd_err_printf("ERROR: Invalid args for adding pmi env to procs\n");
        smpd_exit_fn(FCNAME);
        return SMPD_FAIL;
    }
    
    ret_errno = _itoa_s(port, port_str, SMPD_MAX_INT_LENGTH, 10);
    if(ret_errno != 0){
        smpd_err_printf("ERROR: Unable to convert port (%d) to string\n", port);
        smpd_exit_fn(FCNAME);
        return SMPD_FAIL;
    }

    while(head){
        /* len => len of env string */
        len = strlen(head->env_data);
        head->env_data[len] = ' ';
        env_str = &(head->env_data[len])+1;
        /* len => remaining len in env string */
        len = SMPD_MAX_ENV_LENGTH - len - 1;
        result = MPIU_Str_add_string_arg(&env_str, &len, "PMI_HOST", hostname);
        if(result != MPIU_STR_SUCCESS){
            smpd_err_printf("ERROR: Unable to add PMI host to launch node\n");
            smpd_exit_fn(FCNAME);
            return SMPD_FAIL;
        }
        result = MPIU_Str_add_string_arg(&env_str, &len, "PMI_ROOT_HOST", hostname);
        if(result != MPIU_STR_SUCCESS){
            smpd_err_printf("ERROR: Unable to add PMI root host to launch node\n");
            smpd_exit_fn(FCNAME);
            return SMPD_FAIL;
        }
        result = MPIU_Str_add_string_arg(&env_str, &len, "PMI_PORT", port_str);
        if(result != MPIU_STR_SUCCESS){
            smpd_err_printf("ERROR: Unable to add PMI port to launch node\n");
            smpd_exit_fn(FCNAME);
            return SMPD_FAIL;
        }
        result = MPIU_Str_add_string_arg(&env_str, &len, "PMI_ROOT_PORT", port_str);
        if(result != MPIU_STR_SUCCESS){
            smpd_err_printf("ERROR: Unable to add PMI root port to launch node\n");
            smpd_exit_fn(FCNAME);
            return SMPD_FAIL;
        }
        result = MPIU_Str_add_string_arg(&env_str, &len, "PMI_KVS", kvs_name);
        if(result != MPIU_STR_SUCCESS){
            smpd_err_printf("ERROR: Unable to add PMI kvs name to launch node\n");
            smpd_exit_fn(FCNAME);
            return SMPD_FAIL;
        }
        result = MPIU_Str_add_string_arg(&env_str, &len, "PMI_DOMAIN", domain_name);
        if(result != MPIU_STR_SUCCESS){
            smpd_err_printf("ERROR: Unable to add PMI domain name to launch node\n");
            smpd_exit_fn(FCNAME);
            return SMPD_FAIL;
        }

        ret_errno = _itoa_s(head->nproc, env_int_str, SMPD_MAX_INT_LENGTH, 10);
        if(ret_errno != 0){
            smpd_err_printf("ERROR: Unable to convert nproc (%d) to string\n", head->nproc);
            smpd_exit_fn(FCNAME);
            return SMPD_FAIL;
        }
        result = MPIU_Str_add_string_arg(&env_str, &len, "PMI_SIZE", env_int_str);
        if(result != MPIU_STR_SUCCESS){
            smpd_err_printf("ERROR: Unable to add PMI size for MPI process\n");
            smpd_exit_fn(FCNAME);
            return SMPD_FAIL;
        }
        ret_errno = _itoa_s(head->iproc, env_int_str, SMPD_MAX_INT_LENGTH, 10);
        if(ret_errno != 0){
            smpd_err_printf("ERROR: Unable to convert iproc (%d) to string\n", head->iproc);
            smpd_exit_fn(FCNAME);
            return SMPD_FAIL;
        }
        result = MPIU_Str_add_string_arg(&env_str, &len, "PMI_RANK", env_int_str);
        if(result != MPIU_STR_SUCCESS){
            smpd_err_printf("ERROR: Unable to add PMI rank for MPI process\n");
            smpd_exit_fn(FCNAME);
            return SMPD_FAIL;
        }

        smpd_dbg_printf("ENV(%s) = %s\n", head->hostname, head->env_data);
        head = head->next;
    }

    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}
#endif

#undef FCNAME
#define FCNAME "smpd_handle_result"
int smpd_handle_result(smpd_context_t *context)
{
    int result, ret_val;
    char str[SMPD_MAX_CMD_LENGTH];
    char err_msg[SMPD_MAX_ERROR_LEN];
    smpd_command_t *iter, *trailer, *cmd_ptr;
    int match_tag;
    char ctx_key[100];
    int process_id;
    smpd_context_t *pmi_context;
    smpd_process_t *piter;
    int rank;
    SMPDU_Sock_t insock;
    SMPDU_SOCK_NATIVE_FD stdin_fd;
    smpd_context_t *context_in;
#ifdef HAVE_WINDOWS_H
    DWORD dwThreadID;
    SOCKET hWrite;
#endif
#ifdef USE_PTHREAD_STDIN_REDIRECTION
    int fd[2];
#endif
    char pg_id[SMPD_MAX_DBS_NAME_LEN+1] = "";
    char pg_ctx[100] = "";
    int pg_rank = -1;
    smpd_process_group_t *pg;
    char host_description[256];
    int listener_port;
    char context_str[20];
    smpd_context_t *orig_context;
    int num_decoded;

    smpd_enter_fn(FCNAME);

    if (context->type != SMPD_CONTEXT_PMI && MPIU_Str_get_string_arg(context->read_cmd.cmd, "ctx_key", ctx_key, 100) == MPIU_STR_SUCCESS)
    {
	process_id = atoi(ctx_key);
	smpd_dbg_printf("forwarding the dbs result command to the pmi context %d.\n", process_id);
	pmi_context = NULL;
	piter = smpd_process.process_list;
	while (piter)
	{
	    if (piter->id == process_id)
	    {
		pmi_context = piter->pmi;
		break;
	    }
	    piter = piter->next;
	}
	if (pmi_context == NULL)
	{
	    smpd_err_printf("received dbs result for a pmi context that doesn't exist: unmatched id = %d\n", process_id);
	    smpd_exit_fn(FCNAME);
	    return SMPD_SUCCESS;
	}

	result = smpd_forward_command(context, pmi_context);
	smpd_exit_fn(FCNAME);
	return result;
    }

    if (MPIU_Str_get_int_arg(context->read_cmd.cmd, "cmd_tag", &match_tag) != MPIU_STR_SUCCESS)
    {
	smpd_err_printf("result command received without a cmd_tag field: '%s'\n", context->read_cmd.cmd);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    trailer = iter = context->wait_list;
    while (iter)
    {
	if (iter->tag == match_tag)
	{
	    ret_val = SMPD_SUCCESS;
	    if (MPIU_Str_get_string_arg(context->read_cmd.cmd, "result", str, SMPD_MAX_CMD_LENGTH) == MPIU_STR_SUCCESS)
	    {
		if (strcmp(iter->cmd_str, "connect") == 0)
		{
		    if (strcmp(str, SMPD_SUCCESS_STR) == 0)
		    {
			ret_val = SMPD_CONNECTED;
			switch (context->state)
			{
			case SMPD_MPIEXEC_CONNECTING_TREE:
			    smpd_dbg_printf("successful connect, state: connecting tree.\n");
			    {
				smpd_host_node_t *host_node_iter;
				/* find the host node that this connect command was issued for and set the connect_to variable to to point to it */
				host_node_iter = smpd_process.host_list;
				context->connect_to = NULL;
				while (host_node_iter != NULL)
				{
				    if (host_node_iter->connect_cmd_tag == match_tag)
				    {
					context->connect_to = host_node_iter;
					break;
				    }
				    host_node_iter = host_node_iter->next;
				}
			    }
			    break;
			case SMPD_MPIEXEC_CONNECTING_SMPD:
			    smpd_dbg_printf("successful connect, state: connecting smpd.\n");
			    break;
			case SMPD_CONNECTING:
			    smpd_dbg_printf("successful connect, state: connecting.\n");
			    break;
			default:
			    break;
			}
		    }
		    else
		    {
			smpd_err_printf("connect failed: %s\n", str);
			ret_val = SMPD_ABORT;
		    }
		}
		else if (strcmp(iter->cmd_str, "launch") == 0)
		{
		    smpd_process.nproc_launched++;
		    if (strcmp(str, SMPD_SUCCESS_STR) == 0)
		    {
            char wMsg[256];
			MPIU_Str_get_string_arg(context->read_cmd.cmd, "pg_id", pg_id, SMPD_MAX_DBS_NAME_LEN);
			MPIU_Str_get_int_arg(context->read_cmd.cmd, "pg_rank", &pg_rank);
			MPIU_Str_get_string_arg(context->read_cmd.cmd, "pg_ctx", pg_ctx, 100);
            result = MPIU_Str_get_string_arg(context->read_cmd.cmd, "warning", wMsg, 256);
            if(result == SMPD_SUCCESS){
                smpd_err_printf("*********** Warning ************\n");
                smpd_err_printf("%s\n", wMsg);
                smpd_err_printf("*********** Warning ************\n");
            }
			pg = smpd_process.pg_list;
			while (pg)
			{
			    if (strcmp(pg->kvs, pg_id) == 0)
			    {
				MPIU_Strncpy(pg->processes[pg_rank].ctx_key, pg_ctx, 100);
				break;
			    }
			    pg = pg->next;
			}
			if (context->spawn_context)
			{
			    smpd_dbg_printf("successfully spawned: '%s'\n", iter->cmd);
			    /*printf("successfully spawned: '%s'\n", iter->cmd);fflush(stdout);*/
			    context->spawn_context->num_outstanding_launch_cmds--;
			    smpd_dbg_printf("%d outstanding spawns.\n", context->spawn_context->num_outstanding_launch_cmds);
			    if (context->spawn_context->num_outstanding_launch_cmds == 0)
			    {
				/* add the result string */
				result = smpd_add_command_arg(context->spawn_context->result_cmd, "result", SMPD_SUCCESS_STR);
				if (result != SMPD_SUCCESS)
				{
				    smpd_err_printf("unable to add the result string to the result command.\n");
				    smpd_exit_fn(FCNAME);
				    return result;
				}
				/* send the spawn result command */
				result = smpd_post_write_command(context, context->spawn_context->result_cmd);
				if (result != SMPD_SUCCESS)
				{
				    smpd_err_printf("unable to post a write of the spawn result command.\n");
				    smpd_exit_fn(FCNAME);
				    return result;
				}

				smpd_process.spawning = SMPD_FALSE;
				result = smpd_handle_delayed_spawn_command();
				if (result != SMPD_SUCCESS)
				{
				    smpd_err_printf("An error occurred handling delayed spawn commands.\n");
				    smpd_exit_fn(FCNAME);
				    return result;
				}
			    }
			}
			else
			{
			    smpd_dbg_printf("successfully launched: '%s'\n", iter->cmd);
			    if (!smpd_process.stdin_redirecting)
			    {
				rank = 0;
				MPIU_Str_get_int_arg(iter->cmd, "i", &rank);
				if (rank == 0)
				{
				    smpd_dbg_printf("root process launched, starting stdin redirection.\n");
				    /* get a handle to stdin */
#ifdef HAVE_WINDOWS_H
				    result = smpd_make_socket_loop((SOCKET*)&stdin_fd, &hWrite);
				    if (result)
				    {
					smpd_err_printf("Unable to make a local socket loop to forward stdin.\n");
					smpd_exit_fn(FCNAME);
					return SMPD_FAIL;
				    }
#elif defined(USE_PTHREAD_STDIN_REDIRECTION)
				    socketpair(AF_UNIX, SOCK_STREAM, 0, fd);
				    smpd_process.stdin_read = fd[0];
				    smpd_process.stdin_write = fd[1];
				    stdin_fd = fd[0];
#else
				    stdin_fd = fileno(stdin);
#endif

				    /* convert the native handle to a sock */
				    /*printf("stdin native sock %d\n", stdin_fd);fflush(stdout);*/
				    result = SMPDU_Sock_native_to_sock(smpd_process.set, stdin_fd, NULL, &insock);
				    if (result != SMPD_SUCCESS)
				    {
					smpd_err_printf("unable to create a sock from stdin,\nsock error: %s\n", get_sock_error_string(result));
					smpd_exit_fn(FCNAME);
					return SMPD_FAIL;
				    }
				    /* create a context for reading from stdin */
				    result = smpd_create_context(SMPD_CONTEXT_MPIEXEC_STDIN, smpd_process.set, insock, -1, &context_in);
				    if (result != SMPD_SUCCESS)
				    {
					smpd_err_printf("unable to create a context for stdin.\n");
					smpd_exit_fn(FCNAME);
					return SMPD_FAIL;
				    }
				    SMPDU_Sock_set_user_ptr(insock, context_in);

#ifdef HAVE_WINDOWS_H
				    /* unfortunately, we cannot use stdin directly as a sock.  So, use a thread to read and forward
				    stdin to a sock */
				    smpd_process.hCloseStdinThreadEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
				    if (smpd_process.hCloseStdinThreadEvent == NULL)
				    {
					smpd_err_printf("Unable to create the stdin thread close event, error %d\n", GetLastError());
					smpd_exit_fn(FCNAME);
					return SMPD_FAIL;
				    }
				    smpd_process.hStdinThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)smpd_stdin_thread, (void*)hWrite, 0, &dwThreadID);
				    if (smpd_process.hStdinThread == NULL)
				    {
					smpd_err_printf("Unable to create a thread to read stdin, error %d\n", GetLastError());
					smpd_exit_fn(FCNAME);
					return SMPD_FAIL;
				    }
#elif defined(USE_PTHREAD_STDIN_REDIRECTION)
				    if (pthread_create(&smpd_process.stdin_thread, NULL, smpd_pthread_stdin_thread, NULL) != 0)
				    {
					smpd_err_printf("Unable to create a thread to read stdin, error %d\n", errno);
					smpd_exit_fn(FCNAME);
					return SMPD_FAIL;
				    }
				    /*pthread_detach(smpd_process.stdin_thread);*/
#endif
				    /* set this variable first before posting the first read to avoid a race condition? */
				    smpd_process.stdin_redirecting = SMPD_TRUE;
				    /* post a read for a user command from stdin */
				    context_in->read_state = SMPD_READING_STDIN;
				    result = SMPDU_Sock_post_read(insock, context_in->read_cmd.cmd, 1, 1, NULL);
				    if (result != SMPD_SUCCESS)
				    {
					smpd_err_printf("unable to post a read on stdin for an incoming user command, error:\n%s\n",
					    get_sock_error_string(result));
					smpd_exit_fn(FCNAME);
					return SMPD_FAIL;
				    }
				}
			    }
			}
			if (pg && pg->processes[pg_rank].suspend_cmd != NULL)
			{
			    /* send the delayed suspend command */
			    result = smpd_post_write_command(context, pg->processes[pg_rank].suspend_cmd);
			    if (result != SMPD_SUCCESS)
			    {
				smpd_err_printf("unable to post a write of the suspend command.\n");
				smpd_exit_fn(FCNAME);
				return result;
			    }
			}
		    }
		    else
		    {
			/* FIXME: We shouldn't abort if a process fails to launch as part of a spawn command */
			smpd_process.nproc_exited++;
            smpd_process.mpiexec_exit_code = -1;
			if (MPIU_Str_get_string_arg(context->read_cmd.cmd, "error", err_msg, SMPD_MAX_ERROR_LEN) == MPIU_STR_SUCCESS)
			{
			    smpd_err_printf("launch failed: %s\n", err_msg);
			}
			else
			{
			    smpd_err_printf("launch failed: %s\n", str);
			}
			ret_val = SMPD_ABORT;
		    }
		}
		else if (strcmp(iter->cmd_str, "start_dbs") == 0)
		{
		    if (strcmp(str, SMPD_SUCCESS_STR) == 0)
		    {
			if (context->spawn_context)
			{
			    if (MPIU_Str_get_string_arg(context->read_cmd.cmd, "kvs_name", context->spawn_context->kvs_name, SMPD_MAX_DBS_NAME_LEN) == MPIU_STR_SUCCESS)
			    {
				if (MPIU_Str_get_string_arg(context->read_cmd.cmd, "domain_name", context->spawn_context->domain_name, SMPD_MAX_DBS_NAME_LEN) == MPIU_STR_SUCCESS)
				{
				    smpd_dbg_printf("start_dbs succeeded, kvs_name: '%s', domain_name: '%s'\n", context->spawn_context->kvs_name, context->spawn_context->domain_name);
				    ret_val = smpd_launch_processes(context->spawn_context->launch_list, context->spawn_context->kvs_name, context->spawn_context->domain_name, context->spawn_context);
				}
				else
				{
				    smpd_err_printf("invalid start_dbs result returned, no domain_name specified: '%s'\n", context->read_cmd.cmd);
				    ret_val = SMPD_FAIL;
				}
			    }
			    else
			    {
				smpd_err_printf("invalid start_dbs result returned, no kvs_name specified: '%s'\n", context->read_cmd.cmd);
				ret_val = SMPD_FAIL;
			    }
			}
			else
			{
			    if (MPIU_Str_get_string_arg(context->read_cmd.cmd, "kvs_name", smpd_process.kvs_name, SMPD_MAX_DBS_NAME_LEN) == MPIU_STR_SUCCESS)
			    {
				if (MPIU_Str_get_string_arg(context->read_cmd.cmd, "domain_name", smpd_process.domain_name, SMPD_MAX_DBS_NAME_LEN) == MPIU_STR_SUCCESS)
				{
				    smpd_dbg_printf("start_dbs succeeded, kvs_name: '%s', domain_name: '%s'\n", smpd_process.kvs_name, smpd_process.domain_name);
                    if ((smpd_process.launch_list != NULL) && (!smpd_process.use_ms_hpc))
				    {
					ret_val = smpd_launch_processes(smpd_process.launch_list, smpd_process.kvs_name, smpd_process.domain_name, NULL);
				    }
                    else
				    {
					/* mpiexec connected to an smpd without any processes to launch. 
                     * This means it is running -pmiserver 
                     */
					create_process_group(smpd_process.nproc, smpd_process.kvs_name, &pg);
					result = smpd_create_command("pmi_listen", 0, 1, SMPD_TRUE, &cmd_ptr);
					if (result == SMPD_SUCCESS)
					{
					    result = smpd_add_command_int_arg(cmd_ptr, "nproc", smpd_process.nproc);
					    if (result == SMPD_SUCCESS)
					    {
						result = smpd_post_write_command(smpd_process.left_context, cmd_ptr);
						if (result != SMPD_SUCCESS)
						{
						    smpd_err_printf("unable to post a write for the pmi_listen command\n");
						    ret_val = SMPD_FAIL;
						}
					    }
					    else
					    {
						smpd_err_printf("unable to add the nproc to the pmi_listen command\n");
						ret_val = SMPD_FAIL;
					    }
					}
					else
					{
					    smpd_err_printf("unable to create a pmi_listen command.\n");
					    ret_val = SMPD_FAIL;
					}
				    }
				}
				else
				{
				    smpd_err_printf("invalid start_dbs result returned, no domain_name specified: '%s'\n", context->read_cmd.cmd);
				    ret_val = SMPD_FAIL;
				}
			    }
			    else
			    {
				smpd_err_printf("invalid start_dbs result returned, no kvs_name specified: '%s'\n", context->read_cmd.cmd);
				ret_val = SMPD_FAIL;
			    }
			}
		    }
		    else
		    {
			smpd_err_printf("start_dbs failed: %s\n", str);
			ret_val = SMPD_ABORT;
		    }
		}
		else if (strcmp(iter->cmd_str, "pmi_listen") == 0)
		{
		    MPIU_Str_get_string_arg(context->read_cmd.cmd, "host_description", host_description, 256);
		    MPIU_Str_get_int_arg(context->read_cmd.cmd, "listener_port", &listener_port);
            /* SINGLETON: If no port to connect back print this information else connect to the port 
             * and execute the SINGLETON INIT protocol 
             */
            if(smpd_process.singleton_client_port > 0){
                smpd_context_t *p_singleton_mpiexec_context;
                char connect_to_host[SMPD_SINGLETON_MAX_HOST_NAME_LEN];
                /* Create a context */
                result = smpd_create_context(SMPD_CONTEXT_SINGLETON_INIT_MPIEXEC, context->set, 
                                            SMPDU_SOCK_INVALID_SOCK, -1, &p_singleton_mpiexec_context);
                if(result != SMPD_SUCCESS){
                    context->state = SMPD_DONE;
                    smpd_err_printf("smpd_create_context failed, error = %d\n", result);
                    return SMPD_FAIL;
                }

                p_singleton_mpiexec_context->state = SMPD_SINGLETON_MPIEXEC_CONNECTING;
                p_singleton_mpiexec_context->singleton_init_pm_port = listener_port;
                strncpy(p_singleton_mpiexec_context->singleton_init_hostname, host_description, 
                        SMPD_SINGLETON_MAX_HOST_NAME_LEN);
                strncpy(p_singleton_mpiexec_context->singleton_init_kvsname, smpd_process.kvs_name,
                        SMPD_SINGLETON_MAX_KVS_NAME_LEN);
                strncpy(p_singleton_mpiexec_context->singleton_init_domainname, smpd_process.domain_name,
                        SMPD_SINGLETON_MAX_KVS_NAME_LEN);
                /* Post a connect */
                /* FIXME : mismatch btw size of connect_to->host and max host name len */
                result = get_hostname(connect_to_host, SMPD_SINGLETON_MAX_HOST_NAME_LEN);
                if(result != SMPD_SUCCESS){
                    context->state = SMPD_DONE;
                    smpd_err_printf("gethostname failed, error = %d\n", result);
                    result = smpd_free_context(p_singleton_mpiexec_context);
                    if(result != SMPD_SUCCESS){ 
                        smpd_err_printf("smpd_free_context failed, error = %d\n", result);
                    }
                    return SMPD_FAIL;
                }
                result = SMPDU_Sock_post_connect(context->set, p_singleton_mpiexec_context,
                                                    connect_to_host,
                                                    smpd_process.singleton_client_port,
                                                    &p_singleton_mpiexec_context->sock);
                if(result != SMPD_SUCCESS){
                    context->state = SMPD_DONE;
                    smpd_err_printf("SMPDU_Sock_post_connect failed, error = %s\n", get_sock_error_string(result));
                    result = smpd_free_context(p_singleton_mpiexec_context);
                    if(result != SMPD_SUCCESS){ 
                        smpd_err_printf("smpd_free_context failed, error = %d\n", result);
                    }
                    return SMPD_FAIL;
                }
                ret_val = SMPD_SUCCESS;
            }
#ifdef HAVE_WINDOWS_H /* Define MS HPC launching only for windows systems */
            else if(smpd_process.use_ms_hpc){
                smpd_hpc_js_handle_t js_hnd;
                /* Launch the procs using MS HPC job scheduler */
                smpd_dbg_printf("PMI_ROOT_HOST=%s\nPMI_ROOT_PORT=%d\nPMI_KVS=%s\nPMI_DOMAIN=%s\n", host_description, listener_port, smpd_process.kvs_name, smpd_process.domain_name);
                result = smpd_add_pmi_env_to_procs(smpd_process.launch_list, host_description, listener_port, smpd_process.kvs_name, smpd_process.domain_name);
                if(result != SMPD_SUCCESS){
                    smpd_err_printf("Unable to add PMI environment to procs to be launched with MS hpc\n");
                    smpd_exit_fn(FCNAME);
                    return SMPD_FAIL;
                }
                /* Initialize MS HPC Resource management kernel & bootstrap manager
                 */
                result = smpd_hpc_js_rmk_init(&js_hnd);
                if(result != SMPD_SUCCESS){
                    smpd_err_printf("Unable to initialize MS HPC Resource management kernel\n");
                    smpd_exit_fn(FCNAME);
                    return SMPD_FAIL;
                }

                result = smpd_hpc_js_bs_init(js_hnd);
                if(result != SMPD_SUCCESS){
                    smpd_err_printf("Unable to initialize MS HPC Bootstrap manager\n");
                    smpd_hpc_js_rmk_finalize(&js_hnd);
                    smpd_exit_fn(FCNAME);
                    return SMPD_FAIL;
                }

                /* Allocate nodes using the RM */
                result = smpd_hpc_js_rmk_alloc_nodes(js_hnd, smpd_process.launch_list);
                if(result != SMPD_SUCCESS){
                    smpd_err_printf("Unable to allocate nodes using MS HPC Resource manager\n");
                    smpd_hpc_js_bs_finalize(js_hnd);
                    smpd_hpc_js_rmk_finalize(&js_hnd);
                    smpd_exit_fn(FCNAME);
                    return SMPD_FAIL;
                }

                /* Launch procs using the BSS */
                result = smpd_hpc_js_bs_launch_procs(js_hnd, smpd_process.launch_list);
                if(result != SMPD_SUCCESS){
                    smpd_err_printf("Unable to launch procs using MS HPC Resource manager\n");
                    smpd_hpc_js_bs_finalize(js_hnd);
                    smpd_hpc_js_rmk_finalize(&js_hnd);
                    smpd_exit_fn(FCNAME);
                    return SMPD_FAIL;
                }

                smpd_hpc_js_bs_finalize(js_hnd);
                smpd_hpc_js_rmk_finalize(&js_hnd);
                ret_val = SMPD_SUCCESS;
            }
#endif /* HAVE_WINDOWS_H */
            else{
                printf("PMI_ROOT_HOST=%s\nPMI_ROOT_PORT=%d\nPMI_KVS=%s\nPMI_DOMAIN=%s\n", host_description, listener_port, smpd_process.kvs_name, smpd_process.domain_name);
            }
		    /*printf("%s %d %s\n", smpd_process.host_list->host, smpd_process.port, smpd_process.kvs_name);*/
		    fflush(stdout);
		}
        else if (strcmp(iter->cmd_str, "proc_info") == 0){
            smpd_dbg_printf("%s command result = %s\n", iter->cmd_str, str);
            ret_val = SMPD_DBS_RETURN;
        }
		else if (strcmp(iter->cmd_str, "spawn") == 0)
		{
		    smpd_dbg_printf("%s command result = %s\n", iter->cmd_str, str);
		    ret_val = SMPD_DBS_RETURN;
		}
		else if (iter->cmd_str[0] == 'd' && iter->cmd_str[1] == 'b')
		{
		    smpd_dbg_printf("%s command result = %s\n", iter->cmd_str, str);
		    ret_val = SMPD_DBS_RETURN;
		}
		else if ((strcmp(iter->cmd_str, "init") == 0) || (strcmp(iter->cmd_str, "finalize") == 0))
		{
		    smpd_dbg_printf("%s command result = %s\n", iter->cmd_str, str);
		    ret_val = SMPD_DBS_RETURN;
		}
		else if (strcmp(iter->cmd_str, "barrier") == 0)
		{
		    smpd_dbg_printf("%s command result = %s\n", iter->cmd_str, str);
		    ret_val = SMPD_DBS_RETURN;
		}
		else if (strcmp(iter->cmd_str, "validate") == 0)
		{
		    /* print the result of the validate command */
		    printf("%s\n", str);
		    /* close the session */
		    ret_val = smpd_create_command("done", smpd_process.id, context->id, SMPD_FALSE, &cmd_ptr);
		    if (ret_val == SMPD_SUCCESS)
		    {
			ret_val = smpd_post_write_command(context, cmd_ptr);
			if (ret_val == SMPD_SUCCESS)
			{
			    ret_val = SMPD_CLOSE;
			}
			else
			{
			    smpd_err_printf("unable to post a write of a done command.\n");
			}
		    }
		    else
		    {
			smpd_err_printf("unable to create a done command.\n");
		    }
		}
		else if (strcmp(iter->cmd_str, "status") == 0)
		{
		    /* print the result of the status command */
		    printf("smpd running on %s\n", smpd_process.console_host);
		    if (strcmp(str, "none"))
		    {
			printf("dynamic hosts: %s\n", str);
		    }
		    ret_val = smpd_create_command("done", smpd_process.id, context->id, SMPD_FALSE, &cmd_ptr);
		    if (ret_val == SMPD_SUCCESS)
		    {
			ret_val = smpd_post_write_command(context, cmd_ptr);
			if (ret_val == SMPD_SUCCESS)
			{
			    ret_val = SMPD_CLOSE;
			}
			else
			{
			    smpd_err_printf("unable to post a write of a done command.\n");
			}
		    }
		    else
		    {
			smpd_err_printf("unable to create a done command.\n");
		    }
		}
		else if (strcmp(iter->cmd_str, "get") == 0)
		{
		    if (strcmp(str, SMPD_SUCCESS_STR) == 0)
		    {
			/* print the result of the get command */
			char value[SMPD_MAX_VALUE_LENGTH];
			if (MPIU_Str_get_string_arg(context->read_cmd.cmd, "value", value, SMPD_MAX_VALUE_LENGTH) == MPIU_STR_SUCCESS)
			{
			    printf("%s\n", value);
			}
			else
			{
			    printf("Error: unable to get the value from the result command - '%s'\n", context->read_cmd.cmd);
			}
		    }
		    else
		    {
			printf("%s\n", str);
		    }
		}
		else if (strcmp(iter->cmd_str, "set") == 0 || strcmp(iter->cmd_str, "delete") == 0)
		{
		    /* print the result of the set or delete command */
		    printf("%s\n", str);
		    if (smpd_process.do_console_returns)
		    {
			/* close the session */
			ret_val = smpd_create_command("done", smpd_process.id, context->id, SMPD_FALSE, &cmd_ptr);
			if (ret_val == SMPD_SUCCESS)
			{
			    ret_val = smpd_post_write_command(context, cmd_ptr);
			    if (ret_val == SMPD_SUCCESS)
			    {
				ret_val = SMPD_CLOSE;
			    }
			    else
			    {
				smpd_err_printf("unable to post a write of a done command.\n");
			    }
			}
			else
			{
			    smpd_err_printf("unable to create a done command.\n");
			}
		    }
		}
		else if (strcmp(iter->cmd_str, "stat") == 0)
		{
		    /* print the result of the stat command */
		    printf("%s\n", str);
		}
		else if (strcmp(iter->cmd_str, "cred_request") == 0)
		{
		    if (strcmp(str, SMPD_SUCCESS_STR) == 0)
		    {
			/* FIXME: decrypt the password */
			if (MPIU_Str_get_string_arg(context->read_cmd.cmd, "account", smpd_process.UserAccount, SMPD_MAX_ACCOUNT_LENGTH) == MPIU_STR_SUCCESS &&
			    MPIU_Str_get_string_arg(context->read_cmd.cmd, "password", context->encrypted_password/*smpd_process.UserPassword*/, SMPD_MAX_PASSWORD_LENGTH) == MPIU_STR_SUCCESS)
			{
			    int length = SMPD_MAX_PASSWORD_LENGTH;
			    smpd_dbg_printf("cred_request succeeded, account: '%s'\n", smpd_process.UserAccount);
			    result = smpd_decrypt_data(context->encrypted_password, SMPD_MAX_PASSWORD_LENGTH, smpd_process.UserPassword, &length);
			    if (result == SMPD_SUCCESS)
			    {
				strcpy(iter->context->account, smpd_process.UserAccount);
				strcpy(iter->context->password, smpd_process.UserPassword);
				strcpy(iter->context->cred_request, "yes");
				iter->context->read_state = SMPD_IDLE;
				iter->context->write_state = SMPD_WRITING_CRED_ACK_YES;
				result = SMPDU_Sock_post_write(iter->context->sock, iter->context->cred_request, SMPD_MAX_CRED_REQUEST_LENGTH, SMPD_MAX_CRED_REQUEST_LENGTH, NULL);
			    }
			    ret_val = result == SMPD_SUCCESS ? SMPD_SUCCESS : SMPD_FAIL;
			}
			else
			{
			    smpd_err_printf("invalid cred_request result returned, no account and password specified: '%s'\n", context->read_cmd.cmd);
			    ret_val = SMPD_FAIL;
			}
		    }
		    else if (strcmp(str, "sspi") == 0)
		    {
			strcpy(iter->context->cred_request, SMPD_CRED_ACK_SSPI);
			iter->context->read_state = SMPD_IDLE;
			iter->context->write_state = SMPD_WRITING_CRED_ACK_SSPI;
			result = SMPDU_Sock_post_write(iter->context->sock, iter->context->cred_request, SMPD_MAX_CRED_REQUEST_LENGTH, SMPD_MAX_CRED_REQUEST_LENGTH, NULL);
			ret_val = result == SMPD_SUCCESS ? SMPD_SUCCESS : SMPD_FAIL;
		    }
		    else if (strcmp(str, "sspi_job") == 0)
		    {
			strcpy(iter->context->cred_request, SMPD_CRED_ACK_SSPI_JOB_KEY);
			iter->context->read_state = SMPD_IDLE;
			iter->context->write_state = SMPD_WRITING_CRED_ACK_SSPI_JOB_KEY;
			result = SMPDU_Sock_post_write(iter->context->sock, iter->context->cred_request, SMPD_MAX_CRED_REQUEST_LENGTH, SMPD_MAX_CRED_REQUEST_LENGTH, NULL);
			ret_val = result == SMPD_SUCCESS ? SMPD_SUCCESS : SMPD_FAIL;
		    }
		    else
		    {
			strcpy(iter->context->cred_request, "no");
			iter->context->read_state = SMPD_IDLE;
			iter->context->write_state = SMPD_WRITING_CRED_ACK_NO;
			result = SMPDU_Sock_post_write(iter->context->sock, iter->context->cred_request, SMPD_MAX_CRED_REQUEST_LENGTH, SMPD_MAX_CRED_REQUEST_LENGTH, NULL);
			ret_val = result == SMPD_SUCCESS ? SMPD_SUCCESS : SMPD_FAIL;
		    }
		}
		else if (strcmp(iter->cmd_str, "sspi_init") == 0)
		{
		    if (strcmp(str, SMPD_SUCCESS_STR) == 0)
		    {
			/* decode the sspi buffer and context pointer from the command */
			MPIU_Str_get_string_arg(context->read_cmd.cmd, "sspi_context", context_str, 20);
			if (sscanf(context_str, "%p", &orig_context) != 1)
			{
			    smpd_err_printf("unable to decode a pointer from this string: '%s'\n", context_str);
			    smpd_exit_fn(FCNAME);
			    return SMPD_FAIL;
			}
			MPIU_Str_get_int_arg(context->read_cmd.cmd, "sspi_id", &orig_context->sspi_context->id);
			MPIU_Str_get_int_arg(context->read_cmd.cmd, "data_length", &orig_context->sspi_context->buffer_length);
			if (orig_context->sspi_context->buffer != NULL)
			{
			    MPIU_Free(orig_context->sspi_context->buffer);
			    orig_context->sspi_context->buffer = NULL;
			}
			if (orig_context->sspi_context->buffer_length > 0)
			{
			    orig_context->sspi_context->buffer = MPIU_Malloc(orig_context->sspi_context->buffer_length);
			    if (orig_context->sspi_context->buffer == NULL)
			    {
				smpd_err_printf("unable to allocate %d bytes for an sspi buffer\n", orig_context->sspi_context->buffer_length);
				smpd_exit_fn(FCNAME);
				return SMPD_FAIL;
			    }
			}
			MPIU_Str_get_string_arg(context->read_cmd.cmd, "data", str, SMPD_MAX_CMD_LENGTH);
			smpd_decode_buffer(str, orig_context->sspi_context->buffer, orig_context->sspi_context->buffer_length, &num_decoded);
			/* send the sspi buffer to the decoded context */
			/* move the state of that context to WRITING_SSPI_HEADER */
			orig_context->read_state = SMPD_IDLE;
			orig_context->write_state = SMPD_WRITING_CLIENT_SSPI_HEADER;
			MPIU_Snprintf(orig_context->sspi_header, SMPD_SSPI_HEADER_LENGTH, "%d", num_decoded);
			result = SMPDU_Sock_post_write(orig_context->sock, orig_context->sspi_header, SMPD_SSPI_HEADER_LENGTH, SMPD_SSPI_HEADER_LENGTH, NULL);
			if (result == SMPD_SUCCESS)
			{
			    ret_val = SMPD_SUCCESS;
			}
			else
			{
#ifdef HAVE_WINDOWS_H
			    smpd_process.sec_fn->DeleteSecurityContext(&orig_context->sspi_context->context);
			    smpd_process.sec_fn->FreeCredentialsHandle(&orig_context->sspi_context->credential);
#endif
			    smpd_err_printf("unable to post a write of the sspi header,\nsock error: %s\n", get_sock_error_string(result));
			    orig_context->state = SMPD_CLOSING;
			    result = SMPDU_Sock_post_close(orig_context->sock);
			    smpd_exit_fn(FCNAME);
			    ret_val = (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
			}
		    }
		    else
		    {
			/* close the sspi context */
			/* cleanup the sspi structures */
			/* notify connection failure */
		    }
		}
		else if (strcmp(iter->cmd_str, "sspi_iter") == 0)
		{
		    if (strcmp(str, SMPD_SUCCESS_STR) == 0)
		    {
			/* decode the sspi buffer and context pointer from the command */
			MPIU_Str_get_string_arg(context->read_cmd.cmd, "sspi_context", context_str, 20);
			if (sscanf(context_str, "%p", &orig_context) != 1)
			{
			    smpd_err_printf("unable to decode a pointer from this string: '%s'\n", context_str);
			    smpd_exit_fn(FCNAME);
			    return SMPD_FAIL;
			}
			MPIU_Str_get_int_arg(context->read_cmd.cmd, "data_length", &orig_context->sspi_context->buffer_length);
			if (orig_context->sspi_context->buffer != NULL)
			{
			    MPIU_Free(orig_context->sspi_context->buffer);
			    orig_context->sspi_context->buffer = NULL;
			}
			if (orig_context->sspi_context->buffer_length > 0)
			{
			    orig_context->sspi_context->buffer = MPIU_Malloc(orig_context->sspi_context->buffer_length);
			    if (orig_context->sspi_context->buffer == NULL)
			    {
				smpd_err_printf("unable to allocate %d bytes for an sspi buffer\n", orig_context->sspi_context->buffer_length);
				smpd_exit_fn(FCNAME);
				return SMPD_FAIL;
			    }
			    MPIU_Str_get_string_arg(context->read_cmd.cmd, "data", str, SMPD_MAX_CMD_LENGTH);
			    smpd_decode_buffer(str, orig_context->sspi_context->buffer, orig_context->sspi_context->buffer_length, &num_decoded);
			    /* send the sspi buffer to the decoded context */
			    /* move the state of that context to WRITING_SSPI_HEADER */
			    orig_context->read_state = SMPD_IDLE;
			    orig_context->write_state = SMPD_WRITING_CLIENT_SSPI_HEADER;
			    MPIU_Snprintf(orig_context->sspi_header, SMPD_SSPI_HEADER_LENGTH, "%d", num_decoded);
			    result = SMPDU_Sock_post_write(orig_context->sock, orig_context->sspi_header, SMPD_SSPI_HEADER_LENGTH, SMPD_SSPI_HEADER_LENGTH, NULL);
			    if (result == SMPD_SUCCESS)
			    {
				ret_val = SMPD_SUCCESS;
			    }
			    else
			    {
#ifdef HAVE_WINDOWS_H
				smpd_process.sec_fn->DeleteSecurityContext(&orig_context->sspi_context->context);
				smpd_process.sec_fn->FreeCredentialsHandle(&orig_context->sspi_context->credential);
#endif
				smpd_err_printf("unable to post a write of the sspi header,\nsock error: %s\n", get_sock_error_string(result));
				orig_context->state = SMPD_CLOSING;
				result = SMPDU_Sock_post_close(orig_context->sock);
				smpd_exit_fn(FCNAME);
				ret_val = (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
			    }
			}
			else
			{
			    if (orig_context->sspi_context->buffer_length == 0)
			    {
				/* no more data, empty buffer returned */
				/* FIXME: This assumes that the server knows to post a write of the delegate command because it knows that no buffer will be returned by the iter command */
				orig_context->write_state = SMPD_IDLE;
				orig_context->read_state = SMPD_READING_CLIENT_SSPI_HEADER;
				result = SMPDU_Sock_post_read(orig_context->sock, orig_context->sspi_header, SMPD_SSPI_HEADER_LENGTH, SMPD_SSPI_HEADER_LENGTH, NULL);
				if (result != SMPD_SUCCESS)
				{
				    /* FIXME: Add code to cleanup sspi structures */
				    smpd_err_printf("unable to post a read of the client sspi header,\nsock error: %s\n", get_sock_error_string(result));
				    orig_context->state = SMPD_CLOSING;
				    result = SMPDU_Sock_post_close(orig_context->sock);
				    smpd_exit_fn(FCNAME);
				    return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
				}
			    }
			    else
			    {
				/* error decoding the data_length parameter */
#ifdef HAVE_WINDOWS_H
				smpd_process.sec_fn->DeleteSecurityContext(&orig_context->sspi_context->context);
				smpd_process.sec_fn->FreeCredentialsHandle(&orig_context->sspi_context->credential);
#endif
				smpd_err_printf("unable to decode the data_length parameter,\n");
				orig_context->state = SMPD_CLOSING;
				result = SMPDU_Sock_post_close(orig_context->sock);
				smpd_exit_fn(FCNAME);
				ret_val = (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
			    }
			}
		    }
		    else
		    {
			/* close the sspi context */
			/* cleanup the sspi structures */
			/* notify connection failure */
		    }
		}
		else if (strcmp(iter->cmd_str, "exit_on_done") == 0)
		{
		    ret_val = SMPD_DBS_RETURN;
		    /*
		    if (strcmp(str, SMPD_SUCCESS_STR) == 0)
		    {
			ret_val = SMPD_SUCCESS;
		    }
		    else
		    {
			smpd_err_printf("exit_on_done failed: %s\n", str);
			ret_val = SMPD_ABORT;
		    }
		    */
		}
		else if (strcmp(iter->cmd_str, "suspend") == 0)
		{
		    smpd_dbg_printf("suspend command result returned: %s\n", str);
		    ret_val = smpd_handle_suspend_result(iter, str);
		}
		else if (strcmp(iter->cmd_str, "add_job") == 0)
		{
		    /* print the result of the add_job command */
		    printf("%s\n", str);
		    /* close the session */
		    ret_val = smpd_create_command("done", smpd_process.id, context->id, SMPD_FALSE, &cmd_ptr);
		    if (ret_val == SMPD_SUCCESS)
		    {
			ret_val = smpd_post_write_command(context, cmd_ptr);
			if (ret_val == SMPD_SUCCESS)
			{
			    ret_val = SMPD_CLOSE;
			}
			else
			{
			    smpd_err_printf("unable to post a write of a done command.\n");
			}
		    }
		    else
		    {
			smpd_err_printf("unable to create a done command.\n");
		    }
		}
		else if (strcmp(iter->cmd_str, "add_job_and_password") == 0)
		{
		    /* print the result of the add_job_and_password command */
		    printf("%s\n", str);
		    /* close the session */
		    ret_val = smpd_create_command("done", smpd_process.id, context->id, SMPD_FALSE, &cmd_ptr);
		    if (ret_val == SMPD_SUCCESS)
		    {
			ret_val = smpd_post_write_command(context, cmd_ptr);
			if (ret_val == SMPD_SUCCESS)
			{
			    ret_val = SMPD_CLOSE;
			}
			else
			{
			    smpd_err_printf("unable to post a write of a done command.\n");
			}
		    }
		    else
		    {
			smpd_err_printf("unable to create a done command.\n");
		    }
		}
		else if (strcmp(iter->cmd_str, "remove_job") == 0)
		{
		    /* print the result of the remove_job command */
		    printf("%s\n", str);
		    /* close the session */
		    ret_val = smpd_create_command("done", smpd_process.id, context->id, SMPD_FALSE, &cmd_ptr);
		    if (ret_val == SMPD_SUCCESS)
		    {
			ret_val = smpd_post_write_command(context, cmd_ptr);
			if (ret_val == SMPD_SUCCESS)
			{
			    ret_val = SMPD_CLOSE;
			}
			else
			{
			    smpd_err_printf("unable to post a write of a done command.\n");
			}
		    }
		    else
		    {
			smpd_err_printf("unable to create a done command.\n");
		    }
		}
		else if (strcmp(iter->cmd_str, "associate_job") == 0)
		{
		    /* print the result of the associate_job command */
		    printf("%s\n", str);
		    /* close the session */
		    ret_val = smpd_create_command("done", smpd_process.id, context->id, SMPD_FALSE, &cmd_ptr);
		    if (ret_val == SMPD_SUCCESS)
		    {
			ret_val = smpd_post_write_command(context, cmd_ptr);
			if (ret_val == SMPD_SUCCESS)
			{
			    ret_val = SMPD_CLOSE;
			}
			else
			{
			    smpd_err_printf("unable to post a write of a done command.\n");
			}
		    }
		    else
		    {
			smpd_err_printf("unable to create a done command.\n");
		    }
		}
		else
		{
		    smpd_err_printf("result returned for unhandled '%s' command:\n command: '%s'\n result: '%s'\n", iter->cmd_str, iter->cmd, str);
		}
	    }
	    else
	    {
		smpd_err_printf("no result string in the result command.\n");
	    }
	    if (trailer == iter)
	    {
		context->wait_list = context->wait_list->next;
	    }
	    else
	    {
		trailer->next = iter->next;
	    }
	    result = smpd_free_command(iter);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to free command in the wait_list\n");
	    }
	    smpd_exit_fn(FCNAME);
	    return ret_val;
	}
	else
	{
	    smpd_dbg_printf("tag %d != match_tag %d\n", iter->tag, match_tag);
	}
	if (trailer != iter)
	    trailer = trailer->next;
	iter = iter->next;
    }
    if (context->wait_list == NULL)
    {
	smpd_err_printf("result command received but the wait_list is empty.\n");
    }
    else
    {
	smpd_err_printf("result command did not match any commands in the wait_list.\n");
    }
    smpd_exit_fn(FCNAME);
    return SMPD_FAIL;
}

#undef FCNAME
#define FCNAME "get_name_key_value"
static int get_name_key_value(char *str, char *name, char *key, char *value)
{
    smpd_enter_fn(FCNAME);

    if (str == NULL)
    {
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    if (name != NULL)
    {
	if (MPIU_Str_get_string_arg(str, "name", name, SMPD_MAX_DBS_NAME_LEN) != MPIU_STR_SUCCESS)
	{
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
    }
    if (key != NULL)
    {
	if (MPIU_Str_get_string_arg(str, "key", key, SMPD_MAX_DBS_KEY_LEN) != MPIU_STR_SUCCESS)
	{
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
    }
    if (value != NULL)
    {
	if (MPIU_Str_get_string_arg(str, "value", value, SMPD_MAX_DBS_VALUE_LEN) != MPIU_STR_SUCCESS)
	{
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
    }
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_handle_dbs_command"
int smpd_handle_dbs_command(smpd_context_t *context)
{
    int result;
    smpd_command_t *cmd, *temp_cmd;
    char name[SMPD_MAX_DBS_NAME_LEN+1] = "";
    char key[SMPD_MAX_DBS_KEY_LEN+1] = "";
    char value[SMPD_MAX_DBS_VALUE_LEN+1] = "";
    char ctx_key[100];
    char *result_str;

    smpd_enter_fn(FCNAME);

    cmd = &context->read_cmd;

    /*
    printf("handling dbs command on %s context, sock %d.\n", smpd_get_context_str(context), SMPDU_Sock_get_sock_id(context->sock));
    fflush(stdout);
    */

    /* prepare the result command */
    result = smpd_create_command("result", smpd_process.id, cmd->src, SMPD_FALSE, &temp_cmd);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to create a result command for the dbs command '%s'.\n", cmd->cmd);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    /* add the command tag for result matching */
    result = smpd_add_command_int_arg(temp_cmd, "cmd_tag", cmd->tag);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the tag to the result command for dbs command '%s'.\n", cmd->cmd);
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
	smpd_err_printf("no ctx_key in the db command: '%s'\n", cmd->cmd);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    result = smpd_add_command_arg(temp_cmd, "ctx_key", ctx_key);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the ctx_key to the result command for dbs command '%s'.\n", cmd->cmd);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    /* check to make sure this node is running the dbs */
    if (!smpd_process.have_dbs)
    {
	/*
	printf("havd_dbs is false for this process %s context.\n", smpd_get_context_str(context));
	fflush(stdout);
	*/

	/* create a failure reply because this node does not have an initialized database */
	smpd_dbg_printf("sending a failure reply because this node does not have an initialized database.\n");
	result = smpd_add_command_arg(temp_cmd, "result", SMPD_FAIL_STR" - smpd does not have an initialized database.");
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to add the result string to the result command for dbs command '%s'.\n", cmd->cmd);
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
	smpd_dbg_printf("sending result command to %s context: \"%s\"\n", smpd_get_context_str(context), temp_cmd->cmd);
	result = smpd_post_write_command(context, temp_cmd);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to post a write of the result command to the context: cmd '%s', dbs cmd '%s'", temp_cmd->cmd, cmd->cmd);
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
	smpd_exit_fn(FCNAME);
	return SMPD_SUCCESS;
    }

    /* do the dbs request */
    if (strcmp(cmd->cmd_str, "dbput") == 0)
    {
	if (get_name_key_value(cmd->cmd, name, key, value) != SMPD_SUCCESS)
	    goto invalid_dbs_command;
	if (smpd_dbs_put(name, key, value) == SMPD_SUCCESS)
	    result_str = DBS_SUCCESS_STR;
	else
	    result_str = DBS_FAIL_STR;
    }
    else if (strcmp(cmd->cmd_str, "dbget") == 0)
    {
	if (get_name_key_value(cmd->cmd, name, key, NULL) != SMPD_SUCCESS)
	    goto invalid_dbs_command;
	if (smpd_dbs_get(name, key, value) == SMPD_SUCCESS)
	{
	    result = smpd_add_command_arg(temp_cmd, "value", value);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to add the get value('%s') to the result command.\n", value);
		smpd_exit_fn(FCNAME);
		return SMPD_FAIL;
	    }
	    result_str = DBS_SUCCESS_STR;
	}
	else
	    result_str = DBS_FAIL_STR;
    }
    else if (strcmp(cmd->cmd_str, "dbcreate") == 0)
    {
	if (MPIU_Str_get_string_arg(cmd->cmd, "name", name, SMPD_MAX_DBS_NAME_LEN) == MPIU_STR_SUCCESS)
	{
	    result = smpd_dbs_create_name_in(name);
	}
	else
	{
	    result = smpd_dbs_create(name);
	}
	if (result == SMPD_SUCCESS)
	{
	    result = smpd_add_command_arg(temp_cmd, "name", name);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to add the dbcreate name('%s') to the result command.\n", name);
		smpd_exit_fn(FCNAME);
		return SMPD_FAIL;
	    }
	    result_str = DBS_SUCCESS_STR;
	}
	else
	    result_str = DBS_FAIL_STR;
    }
    else if (strcmp(cmd->cmd_str, "dbdestroy") == 0)
    {
	if (get_name_key_value(cmd->cmd, name, NULL, NULL) != SMPD_SUCCESS)
	    goto invalid_dbs_command;
	if (smpd_dbs_destroy(name) == SMPD_SUCCESS)
	    result_str = DBS_SUCCESS_STR;
	else
	    result_str = DBS_FAIL_STR;
    }
    else if (strcmp(cmd->cmd_str, "dbfirst") == 0)
    {
	if (get_name_key_value(cmd->cmd, name, NULL, NULL) != SMPD_SUCCESS)
	    goto invalid_dbs_command;
	if (smpd_dbs_first(name, key, value) == SMPD_SUCCESS)
	{
	    if (*key == '\0')
	    {
		/* this can be changed to a special end_key if we don't want DBS_END_STR to be a reserved key */
		result = smpd_add_command_arg(temp_cmd, "key", DBS_END_STR);
		if (result != SMPD_SUCCESS)
		{
		    smpd_err_printf("unable to add the dbfirst key('%s') to the result command.\n", key);
		    smpd_exit_fn(FCNAME);
		    return SMPD_FAIL;
		}
	    }
	    else
	    {
		result = smpd_add_command_arg(temp_cmd, "key", key);
		if (result != SMPD_SUCCESS)
		{
		    smpd_err_printf("unable to add the dbfirst key('%s') to the result command.\n", key);
		    smpd_exit_fn(FCNAME);
		    return SMPD_FAIL;
		}
		result = smpd_add_command_arg(temp_cmd, "value", value);
		if (result != SMPD_SUCCESS)
		{
		    smpd_err_printf("unable to add the dbfirst value('%s') to the result command.\n", value);
		    smpd_exit_fn(FCNAME);
		    return SMPD_FAIL;
		}
	    }
	    result_str = DBS_SUCCESS_STR;
	}
	else
	{
	    result_str = DBS_FAIL_STR;
	}
    }
    else if (strcmp(cmd->cmd_str, "dbnext") == 0)
    {
	if (get_name_key_value(cmd->cmd, name, NULL, NULL) != SMPD_SUCCESS)
	    goto invalid_dbs_command;
	if (smpd_dbs_next(name, key, value) == SMPD_SUCCESS)
	{
	    if (*key == '\0')
	    {
		/* this can be changed to a special end_key if we don't want DBS_END_STR to be a reserved key */
		result = smpd_add_command_arg(temp_cmd, "key", DBS_END_STR);
		if (result != SMPD_SUCCESS)
		{
		    smpd_err_printf("unable to add the dbndext key('%s') to the result command.\n", key);
		    smpd_exit_fn(FCNAME);
		    return SMPD_FAIL;
		}
	    }
	    else
	    {
		result = smpd_add_command_arg(temp_cmd, "key", key);
		if (result != SMPD_SUCCESS)
		{
		    smpd_err_printf("unable to add the dbnext key('%s') to the result command.\n", key);
		    smpd_exit_fn(FCNAME);
		    return SMPD_FAIL;
		}
		result = smpd_add_command_arg(temp_cmd, "value", value);
		if (result != SMPD_SUCCESS)
		{
		    smpd_err_printf("unable to add the dbnext value('%s') to the result command.\n", value);
		    smpd_exit_fn(FCNAME);
		    return SMPD_FAIL;
		}
	    }
	    result_str = DBS_SUCCESS_STR;
	}
	else
	{
	    result_str = DBS_FAIL_STR;
	}
    }
    else if (strcmp(cmd->cmd_str, "dbfirstdb") == 0)
    {
	if (smpd_dbs_firstdb(name) == SMPD_SUCCESS)
	{
	    /* this can be changed to a special end_key if we don't want DBS_END_STR to be a reserved key */
	    if (*name == '\0')
		result = smpd_add_command_arg(temp_cmd, "name", DBS_END_STR);
	    else
		result = smpd_add_command_arg(temp_cmd, "name", name);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to add the dbfirstdb name('%s') to the result command.\n", name);
		smpd_exit_fn(FCNAME);
		return SMPD_FAIL;
	    }
	    result_str = DBS_SUCCESS_STR;
	}
	else
	{
	    result_str = DBS_FAIL_STR;
	}
    }
    else if (strcmp(cmd->cmd_str, "dbnextdb") == 0)
    {
	if (smpd_dbs_nextdb(name) == SMPD_SUCCESS)
	{
	    /* this can be changed to a special end_key if we don't want DBS_END_STR to be a reserved key */
	    if (*name == '\0')
		result = smpd_add_command_arg(temp_cmd, "name", DBS_END_STR);
	    else
		result = smpd_add_command_arg(temp_cmd, "name", name);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to add the dbnextdb name('%s') to the result command.\n", name);
		smpd_exit_fn(FCNAME);
		return SMPD_FAIL;
	    }
	    result_str = DBS_SUCCESS_STR;
	}
	else
	{
	    result_str = DBS_FAIL_STR;
	}
    }
    else if (strcmp(cmd->cmd_str, "dbdelete") == 0)
    {
	if (get_name_key_value(cmd->cmd, name, key, NULL) != SMPD_SUCCESS)
	    goto invalid_dbs_command;
	if (smpd_dbs_delete(name, key) == SMPD_SUCCESS)
	    result_str = DBS_SUCCESS_STR;
	else
	    result_str = DBS_FAIL_STR;
    }
    else
    {
	smpd_err_printf("unknown dbs command '%s', replying with failure.\n", cmd->cmd_str);
	goto invalid_dbs_command;
    }

    /* send the reply */
    smpd_dbg_printf("sending reply to dbs command '%s'.\n", cmd->cmd);
    result = smpd_add_command_arg(temp_cmd, "result", result_str);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the result string to the result command for dbs command '%s'.\n", cmd->cmd);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    smpd_dbg_printf("sending result command to %s context: \"%s\"\n", smpd_get_context_str(context), temp_cmd->cmd);
    result = smpd_post_write_command(context, temp_cmd);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to post a write of the result command to the context: cmd '%s', dbs cmd '%s'", temp_cmd->cmd, cmd->cmd);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;

invalid_dbs_command:

    result = smpd_add_command_arg(temp_cmd, "result", SMPD_FAIL_STR" - invalid dbs command.");
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the result string to the result command for dbs command '%s'.\n", cmd->cmd);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    smpd_dbg_printf("sending result command to %s context: \"%s\"\n", smpd_get_context_str(context), temp_cmd->cmd);
    result = smpd_post_write_command(context, temp_cmd);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to post a write of the result command to the context: cmd '%s', dbs cmd '%s'", temp_cmd->cmd, cmd->cmd);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_handle_die_command"
int smpd_handle_die_command(smpd_context_t *context){
    int result;
    smpd_enter_fn(FCNAME);
    context->state = SMPD_DONE;
    result = SMPDU_Sock_post_close(context->sock);
    if(result != SMPD_SUCCESS){
        smpd_err_printf("Unable to post a close after 'die' on a singleton client, error = %s\n",
                    get_sock_error_string(result));
        smpd_exit_fn(FCNAME);
        return SMPD_FAIL;
    }
    smpd_exit_fn(FCNAME);
    return SMPD_CLOSE;
}

#undef FCNAME
#define FCNAME "smpd_handle_proc_info_command"
int smpd_handle_proc_info_command(smpd_context_t *context){
    int result;
    smpd_command_t *cmd, *temp_cmd;
    char ctx_key[100];
    smpd_enter_fn(FCNAME);
    cmd = &context->read_cmd;
    /* Prepare a reply cmd */
    /* prepare the result command */
    result = smpd_create_command("result", smpd_process.id, cmd->src, SMPD_FALSE, &temp_cmd);
    if (result != SMPD_SUCCESS){
	    smpd_err_printf("unable to create a result command for the 'proc_info' command '%s'.\n", cmd->cmd);
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
    }
    /* add the command tag for result matching */
    result = smpd_add_command_int_arg(temp_cmd, "cmd_tag", cmd->tag);
    if (result != SMPD_SUCCESS){
	    smpd_err_printf("unable to add the tag to the result command for 'proc_info' command '%s'.\n", cmd->cmd);
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
    }
    result = smpd_add_command_arg(temp_cmd, "cmd_orig", cmd->cmd_str);
    if (result != SMPD_SUCCESS){
	    smpd_err_printf("unable to add cmd_orig to the result command for a %s command\n", cmd->cmd_str);
    	smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
    }
    /* copy the ctx_key for pmi control channel lookup */
    if (MPIU_Str_get_string_arg(cmd->cmd, "ctx_key", ctx_key, 100) != MPIU_STR_SUCCESS){
	    smpd_err_printf("no ctx_key in the 'proc_info' command: '%s'\n", cmd->cmd);
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
    }
    result = smpd_add_command_arg(temp_cmd, "ctx_key", ctx_key);
    if (result != SMPD_SUCCESS){
	    smpd_err_printf("unable to add the ctx_key to the result command for 'proc_info' command '%s'.\n", cmd->cmd);
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
    }

    /* Get the process info */
    MPIU_Str_get_string_arg(cmd->cmd, "c", context->process->exe, SMPD_MAX_EXE_LENGTH);
    MPIU_Str_get_int_arg(cmd->cmd, "i", &context->process->rank);
    MPIU_Str_get_int_arg(cmd->cmd, "n", &context->process->nproc);
    if(MPIU_Str_get_int_arg(cmd->cmd, "s", &result) == MPIU_STR_SUCCESS){
        context->process->is_singleton_client = result ? SMPD_TRUE : SMPD_FALSE;    
    }
    /* For unix systems - get the PID. On windows a handle to a process has to be explicitly duplicated with
     * appropriate rights 
     */
#ifndef HAVE_WINDOWS_H
    MPIU_Str_get_int_arg(cmd->cmd, "p", &(context->process->wait));
#endif
    /* send the reply */

    smpd_dbg_printf("sending reply to 'proc_info' command '%s'.\n", cmd->cmd);
    result = smpd_add_command_arg(temp_cmd, "result", SMPD_SUCCESS_STR);
    if (result != SMPD_SUCCESS){
	    smpd_err_printf("unable to add the result string to the result command for 'proc_info' command '%s'.\n", cmd->cmd);
    	smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
    }
    smpd_dbg_printf("sending result command to %s context: \"%s\"\n", smpd_get_context_str(context), temp_cmd->cmd);
    result = smpd_post_write_command(context, temp_cmd);
    if (result != SMPD_SUCCESS){
	    smpd_err_printf("unable to post a write of the result command to the context: cmd '%s', dbs cmd '%s'", temp_cmd->cmd, cmd->cmd);
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
    }

    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#define SMPD_LAUNCH_WARNING_MSG_LEN 256
#undef FCNAME
#define FCNAME "smpd_handle_launch_command"
int smpd_handle_launch_command(smpd_context_t *context)
{
    int result;
    char err_msg[256];
    char wMsg[SMPD_LAUNCH_WARNING_MSG_LEN];
    char *pWMsg = wMsg;
    int wMsgCurLen = SMPD_LAUNCH_WARNING_MSG_LEN;
    int iproc;
    int i, nmaps;
    char drive_arg_str[20];
    smpd_command_t *cmd, *temp_cmd;
    smpd_process_t *process;
    char share[SMPD_MAX_EXE_LENGTH];
    int priority_class = SMPD_DEFAULT_PRIORITY_CLASS, priority_thread = SMPD_DEFAULT_PRIORITY;

    smpd_enter_fn(FCNAME);
    wMsg[0] = '\0';

    cmd = &context->read_cmd;

    /* parse the command */
    if (MPIU_Str_get_int_arg(cmd->cmd, "i", &iproc) != MPIU_STR_SUCCESS)
	iproc = 0;
    result = smpd_create_process_struct(iproc, &process);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to create a process structure.\n");
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    if (MPIU_Str_get_string_arg(cmd->cmd, "c", process->exe, SMPD_MAX_EXE_LENGTH) != MPIU_STR_SUCCESS)
    {
	smpd_err_printf("launch command received with no executable: '%s'\n", cmd->cmd);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    MPIU_Str_get_string_arg(cmd->cmd, "e", process->env, SMPD_MAX_ENV_LENGTH);
    MPIU_Str_get_string_arg(cmd->cmd, "d", process->dir, SMPD_MAX_DIR_LENGTH);
    MPIU_Str_get_string_arg(cmd->cmd, "p", process->path, SMPD_MAX_PATH_LENGTH);
    MPIU_Str_get_string_arg(cmd->cmd, "k", process->kvs_name, SMPD_MAX_DBS_NAME_LEN);
    MPIU_Str_get_string_arg(cmd->cmd, "kd", process->domain_name, SMPD_MAX_DBS_NAME_LEN);
    MPIU_Str_get_string_arg(cmd->cmd, "q", process->clique, SMPD_MAX_CLIQUE_LENGTH);
    MPIU_Str_get_int_arg(cmd->cmd, "n", &process->nproc);
    MPIU_Str_get_int_arg(cmd->cmd, "s", &process->spawned);
    MPIU_Str_get_int_arg(cmd->cmd, "a", &process->appnum);
    MPIU_Str_get_int_arg(cmd->cmd, "pc", &priority_class);
    MPIU_Str_get_int_arg(cmd->cmd, "pt", &priority_thread);
#ifdef HAVE_WINDOWS_H
    MPIU_Str_get_int_arg(cmd->cmd, "af", &smpd_process.set_affinity);
    MPIU_Str_get_int_arg(cmd->cmd, "afp", &process->binding_proc);
#endif
    /* parse the -m drive mapping options */
    nmaps = 0;
    MPIU_Str_get_int_arg(cmd->cmd, "mn", &nmaps);
    if (nmaps > 0)
    {
	/* make a linked list out of one big array */
	process->map_list = (smpd_map_drive_node_t *)MPIU_Malloc(sizeof(smpd_map_drive_node_t) * nmaps);
    }
    for (i=0; i<nmaps; i++)
    {
	if (i == nmaps-1)
	    process->map_list[i].next = NULL;
	else
	    process->map_list[i].next = &process->map_list[i+1];
	process->map_list[i].ref_count = 0;
	sprintf(drive_arg_str, "m%d", i);
	result = MPIU_Str_get_string_arg(cmd->cmd, drive_arg_str, share, SMPD_MAX_EXE_LENGTH);
	if (result == MPIU_STR_SUCCESS)
	 {
	    process->map_list[i].drive = share[0];
	    strncpy(process->map_list[i].share, &share[2], SMPD_MAX_EXE_LENGTH);

#ifdef HAVE_WINDOWS_H
	    /* map the drive */
	    /* FIXME: username and password needed to map a drive */
	    result = smpd_map_user_drives(share, NULL, NULL, pWMsg, wMsgCurLen);
	    if (result != SMPD_TRUE)
	    {
        int wMsgAddedLen=0;
		smpd_err_printf("mapping drive failed: input string - %s, error - %s\n", share, pWMsg);
	    process->map_list[i].drive = '\0';
	    process->map_list[i].share[0] = '\0';
        wMsgAddedLen = strlen(pWMsg);
        pWMsg += wMsgAddedLen;
        wMsgCurLen -= wMsgAddedLen;
        if(wMsgCurLen <= 1){
            /* Circular buffer */
            pWMsg = wMsg;
            wMsgCurLen = SMPD_LAUNCH_WARNING_MSG_LEN;
        }
        /* Don't abort if we fail to map a drive */
        /*
		smpd_free_process_struct(process);
		smpd_exit_fn(FCNAME);
		return SMPD_FAIL;
        */
	    }
#endif
	}
	else
	{
	    process->map_list[i].drive = '\0';
	    process->map_list[i].share[0] = '\0';
	}
    }

    /* launch the process */
    smpd_dbg_printf("launching: '%s'\n", process->exe);
    result = smpd_launch_process(process, priority_class, priority_thread, SMPD_FALSE, context->set);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("launch_process failed.\n");

	/* create the result command */
	result = smpd_create_command("result", smpd_process.id, cmd->src, SMPD_FALSE, &temp_cmd);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to create a result command in response to launch command: '%s'\n", cmd->cmd);
	    smpd_free_process_struct(process);
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
	result = smpd_add_command_int_arg(temp_cmd, "cmd_tag", cmd->tag);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to add the tag to the result command in response to launch command: '%s'\n", cmd->cmd);
	    smpd_free_process_struct(process);
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
	/* launch process should provide a reason for the error, for now just return FAIL */
	result = smpd_add_command_arg(temp_cmd, "result", SMPD_FAIL_STR);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to add the result field to the result command in response to launch command: '%s'\n", cmd->cmd);
	    smpd_free_process_struct(process);
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
	if (process->err_msg[0] != '\0')
	{
	    result = smpd_add_command_arg(temp_cmd, "error", process->err_msg);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to add the error field to the result command in response to launch command: '%s'\n", cmd->cmd);
		smpd_free_process_struct(process);
		smpd_exit_fn(FCNAME);
		return SMPD_FAIL;
	    }
	}

	/* send the result back */
	result = smpd_post_write_command(context, temp_cmd);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to post a write of the result command in response to launch command: '%s'\n", cmd->cmd);
	    smpd_free_process_struct(process);
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}

	/* free the failed process structure */
	smpd_free_process_struct(process);
	process = NULL;

	smpd_exit_fn(FCNAME);
	return SMPD_SUCCESS;
    }

    /* save the new process in the list */
    process->next = smpd_process.process_list;
    smpd_process.process_list = process;

    /* create the result command */
    result = smpd_create_command("result", smpd_process.id, cmd->src, SMPD_FALSE, &temp_cmd);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to create a result command in response to launch command: '%s'\n", cmd->cmd);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    result = smpd_add_command_int_arg(temp_cmd, "cmd_tag", cmd->tag);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the tag to the result command in response to launch command: '%s'\n", cmd->cmd);
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
    result = smpd_add_command_arg(temp_cmd, "result", SMPD_SUCCESS_STR);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the result field to the result command in response to launch command: '%s'\n", cmd->cmd);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    result = smpd_add_command_arg(temp_cmd, "pg_id", process->kvs_name);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the pg_id field to the result command in response to launch command: '%s'\n", cmd->cmd);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    result = smpd_add_command_int_arg(temp_cmd, "pg_rank", process->rank);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the pg_rank field to the result command in response to launch command: '%s'\n", cmd->cmd);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    result = smpd_add_command_int_arg(temp_cmd, "pg_ctx", process->id);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the pg_ctx field to the result command in response to launch command: '%s'\n", cmd->cmd);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    
    if(wMsgCurLen < SMPD_LAUNCH_WARNING_MSG_LEN){
        result = smpd_add_command_arg(temp_cmd, "warning", wMsg);
        if(result != SMPD_SUCCESS){
            /* Don't abort if we cannot add the warning message... */
            smpd_err_printf("unable to add warning message to the result command in response to launch command '%s'\n", cmd->cmd);
        }
    }

    /* send the result back */
    result = smpd_post_write_command(context, temp_cmd);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to post a write of the result command in response to launch command: '%s'\n", cmd->cmd);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_handle_close_command"
int smpd_handle_close_command(smpd_context_t *context)
{
    int result;
    smpd_command_t *cmd, *temp_cmd;

    smpd_enter_fn(FCNAME);

    cmd = &context->read_cmd;

    smpd_process.closing = SMPD_TRUE;
    if (smpd_process.left_context || smpd_process.right_context)
    {
	if (smpd_process.left_context)
	{
	    result = smpd_create_command("close", smpd_process.id, smpd_process.left_context->id, SMPD_FALSE, &temp_cmd);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to create a close command for the left context.\n");
		smpd_exit_fn(FCNAME);
		return SMPD_FAIL;
	    }
	    smpd_dbg_printf("sending close command to left child: \"%s\"\n", temp_cmd->cmd);
	    result = smpd_post_write_command(smpd_process.left_context, temp_cmd);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to post a write of a close command for the left context.\n");
		smpd_exit_fn(FCNAME);
		return SMPD_FAIL;
	    }
	}
	if (smpd_process.right_context)
	{
	    result = smpd_create_command("close", smpd_process.id, smpd_process.right_context->id, SMPD_FALSE, &temp_cmd);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to create a close command for the right context.\n");
		smpd_exit_fn(FCNAME);
		return SMPD_FAIL;
	    }
	    smpd_dbg_printf("sending close command to right child: \"%s\"\n", temp_cmd->cmd);
	    result = smpd_post_write_command(smpd_process.right_context, temp_cmd);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to post a write of a close command for the right context.\n");
		smpd_exit_fn(FCNAME);
		return SMPD_FAIL;
	    }
	}
	smpd_exit_fn(FCNAME);
	/* If we return success here, a post_read will be made for the next command header. */
	return SMPD_SUCCESS;
	/*return SMPD_CLOSE;*/
    }
    result = smpd_create_command("closed", smpd_process.id, cmd->src, SMPD_FALSE, &temp_cmd);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to create a closed command for the parent context.\n");
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    smpd_dbg_printf("sending closed command to parent: \"%s\"\n", temp_cmd->cmd);
    result = smpd_post_write_command(context, temp_cmd);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to post a write of the closed command to the parent context.\n");
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    smpd_dbg_printf("posted closed command.\n");

    smpd_exit_fn(FCNAME);
    return SMPD_CLOSE;
}

#undef FCNAME
#define FCNAME "smpd_handle_closed_command"
int smpd_handle_closed_command(smpd_context_t *context)
{
    int result;
    smpd_command_t *cmd, *temp_cmd;

    smpd_enter_fn(FCNAME);

    cmd = &context->read_cmd;

    if (context == smpd_process.left_context)
    {
	smpd_dbg_printf("closed command received from left child, closing sock.\n");
	smpd_dbg_printf("SMPDU_Sock_post_close(%d)\n", SMPDU_Sock_get_sock_id(smpd_process.left_context->sock));
	smpd_process.left_context->state = SMPD_CLOSING;
	SMPDU_Sock_post_close(smpd_process.left_context->sock);
	if (smpd_process.right_context)
	{
	    smpd_exit_fn(FCNAME);
	    return SMPD_CLOSE;
	}
	/* fall through if there are no more contexts */
    }
    else if (context == smpd_process.right_context)
    {
	smpd_dbg_printf("closed command received from right child, closing sock.\n");
	smpd_dbg_printf("SMPDU_Sock_post_close(%d)\n", SMPDU_Sock_get_sock_id(smpd_process.right_context->sock));
	smpd_process.right_context->state = SMPD_CLOSING;
	SMPDU_Sock_post_close(smpd_process.right_context->sock);
	if (smpd_process.left_context)
	{
	    smpd_exit_fn(FCNAME);
	    return SMPD_CLOSE;
	}
	/* fall through if there are no more contexts */
    }
    else if (context == smpd_process.parent_context)
    {
	/* closed command received from the parent in response to an earlier closed_request command from this node */
	smpd_dbg_printf("closed command received from parent, closing sock.\n");
	smpd_dbg_printf("SMPDU_Sock_post_close(%d)\n", SMPDU_Sock_get_sock_id(smpd_process.parent_context->sock));
	smpd_process.parent_context->state = SMPD_CLOSING;
	SMPDU_Sock_post_close(smpd_process.parent_context->sock);
	smpd_exit_fn(FCNAME);
	return SMPD_EXITING;
    }
    else
    {
	smpd_err_printf("closed command received from unknown context.\n");
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    if (smpd_process.parent_context == NULL)
    {
	/* The last child has closed and there is no parent so this must be the root node (mpiexec) */
	/* Set the state to SMPD_EXITING so when the posted close from above finishes the state machine will exit */
	context->state = SMPD_EXITING;
	smpd_dbg_printf("received a closed at node with no parent context, assuming root, returning SMPD_EXITING.\n");
	smpd_exit_fn(FCNAME);
	return SMPD_EXITING;
    }

    /* The last child has closed and there is a parent so send a closed_request up to the parent */
    /* The parent will respond with a closed command and then this node will exit */
    result = smpd_create_command("closed_request", smpd_process.id, smpd_process.parent_context->id, SMPD_FALSE, &temp_cmd);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to create a closed_request command for the parent context.\n");
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    /*smpd_dbg_printf("posting write of closed_request command to parent: \"%s\"\n", temp_cmd->cmd);*/
    result = smpd_post_write_command(smpd_process.parent_context, temp_cmd);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to post a write of the closed_request command to the parent context.\n");
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
	
    smpd_exit_fn(FCNAME);
    return SMPD_CLOSE;
}

#undef FCNAME
#define FCNAME "smpd_handle_closed_request_command"
int smpd_handle_closed_request_command(smpd_context_t *context)
{
    int result;
    smpd_command_t *cmd, *temp_cmd;

    smpd_enter_fn(FCNAME);

    cmd = &context->read_cmd;

    result = smpd_create_command("closed", smpd_process.id, context->id, SMPD_FALSE, &temp_cmd);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to create a closed command for the context.\n");
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    smpd_dbg_printf("sending closed command to context: \"%s\"\n", temp_cmd->cmd);
    result = smpd_post_write_command(context, temp_cmd);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to post a write of the closed command to the context.\n");
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    smpd_dbg_printf("posted closed command to context.\n");
    smpd_exit_fn(FCNAME);
    return SMPD_CLOSE;
}

#undef FCNAME
#define FCNAME "smpd_handle_connect_command"
int smpd_handle_connect_command(smpd_context_t *context)
{
    int result;
    smpd_command_t *cmd, *temp_cmd;
    smpd_context_t *dest;
    SMPDU_Sock_set_t dest_set;
    SMPDU_Sock_t dest_sock;
    int dest_id;
    char host[SMPD_MAX_HOST_LENGTH];
    char plaintext[4];

    smpd_enter_fn(FCNAME);

    cmd = &context->read_cmd;

    if (smpd_process.root_smpd)
    {
	smpd_err_printf("the root smpd is not allowed to connect to other smpds.\n");
	/* send connect failed return command */
	result = smpd_create_command("result", smpd_process.id, cmd->src, SMPD_FALSE, &temp_cmd);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to create a result command for the connect request.\n");
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
	result = smpd_add_command_int_arg(temp_cmd, "cmd_tag", cmd->tag);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to add the tag to the result command.\n");
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
	result = smpd_add_command_arg(temp_cmd, "result", SMPD_FAIL_STR" - root smpd is not allowed to connect to other smpds.");
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to add the result string to the result command.\n");
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
	smpd_dbg_printf("sending result command to context: \"%s\"\n", temp_cmd->cmd);
	result = smpd_post_write_command(context, temp_cmd);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to post a write of the result command to the context.\n");
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
	smpd_exit_fn(FCNAME);
	return SMPD_SUCCESS;
    }
    if (smpd_process.closing)
    {
	smpd_err_printf("connect command received while session is closing, ignoring connect.\n");
	result = smpd_create_command("result", smpd_process.id, cmd->src, SMPD_FALSE, &temp_cmd);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to create a result command for the connect request.\n");
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
	result = smpd_add_command_int_arg(temp_cmd, "cmd_tag", cmd->tag);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to add the tag to the result command.\n");
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
	result = smpd_add_command_arg(temp_cmd, "result", SMPD_FAIL_STR" - connect command received while closing.");
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to add the result string to the result command.\n");
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
	smpd_dbg_printf("sending result command to context: \"%s\"\n", temp_cmd->cmd);
	result = smpd_post_write_command(context, temp_cmd);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to post a write of the result command to the context.\n");
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
	smpd_exit_fn(FCNAME);
	return SMPD_SUCCESS;
    }
    if (MPIU_Str_get_string_arg(cmd->cmd, "host", host, SMPD_MAX_HOST_LENGTH) != MPIU_STR_SUCCESS)
    {
	smpd_err_printf("connect command does not have a target host argument, discarding: \"%s\"\n", cmd->cmd);
	/* return failure result */
	smpd_exit_fn(FCNAME);
	return SMPD_SUCCESS;
    }
    if (MPIU_Str_get_int_arg(cmd->cmd, "id", &dest_id) != MPIU_STR_SUCCESS)
    {
	smpd_err_printf("connect command does not have a target id argument, discarding: \"%s\"\n", cmd->cmd);
	/* return failure result */
	smpd_exit_fn(FCNAME);
	return SMPD_SUCCESS;
    }
    if (MPIU_Str_get_string_arg(cmd->cmd, "plaintext", plaintext, 4) == MPIU_STR_SUCCESS)
    {
	if (strncmp(plaintext, "yes", 4) == 0)
	{
	    smpd_dbg_printf("setting smpd_process.plaintext due to plaintext option in the connect command\n");
	    smpd_process.plaintext = SMPD_TRUE;
	}
    }
    if (dest_id < smpd_process.id)
    {
	smpd_dbg_printf("connect command has an invalid id, discarding: %d\n", dest_id);
	/* return failure result */
	smpd_exit_fn(FCNAME);
	return SMPD_SUCCESS;
    }
    if (smpd_process.left_context != NULL && smpd_process.right_context != NULL)
    {
	smpd_err_printf("unable to connect to a new session, left and right sessions already exist, discarding.\n");
	/* return failure result */
	smpd_exit_fn(FCNAME);
	return SMPD_SUCCESS;
    }
    smpd_dbg_printf("now connecting to %s\n", host);
    /* create a new context */
    result = smpd_create_context(SMPD_CONTEXT_UNDETERMINED, context->set, SMPDU_SOCK_INVALID_SOCK, dest_id, &dest);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to create a new context.\n");
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    dest_set = context->set; /*smpd_process.set;*/

    /* start the connection logic here */
    result = SMPDU_Sock_post_connect(dest_set, dest, host, smpd_process.port, &dest_sock);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to post a connect to start the connect command,\nsock error: %s\n",
	    get_sock_error_string(result));
	result = smpd_post_abort_command("Unable to connect to '%s:%d',\nsock error: %s\n",
	    host, smpd_process.port, get_sock_error_string(result));
	smpd_exit_fn(FCNAME);
	/*return SMPD_FAIL;*/
	return result;
    }

    if (smpd_process.left_context == NULL)
    {
	smpd_dbg_printf("adding new left child context\n");
	smpd_init_context(dest, SMPD_CONTEXT_LEFT_CHILD, dest_set, dest_sock, dest_id);
	smpd_process.left_context = dest;
	SMPDU_Sock_set_user_ptr(dest_sock, dest);
    }
    else if (smpd_process.right_context == NULL)
    {
	smpd_dbg_printf("adding new right child context\n");
	smpd_init_context(dest, SMPD_CONTEXT_RIGHT_CHILD, dest_set, dest_sock, dest_id);
	smpd_process.right_context = dest;
	SMPDU_Sock_set_user_ptr(dest_sock, dest);
    }
    else
    {
	smpd_err_printf("impossible to be here, both left and right contexts are non-NULL.\n");
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    dest->state = SMPD_CONNECTING;
    dest->connect_to = (smpd_host_node_t*)MPIU_Malloc(sizeof(smpd_host_node_t));
    if (dest->connect_to == NULL)
    {
	smpd_err_printf("unable to allocate a host node structure.\n");
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    strcpy(dest->connect_to->host, host);
    dest->connect_to->alt_host[0] = '\0';
    dest->connect_to->id = dest_id;
    dest->connect_to->nproc = 1;
    dest->connect_to->connected = SMPD_FALSE;
    dest->connect_to->connect_cmd_tag = -1;
    dest->connect_to->parent = smpd_process.id;
    dest->connect_to->next = NULL;
    dest->connect_to->left = NULL;
    dest->connect_to->right = NULL;
    dest->connect_return_id = cmd->src;
    dest->connect_return_tag = cmd->tag;

    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_handle_start_dbs_command"
int smpd_handle_start_dbs_command(smpd_context_t *context)
{
    int result;
    smpd_command_t *cmd, *temp_cmd;
    char kvs_name[SMPD_MAX_DBS_NAME_LEN];
    int npreput;
    char keyvals_str[SMPD_MAX_CMD_LENGTH];
    int j;
    char key[100], val[1024], val2[1024], equals_str[10];
    char *iter;

    smpd_enter_fn(FCNAME);

    cmd = &context->read_cmd;

    if (smpd_process.have_dbs == SMPD_FALSE)
    {
	smpd_dbs_init();
	smpd_process.have_dbs = SMPD_TRUE;
    }

    /* prepare the result command */
    result = smpd_create_command("result", smpd_process.id, cmd->src, SMPD_FALSE, &temp_cmd);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to create a result command for the dbs command '%s'.\n", cmd->cmd);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    result = smpd_add_command_int_arg(temp_cmd, "cmd_tag", cmd->tag);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the tag to the result command for dbs command '%s'.\n", cmd->cmd);
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

    /* create a db */
    result = smpd_dbs_create(/*smpd_process.*/kvs_name);
    if (result == SMPD_DBS_SUCCESS)
    {
	if (MPIU_Str_get_int_arg(cmd->cmd, "npreput", &npreput) == MPIU_STR_SUCCESS)
	{
	    smpd_dbg_printf("npreput = %d\n", npreput);
	    /*printf("npreput = %d\n", npreput);fflush(stdout);*/
	    if (MPIU_Str_get_string_arg(cmd->cmd, "preput", keyvals_str, SMPD_MAX_CMD_LENGTH) == MPIU_STR_SUCCESS)
	    {
		smpd_dbg_printf("preput string = '%s'\n", keyvals_str);
		/*printf("preput string = '%s'\n", keyvals_str);fflush(stdout);*/
		for (j=0; j<npreput; j++)
		{
		    sprintf(key, "%d", j);
		    if (MPIU_Str_get_string_arg(keyvals_str, key, val, 1024) != MPIU_STR_SUCCESS)
		    {
			smpd_err_printf("unable to get key %s from the preput keyval string '%s'.\n", key, keyvals_str);
			smpd_exit_fn(FCNAME);
			return SMPD_FAIL;
		    }
		    iter = val;
		    if (MPIU_Str_get_string(&iter, key, 100) != MPIU_STR_SUCCESS)
		    {
			smpd_err_printf("unable to get key name from the preput keyval string '%s'.\n", val);
			smpd_exit_fn(FCNAME);
			return SMPD_FAIL;
		    }
		    if (MPIU_Str_get_string(&iter, equals_str, 10) != MPIU_STR_SUCCESS)
		    {
			smpd_err_printf("unable to get equals deliminator from the preput keyval string '%s'.\n", val);
			smpd_exit_fn(FCNAME);
			return SMPD_FAIL;
		    }
		    if (MPIU_Str_get_string(&iter, val2, 1024) != MPIU_STR_SUCCESS)
		    {
			smpd_err_printf("unable to get value from the preput keyval string '%s'.\n", val);
			smpd_exit_fn(FCNAME);
			return SMPD_FAIL;
		    }
		    result = smpd_dbs_put(/*smpd_process.*/kvs_name, key, val2);
		    if (result != SMPD_DBS_SUCCESS)
		    {
			smpd_err_printf("smpd_dbs_put(%s:%s) failed.\n", key, val2);
			smpd_exit_fn(FCNAME);
			return SMPD_FAIL;
		    }
		    /*printf("preput %s = %s\n", key,val2);fflush(stdout);*/
		}
	    }
	}

	/* send the name back to the root */
	result = smpd_add_command_arg(temp_cmd, "kvs_name", /*smpd_process.*/kvs_name);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to add the kvs_name string to the result command for dbs command '%s'.\n", cmd->cmd);
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
	result = smpd_add_command_arg(temp_cmd, "domain_name", smpd_process.domain_name);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to add the domain_name string to the result command for dbs command '%s'.\n", cmd->cmd);
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
	result = smpd_add_command_arg(temp_cmd, "result", SMPD_SUCCESS_STR);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to add the result string to the result command for dbs command '%s'.\n", cmd->cmd);
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
    }
    else
    {
	smpd_err_printf("unable to create a db\n");
	/* Shoud we send a bogus kvs_name field in the command so the reader won't blow up or complain that it is not there?
	result = smpd_add_command_arg(temp_cmd, "kvs_name", "0");
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to add the kvs_name string to the result command for dbs command '%s'.\n", cmd->cmd);
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
	*/
	result = smpd_add_command_arg(temp_cmd, "result", SMPD_FAIL_STR);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to add the result string to the result command for dbs command '%s'.\n", cmd->cmd);
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
    }
    smpd_dbg_printf("sending result command to %s context: \"%s\"\n", smpd_get_context_str(context), temp_cmd->cmd);
    result = smpd_post_write_command(context, temp_cmd);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to post a write of the result command to the context: cmd '%s', dbs cmd '%s'", temp_cmd->cmd, cmd->cmd);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    smpd_exit_fn(FCNAME);
    return result;
}

#undef FCNAME
#define FCNAME "smpd_handle_print_command"
int smpd_handle_print_command(smpd_context_t *context)
{
    int result = SMPD_SUCCESS;
    smpd_command_t *cmd, *temp_cmd;

    smpd_enter_fn(FCNAME);

    cmd = &context->read_cmd;

    smpd_dbg_printf("PRINT: node %s:%d, level %d, parent = %d, left = %d, right = %d\n",
	smpd_process.host,
	smpd_process.id,
	smpd_process.level,
	/*smpd_process.parent_context ? smpd_process.parent_context->id : -1,*/
	smpd_process.parent_id,
	smpd_process.left_context ? smpd_process.left_context->id : -1,
	smpd_process.right_context ? smpd_process.right_context->id : -1);
    if (smpd_process.left_context)
    {
	result = smpd_create_command("print", smpd_process.id, smpd_process.left_context->id, SMPD_FALSE, &temp_cmd);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to create a 'print' command for the left context.\n");
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
	result = smpd_post_write_command(smpd_process.left_context, temp_cmd);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to post a write for the 'print' command to the left context.\n");
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
    }
    if (smpd_process.right_context)
    {
	result = smpd_create_command("print", smpd_process.id, smpd_process.right_context->id, SMPD_FALSE, &temp_cmd);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to create a 'print' command for the right context.\n");
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
	result = smpd_post_write_command(smpd_process.right_context, temp_cmd);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to post a write for the 'print' command to the right context.\n");
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
    }
    smpd_exit_fn(FCNAME);
    return result;
}

#undef FCNAME
#define FCNAME "smpd_handle_stat_command"
int smpd_handle_stat_command(smpd_context_t *context)
{
    int result;
    smpd_command_t *cmd, *temp_cmd, *cmd_iter;
    char param[100];
    char result_str[SMPD_MAX_CMD_LENGTH-100];
    smpd_context_t *iter;
    char *str;
    int len;

    smpd_enter_fn(FCNAME);

    cmd = &context->read_cmd;

    if (MPIU_Str_get_string_arg(cmd->cmd, "param", param, 1024) != MPIU_STR_SUCCESS)
    {
	smpd_err_printf("stat command missing param parameter\n");
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    if (strcmp(param, "context") == 0)
    {
	if (smpd_process.context_list == NULL)
	{
	    strcpy(result_str, "none");
	}
	else
	{
	    str = result_str;
	    len = SMPD_MAX_CMD_LENGTH-100;
	    iter = smpd_process.context_list;
	    while (iter)
	    {
		smpd_snprintf_update(&str, &len, "{\n");
		smpd_snprintf_update(&str, &len, " type               = %s\n", smpd_get_context_str(iter));
		smpd_snprintf_update(&str, &len, " id                 = %d\n", iter->id);
		smpd_snprintf_update(&str, &len, " state              = %s\n", smpd_get_state_string(iter->state));
		smpd_snprintf_update(&str, &len, " read_state         = %s\n", smpd_get_state_string(iter->read_state));
		smpd_snprintf_update(&str, &len, " read_cmd:\n");
		smpd_command_to_string(&str, &len, 2, &iter->read_cmd);
		smpd_snprintf_update(&str, &len, " write_state        = %s\n", smpd_get_state_string(iter->write_state));
		smpd_snprintf_update(&str, &len, " write_list         = %p\n", iter->write_list);
		cmd_iter = iter->write_list;
		while (cmd_iter)
		{
		    smpd_snprintf_update(&str, &len, " write_cmd:\n");
		    smpd_command_to_string(&str, &len, 2, cmd_iter);
		    cmd_iter = cmd_iter->next;
		}
		smpd_snprintf_update(&str, &len, " host               = %s\n", iter->host);
		smpd_snprintf_update(&str, &len, " rank               = %d\n", iter->rank);
		smpd_snprintf_update(&str, &len, " set                = %d\n", SMPDU_Sock_get_sock_set_id(iter->set));
		smpd_snprintf_update(&str, &len, " sock               = %d\n", SMPDU_Sock_get_sock_id(iter->sock));
		smpd_snprintf_update(&str, &len, " account            = %s\n", iter->account);
		smpd_snprintf_update(&str, &len, " password           = ***\n");
		smpd_snprintf_update(&str, &len, " connect_return_id  = %d\n", iter->connect_return_id);
		smpd_snprintf_update(&str, &len, " connect_return_tag = %d\n", iter->connect_return_tag);
		smpd_snprintf_update(&str, &len, " connect_to         = %p\n", iter->connect_to);
		smpd_snprintf_update(&str, &len, " cred_request       = %s\n", iter->cred_request);
		smpd_snprintf_update(&str, &len, " port_str           = %s\n", iter->port_str);
		smpd_snprintf_update(&str, &len, " pszChallengeResponse = %s\n", iter->pszChallengeResponse);
		smpd_snprintf_update(&str, &len, " pszCrypt           = %s\n", iter->pszCrypt);
		smpd_snprintf_update(&str, &len, " pwd_request        = %s\n", iter->pwd_request);
		smpd_snprintf_update(&str, &len, " session            = %s\n", iter->session);
		smpd_snprintf_update(&str, &len, " session_header     = '%s'\n", iter->session_header);
		smpd_snprintf_update(&str, &len, " smpd_pwd           = %s\n", iter->smpd_pwd);
#ifdef HAVE_WINDOWS_H
		smpd_snprintf_update(&str, &len, " wait.hProcess      = %p\n", iter->wait.hProcess);
		smpd_snprintf_update(&str, &len, " wait.hThread       = %p\n", iter->wait.hThread);
#else
		smpd_snprintf_update(&str, &len, " wait               = %d\n", (int)(iter->wait));
#endif
		smpd_snprintf_update(&str, &len, " wait_list          = %p\n", iter->wait_list);
		smpd_snprintf_update(&str, &len, " process            = %p\n", iter->process);
		if (iter->process)
		{
		    smpd_process_to_string(&str, &len, 2, iter->process);
		}
		smpd_snprintf_update(&str, &len, " next               = %p\n", iter->next);
		smpd_snprintf_update(&str, &len, "}\n");
		iter = iter->next;
	    }
	}
    }
    else if (strcmp(param, "process") == 0)
    {
	strcpy(result_str, "none");
    }
    else
    {
	strcpy(result_str, "unknown");
    }

    /* create a result command */
    result = smpd_create_command("result", smpd_process.id, cmd->src, SMPD_FALSE, &temp_cmd);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to create a result command for the context.\n");
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    /* add the command tag for result matching */
    result = smpd_add_command_int_arg(temp_cmd, "cmd_tag", cmd->tag);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the tag to the result command for a stat command.\n");
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
    /* add the result string */
    result = smpd_add_command_arg(temp_cmd, "result", result_str);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the result string to the result command for a stat command.\n");
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    /* post the result command */
    smpd_dbg_printf("replying to stat command: \"%s\"\n", temp_cmd->cmd);
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
#define FCNAME "smpd_handle_abort_command"
int smpd_handle_abort_command(smpd_context_t *context)
{
    char error_str[2048];

    smpd_enter_fn(FCNAME);

    if (MPIU_Str_get_string_arg(context->read_cmd.cmd, "error", error_str, 2048) == MPIU_STR_SUCCESS)
    {
	smpd_err_printf("abort: %s\n", error_str);
    }
    else
    {
	/* abort without an error means return and exit peacefully */
	/* FIXME: create a new return class, for now use SMPD_DBS_RETURN */
	smpd_exit_fn(FCNAME);
	return SMPD_DBS_RETURN;
    }

    smpd_exit_fn(FCNAME);
    return SMPD_EXIT;
}

#undef FCNAME
#define FCNAME "smpd_handle_abort_job_command"
int smpd_handle_abort_job_command(smpd_context_t *context)
{
    char error_str[2048];
    char name[SMPD_MAX_DBS_NAME_LEN+1] = "";
    smpd_command_t *cmd;
    int rank;
    int result;
    int exit_code;

    smpd_enter_fn(FCNAME);

    cmd = &context->read_cmd;
    if (MPIU_Str_get_string_arg(cmd->cmd, "error", error_str, 2048) != MPIU_STR_SUCCESS)
    {
	smpd_err_printf("invalid abort_job command, no error field in the command: '%s'\n", cmd->cmd);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    if (MPIU_Str_get_string_arg(cmd->cmd, "name", name, SMPD_MAX_DBS_NAME_LEN) != MPIU_STR_SUCCESS)
    {
	smpd_err_printf("invalid abort_job command, no name field in the command: '%s'\n", cmd->cmd);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    if (MPIU_Str_get_int_arg(cmd->cmd, "rank", &rank) != MPIU_STR_SUCCESS)
    {
	smpd_err_printf("invalid abort_job command, no rank field in the command: '%s'\n", cmd->cmd);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    if (MPIU_Str_get_int_arg(cmd->cmd, "exit_code", &exit_code) != MPIU_STR_SUCCESS)
    {
	smpd_err_printf("invalid abort_job command, no exit_code field in the command: '%s'\n", cmd->cmd);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    result = smpd_do_abort_job(name, rank, error_str, exit_code);

    smpd_exit_fn(FCNAME);
    return result;
}

#undef FCNAME
#define FCNAME "smpd_handle_init_command"
int smpd_handle_init_command(smpd_context_t *context)
{
    int result;
    smpd_command_t *cmd, *temp_cmd;
    char name[SMPD_MAX_DBS_NAME_LEN+1] = "";
    char key[SMPD_MAX_DBS_KEY_LEN+1] = "";
    char value[SMPD_MAX_DBS_VALUE_LEN+1] = "";
    char fail_str[SMPD_MAX_DBS_VALUE_LEN+1] = "";
    char ctx_key[100];
    char *result_str = NULL;
    int rank, size, node_id=0;
    smpd_process_group_t *pg;

    smpd_enter_fn(FCNAME);

    cmd = &context->read_cmd;

    /* prepare the result command */
    result = smpd_create_command("result", smpd_process.id, cmd->src, SMPD_FALSE, &temp_cmd);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to create a result command for the init command '%s'.\n", cmd->cmd);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    /* add the command tag for result matching */
    result = smpd_add_command_int_arg(temp_cmd, "cmd_tag", cmd->tag);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the tag to the result command for the init command '%s'.\n", cmd->cmd);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    /* copy the ctx_key for pmi control channel lookup */
    if (MPIU_Str_get_string_arg(cmd->cmd, "ctx_key", ctx_key, 100) != MPIU_STR_SUCCESS)
    {
	smpd_err_printf("no ctx_key in the db command: '%s'\n", cmd->cmd);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    result = smpd_add_command_arg(temp_cmd, "ctx_key", ctx_key);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the ctx_key to the result command for dbs command '%s'.\n", cmd->cmd);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    /* do init stuff */
    if ((get_name_key_value(cmd->cmd, name, key, value) != SMPD_SUCCESS) || (MPIU_Str_get_int_arg(cmd->cmd, "node_id", &node_id) != MPIU_STR_SUCCESS))
    {
	snprintf(fail_str, SMPD_MAX_DBS_VALUE_LEN, SMPD_FAIL_STR" - invalid init command, name = %s, key = %s, value = %s, node_id = %d.",
	    name, key, value, node_id);
	result = smpd_add_command_arg(temp_cmd, "result", fail_str);
	/*result = smpd_add_command_arg(temp_cmd, "result", SMPD_FAIL_STR" - invalid init command.");*/
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to add the result string to the result command for init command '%s'.\n", cmd->cmd);
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
	smpd_dbg_printf("sending result command to %s context: \"%s\"\n", smpd_get_context_str(context), temp_cmd->cmd);
	result = smpd_post_write_command(context, temp_cmd);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to post a write of the result command to the context: cmd '%s', init cmd '%s'", temp_cmd->cmd, cmd->cmd);
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
	smpd_exit_fn(FCNAME);
	return SMPD_SUCCESS;
    }

    rank = atoi(key);
    size = atoi(value);
    if (rank < 0 || size < 1)
    {
	snprintf(fail_str, SMPD_MAX_DBS_VALUE_LEN, SMPD_FAIL_STR" - invalid init command, rank = %d, size = %d.", rank, size);
	result = smpd_add_command_arg(temp_cmd, "result", fail_str);
	/*result = smpd_add_command_arg(temp_cmd, "result", SMPD_FAIL_STR" - invalid init command.");*/
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to add the result string to the result command for init command '%s'.\n", cmd->cmd);
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
	smpd_dbg_printf("sending result command to %s context: \"%s\"\n", smpd_get_context_str(context), temp_cmd->cmd);
	result = smpd_post_write_command(context, temp_cmd);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to post a write of the result command to the context: cmd '%s', init cmd '%s'", temp_cmd->cmd, cmd->cmd);
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
	smpd_exit_fn(FCNAME);
	return SMPD_SUCCESS;
    }
    smpd_dbg_printf("init: %d:%d:%s\n", rank, size, name);
    pg = smpd_process.pg_list;
    while (pg)
    {
	if (strcmp(pg->kvs, name) == 0)
	{
	    if (rank >= pg->num_procs)
	    {
		sprintf(value, "%s - rank %d out of range, number of processes = %d", SMPD_FAIL_STR, rank, pg->num_procs);
		result_str = value;
		break;
	    }
	    pg->any_init_received = SMPD_TRUE;
	    pg->processes[rank].init_called = SMPD_TRUE;
	    pg->processes[rank].node_id = node_id;
	    memcpy(pg->processes[rank].ctx_key, ctx_key, sizeof(pg->processes[rank].ctx_key));
	    if (pg->any_noinit_process_exited == SMPD_TRUE)
	    {
		result_str = SMPD_FAIL_STR" - init called when another process has exited without calling init";
	    }
	    else
	    {
		result_str = SMPD_SUCCESS_STR;
	    }
	    break;
	}
	pg = pg->next;
    }
    if (pg == NULL)
    {
	smpd_err_printf("init command received but no process group structure found to match it: pg <%s>\n", name);
	result_str = SMPD_FAIL_STR" - init command received but no process group structure fount to match it.";
    }

    /* send the reply */
    smpd_dbg_printf("sending reply to init command '%s'.\n", cmd->cmd);
    result = smpd_add_command_arg(temp_cmd, "result", result_str);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the result string to the result command for the init command '%s'.\n", cmd->cmd);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    smpd_dbg_printf("sending result command to %s context: \"%s\"\n", smpd_get_context_str(context), temp_cmd->cmd);
    result = smpd_post_write_command(context, temp_cmd);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to post a write of the result command to the context: cmd '%s', init cmd '%s'", temp_cmd->cmd, cmd->cmd);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_handle_finalize_command"
int smpd_handle_finalize_command(smpd_context_t *context)
{
    int result;
    smpd_command_t *cmd, *temp_cmd;
    char name[SMPD_MAX_DBS_NAME_LEN+1] = "";
    char key[SMPD_MAX_DBS_KEY_LEN+1] = "";
    char value[SMPD_MAX_DBS_VALUE_LEN+1] = "";
    char ctx_key[100];
    char *result_str = NULL;
    int rank;
    smpd_process_group_t *pg;

    smpd_enter_fn(FCNAME);

    cmd = &context->read_cmd;

    /* prepare the result command */
    result = smpd_create_command("result", smpd_process.id, cmd->src, SMPD_FALSE, &temp_cmd);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to create a result command for the finalize command '%s'.\n", cmd->cmd);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    /* add the command tag for result matching */
    result = smpd_add_command_int_arg(temp_cmd, "cmd_tag", cmd->tag);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the tag to the result command for the finalize command '%s'.\n", cmd->cmd);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    /* copy the ctx_key for pmi control channel lookup */
    if (MPIU_Str_get_string_arg(cmd->cmd, "ctx_key", ctx_key, 100) != MPIU_STR_SUCCESS)
    {
	smpd_err_printf("no ctx_key in the db command: '%s'\n", cmd->cmd);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    result = smpd_add_command_arg(temp_cmd, "ctx_key", ctx_key);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the ctx_key to the result command for dbs command '%s'.\n", cmd->cmd);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    /* do finalize stuff */
    if (get_name_key_value(cmd->cmd, name, key, NULL) != SMPD_SUCCESS)
    {
	result = smpd_add_command_arg(temp_cmd, "result", SMPD_FAIL_STR" - invalid finalize command.");
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to add the result string to the result command for finalize command '%s'.\n", cmd->cmd);
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
	smpd_dbg_printf("sending result command to %s context: \"%s\"\n", smpd_get_context_str(context), temp_cmd->cmd);
	result = smpd_post_write_command(context, temp_cmd);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to post a write of the result command to the context: cmd '%s', finalize cmd '%s'", temp_cmd->cmd, cmd->cmd);
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
	smpd_exit_fn(FCNAME);
	return SMPD_SUCCESS;
    }
    rank = atoi(key);
    if (rank < 0)
    {
	result = smpd_add_command_arg(temp_cmd, "result", SMPD_FAIL_STR" - invalid finalize command.");
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to add the result string to the result command for finalize command '%s'.\n", cmd->cmd);
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
	smpd_dbg_printf("sending result command to %s context: \"%s\"\n", smpd_get_context_str(context), temp_cmd->cmd);
	result = smpd_post_write_command(context, temp_cmd);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to post a write of the result command to the context: cmd '%s', finalize cmd '%s'", temp_cmd->cmd, cmd->cmd);
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
	smpd_exit_fn(FCNAME);
	return SMPD_SUCCESS;
    }
    smpd_dbg_printf("finalize: %d:%s\n", rank, name);
    pg = smpd_process.pg_list;
    while (pg)
    {
	if (strcmp(pg->kvs, name) == 0)
	{
	    if (rank >= pg->num_procs)
	    {
		sprintf(value, "%s - rank %d out of range, number of processes = %d", SMPD_FAIL_STR, rank, pg->num_procs);
		result_str = value;
		break;
	    }
	    pg->processes[rank].finalize_called = SMPD_TRUE;
	    result_str = SMPD_SUCCESS_STR;
	    break;
	}
	pg = pg->next;
    }
    if (pg == NULL)
    {
	smpd_err_printf("finalize command received but no process group structure found to match it: pg <%s>\n", name);
	result_str = SMPD_FAIL_STR" - finalize command received but no process group structure fount to match it.";
    }

    /* send the reply */
    smpd_dbg_printf("sending reply to finalize command '%s'.\n", cmd->cmd);
    result = smpd_add_command_arg(temp_cmd, "result", result_str);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the result string to the result command for the finalize command '%s'.\n", cmd->cmd);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    smpd_dbg_printf("sending result command to %s context: \"%s\"\n", smpd_get_context_str(context), temp_cmd->cmd);
    result = smpd_post_write_command(context, temp_cmd);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to post a write of the result command to the context: cmd '%s', finalize cmd '%s'", temp_cmd->cmd, cmd->cmd);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_handle_validate_command"
int smpd_handle_validate_command(smpd_context_t *context)
{
    int result = SMPD_SUCCESS;
    smpd_command_t *cmd, *temp_cmd;
    char fullaccount[SMPD_MAX_ACCOUNT_LENGTH]="", domain[SMPD_MAX_ACCOUNT_LENGTH];
    char account[SMPD_MAX_ACCOUNT_LENGTH]="", password[SMPD_MAX_PASSWORD_LENGTH]="", encrypted_password[SMPD_MAX_PASSWORD_LENGTH]="";
    char result_str[100];
    int length;
#ifdef HAVE_WINDOWS_H
    HANDLE hUser;
#endif

    smpd_enter_fn(FCNAME);

    cmd = &context->read_cmd;

    if (MPIU_Str_get_string_arg(cmd->cmd, "account", fullaccount, SMPD_MAX_ACCOUNT_LENGTH) != MPIU_STR_SUCCESS)
    {
	smpd_err_printf("validate command missing account parameter\n");
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    /* FIXME: decrypt the password */
    if (MPIU_Str_get_string_arg(cmd->cmd, "password", encrypted_password, SMPD_MAX_PASSWORD_LENGTH) != MPIU_STR_SUCCESS)
    {
	smpd_err_printf("validate command missing password parameter\n");
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    length = SMPD_MAX_PASSWORD_LENGTH;
    result = smpd_decrypt_data(encrypted_password, SMPD_MAX_PASSWORD_LENGTH, password, &length);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to decrypt the password\n");
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    /* validate user */
#ifdef HAVE_WINDOWS_H
    smpd_parse_account_domain(fullaccount, account, domain);
    result = smpd_get_user_handle(account, domain, password, &hUser);
    if (result != SMPD_SUCCESS)
    {
	strcpy(result_str, SMPD_FAIL_STR);
    }
    else
    {
	strcpy(result_str, SMPD_SUCCESS_STR);
    }
    if (hUser != INVALID_HANDLE_VALUE)
	CloseHandle(hUser);
#else
    strcpy(result_str, SMPD_FAIL_STR);
#endif

    /* prepare the result command */
    result = smpd_create_command("result", smpd_process.id, cmd->src, SMPD_FALSE, &temp_cmd);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to create a result command for a validate command.\n");
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    /* add the command tag for result matching */
    result = smpd_add_command_int_arg(temp_cmd, "cmd_tag", cmd->tag);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the tag to the result command for a validate command.\n");
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
    result = smpd_add_command_arg(temp_cmd, "result", result_str);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the result string to the result command for a validate command.\n");
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    /* send result back */
    smpd_dbg_printf("replying to validate command: \"%s\"\n", temp_cmd->cmd);
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
#define FCNAME "smpd_handle_status_command"
int smpd_handle_status_command(smpd_context_t *context)
{
    int result = SMPD_SUCCESS;
    smpd_command_t *cmd, *temp_cmd;
    char dynamic_hosts[SMPD_MAX_CMD_LENGTH - 100];

    smpd_enter_fn(FCNAME);

    cmd = &context->read_cmd;

    result = smpd_get_smpd_data("dynamic_hosts", dynamic_hosts, SMPD_MAX_CMD_LENGTH - 100);
    if (result != SMPD_SUCCESS)
	strcpy(dynamic_hosts, "none");

    /* prepare the result command */
    result = smpd_create_command("result", smpd_process.id, cmd->src, SMPD_FALSE, &temp_cmd);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to create a result command for a status command.\n");
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    /* add the command tag for result matching */
    result = smpd_add_command_int_arg(temp_cmd, "cmd_tag", cmd->tag);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the tag to the result command for a status command.\n");
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
    result = smpd_add_command_arg(temp_cmd, "result", dynamic_hosts);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the dynamic hosts result string to the result command for a status command.\n");
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    /* send result back */
    smpd_dbg_printf("replying to status command: \"%s\"\n", temp_cmd->cmd);
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
#define FCNAME "smpd_handle_get_command"
int smpd_handle_get_command(smpd_context_t *context)
{
    int result = SMPD_SUCCESS;
    smpd_command_t *cmd, *temp_cmd;
    char result_str[100];
    char key[SMPD_MAX_NAME_LENGTH];
    char value[SMPD_MAX_VALUE_LENGTH];

    smpd_enter_fn(FCNAME);

    cmd = &context->read_cmd;

    if (MPIU_Str_get_string_arg(cmd->cmd, "key", key, SMPD_MAX_NAME_LENGTH) != MPIU_STR_SUCCESS)
    {
	smpd_err_printf("get command missing key parameter\n");
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    /* get key */
    result = smpd_get_smpd_data(key, value, SMPD_MAX_VALUE_LENGTH);
    if (result != SMPD_SUCCESS)
    {
	/*
	smpd_err_printf("unable to get %s\n", key);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
	*/
	strcpy(result_str, SMPD_FAIL_STR);
    }
    else
    {
	strcpy(result_str, SMPD_SUCCESS_STR);
    }

    /* prepare the result command */
    result = smpd_create_command("result", smpd_process.id, cmd->src, SMPD_FALSE, &temp_cmd);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to create a result command for a get %s=%s command.\n", key, value);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    /* add the command tag for result matching */
    result = smpd_add_command_int_arg(temp_cmd, "cmd_tag", cmd->tag);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the tag to the result command for a get %s=%s command.\n", key, value);
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
    if (strcmp(result_str, SMPD_SUCCESS_STR) == 0)
    {
	/* add the value */
	result = smpd_add_command_arg(temp_cmd, "value", value);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to add the value to the result command for a get %s=%s command.\n", key, value);
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
    }
    /* add the result */
    result = smpd_add_command_arg(temp_cmd, "result", result_str);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the result string to the result command for a get %s=%s command.\n", key, value);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    /* send result back */
    smpd_dbg_printf("replying to get %s command: \"%s\"\n", key, temp_cmd->cmd);
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
#define FCNAME "smpd_handle_set_command"
int smpd_handle_set_command(smpd_context_t *context)
{
    int result = SMPD_SUCCESS;
    smpd_command_t *cmd, *temp_cmd;
    char result_str[100];
    char key[SMPD_MAX_NAME_LENGTH];
    char value[SMPD_MAX_VALUE_LENGTH];

    smpd_enter_fn(FCNAME);

    cmd = &context->read_cmd;

    if (MPIU_Str_get_string_arg(cmd->cmd, "key", key, SMPD_MAX_NAME_LENGTH) != MPIU_STR_SUCCESS)
    {
	smpd_err_printf("set command missing key parameter\n");
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    if (MPIU_Str_get_string_arg(cmd->cmd, "value", value, SMPD_MAX_VALUE_LENGTH) != MPIU_STR_SUCCESS)
    {
	smpd_err_printf("set command missing value parameter\n");
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    /* set key=value */
    result = smpd_set_smpd_data(key, value);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to set %s=%s\n", key, value);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    strcpy(result_str, SMPD_SUCCESS_STR);

    /* prepare the result command */
    result = smpd_create_command("result", smpd_process.id, cmd->src, SMPD_FALSE, &temp_cmd);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to create a result command for a set %s=%s command.\n", key, value);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    /* add the command tag for result matching */
    result = smpd_add_command_int_arg(temp_cmd, "cmd_tag", cmd->tag);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the tag to the result command for a set %s=%s command.\n", key, value);
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
    result = smpd_add_command_arg(temp_cmd, "result", result_str);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the result string to the result command for a set %s=%s command.\n", key, value);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    /* send result back */
    smpd_dbg_printf("replying to set %s=%s command: \"%s\"\n", key, value, temp_cmd->cmd);
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
#define FCNAME "smpd_handle_delete_command"
int smpd_handle_delete_command(smpd_context_t *context)
{
    int result = SMPD_SUCCESS;
    smpd_command_t *cmd, *temp_cmd;
    char result_str[100];
    char key[SMPD_MAX_NAME_LENGTH];

    smpd_enter_fn(FCNAME);

    cmd = &context->read_cmd;

    if (MPIU_Str_get_string_arg(cmd->cmd, "key", key, SMPD_MAX_NAME_LENGTH) != MPIU_STR_SUCCESS)
    {
	smpd_err_printf("set command missing key parameter\n");
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    /* delete key */
    result = smpd_delete_smpd_data(key);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to delete smpd data %s\n", key);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    strcpy(result_str, SMPD_SUCCESS_STR);

    /* prepare the result command */
    result = smpd_create_command("result", smpd_process.id, cmd->src, SMPD_FALSE, &temp_cmd);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to create a result command for a delete %s command.\n", key);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    /* add the command tag for result matching */
    result = smpd_add_command_int_arg(temp_cmd, "cmd_tag", cmd->tag);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the tag to the result command for a delete %s command.\n", key);
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
    result = smpd_add_command_arg(temp_cmd, "result", result_str);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the result string to the result command for a delete %s command.\n", key);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    /* send result back */
    smpd_dbg_printf("replying to delete %s command: \"%s\"\n", key, temp_cmd->cmd);
    result = smpd_post_write_command(context, temp_cmd);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to post a write of the result command to the %s context.\n", smpd_get_context_str(context));
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    smpd_exit_fn(FCNAME);
    return result;
}

#undef FCNAME
#define FCNAME "smpd_handle_cred_request_command"
int smpd_handle_cred_request_command(smpd_context_t *context)
{
    int result;
    smpd_command_t *cmd, *temp_cmd;
    char host[SMPD_MAX_HOST_LENGTH];

    smpd_enter_fn(FCNAME);

    cmd = &context->read_cmd;

    if (MPIU_Str_get_string_arg(cmd->cmd, "host", host, SMPD_MAX_HOST_LENGTH) != MPIU_STR_SUCCESS)
    {
	smpd_err_printf("no host parameter in the cred_request command: '%s'\n", cmd->cmd);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    /* prepare the result command */
    result = smpd_create_command("result", smpd_process.id, cmd->src, SMPD_FALSE, &temp_cmd);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to create a result command for a cred_request command.\n");
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    /* add the command tag for result matching */
    result = smpd_add_command_int_arg(temp_cmd, "cmd_tag", cmd->tag);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the tag to the result command for a cred_request command.\n");
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

#ifdef HAVE_WINDOWS_H
    if (smpd_process.use_sspi)
    {
	result = smpd_add_command_arg(temp_cmd, "result", "sspi");
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to add the result parameter to the result command.\n");
	    smpd_exit_fn(FCNAME);
	    return result;
	}
    }
    else
    {
	if (smpd_process.UserAccount[0] == '\0')
	{
	    if (smpd_process.logon || 
		(!smpd_get_cached_password(smpd_process.UserAccount, smpd_process.UserPassword) &&
		!smpd_read_password_from_registry(smpd_process.user_index, smpd_process.UserAccount, smpd_process.UserPassword)))
	    {
		if (smpd_process.credentials_prompt)
		{
		    fprintf(stderr, "User credentials needed to launch processes on %s:\n", host);
		    smpd_get_account_and_password(smpd_process.UserAccount, smpd_process.UserPassword);
		    smpd_cache_password(smpd_process.UserAccount, smpd_process.UserPassword);
		    result = smpd_add_command_arg(temp_cmd, "account", smpd_process.UserAccount);
		    if (result != SMPD_SUCCESS)
		    {
			smpd_err_printf("unable to add the account parameter to the result command.\n");
			smpd_exit_fn(FCNAME);
			return result;
		    }
		    result = smpd_encrypt_data(smpd_process.UserPassword, (int)strlen(smpd_process.UserPassword)+1, context->encrypted_password, SMPD_MAX_PASSWORD_LENGTH);
		    if (result != SMPD_SUCCESS)
		    {
			smpd_err_printf("unable to encrypt the password parameter to the result command.\n");
			smpd_exit_fn(FCNAME);
			return result;
		    }
		    result = smpd_add_command_arg(temp_cmd, "password", context->encrypted_password/*smpd_process.UserPassword*/);
		    if (result != SMPD_SUCCESS)
		    {
			smpd_err_printf("unable to add the password parameter to the result command.\n");
			smpd_exit_fn(FCNAME);
			return result;
		    }
		    result = smpd_add_command_arg(temp_cmd, "result", SMPD_SUCCESS_STR);
		    if (result != SMPD_SUCCESS)
		    {
			smpd_err_printf("unable to add the result parameter to the result command.\n");
			smpd_exit_fn(FCNAME);
			return result;
		    }
		}
		else
		{
		    result = smpd_add_command_arg(temp_cmd, "result", SMPD_FAIL_STR);
		    if (result != SMPD_SUCCESS)
		    {
			smpd_err_printf("unable to add the result parameter to the result command.\n");
			smpd_exit_fn(FCNAME);
			return result;
		    }
		}
	    }
	    else
	    {
		result = smpd_add_command_arg(temp_cmd, "account", smpd_process.UserAccount);
		if (result != SMPD_SUCCESS)
		{
		    smpd_err_printf("unable to add the account parameter to the result command.\n");
		    smpd_exit_fn(FCNAME);
		    return result;
		}
		result = smpd_encrypt_data(smpd_process.UserPassword, (int)strlen(smpd_process.UserPassword)+1, context->encrypted_password, SMPD_MAX_PASSWORD_LENGTH);
		if (result != SMPD_SUCCESS)
		{
		    smpd_err_printf("unable to encrypt the password parameter for the result command.\n");
		    smpd_exit_fn(FCNAME);
		    return result;
		}
		result = smpd_add_command_arg(temp_cmd, "password", context->encrypted_password/*smpd_process.UserPassword*/);
		if (result != SMPD_SUCCESS)
		{
		    smpd_err_printf("unable to add the password parameter to the result command.\n");
		    smpd_exit_fn(FCNAME);
		    return result;
		}
		result = smpd_add_command_arg(temp_cmd, "result", SMPD_SUCCESS_STR);
		if (result != SMPD_SUCCESS)
		{
		    smpd_err_printf("unable to add the result parameter to the result command.\n");
		    smpd_exit_fn(FCNAME);
		    return result;
		}
	    }
	}
	else
	{
	    result = smpd_add_command_arg(temp_cmd, "account", smpd_process.UserAccount);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to add the account parameter to the result command.\n");
		smpd_exit_fn(FCNAME);
		return result;
	    }
	    result = smpd_encrypt_data(smpd_process.UserPassword, (int)strlen(smpd_process.UserPassword)+1, context->encrypted_password, SMPD_MAX_PASSWORD_LENGTH);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to encrypt the password parameter for the result command.\n");
		smpd_exit_fn(FCNAME);
		return result;
	    }
	    result = smpd_add_command_arg(temp_cmd, "password", context->encrypted_password/*smpd_process.UserPassword*/);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to add the password parameter to the result command.\n");
		smpd_exit_fn(FCNAME);
		return result;
	    }
	    result = smpd_add_command_arg(temp_cmd, "result", SMPD_SUCCESS_STR);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to add the result parameter to the result command.\n");
		smpd_exit_fn(FCNAME);
		return result;
	    }
	}
    }
#else
    if (smpd_process.UserAccount[0] == '\0')
    {
	if (smpd_process.credentials_prompt)
	{
	    fprintf(stderr, "User credentials needed to launch processes on %s:\n", host);
	    smpd_get_account_and_password(smpd_process.UserAccount, smpd_process.UserPassword);
	    result = smpd_add_command_arg(temp_cmd, "account", smpd_process.UserAccount);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to add the account parameter to the result command.\n");
		smpd_exit_fn(FCNAME);
		return result;
	    }
	    result = smpd_encrypt_data(smpd_process.UserPassword, (int)strlen(smpd_process.UserPassword)+1, context->encrypted_password, SMPD_MAX_PASSWORD_LENGTH);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to encrypt the password parameter for the result command.\n");
		smpd_exit_fn(FCNAME);
		return result;
	    }
	    result = smpd_add_command_arg(temp_cmd, "password", context->encrypted_password/*smpd_process.UserPassword*/);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to add the password parameter to the result command.\n");
		smpd_exit_fn(FCNAME);
		return result;
	    }
	    result = smpd_add_command_arg(temp_cmd, "result", SMPD_SUCCESS_STR);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to add the result parameter to the result command.\n");
		smpd_exit_fn(FCNAME);
		return result;
	    }
	}
	else
	{
	    result = smpd_add_command_arg(temp_cmd, "result", SMPD_FAIL_STR);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to add the result parameter to the result command.\n");
		smpd_exit_fn(FCNAME);
		return result;
	    }
	}
    }
    else
    {
	result = smpd_add_command_arg(temp_cmd, "account", smpd_process.UserAccount);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to add the account parameter to the result command.\n");
	    smpd_exit_fn(FCNAME);
	    return result;
	}
	result = smpd_encrypt_data(smpd_process.UserPassword, (int)strlen(smpd_process.UserPassword)+1, context->encrypted_password, SMPD_MAX_PASSWORD_LENGTH);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to encrypt the password parameter for the result command.\n");
	    smpd_exit_fn(FCNAME);
	    return result;
	}
	result = smpd_add_command_arg(temp_cmd, "password", context->encrypted_password/*smpd_process.UserPassword*/);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to add the password parameter to the result command.\n");
	    smpd_exit_fn(FCNAME);
	    return result;
	}
	result = smpd_add_command_arg(temp_cmd, "result", SMPD_SUCCESS_STR);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to add the result parameter to the result command.\n");
	    smpd_exit_fn(FCNAME);
	    return result;
	}
    }
#endif
    /* send result back */
    result = smpd_post_write_command(context, temp_cmd);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to post a write of the result command to the %s context.\n", smpd_get_context_str(context));
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    smpd_exit_fn(FCNAME);
    return result;
}

#undef FCNAME
#define FCNAME "smpd_sspi_context_init"
int smpd_sspi_context_init(smpd_sspi_client_context_t **sspi_context_pptr, const char *host, int port, smpd_sspi_type_t type)
{
#ifdef HAVE_WINDOWS_H
    int result;
    char err_msg[256];
    SEC_WINNT_AUTH_IDENTITY *identity = NULL;
    SECURITY_STATUS sec_result, sec_result_copy;
    SecBufferDesc outbound_descriptor;
    SecBuffer outbound_buffer;
    ULONG attr;
    TimeStamp ts;
    SecPkgInfo *info;
    smpd_sspi_client_context_t *sspi_context;
    /*
    char account[SMPD_MAX_ACCOUNT_LENGTH] = "";
    char domain[SMPD_MAX_ACCOUNT_LENGTH] = "";
    */
    char target_[SMPD_MAX_NAME_LENGTH] = "", *target = target_;
    double t1, t2;

    smpd_enter_fn(FCNAME);

    if (smpd_process.sec_fn == NULL)
    {
	smpd_dbg_printf("calling InitSecurityInterface\n");
	smpd_process.sec_fn = InitSecurityInterface();
	if (smpd_process.sec_fn == NULL)
	{
	    smpd_err_printf("unable to initialize the sspi interface.\n");
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
    }
    /* FIXME: How do we determine whether to provide user credentials or impersonate the current user? */
    /*
    if (smpd_process.logon)
    {
	// This doesn't work because it causes LogonUser to be called and mpiexec can't do that
	identity = (SEC_WINNT_AUTH_IDENTITY *)MPIU_Malloc(sizeof(SEC_WINNT_AUTH_IDENTITY));
	if (identity == NULL)
	{
	    smpd_err_printf("unable to allocate an sspi security identity.\n");
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
	smpd_parse_account_domain(smpd_process.UserAccount, account, domain);
	if (domain[0] == '\0')
	{
	    identity->Domain = NULL;
	    identity->DomainLength = 0;
	}
	else
	{
	    smpd_dbg_printf("using identity domain: %s\n", domain);
	    identity->Domain = domain;
	    identity->DomainLength = (unsigned long)strlen(domain);
	}
	smpd_dbg_printf("using identity user: %s\n", account);
	identity->User = account;
	identity->UserLength = (unsigned long)strlen(account);
	identity->Password = smpd_process.UserPassword;
	identity->PasswordLength = (unsigned long)strlen(smpd_process.UserPassword);
	identity->Flags = SEC_WINNT_AUTH_IDENTITY_ANSI;
	strncpy(target, smpd_process.UserAccount, SMPD_MAX_ACCOUNT_LENGTH);
    }
    else
    {
    */
	result = smpd_lookup_spn(target, SMPD_MAX_NAME_LENGTH, host, port);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to lookup the smpd Service Principal Name.\n");
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
	if (*target == '\0')
	{
	    target = NULL;
	}
    /*}*/
    result = smpd_create_sspi_client_context(&sspi_context);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to allocate an sspi client context.\n");
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    smpd_dbg_printf("calling QuerySecurityPackageInfo\n");
    sec_result = smpd_process.sec_fn->QuerySecurityPackageInfo(SMPD_SECURITY_PACKAGE, &info);
    if (sec_result != SEC_E_OK)
    {
	smpd_err_printf("unable to query the security package, error %d\n", sec_result);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    smpd_dbg_printf("%s package, %s, with: max %d byte token, capabilities bitmask 0x%x\n",
	info->Name, info->Comment, info->cbMaxToken, info->fCapabilities);
    smpd_dbg_printf("calling AcquireCredentialsHandle\n");
    t1 = PMPI_Wtime();
    sec_result = smpd_process.sec_fn->AcquireCredentialsHandle(NULL, SMPD_SECURITY_PACKAGE, SECPKG_CRED_OUTBOUND, NULL, identity, NULL, NULL, &sspi_context->credential, &ts);
    t2 = PMPI_Wtime();
    smpd_dbg_printf("AcquireCredentialsHandle took %0.6f seconds\n", t2-t1);
    if (identity != NULL)
    {
	MPIU_Free(identity);
    }
    if (sec_result != SEC_E_OK)
    {
	smpd_err_printf("unable to acquire the outbound client credential, error %d\n", sec_result);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    sspi_context->buffer = MPIU_Malloc(max(info->cbMaxToken, SMPD_SSPI_MAX_BUFFER_SIZE));
    if (sspi_context->buffer == NULL)
    {
	smpd_err_printf("unable to allocate a %d byte sspi buffer\n", info->cbMaxToken);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    smpd_dbg_printf("first sspi buffer of length %d bytes\n", info->cbMaxToken);
    sspi_context->buffer_length = info->cbMaxToken;
    sspi_context->max_buffer_size = max(info->cbMaxToken, SMPD_SSPI_MAX_BUFFER_SIZE);
    outbound_descriptor.ulVersion = SECBUFFER_VERSION;
    outbound_descriptor.cBuffers = 1;
    outbound_descriptor.pBuffers = &outbound_buffer;
    outbound_buffer.BufferType = SECBUFFER_TOKEN;
    outbound_buffer.cbBuffer = info->cbMaxToken;
    outbound_buffer.pvBuffer = sspi_context->buffer;
    smpd_dbg_printf("calling FreeContextBuffer\n");
    sec_result = smpd_process.sec_fn->FreeContextBuffer(info);
    if (sec_result != SEC_E_OK)
    {
	smpd_err_printf("unable to free the security info structure, error %d\n", sec_result);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    if (target != NULL)
    {
	result = MPIU_Strncpy(sspi_context->target, target, SMPD_MAX_NAME_LENGTH);
	if (result != SMPD_SUCCESS)
	{
	}
    }
    switch (type)
    {
    case SMPD_SSPI_IDENTIFY:
	sspi_context->flags = ISC_REQ_IDENTIFY;
	break;
    case SMPD_SSPI_IMPERSONATE:
	sspi_context->flags = ISC_REQ_REPLAY_DETECT | ISC_REQ_SEQUENCE_DETECT | ISC_REQ_CONFIDENTIALITY;
	break;
    case SMPD_SSPI_DELEGATE:
	sspi_context->flags = ISC_REQ_REPLAY_DETECT | ISC_REQ_SEQUENCE_DETECT | ISC_REQ_CONFIDENTIALITY | ISC_REQ_MUTUAL_AUTH | ISC_REQ_DELEGATE;
	break;
    default:
	sspi_context->flags = ISC_REQ_REPLAY_DETECT | ISC_REQ_SEQUENCE_DETECT | ISC_REQ_CONFIDENTIALITY | ISC_REQ_MUTUAL_AUTH | ISC_REQ_DELEGATE;
	break;
    }
    smpd_dbg_printf("calling InitializeSecurityContext: target = %s\n", sspi_context->target);
    t1 = PMPI_Wtime();
    sec_result = sec_result_copy = smpd_process.sec_fn->InitializeSecurityContext(
	&sspi_context->credential, NULL,
	sspi_context->target,
	sspi_context->flags,
	0,
	/*SECURITY_NATIVE_DREP, */SECURITY_NETWORK_DREP,
	NULL, 0, &sspi_context->context, &outbound_descriptor, &attr, &ts);
    t2 = PMPI_Wtime();
    smpd_dbg_printf("InitializeSecurityContext took %0.6f seconds\n", t2-t1);
    switch (sec_result)
    {
    case SEC_E_OK:
	smpd_dbg_printf("SEC_E_OK\n");
	break;
    case SEC_I_COMPLETE_NEEDED:
	smpd_dbg_printf("SEC_I_COMPLETE_NEEDED\n");
	sspi_context->buffer_length = 0;
    case SEC_I_COMPLETE_AND_CONTINUE:
	if (sec_result == SEC_I_COMPLETE_AND_CONTINUE)
	{
	    smpd_dbg_printf("SEC_I_COMPLETE_AND_CONTINUE\n");
	}
	smpd_dbg_printf("calling CompleteAuthToken\n");
	sec_result = smpd_process.sec_fn->CompleteAuthToken(&sspi_context->context, &outbound_descriptor);
	if (sec_result != SEC_E_OK)
	{
	    smpd_process.sec_fn->DeleteSecurityContext(&sspi_context->context);
	    smpd_process.sec_fn->FreeCredentialsHandle(&sspi_context->credential);
	    smpd_err_printf("CompleteAuthToken failed with error %d\n", sec_result);
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
	if (sec_result_copy == SEC_I_COMPLETE_NEEDED)
	    break;
    case SEC_I_CONTINUE_NEEDED:
	if (sec_result_copy == SEC_I_CONTINUE_NEEDED)
	{
	    smpd_dbg_printf("SEC_I_CONTINUE_NEEDED\n");
	}
	smpd_dbg_printf("outbound buffer size: %d\n", outbound_buffer.cbBuffer);
	sspi_context->buffer_length = outbound_buffer.cbBuffer;
	break;
    default:
	/* error occurred */
	smpd_translate_win_error(sec_result, err_msg, 256, NULL);
	smpd_err_printf("InitializeSecurityContext failed with error %d: %s\n", sec_result, err_msg);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    *sspi_context_pptr = sspi_context;
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
#else
    smpd_enter_fn(FCNAME);
    smpd_err_printf("function not implemented.\n");
    smpd_exit_fn(FCNAME);
    return SMPD_FAIL;
#endif
}

#undef FCNAME
#define FCNAME "smpd_handle_sspi_init_command"
int smpd_handle_sspi_init_command(smpd_context_t *context)
{
    int result;
    smpd_command_t *cmd, *temp_cmd;
    char context_str[20];
    smpd_sspi_client_context_t *sspi_context;
    char host[SMPD_MAX_HOST_LENGTH];
    int port;

    smpd_enter_fn(FCNAME);

    cmd = &context->read_cmd;

    if (MPIU_Str_get_string_arg(cmd->cmd, "sspi_context", context_str, 20) != MPIU_STR_SUCCESS)
    {
	smpd_err_printf("no context parameter in the sspi_init command: '%s'\n", cmd->cmd);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    if (MPIU_Str_get_string_arg(cmd->cmd, "sspi_host", host, SMPD_MAX_HOST_LENGTH) != MPIU_STR_SUCCESS)
    {
	smpd_err_printf("no host parameter in the sspi_init command: '%s'\n", cmd->cmd);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    if (MPIU_Str_get_int_arg(cmd->cmd, "sspi_port", &port) != MPIU_STR_SUCCESS)
    {
	smpd_err_printf("no port parameter in the sspi_init command: '%s'\n", cmd->cmd);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    /* prepare the result command */
    result = smpd_create_command("result", smpd_process.id, cmd->src, SMPD_FALSE, &temp_cmd);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to create a result command for a cred_request command.\n");
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    /* add the command tag for result matching */
    result = smpd_add_command_int_arg(temp_cmd, "cmd_tag", cmd->tag);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the tag to the result command for a cred_request command.\n");
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
    result = smpd_add_command_arg(temp_cmd, "sspi_context", context_str);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add context_str to the result command for a %s command\n", cmd->cmd_str);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    /* create and initialize an sspi context */
    result = smpd_sspi_context_init(&sspi_context, host, port, SMPD_SSPI_DELEGATE);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to initialize an sspi context\n");
	goto fn_fail;
    }

    result = smpd_add_command_int_arg(temp_cmd, "data_length", sspi_context->buffer_length);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the data_length parameter to the result command.\n");
	smpd_exit_fn(FCNAME);
	return result;
    }
    result = smpd_add_command_binary_arg(temp_cmd, "data", sspi_context->buffer, sspi_context->buffer_length);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the data parameter to the result command.\n");
	smpd_err_printf("temp_cmd.cmd = '%s'\n", temp_cmd->cmd);
	smpd_exit_fn(FCNAME);
	return result;
    }
    result = smpd_add_command_int_arg(temp_cmd, "sspi_id", sspi_context->id);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the sspi_id parameter to the result command.\n");
	smpd_exit_fn(FCNAME);
	return result;
    }
    result = smpd_add_command_arg(temp_cmd, "result", SMPD_SUCCESS_STR);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the result parameter to the result command.\n");
	smpd_exit_fn(FCNAME);
	return result;
    }

fn_exit:
    /* send result back */
    result = smpd_post_write_command(context, temp_cmd);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to post a write of the result command to the %s context.\n", smpd_get_context_str(context));
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    smpd_exit_fn(FCNAME);
    return result;
fn_fail:
    result = smpd_add_command_arg(temp_cmd, "result", SMPD_FAIL_STR);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the result parameter to the result command.\n");
	smpd_exit_fn(FCNAME);
	return result;
    }
    goto fn_exit;
}

#undef FCNAME
#define FCNAME "smpd_sspi_context_iter"
int smpd_sspi_context_iter(int sspi_id, void **sspi_buffer_pptr, int *length_ptr)
{
#ifdef HAVE_WINDOWS_H
    char err_msg[256];
    SECURITY_STATUS sec_result, sec_result_copy;
    SecBufferDesc outbound_descriptor, inbound_descriptor;
    SecBuffer outbound_buffer, inbound_buffer;
    ULONG attr;
    TimeStamp ts;
    SecPkgInfo *info;
    smpd_sspi_client_context_t *sspi_context;
    double t1, t2;

    smpd_enter_fn(FCNAME);

    if (smpd_process.sec_fn == NULL)
    {
	smpd_dbg_printf("calling InitSecurityInterface\n");
	smpd_process.sec_fn = InitSecurityInterface();
	if (smpd_process.sec_fn == NULL)
	{
	    smpd_err_printf("unable to initialize the sspi interface.\n");
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
    }

    /* look up the sspi_context based on the id */
    sspi_context = smpd_process.sspi_context_list;
    while (sspi_context)
    {
	if (sspi_context->id == sspi_id)
	    break;
	sspi_context = sspi_context->next;
    }
    if (sspi_context == NULL)
    {
	smpd_err_printf("unable to look up the sspi_id %d\n", sspi_id);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    inbound_descriptor.ulVersion = SECBUFFER_VERSION;
    inbound_descriptor.cBuffers = 1;
    inbound_descriptor.pBuffers = &inbound_buffer;
    inbound_buffer.BufferType = SECBUFFER_TOKEN;
    inbound_buffer.cbBuffer = *length_ptr;
    inbound_buffer.pvBuffer = *sspi_buffer_pptr;

    smpd_dbg_printf("calling QuerySecurityPackageInfo\n");
    sec_result = smpd_process.sec_fn->QuerySecurityPackageInfo(SMPD_SECURITY_PACKAGE, &info);
    if (sec_result != SEC_E_OK)
    {
	smpd_err_printf("unable to query the security package, error %d\n", sec_result);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    smpd_dbg_printf("%s package, %s, with: max %d byte token, capabilities bitmask 0x%x\n",
	info->Name, info->Comment, info->cbMaxToken, info->fCapabilities);
    sspi_context->buffer = MPIU_Malloc(info->cbMaxToken);
    if (sspi_context->buffer == NULL)
    {
	smpd_err_printf("unable to allocate a %d byte sspi buffer\n", info->cbMaxToken);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    sspi_context->buffer_length = info->cbMaxToken;
    outbound_descriptor.ulVersion = SECBUFFER_VERSION;
    outbound_descriptor.cBuffers = 1;
    outbound_descriptor.pBuffers = &outbound_buffer;
    outbound_buffer.BufferType = SECBUFFER_TOKEN;
    outbound_buffer.cbBuffer = info->cbMaxToken;
    outbound_buffer.pvBuffer = sspi_context->buffer;
    smpd_dbg_printf("calling FreeContextBuffer\n");
    sec_result = smpd_process.sec_fn->FreeContextBuffer(info);
    if (sec_result != SEC_E_OK)
    {
	smpd_err_printf("unable to free the security info structure, error %d\n", sec_result);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    smpd_dbg_printf("calling InitializeSecurityContext: target = %s\n", sspi_context->target);
    t1 = PMPI_Wtime();
    sec_result = sec_result_copy = smpd_process.sec_fn->InitializeSecurityContext(
	&sspi_context->credential,
	&sspi_context->context,
	sspi_context->target,
	sspi_context->flags,
	0,
	/*SECURITY_NATIVE_DREP, */SECURITY_NETWORK_DREP,
	&inbound_descriptor, 0, &sspi_context->context,
	&outbound_descriptor, &attr, &ts);
    t2 = PMPI_Wtime();
    smpd_dbg_printf("InitializeSecurityContext took %0.6f seconds\n", t2-t1);
    switch (sec_result)
    {
    case SEC_E_OK:
	smpd_dbg_printf("SEC_E_OK\n");
	break;
    case SEC_I_COMPLETE_NEEDED:
	smpd_dbg_printf("SEC_I_COMPLETE_NEEDED\n");
    case SEC_I_COMPLETE_AND_CONTINUE:
	if (sec_result == SEC_I_COMPLETE_AND_CONTINUE)
	{
	    smpd_dbg_printf("SEC_I_COMPLETE_AND_CONTINUE\n");
	}
	smpd_dbg_printf("calling CompleteAuthToken\n");
	sec_result = smpd_process.sec_fn->CompleteAuthToken(&sspi_context->context, &outbound_descriptor);
	if (sec_result != SEC_E_OK)
	{
	    smpd_process.sec_fn->DeleteSecurityContext(&sspi_context->context);
	    smpd_process.sec_fn->FreeCredentialsHandle(&sspi_context->credential);
	    smpd_err_printf("CompleteAuthToken failed with error %d\n", sec_result);
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
	if (sec_result_copy == SEC_I_COMPLETE_NEEDED)
	    break;
    case SEC_I_CONTINUE_NEEDED:
	if (sec_result_copy == SEC_I_CONTINUE_NEEDED)
	{
	    smpd_dbg_printf("SEC_I_CONTINUE_NEEDED\n");
	}
	/*
	*sspi_buffer_pptr = outbound_buffer.pvBuffer;
	*length_ptr = outbound_buffer.cbBuffer;
	*/
	break;
    default:
	/* error occurred */
	smpd_translate_win_error(sec_result, err_msg, 256, NULL);
	smpd_err_printf("InitializeSecurityContext failed with error %d: %s\n", sec_result, err_msg);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    if (outbound_buffer.cbBuffer > 0)
    {
	*sspi_buffer_pptr = outbound_buffer.pvBuffer;
	*length_ptr = outbound_buffer.cbBuffer;
    }
    else
    {
	*sspi_buffer_pptr = NULL;
	*length_ptr = 0;
    }
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
#else
    smpd_enter_fn(FCNAME);
    smpd_err_printf("function not implemented.\n");
    smpd_exit_fn(FCNAME);
    return SMPD_FAIL;
#endif
}

#undef FCNAME
#define FCNAME "smpd_handle_sspi_iter_command"
int smpd_handle_sspi_iter_command(smpd_context_t *context)
{
#ifdef HAVE_WINDOWS_H
    int result;
    smpd_command_t *cmd, *temp_cmd;
    char context_str[20];
    int sspi_id;
    void *sspi_buffer;
    int sspi_buffer_length;
    SecPkgInfo *info;
    SECURITY_STATUS sec_result;

    smpd_enter_fn(FCNAME);

    cmd = &context->read_cmd;

    if (MPIU_Str_get_string_arg(cmd->cmd, "sspi_context", context_str, 20) != MPIU_STR_SUCCESS)
    {
	smpd_err_printf("no context parameter in the sspi_init command: '%s'\n", cmd->cmd);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    if (MPIU_Str_get_int_arg(cmd->cmd, "sspi_id", &sspi_id) != MPIU_STR_SUCCESS)
    {
	smpd_err_printf("no sspi_id parameter in the sspi_iter command: '%s'\n", cmd->cmd);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    /* get the maximum length the binary data can be */
    if (smpd_process.sec_fn == NULL)
    {
	smpd_dbg_printf("calling InitSecurityInterface\n");
	smpd_process.sec_fn = InitSecurityInterface();
	if (smpd_process.sec_fn == NULL)
	{
	    smpd_err_printf("unable to initialize the sspi interface.\n");
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
    }
    smpd_dbg_printf("calling QuerySecurityPackageInfo\n");
    sec_result = smpd_process.sec_fn->QuerySecurityPackageInfo(SMPD_SECURITY_PACKAGE, &info);
    if (sec_result != SEC_E_OK)
    {
	smpd_err_printf("unable to query the security package, error %d\n", sec_result);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    smpd_dbg_printf("%s package, %s, with: max %d byte token, capabilities bitmask 0x%x\n",
	info->Name, info->Comment, info->cbMaxToken, info->fCapabilities);
    sspi_buffer = MPIU_Malloc(info->cbMaxToken);
    if (sspi_buffer == NULL)
    {
	smpd_err_printf("unable to allocate an sspi buffer\n");
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    if (MPIU_Str_get_binary_arg(cmd->cmd, "data", sspi_buffer, info->cbMaxToken, &sspi_buffer_length) != MPIU_STR_SUCCESS)
    {
	smpd_err_printf("unable to get the data parameter from the sspi_iter command: '%s'\n", cmd->cmd);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    smpd_dbg_printf("calling FreeContextBuffer\n");
    sec_result = smpd_process.sec_fn->FreeContextBuffer(info);
    if (sec_result != SEC_E_OK)
    {
	smpd_err_printf("unable to free the security info structure, error %d\n", sec_result);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    /* prepare the result command */
    result = smpd_create_command("result", smpd_process.id, cmd->src, SMPD_FALSE, &temp_cmd);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to create a result command for a cred_request command.\n");
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    /* add the command tag for result matching */
    result = smpd_add_command_int_arg(temp_cmd, "cmd_tag", cmd->tag);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the tag to the result command for a cred_request command.\n");
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
    result = smpd_add_command_arg(temp_cmd, "sspi_context", context_str);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add context_str to the result command for a %s command\n", cmd->cmd_str);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    result = smpd_sspi_context_iter(sspi_id, &sspi_buffer, &sspi_buffer_length);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to iterate on the sspi buffer.\n");
	goto fn_fail;
    }

    result = smpd_add_command_int_arg(temp_cmd, "data_length", sspi_buffer_length);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the data_length parameter to the result command.\n");
	smpd_exit_fn(FCNAME);
	return result;
    }

    if (sspi_buffer_length > 0)
    {
	result = smpd_add_command_binary_arg(temp_cmd, "data", sspi_buffer, sspi_buffer_length);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to add the data parameter to the result command.\n");
	    smpd_exit_fn(FCNAME);
	    return result;
	}
    }

    result = smpd_add_command_arg(temp_cmd, "result", SMPD_SUCCESS_STR);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the result parameter to the result command.\n");
	smpd_exit_fn(FCNAME);
	return result;
    }

fn_exit:
    /* send result back */
    result = smpd_post_write_command(context, temp_cmd);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to post a write of the result command to the %s context.\n", smpd_get_context_str(context));
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    smpd_exit_fn(FCNAME);
    return result;
fn_fail:
    result = smpd_add_command_arg(temp_cmd, "result", SMPD_FAIL_STR);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the result parameter to the result command.\n");
	smpd_exit_fn(FCNAME);
	return result;
    }
    goto fn_exit;
#else
    smpd_enter_fn(FCNAME);
    smpd_err_printf("function not implemented.\n");
    smpd_exit_fn(FCNAME);
    return SMPD_FAIL;
#endif
}

#undef FCNAME
#define FCNAME "smpd_handle_exit_on_done_command"
int smpd_handle_exit_on_done_command(smpd_context_t *context)
{
    int result;
    smpd_command_t *cmd, *temp_cmd;

    smpd_enter_fn(FCNAME);

    cmd = &context->read_cmd;
    smpd_process.exit_on_done = SMPD_TRUE;

    /* prepare the result command */
    result = smpd_create_command("result", smpd_process.id, cmd->src, SMPD_FALSE, &temp_cmd);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to create a result command for a exit_on_done command.\n");
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    /* add the command tag for result matching */
    result = smpd_add_command_int_arg(temp_cmd, "cmd_tag", cmd->tag);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the tag to the result command for a exit_on_done command.\n");
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

    /* add the result */
    result = smpd_add_command_arg(temp_cmd, "result", SMPD_SUCCESS_STR);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the result string to the result command for a exit_on_done command.\n");
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    /* send result back */
    smpd_dbg_printf("replying to exit_on_done command: \"%s\"\n", temp_cmd->cmd);
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
#define FCNAME "smpd_handle_exit_command"
int smpd_handle_exit_command(smpd_context_t *context)
{
    smpd_command_t *cmd;
    int exitcode, iproc;
    char name[SMPD_MAX_DBS_NAME_LEN+1] = "";
    smpd_process_group_t *pg;
    int i, print;

    smpd_enter_fn(FCNAME);

    cmd = &context->read_cmd;

    if (MPIU_Str_get_int_arg(cmd->cmd, "code", &exitcode) != MPIU_STR_SUCCESS)
    {
	smpd_err_printf("no exit code in exit command: '%s'\n", cmd->cmd);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    if (exitcode != 0)
    {
	/* mpiexec will return the last non-zero exit code returned by a process. */
	smpd_process.mpiexec_exit_code = exitcode;
    }
    if (MPIU_Str_get_int_arg(cmd->cmd, "rank", &iproc) != MPIU_STR_SUCCESS)
    {
	smpd_err_printf("no iproc in exit command: '%s'\n", cmd->cmd);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    if (MPIU_Str_get_string_arg(cmd->cmd, "kvs", name, SMPD_MAX_DBS_NAME_LEN) != MPIU_STR_SUCCESS)
    {
	smpd_err_printf("no kvs in exit command: '%s'\n", cmd->cmd);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    pg = smpd_process.pg_list;
    while (pg)
    {
	if (strcmp(pg->kvs, name) == 0)
	{
	    if (iproc < 0 || iproc >= pg->num_procs)
	    {
		smpd_err_printf("received exit code for process out of range: process %d not in group of size %d.\n", iproc, pg->num_procs);
		smpd_exit_fn(FCNAME);
		return SMPD_FAIL;
	    }
	    if (pg->processes[iproc].exited)
	    {
		smpd_err_printf("received exit code for process %d more than once.\n", iproc);
		smpd_exit_fn(FCNAME);
		return SMPD_FAIL;
	    }
	    smpd_dbg_printf("saving exit code: rank %d, exitcode %d, pg <%s>\n", iproc, exitcode, name);
	    pg->processes[iproc].exited = SMPD_TRUE;
	    pg->processes[iproc].exitcode = exitcode;
	    if (pg->processes[iproc].init_called && !pg->processes[iproc].finalize_called && !pg->processes[iproc].suspended)
	    {
		/* process exited after init but before finalize */
		smpd_abort_job(pg->kvs, iproc, "process %d exited without calling finalize", iproc);
	    }
	    if (!pg->processes[iproc].init_called && !pg->processes[iproc].suspended)
	    {
		smpd_dbg_printf("process exited without calling init.\n");
		/* this process never called init or finalize, check to make sure no other process has called init */
		if (pg->any_init_received == SMPD_TRUE)
		{
		    smpd_abort_job(pg->kvs, iproc, "process %d exited without calling init while other processes have called init", iproc);
		}
		else
		{
		    pg->any_noinit_process_exited = SMPD_TRUE;
		    smpd_dbg_printf("process exited before anyone has called init.\n");
		}
	    }
	    break;
	}
	pg = pg->next;
    }

    if (pg && pg->aborted)
    {
	print = 1;
	for (i=0; i<pg->num_procs; i++)
	{
	    if (pg->processes[i].exited == SMPD_FALSE)
		print = 0;
	}
	if (print)
	{
	    if (smpd_process.verbose_abort_output)
	    {
		printf("\njob aborted:\n");
		printf("rank: node: exit code[: error message]\n");
		for (i=0; i<pg->num_procs; i++)
		{
		    printf("%d: %s: %d", i, pg->processes[i].host, pg->processes[i].exitcode);
		    if (pg->processes[i].errmsg != NULL)
		    {
			printf(": %s", pg->processes[i].errmsg);
		    }
		    printf("\n");
		}
	    }
	    else
	    {
		for (i=0; i<pg->num_procs; i++)
		{
		    if (pg->processes[i].errmsg != NULL)
		    {
			printf("%s\n", pg->processes[i].errmsg);
		    }
		}
	    }
	    fflush(stdout);
	}
    }
    else if (smpd_process.output_exit_codes)
    {
	if (pg)
	{
	    print = 1;
	    for (i=0; i<pg->num_procs; i++)
	    {
		if (pg->processes[i].exited == SMPD_FALSE)
		    print = 0;
	    }
	    if (print)
	    {
		printf("rank: node: exit code\n");
		for (i=0; i<pg->num_procs; i++)
		{
		    printf("%d: %s: %d\n", i, pg->processes[i].host, pg->processes[i].exitcode);
		}
		fflush(stdout);
	    }
	}
	else
	{
	    printf("process %d exited with exit code %d\n", iproc, exitcode);
	    fflush(stdout);
	}
    }

    /*printf("process %d exited with exit code %d\n", iproc, exitcode);fflush(stdout);*/
    smpd_process.nproc_exited++;
    if (smpd_process.nproc == smpd_process.nproc_exited)
    {
	/*printf("last process exited, returning SMPD_EXIT.\n");fflush(stdout);*/
	smpd_dbg_printf("last process exited, returning SMPD_EXIT.\n");
	smpd_exit_fn(FCNAME);
	return SMPD_EXIT;
    }
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_handle_suspend_result"
int smpd_handle_suspend_result(smpd_command_t *cmd, char *result_str)
{
    char name[SMPD_MAX_DBS_NAME_LEN+1] = "";
    int rank, i, exit_code;
    smpd_process_group_t *pg;
    int result;
    smpd_command_t *cmd_ptr;

    smpd_enter_fn(FCNAME);

    /* get the rank and pg name out of the command */
    if (MPIU_Str_get_string_arg(cmd->cmd, "name", name, SMPD_MAX_DBS_NAME_LEN) != MPIU_STR_SUCCESS)
    {
	smpd_err_printf("no kvs in suspend command: '%s'\n", cmd->cmd);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    if (MPIU_Str_get_int_arg(cmd->cmd, "rank", &rank) != MPIU_STR_SUCCESS)
    {
	smpd_err_printf("no rank in suspend command: '%s'\n", cmd->cmd);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    if (MPIU_Str_get_int_arg(cmd->cmd, "exit_code", &exit_code) != MPIU_STR_SUCCESS)
    {
	smpd_err_printf("no exit code in suspend command: '%s'\n", cmd->cmd);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    /* look up the pg */
    pg = smpd_process.pg_list;
    while (pg)
    {
	if (strcmp(pg->kvs, name) == 0)
	{
	    break;
	}
	pg = pg->next;
    }
    if (pg == NULL)
    {
	smpd_err_printf("received suspend result for process with no matching process group: rank %d, name <%s>\n", rank, name);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    /* save whether the suspend was successful or not */
    pg->processes[rank].suspended = (strcmp(result_str, SMPD_SUCCESS_STR) == 0) ? SMPD_TRUE : SMPD_FALSE;

    /* decrement then num_outstanding_suspends */
    pg->num_pending_suspends--;
    if (rank < 0 || rank >= pg->num_procs)
    {
	smpd_err_printf("received suspend result for process out of range: process %d not in group of size %d.\n", rank, pg->num_procs);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    /* if num_outstanding goes to zero, send kill commands */
    if (pg->num_pending_suspends == 0)
    {
	for (i=0; i<pg->num_procs; i++)
	{
	    if (pg->processes[i].exited)
	    {
		smpd_dbg_printf("suspended rank %d already exited, no need to kill it.\n", i);
	    }
	    else
	    {
		/* create the kill command */
		result = smpd_create_command("kill", smpd_process.id, pg->processes[i].node_id, SMPD_FALSE, &cmd_ptr);
		if (result != SMPD_SUCCESS)
		{
		    smpd_err_printf("unable to create a kill command.\n");
		    smpd_exit_fn(FCNAME);
		    return SMPD_FAIL;
		}
		result = smpd_add_command_int_arg(cmd_ptr, "exit_code", exit_code);
		if (result != SMPD_SUCCESS)
		{
		    smpd_err_printf("unable to add the exit code %d to the kill command\n", exit_code);
		    smpd_exit_fn(FCNAME);
		    return SMPD_FAIL;
		}
		result = smpd_add_command_arg(cmd_ptr, "ctx_key", pg->processes[i].ctx_key);
		if (result != SMPD_SUCCESS)
		{
		    smpd_err_printf("unable to add the ctx_key to the kill command: '%s'\n", pg->processes[i].ctx_key);
		    smpd_exit_fn(FCNAME);
		    return SMPD_FAIL;
		}

		/* send the kill command */
		result = smpd_post_write_command(smpd_process.left_context, cmd_ptr);
		if (result != SMPD_SUCCESS)
		{
		    smpd_err_printf("unable to post a write for the kill command: rank %d\n", i);
		    smpd_exit_fn(FCNAME);
		    return SMPD_FAIL;
		}
	    }
	}
    }

    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_handle_suspend_command"
int smpd_handle_suspend_command(smpd_context_t *context)
{
    smpd_command_t *cmd, *temp_cmd;
    char ctx_key[100];
    int process_id;
    smpd_context_t *pmi_context;
    smpd_process_t *piter;
    int result;
    char error_str[1024];

    smpd_enter_fn(FCNAME);

    cmd = &context->read_cmd;
    
    /* prepare the result command */
    result = smpd_create_command("result", smpd_process.id, cmd->src, SMPD_FALSE, &temp_cmd);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to create a result command for the init command '%s'.\n", cmd->cmd);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    /* add the command tag for result matching */
    result = smpd_add_command_int_arg(temp_cmd, "cmd_tag", cmd->tag);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the tag to the result command for the init command '%s'.\n", cmd->cmd);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    /* get the ctx_key of the process to be suspended */
    if (MPIU_Str_get_string_arg(cmd->cmd, "ctx_key", ctx_key, 100) != MPIU_STR_SUCCESS)
    {
	smpd_err_printf("no ctx_key in suspend command: '%s'\n", cmd->cmd);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    process_id = atoi(ctx_key);
    pmi_context = NULL;
    piter = smpd_process.process_list;
    while (piter)
    {
	if (piter->id == process_id)
	{
	    pmi_context = piter->pmi;
	    break;
	}
	piter = piter->next;
    }
    if (pmi_context == NULL)
    {
	smpd_err_printf("received suspend command for a pmi context that doesn't exist: unmatched id = %d\n", process_id);
	result = smpd_add_command_arg(temp_cmd, "result", SMPD_FAIL_STR" - no matching pmi context.");
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to add the result string to the result command for a suspend command '%s'.\n", cmd->cmd);
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
	smpd_dbg_printf("sending result command to %s context: \"%s\"\n", smpd_get_context_str(context), temp_cmd->cmd);
	result = smpd_post_write_command(context, temp_cmd);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to post a write of the result command to the context: cmd '%s', suspend cmd '%s'", temp_cmd->cmd, cmd->cmd);
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
	smpd_exit_fn(FCNAME);
	return SMPD_SUCCESS;
    }

    if (pmi_context->process == NULL)
    {
	smpd_err_printf("received suspend command for a pmi context that does not have a process structure.\n");
	result = smpd_add_command_arg(temp_cmd, "result", SMPD_FAIL_STR" - no process in the pmi context.");
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to add the result string to the result command for a suspend command '%s'.\n", cmd->cmd);
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
	smpd_dbg_printf("sending result command to %s context: \"%s\"\n", smpd_get_context_str(context), temp_cmd->cmd);
	result = smpd_post_write_command(context, temp_cmd);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to post a write of the result command to the context: cmd '%s', suspend cmd '%s'", temp_cmd->cmd, cmd->cmd);
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
	smpd_exit_fn(FCNAME);
	return SMPD_SUCCESS;
    }
    
    result = smpd_suspend_process(pmi_context->process);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to suspend process.\n");
	snprintf(error_str, 1024, SMPD_FAIL_STR" - unable to suspend process, error %d", result);
	result = smpd_add_command_arg(temp_cmd, "result", error_str);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to add the result string to the result command for a suspend command '%s'.\n", cmd->cmd);
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
    }
    else
    {
	result = smpd_add_command_arg(temp_cmd, "result", SMPD_SUCCESS_STR);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to add the result string to the result command for a suspend command '%s'.\n", cmd->cmd);
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
    }

    /* return result */
    smpd_dbg_printf("sending result command to %s context: \"%s\"\n", smpd_get_context_str(context), temp_cmd->cmd);
    result = smpd_post_write_command(context, temp_cmd);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to post a write of the result command to the context: cmd '%s', suspend cmd '%s'", temp_cmd->cmd, cmd->cmd);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_handle_kill_command"
int smpd_handle_kill_command(smpd_context_t *context)
{
    smpd_command_t *cmd, *temp_cmd;
    char ctx_key[100];
    int process_id;
    smpd_context_t *pmi_context;
    smpd_process_t *piter;
    int result;
    int exit_code;

    smpd_enter_fn(FCNAME);

    cmd = &context->read_cmd;
    
    if (MPIU_Str_get_string_arg(cmd->cmd, "ctx_key", ctx_key, 100) != MPIU_STR_SUCCESS)
    {
	smpd_err_printf("no ctx_key in suspend command: '%s'\n", cmd->cmd);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    if (MPIU_Str_get_int_arg(cmd->cmd, "exit_code", &exit_code) != MPIU_STR_SUCCESS)
    {
	smpd_err_printf("no exit code in suspend command: '%s'\n", cmd->cmd);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    process_id = atoi(ctx_key);
    pmi_context = NULL;
    piter = smpd_process.process_list;
    while (piter)
    {
	if (piter->id == process_id)
	{
	    pmi_context = piter->pmi;
	    break;
	}
	piter = piter->next;
    }
    if (pmi_context == NULL)
    {
	smpd_err_printf("received kill command for a pmi context that doesn't exist: unmatched id = %d\n", process_id);
	smpd_exit_fn(FCNAME);
	return SMPD_SUCCESS;
    }

    if (pmi_context->process == NULL)
    {
	smpd_err_printf("received kill command for a pmi context that does not have a process structure.\n");
	smpd_exit_fn(FCNAME);
	return SMPD_SUCCESS;
    }
    
    result = smpd_kill_process(pmi_context->process, exit_code);
    if (result != SMPD_SUCCESS){
	    smpd_err_printf("unable to kill process. result = %d\n", result);
        pmi_context->state = SMPD_CLOSING;
        if(pmi_context->process->in){
            smpd_dbg_printf("Closing stdin ...\n");
            result = SMPDU_Sock_post_close(pmi_context->process->in->sock);
            if(result != SMPD_SUCCESS){
                smpd_err_printf("Unable to post close on stdin sock\n");
            }
        }
        if(pmi_context->process->out){
            smpd_dbg_printf("Closing stdout ...\n");
            result = SMPDU_Sock_post_close(pmi_context->process->out->sock);
            if(result != SMPD_SUCCESS){
                smpd_err_printf("Unable to post close on stdout sock\n");
            }
        }
        if(pmi_context->process->err){
            smpd_dbg_printf("Closing stderr ...\n");
            result = SMPDU_Sock_post_close(pmi_context->process->err->sock);
            if(result != SMPD_SUCCESS){
                smpd_err_printf("Unable to post close on stderr sock\n");
            }
        }
        if(!(pmi_context->process->is_singleton_client)){
            if(pmi_context->process->pmi){
                smpd_dbg_printf("Closing pmi ...\n");
                result = SMPDU_Sock_post_close(pmi_context->process->pmi->sock);
                if(result != SMPD_SUCCESS){
                    smpd_err_printf("Unable to post close on pmi sock\n");
                }
            }
        }
        else{
            /* Send the "die" command to the process */
            smpd_command_t *temp_cmd;
            result = smpd_create_command("die", smpd_process.id, pmi_context->process->id, SMPD_FALSE, &temp_cmd);
            if(result != SMPD_SUCCESS){
                smpd_err_printf("Unable to create 'die' command for singleton client \n");
                smpd_exit_fn(FCNAME);
                return SMPD_FAIL;
            }
            result = smpd_post_write_command(pmi_context, temp_cmd);
            if(result != SMPD_SUCCESS){
                smpd_err_printf("Unable to post 'die' command for singleton client \n");
                smpd_exit_fn(FCNAME);
                return SMPD_FAIL;
            }
        }
    	smpd_exit_fn(FCNAME);
	    return SMPD_SUCCESS;
    }

    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_handle_pmi_listen_command"
int smpd_handle_pmi_listen_command(smpd_context_t *context)
{
    int result;
    smpd_command_t *cmd, *temp_cmd;
    int nproc;
    SMPDU_Sock_t sock_pmi_listener;
    smpd_context_t *listener_context;
    int listener_port;
    /*smpd_process_t *process;*/
    char host_description[256];

    smpd_enter_fn(FCNAME);

    cmd = &context->read_cmd;

    result = MPIU_Str_get_int_arg(cmd->cmd, "nproc", &nproc);
    if (result != MPIU_STR_SUCCESS)
    {
	smpd_err_printf("no nproc field in the pmi_listen command: '%s'\n", cmd->cmd);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    /* start a pmi listener for nproc processes to connect to */
    /* insert code here */
    /*
    result = smpd_create_process_struct(-1, &process);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to create a process structure for the pmi_listen command\n");
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    */

    listener_port = 0;
    result = SMPDU_Sock_listen(context->set, NULL, &listener_port, &sock_pmi_listener); 
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("SMPDU_Sock_listen failed,\nsock error: %s\n", get_sock_error_string(result));
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    smpd_dbg_printf("pmiserver listening on port %d\n", listener_port);

    result = smpd_create_context(SMPD_CONTEXT_PMI_LISTENER, context->set, sock_pmi_listener, -1, &listener_context);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to create a context for the pmi listener.\n");
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    result = SMPDU_Sock_set_user_ptr(sock_pmi_listener, listener_context);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("SMPDU_Sock_set_user_ptr failed,\nsock error: %s\n", get_sock_error_string(result));
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    listener_context->state = SMPD_SMPD_LISTENING/*SMPD_PMI_SERVER_LISTENING*/;
    result = smpd_get_hostname(host_description, 256);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("smpd_get_hostname failed\n");
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    /*result = SMPDU_Sock_get_host_description(host_description, 256);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("SMPDU_Sock_get_host_description failed,\nsock error: %s\n", get_sock_error_string(result));
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    */
    /* save the listener sock in the slot reserved for the client sock so we can match the listener
    to the process structure when the client sock is accepted */
    /*process->pmi->sock = sock_pmi_listener;*/

    /* create the result command */
    result = smpd_create_command("result", smpd_process.id, cmd->src, SMPD_FALSE, &temp_cmd);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to create a result command in response to pmi_listen command: '%s'\n", cmd->cmd);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    result = smpd_add_command_int_arg(temp_cmd, "cmd_tag", cmd->tag);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the tag to the result command in response to pmi_listen command: '%s'\n", cmd->cmd);
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
    result = smpd_add_command_arg(temp_cmd, "result", SMPD_SUCCESS_STR);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the result field to the result command in response to pmi_listen command: '%s'\n", cmd->cmd);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    result = smpd_add_command_arg(temp_cmd, "host_description", host_description);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the host_description field to the result command in response to pmi_listen command: '%s'\n", cmd->cmd);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    result = smpd_add_command_int_arg(temp_cmd, "listener_port", listener_port);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the listener_port field to the result command in response to pmi_listen command: '%s'\n", cmd->cmd);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    result = smpd_post_write_command(context, temp_cmd);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to post a write of the result command in response to pmi_listen command: '%s'\n", cmd->cmd);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    smpd_exit_fn(FCNAME);
    return result;
}

#undef FCNAME
#define FCNAME "smpd_handle_add_job_command"
int smpd_handle_add_job_command(smpd_context_t *context)
{
#ifdef HAVE_WINDOWS_H
    int result = SMPD_SUCCESS;
    smpd_command_t *cmd, *temp_cmd;
    char result_str[100];
    char key[SMPD_MAX_NAME_LENGTH];
    char username[SMPD_MAX_NAME_LENGTH];

    smpd_enter_fn(FCNAME);

    cmd = &context->read_cmd;

    if (MPIU_Str_get_string_arg(cmd->cmd, "key", key, SMPD_MAX_NAME_LENGTH) != MPIU_STR_SUCCESS)
    {
	smpd_err_printf("add_job command missing key parameter\n");
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    if (MPIU_Str_get_string_arg(cmd->cmd, "username", username, SMPD_MAX_NAME_LENGTH) != MPIU_STR_SUCCESS)
    {
	smpd_err_printf("add_job command missing username parameter\n");
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    result = smpd_add_job_key(key, username, NULL, NULL);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to set job key %s=%s\n", key, username);
	strcpy(result_str, SMPD_FAIL_STR);
    }
    else
    {
	strcpy(result_str, SMPD_SUCCESS_STR);
    }

    /* prepare the result command */
    result = smpd_create_command("result", smpd_process.id, cmd->src, SMPD_FALSE, &temp_cmd);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to create a result command for a add job key %s=%s command.\n", key, username);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    /* add the command tag for result matching */
    result = smpd_add_command_int_arg(temp_cmd, "cmd_tag", cmd->tag);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the tag to the result command for a add job key %s=%s command.\n", key, username);
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
    result = smpd_add_command_arg(temp_cmd, "result", result_str);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the result string to the result command for a add job key %s=%s command.\n", key, username);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    /* send result back */
    smpd_dbg_printf("replying to add job key %s=%s command: \"%s\"\n", key, username, temp_cmd->cmd);
    result = smpd_post_write_command(context, temp_cmd);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to post a write of the result command to the context.\n");
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    smpd_exit_fn(FCNAME);
    return result;
#else
    smpd_enter_fn(FCNAME);
    smpd_exit_fn(FCNAME);
    return SMPD_FAIL;
#endif
}

#undef FCNAME
#define FCNAME "smpd_handle_add_job_command_and_password"
int smpd_handle_add_job_command_and_password(smpd_context_t *context)
{
#ifdef HAVE_WINDOWS_H
    int result = SMPD_SUCCESS;
    smpd_command_t *cmd, *temp_cmd;
    char result_str[100];
    char key[SMPD_MAX_NAME_LENGTH];
    char value[SMPD_MAX_NAME_LENGTH];
    char encrypted[SMPD_MAX_PASSWORD_LENGTH];
    char decrypted[SMPD_MAX_PASSWORD_LENGTH];
    char account[SMPD_MAX_ACCOUNT_LENGTH], domain[SMPD_MAX_ACCOUNT_LENGTH];
    int len;
    HANDLE hUser;

    smpd_enter_fn(FCNAME);

    cmd = &context->read_cmd;

    if (MPIU_Str_get_string_arg(cmd->cmd, "key", key, SMPD_MAX_NAME_LENGTH) != MPIU_STR_SUCCESS)
    {
	smpd_err_printf("add_job_and_password command missing key parameter\n");
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    if (MPIU_Str_get_string_arg(cmd->cmd, "username", value, SMPD_MAX_NAME_LENGTH) != MPIU_STR_SUCCESS)
    {
	smpd_err_printf("add_job_and_password command missing username parameter\n");
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    if (MPIU_Str_get_string_arg(cmd->cmd, "password", encrypted, SMPD_MAX_PASSWORD_LENGTH) != MPIU_STR_SUCCESS)
    {
	smpd_err_printf("add_job_and_password command missing password parameter\n");
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    len = SMPD_MAX_PASSWORD_LENGTH;
    result = smpd_decrypt_data(encrypted, (int)strlen(encrypted), decrypted, &len);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to decrypt the password in the add_job_and_password command.\n");
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    if (len < 0 || len >= SMPD_MAX_PASSWORD_LENGTH)
    {
	smpd_err_printf("invalid password length: %d\n", len);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    decrypted[len] = '\0';

    account[0] = '\0';
    domain[0] = '\0';
    smpd_parse_account_domain(value, account, domain);
    result = smpd_get_user_handle(account, domain[0] != '\0' ? domain : NULL, decrypted, &hUser);
    if (result == SMPD_SUCCESS)
    {
	result = smpd_add_job_key_and_handle(key, value, NULL, NULL, hUser);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to set job key %s=%s:%p\n", key, value, hUser);
	    strcpy(result_str, SMPD_FAIL_STR);
	}
	else
	{
	    strcpy(result_str, SMPD_SUCCESS_STR);
	}
    }
    else
    {
	smpd_dbg_printf("smpd_get_user_handle(%s,%s,%d) returned error: %d\n", account, domain, strlen(decrypted), result);
	strcpy(result_str, SMPD_FAIL_STR);
    }

    /* prepare the result command */
    result = smpd_create_command("result", smpd_process.id, cmd->src, SMPD_FALSE, &temp_cmd);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to create a result command for a add job key %s=%s command.\n", key, value);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    /* add the command tag for result matching */
    result = smpd_add_command_int_arg(temp_cmd, "cmd_tag", cmd->tag);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the tag to the result command for a add job key %s=%s command.\n", key, value);
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
    result = smpd_add_command_arg(temp_cmd, "result", result_str);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the result string to the result command for a add job key %s=%s command.\n", key, value);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    /* send result back */
    smpd_dbg_printf("replying to add job key %s=%s command: \"%s\"\n", key, value, temp_cmd->cmd);
    result = smpd_post_write_command(context, temp_cmd);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to post a write of the result command to the context.\n");
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    smpd_exit_fn(FCNAME);
    return result;
#else
    smpd_enter_fn(FCNAME);
    smpd_exit_fn(FCNAME);
    return SMPD_FAIL;
#endif
}

#undef FCNAME
#define FCNAME "smpd_handle_remove_job_command"
int smpd_handle_remove_job_command(smpd_context_t *context)
{
#ifdef HAVE_WINDOWS_H
    int result = SMPD_SUCCESS;
    smpd_command_t *cmd, *temp_cmd;
    char result_str[100];
    char key[SMPD_MAX_NAME_LENGTH];

    smpd_enter_fn(FCNAME);

    cmd = &context->read_cmd;

    if (MPIU_Str_get_string_arg(cmd->cmd, "key", key, SMPD_MAX_NAME_LENGTH) != MPIU_STR_SUCCESS)
    {
	smpd_err_printf("remove_job command missing key parameter\n");
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    result = smpd_remove_job_key(key);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to remove the job key %s\n", key);
	strcpy(result_str, SMPD_FAIL_STR);
    }
    else
    {
	strcpy(result_str, SMPD_SUCCESS_STR);
    }

    /* prepare the result command */
    result = smpd_create_command("result", smpd_process.id, cmd->src, SMPD_FALSE, &temp_cmd);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to create a result command for a remove job key %s command.\n", key);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    /* add the command tag for result matching */
    result = smpd_add_command_int_arg(temp_cmd, "cmd_tag", cmd->tag);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the tag to the result command for a remove job key %s command.\n", key);
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
    result = smpd_add_command_arg(temp_cmd, "result", result_str);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the result string to the result command for a remove job key %s command.\n", key);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    /* send result back */
    smpd_dbg_printf("replying to remove job key %s command: \"%s\"\n", key, temp_cmd->cmd);
    result = smpd_post_write_command(context, temp_cmd);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to post a write of the result command to the context.\n");
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    smpd_exit_fn(FCNAME);
    return result;
#else
    smpd_enter_fn(FCNAME);
    smpd_exit_fn(FCNAME);
    return SMPD_FAIL;
#endif
}

#undef FCNAME
#define FCNAME "smpd_handle_associate_job_command"
int smpd_handle_associate_job_command(smpd_context_t *context)
{
#ifdef HAVE_WINDOWS_H
    int result = SMPD_SUCCESS;
    smpd_command_t *cmd, *temp_cmd;
    char result_str[100];
    char key[SMPD_MAX_NAME_LENGTH];

    smpd_enter_fn(FCNAME);

    cmd = &context->read_cmd;

    if (MPIU_Str_get_string_arg(cmd->cmd, "key", key, SMPD_MAX_NAME_LENGTH) != MPIU_STR_SUCCESS)
    {
	smpd_err_printf("associate_job command missing key parameter\n");
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    result = smpd_associate_job_key(key, context->account, context->domain, context->full_domain, context->sspi_context->user_handle);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to associate the job key %s\n", key);
	strcpy(result_str, SMPD_FAIL_STR);
    }
    else
    {
	strcpy(result_str, SMPD_SUCCESS_STR);
    }

    /* prepare the result command */
    result = smpd_create_command("result", smpd_process.id, cmd->src, SMPD_FALSE, &temp_cmd);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to create a result command for a associate job key %s command.\n", key);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    /* add the command tag for result matching */
    result = smpd_add_command_int_arg(temp_cmd, "cmd_tag", cmd->tag);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the tag to the result command for a associate job key %s command.\n", key);
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
    result = smpd_add_command_arg(temp_cmd, "result", result_str);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the result string to the result command for a associate job key %s command.\n", key);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    /* send result back */
    smpd_dbg_printf("replying to associate job key %s command: \"%s\"\n", key, temp_cmd->cmd);
    result = smpd_post_write_command(context, temp_cmd);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to post a write of the result command to the context.\n");
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    smpd_exit_fn(FCNAME);
    return result;
#else
    smpd_enter_fn(FCNAME);
    smpd_exit_fn(FCNAME);
    return SMPD_FAIL;
#endif
}

#undef FCNAME
#define FCNAME "smpd_fail_unexpected_command"
int smpd_fail_unexpected_command(smpd_context_t *context)
{
    int result = SMPD_SUCCESS;
    smpd_command_t *cmd, *temp_cmd;

    smpd_enter_fn(FCNAME);

    cmd = &context->read_cmd;

    /* prepare the result command */
    result = smpd_create_command("result", smpd_process.id, cmd->src, SMPD_FALSE, &temp_cmd);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to create a result command.\n");
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    /* add the command tag for result matching */
    result = smpd_add_command_int_arg(temp_cmd, "cmd_tag", cmd->tag);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the tag to the result command.\n");
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    result = smpd_add_command_arg(temp_cmd, "cmd_orig", cmd->cmd_str);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add cmd_orig to the result command\n");
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    result = smpd_add_command_arg(temp_cmd, "result", SMPD_FAIL_STR);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the result string to the result command.\n");
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    /* send result back */
    smpd_dbg_printf("replying with failure to unknown command: \"%s\"\n", temp_cmd->cmd);
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
#define FCNAME "smpd_generic_fail_command"
int smpd_generic_fail_command(smpd_context_t *context)
{
    int result = SMPD_SUCCESS;
    smpd_command_t *cmd, *temp_cmd;

    smpd_enter_fn(FCNAME);

    cmd = &context->read_cmd;

    /* prepare the result command */
    result = smpd_create_command("result", smpd_process.id, cmd->src, SMPD_FALSE, &temp_cmd);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to create a result command.\n");
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    /* add the command tag for result matching */
    result = smpd_add_command_int_arg(temp_cmd, "cmd_tag", cmd->tag);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the tag to the result command.\n");
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    result = smpd_add_command_arg(temp_cmd, "cmd_orig", cmd->cmd_str);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add cmd_orig to the result command\n");
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    result = smpd_add_command_arg(temp_cmd, "result", SMPD_FAIL_STR);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the result string to the result command.\n");
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    /* send result back */
    smpd_dbg_printf("replying with failure to command: \"%s\"\n", temp_cmd->cmd);
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
#define FCNAME "smpd_handle_singinit_info_command"
int smpd_handle_singinit_info_command(smpd_context_t *context){
    int result;
    smpd_command_t *cmd;

    smpd_enter_fn(FCNAME);
    cmd = &context->read_cmd;

    if (MPIU_Str_get_string_arg(cmd->cmd, "kvsname", smpd_process.kvs_name, 
            SMPD_SINGLETON_MAX_KVS_NAME_LEN) != MPIU_STR_SUCCESS){
	    smpd_err_printf("singinit_info command missing kvsname\n");
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
    }

    if (MPIU_Str_get_string_arg(cmd->cmd, "domainname", smpd_process.domain_name, 
            SMPD_SINGLETON_MAX_KVS_NAME_LEN) != MPIU_STR_SUCCESS){
	    smpd_err_printf("singinit_info command missing domainname\n");
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
    }

    if (MPIU_Str_get_string_arg(cmd->cmd, "host", smpd_process.host, 
            SMPD_SINGLETON_MAX_HOST_NAME_LEN) != MPIU_STR_SUCCESS){
        smpd_err_printf("singinit_info command missing hostname\n");
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
    }

    if (MPIU_Str_get_int_arg(cmd->cmd, "port", &smpd_process.port) 
            != MPIU_STR_SUCCESS){
        smpd_err_printf("singinit_info command missing pm port number\n");
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
    }

    context->state = SMPD_DONE;
    result = SMPDU_Sock_post_close(context->sock);
    if( result != SMPD_SUCCESS){
        smpd_err_printf("SMPDU_Sock_post_close failed , error = %s\n", get_sock_error_string(result));
    }

    smpd_exit_fn(FCNAME);
    return SMPD_CLOSE;
}

#undef FCNAME
#define FCNAME "smpd_handle_command"
int smpd_handle_command(smpd_context_t *context)
{
    int result;
    smpd_context_t *dest;
    smpd_command_t *cmd, *temp_cmd;

    smpd_enter_fn(FCNAME);

    cmd = &context->read_cmd;

    smpd_dbg_printf("handling command:\n");
    smpd_dbg_printf(" src  = %d\n", cmd->src);
    smpd_dbg_printf(" dest = %d\n", cmd->dest);
    smpd_dbg_printf(" cmd  = %s\n", cmd->cmd_str);
    smpd_dbg_printf(" tag  = %d\n", cmd->tag);
    smpd_dbg_printf(" ctx  = %s\n", smpd_get_context_str(context));
    smpd_dbg_printf(" len  = %d\n", cmd->length);
    smpd_dbg_printf(" str  = %s\n", cmd->cmd);

    /* set the command state to handled */
    context->read_cmd.state = SMPD_CMD_HANDLED;

    /* FIXME: Assign the appropriate src/dst for singinit/die */
    if (strcmp(cmd->cmd_str, "singinit_info") == 0){
        result = smpd_handle_singinit_info_command(context);
        smpd_exit_fn(FCNAME);
        return result;
    }
    if(strcmp(cmd->cmd_str, "die") == 0){
        result = smpd_handle_die_command(context);
        smpd_exit_fn(FCNAME);
        return result;
    }

    result = smpd_command_destination(cmd->dest, &dest);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("invalid command received, unable to determine the destination: '%s'\n", cmd->cmd);
	smpd_exit_fn(FCNAME);
	return SMPD_SUCCESS;
    }
    if (dest)
    {
	smpd_dbg_printf("forwarding command to %d\n", dest->id);
	result = smpd_forward_command(context, dest);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to forward the command.\n");
	    smpd_exit_fn(FCNAME);
	    return SMPD_SUCCESS;
	}
	smpd_exit_fn(FCNAME);
	return SMPD_SUCCESS;
    }

    if (context->access == SMPD_ACCESS_USER_PROCESS || context->access == SMPD_ACCESS_ADMIN)
    {
	if (strcmp(cmd->cmd_str, "close") == 0)
	{
	    result = smpd_handle_close_command(context);
	    smpd_exit_fn(FCNAME);
	    return result;
	}
	else if (strcmp(cmd->cmd_str, "closed") == 0)
	{
	    result = smpd_handle_closed_command(context);
	    smpd_exit_fn(FCNAME);
	    return result;
	}
	else if (strcmp(cmd->cmd_str, "closed_request") == 0)
	{
	    result = smpd_handle_closed_request_command(context);
	    smpd_exit_fn(FCNAME);
	    return result;
	}
	else if (strcmp(cmd->cmd_str, "result") == 0)
	{
	    result = smpd_handle_result(context);
	    smpd_exit_fn(FCNAME);
	    return result;
	}
	else if (strcmp(cmd->cmd_str, "exit") == 0)
	{
	    result = smpd_handle_exit_command(context);
	    smpd_exit_fn(FCNAME);
	    return result;
	}
	else if (strcmp(cmd->cmd_str, "abort") == 0)
	{
	    result = smpd_handle_abort_command(context);
	    smpd_exit_fn(FCNAME);
	    return result;
	}
	else if (strcmp(cmd->cmd_str, "abort_job") == 0)
	{
	    result = smpd_handle_abort_job_command(context);
	    smpd_exit_fn(FCNAME);
	    return result;
	}
	else if (strcmp(cmd->cmd_str, "init") == 0)
	{
	    result = smpd_handle_init_command(context);
	    smpd_exit_fn(FCNAME);
	    return result;
	}
	else if (strcmp(cmd->cmd_str, "finalize") == 0)
	{
	    result = smpd_handle_finalize_command(context);
	    smpd_exit_fn(FCNAME);
	    return result;
	}
	else if (strcmp(cmd->cmd_str, "stdin") == 0)
	{
	    result = smpd_handle_stdin_command(context);
	    smpd_exit_fn(FCNAME);
	    return result;
	}
	else if (strcmp(cmd->cmd_str, "close_stdin") == 0)
	{
	    result = smpd_handle_close_stdin_command(context);
	    smpd_exit_fn(FCNAME);
	    return result;
	}
	else if (strcmp(cmd->cmd_str, "stdout") == 0)
	{
	    result = smpd_handle_stdout_command(context);
	    smpd_exit_fn(FCNAME);
	    return result;
	}
	else if (strcmp(cmd->cmd_str, "stderr") == 0)
	{
	    result = smpd_handle_stderr_command(context);
	    smpd_exit_fn(FCNAME);
	    return result;
	}
	else if (strcmp(cmd->cmd_str, "launch") == 0)
	{
	    result = smpd_handle_launch_command(context);
	    smpd_exit_fn(FCNAME);
	    return result;
	}
    else if (strcmp(cmd->cmd_str, "proc_info") == 0){
        result = smpd_handle_proc_info_command(context);
        smpd_exit_fn(FCNAME);
        return result;
    }
	else if (strcmp(cmd->cmd_str, "connect") == 0)
	{
	    result = smpd_handle_connect_command(context);
	    smpd_exit_fn(FCNAME);
	    return result;
	}
	else if (strcmp(cmd->cmd_str, "print") == 0)
	{
	    result = smpd_handle_print_command(context);
	    smpd_exit_fn(FCNAME);
	    return result;
	}
	else if (strcmp(cmd->cmd_str, "start_dbs") == 0)
	{
	    result = smpd_handle_start_dbs_command(context);
	    smpd_exit_fn(FCNAME);
	    return result;
	}
	else if (strcmp(cmd->cmd_str, "pmi_listen") == 0)
	{
	    result = smpd_handle_pmi_listen_command(context);
	    smpd_exit_fn(FCNAME);
	    return result;
	}
	else if ((cmd->cmd_str[0] == 'd') && (cmd->cmd_str[1] == 'b'))
	{
	    /* handle database command */
	    result = smpd_handle_dbs_command(context);
	    smpd_exit_fn(FCNAME);
	    return result;
	}
	else if (strcmp(cmd->cmd_str, "barrier") == 0)
	{
	    result = smpd_handle_barrier_command(context);
	    smpd_exit_fn(FCNAME);
	    return result;
	}
	else if (strcmp(cmd->cmd_str, "cred_request") == 0)
	{
	    result = smpd_handle_cred_request_command(context);
	    smpd_exit_fn(FCNAME);
	    return result;
	}
	else if (strcmp(cmd->cmd_str, "sspi_init") == 0)
	{
	    result = smpd_handle_sspi_init_command(context);
	    smpd_exit_fn(FCNAME);
	    return result;
	}
	else if (strcmp(cmd->cmd_str, "sspi_iter") == 0)
	{
	    result = smpd_handle_sspi_iter_command(context);
	    smpd_exit_fn(FCNAME);
	    return result;
	}
	else if (strcmp(cmd->cmd_str, "down") == 0)
	{
	    context->state = SMPD_EXITING;
	    result = SMPDU_Sock_post_close(context->sock);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to post a close on sock %d,\nsock error: %s\n",
		    SMPDU_Sock_get_sock_id(context->sock), get_sock_error_string(result));
		smpd_exit_fn(FCNAME);
		return SMPD_FAIL;
	    }
	    smpd_exit_fn(FCNAME);
	    return SMPD_EXITING;
	}
	else if (strcmp(cmd->cmd_str, "exit_on_done") == 0)
	{
	    result = smpd_handle_exit_on_done_command(context);
	    smpd_exit_fn(FCNAME);
	    return result;
	}
	else if (strcmp(cmd->cmd_str, "done") == 0)
	{
	    if (context->type != SMPD_CONTEXT_PMI)
	    {
		smpd_err_printf("done command read on %s context.\n", smpd_get_context_str(context));
	    }
	    context->state = SMPD_CLOSING;
	    result = SMPDU_Sock_post_close(context->sock);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to post a close on sock %d,\nsock error: %s\n",
		    SMPDU_Sock_get_sock_id(context->sock), get_sock_error_string(result));
		smpd_exit_fn(FCNAME);
		return SMPD_FAIL;
	    }
	    if (smpd_process.exit_on_done)
	    {
		smpd_process.nproc_exited++;
		/*printf("%d exited\n", smpd_process.nproc_exited);*/
		if (smpd_process.nproc == smpd_process.nproc_exited)
		{
		    context->state = SMPD_EXITING;
		    smpd_dbg_printf("last process exited, returning SMPD_EXIT.\n");
		    /*printf("last process exited, returning SMPD_EXIT.\n");fflush(stdout);*/
		    smpd_exit_fn(FCNAME);
		    return /*SMPD_EXIT*/ SMPD_EXITING;
		}
	    }
	    smpd_exit_fn(FCNAME);
	    return SMPD_CLOSE;
	}
	else if (strcmp(cmd->cmd_str, "spawn") == 0)
	{
	    result = smpd_handle_spawn_command(context);
	    smpd_exit_fn(FCNAME);
	    return result;
	}
	else if (strcmp(cmd->cmd_str, "suspend") == 0)
	{
	    result = smpd_handle_suspend_command(context);
	    smpd_exit_fn(FCNAME);
	    return result;
	}
	else if (strcmp(cmd->cmd_str, "kill") == 0)
	{
	    result = smpd_handle_kill_command(context);
	    smpd_exit_fn(FCNAME);
	    return result;
	}
	else
	{
	    /* handle root commands */
	    if (smpd_process.root_smpd)
	    {
		if ( (strcmp(cmd->cmd_str, "shutdown") == 0) || (strcmp(cmd->cmd_str, "restart") == 0) )
		{
		    if (strcmp(cmd->cmd_str, "restart") == 0)
			smpd_process.builtin_cmd = SMPD_CMD_RESTART;
		    result = smpd_create_command("down", smpd_process.id, cmd->src, SMPD_FALSE, &temp_cmd);
		    if (result != SMPD_SUCCESS)
		    {
			smpd_err_printf("unable to create a closed command for the context.\n");
			smpd_exit_fn(FCNAME);
			return SMPD_FAIL;
		    }
		    smpd_dbg_printf("shutdown received, replying with down command: \"%s\"\n", temp_cmd->cmd);
		    result = smpd_post_write_command(context, temp_cmd);
		    if (result != SMPD_SUCCESS)
		    {
			smpd_err_printf("unable to post a write of the closed command to the context.\n");
			smpd_exit_fn(FCNAME);
			return SMPD_FAIL;
		    }
		    smpd_exit_fn(FCNAME);
		    return SMPD_EXITING; /* return close to prevent posting another read on this context */
		}
		else if (strcmp(cmd->cmd_str, "validate") == 0)
		{
		    result = smpd_handle_validate_command(context);
		    smpd_exit_fn(FCNAME);
		    return result;
		}
		else if (strcmp(cmd->cmd_str, "status") == 0)
		{
		    result = smpd_handle_status_command(context);
		    smpd_exit_fn(FCNAME);
		    return result;
		}
		else if (strcmp(cmd->cmd_str, "stat") == 0)
		{
		    result = smpd_handle_stat_command(context);
		    smpd_exit_fn(FCNAME);
		    return result;
		}
		else if (strcmp(cmd->cmd_str, "get") == 0)
		{
		    result = smpd_handle_get_command(context);
		    smpd_exit_fn(FCNAME);
		    return result;
		}
		else if (strcmp(cmd->cmd_str, "set") == 0)
		{
		    result = smpd_handle_set_command(context);
		    smpd_exit_fn(FCNAME);
		    return result;
		}
		else if (strcmp(cmd->cmd_str, "delete") == 0)
		{
		    result = smpd_handle_delete_command(context);
		    smpd_exit_fn(FCNAME);
		    return result;
		}
		else if (strcmp(cmd->cmd_str, "add_job") == 0)
		{
		    result = smpd_handle_add_job_command(context);
		    smpd_exit_fn(FCNAME);
		    return result;
		}
		else if (strcmp(cmd->cmd_str, "add_job_and_password") == 0)
		{
		    result = smpd_handle_add_job_command_and_password(context);
		    smpd_exit_fn(FCNAME);
		    return result;
		}
		else if (strcmp(cmd->cmd_str, "remove_job") == 0)
		{
		    result = smpd_handle_remove_job_command(context);
		    smpd_exit_fn(FCNAME);
		    return result;
		}
		else if (strcmp(cmd->cmd_str, "associate_job") == 0)
		{
		    smpd_dbg_printf("associate_job command received with improper smpd access\n");
		    result = smpd_generic_fail_command(context);
		    smpd_exit_fn(FCNAME);
		    return result;
		}
		else
		{
		    smpd_err_printf("returning error for unknown smpd session command: \"%s\"\n", cmd->cmd);
		    result = smpd_fail_unexpected_command(context);
		    smpd_exit_fn(FCNAME);
		    return result;
		}
	    }
	    else
	    {
		smpd_err_printf("returning error for unknown process session command: \"%s\"\n", cmd->cmd);
		result = smpd_fail_unexpected_command(context);
		smpd_exit_fn(FCNAME);
		return result;
	    }
	}
    }
    if (context->access == SMPD_ACCESS_USER)
    {
	if (strcmp(cmd->cmd_str, "associate_job") == 0)
	{
	    result = smpd_handle_associate_job_command(context);
	    smpd_exit_fn(FCNAME);
	    return result;
	}
	else if (strcmp(cmd->cmd_str, "done") == 0)
	{
	    context->state = SMPD_CLOSING;
	    result = SMPDU_Sock_post_close(context->sock);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to post a close on sock %d,\nsock error: %s\n",
		    SMPDU_Sock_get_sock_id(context->sock), get_sock_error_string(result));
		smpd_exit_fn(FCNAME);
		return SMPD_FAIL;
	    }
	    smpd_exit_fn(FCNAME);
	    return SMPD_CLOSE;
	}
	else
	{
	    smpd_err_printf("returning error for unknown smpd user session command: \"%s\"\n", cmd->cmd);
	    result = smpd_fail_unexpected_command(context);
	    smpd_exit_fn(FCNAME);
	    return result;
	}
    }

    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}
