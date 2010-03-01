/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include "smpd.h"
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_STDARG_H
#include <stdarg.h>
#endif
#ifdef HAVE_TERMIOS_H
#include <termios.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef HAVE_SIGACTION
#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif
#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif
#endif

smpd_global_t smpd_process = 
    { SMPD_IDLE,        /* state                  */
      -1,               /* id                     */
      -1,               /* parent_id              */
      -1,               /* level                  */
      NULL,             /* left_context           */
      NULL,             /* right_context          */
      NULL,             /* parent_context         */
      NULL,             /* context_list           */
      NULL,             /* listener_context       */
      NULL,             /* process_list           */
      SMPD_FALSE,       /* closing                */
      SMPD_FALSE,       /* root_smpd              */
      SMPDU_SOCK_INVALID_SET, /* set                    */
      "",               /* host                   */
      "",               /* pszExe                 */
      SMPD_FALSE,       /* bService               */
      SMPD_TRUE,        /* bNoTTY                 */
      SMPD_FALSE,       /* bPasswordProtect       */
      "",               /* SMPDPassword           */
      "",               /* passphrase             */
      SMPD_FALSE,       /* logon                  */
      "",               /* UserAccount            */
      "",               /* UserPassword           */
      0,                /* cur_tag                */
      SMPD_DBG_STATE_ERROUT, /* dbg_state         */
      /*NULL,*/             /* dbg_fout               */
      "",               /* dbg_filename           */
      SMPD_DBG_FILE_SIZE,/*dbg_file_size          */
      SMPD_FALSE,       /* have_dbs               */
      "",               /* kvs_name               */
      "",               /* domain_name            */
#ifdef HAVE_WINDOWS_H
      NULL,             /* hCloseStdinThreadEvent */
      NULL,             /* hStdinThread           */
#endif
#ifdef USE_PTHREAD_STDIN_REDIRECTION
      0,                /* stdin_thread           */
      0,                /* stdin_read             */
      0,                /* stdin_write            */
#endif
      SMPD_FALSE,       /* do_console             */
      SMPD_LISTENER_PORT, /* smpd port            */
      SMPD_FALSE,       /* Is singleton client ? */
      -1,               /* Port to connect back to a singleton client*/
      "",               /* console_host           */
      NULL,             /* host_list              */
      NULL,             /* launch_list            */
      SMPD_TRUE,        /* credentials_prompt     */
      SMPD_TRUE,        /* do_multi_color_output  */
      SMPD_FALSE,       /* no_mpi                 */
      SMPD_FALSE,       /* output_exit_codes      */
      SMPD_FALSE,       /* local_root             */
      SMPD_FALSE,       /* use_iproot             */
      SMPD_FALSE,       /* use_process_session    */
      0,                /* nproc                  */
      0,                /* nproc_launched         */
      0,                /* nproc_exited           */
      SMPD_FALSE,       /* verbose                */
#ifdef HAVE_WINDOWS_H
      SMPD_FALSE,       /* set_affinity             */
      NULL,             /* affinity_map             */
      0,                /* affinity_map_sz          */
#endif
      /*SMPD_FALSE,*/       /* shutdown               */
      /*SMPD_FALSE,*/       /* restart                */
      /*SMPD_FALSE,*/       /* validate               */
      /*SMPD_FALSE,*/       /* do_status              */
      SMPD_CMD_NONE,    /* builtin_cmd            */
#ifdef HAVE_WINDOWS_H
      FALSE,            /* bOutputInitialized     */
      NULL,             /* hOutputMutex           */
      NULL,             /* hLaunchProcessMutex    */
#endif
#ifdef USE_WIN_MUTEX_PROTECT
      NULL,             /* hDBSMutext             */
#endif
      NULL,             /* pDatabase              */
      NULL,             /* pDatabaseIter          */
      /*0,*/                /* nNextAvailableDBSID    */
      0,                /* nInitDBSRefCount       */
      NULL,             /* barrier_list           */
#ifdef HAVE_WINDOWS_H
      {                 /* ssStatus                    */
	  SERVICE_WIN32_OWN_PROCESS, /* dwServiceType  */
	  SERVICE_STOPPED,           /* dwCurrentState */
	  SERVICE_ACCEPT_STOP,   /* dwControlsAccepted */
	  NO_ERROR,     /* dwWin32ExitCode             */
	  0,            /* dwServiceSpecificExitCode   */
	  0,            /* dwCheckPoint                */
	  3000,         /* dwWaitHint                  */
      },
      NULL,             /* sshStatusHandle         */
      NULL,             /* hBombDiffuseEvent       */
      NULL,             /* hBombThread             */
#endif
      SMPD_FALSE,       /* service_stop            */
      SMPD_FALSE,       /* noprompt                */
      "",               /* smpd_filename           */
      SMPD_FALSE,       /* stdin_toall             */
      SMPD_FALSE,       /* stdin_redirecting       */
      NULL,             /* default_host_list       */
      NULL,             /* cur_default_host        */
      0,                /* cur_default_iproc       */
#ifdef HAVE_WINDOWS_H
      NULL,             /* hSMPDDataMutex          */
#endif
      "",               /* printf_buffer           */
      SMPD_SUCCESS,     /* state_machine_ret_val   */
      SMPD_FALSE,       /* exit_on_done            */
      0,                /* tree_parent             */
      1,                /* tree_id                 */
      NULL,             /* s_host_list             */
      NULL,             /* s_cur_host              */
      0,                /* s_cur_count             */
      SMPD_FALSE,       /* use_inherited_handles   */
      NULL,             /* pg_list                 */
      SMPD_FALSE,       /* use_abort_exit_code     */
      0,                /* abort_exit_code         */
      SMPD_TRUE,        /* verbose_abort_output    */
      0,                /* mpiexec_exit_code       */
      SMPD_FALSE,       /* map0to1                 */
      SMPD_FALSE,       /* rsh_mpiexec             */
      SMPD_FALSE,       /* mpiexec_inorder_launch  */
      SMPD_FALSE,       /* mpiexec_run_local       */
#ifdef HAVE_WINDOWS_H
      NULL,             /* timeout_thread          */
#else
#ifdef HAVE_PTHREADS_H
      NULL,             /* timeout_thread          */
#endif
#endif
      -1,               /* timeout                 */
      SMPDU_SOCK_INVALID_SOCK, /* timeout_sock     */
      SMPDU_SOCK_INVALID_SOCK, /* mpiexec_abort_sock */
      SMPD_TRUE,        /* use_pmi_server          */
      NULL,             /* mpiexec_argv0           */
      "dummy",          /* encrypt_prefix          */
      SMPD_FALSE,       /* plaintext               */
      SMPD_FALSE,       /* use_sspi                */
      SMPD_FALSE,       /* use_delegation          */
      SMPD_FALSE,       /* use_sspi_job_key        */
      SMPD_FALSE,       /* use_ms_hpc              */
#ifdef HAVE_WINDOWS_H
      NULL,             /* sec_fn                  */
#endif
      NULL,             /* sspi_context_list       */
      "",               /* job_key                 */
      "",               /* job_key_account         */
      "",               /* job_key_password        */
      "",               /* key                     */
      "",               /* val                     */
      SMPD_FALSE,       /* do_console_returns      */
      "",               /* env_channel             */
      "",               /* env_netmod              */
      "",               /* env_dll                 */
      "",               /* env_wrap_dll            */
      NULL,             /* delayed_spawn_queue     */
      SMPD_FALSE,       /* spawning                */
      0,                /* user_index              */
      SMPD_FALSE        /* prefix_output           */
    };

