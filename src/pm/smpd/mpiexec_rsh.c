/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include "mpiexec.h"
#include "smpd.h"

#define MAX_RSH_CMD_LENGTH 8192
#define MAX_OUTPUT_LENGTH 4096
#define MAX_INPUT_LENGTH 4096

static char root_host[100];
static int root_port;

#undef FCNAME
#define FCNAME "FmtEnvVarsForSSH"
/* This function formats the environment strings of the form,
 *    foo1=bar1 foo2=bar2 foo3=bar3
 *    TO
 *     "foo1=bar1" "foo2=bar2" "foo3=bar3"
 *    and returns the length of the formatted string
 *    Note: The formatted string contains a space in the front
 */
static int FmtEnvVarsForSSH(char *bEnv, char *fmtEnv, int fmtEnvLen)
{
    char name[SMPD_MAX_ENV_LENGTH], equals[3], value[SMPD_MAX_ENV_LENGTH];
    int curLen = 0, totLen = 0;

    smpd_enter_fn(FCNAME);
    if(fmtEnv && bEnv){
     for (;;)
     {
	 name[0] = '\0';
	 equals[0] = '\0';
	 value[0] = '\0';
	 if(fmtEnvLen <= 0)
	     break;
	 if (MPIU_Str_get_string(&bEnv, name, SMPD_MAX_ENV_LENGTH) != MPIU_STR_SUCCESS)
	     break;
	 if (name[0] == '\0')
	     break;
	 if (MPIU_Str_get_string(&bEnv, equals, 3) != MPIU_STR_SUCCESS)
	     break;
	 if (equals[0] == '\0')
	     break;
	 if (MPIU_Str_get_string(&bEnv, value, SMPD_MAX_ENV_LENGTH) != MPIU_STR_SUCCESS)
	     break;
	 MPIU_Snprintf(fmtEnv, fmtEnvLen, " \"%s=%s\"", name, value);
	 curLen = strlen(fmtEnv);
	 totLen += curLen;
	 fmtEnv += curLen;
	 fmtEnvLen -= curLen;
     }
    }
    smpd_exit_fn(FCNAME);
    return totLen;
}

static int setup_stdin_redirection(smpd_process_t *process, MPIDU_Sock_set_t set)
{
    int result;
    smpd_context_t *context_in;
    MPIDU_SOCK_NATIVE_FD stdin_fd;
    MPIDU_Sock_t insock;
#ifdef HAVE_WINDOWS_H
    DWORD dwThreadID;
    SOCKET hWrite;
#endif

    smpd_enter_fn("setup_stdin_redirection");

    /* get a handle to stdin */
#ifdef HAVE_WINDOWS_H
    result = smpd_make_socket_loop((SOCKET*)&stdin_fd, &hWrite);
    if (result)
    {
	smpd_err_printf("Unable to make a local socket loop to forward stdin.\n");
	smpd_exit_fn("setup_stdin_redirection");
	return SMPD_FAIL;
    }
#else
    stdin_fd = fileno(stdin);
#endif

    /* convert the native handle to a sock */
    result = MPIDU_Sock_native_to_sock(set, stdin_fd, NULL, &insock);
    if (result != MPI_SUCCESS)
    {
	smpd_err_printf("unable to create a sock from stdin,\nsock error: %s\n", get_sock_error_string(result));
	smpd_exit_fn("setup_stdin_redirection");
	return SMPD_FAIL;
    }
    /* create a context for reading from stdin */
    result = smpd_create_context(SMPD_CONTEXT_MPIEXEC_STDIN_RSH, set, insock, -1, &context_in);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to create a context for stdin.\n");
	smpd_exit_fn("setup_stdin_redirection");
	return SMPD_FAIL;
    }
    MPIDU_Sock_set_user_ptr(insock, context_in);
    context_in->process = process;

