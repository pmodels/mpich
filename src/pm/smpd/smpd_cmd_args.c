/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "smpd.h"
#ifdef HAVE_WINDOWS_H
#include "smpd_service.h"
#endif
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef HAVE_WINDOWS_H
BOOL WINAPI smpd_ctrl_handler(DWORD dwCtrlType)
{
    switch (dwCtrlType)
    {
    case CTRL_C_EVENT:
    case CTRL_BREAK_EVENT:
    case CTRL_CLOSE_EVENT:
    case CTRL_SHUTDOWN_EVENT:
	break;
    case CTRL_LOGOFF_EVENT:
	return FALSE;
    }
    smpd_kill_all_processes();
    return TRUE;
}
#endif

#undef FCNAME
#define FCNAME "smpd_print_options"
void smpd_print_options(void)
{
    smpd_enter_fn(FCNAME);
    printf("smpd options:\n");
    printf(" -port <port> or -p <port>\n");
    printf(" -phrase <passphrase>\n");
    printf(" -getphrase\n");
    printf(" -debug or -d\n");
    printf(" -noprompt\n");
    printf(" -restart [hostname]\n");
    printf(" -shutdown [hostname]\n");
    printf(" -console [hostname]\n");
    printf(" -status [hostname]\n");
    printf(" -anyport\n");
    printf(" -hosts\n");
    printf(" -sethosts\n");
    printf(" -set <option_name> <option_value>\n");
    printf(" -get <option_name>\n");
    printf(" -query [domain]\n");
    printf(" -help\n");
    printf("unix only options:\n");
    printf(" -s\n");
    printf(" -r\n");
    printf(" -smpdfile <filename>\n");
    printf("windows only options:\n");
    printf(" -install or -regserver\n");
    printf(" -remove  or -unregserver or -uninstall\n");
    printf(" -start\n");
    printf(" -stop\n");
    printf(" -register_spn\n");
    printf(" -remove_spn\n");
    printf(" -traceon <logfilename> [<hostA> <hostB> ...]\n");
    printf(" -traceoff [<hostA> <hostB> ...]\n");
    printf("\n");
    printf("bracketed [] items are optional\n");
    printf("\n");
    printf("\"smpd -d\" will start the smpd in debug mode.\n");
    printf("\"smpd -s\" will start the smpd in daemon mode for the current unix user.\n");
    printf("\"smpd -install\" will install and start the smpd in Windows service mode.\n");
    printf(" This must be done by a user with administrator privileges and then all\n");
    printf(" users can launch processes with mpiexec.\n");
    printf("Not yet implemented:\n");
    printf("\"smpd -r\" will start the smpd in root daemon mode for unix.\n");
    fflush(stdout);
    smpd_exit_fn(FCNAME);
}