#undef FCNAME
#define FCNAME "smpd_post_abort_command"
int smpd_post_abort_command(char *fmt, ...)
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

    result = smpd_create_command("abort", smpd_process.id, 0, SMPD_FALSE, &cmd_ptr);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to create an abort command.\n");
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    result = smpd_add_command_arg(cmd_ptr, "error", error_str);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("Unable to add the error string to the abort command.\n");
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    result = smpd_command_destination(0, &context);
    if(result != SMPD_SUCCESS){
        smpd_err_printf("Unable to find destination for command...Aborting: %s\n", error_str);
        smpd_exit_fn(FCNAME);
        return SMPD_FAIL;
    }

    if (context == NULL)
    {
	if (smpd_process.left_context == NULL)
	{
	    printf("Aborting: %s\n", error_str);
	    fflush(stdout);
	    smpd_exit_fn(FCNAME);
	    smpd_exit(-1);
	}

	smpd_process.closing = SMPD_TRUE;
	result = smpd_create_command("close", 0, 1, SMPD_FALSE, &cmd_ptr);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to create the close command to tear down the job tree.\n");
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
	result = smpd_post_write_command(smpd_process.left_context, cmd_ptr);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to post a write of the close command to tear down the job tree as part of the abort process.\n");
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
    }
    else
    {
	smpd_dbg_printf("sending abort command to %s context: \"%s\"\n", smpd_get_context_str(context), cmd_ptr->cmd);
	result = smpd_post_write_command(context, cmd_ptr);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to post a write of the abort command to the %s context.\n", smpd_get_context_str(context));
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
    }
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#ifdef HAVE_SIGACTION
void smpd_child_handler(int code)
{
    int status;
    int pid;

    if (smpd_process.root_smpd && code == SIGCHLD)
    {
	/*pid = waitpid(-1, &status, WNOHANG);*/
	pid = waitpid(-1, &status, 0);
	if (pid < 0)
	{
	    fprintf(stderr, "waitpid failed, error %d\n", errno);
	}
	/*
	else
	{
	    printf("process %d exited with code: %d\n", pid, WIFEXITED(status) ? WEXITSTATUS(status) : -1);
	    fflush(stdout);
	}
	*/
    }
}
#endif