#ifdef HAVE_WINDOWS_H
    /* unfortunately, we cannot use stdin directly as a sock.  So, use a thread to read and forward
    stdin to a sock */
    smpd_process.hCloseStdinThreadEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (smpd_process.hCloseStdinThreadEvent == NULL)
    {
	smpd_err_printf("Unable to create the stdin thread close event, error %d\n", GetLastError());
	smpd_exit_fn("setup_stdin_redirection");
	return SMPD_FAIL;
    }
    smpd_process.hStdinThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)smpd_stdin_thread, (void*)hWrite, 0, &dwThreadID);
    if (smpd_process.hStdinThread == NULL)
    {
	smpd_err_printf("Unable to create a thread to read stdin, error %d\n", GetLastError());
	smpd_exit_fn("setup_stdin_redirection");
	return SMPD_FAIL;
    }
#endif
    /* set this variable first before posting the first read to avoid a race condition? */
    smpd_process.stdin_redirecting = SMPD_TRUE;
    /* post a read for a user command from stdin */
    context_in->read_state = SMPD_READING_STDIN;
    result = MPIDU_Sock_post_read(insock, context_in->read_cmd.cmd, 1, 1, NULL);
    if (result != MPI_SUCCESS)
    {
	smpd_err_printf("unable to post a read on stdin for an incoming user command, error:\n%s\n",
	    get_sock_error_string(result));
	smpd_exit_fn("setup_stdin_redirection");
	return SMPD_FAIL;
    }
    smpd_exit_fn("setup_stdin_redirection");
    return SMPD_SUCCESS;
}

#ifdef HAVE_WINDOWS_H
BOOL WINAPI mpiexec_rsh_handler(DWORD type)
{
    static int first = 1;
    char ch = -1;
    MPIU_Size_t num_written;

    switch (type)
    {
    case CTRL_LOGOFF_EVENT:
	break;
    case CTRL_C_EVENT:
    case CTRL_BREAK_EVENT:
    case CTRL_CLOSE_EVENT:
    case CTRL_SHUTDOWN_EVENT:
	if (smpd_process.timeout_sock != MPIDU_SOCK_INVALID_SOCK)
	{
	    if (first == 1)
	    {
		first = 0;
		MPIDU_Sock_write(smpd_process.timeout_sock, &ch, 1, &num_written);
		return TRUE;
	    }
	}
	if (smpd_process.hCloseStdinThreadEvent)
	{
	    SetEvent(smpd_process.hCloseStdinThreadEvent);
	}
	if (smpd_process.hStdinThread != NULL)
	{
	    /* close stdin so the input thread will exit */
	    CloseHandle(GetStdHandle(STD_INPUT_HANDLE));
	    if (WaitForSingleObject(smpd_process.hStdinThread, 3000) != WAIT_OBJECT_0)
	    {
		TerminateThread(smpd_process.hStdinThread, 321);
	    }
	    CloseHandle(smpd_process.hStdinThread);
	}
	/*
	if (smpd_process.hCloseStdinThreadEvent)
	{
	    CloseHandle(smpd_process.hCloseStdinThreadEvent);
	    smpd_process.hCloseStdinThreadEvent = NULL;
	}
	*/
	smpd_exit(-1);
	return TRUE;
    }
    return FALSE;
}
#endif

static int ConnectToHost(char *host, int port, smpd_state_t state, MPIDU_Sock_set_t set, MPIDU_Sock_t *sockp, smpd_context_t **contextpp)
{
    int result;
    char error_msg[MPI_MAX_ERROR_STRING];
    int len;

    /*printf("posting a connect to %s:%d\n", host, port);fflush(stdout);*/
    result = smpd_create_context(SMPD_CONTEXT_PMI, set, MPIDU_SOCK_INVALID_SOCK/**sockp*/, -1, contextpp);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("ConnectToHost failed: unable to create a context to connect to %s:%d with.\n", host, port);
	return SMPD_FAIL;
    }

    result = MPIDU_Sock_post_connect(set, *contextpp, host, port, sockp);
    if (result != MPI_SUCCESS)
    {
	len = MPI_MAX_ERROR_STRING;
	PMPI_Error_string(result, error_msg, &len);
	smpd_err_printf("ConnectToHost failed: unable to post a connect to %s:%d, error: %s\n", host, port, error_msg);
	return SMPD_FAIL;
    }

    (*contextpp)->sock = *sockp;
    (*contextpp)->state = state;

    result = smpd_enter_at_state(set, state);
    if (result != MPI_SUCCESS)
    {
	smpd_err_printf("ConnectToHost failed: unable to connect to %s:%d.\n", host, port);
	return SMPD_FAIL;
    }

    return SMPD_SUCCESS;
}

