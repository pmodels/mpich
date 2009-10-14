/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <stdlib.h>
#include "smpd.h"
#include "mpi.h"
/*
#include "smpd_implthread.h"
*/
#ifdef HAVE_WINDOWS_H
#include "smpd_service.h"
#endif
#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#undef FCNAME
#define FCNAME "main"
int main(int argc, char* argv[])
{
    int result;
#ifdef HAVE_WINDOWS_H
    SERVICE_TABLE_ENTRY dispatchTable[] =
    {
        { TEXT(SMPD_SERVICE_NAME), (LPSERVICE_MAIN_FUNCTION)smpd_service_main },
        { NULL, NULL }
    };
#else
    char smpd_filename[SMPD_MAX_FILENAME] = "";
    char response[100] = "no";
    char *homedir;
#endif

    smpd_enter_fn(FCNAME);

    /* FIXME: Get rid of this hack - we already create 
     * local KVS for all singleton clients by default
     */
    /* initialization */
    putenv("PMI_SMPD_FD=0");
    
    result = PMPI_Init(&argc, &argv);
    
    /* SMPD_CS_ENTER(); */
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("SMPD_Init failed,\n error: %d\n", result);
	smpd_exit_fn(FCNAME);
	return result;
    }


    result = SMPDU_Sock_init();
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("SMPDU_Sock_init failed,\nsock error: %s\n", get_sock_error_string(result));
	smpd_exit_fn(FCNAME);
	return result;
    }

    result = smpd_init_process();
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("smpd_init_process failed.\n");
	smpd_exit_fn(FCNAME);
	return result;
    }

    /* parse the command line */
    result = smpd_parse_command_args(&argc, &argv);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("Unable to parse the command arguments.\n");
	smpd_exit_fn(FCNAME);
	return result;
    }

#ifdef HAVE_WINDOWS_H
    if (smpd_process.bService)
    {
	/*
	printf( "\nStartServiceCtrlDispatcher being called.\n" );
	printf( "This may take several seconds.  Please wait.\n" );
	fflush(stdout);
	*/

	/* If StartServiceCtrlDispatcher returns true the service has exited */
	result = StartServiceCtrlDispatcher(dispatchTable);
	if (result)
	{
	    smpd_exit_fn(FCNAME);
	    smpd_exit(0);
	}

	result = GetLastError();
	if (result != ERROR_FAILED_SERVICE_CONTROLLER_CONNECT)
	{
	    smpd_add_error_to_message_log(TEXT("StartServiceCtrlDispatcher failed."));
	    smpd_exit_fn(FCNAME);
	    smpd_exit(0);
	}
	smpd_print_options();
	smpd_exit_fn(FCNAME);
	smpd_exit(0);
    }
    /*
    printf("\nRunning smpd from the console, not as a service.\n");
    fflush(stdout);
    smpd_process.bService = SMPD_FALSE;
    */
#endif

    if (smpd_process.passphrase[0] == '\0')
	smpd_get_smpd_data("phrase", smpd_process.passphrase, SMPD_PASSPHRASE_MAX_LENGTH);
    if (smpd_process.passphrase[0] == '\0')
    {
	if (smpd_process.noprompt)
	{
	    printf("Error: No smpd passphrase specified through the registry or .smpd file, exiting.\n");
	    smpd_exit_fn(FCNAME);
	    return -1;
	}
	printf("Please specify an authentication passphrase for this smpd: ");
	fflush(stdout);
	smpd_get_password(smpd_process.passphrase);
#ifndef HAVE_WINDOWS_H
	homedir = getenv("HOME");
	if(homedir != NULL){
	    strcpy(smpd_filename, homedir);
	    if (smpd_filename[strlen(smpd_filename)-1] != '/')
	        strcat(smpd_filename, "/.smpd");
	    else
	        strcat(smpd_filename, ".smpd");
	}else{
	    strcpy(smpd_filename, ".smpd");
	}
	printf("Would you like to save this passphrase in '%s'? ", smpd_filename);
	fflush(stdout);
	fgets(response, 100, stdin);
	if (smpd_is_affirmative(response))
	{
	    FILE *fout;
	    umask(0077);
	    fout = fopen(smpd_filename, "w");
	    if (fout == NULL)
	    {
		printf("Error: unable to open '%s', errno = %d\n", smpd_filename, errno);
		smpd_exit_fn(FCNAME);
		return errno;
	    }
	    fprintf(fout, "phrase=%s\n", smpd_process.passphrase);
	    fclose(fout);
	}
#endif
    }

    result = smpd_entry_point();

    smpd_finalize_printf();

    smpd_dbg_printf("calling SMPDU_Sock_finalize\n");
    result = SMPDU_Sock_finalize();
    if (result != SMPD_SUCCESS){
	    smpd_err_printf("SMPDU_Sock_finalize failed,\nsock error: %s\n", get_sock_error_string(result));
    }

    /* SMPD_CS_EXIT(); */
    smpd_exit(result);
    smpd_exit_fn(FCNAME);
    return result;
}

