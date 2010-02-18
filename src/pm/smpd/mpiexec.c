/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include "mpiexec.h"
#include "smpd.h"
#include "mpi.h"
/*
#include "smpd_implthread.h"
*/

#ifdef HAVE_WINDOWS_H
void timeout_thread(void *p)
{
    SMPDU_Size_t num_written;
    char ch = -1;

    SMPD_UNREFERENCED_ARG(p);

    Sleep(smpd_process.timeout * 1000);
    smpd_err_printf("\nmpiexec terminated job due to %d second timeout.\n", smpd_process.timeout);
    if (smpd_process.timeout_sock != SMPDU_SOCK_INVALID_SOCK)
    {
	SMPDU_Sock_write(smpd_process.timeout_sock, &ch, 1, &num_written);
	Sleep(30000); /* give the engine 30 seconds to shutdown and then force an exit */
    }
    ExitProcess((UINT)-1);
}
#else
#ifdef SIGALRM
void timeout_function(int signo)
{
    SMPDU_Size_t num_written;
    char ch = -1;
    static int alarmed = 0;
    if (signo == SIGALRM)
    {
	if (smpd_process.timeout_sock != SMPDU_SOCK_INVALID_SOCK && alarmed == 0)
	{
	    alarmed = 1;
	    smpd_err_printf("\nmpiexec terminated job due to %d second timeout.\n", smpd_process.timeout);
	    SMPDU_Sock_write(smpd_process.timeout_sock, &ch, 1, &num_written);
	    alarm(30); /* give the engine 30 seconds to shutdown and then force an exit */
	}
	else
	{
	    if (alarmed == 0)
	    {
		smpd_err_printf("\nmpiexec terminated job due to %d second timeout.\n", smpd_process.timeout);
	    }
	    exit(-1);
	}
    }
}
#else
#ifdef HAVE_PTHREAD_H
void *timeout_thread(void *p)
{
    SMPDU_Size_t num_written;
    char ch = -1;
    sleep(smpd_process.timeout);
    smpd_err_printf("\nmpiexec terminated job due to %d second timeout.\n", smpd_process.timeout);
    if (smpd_process.timeout_sock != SMPDU_SOCK_INVALID_SOCK)
    {
	SMPDU_Sock_write(smpd_process.timeout_sock, &ch, 1, &num_written);
	sleep(30); /* give the engine 30 seconds to shutdown and then force an exit */
    }
    exit(-1);
}
#endif
#endif
#endif

