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


static char root_host[SMPD_MAX_HOST_LENGTH];
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

#ifndef HAVE_WINDOWS_H
static int writebuf(int fd, void *buffer, int length)
{
    unsigned char *buf;
    int num_written;
    
    buf = (unsigned char *)buffer;
    while (length)
    {
	num_written = write(fd, buf, length);
	if (num_written < 0)
	{
	    if (errno != EINTR)
	    {
		return errno;
	    }
	    num_written = 0;
	}
	buf = buf + num_written;
	length = length - num_written;
    }
    return 0;
}

static int readbuf(int fd, void *buffer, int length)
{
    unsigned char *buf;
    int num_read;

    buf = (unsigned char *)buffer;
    while (length)
    {
	num_read = read(fd, buf, length);
	if (num_read < 0)
	{
	    if (errno != EINTR)
	    {
		return errno;
	    }
	    num_read = 0;
	}
	else if (num_read == 0)
	{
	    return -1;
	}
	buf = buf + num_read;
	length = length - num_read;
    }
    return 0;
}
#endif

static int setup_stdin_redirection(smpd_process_t *process, SMPDU_Sock_set_t set)
{
    int result;
    smpd_context_t *context_in;
    SMPDU_SOCK_NATIVE_FD stdin_fd;
    SMPDU_Sock_t insock;
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
    result = SMPDU_Sock_native_to_sock(set, stdin_fd, NULL, &insock);
    if (result != SMPD_SUCCESS)
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
    SMPDU_Sock_set_user_ptr(insock, context_in);
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
    result = SMPDU_Sock_post_read(insock, context_in->read_cmd.cmd, 1, 1, NULL);
    if (result != SMPD_SUCCESS)
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
    SMPDU_Size_t num_written;

    switch (type)
    {
    case CTRL_LOGOFF_EVENT:
	break;
    case CTRL_C_EVENT:
    case CTRL_BREAK_EVENT:
    case CTRL_CLOSE_EVENT:
    case CTRL_SHUTDOWN_EVENT:
	if (smpd_process.timeout_sock != SMPDU_SOCK_INVALID_SOCK)
	{
	    if (first == 1)
	    {
		first = 0;
		SMPDU_Sock_write(smpd_process.timeout_sock, &ch, 1, &num_written);
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

static int ConnectToHost(char *host, int port, smpd_state_t state, SMPDU_Sock_set_t set, SMPDU_Sock_t *sockp, smpd_context_t **contextpp)
{
    int result;
    char error_msg[SMPD_MAX_ERROR_LEN];
    int len;

    /*printf("posting a connect to %s:%d\n", host, port);fflush(stdout);*/
    result = smpd_create_context(SMPD_CONTEXT_PMI, set, SMPDU_SOCK_INVALID_SOCK/**sockp*/, -1, contextpp);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("ConnectToHost failed: unable to create a context to connect to %s:%d with.\n", host, port);
	return SMPD_FAIL;
    }

    result = SMPDU_Sock_post_connect(set, *contextpp, host, port, sockp);
    if (result != SMPD_SUCCESS)
    {
	len = SMPD_MAX_ERROR_LEN;
	PMPI_Error_string(result, error_msg, &len);
	smpd_err_printf("ConnectToHost failed: unable to post a connect to %s:%d, error: %s\n", host, port, error_msg);
	return SMPD_FAIL;
    }

    (*contextpp)->sock = *sockp;
    (*contextpp)->state = state;

    result = smpd_enter_at_state(set, state);
    if (result != SMPD_SUCCESS)
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
static int start_pmi_server(int nproc, char *host, int len, int *port)
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

static int stop_pmi_server()
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

#ifdef HAVE_WINDOWS_H
    #define PROCESS_HANDLE_TYPE    HANDLE
    #define PROCESS_INVALID_HANDLE    NULL
    #define PROCESS_HANDLE_GET_PID(hnd)   (GetProcessId(hnd))
    #define PROCESS_HANDLE_SET_PID(hnd, pid)
#else
    #define PROCESS_HANDLE_TYPE    int
    #define PROCESS_INVALID_HANDLE    -1
    #define PROCESS_HANDLE_GET_PID(hnd)   (hnd)
    #define PROCESS_HANDLE_SET_PID(hnd, pid)    (hnd = pid)
#endif

/* Arg sent to the root smpd thread/process */
typedef struct mpiexec_rsh_rsmpd_args{
#ifdef HAVE_WINDOWS_H
    /* Event is signalled by root smpd thread after filling in reqd data */
    HANDLE hRootSMPDRdyEvent;
    int *pRootPort;
#else
    /* Pipe used by root smpd proc to write back the port/KVS name */
    int pipe_fd;
#endif
}mpiexec_rsh_rsmpd_args_t;

/* FIXME: Why is this func defined here ? 
 * - shouldn't this be in smpd_util*.lib ?
 */
static int root_smpd(void *p)
{
    int result;
    SMPDU_Sock_set_t set;
    SMPDU_Sock_t listener;
    mpiexec_rsh_rsmpd_args_t *pArgs = NULL;
    smpd_process_group_t *pg;
    int i, rootPort;
#ifndef HAVE_WINDOWS_H
    int send_kvs = 0;
    int pipe_fd;
#endif


    pArgs = (mpiexec_rsh_rsmpd_args_t *)p;
    if(!pArgs){
        smpd_err_printf("Invalid args - NULL - to root smpd\n");
        return SMPD_FAIL;
    }
    smpd_process.id = 1;
    smpd_process.root_smpd = SMPD_FALSE;
    smpd_process.map0to1 = SMPD_TRUE;

    result = SMPDU_Sock_create_set(&set);
    if (result != SMPD_SUCCESS){
	    smpd_err_printf(result, "SMPDU_Sock_create_set failed.\n");
	    return SMPD_FAIL;
    }
    smpd_process.set = set;
    smpd_dbg_printf("created a set for the listener: %d\n", SMPDU_Sock_get_sock_set_id(set));
    result = SMPDU_Sock_listen(set, NULL, &rootPort, &listener); 
    if (result != SMPD_SUCCESS){
	    smpd_err_printf(result, "SMPDU_Sock_listen failed.\n");
	    return SMPD_FAIL;
    }
    smpd_dbg_printf("smpd listening on port %d\n", rootPort);

    result = smpd_create_context(SMPD_CONTEXT_LISTENER, set, listener, -1, &smpd_process.listener_context);
    if (result != SMPD_SUCCESS){
	    smpd_err_printf("unable to create a context for the smpd listener.\n");
	    return SMPD_FAIL;
    }

    result = SMPDU_Sock_set_user_ptr(listener, smpd_process.listener_context);
    if (result != SMPD_SUCCESS){
	    smpd_err_printf(result, "SMPDU_Sock_set_user_ptr failed.\n");
	    return SMPD_FAIL;
    }
    smpd_process.listener_context->state = SMPD_SMPD_LISTENING;

    smpd_dbs_init();
    smpd_process.have_dbs = SMPD_TRUE;
    if (smpd_process.kvs_name[0] != '\0'){
	    result = smpd_dbs_create_name_in(smpd_process.kvs_name);
    }
    else{
	    result = smpd_dbs_create(smpd_process.kvs_name);
#ifndef HAVE_WINDOWS_H
	    send_kvs = 1;
#endif
    }
    if (result != SMPD_DBS_SUCCESS){
	    smpd_err_printf("unable to create a kvs database: name = <%s>.\n", smpd_process.kvs_name);
	    return SMPD_FAIL;
    }

    /* Set up the process group */
    /* initialize a new process group structure */
    pg = (smpd_process_group_t*)MPIU_Malloc(sizeof(smpd_process_group_t));
    if (pg == NULL){
	    smpd_err_printf("unable to allocate memory for a process group structure.\n");
	    return SMPD_FAIL;
    }

    pg->aborted = SMPD_FALSE;
    pg->any_init_received = SMPD_FALSE;
    pg->any_noinit_process_exited = SMPD_FALSE;
    strncpy(pg->kvs, smpd_process.kvs_name, SMPD_MAX_DBS_NAME_LEN);
    pg->num_procs = smpd_process.nproc;
    pg->processes = (smpd_exit_process_t*)MPIU_Malloc(smpd_process.nproc * sizeof(smpd_exit_process_t));
    if (pg->processes == NULL){
	    smpd_err_printf("unable to allocate an array of %d process exit structures.\n", smpd_process.nproc);
	    return SMPD_FAIL;
    }
    for (i=0; i<smpd_process.nproc; i++){
	    pg->processes[i].ctx_key[0] = '\0';
	    pg->processes[i].errmsg = NULL;
	    pg->processes[i].exitcode = -1;
	    pg->processes[i].exited = SMPD_FALSE;
	    pg->processes[i].finalize_called = SMPD_FALSE;
	    pg->processes[i].init_called = SMPD_FALSE;
	    pg->processes[i].node_id = i+1;
	    pg->processes[i].host[0] = '\0';
	    pg->processes[i].suspended = SMPD_FALSE;
	    pg->processes[i].suspend_cmd = NULL;
    }
    /* add the process group to the global list */
    pg->next = smpd_process.pg_list;
    smpd_process.pg_list = pg;

#ifdef HAVE_WINDOWS_H
    *(pArgs->pRootPort) = rootPort;
    SetEvent(pArgs->hRootSMPDRdyEvent);
#else
    if (p != NULL){
	    /*pipe_fd = *(int*)p;*/
        pipe_fd = pArgs->pipe_fd;
	    /* send the root port back over the pipe */
	    writebuf(pipe_fd, &rootPort, sizeof(int));
	    if (send_kvs){
	        writebuf(pipe_fd, smpd_process.kvs_name, SMPD_MAX_DBS_NAME_LEN);
	    }  
	    close(pipe_fd);
    }
#endif

    result = smpd_enter_at_state(set, SMPD_SMPD_LISTENING);
    if (result != SMPD_SUCCESS){
	    smpd_err_printf("root_smpd state machine failed.\n");
	    return SMPD_FAIL;
    }

    result = SMPDU_Sock_destroy_set(set);
    if (result != SMPD_SUCCESS){
	    smpd_err_printf("unable to destroy the set (result = %d)\n", result);
    }

    return SMPD_SUCCESS;
}

static int start_root_smpd(char *host, int len, int *port, PROCESS_HANDLE_TYPE *pHnd)
{
    mpiexec_rsh_rsmpd_args_t rsmpdArgs;
#ifdef HAVE_WINDOWS_H
    DWORD dwLength = len;
#else
    int pipe_fd[2];
    int result;
#endif
    if(!host){
        smpd_err_printf("Invalid host name - NULL - provided\n");
        return SMPD_FAIL;
    }
    if(len <= 0){
        smpd_err_printf("Invalid host name length (%d) provided\n", len);
        return SMPD_FAIL;
    }
    if(!port){
        smpd_err_printf("Invalid port - NULL - provided\n");
        return SMPD_FAIL;
    }
    if(!pHnd){
        smpd_err_printf("Invalid pointer to process handle - NULL - provided\n");
        return SMPD_FAIL;
    }

#ifdef HAVE_WINDOWS_H
    if(!(rsmpdArgs.hRootSMPDRdyEvent = CreateEvent(NULL, TRUE, FALSE, NULL))){
    	smpd_err_printf("unable to create the root listener synchronization event, error: %d\n", 
            GetLastError());
	    return SMPD_FAIL;
    }
    rsmpdArgs.pRootPort = port;
    /* FIXME: Do we need a generic handle type for smpd thread/proc ? */
    if(!(*pHnd = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)root_smpd, &rsmpdArgs, 0, NULL))){
	    smpd_err_printf("unable to create the root listener thread: error %d\n", GetLastError());
	    return SMPD_FAIL;
    }
    /* Wait for 1 minute for root smpd thread to start */
    if (WaitForSingleObject(rsmpdArgs.hRootSMPDRdyEvent, 60000) != WAIT_OBJECT_0){
	    smpd_err_printf("the root process thread failed to initialize.\n");
	    return SMPD_FAIL;
    }
    CloseHandle(rsmpdArgs.hRootSMPDRdyEvent);
    /*GetComputerName(host, &dwLength);*/
    GetComputerNameEx(ComputerNameDnsFullyQualified, host, &dwLength);
#else
    pipe(pipe_fd);
    result = fork();
    if (result == -1){
	    smpd_err_printf("unable to fork the root listener, errno %d\n", errno);
	    return SMPD_FAIL;
    }
    if (result == 0){
        /* child process */
	    close(pipe_fd[0]); /* close the read end of the pipe */
        rsmpdArgs.pipe_fd = pipe_fd[1];
	    result = root_smpd(&rsmpdArgs);
	    exit(result);
    }
    /* parent process */
    PROCESS_HANDLE_SET_PID(*pHnd, result);

    /* close the write end of the pipe */
    close(pipe_fd[1]);
    /* read the port from the root_smpd process */
    readbuf(pipe_fd[0], port, sizeof(int));
    /* read the kvs name */
    readbuf(pipe_fd[0], smpd_process.kvs_name, SMPD_MAX_DBS_NAME_LEN);
    /* close the read end of the pipe */
    close(pipe_fd[0]);
    gethostname(host, len);
#endif
    return SMPD_SUCCESS;
}

static int stop_root_smpd(PROCESS_HANDLE_TYPE hnd)
{
#ifdef HAVE_WINDOWS_H
    DWORD result;
#else
    int status;
#endif

#ifdef HAVE_WINDOWS_H
    result = WaitForSingleObject(hnd, INFINITE);
    if (result != WAIT_OBJECT_0){
        smpd_err_printf("Error waiting for root smpd to stop\n");
	    return SMPD_FAIL;
    }
#else
    kill(PROCESS_HANDLE_GET_PID(hnd), SIGKILL);
    /*
    if (waitpid(PROCESS_HANDLE_GET_PID(hnd), &status, WUNTRACED) == -1)
    {
	return SMPD_FAIL;
    }
    */
#endif
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
    SMPDU_Sock_set_t set;
    SMPD_BOOL escape_escape = SMPD_TRUE;
    char *env_str;
    int maxlen;
    SMPDU_Sock_t abort_sock;
    smpd_context_t *abort_context = NULL;
    smpd_command_t *cmd_ptr;
    PROCESS_HANDLE_TYPE hnd;

    smpd_enter_fn("mpiexec_rsh");

#ifdef HAVE_WINDOWS_H
    SetConsoleCtrlHandler(mpiexec_rsh_handler, TRUE);
#else
    /* setup a signall hander? */
#endif

    p = getenv("MPIEXEC_RSH");
    if (p != NULL && strlen(p) > 0){
	    strncpy(ssh_cmd, p, 100);
    }

    p = getenv("MPIEXEC_RSH_NO_ESCAPE");
    if (p != NULL){
	    if (smpd_is_affirmative(p) || strcmp(p, "1") == 0){
	        escape_escape = SMPD_FALSE;
	    }
    }

    result = SMPDU_Sock_create_set(&set);
    if (result != SMPD_SUCCESS){
	    smpd_err_printf("unable to create a set for the mpiexec_rsh.\n");
	    smpd_exit_fn("mpiexec_rsh");
	    return SMPD_FAIL;
    }

    smpd_process.nproc = smpd_process.launch_list->nproc;

    if (smpd_process.use_pmi_server){
	    result = start_pmi_server(smpd_process.launch_list->nproc, root_host, 100, &root_port);
	    if (result != SMPD_SUCCESS){
	        smpd_err_printf("mpiexec_rsh is unable to start the local pmi server.\n");
	        smpd_exit_fn("mpiexec_rsh");
	        return SMPD_FAIL;
	    }
	    smpd_dbg_printf("the pmi server is listening on %s:%d\n", root_host, root_port);
    }
    else{
	    /* start the root smpd */
	    result = start_root_smpd(root_host, SMPD_MAX_HOST_LENGTH, &root_port, &hnd);
	    if (result != SMPD_SUCCESS){
		    smpd_err_printf("mpiexec_rsh is unable to start the root smpd.\n");
		    smpd_exit_fn("mpiexec_rsh");
		    return SMPD_FAIL;
	    }
	    smpd_dbg_printf("the root smpd is listening on %s:%d\n", root_host, root_port);

	    /* create a connection to the root smpd used to abort the job */
	    result = ConnectToHost(root_host, root_port, SMPD_CONNECTING_RPMI, set, &abort_sock, &abort_context);
	    if (result != SMPD_SUCCESS){
	        smpd_exit_fn("mpiexec_rsh");
	        return SMPD_FAIL;
	    }
    }

    processes = (smpd_process_t**)MPIU_Malloc(sizeof(smpd_process_t*) * smpd_process.launch_list->nproc);
    if (processes == NULL){
	    smpd_err_printf("unable to allocate process array.\n");
	    smpd_exit_fn("mpiexec_rsh");
	    return SMPD_FAIL;
    }

    launch_node_ptr = smpd_process.launch_list;
    for (i=0; i<smpd_process.launch_list->nproc; i++){
	    if (launch_node_ptr == NULL){
		    smpd_err_printf("Error: not enough launch nodes.\n");
		    smpd_exit_fn("mpiexec_rsh");
		    return SMPD_FAIL;
	    }

	    /* initialize process structure */
	    result = smpd_create_process_struct(i, &process);
	    if (result != SMPD_SUCCESS){
		    smpd_err_printf("unable to create a process structure.\n");
		    smpd_exit_fn("mpiexec_rsh");
		    return SMPD_FAIL;
	    }
	    /* no need for a pmi context */
	    if (process->pmi){
		    smpd_free_context(process->pmi);
        }
	    process->pmi = NULL;
	    /* change stdout and stderr to rsh behavior: 
	     * write stdout/err directly to stdout/err instead of creating
	     * an smpd stdout/err command 
	     */
	    if (process->out != NULL){
	        process->out->type = SMPD_CONTEXT_STDOUT_RSH;
	    }
	    if (process->err != NULL){
	        process->err->type = SMPD_CONTEXT_STDERR_RSH;
	    }
	    MPIU_Strncpy(process->clique, launch_node_ptr->clique, SMPD_MAX_CLIQUE_LENGTH);
	    MPIU_Strncpy(process->dir, launch_node_ptr->dir, SMPD_MAX_DIR_LENGTH);
	    MPIU_Strncpy(process->domain_name, smpd_process.kvs_name, SMPD_MAX_DBS_NAME_LEN);
	    MPIU_Strncpy(process->env, launch_node_ptr->env, SMPD_MAX_ENV_LENGTH);
	    if (escape_escape == SMPD_TRUE && smpd_process.mpiexec_run_local != SMPD_TRUE){
		    /* convert \ to \\ to make cygwin ssh happy */
		    iter1 = launch_node_ptr->exe;
		    iter2 = exe;
		    while (*iter1){
			    if (*iter1 == '\\'){
				    *iter2 = *iter1;
				    iter2++;
				    *iter2 = *iter1;
			    }
			    else{
				    *iter2 = *iter1;
			    }
			    iter1++;
			    iter2++;
		    }
		    *iter2 = '\0';
		    /*printf("[%s] -> [%s]\n", launch_node_ptr->exe, exe);*/
	    }
	    else{
	        MPIU_Strncpy(exe, launch_node_ptr->exe, SMPD_MAX_EXE_LENGTH);
	    }

	    /* Two samples for testing on the local machine */

	    /* static rPMI initialization */
	    /*sprintf(process->exe, "env PMI_RANK=%d PMI_SIZE=%d PMI_KVS=%s PMI_ROOT_HOST=%s PMI_ROOT_PORT=8888 PMI_ROOT_LOCAL=1 PMI_APPNUM=%d %s",
		    launch_node_ptr->iproc, launch_node_ptr->nproc, smpd_process.kvs_name, root_host, launch_node_ptr->appnum, exe);*/

	    /* dynamic rPMI initialization */
	    /*sprintf(process->exe, "env PMI_RANK=%d PMI_SIZE=%d PMI_KVS=%s PMI_ROOT_HOST=%s PMI_ROOT_PORT=%d PMI_ROOT_LOCAL=0 PMI_APPNUM=%d %s",
		    launch_node_ptr->iproc, launch_node_ptr->nproc, smpd_process.kvs_name, root_host, root_port, launch_node_ptr->appnum, exe);*/

	    if (smpd_process.mpiexec_run_local == SMPD_TRUE){
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
	    else{
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
	    if (result != SMPD_SUCCESS){
		    smpd_err_printf("unable to launch process %d <%s>.\n", i, process->exe);
		    smpd_exit_fn("mpiexec_rsh");
		    return SMPD_FAIL;
	    }
	    /* save the new process in the list */
	    process->next = smpd_process.process_list;
	    smpd_process.process_list = process;
	    if (i == 0){
		    /* start the stdin redirection thread to the first process */
		    setup_stdin_redirection(process, set);
	    }

	    smpd_process.nproc_launched++;
	    processes[i] = process;
	    launch_node_ptr = launch_node_ptr->next;
    } /* for (i=0; i<smpd_process.launch_list->nproc; i++) */
    
    if (launch_node_ptr != NULL){
	    smpd_err_printf("Error: too many launch nodes.\n");
	    smpd_exit_fn("mpiexec_rsh");
	    return SMPD_FAIL;
    }

    /* Start the timeout mechanism if specified */
    if (smpd_process.timeout > 0){
	    smpd_context_t *reader_context;
	    SMPDU_Sock_t sock_reader;
	    SMPDU_SOCK_NATIVE_FD reader, writer;
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
	    result = SMPDU_Sock_native_to_sock(set, reader, NULL, &sock_reader);
	    result = SMPDU_Sock_native_to_sock(set, writer, NULL, &smpd_process.timeout_sock);
	    result = smpd_create_context(SMPD_CONTEXT_TIMEOUT, set, sock_reader, -1, &reader_context);
	    reader_context->read_state = SMPD_READING_TIMEOUT;
	    result = SMPDU_Sock_post_read(sock_reader, &reader_context->read_cmd.cmd, 1, 1, NULL);
#ifdef HAVE_WINDOWS_H
	    /* create a Windows thread to sleep until the timeout expires */
	    smpd_process.timeout_thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)timeout_thread, NULL, 0, NULL);
	    if (smpd_process.timeout_thread == NULL){
		    printf("Error: unable to create a timeout thread, errno %d.\n", GetLastError());
		    smpd_exit_fn("mp_parse_command_args");
		    return SMPD_FAIL;
	    }
#else /* HAVE_WINDOWS_H */
#ifdef SIGALRM
	    /* create an alarm to signal mpiexec when the timeout expires */
	    smpd_signal(SIGALRM, timeout_function);
	    alarm(smpd_process.timeout);
#else /* SIGALARM */
#ifdef HAVE_PTHREAD_H
	    /* create a pthread to sleep until the timeout expires */
	    result = pthread_create(&smpd_process.timeout_thread, NULL, timeout_thread, NULL);
	    if (result != 0){
		    printf("Error: unable to create a timeout thread, errno %d.\n", result);
		    smpd_exit_fn("mp_parse_command_args");
		    return SMPD_FAIL;
	    }
#else /* HAVE_PTHREAD_H */
	/* no timeout mechanism available */
#endif /* HAVE_PTHREAD_H */
#endif /* SIGALARM */
#endif /* HAVE_WINDOWS_H */
    } /* if (smpd_process.timeout > 0) */

    result = smpd_enter_at_state(set, SMPD_IDLE);
    if (result != SMPD_SUCCESS){
	    smpd_err_printf("mpiexec_rsh state machine failed.\n");
	    smpd_exit_fn("mpiexec_rsh");
	    return SMPD_FAIL;
    }

    if (smpd_process.use_pmi_server){
	    result = stop_pmi_server();
	    if (result != SMPD_SUCCESS){
		    smpd_err_printf("mpiexec_rsh unable to stop the pmi server.\n");
		    smpd_exit_fn("mpiexec_rsh");
		    return SMPD_FAIL;
	    }
    }
    else{
	    /* Send an abort command to the root_smpd thread/process to insure that it exits.
	     * This only needs to be sent when there is an error or failed process of some sort
	     * but it is safe to send it in all cases.
	     */
	    result = smpd_create_command("abort", 0, 0, SMPD_FALSE, &cmd_ptr);
	    if (result != SMPD_SUCCESS){
		    smpd_err_printf("unable to create an abort command.\n");
		    smpd_exit_fn("mpiexec_rsh");
		    return SMPD_FAIL;
	    }
	    result = smpd_post_write_command(abort_context, cmd_ptr);
	    if (result != SMPD_SUCCESS){
		    /* Only print this as a debug message instead of an error because the root_smpd thread/process may have already exited. */
		    smpd_dbg_printf("unable to post a write of the abort command to the %s context.\n", smpd_get_context_str(abort_context));
		    smpd_exit_fn("mpiexec_rsh");
		    return SMPD_FAIL;
	    }

	    result = stop_root_smpd(hnd);
	    if (result != PMI_SUCCESS){
		    smpd_err_printf("mpiexec_rsh unable to stop the root smpd.\n");
		    smpd_exit_fn("mpiexec_rsh");
		    return SMPD_FAIL;
	    }
    }

    smpd_exit_fn("mpiexec_rsh");
    return 0;
}