#undef FCNAME
#define FCNAME "smpd_parse_command_args"
int smpd_parse_command_args(int *argcp, char **argvp[])
{
    int result = 0;
#ifdef HAVE_WINDOWS_H
    char str[20], read_handle_str[20], write_handle_str[20];
    int port;
    SMPDU_Sock_t listener;
    SMPDU_Sock_set_t set;
    HANDLE hWrite, hRead;
    DWORD num_written, num_read;
#endif
    int dbg_flag;
    char domain[SMPD_MAX_HOST_LENGTH];
    char opt[SMPD_MAX_NAME_LENGTH];
    char opt_val[SMPD_MAX_VALUE_LENGTH];
    char filename[SMPD_MAX_FILENAME];
    int i;

    smpd_enter_fn(FCNAME);

    /* check for help option */
    if (
#ifndef HAVE_WINDOWS_H
	*argcp < 2 || /* unix: print the options if no arguments are supplied */
#endif
	smpd_get_opt(argcp, argvp, "-help") || smpd_get_opt(argcp, argvp, "-?"))
    {
	smpd_print_options();
	smpd_exit(0);
    }

    /* check for the printprocs option */
    if (smpd_get_opt(argcp, argvp, "-printprocs"))
    {
	smpd_watch_processes();
	smpd_exit(0);
    }

    if (smpd_get_opt(argcp, argvp, "-hosts"))
    {
	char first_host[SMPD_MAX_HOST_LENGTH], host[SMPD_MAX_HOST_LENGTH], alt_host[SMPD_MAX_HOST_LENGTH];

	smpd_get_default_hosts();

	result = smpd_get_next_hostname(first_host, alt_host);
	if (result != SMPD_SUCCESS)
	    smpd_exit(result);
	printf("%s\n", first_host);
	result = smpd_get_next_hostname(host, alt_host);
	if (result != SMPD_SUCCESS)
	    smpd_exit(result);
	while (strcmp(host, first_host) != 0)
	{
	    printf("%s\n", host);
	    result = smpd_get_next_hostname(host, alt_host);
	    if (result != SMPD_SUCCESS)
		smpd_exit(result);
	}
	smpd_exit(0);
    }

    if (smpd_get_opt(argcp, argvp, "-sethosts"))
    {
	char *buffer, *iter;
	int i, length;

	length = (*argcp) * SMPD_MAX_HOST_LENGTH;
	buffer = MPIU_Malloc(length);
	if (buffer == NULL)
	{
	    smpd_err_printf("unable to allocate memory to store the host names.\n");
	    smpd_exit(-1);
	}
	iter = buffer;
	for (i=1; i<*argcp; i++)
	{
	    result = MPIU_Str_add_string(&iter, &length, (*argvp)[i]);
	    if (result)
	    {
		printf("unable to add host #%d, %s\n", i, (*argvp)[i]);
		MPIU_Free(buffer);
		smpd_exit(-1);
	    }
	}
	/*printf("hosts: %s\n", buffer);*/
	result = smpd_set_smpd_data("hosts", buffer);
	if (result == SMPD_SUCCESS)
	{
	    printf("hosts data saved successfully.\n");
	}
	else
	{
	    printf("Error: unable to save the hosts data.\n");
	}
	MPIU_Free(buffer);
	smpd_exit(0);
    }

    if (smpd_get_opt_two_strings(argcp, argvp, "-set", opt, SMPD_MAX_NAME_LENGTH, opt_val, SMPD_MAX_VALUE_LENGTH))
    {
	/* The do loop allows for multiple -set operations to be specified on the command line */
	do
	{
	    if (strlen(opt) == 0)
	    {
		printf("invalid option specified.\n");
		smpd_exit(-1);
	    }
	    if (strlen(opt_val) == 0)
	    {
		result = smpd_delete_smpd_data(opt);
	    }
	    else
	    {
		result = smpd_set_smpd_data(opt, opt_val);
	    }
	    if (result == SMPD_SUCCESS)
	    {
		printf("%s = %s\n", opt, opt_val);
	    }
	    else
	    {
		printf("unable to set %s option.\n", opt);
	    }
	} while (smpd_get_opt_two_strings(argcp, argvp, "-set", opt, SMPD_MAX_NAME_LENGTH, opt_val, SMPD_MAX_VALUE_LENGTH));
	smpd_exit(0);
    }

    if (smpd_get_opt_string(argcp, argvp, "-get", opt, SMPD_MAX_NAME_LENGTH))
    {
	if (strlen(opt) == 0)
	{
	    printf("invalid option specified.\n");
	    smpd_exit(-1);
	}
	result = smpd_get_smpd_data(opt, opt_val, SMPD_MAX_VALUE_LENGTH);
	if (result == SMPD_SUCCESS)
	{
	    printf("%s\n", opt_val);
	}
	else
	{
	    printf("default\n");
	}
	smpd_exit(0);
    }

    /* If we've made it here and there still is "-set" or "-get" on the command line then the user
     * probably didn't supply the correct number of parameters.  So print the usage message
     * and exit.
     */
    if (smpd_get_opt(argcp, argvp, "-set") || smpd_get_opt(argcp, argvp, "-get"))
    {
	smpd_print_options();
	smpd_exit(-1);
    }

    if (smpd_get_opt(argcp, argvp, "-enumerate") || smpd_get_opt(argcp, argvp, "-enum"))
    {
	smpd_data_t *data;
	if (smpd_get_all_smpd_data(&data) == SMPD_SUCCESS)
	{
	    smpd_data_t *iter = data;
	    while (iter != NULL)
	    {
		printf("%s\n%s\n", iter->name, iter->value);
		iter = iter->next;
	    }
	    while (data != NULL)
	    {
		iter = data;
		data = data->next;
		MPIU_Free(iter);
	    }
	}
	smpd_exit(0);
    }

    if (smpd_get_opt_string(argcp, argvp, "-query", domain, SMPD_MAX_HOST_LENGTH))
    {
	printf("querying hosts in the %s domain:\n", domain);
	printf("Not implemented.\n");
	smpd_exit(0);
    }
    if (smpd_get_opt(argcp, argvp, "-query"))
    {
	printf("querying hosts in the default domain:\n");
	printf("Not implemented.\n");
	smpd_exit(0);
    }

    /* check for the service/silent option */
#ifdef HAVE_WINDOWS_H
    smpd_process.bService = SMPD_TRUE;
#endif

    if (smpd_get_opt(argcp, argvp, "-s"))
    {
#ifdef HAVE_WINDOWS_H
	printf("The -s option is only available under unix.\n");
	smpd_print_options();
	smpd_exit(0);
#else
	smpd_process.bNoTTY = SMPD_TRUE;
#endif
    }

    if (smpd_get_opt(argcp, argvp, "-r"))
    {
#ifdef HAVE_WINDOWS_H
	printf("The -r option is only available under unix.\n");
	smpd_print_options();
#else
	printf("The -r root option is not yet implemented.\n");
#endif
	smpd_exit(0);
    }

    /* check for debug option */
    if (smpd_get_opt_int(argcp, argvp, "-d", &dbg_flag))
    {
	smpd_process.dbg_state = dbg_flag;
	smpd_process.bNoTTY = SMPD_FALSE;
	smpd_process.bService = SMPD_FALSE;
    }
    if (smpd_get_opt(argcp, argvp, "-d"))
    {
	smpd_process.dbg_state = SMPD_DBG_STATE_ERROUT | SMPD_DBG_STATE_STDOUT | SMPD_DBG_STATE_PREPEND_RANK | SMPD_DBG_STATE_TRACE;
	smpd_process.bNoTTY = SMPD_FALSE;
	smpd_process.bService = SMPD_FALSE;
    }
    if (smpd_get_opt_int(argcp, argvp, "-debug", &dbg_flag))
    {
	smpd_process.dbg_state = dbg_flag;
	smpd_process.bNoTTY = SMPD_FALSE;
	smpd_process.bService = SMPD_FALSE;
    }
    if (smpd_get_opt(argcp, argvp, "-debug"))
    {
	smpd_process.dbg_state = SMPD_DBG_STATE_ERROUT | SMPD_DBG_STATE_STDOUT | SMPD_DBG_STATE_PREPEND_RANK | SMPD_DBG_STATE_TRACE;
	smpd_process.bNoTTY = SMPD_FALSE;
	smpd_process.bService = SMPD_FALSE;
    }

    /* check for port option */
    smpd_get_opt_int(argcp, argvp, "-p", &smpd_process.port);
    smpd_get_opt_int(argcp, argvp, "-port", &smpd_process.port);
    if (smpd_get_opt(argcp, argvp, "-anyport"))
    {
	smpd_process.port = 0;
	smpd_process.dbg_state = 0; /* turn of debugging or you won't be able to read the port from stdout */
	smpd_process.bNoTTY = SMPD_FALSE;
	smpd_process.bService = SMPD_FALSE;
    }

    smpd_process.noprompt = smpd_get_opt(argcp, argvp, "-noprompt");

#ifdef HAVE_WINDOWS_H

    /* check for service options */
    if (smpd_get_opt(argcp, argvp, "-remove") ||
	smpd_get_opt(argcp, argvp, "-unregserver") ||
	smpd_get_opt(argcp, argvp, "-uninstall") ||
	smpd_get_opt(argcp, argvp, "/Remove") ||
	smpd_get_opt(argcp, argvp, "/Uninstall"))
    {
	/*RegDeleteKey(HKEY_CURRENT_USER, MPICHKEY);*/
	smpd_remove_service(SMPD_TRUE);
	ExitProcess(0);
    }
    if (smpd_get_opt(argcp, argvp, "-install") ||
	smpd_get_opt(argcp, argvp, "-regserver") ||
	smpd_get_opt(argcp, argvp, "/Install") ||
	smpd_get_opt(argcp, argvp, "/install") ||
	smpd_get_opt(argcp, argvp, "/RegServer"))
    {
	char phrase[SMPD_PASSPHRASE_MAX_LENGTH]="", port_str[12]="";

	if (smpd_remove_service(SMPD_FALSE) == SMPD_FALSE)
	{
	    printf("Unable to remove the previous installation, install failed.\n");
	    ExitProcess(0);
	}

	if (smpd_get_opt_string(argcp, argvp, "-phrase", phrase, SMPD_PASSPHRASE_MAX_LENGTH) ||
	    smpd_get_win_opt_string(argcp, argvp, "/phrase", phrase, SMPD_PASSPHRASE_MAX_LENGTH))
	{
	    smpd_set_smpd_data("phrase", phrase);
	}
	if (smpd_get_opt(argcp, argvp, "-getphrase"))
	{
	    printf("passphrase for smpd: ");fflush(stdout);
	    smpd_get_password(phrase);
	    smpd_set_smpd_data("phrase", phrase);
	}
	if (smpd_get_opt_string(argcp, argvp, "-port", port_str, 10))
	{
	    smpd_set_smpd_data("port", port_str);
	}
	smpd_install_service(SMPD_FALSE, SMPD_TRUE, smpd_get_opt(argcp, argvp, "-delegation"));
	smpd_set_smpd_data("version", SMPD_VERSION);
	ExitProcess(0);
    }
    if (smpd_get_opt(argcp, argvp, "-start"))
    {
	smpd_start_service();
	ExitProcess(0);
    }
    if (smpd_get_opt(argcp, argvp, "-stop"))
    {
	smpd_stop_service();
	ExitProcess(0);
    }
    if (smpd_get_opt(argcp, argvp, "-register_spn"))
    {
	/*
	char domain_controller[SMPD_MAX_HOST_LENGTH] = "";
	char domain_name[SMPD_MAX_HOST_LENGTH] = "";
	char domain_host[SMPD_MAX_HOST_LENGTH] = "";
	smpd_get_opt_string(argcp, argvp, "-domain", domain_name, SMPD_MAX_HOST_LENGTH);
	smpd_get_opt_string(argcp, argvp, "-dc", domain_controller, SMPD_MAX_HOST_LENGTH);
	smpd_get_opt_string(argcp, argvp, "-host", domain_host, SMPD_MAX_HOST_LENGTH);
	smpd_register_spn(domain_controller, domain_name, domain_host);
	*/
	if (!smpd_setup_scp())
	{
	    printf("Failed to register smpd's Service Principal Name with the domain controller.\n");
	    ExitProcess((UINT)-1);
	}
	printf("Service Principal Name registered with the domain controller.\n");
	printf("SMPD is now capable of launching processes using passwordless delegation.\n");
	printf("The system administrator must ensure the following:\n");
	printf(" 1) This host is trusted for delegation in Active Directory\n");
	printf(" 2) All users who will run jobs are trusted for delegation.\n");
	printf("Domain administrators can enable these options for hosts and users\nin Active Directory on the domain controller.\n");
	ExitProcess(0);
    }
    if (smpd_get_opt(argcp, argvp, "-remove_spn"))
    {
	if (smpd_remove_scp())
	{
	    printf("Service Principal Name removed from the domain controller.\n");
	}
	else
	{
	    printf("Error: Failed to remove the Service Principal Name from the domain controller.\n");
	}
	ExitProcess(0);
    }

    if (smpd_get_opt(argcp, argvp, "-mgr"))
    {
	/* Set a ctrl-handler to kill child processes if this smpd is killed */
	if (!SetConsoleCtrlHandler(smpd_ctrl_handler, TRUE))
	{
	    result = GetLastError();
	    smpd_dbg_printf("unable to set the ctrl handler for the smpd manager, error %d.\n", result);
	}

	smpd_process.bService = SMPD_FALSE;
	if (!smpd_get_opt_string(argcp, argvp, "-read", read_handle_str, 20))
	{
	    smpd_err_printf("manager started without a read pipe handle.\n");
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
	if (!smpd_get_opt_string(argcp, argvp, "-write", write_handle_str, 20))
	{
	    smpd_err_printf("manager started without a write pipe handle.\n");
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
	hRead = smpd_decode_handle(read_handle_str);
	hWrite = smpd_decode_handle(write_handle_str);

	smpd_dbg_printf("manager creating listener and session sets.\n");

	result = SMPDU_Sock_create_set(&set);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("SMPDU_Sock_create_set(listener) failed,\nsock error: %s\n", get_sock_error_string(result));
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
	smpd_process.set = set;
	smpd_dbg_printf("created set for manager listener, %d\n", SMPDU_Sock_get_sock_set_id(set));
	port = 0;
	result = SMPDU_Sock_listen(set, NULL, &port, &listener); 
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("SMPDU_Sock_listen failed,\nsock error: %s\n", get_sock_error_string(result));
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
	smpd_dbg_printf("smpd manager listening on port %d\n", port);

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
	smpd_process.listener_context->state = SMPD_MGR_LISTENING;

	memset(str, 0, 20);
	snprintf(str, 20, "%d", port);
	smpd_dbg_printf("manager writing port back to smpd.\n");
	if (!WriteFile(hWrite, str, 20, &num_written, NULL))
	{
	    smpd_err_printf("WriteFile failed, error %d\n", GetLastError());
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
	CloseHandle(hWrite);
	if (num_written != 20)
	{
	    smpd_err_printf("wrote only %d bytes of 20\n", num_written);
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
	smpd_dbg_printf("manager reading account and password from smpd.\n");
	if (!ReadFile(hRead, smpd_process.UserAccount, SMPD_MAX_ACCOUNT_LENGTH, &num_read, NULL))
	{
	    smpd_err_printf("ReadFile failed, error %d\n", GetLastError());
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
	if (num_read != SMPD_MAX_ACCOUNT_LENGTH)
	{
	    smpd_err_printf("read only %d bytes of %d\n", num_read, SMPD_MAX_ACCOUNT_LENGTH);
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
	if (!ReadFile(hRead, smpd_process.UserPassword, SMPD_MAX_PASSWORD_LENGTH, &num_read, NULL))
	{
	    smpd_err_printf("ReadFile failed, error %d\n", GetLastError());
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
	if (num_read != SMPD_MAX_PASSWORD_LENGTH)
	{
	    smpd_err_printf("read only %d bytes of %d\n", num_read, SMPD_MAX_PASSWORD_LENGTH);
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
	if (!ReadFile(hRead, smpd_process.passphrase, SMPD_PASSPHRASE_MAX_LENGTH, &num_read, NULL))
	{
	    smpd_err_printf("ReadFile failed, error %d\n", GetLastError());
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
	if (num_read != SMPD_PASSPHRASE_MAX_LENGTH)
	{
	    smpd_err_printf("read only %d bytes of %d\n", num_read, SMPD_PASSPHRASE_MAX_LENGTH);
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
	smpd_process.credentials_prompt = SMPD_FALSE;

	result = smpd_enter_at_state(set, SMPD_MGR_LISTENING);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("state machine failed.\n");
	}

	/*
	result = SMPDU_Sock_finalize();
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("SMPDU_Sock_finalize failed,\nsock error: %s\n", get_sock_error_string(result));
	}
	*/
	smpd_exit(0);
	smpd_exit_fn(FCNAME);
	ExitProcess(0);
    }
#endif

    /* check for the status option */
    if (smpd_get_opt_string(argcp, argvp, "-status", smpd_process.console_host, SMPD_MAX_HOST_LENGTH))
    {
	smpd_process.do_console = 1;
	smpd_process.builtin_cmd = SMPD_CMD_DO_STATUS;
    }
    else if (smpd_get_opt(argcp, argvp, "-status"))
    {
	smpd_get_hostname(smpd_process.console_host, SMPD_MAX_HOST_LENGTH);
	smpd_process.do_console = 1;
	smpd_process.builtin_cmd = SMPD_CMD_DO_STATUS;
    }

    /* check for console options */
    if (smpd_get_opt_string(argcp, argvp, "-console", smpd_process.console_host, SMPD_MAX_HOST_LENGTH))
    {
	smpd_process.do_console = 1;
    }
    else if (smpd_get_opt(argcp, argvp, "-console"))
    {
	smpd_get_hostname(smpd_process.console_host, SMPD_MAX_HOST_LENGTH);
	smpd_process.do_console = 1;
    }
    if (smpd_process.do_console)
    {
	/* This may need to be changed to avoid conflict */
	if (smpd_get_opt(argcp, argvp, "-p"))
	{
	    smpd_process.use_process_session = 1;
	}
    }

    if (smpd_get_opt_string(argcp, argvp, "-shutdown", smpd_process.console_host, SMPD_MAX_HOST_LENGTH))
    {
	smpd_process.do_console = 1;
	smpd_process.builtin_cmd = SMPD_CMD_SHUTDOWN;
    }
    else if (smpd_get_opt(argcp, argvp, "-shutdown"))
    {
	smpd_get_hostname(smpd_process.console_host, SMPD_MAX_HOST_LENGTH);
	smpd_process.do_console = 1;
	smpd_process.builtin_cmd = SMPD_CMD_SHUTDOWN;
    }

    if (smpd_get_opt_string(argcp, argvp, "-restart", smpd_process.console_host, SMPD_MAX_HOST_LENGTH))
    {
	smpd_process.do_console = 1;
	smpd_process.builtin_cmd = SMPD_CMD_RESTART;
    }
    else if (smpd_get_opt(argcp, argvp, "-restart"))
    {
#ifdef HAVE_WINDOWS_H
	printf("restarting the smpd service...\n");
	smpd_stop_service();
	Sleep(1000);
	smpd_start_service();
	smpd_exit(0);
#else
	smpd_get_hostname(smpd_process.console_host, SMPD_MAX_HOST_LENGTH);
	smpd_process.do_console = 1;
	smpd_process.builtin_cmd = SMPD_CMD_RESTART;
#endif
    }

    if (smpd_get_opt_string(argcp, argvp, "-version", smpd_process.console_host, SMPD_MAX_HOST_LENGTH))
    {
	smpd_process.do_console = 1;
	smpd_process.builtin_cmd = SMPD_CMD_VERSION;
    }
    else if (smpd_get_opt(argcp, argvp, "-version"))
    {
	printf("%s\n", SMPD_VERSION);
	fflush(stdout);
	smpd_exit(0);
    }

    /* These commands are handled by mpiexec although doing them here is an alternate solution.
    if (smpd_get_opt_two_strings(argcp, argvp, "-add_job", smpd_process.job_key, SMPD_MAX_NAME_LENGTH, smpd_process.job_key_account, SMPD_MAX_ACCOUNT_LENGTH))
    {
	if (!smpd_get_opt_string(argcp, argvp, "-host", smpd_process.console_host, SMPD_MAX_HOST_LENGTH))
	    smpd_get_hostname(smpd_process.console_host, SMPD_MAX_HOST_LENGTH);
	if (smpd_get_opt_string(argcp, argvp, "-password", smpd_process.job_key_password, SMPD_MAX_PASSWORD_LENGTH)
	    smpd_process.builtin_cmd = SMPD_CMD_ADD_JOB_KEY_AND_PASSWORD;
	else
	    smpd_process.builtin_cmd = SMPD_CMD_ADD_JOB_KEY;
	smpd_process.do_console = 1;
    }

    if (smpd_get_opt_string(argcp, argvp, "-remove_job", smpd_process.job_key, SMPD_MAX_NAME_LENGTH))
    {
	if (!smpd_get_opt_string(argcp, argvp, "-host", smpd_process.console_host, SMPD_MAX_HOST_LENGTH))
	    smpd_get_hostname(smpd_process.console_host, SMPD_MAX_HOST_LENGTH);
	smpd_process.do_console = 1;
	smpd_process.builtin_cmd = SMPD_CMD_REMOVE_JOB_KEY;
    }

    if (smpd_get_opt_string(argcp, argvp, "-associate_job", smpd_process.job_key, SMPD_MAX_NAME_LENGTH))
    {
	if (!smpd_get_opt_string(argcp, argvp, "-host", smpd_process.console_host, SMPD_MAX_HOST_LENGTH))
	    smpd_get_hostname(smpd_process.console_host, SMPD_MAX_HOST_LENGTH);
	smpd_process.do_console = 1;
	smpd_process.builtin_cmd = SMPD_CMD_ASSOCIATE_JOB_KEY;
    }
    */

    smpd_get_opt_string(argcp, argvp, "-phrase", smpd_process.passphrase, SMPD_PASSPHRASE_MAX_LENGTH);
    if (smpd_get_opt(argcp, argvp, "-getphrase"))
    {
	printf("passphrase for smpd: ");fflush(stdout);
	smpd_get_password(smpd_process.passphrase);
    }

    if (smpd_get_opt_string(argcp, argvp, "-smpdfile", smpd_process.smpd_filename, SMPD_MAX_FILENAME))
    {
	struct stat s;

	if (stat(smpd_process.smpd_filename, &s) == 0)
	{
	    if (s.st_mode & 00077)
	    {
		printf(".smpd file cannot be readable by anyone other than the current user.\n");
		smpd_exit_fn(FCNAME);
		return SMPD_FAIL;
	    }
	}
    }

    if (smpd_get_opt_string(argcp, argvp, "-traceon", filename, SMPD_MAX_FILENAME))
    {
	smpd_process.do_console_returns = SMPD_TRUE;
	if (*argcp > 1)
	{
	    for (i=1; i<*argcp; i++)
	    {
		strcpy(smpd_process.console_host, (*argvp)[i]);

		smpd_process.do_console = 1;
		smpd_process.builtin_cmd = SMPD_CMD_SET;
		strcpy(smpd_process.key, "logfile");
		strcpy(smpd_process.val, filename);
		result = smpd_do_console();
		if (result != SMPD_SUCCESS)
		{
		    smpd_err_printf("Unable to set the logfile name on host '%s'\n", smpd_process.console_host);
		    smpd_exit_fn(FCNAME);
		    return result;
		}

		smpd_process.do_console = 1;
		smpd_process.builtin_cmd = SMPD_CMD_SET;
		strcpy(smpd_process.key, "log");
		strcpy(smpd_process.val, "yes");
		result = smpd_do_console();
		if (result != SMPD_SUCCESS)
		{
		    smpd_err_printf("Unable to set the log option on host '%s'\n", smpd_process.console_host);
		    smpd_exit_fn(FCNAME);
		    return result;
		}

		smpd_process.do_console = 1;
		smpd_process.builtin_cmd = SMPD_CMD_RESTART;
		result = smpd_do_console();
		if (result != SMPD_SUCCESS)
		{
		    smpd_err_printf("Unable to restart the smpd on host '%s'\n", smpd_process.console_host);
		    smpd_exit_fn(FCNAME);
		    return result;
		}
	    }
	}
	else
	{
	    result = smpd_set_smpd_data("logfile", filename);
	    result = smpd_set_smpd_data("log", "yes");

#ifdef HAVE_WINDOWS_H
	    printf("restarting the smpd service...\n");
	    smpd_stop_service();
	    Sleep(1000);
	    smpd_start_service();
#else
	    smpd_get_hostname(smpd_process.console_host, SMPD_MAX_HOST_LENGTH);
	    smpd_process.do_console = 1;
	    smpd_process.builtin_cmd = SMPD_CMD_RESTART;
	    result = smpd_do_console();
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("Unable to restart the smpd on host '%s'\n", smpd_process.console_host);
		smpd_exit_fn(FCNAME);
		return result;
	    }
#endif
	}
	smpd_exit_fn(FCNAME);
	smpd_exit(result);
    }

    if (smpd_get_opt(argcp, argvp, "-traceoff"))
    {
	smpd_process.do_console_returns = SMPD_TRUE;
	if (*argcp > 1)
	{
	    for (i=1; i<*argcp; i++)
	    {
		strcpy(smpd_process.console_host, (*argvp)[i]);

		smpd_process.do_console = 1;
		smpd_process.builtin_cmd = SMPD_CMD_SET;
		strcpy(smpd_process.key, "log");
		strcpy(smpd_process.val, "no");
		result = smpd_do_console();
		if (result != SMPD_SUCCESS)
		{
		    smpd_err_printf("Unable to set the log option on host '%s'\n", smpd_process.console_host);
		    smpd_exit_fn(FCNAME);
		    return result;
		}

		smpd_process.do_console = 1;
		smpd_process.builtin_cmd = SMPD_CMD_RESTART;
		result = smpd_do_console();
		if (result != SMPD_SUCCESS)
		{
		    smpd_err_printf("Unable to restart the smpd on host '%s'\n", smpd_process.console_host);
		    smpd_exit_fn(FCNAME);
		    return result;
		}
	    }
	}
	else
	{
	    result = smpd_set_smpd_data("log", "no");

#ifdef HAVE_WINDOWS_H
	    printf("restarting the smpd service...\n");
	    smpd_stop_service();
	    Sleep(1000);
	    smpd_start_service();
#else
	    smpd_get_hostname(smpd_process.console_host, SMPD_MAX_HOST_LENGTH);
	    smpd_process.do_console = 1;
	    smpd_process.builtin_cmd = SMPD_CMD_RESTART;
	    result = smpd_do_console();
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("Unable to restart the smpd on host '%s'\n", smpd_process.console_host);
		smpd_exit_fn(FCNAME);
		return result;
	    }
#endif
	}
	smpd_exit_fn(FCNAME);
	smpd_exit(result);
    }

    if (smpd_process.do_console)
    {
	result = smpd_do_console();
	smpd_exit_fn(FCNAME);
	return result;
    }

    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}