#ifdef HAVE_WINDOWS_H
BOOL WINAPI mpiexec_ctrl_handler(DWORD dwCtrlType)
{
    static int first = 1;
    char ch = -1;
    SMPDU_Size_t num_written;

    /* This handler could be modified to send the event to the remote processes instead of killing the job. */
    /* Doing so would require new command types for smpd */

    switch (dwCtrlType)
    {
    case CTRL_LOGOFF_EVENT:
	/* The logoff event could be a problem if two users are logged on to the same machine remotely.
	 * If user A runs mpiexec and user B logs out of his session, this event will cause user A's
	 * mpiexec command to kill the job and make user A mad.  But if we ignore the logoff event then 
	 * when user A logs out mpiexec will hang?
	 */
	break;
    case CTRL_C_EVENT:
    case CTRL_BREAK_EVENT:
    case CTRL_CLOSE_EVENT:
    case CTRL_SHUTDOWN_EVENT:
	if (smpd_process.mpiexec_abort_sock != SMPDU_SOCK_INVALID_SOCK)
	{
	    if (first == 1)
	    {
		/* The first break signal tries to abort the job with an abort message */
		first = 0;
		printf("mpiexec aborting job...\n");fflush(stdout);
		num_written = 0;
		SMPDU_Sock_write(smpd_process.mpiexec_abort_sock, &ch, 1, &num_written);
		if (num_written == 1)
		    return TRUE;
	    }
	    /* The second break signal unconditionally exits mpiexec */
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
	smpd_exit(-1);
	return TRUE;
    }
    return FALSE;
}
#endif

int main(int argc, char* argv[])
{
    int result = SMPD_SUCCESS;
    smpd_host_node_t *host_node_ptr;
    smpd_launch_node_t *launch_node_ptr;
    smpd_context_t *context;
    SMPDU_Sock_set_t set;
    SMPDU_Sock_t sock = SMPDU_SOCK_INVALID_SOCK;
    smpd_state_t state;

    smpd_enter_fn("main");

    /* catch an empty command line */
    if (argc < 2)
    {
	mp_print_options();
	exit(0);
    }

    smpd_process.mpiexec_argv0 = argv[0];

    /* initialize */
    /* FIXME: Get rid of this hack - we already create 
     * local KVS for all singleton clients by default
     */
    putenv("PMI_SMPD_FD=0");
    result = PMPI_Init(&argc, &argv);
    /* SMPD_CS_ENTER(); */
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("SMPD_Init failed,\nerror: %d\n", result);
	smpd_exit_fn("main");
	return result;
    }

    result = SMPDU_Sock_init();
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("SMPDU_Sock_init failed,\nsock error: %s\n",
		      get_sock_error_string(result));
	smpd_exit_fn("main");
	return result;
    }

    result = smpd_init_process();
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("smpd_init_process failed.\n");
	goto quit_job;
    }

    smpd_process.dbg_state = SMPD_DBG_STATE_ERROUT;

    /* parse the command line */
    smpd_dbg_printf("parsing the command line.\n");
    result = mp_parse_command_args(&argc, &argv);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("Unable to parse the mpiexec command arguments.\n");
	goto quit_job;
    }

    /* If we are using MS HPC job scheduler we only connect
     * to the local SMPD
     */
    if(smpd_process.use_ms_hpc){
        char host[100];
        int id;
        /* Free the current host list */
        result = smpd_free_host_list();
        if(result != SMPD_SUCCESS){
            smpd_err_printf("Unable to free the global host list\n");
            goto quit_job;
        }
        /* Add local host to the host list */
        result = smpd_get_hostname(host, 100);
        if(result != SMPD_SUCCESS){
            smpd_err_printf("Unable to get the local hostname\n");
            goto quit_job;
        }
	    result = smpd_get_host_id(host, &id);
        if(result != SMPD_SUCCESS){
            smpd_err_printf("Unable to get host id for local host\n");
            goto quit_job;
        }
        /* Set the number of PMI procs since they are not launched by mpiexec */
        smpd_process.nproc = smpd_process.launch_list->nproc;
        smpd_dbg_printf("Adding (%s:%d) == (localhost) to the host list\n", host, id);
    }

    /* print and see what we've got */
    /* debugging output *************/
    smpd_dbg_printf("host tree:\n");
    host_node_ptr = smpd_process.host_list;
    if (!host_node_ptr)
	smpd_dbg_printf("<none>\n");
    while (host_node_ptr)
    {
	smpd_dbg_printf(" host: %s, parent: %d, id: %d\n",
	    host_node_ptr->host,
	    host_node_ptr->parent, host_node_ptr->id);
	host_node_ptr = host_node_ptr->next;
    }
    smpd_dbg_printf("launch nodes:\n");
    launch_node_ptr = smpd_process.launch_list;
    if (!launch_node_ptr)
	smpd_dbg_printf("<none>\n");
    while (launch_node_ptr)
    {
	smpd_dbg_printf(" iproc: %d, id: %d, exe: %s\n",
	    launch_node_ptr->iproc, launch_node_ptr->host_id,
	    launch_node_ptr->exe);
	launch_node_ptr = launch_node_ptr->next;
    }
    /* end debug output *************/

    /* set the id of the mpiexec node to zero */
    smpd_process.id = 0;

    result = SMPDU_Sock_create_set(&set);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("SMPDU_Sock_create_set failed,\nsock error: %s\n", get_sock_error_string(result));
	goto quit_job;
    }
    smpd_process.set = set;

    /* Check to see if the user wants to use a remote shell mechanism for launching the processes
     * instead of using the smpd process managers.
     */
    if (smpd_process.rsh_mpiexec == SMPD_TRUE)
    {
	/* Do rsh or localonly stuff */
	result = mpiexec_rsh();

	/* skip over the non-rsh code and go to the cleanup section */
	goto quit_job;
    }

    /* Start the timeout mechanism if specified */
    /* This code occurs after the rsh_mpiexec option check because the rsh code implementes timeouts differently */
    if (smpd_process.timeout > 0)
    {
#ifdef HAVE_WINDOWS_H
	/* create a Windows thread to sleep until the timeout expires */
	if (smpd_process.timeout_thread == NULL)
	{
	    smpd_process.timeout_thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)timeout_thread, NULL, 0, NULL);
	    if (smpd_process.timeout_thread == NULL)
	    {
		printf("Error: unable to create a timeout thread, errno %d.\n", GetLastError());
		smpd_exit_fn("mp_parse_command_args");
		return SMPD_FAIL;
	    }
	}