#ifdef HAVE_WINDOWS_H
#define popen _popen
#define pclose _pclose
#endif
static FILE *pmi_server = NULL;
int start_pmi_server(int nproc, char *host, int len, int *port)
{
    char cmd[100];
    char line[1024];
    char *argv0 = NULL;

    if (smpd_process.mpiexec_argv0 != NULL)
    {
	argv0 = smpd_process.mpiexec_argv0;
    }
    sprintf(cmd, "%s -pmiserver %d", argv0, nproc);
    pmi_server = popen(cmd, "r");
    if (pmi_server == NULL)
    {
	smpd_err_printf("popen failed: %d\n", errno);
	return SMPD_FAIL;
    }
    fgets(line, 1024, pmi_server);
    strtok(line, "\r\n");
    strncpy(host, line, len);
    /*printf("read host: %s\n", host);*/
    fgets(line, 1024, pmi_server);
    *port = atoi(line);
    /*printf("read port: %d\n", *port);*/
    fgets(line, 1024, pmi_server);
    strtok(line, "\r\n");
    strncpy(smpd_process.kvs_name, line, SMPD_MAX_DBS_NAME_LEN);
    /*printf("read kvs: %s\n", smpd_process.kvs_name);*/
    return SMPD_SUCCESS;
}

int stop_pmi_server()
{
    if (pmi_server != NULL)
    {
	if (pclose(pmi_server) == -1)
	{
	    smpd_err_printf("pclose failed: %d\n", errno);
	    return SMPD_FAIL;
	}
    }
    return SMPD_SUCCESS;
}