#ifdef HAVE_WINDOWS_H
#undef FCNAME
#define FCNAME "smpd_make_socket_loop"
int smpd_make_socket_loop(SOCKET *pRead, SOCKET *pWrite)
{
    SOCKET sock;
    char host[100];
    int port;
    int len;
    /*LINGER linger;*/
    BOOL b;
    SOCKADDR_IN sockAddr;
    int error;

    smpd_enter_fn(FCNAME);

    /* Create a listener */

    /* create the socket */
    sock = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
    if (sock == INVALID_SOCKET)
    {
	*pRead = INVALID_SOCKET;
	*pWrite = INVALID_SOCKET;
	smpd_exit_fn(FCNAME);
	return WSAGetLastError();
    }

    memset(&sockAddr,0,sizeof(sockAddr));
    
    sockAddr.sin_family = AF_INET;
    sockAddr.sin_addr.s_addr = INADDR_ANY;
    sockAddr.sin_port = htons((unsigned short)ADDR_ANY);
    
    if (bind(sock, (SOCKADDR*)&sockAddr, sizeof(sockAddr)) == SOCKET_ERROR)
    {
	error = WSAGetLastError();
	smpd_err_printf("bind failed: error %d\n", error);
	*pRead = INVALID_SOCKET;
	*pWrite = INVALID_SOCKET;
	smpd_exit_fn(FCNAME);
	return error;
    }
    
    /* listen */
    listen(sock, 2);

    /* get the host and port where we're listening */
    len = sizeof(sockAddr);
    getsockname(sock, (struct sockaddr*)&sockAddr, &len);
    port = ntohs(sockAddr.sin_port);
    smpd_get_hostname(host, 100);

    /* Connect to myself */

    /* create the socket */
    *pWrite = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
    if (*pWrite == INVALID_SOCKET)
    {
	error = WSAGetLastError();
	smpd_err_printf("WSASocket failed, error %d\n", error);
	if (closesocket(sock) == SOCKET_ERROR)
	{
	    smpd_err_printf("closesocket failed, sock %d, error %d\n", sock, WSAGetLastError());
	}
	*pRead = INVALID_SOCKET;
	*pWrite = INVALID_SOCKET;
	smpd_exit_fn(FCNAME);
	return error;
    }

    /* set the nodelay option */
    /*
    b = TRUE;
    setsockopt(*pWrite, IPPROTO_TCP, TCP_NODELAY, (char*)&b, sizeof(BOOL));
    */

    /* Set the linger on close option */
    /*
    linger.l_onoff = 1 ;
    linger.l_linger = 60;
    setsockopt(*pWrite, SOL_SOCKET, SO_LINGER, (char*)&linger, sizeof(linger));
    */

    sockAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    /* connect to myself */
    if (connect(*pWrite, (SOCKADDR*)&sockAddr, sizeof(sockAddr)) == SOCKET_ERROR)
    {
	error = WSAGetLastError();
	if (closesocket(*pWrite) ==  SOCKET_ERROR)
	{
	    smpd_err_printf("closesocket failed, sock %d, error %d\n", *pWrite, WSAGetLastError());
	}
	if (closesocket(sock) == SOCKET_ERROR)
	{
	    smpd_err_printf("closesocket failed, sock %d, error %d\n", sock, WSAGetLastError());
	}
	*pRead = INVALID_SOCKET;
	*pWrite = INVALID_SOCKET;
	smpd_exit_fn(FCNAME);
	return error;
    }

    /* Accept the connection from myself */
    len = sizeof(sockAddr);
    *pRead = accept(sock, (SOCKADDR*)&sockAddr, &len);

    /* set the nodelay option */
    b = TRUE;
    setsockopt(*pWrite, IPPROTO_TCP, TCP_NODELAY, (char*)&b, sizeof(BOOL));
    /* set the nodelay option */
    b = TRUE;
    setsockopt(*pRead, IPPROTO_TCP, TCP_NODELAY, (char*)&b, sizeof(BOOL));

    if (closesocket(sock) == SOCKET_ERROR)
    {
	smpd_err_printf("closesocket failed, sock %d, error %d\n", sock, WSAGetLastError());
    }
    smpd_exit_fn(FCNAME);
    return 0;
}

#undef FCNAME
#define FCNAME "smpd_make_socket_loop_choose"
int smpd_make_socket_loop_choose(SOCKET *pRead, int read_overlapped, SOCKET *pWrite, int write_overlapped)
{
    SOCKET sock;
    char host[100];
    int port;
    int len;
    /*LINGER linger;*/
    BOOL b;
    SOCKADDR_IN sockAddr;
    int error;
    DWORD flag;

    smpd_enter_fn(FCNAME);

    /* Create a listener */

    /* create the socket */
    flag = read_overlapped ? WSA_FLAG_OVERLAPPED : 0;
    sock = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, flag);
    if (sock == INVALID_SOCKET)
    {
	*pRead = INVALID_SOCKET;
	*pWrite = INVALID_SOCKET;
	smpd_exit_fn(FCNAME);
	return WSAGetLastError();
    }

    memset(&sockAddr,0,sizeof(sockAddr));
    
    sockAddr.sin_family = AF_INET;
    sockAddr.sin_addr.s_addr = INADDR_ANY;
    sockAddr.sin_port = htons((unsigned short)ADDR_ANY);

    if (bind(sock, (SOCKADDR*)&sockAddr, sizeof(sockAddr)) == SOCKET_ERROR)
    {
	error = WSAGetLastError();
	smpd_err_printf("bind failed: error %d\n", error);
	*pRead = INVALID_SOCKET;
	*pWrite = INVALID_SOCKET;
	smpd_exit_fn(FCNAME);
	return error;
    }
    
    /* listen */
    listen(sock, 2);

    /* get the host and port where we're listening */
    len = sizeof(sockAddr);
    getsockname(sock, (struct sockaddr*)&sockAddr, &len);
    port = ntohs(sockAddr.sin_port);
    smpd_get_hostname(host, 100);

    /* Connect to myself */

    /* create the socket */
    flag = write_overlapped ? WSA_FLAG_OVERLAPPED : 0;
    *pWrite = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, flag);
    if (*pWrite == INVALID_SOCKET)
    {
	error = WSAGetLastError();
	smpd_err_printf("WSASocket failed, error %d\n", error);
	if (closesocket(sock) == SOCKET_ERROR)
	{
	    smpd_err_printf("closesocket failed, sock %d, error %d\n", sock, WSAGetLastError());
	}
	*pRead = INVALID_SOCKET;
	*pWrite = INVALID_SOCKET;
	smpd_exit_fn(FCNAME);
	return error;
    }

    /* set the nodelay option */
    b = TRUE;
    setsockopt(*pWrite, IPPROTO_TCP, TCP_NODELAY, (char*)&b, sizeof(BOOL));

    /* Set the linger on close option */
    /*
    linger.l_onoff = 1 ;
    linger.l_linger = 60;
    setsockopt(*pWrite, SOL_SOCKET, SO_LINGER, (char*)&linger, sizeof(linger));
    */

    sockAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    /* connect to myself */
    if (connect(*pWrite, (SOCKADDR*)&sockAddr, sizeof(sockAddr)) == SOCKET_ERROR)
    {
	error = WSAGetLastError();
	if (closesocket(*pWrite) == SOCKET_ERROR)
	{
	    smpd_err_printf("closesocket failed, sock %d, error %d\n", *pWrite, WSAGetLastError());
	}
	if (closesocket(sock) == SOCKET_ERROR)
	{
	    smpd_err_printf("closesocket failed, sock %d, error %d\n", sock, WSAGetLastError());
	}
	*pRead = INVALID_SOCKET;
	*pWrite = INVALID_SOCKET;
	smpd_exit_fn(FCNAME);
	return error;
    }

    /* Accept the connection from myself */
    len = sizeof(sockAddr);
    *pRead = accept(sock, (SOCKADDR*)&sockAddr, &len);

    /* set the nodelay option */
    b = TRUE;
    setsockopt(*pRead, IPPROTO_TCP, TCP_NODELAY, (char*)&b, sizeof(BOOL));

    if (closesocket(sock) == SOCKET_ERROR)
    {
	smpd_err_printf("closesocket failed, sock %d, error %d\n", sock, WSAGetLastError());
    }
    smpd_exit_fn(FCNAME);
    return 0;
}
#endif