#undef FCNAME
#define FCNAME "smpd_entry_point"
int smpd_entry_point()
{
    int result;
    SMPDU_Sock_set_t set;
    SMPDU_Sock_t listener;
    SMPD_BOOL print_port = SMPD_FALSE;

    /* This function is called by main or by smpd_service_main in the case of a Windows service */

    smpd_enter_fn(FCNAME);

#ifdef HAVE_WINDOWS_H
    /* prevent the os from bringing up debug message boxes if this process crashes */
    if (smpd_process.bService)
    {
    char port[SMPD_MAX_PORT_STR_LENGTH];
	SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);
	if (!smpd_report_status_to_sc_mgr(SERVICE_RUNNING, NO_ERROR, 0))
	{
	    result = GetLastError();
	    smpd_err_printf("Unable to report that the service has started, error: %d\n", result);
	    smpd_exit_fn(FCNAME);
	    return result;
	}
	smpd_clear_process_registry();
    smpd_get_smpd_data("phrase", smpd_process.passphrase, SMPD_PASSPHRASE_MAX_LENGTH);
    smpd_get_smpd_data("port", port, SMPD_MAX_PORT_STR_LENGTH);
    smpd_process.port = atoi(port);
    }
#endif

    /* This process is the root_smpd.  All sessions are child processes of this process. */
    smpd_process.id = 0;
    smpd_process.root_smpd = SMPD_TRUE;

    if (smpd_process.pszExe[0] != '\0')
    {
	smpd_set_smpd_data("binary", smpd_process.pszExe);
    }

    result = SMPDU_Sock_create_set(&set);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("SMPDU_Sock_create_set failed,\nsock error: %s\n", get_sock_error_string(result));
	smpd_exit_fn(FCNAME);
	return result;
    }
    smpd_process.set = set;
    smpd_dbg_printf("created a set for the listener: %d\n", SMPDU_Sock_get_sock_set_id(set));
    if (smpd_process.port == 0)
	print_port = SMPD_TRUE;
    result = SMPDU_Sock_listen(set, NULL, &smpd_process.port, &listener); 
    if (result != SMPD_SUCCESS)
    {
	/* If another smpd is running and listening on this port, tell it to shutdown or restart? */
	smpd_err_printf("SMPDU_Sock_listen failed,\nsock error: %s\n", get_sock_error_string(result));
	smpd_exit_fn(FCNAME);
	return result;
    }
    smpd_dbg_printf("smpd listening on port %d\n", smpd_process.port);
    if (print_port)
    {
	printf("%d\n", smpd_process.port);
	fflush(stdout);
    }

    result = smpd_create_context(SMPD_CONTEXT_LISTENER, set, listener, -1, &smpd_process.listener_context);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to create a context for the smpd listener.\n");
	smpd_exit_fn(FCNAME);
	return result;
    }
    result = SMPDU_Sock_set_user_ptr(listener, smpd_process.listener_context);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("SMPDU_Sock_set_user_ptr failed,\nsock error: %s\n", get_sock_error_string(result));
	smpd_exit_fn(FCNAME);
	return result;
    }
    smpd_process.listener_context->state = SMPD_SMPD_LISTENING;

    if (smpd_process.root_smpd){
		if(smpd_option_on("no_dynamic_hosts") == SMPD_FALSE){
			smpd_insert_into_dynamic_hosts();
		}
	}

#ifndef HAVE_WINDOWS_H
    /* put myself in the background if flag is set */
    if (smpd_process.bNoTTY)
    {
	int fd, maxfd;

        if (fork() != 0)
	{
	    /* parent exits */
	    exit(0);
	}
	/* become session leader; no controlling tty */
	setsid();
	/* make sure no sighup when leader ends */
	smpd_signal(SIGHUP, SIG_IGN);
	/* leader exits; svr4: make sure do not get another controlling tty */
        if (fork() != 0)
	    exit(0);

	/* redirect stdout/err to nothing */
	fd = open("/dev/null", O_APPEND);
	if (fd != -1)
	{
	    close(1);
	    close(2);
	    dup2(fd, 1);
	    dup2(fd, 2);
	    close(fd);
	}

	/* the stdin file descriptor 0 needs to be occupied so it doesn't get used by socketpair */
	/*close(0);*/
	/* maybe 0 should be redirected to "/dev/null" just like 1 and 2? */
	fd = open("/dev/null", O_RDONLY);
	if (fd != -1)
	{
	    close(0);
	    dup2(fd, 0);
	    close(fd);
	}

	/* get out of the current directory to get out of the way of possibly mounted directories */
	chdir("/");

	/* close all open file descriptors */
	/* We don't want to close the listener sock.  This code should be moved to the very beginning of main */
	/*
	maxfd = sysconf(_SC_OPEN_MAX);
	if (maxfd < 0)
	    maxfd = 255;
	for (i=3; i<maxfd; i++)
	    close(i);
	*/
    }
#endif

    result = smpd_enter_at_state(set, SMPD_SMPD_LISTENING);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("state machine failed.\n");
    }

    if (smpd_process.root_smpd)
	smpd_remove_from_dynamic_hosts();

    result = SMPDU_Sock_destroy_set(set);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to destroy the set, error:\n%s\n",
	    get_sock_error_string(result));
    }

    /*
    result = SMPDU_Sock_finalize();
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("SMPDU_Sock_finalize failed,\nsock error: %s\n", get_sock_error_string(result));
    }
    */

    smpd_exit_fn(FCNAME);
    return 0;
}