int mpiexec_rsh()
{
    int i;
    smpd_launch_node_t *launch_node_ptr;
    smpd_process_t *process, **processes;
    int result;
    char *iter1, *iter2;
    char exe[SMPD_MAX_EXE_LENGTH];
    char *p;
    char ssh_cmd[100] = "ssh -x";
    MPIDU_Sock_set_t set;
    SMPD_BOOL escape_escape = SMPD_TRUE;
    char *env_str;
    int maxlen;
    MPIDU_Sock_t abort_sock;
    smpd_context_t *abort_context = NULL;
    smpd_command_t *cmd_ptr;

    smpd_enter_fn("mpiexec_rsh");

#ifdef HAVE_WINDOWS_H
    SetConsoleCtrlHandler(mpiexec_rsh_handler, TRUE);
#else
    /* setup a signall hander? */
#endif

    p = getenv("MPIEXEC_RSH");
    if (p != NULL && strlen(p) > 0)
    {
	strncpy(ssh_cmd, p, 100);
    }

    p = getenv("MPIEXEC_RSH_NO_ESCAPE");
    if (p != NULL)
    {
	if (smpd_is_affirmative(p) || strcmp(p, "1") == 0)
	{
	    escape_escape = SMPD_FALSE;
	}
    }

    result = MPIDU_Sock_create_set(&set);
    if (result != MPI_SUCCESS)
    {
	smpd_err_printf("unable to create a set for the mpiexec_rsh.\n");
	smpd_exit_fn("mpiexec_rsh");
	return SMPD_FAIL;
    }

    if (smpd_process.use_pmi_server)
    {
	result = start_pmi_server(smpd_process.launch_list->nproc, root_host, 100, &root_port);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("mpiexec_rsh is unable to start the local pmi server.\n");
	    smpd_exit_fn("mpiexec_rsh");
	    return SMPD_FAIL;
	}
	smpd_dbg_printf("the pmi server is listening on %s:%d\n", root_host, root_port);
    }
    else
    {
	/* start the root smpd */
	result = PMIX_Start_root_smpd(smpd_process.launch_list->nproc, root_host, 100, &root_port);
	if (result != PMI_SUCCESS)
	{
	    smpd_err_printf("mpiexec_rsh is unable to start the root smpd.\n");
	    smpd_exit_fn("mpiexec_rsh");
	    return SMPD_FAIL;
	}
	smpd_dbg_printf("the root smpd is listening on %s:%d\n", root_host, root_port);

	/* create a connection to the root smpd used to abort the job */
	result = ConnectToHost(root_host, root_port, SMPD_CONNECTING_RPMI, set, &abort_sock, &abort_context);
	if (result != SMPD_SUCCESS)
	{
	    smpd_exit_fn("mpiexec_rsh");
	    return SMPD_FAIL;
	}
    }

    processes = (smpd_process_t**)MPIU_Malloc(sizeof(smpd_process_t*) * smpd_process.launch_list->nproc);
    if (processes == NULL)
    {
	smpd_err_printf("unable to allocate process array.\n");
	smpd_exit_fn("mpiexec_rsh");
	return SMPD_FAIL;
    }
    smpd_process.nproc = smpd_process.launch_list->nproc;

    launch_node_ptr = smpd_process.launch_list;
    for (i=0; i<smpd_process.launch_list->nproc; i++)
    {
	if (launch_node_ptr == NULL)
	{
	    smpd_err_printf("Error: not enough launch nodes.\n");
	    smpd_exit_fn("mpiexec_rsh");
	    return SMPD_FAIL;
	}

	/* initialize process structure */
	result = smpd_create_process_struct(i, &process);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to create a process structure.\n");
	    smpd_exit_fn("mpiexec_rsh");
	    return SMPD_FAIL;
	}
	/* no need for a pmi context */
	if (process->pmi)
	    smpd_free_context(process->pmi);
	process->pmi = NULL;
	/* change stdout and stderr to rsh behavior: 
	 * write stdout/err directly to stdout/err instead of creating
	 * an smpd stdout/err command 
	 */
	if (process->out != NULL)
	{
	    process->out->type = SMPD_CONTEXT_STDOUT_RSH;
	}
	if (process->err != NULL)
	{
	    process->err->type = SMPD_CONTEXT_STDERR_RSH;
	}
	MPIU_Strncpy(process->clique, launch_node_ptr->clique, SMPD_MAX_CLIQUE_LENGTH);
	MPIU_Strncpy(process->dir, launch_node_ptr->dir, SMPD_MAX_DIR_LENGTH);
	MPIU_Strncpy(process->domain_name, smpd_process.kvs_name, SMPD_MAX_DBS_NAME_LEN);
	MPIU_Strncpy(process->env, launch_node_ptr->env, SMPD_MAX_ENV_LENGTH);
	if (escape_escape == SMPD_TRUE && smpd_process.mpiexec_run_local != SMPD_TRUE)
	{
	    /* convert \ to \\ to make cygwin ssh happy */
	    iter1 = launch_node_ptr->exe;
	    iter2 = exe;
	    while (*iter1)
	    {
		if (*iter1 == '\\')
		{
		    *iter2 = *iter1;
		    iter2++;
		    *iter2 = *iter1;
		}
		else
		{
		    *iter2 = *iter1;
		}
		iter1++;
		iter2++;
	    }
	    *iter2 = '\0';
	    /*printf("[%s] -> [%s]\n", launch_node_ptr->exe, exe);*/
	}
	else
	{
	    MPIU_Strncpy(exe, launch_node_ptr->exe, SMPD_MAX_EXE_LENGTH);
	}

	/* Two samples for testing on the local machine */

	/* static rPMI initialization */
	/*sprintf(process->exe, "env PMI_RANK=%d PMI_SIZE=%d PMI_KVS=%s PMI_ROOT_HOST=%s PMI_ROOT_PORT=8888 PMI_ROOT_LOCAL=1 PMI_APPNUM=%d %s",
	    launch_node_ptr->iproc, launch_node_ptr->nproc, smpd_process.kvs_name, root_host, launch_node_ptr->appnum, exe);*/

	/* dynamic rPMI initialization */
	/*sprintf(process->exe, "env PMI_RANK=%d PMI_SIZE=%d PMI_KVS=%s PMI_ROOT_HOST=%s PMI_ROOT_PORT=%d PMI_ROOT_LOCAL=0 PMI_APPNUM=%d %s",
	    launch_node_ptr->iproc, launch_node_ptr->nproc, smpd_process.kvs_name, root_host, root_port, launch_node_ptr->appnum, exe);*/

	if (smpd_process.mpiexec_run_local == SMPD_TRUE)
	{
	    /* -localonly option and dynamic rPMI initialization */
	    env_str = &process->env[strlen(process->env)];
	    maxlen = (int)(SMPD_MAX_ENV_LENGTH - strlen(process->env));
	    MPIU_Str_add_int_arg(&env_str, &maxlen, "PMI_RANK", launch_node_ptr->iproc);
	    MPIU_Str_add_int_arg(&env_str, &maxlen, "PMI_SIZE", launch_node_ptr->nproc);
	    MPIU_Str_add_string_arg(&env_str, &maxlen, "PMI_KVS", smpd_process.kvs_name);
	    MPIU_Str_add_string_arg(&env_str, &maxlen, "PMI_ROOT_HOST", root_host);
	    MPIU_Str_add_int_arg(&env_str, &maxlen, "PMI_ROOT_PORT", root_port);
	    MPIU_Str_add_string_arg(&env_str, &maxlen, "PMI_ROOT_LOCAL", "0");
	    MPIU_Str_add_int_arg(&env_str, &maxlen, "PMI_APPNUM", launch_node_ptr->appnum);

	    MPIU_Strncpy(process->exe, exe, SMPD_MAX_EXE_LENGTH);
	}
	else
	{
	    /* ssh and dynamic rPMI initialization */
            char fmtEnv[SMPD_MAX_ENV_LENGTH];
	    int fmtEnvLen = SMPD_MAX_ENV_LENGTH;
	    char *pExe = process->exe;
	    int curLen = 0;
	    MPIU_Snprintf(pExe, SMPD_MAX_EXE_LENGTH, "%s %s env", ssh_cmd, launch_node_ptr->hostname);
	    curLen = strlen(process->exe);
	    pExe = process->exe + curLen;
            if(FmtEnvVarsForSSH(launch_node_ptr->env, fmtEnv, fmtEnvLen)){
                /* Add user specified env vars */
                MPIU_Snprintf(pExe, SMPD_MAX_EXE_LENGTH - curLen, "%s", fmtEnv);
		curLen = strlen(process->exe);
		pExe = process->exe + curLen;
	    }
	    MPIU_Snprintf(pExe, SMPD_MAX_EXE_LENGTH - curLen, " \"PMI_RANK=%d\" \"PMI_SIZE=%d\" \"PMI_KVS=%s\" \"PMI_ROOT_HOST=%s\" \"PMI_ROOT_PORT=%d\" \"PMI_ROOT_LOCAL=0\" \"PMI_APPNUM=%d\" %s",
		launch_node_ptr->iproc, launch_node_ptr->nproc, smpd_process.kvs_name, root_host, root_port, launch_node_ptr->appnum, exe);
	}

	MPIU_Strncpy(process->kvs_name, smpd_process.kvs_name, SMPD_MAX_DBS_NAME_LEN);
	process->nproc = launch_node_ptr->nproc;
	MPIU_Strncpy(process->path, launch_node_ptr->path, SMPD_MAX_PATH_LENGTH);

	/* call smpd_launch_process */
	smpd_dbg_printf("launching: %s\n", process->exe);
	result = smpd_launch_process(process, SMPD_DEFAULT_PRIORITY_CLASS, SMPD_DEFAULT_PRIORITY, SMPD_FALSE, set);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to launch process %d <%s>.\n", i, process->exe);
	    smpd_exit_fn("mpiexec_rsh");
	    return SMPD_FAIL;
	}
	/* save the new process in the list */
	process->next = smpd_process.process_list;
	smpd_process.process_list = process;
	if (i == 0)
	{
	    /* start the stdin redirection thread to the first process */
	    setup_stdin_redirection(process, set);
	}

	smpd_process.nproc_launched++;
	processes[i] = process;
	launch_node_ptr = launch_node_ptr->next;
    }
    if (launch_node_ptr != NULL)
    {
	smpd_err_printf("Error: too many launch nodes.\n");
	smpd_exit_fn("mpiexec_rsh");
	return SMPD_FAIL;
    }

    /* Start the timeout mechanism if specified */
    if (smpd_process.timeout > 0)
    {
	smpd_context_t *reader_context;
	MPIDU_Sock_t sock_reader;
	MPIDU_SOCK_NATIVE_FD reader, writer;
#ifdef HAVE_WINDOWS_H
	/*SOCKET reader, writer;*/
	smpd_make_socket_loop((SOCKET*)&reader, (SOCKET*)&writer);
#else
	/*int reader, writer;*/
	int pair[2];
	socketpair(AF_UNIX, SOCK_STREAM, 0, pair);
	reader = pair[0];
	writer = pair[1];
#endif
	result = MPIDU_Sock_native_to_sock(set, reader, NULL, &sock_reader);
	result = MPIDU_Sock_native_to_sock(set, writer, NULL, &smpd_process.timeout_sock);
	result = smpd_create_context(SMPD_CONTEXT_TIMEOUT, set, sock_reader, -1, &reader_context);
	reader_context->read_state = SMPD_READING_TIMEOUT;
	result = MPIDU_Sock_post_read(sock_reader, &reader_context->read_cmd.cmd, 1, 1, NULL);
#ifdef HAVE_WINDOWS_H
	/* create a Windows thread to sleep until the timeout expires */
	smpd_process.timeout_thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)timeout_thread, NULL, 0, NULL);
	if (smpd_process.timeout_thread == NULL)
	{
	    printf("Error: unable to create a timeout thread, errno %d.\n", GetLastError());
	    smpd_exit_fn("mp_parse_command_args");
	    return SMPD_FAIL;
	}