#elif defined(SIGALRM)
	/* create an alarm to signal mpiexec when the timeout expires */
	smpd_signal(SIGALRM, timeout_function);
	alarm(smpd_process.timeout);
#elif defined(HAVE_PTHREAD_H)
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
    }

    /* make sure we have a passphrase to authenticate connections to the smpds */
    if (smpd_process.passphrase[0] == '\0')
	smpd_get_smpd_data("phrase", smpd_process.passphrase, SMPD_PASSPHRASE_MAX_LENGTH);
    if (smpd_process.passphrase[0] == '\0')
    {
	if (smpd_process.noprompt)
	{
	    printf("Error: No smpd passphrase specified through the registry or .smpd file, exiting.\n");
	    result = SMPD_FAIL;
	    goto quit_job;
	}
	printf("Please specify an authentication passphrase for smpd: ");
	fflush(stdout);
	smpd_get_password(smpd_process.passphrase);
    }

    /* set the state to create a console session or a job session */
    state = smpd_process.do_console ? SMPD_MPIEXEC_CONNECTING_SMPD : SMPD_MPIEXEC_CONNECTING_TREE;

    result = smpd_create_context(SMPD_CONTEXT_LEFT_CHILD, set, sock, 1, &context);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to create a context for the first host in the tree.\n");
	goto quit_job;
    }
#ifdef HAVE_WINDOWS_H
    if (!smpd_process.local_root)
    {
#endif
	/* start connecting the tree by posting a connect to the first host */
	result = SMPDU_Sock_post_connect(set, context, smpd_process.host_list->host, smpd_process.port, &sock);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("Unable to connect to '%s:%d',\nsock error: %s\n",
		smpd_process.host_list->host, smpd_process.port, get_sock_error_string(result));
	    goto quit_job;
	}
#ifdef HAVE_WINDOWS_H
    }
#endif
    context->sock = sock;
    context->state = state;
    context->connect_to = smpd_process.host_list;
#ifdef HAVE_WINDOWS_H
    if (smpd_process.local_root)
    {
	int port;
	smpd_context_t *rc_context;

	/* The local_root option is implemented by having mpiexec act as the smpd
	 * and launch the smpd manager.  Then mpiexec connects to this manager just
	 * as if it had been created by a real smpd.  This causes all the processes
	 * destined for the first smpd host to be launched by this child process of
	 * mpiexec and not the smpd service.  This allows for these processes to
	 * create windows that are visible to the interactive user.  It also means 
	 * that the job cannot be run in the context of a user other than the user
	 * running mpiexec. */

	/* get the path to smpd.exe because pszExe is currently mpiexec.exe */
	smpd_get_smpd_data("binary", smpd_process.pszExe, SMPD_MAX_EXE_LENGTH);

	/* launch the manager process */
	result = smpd_start_win_mgr(context, SMPD_FALSE);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to start the local smpd manager.\n");
	    goto quit_job;
	}

	/* connect to the manager */
	smpd_dbg_printf("connecting a new socket.\n");
	port = atol(context->port_str);
	if (port < 1)
	{
	    smpd_err_printf("Invalid reconnect port read: %d\n", port);
	    goto quit_job;
	}
	result = smpd_create_context(context->type, context->set, SMPDU_SOCK_INVALID_SOCK, context->id, &rc_context);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to create a new context for the reconnection.\n");
	    goto quit_job;
	}
	rc_context->state = context->state;
	rc_context->write_state = SMPD_RECONNECTING;
	context->state = SMPD_CLOSING;
	rc_context->connect_to = context->connect_to;
	rc_context->connect_return_id = context->connect_return_id;
	rc_context->connect_return_tag = context->connect_return_tag;
	strcpy(rc_context->host, context->host);
	smpd_process.left_context = rc_context;
	smpd_dbg_printf("posting a re-connect to %s:%d in %s context.\n", rc_context->connect_to->host, port, smpd_get_context_str(rc_context));
	result = SMPDU_Sock_post_connect(rc_context->set, rc_context, rc_context->connect_to->host, port, &rc_context->sock);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("Unable to post a connect to '%s:%d',\nsock error: %s\n",
		rc_context->connect_to->host, port, get_sock_error_string(result));
	    if (smpd_post_abort_command("Unable to connect to '%s:%d',\nsock error: %s\n",
		rc_context->connect_to->host, port, get_sock_error_string(result)) != SMPD_SUCCESS)
	    {
		goto quit_job;
	    }
	}
    }
    else
    {
#endif
	smpd_process.left_context = context;
	result = SMPDU_Sock_set_user_ptr(sock, context);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to set the smpd sock user pointer,\nsock error: %s\n",
		get_sock_error_string(result));
	    goto quit_job;
	}