#undef FCNAME
#define FCNAME "smpd_init_process"
int smpd_init_process(void)
{
#ifdef HAVE_WINDOWS_H
    HMODULE hModule;
#else
    char *homedir;
    struct stat s;
#endif
#ifdef HAVE_SIGACTION
    struct sigaction act;
#endif

    smpd_enter_fn(FCNAME);

    /* initialize the debugging output print engine */
    smpd_init_printf();

    /* tree data */
    smpd_process.parent_id = -1;
    smpd_process.id = -1;
    smpd_process.level = -1;
    smpd_process.left_context = NULL;
    smpd_process.right_context = NULL;
    smpd_process.parent_context = NULL;
    smpd_process.set = SMPDU_SOCK_INVALID_SET;

    /* local data */
#ifdef HAVE_WINDOWS_H
    hModule = GetModuleHandle(NULL);
    if (!GetModuleFileName(hModule, smpd_process.pszExe, SMPD_MAX_EXE_LENGTH)) 
	smpd_process.pszExe[0] = '\0';
#else
    smpd_process.pszExe[0] = '\0';
#endif
    strcpy(smpd_process.SMPDPassword, SMPD_DEFAULT_PASSWORD);
    smpd_process.bPasswordProtect = SMPD_FALSE;
    smpd_process.bService = SMPD_FALSE;
    smpd_get_hostname(smpd_process.host, SMPD_MAX_HOST_LENGTH);
    smpd_process.UserAccount[0] = '\0';
    smpd_process.UserPassword[0] = '\0';
    smpd_process.closing = SMPD_FALSE;
    smpd_process.root_smpd = SMPD_FALSE;

    srand(smpd_getpid());

#ifdef HAVE_SIGACTION
    memset(&act, 0, sizeof(act));
    act.sa_handler = smpd_child_handler;
#ifdef SA_NODEFER
    act.sa_flags = SA_NOCLDSTOP | SA_NODEFER;
#else
    act.sa_flags = SA_NOCLDSTOP;
#endif
    sigaction(SIGCHLD, &act, NULL);
#endif

#ifdef HAVE_WINDOWS_H
    smpd_process.hBombDiffuseEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    smpd_process.hLaunchProcessMutex = CreateMutex(NULL, FALSE, NULL);
#else
    /* Setting the smpd_filename to default filename is done in smpd_get_smpd_data()
     * Avoid duplicating the effort
     */ 
	/*
    homedir = getenv("HOME");
    if(homedir != NULL){
        strcpy(smpd_process.smpd_filename, homedir);
        if (smpd_process.smpd_filename[strlen(smpd_process.smpd_filename)-1] != '/')
	    strcat(smpd_process.smpd_filename, "/.smpd");
        else
	    strcat(smpd_process.smpd_filename, ".smpd");
    }else{
	strcpy(smpd_process.smpd_filename, ".smpd");
    }
    if (stat(smpd_process.smpd_filename, &s) == 0)
    {
	if (s.st_mode & 00077)
	{
	    printf("smpd file, %s, cannot be readable by anyone other than the current user, please set the permissions accordingly (0600).\n", smpd_process.smpd_filename);
	    smpd_process.smpd_filename[0] = '\0';
	}
    }
    else
    {
	smpd_process.smpd_filename[0] = '\0';
    }
	*/
#endif
	smpd_process.smpd_filename[0] = '\0';
	smpd_process.passphrase[0] = '\0';
    /* smpd_init_process() should not try to get the passphrase. Just initialize the values */
    /* smpd_get_smpd_data("phrase", smpd_process.passphrase, SMPD_PASSPHRASE_MAX_LENGTH); */

    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_init_context"
int smpd_init_context(smpd_context_t *context, smpd_context_type_t type, SMPDU_Sock_set_t set, SMPDU_Sock_t sock, int id)
{
    int result;

    smpd_enter_fn(FCNAME);
    context->type = type;
    context->target = SMPD_TARGET_UNDETERMINED;
    context->access = SMPD_ACCESS_USER_PROCESS;/*SMPD_ACCESS_NONE;*/
    context->host[0] = '\0';
    context->id = id;
    context->rank = 0;
    context->write_list = NULL;
    context->wait_list = NULL;
    smpd_init_command(&context->read_cmd);
    context->next = NULL;
    context->set = set;
    context->sock = sock;
    context->state = SMPD_IDLE;
    context->read_state = SMPD_IDLE;
    context->write_state = SMPD_IDLE;
    context->account[0] = '\0';
    context->domain[0] = '\0';
    context->full_domain[0] = '\0';
    context->connect_return_id = -1;
    context->connect_return_tag = -1;
    context->connect_to = NULL;
    context->spawn_context = NULL;
    context->cred_request[0] = '\0';
    context->password[0] = '\0';
    context->encrypted_password[0] = '\0';
    context->port_str[0] = '\0';
    context->pszChallengeResponse[0] = '\0';
    context->pszCrypt[0] = '\0';
    context->pwd_request[0] = '\0';
    context->session[0] = '\0';
    context->session_header[0] = '\0';
    context->singleton_init_hostname[0] = '\0';
    context->singleton_init_kvsname[0] = '\0';
    context->singleton_init_domainname[0] = '\0';
    context->singleton_init_pm_port = -1;
    context->smpd_pwd[0] = '\0';
#ifdef HAVE_WINDOWS_H
    context->wait.hProcess = NULL;
    context->wait.hThread = NULL;
#else
    context->wait = 0;
#endif
    context->process = NULL;
    context->sspi_header[0] = '\0';
    context->sspi_context = NULL;
    context->sspi_type = SMPD_SSPI_DELEGATE;
    context->sspi_job_key[0] = '\0';
    context->first_output_stderr = SMPD_TRUE;
    context->first_output_stdout = SMPD_TRUE;

    if (sock != SMPDU_SOCK_INVALID_SOCK)
    {
	result = SMPDU_Sock_set_user_ptr(sock, context);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to set the sock user ptr while initializing context,\nsock error: %s\n",
		get_sock_error_string(result));
	}
    }

    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_create_sspi_client_context"