#else
#ifdef SIGALRM
	/* create an alarm to signal mpiexec when the timeout expires */
	smpd_signal(SIGALRM, timeout_function);
	alarm(smpd_process.timeout);
#else
#ifdef HAVE_PTHREAD_H
	/* create a pthread to sleep until the timeout expires */
	result = pthread_create(&smpd_process.timeout_thread, NULL, timeout_thread, NULL);
	if (result != 0)
	{
	    printf("Error: unable to create a timeout thread, errno %d.\n", result);
	    smpd_exit_fn("mp_parse_command_args");
	    return SMPD_FAIL;
	}
#else
	/* no timeout mechanism available */
#endif
#endif
#endif
    }

    result = smpd_enter_at_state(set, SMPD_IDLE);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("mpiexec_rsh state machine failed.\n");
	smpd_exit_fn("mpiexec_rsh");
	return SMPD_FAIL;
    }

    if (smpd_process.use_pmi_server)
    {
	result = stop_pmi_server();
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("mpiexec_rsh unable to stop the pmi server.\n");
	    smpd_exit_fn("mpiexec_rsh");
	    return SMPD_FAIL;
	}
    }
    else
    {
	/* Send an abort command to the root_smpd thread/process to insure that it exits.
	 * This only needs to be sent when there is an error or failed process of some sort
	 * but it is safe to send it in all cases.
	 */
	result = smpd_create_command("abort", 0, 0, SMPD_FALSE, &cmd_ptr);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to create an abort command.\n");
	    smpd_exit_fn("mpiexec_rsh");
	    return SMPD_FAIL;
	}
	result = smpd_post_write_command(abort_context, cmd_ptr);
	if (result != SMPD_SUCCESS)
	{
	    /* Only print this as a debug message instead of an error because the root_smpd thread/process may have already exited. */
	    smpd_dbg_printf("unable to post a write of the abort command to the %s context.\n", smpd_get_context_str(abort_context));
	    smpd_exit_fn("mpiexec_rsh");
	    return SMPD_FAIL;
	}

	result = PMIX_Stop_root_smpd();
	if (result != PMI_SUCCESS)
	{
	    smpd_err_printf("mpiexec_rsh unable to stop the root smpd.\n");
	    smpd_exit_fn("mpiexec_rsh");
	    return SMPD_FAIL;
	}
    }

    smpd_exit_fn("mpiexec_rsh");
    return 0;
}