#ifdef HAVE_WINDOWS_H
    }
#endif

#ifdef HAVE_WINDOWS_H
    {
	/* Create a break handler and a socket to handle aborting the job when mpiexec receives break signals */
	smpd_context_t *reader_context;
	SMPDU_Sock_t sock_reader;
	SMPDU_SOCK_NATIVE_FD reader, writer;

	smpd_make_socket_loop((SOCKET*)&reader, (SOCKET*)&writer);
	result = SMPDU_Sock_native_to_sock(set, reader, NULL, &sock_reader);
	result = SMPDU_Sock_native_to_sock(set, writer, NULL, &smpd_process.mpiexec_abort_sock);
	result = smpd_create_context(SMPD_CONTEXT_MPIEXEC_ABORT, set, sock_reader, -1, &reader_context);
	reader_context->read_state = SMPD_READING_MPIEXEC_ABORT;
	result = SMPDU_Sock_post_read(sock_reader, &reader_context->read_cmd.cmd, 1, 1, NULL);

	if (!SetConsoleCtrlHandler(mpiexec_ctrl_handler, TRUE))
	{
	    /* Don't error out; allow the job to run without a ctrl handler? */
	    result = GetLastError();
	    smpd_dbg_printf("unable to set a ctrl handler for mpiexec, error %d\n", result);
	}
    }
#endif

    result = smpd_enter_at_state(set, state);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("state machine failed.\n");
	goto quit_job;
    }

quit_job:

    if ((result != SMPD_SUCCESS) && (smpd_process.mpiexec_exit_code == 0))
    {
	smpd_process.mpiexec_exit_code = -1;
    }

    /* finalize */

    smpd_dbg_printf("calling SMPDU_Sock_finalize\n");
    result = SMPDU_Sock_finalize();
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("SMPDU_Sock_finalize failed,\nsock error: %s\n", get_sock_error_string(result));
    }

    /* SMPD_Finalize called in smpd_exit()
    smpd_dbg_printf("calling SMPD_Finalize\n");
    result = PMPI_Finalize();
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("SMPD_Finalize failed,\nerror: %d\n", result);
    }
    */

#ifdef HAVE_WINDOWS_H
    if (smpd_process.hCloseStdinThreadEvent)
	SetEvent(smpd_process.hCloseStdinThreadEvent);
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
    if (smpd_process.hCloseStdinThreadEvent)
    {
	CloseHandle(smpd_process.hCloseStdinThreadEvent);
	smpd_process.hCloseStdinThreadEvent = NULL;
    }
#elif defined(USE_PTHREAD_STDIN_REDIRECTION)
    smpd_cancel_stdin_thread();
#endif
    smpd_exit_fn("main");
    /* SMPD_CS_EXIT(); */
    return smpd_exit(smpd_process.mpiexec_exit_code);
}