int smpd_create_sspi_client_context(smpd_sspi_client_context_t **new_context)
{
    smpd_sspi_client_context_t *context;

    smpd_enter_fn(FCNAME);

    context = (smpd_sspi_client_context_t *)MPIU_Malloc(sizeof(smpd_sspi_client_context_t));
    if (context == NULL)
    {
	*new_context = NULL;
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    context->buffer = NULL;
    context->buffer_length = 0;
    context->out_buffer = NULL;
    context->out_buffer_length = 0;
    context->max_buffer_size = 0;
#ifdef HAVE_WINDOWS_H
    SecInvalidateHandle(&context->credential);
    SecInvalidateHandle(&context->context);
    memset(&context->expiration_time, 0, sizeof(TimeStamp));
    context->user_handle = INVALID_HANDLE_VALUE;
    context->job = INVALID_HANDLE_VALUE;
    context->flags = 0;
    context->close_handle = SMPD_TRUE;
#endif
    /* FIXME: this insertion needs to be thread safe */
    if (smpd_process.sspi_context_list == NULL)
    {
	context->id = 0;
    }
    else
    {
	context->id = smpd_process.sspi_context_list->id + 1;
    }
    context->next = smpd_process.sspi_context_list;
    smpd_process.sspi_context_list = context;
    *new_context = context;
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_free_sspi_client_context"
int smpd_free_sspi_client_context(smpd_sspi_client_context_t **context)
{
    smpd_sspi_client_context_t *iter, *trailer;

    smpd_enter_fn(FCNAME);

    trailer = iter = smpd_process.sspi_context_list;
    while (iter)
    {
	if (iter == *context)
	{
	    if (trailer != iter)
	    {
		trailer->next = iter->next;
	    }
	    else
	    {
		smpd_process.sspi_context_list = iter->next;
	    }
	    break;
	}
	if (trailer != iter)
	    trailer = trailer->next;
	iter = iter->next;
    }
    if (iter == NULL)
    {
	smpd_dbg_printf("freeing a sspi_client_context not in the global list\n");
    }
    /* FIXME: cleanup sspi structures */
    MPIU_Free(*context);
    *context = NULL;
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_generate_session_header"
int smpd_generate_session_header(char *str, int session_id)
{
    char * str_orig;
    int result;
    int len;

    smpd_enter_fn(FCNAME);

    str_orig = str;
    *str = '\0';
    len = SMPD_MAX_SESSION_HEADER_LENGTH;

    /* add header fields */
    result = MPIU_Str_add_int_arg(&str, &len, "id", session_id);
    if (result != MPIU_STR_SUCCESS)
    {
	smpd_err_printf("unable to create session header, adding session id failed.\n");
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    result = MPIU_Str_add_int_arg(&str, &len, "parent", smpd_process.id);
    if (result != MPIU_STR_SUCCESS)
    {
	smpd_err_printf("unable to create session header, adding parent id failed.\n");
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    result = MPIU_Str_add_int_arg(&str, &len, "level", smpd_process.level + 1);
    if (result != MPIU_STR_SUCCESS)
    {
	smpd_err_printf("unable to create session header, adding session level failed.\n");
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    /* remove the trailing space */
    str--;
    *str = '\0';

    smpd_dbg_printf("session header: (%s)\n", str_orig);
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_get_password"
void smpd_get_password(char *password)
{
#ifdef HAVE_WINDOWS_H
    HANDLE hStdin;
    DWORD dwMode;
#else
    struct termios terminal_settings, original_settings;
#endif
    size_t len;

    smpd_enter_fn(FCNAME);

#ifdef HAVE_WINDOWS_H

    hStdin = GetStdHandle(STD_INPUT_HANDLE);
    if (!GetConsoleMode(hStdin, &dwMode))
	dwMode = ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT | ENABLE_MOUSE_INPUT;
    SetConsoleMode(hStdin, dwMode & ~ENABLE_ECHO_INPUT);
    *password = '\0';
    fgets(password, 100, stdin);
    SetConsoleMode(hStdin, dwMode);

    fprintf(stderr, "\n");

#else

    /* save the current terminal settings */
    tcgetattr(STDIN_FILENO, &terminal_settings);
    original_settings = terminal_settings;

    /* turn off echo */
    terminal_settings.c_lflag &= ~ECHO;
    terminal_settings.c_lflag |= ECHONL;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &terminal_settings);

    /* check that echo is off */
    tcgetattr(STDIN_FILENO, &terminal_settings);
    if (terminal_settings.c_lflag & ECHO)
    {
	smpd_err_printf("\nunable to turn off the terminal echo\n");
	tcsetattr(STDIN_FILENO, TCSANOW, &original_settings);
    }

    fgets(password, 100, stdin);

    /* restore the original settings */
    tcsetattr(STDIN_FILENO, TCSANOW, &original_settings);

#endif

    while ((len = strlen(password)) > 0)
    {
	if (password[len-1] == '\r' || password[len-1] == '\n')
	    password[len-1] = '\0';
	else
	    break;
    }
    smpd_exit_fn(FCNAME);
}

#undef FCNAME
#define FCNAME "smpd_get_account_and_password"
void smpd_get_account_and_password(char *account, char *password)
{
    size_t len;
#ifdef HAVE_WINDOWS_H
    char default_username[100] = "";
    ULONG default_len = 100;
#endif

    smpd_enter_fn(FCNAME);

#ifdef HAVE_WINDOWS_H
    if (!GetUserNameEx(NameSamCompatible, default_username, &default_len))
    {
	default_username[0] = '\0';
    }
#endif

    do
    {
	do
	{
#ifdef HAVE_WINDOWS_H
	    if (default_username[0] != '\0')
	    {
		fprintf(stderr, "account (domain\\user) [%s]: ", default_username);
	    }
	    else
	    {
		fprintf(stderr, "account (domain\\user): ");
	    }
#else
	    fprintf(stderr, "account (domain\\user): ");
#endif
	    fflush(stderr);
	    *account = '\0';
	    fgets(account, 100, stdin);
	    while (strlen(account))
	    {
		len = strlen(account);
		if (account[len-1] == '\r' || account[len-1] == '\n')
		    account[len-1] = '\0';
		else
		    break;
	    }
#ifdef HAVE_WINDOWS_H
	    if ((strlen(account) == 0) && (default_username[0] != '\0'))
	    {
		strcpy(account, default_username);
	    }
#endif
	} 
	while (strlen(account) == 0);

	fprintf(stderr, "password: ");
	fflush(stderr);

	smpd_get_password(password);
	if (strlen(password) == 0)
	{
	    fprintf(stderr, "MPICH2 is unable to manage jobs using credentials with a blank password.\nPlease enter another account.\n");
	}
    }
    while (strlen(password) == 0);

    smpd_exit_fn(FCNAME);
}

#undef FCNAME
#define FCNAME "smpd_get_credentials_from_parent"
int smpd_get_credentials_from_parent(SMPDU_Sock_set_t set, SMPDU_Sock_t sock)
{
    smpd_enter_fn(FCNAME);
    SMPD_UNREFERENCED_ARG(set);
    SMPD_UNREFERENCED_ARG(sock);
    smpd_exit_fn(FCNAME);
    return SMPD_FAIL;
}

#undef FCNAME
#define FCNAME "smpd_get_smpd_password_from_parent"
int smpd_get_smpd_password_from_parent(SMPDU_Sock_set_t set, SMPDU_Sock_t sock)
{
    smpd_enter_fn(FCNAME);
    SMPD_UNREFERENCED_ARG(set);
    SMPD_UNREFERENCED_ARG(sock);
    smpd_exit_fn(FCNAME);
    return SMPD_FAIL;
}

#undef FCNAME
#define FCNAME "smpd_get_default_hosts"
int smpd_get_default_hosts()
{
    char hosts[8192];
    char *host;
    char *ncpu;
    smpd_host_node_t *cur_host, *iter;
#ifdef HAVE_WINDOWS_H
    DWORD len;
#else
    int dynamic = SMPD_FALSE;
    char myhostname[SMPD_MAX_HOST_LENGTH];
    int found;
#endif

    smpd_enter_fn(FCNAME);

    if (smpd_process.default_host_list != NULL && smpd_process.cur_default_host != NULL)
    {
	smpd_dbg_printf("default list already populated, returning success.\n");
	smpd_exit_fn(FCNAME);
	return SMPD_SUCCESS;
    }

    if (smpd_get_smpd_data("hosts", hosts, 8192) != SMPD_SUCCESS)
    {
#ifdef HAVE_WINDOWS_H
	len = 8192;
	/*if (GetComputerName(hosts, &len))*/
	if (GetComputerNameEx(ComputerNameDnsFullyQualified, hosts, &len))
	{
	    smpd_process.default_host_list = (smpd_host_node_t*)MPIU_Malloc(sizeof(smpd_host_node_t));
	    if (smpd_process.default_host_list == NULL)
	    {
		smpd_exit_fn(FCNAME);
		return SMPD_FAIL;
	    }
	    strcpy(smpd_process.default_host_list->host, hosts);
	    smpd_process.default_host_list->alt_host[0] = '\0';
	    smpd_process.default_host_list->nproc = 1;
	    smpd_process.default_host_list->connected = SMPD_FALSE;
	    smpd_process.default_host_list->connect_cmd_tag = -1;
	    smpd_process.default_host_list->next = smpd_process.default_host_list;
	    smpd_process.default_host_list->left = NULL;
	    smpd_process.default_host_list->right = NULL;
	    smpd_process.cur_default_host = smpd_process.default_host_list;
	    smpd_exit_fn(FCNAME);
	    return SMPD_SUCCESS;
	}
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
#else
	if (smpd_option_on("no_dynamic_hosts"))
	{
	    if (smpd_get_hostname(myhostname, SMPD_MAX_HOST_LENGTH) == SMPD_SUCCESS)
	    {
		smpd_process.default_host_list = (smpd_host_node_t*)MPIU_Malloc(sizeof(smpd_host_node_t));
		if (smpd_process.default_host_list == NULL)
		{
		    smpd_exit_fn(FCNAME);
		    return SMPD_FAIL;
		}
		strcpy(smpd_process.default_host_list->host, myhostname);
		smpd_process.default_host_list->alt_host[0] = '\0';
		smpd_process.default_host_list->nproc = 1;
		smpd_process.default_host_list->connected = SMPD_FALSE;
		smpd_process.default_host_list->connect_cmd_tag = -1;
		smpd_process.default_host_list->next = smpd_process.default_host_list;
		smpd_process.default_host_list->left = NULL;
		smpd_process.default_host_list->right = NULL;
		smpd_process.cur_default_host = smpd_process.default_host_list;
		smpd_exit_fn(FCNAME);
		return SMPD_SUCCESS;
	    }
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}

	smpd_lock_smpd_data();
	if (smpd_get_smpd_data(SMPD_DYNAMIC_HOSTS_KEY, hosts, 8192) != SMPD_SUCCESS)
	{
	    smpd_unlock_smpd_data();
	    if (smpd_get_hostname(hosts, 8192) == 0)
	    {
		smpd_process.default_host_list = (smpd_host_node_t*)MPIU_Malloc(sizeof(smpd_host_node_t));
		if (smpd_process.default_host_list == NULL)
		{
		    smpd_exit_fn(FCNAME);
		    return SMPD_FAIL;
		}
		strcpy(smpd_process.default_host_list->host, hosts);
		smpd_process.default_host_list->alt_host[0] = '\0';
		smpd_process.default_host_list->nproc = 1;
		smpd_process.default_host_list->connected = SMPD_FALSE;
		smpd_process.default_host_list->connect_cmd_tag = -1;
		smpd_process.default_host_list->next = smpd_process.default_host_list;
		smpd_process.default_host_list->left = NULL;
		smpd_process.default_host_list->right = NULL;
		smpd_process.cur_default_host = smpd_process.default_host_list;
		/* add this host to the dynamic_hosts key */
		strcpy(myhostname, hosts);
		smpd_lock_smpd_data();
		hosts[0] = '\0';
		smpd_get_smpd_data(SMPD_DYNAMIC_HOSTS_KEY, hosts, 8192);
		if (strlen(hosts) > 0)
		{
		    /* FIXME this could overflow */
		    strcat(hosts, " ");
		    strcat(hosts, myhostname);
		}
		else
		{
		    strcpy(hosts, myhostname);
		}
		smpd_set_smpd_data(SMPD_DYNAMIC_HOSTS_KEY, hosts);
		smpd_unlock_smpd_data();
		smpd_exit_fn(FCNAME);
		return SMPD_SUCCESS;
	    }
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
	smpd_unlock_smpd_data();
	if (smpd_get_hostname(myhostname, SMPD_MAX_HOST_LENGTH) != 0)
	{
	    dynamic = SMPD_FALSE;
	    myhostname[0] = '\0';
	}
	else
	{
	    dynamic = SMPD_TRUE;
	}
#endif
    }

    /* FIXME: Insert code here to parse a compressed host string */
    /* For now, just use a space separated list of host names */

    host = strtok(hosts, " \t\r\n");
    while (host)
    {
	cur_host = (smpd_host_node_t*)MPIU_Malloc(sizeof(smpd_host_node_t));
	if (cur_host != NULL)
	{
	    /*printf("default host: %s\n", host);*/
	    strcpy(cur_host->host, host);
	    cur_host->alt_host[0] = '\0';
	    cur_host->nproc = 1;
	    ncpu = strstr(cur_host->host, ":");
	    if (ncpu)
	    {
		*ncpu = '\0';
		ncpu++;
		cur_host->nproc = atoi(ncpu);
		if (cur_host->nproc < 1)
		    cur_host->nproc = 1;
	    }
	    cur_host->connected = SMPD_FALSE;
	    cur_host->connect_cmd_tag = -1;
	    cur_host->next = NULL;
	    cur_host->left = NULL;
	    cur_host->right = NULL;
	    if (smpd_process.default_host_list == NULL)
	    {
		smpd_process.default_host_list = cur_host;
	    }
	    else
	    {
		iter = smpd_process.default_host_list;
		while (iter->next)
		    iter = iter->next;
		iter->next = cur_host;
	    }
	}
	host = strtok(NULL, " \t\r\n");
    }
    if (smpd_process.default_host_list)
    {
#ifndef HAVE_WINDOWS_H
	if (dynamic)
	{
	    found = SMPD_FALSE;
	    iter = smpd_process.default_host_list;
	    while (iter)
	    {
		if (strcmp(iter->host, myhostname) == 0)
		{
		    found = SMPD_TRUE;
		    break;
		}
		iter = iter->next;
	    }
	    if (!found)
	    {
		/* add this host to the dynamic_hosts key */
		smpd_lock_smpd_data();
		hosts[0] = '\0';
		smpd_get_smpd_data(SMPD_DYNAMIC_HOSTS_KEY, hosts, 8192);
		if (strlen(hosts) > 0)
		{
		    /* FIXME this could overflow */
		    strcat(hosts, " ");
		    strcat(hosts, myhostname);
		}
		else
		{
		    strcpy(hosts, myhostname);
		}
		smpd_set_smpd_data(SMPD_DYNAMIC_HOSTS_KEY, hosts);
		smpd_unlock_smpd_data();
	    }
	}
#endif
	/* make the default list into a ring */
	iter = smpd_process.default_host_list;
	while (iter->next)
	    iter = iter->next;
	iter->next = smpd_process.default_host_list;
	/* point the cur_default_host to the first node in the ring */
	smpd_process.cur_default_host = smpd_process.default_host_list;
    }

    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_add_host_to_default_list"
int smpd_add_host_to_default_list(const char *hostname)
{
    int result;
    smpd_enter_fn(FCNAME);
    result = smpd_add_extended_host_to_default_list(hostname, NULL, 1);
    smpd_exit_fn(FCNAME);
    return result;
}

#undef FCNAME
#define FCNAME "smpd_add_extended_host_to_default_list"
int smpd_add_extended_host_to_default_list(const char *hostname, const char *alt_hostname, const int num_cpus)
{
    smpd_host_node_t *iter;

    smpd_enter_fn(FCNAME);

    iter = smpd_process.default_host_list;
    if (iter == NULL)
    {
	smpd_process.default_host_list = (smpd_host_node_t*)MPIU_Malloc(sizeof(smpd_host_node_t));
	if (smpd_process.default_host_list == NULL)
	{
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
	strcpy(smpd_process.default_host_list->host, hostname);
	smpd_process.default_host_list->alt_host[0] = '\0';
	if (alt_hostname != NULL)
	{
	    strcpy(smpd_process.default_host_list->alt_host, alt_hostname);
	}
	smpd_process.default_host_list->nproc = num_cpus;
	smpd_process.default_host_list->connected = SMPD_FALSE;
	smpd_process.default_host_list->connect_cmd_tag = -1;
	smpd_process.default_host_list->next = smpd_process.default_host_list;
	smpd_process.default_host_list->left = NULL;
	smpd_process.default_host_list->right = NULL;
	smpd_process.cur_default_host = smpd_process.default_host_list;
	smpd_exit_fn(FCNAME);
	return SMPD_SUCCESS;
    }

    if (strcmp(iter->host, hostname) == 0)
    {
	smpd_exit_fn(FCNAME);
	return SMPD_SUCCESS;
    }
    while (iter->next != NULL && iter->next != smpd_process.default_host_list)
    {
	iter = iter->next;
	if (strcmp(iter->host, hostname) == 0)
	{
	    smpd_exit_fn(FCNAME);
	    return SMPD_SUCCESS;
	}
    }

    iter->next = (smpd_host_node_t*)MPIU_Malloc(sizeof(smpd_host_node_t));
    if (iter->next == NULL)
    {
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    iter = iter->next;
    strcpy(iter->host, hostname);
    iter->alt_host[0] = '\0';
    if (alt_hostname != NULL)
    {
	strcpy(iter->alt_host, alt_hostname);
    }
    iter->nproc = num_cpus;
    iter->connected = SMPD_FALSE;
    iter->connect_cmd_tag = -1;
    iter->next = smpd_process.default_host_list;
    iter->left = NULL;
    iter->right = NULL;

    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_insert_into_dynamic_hosts"
int smpd_insert_into_dynamic_hosts(void)
{
#ifndef HAVE_WINDOWS_H
    char myhostname[SMPD_MAX_HOST_LENGTH];
    char hosts[8192];
    char *host;
#endif

    smpd_enter_fn(FCNAME);

#ifndef HAVE_WINDOWS_H
    smpd_lock_smpd_data();
    if (smpd_get_smpd_data(SMPD_DYNAMIC_HOSTS_KEY, hosts, 8192) != SMPD_SUCCESS)
    {
	if (smpd_get_hostname(hosts, 8192) == 0)
	{
	    /* add this host to the dynamic_hosts key */
	    smpd_set_smpd_data(SMPD_DYNAMIC_HOSTS_KEY, hosts);
	    smpd_unlock_smpd_data();
	    smpd_exit_fn(FCNAME);
	    return SMPD_SUCCESS;
	}
	smpd_unlock_smpd_data();
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    smpd_unlock_smpd_data();

    if (smpd_get_hostname(myhostname, SMPD_MAX_HOST_LENGTH) != 0)
    {
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    /* check to see if the host is already in the key */
    host = strtok(hosts, " \t\r\n");
    while (host)
    {
	if (strcmp(host, myhostname) == 0)
	{
	    smpd_exit_fn(FCNAME);
	    return SMPD_SUCCESS;
	}
	host = strtok(NULL, " \t\r\n");
    }
    
    /* add this host to the dynamic_hosts key */
    smpd_lock_smpd_data();

    hosts[0] = '\0';
    if (smpd_get_smpd_data(SMPD_DYNAMIC_HOSTS_KEY, hosts, 8192) != SMPD_SUCCESS)
    {
	smpd_unlock_smpd_data();
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    if (strlen(hosts) > 0)
    {
	/* FIXME this could overflow */
	strcat(hosts, " ");
	strcat(hosts, myhostname);
    }
    else
    {
	strcpy(hosts, myhostname);
    }
    smpd_set_smpd_data(SMPD_DYNAMIC_HOSTS_KEY, hosts);
    smpd_unlock_smpd_data();
#endif
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_remove_from_dynamic_hosts"
int smpd_remove_from_dynamic_hosts(void)
{
#ifndef HAVE_WINDOWS_H
    char myhostname[SMPD_MAX_HOST_LENGTH];
    char hosts[8192];
    char hosts_less_me[8192];
    char *host;
#endif

    smpd_enter_fn(FCNAME);
#ifndef HAVE_WINDOWS_H
    if (smpd_get_hostname(myhostname, SMPD_MAX_HOST_LENGTH) != 0)
    {
	smpd_err_printf("smpd_get_hostname failed, errno = %d\n", errno);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    smpd_lock_smpd_data();

    hosts[0] = '\0';
    if (smpd_get_smpd_data(SMPD_DYNAMIC_HOSTS_KEY, hosts, 8192) != SMPD_SUCCESS)
    {
	smpd_unlock_smpd_data();
	smpd_dbg_printf("not removing host because "SMPD_DYNAMIC_HOSTS_KEY" does not exist\n");
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    /* create a new hosts lists without this host name in it */
    hosts_less_me[0] = '\0';
    host = strtok(hosts, " \t\r\n");
    while (host)
    {
	if (strcmp(host, myhostname))
	{
	    if (hosts_less_me[0] != '\0')
		strcat(hosts_less_me, " ");
	    strcat(hosts_less_me, host);
	}
	host = strtok(NULL, " \t\r\n");
    }

    if (hosts_less_me[0] == '\0')
    {
	smpd_dbg_printf("removing "SMPD_DYNAMIC_HOSTS_KEY"\n");
	smpd_delete_smpd_data(SMPD_DYNAMIC_HOSTS_KEY);
    }
    else
    {
	smpd_dbg_printf("setting new "SMPD_DYNAMIC_HOSTS_KEY": %s\n", hosts_less_me);
	smpd_set_smpd_data(SMPD_DYNAMIC_HOSTS_KEY, hosts_less_me);
    }
    smpd_unlock_smpd_data();
#endif
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}
