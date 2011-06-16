/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "ipmi.h"
#ifdef HAVE_CTYPE_H
#include <ctype.h>
#endif

/* pmiimpl.h */

static int root_smpd(void *p);

/* Define to prevent an smpd root thread or process from being created when 
   there is only one process. */
/* Currently, defining this prevents the use of the spawn command. */
/*#define SINGLE_PROCESS_OPTIMIZATION*/

#define PMI_MAX_KEY_LEN          256
#define PMI_MAX_VALUE_LEN        8192
#define PMI_MAX_KVS_NAME_LENGTH  100
#define PMI_MAX_HOST_NAME_LENGTH 100
#define PMI_MAX_STR_VAL_LENGTH   100

#ifdef HAVE_WINDOWS_H
    #define PMII_PROCESS_HANDLE_TYPE    HANDLE
    #define PMII_PROCESS_INVALID_HANDLE    NULL
#else
    #define PMII_PROCESS_HANDLE_TYPE    int
    #define PMII_PROCESS_INVALID_HANDLE    -1
#endif

typedef enum {PMI_UNINITIALIZED,
                PMI_SINGLETON_INIT_BUT_NO_PM,
                PMI_SINGLETON_INIT_WITH_PM, 
                PMI_INITIALIZED,
                PMI_FINALIZED} PMIState;

/*
#define PMI_INITIALIZED 0
#define PMI_FINALIZED   1
*/

#define PMI_TRUE        1
#define PMI_FALSE       0

typedef struct pmi_process_t
{
    int rpmi;
#ifdef HAVE_WINDOWS_H
    HANDLE hRootThread;
    HANDLE hRootThreadReadyEvent;
#else
    int root_pid;
#endif
    char root_host[100];
    int root_port;
    int local_kvs;
    char kvs_name[PMI_MAX_KVS_NAME_LENGTH];
    char domain_name[PMI_MAX_KVS_NAME_LENGTH];
    SMPDU_Sock_t sock;
    SMPDU_Sock_set_t set;
    int iproc;
    int nproc;
    PMIState init_finalized;
    int smpd_id;
    SMPDU_SOCK_NATIVE_FD smpd_fd;
    int smpd_key;
    smpd_context_t *context;
    int clique_size;
    int *clique_ranks;
    char host[PMI_MAX_HOST_NAME_LENGTH];
    int port;
    int appnum;
    PMII_PROCESS_HANDLE_TYPE singleton_mpiexec_fd;
    char kvs_name_singleton_nopm[PMI_MAX_KVS_NAME_LENGTH];
} pmi_process_t;

/* global variables */
static pmi_process_t pmi_process =
{
    PMI_FALSE,           /* rpmi           */
#ifdef HAVE_WINDOWS_H
    NULL,                /* root thread    */
    NULL,                /* hRootThreadReadyEvent */
#else
    0,                   /* root pid       */
#endif
    "",                  /* root host      */
    0,                   /* root port      */
    PMI_FALSE,           /* local_kvs      */
    "",                  /* kvs_name       */
    "",                  /* domain_name    */
    SMPDU_SOCK_INVALID_SOCK,  /* sock           */
    SMPDU_SOCK_INVALID_SET,   /* set            */
    -1,                  /* iproc          */
    -1,                  /* nproc          */
    PMI_UNINITIALIZED,       /* init_finalized */
    -1,                  /* smpd_id        */
    0,                   /* smpd_fd        */
    0,                   /* smpd_key       */
    NULL,                /* context        */
    0,                   /* clique_size    */
    NULL,                /* clique_ranks   */
    "",                  /* host           */
    -1,                  /* port           */
    0,                    /* appnum         */
    PMII_PROCESS_INVALID_HANDLE, /* singleton mpiexec proc handle/pid */
    ""                      /* kvs_name of singleton proc with no PM */
};


void pmi_init_printf(void)
{
    char *env;
    
    env = getenv("SMPD_DBG_OUTPUT");
    if(env != NULL){
        /* We only support tracing for now */
        smpd_process.verbose = SMPD_TRUE;
        smpd_process.dbg_state |= SMPD_DBG_STATE_ERROUT | SMPD_DBG_STATE_STDOUT | SMPD_DBG_STATE_TRACE;
    }
}

static int silence = 0;
static int pmi_err_printf(char *str, ...)
{
    int n=0;
    va_list list;

    if (!silence)
    {
	printf("[%d] ", pmi_process.iproc);
	va_start(list, str);
	n = vprintf(str, list);
	va_end(list);

	fflush(stdout);
    }

    return n;
}

#ifdef PMII_DEBUG_
static int pmi_dbg_printf(char *str, ...)
{
    int n=0;
    va_list list;

	printf("[%d] ", pmi_process.iproc);
	va_start(list, str);
	n = vprintf(str, list);
	va_end(list);
	fflush(stdout);

    return n;
}
#else
#   define pmi_dbg_printf(...)  1
#endif

static int pmi_mpi_err_printf(int mpi_errno, char *fmt, ... )
{
    int n;
    va_list list;

    /* convert the error code to a string */
    printf("mpi_errno: %d\n", mpi_errno);

    printf("[%d] ", pmi_process.iproc);
    va_start(list, fmt);
    n = vprintf(fmt, list);
    va_end(list);

    fflush(stdout);

    MPIR_Err_return_comm(NULL, "", mpi_errno);

    return n;
}

static int pmi_create_post_command(const char *command, const char *name, const char *key, const char *value)
{
    int result;
    smpd_command_t *cmd_ptr;
    int dest = 1;
    int add_id = 0;

    if (!pmi_process.rpmi)
    {
	if (strcmp(command, "done") == 0)
	{
	    /* done commands go to the immediate smpd, not the root */
	    dest = pmi_process.smpd_id;
	}
    }
    if ((strcmp(command, "init") == 0) || (strcmp(command, "finalize") == 0))
    {
	add_id = 1;
	dest = 0;
    }

    result = smpd_create_command((char*)command, pmi_process.smpd_id, dest, SMPD_TRUE, &cmd_ptr);
    if (result != SMPD_SUCCESS)
    {
	pmi_err_printf("unable to create a %s command.\n", command);
	return PMI_FAIL;
    }
    result = smpd_add_command_int_arg(cmd_ptr, "ctx_key", pmi_process.smpd_key);
    if (result != SMPD_SUCCESS)
    {
	pmi_err_printf("unable to add the key to the %s command.\n", command);
	return PMI_FAIL;
    }

    if (name != NULL)
    {
	result = smpd_add_command_arg(cmd_ptr, "name", (char*)name);
	if (result != SMPD_SUCCESS)
	{
	    pmi_err_printf("unable to add the kvs name('%s') to the %s command.\n", name, command);
	    return PMI_FAIL;
	}
    }

    if (key != NULL)
    {
	result = smpd_add_command_arg(cmd_ptr, "key", (char*)key);
	if (result != SMPD_SUCCESS)
	{
	    pmi_err_printf("unable to add the key('%s') to the %s command.\n", key, command);
	    return PMI_FAIL;
	}
    }

    if (value != NULL)
    {
	result = smpd_add_command_arg(cmd_ptr, "value", (char*)value);
	if (result != SMPD_SUCCESS)
	{
	    pmi_err_printf("unable to add the value('%s') to the %s command.\n", value, command);
	    return PMI_FAIL;
	}
    }

    if (add_id)
    {
	result = smpd_add_command_int_arg(cmd_ptr, "node_id", pmi_process.smpd_id);
	if (result != SMPD_SUCCESS)
	{
	    pmi_err_printf("unable to add the node_id(%d) to the %s command.\n", pmi_process.smpd_id, command);
	    return PMI_FAIL;
	}
    }

    /* post the write of the command */
    /*
    printf("posting write of dbs command to %s context, sock %d: '%s'\n",
	smpd_get_context_str(pmi_process.context), SMPDU_Sock_getid(pmi_process.context->sock), cmd_ptr->cmd);
    fflush(stdout);
    */
    /* If proc_info command add the proc_info args */
    if(strcmp(command, "proc_info") == 0){
        /* FIXME - Send the actual exe name */
        result = smpd_add_command_arg(cmd_ptr, "c", "singleton_client");
        if(result != SMPD_SUCCESS){
            smpd_err_printf("Unable to add executable name to 'proc_info' cmd\n");
        }
        result = smpd_add_command_int_arg(cmd_ptr, "i", pmi_process.iproc);
        if(result != SMPD_SUCCESS){
            smpd_err_printf("Unable to add rank to 'proc_info' cmd\n");        
        }
        result = smpd_add_command_int_arg(cmd_ptr, "n", pmi_process.nproc);
        if(result != SMPD_SUCCESS){
            smpd_err_printf("Unable to add nprocs to 'proc_info' cmd\n");
        }
        result = smpd_add_command_int_arg(cmd_ptr, "s", smpd_process.is_singleton_client ? 1 : 0);
        if(result != SMPD_SUCCESS){
            smpd_err_printf("Unable to add 'is_singleton_client' to 'proc_info' cmd\n");
        }
#ifndef HAVE_WINDOWS_H
        /* For non-windows systems send the PID in 'proc_info' */
        /* FIXME: Can we send a pid_t as an int ? */
        result = smpd_add_command_int_arg(cmd_ptr, "p", getpid());
        if(result != SMPD_SUCCESS){
            smpd_err_printf("Unable to add PID to 'proc_info' cmd \n");
        }
#endif
    }

    result = smpd_post_write_command(pmi_process.context, cmd_ptr);
    if (result != SMPD_SUCCESS)
    {
	pmi_err_printf("unable to post a write of the %s command.\n", command);
	return PMI_FAIL;
    }
    if (strcmp(command, "done"))
    {
	/* and post a read for the result if it is not a done command */
	result = smpd_post_read_command(pmi_process.context);
	if (result != SMPD_SUCCESS)
	{
	    pmi_err_printf("unable to post a read of the next command on the pmi context.\n");
	    return PMI_FAIL;
	}
    }

    /* let the state machine send the command and receive the result */
    result = smpd_enter_at_state(pmi_process.set, SMPD_WRITING_CMD);
    if (result != SMPD_SUCCESS)
    {
	pmi_err_printf("the state machine logic failed to get the result of the %s command.\n", command);
	return PMI_FAIL;
    }
    return PMI_SUCCESS;
}

static int uPMI_ConnectToHost(char *host, int port, smpd_state_t state)
{
    int result;
    char error_msg[MPI_MAX_ERROR_STRING];
    int len;

    /* Make sure that we have the smpd passphrase before connecting to PM */
    if (smpd_process.passphrase[0] == '\0'){
        smpd_get_smpd_data("phrase", smpd_process.passphrase, SMPD_PASSPHRASE_MAX_LENGTH);
    }
    if (smpd_process.passphrase[0] == '\0'){
        if (smpd_process.noprompt){
            pmi_err_printf("Error: No smpd passphrase specified through the registry or .smpd file, exiting.\n");
            return PMI_FAIL;
        }
        else{
            printf("Please specify an authentication passphrase for smpd: "); fflush(stdout);
            smpd_get_password(smpd_process.passphrase);
        }
    }

    /*printf("posting a connect to %s:%d\n", host, port);fflush(stdout);*/
    result = smpd_create_context(SMPD_CONTEXT_PMI, pmi_process.set, SMPDU_SOCK_INVALID_SOCK/*pmi_process.sock*/, smpd_process.id, &pmi_process.context);
    if (result != SMPD_SUCCESS)
    {
	pmi_err_printf("PMI_ConnectToHost failed: unable to create a context to connect to %s:%d with.\n", host, port);
	return PMI_FAIL;
    }

    result = SMPDU_Sock_post_connect(pmi_process.set, pmi_process.context, host, port, &pmi_process.sock);
    if (result != SMPD_SUCCESS)
    {
	printf("SMPDU_Sock_post_connect failed.\n");fflush(stdout);
	len = MPI_MAX_ERROR_STRING;
	PMPI_Error_string(result, error_msg, &len);
	pmi_err_printf("PMI_ConnectToHost failed: unable to post a connect to %s:%d, error: %s\n", host, port, error_msg);
	printf("uPMI_ConnectToHost returning PMI_FAIL\n");fflush(stdout);
	return PMI_FAIL;
    }

    pmi_process.context->sock = pmi_process.sock;
    pmi_process.context->state = state;

    result = smpd_enter_at_state(pmi_process.set, state);
    if (result != MPI_SUCCESS)
    {
	pmi_mpi_err_printf(result, "PMI_ConnectToHost failed: unable to connect to %s:%d.\n", host, port);
	return PMI_FAIL;
    }

    if (state == SMPD_CONNECTING_RPMI)
    {
	/* remote pmi processes receive their smpd_key when they connect to the smpd pmi server */
	pmi_process.smpd_key = atoi(pmi_process.context->session);
    }

    return SMPD_SUCCESS;
}

static int pmi_create_localKVS(void ){
    /* Its ok to init here since we can only have one local db */
	if (smpd_dbs_init() != SMPD_DBS_SUCCESS){
	    pmi_err_printf("unable to initialize the local dbs engine.\n");
	    return PMI_FAIL;
	}

	if (smpd_dbs_create(pmi_process.kvs_name) != SMPD_DBS_SUCCESS){
	    pmi_err_printf("unable to create the process group kvs\n");
	    return PMI_FAIL;
	}
    /* smpd_process.domain_name is created in smpd_dbs_init() */
	MPIU_Strncpy(pmi_process.domain_name, smpd_process.domain_name, 
        PMI_MAX_KVS_NAME_LENGTH);
	pmi_process.local_kvs = PMI_TRUE;
    return PMI_SUCCESS;
}

static int pmi_destroy_localKVS(void ){
    /* Its ok to finalize here since we can only have one local db */
    if(smpd_dbs_finalize() != SMPD_DBS_SUCCESS){
        pmi_err_printf("unable to finalize the local dbs engine.\n");
        return PMI_FAIL;
    }
    pmi_process.local_kvs = PMI_FALSE;
    return PMI_SUCCESS;
}

/* FIXME : Currently only used for singleton init -- mostly only one
 * pair of (key, val) . Inefficient for large number of (key,val)s
 */

static int pmi_rsync_localKVS(const char *localKVSName,
        const char *remoteKVSName){
    smpd_dbsIter_t localKVSIter;
    char key[SMPD_MAX_DBS_KEY_LEN], value[SMPD_MAX_DBS_VALUE_LEN];
    if(smpd_dbsIter_init(localKVSName, &localKVSIter) !=
        SMPD_DBS_SUCCESS){
        pmi_err_printf("Error initializing local KVS iterator\n");
        return PMI_FAIL;
    }
    while(smpd_dbs_hasMoreKeys(localKVSIter)){
        if(smpd_dbs_getNextKeyVal(&localKVSIter, key, value)
            != SMPD_DBS_SUCCESS){
            smpd_err_printf("Error reading key/val from localKVS\n");
            return PMI_FAIL;
        }
        if(PMI_KVS_Put(remoteKVSName, key, value) != PMI_SUCCESS){
            smpd_err_printf("Error syncing localKVS to remoteKVS\n");
            return PMI_FAIL;
        }
    }
    smpd_dbsIter_finalize(&localKVSIter);
    return PMI_SUCCESS;
}

/* Launch an instance of mpiexec which will connect to SMPD and start a PMI service.
 * This instance of mpiexec will connect back using the portNo specified in the "-port" option
 * and provide info about the new PMI service.
 */
static PMII_PROCESS_HANDLE_TYPE launch_mpiexec_process(int portNo){
#ifdef HAVE_WINDOWS_H
#define PMII_MAX_MPIEXEC_CMD_STR_LENGTH 100
    char progName[PMII_MAX_MPIEXEC_CMD_STR_LENGTH];
    STARTUPINFO sInfo;
    PROCESS_INFORMATION pInfo = { 0 };
    ZeroMemory(&sInfo, sizeof(sInfo));
    sInfo.cb = sizeof(sInfo);
    ZeroMemory(&pInfo, sizeof(pInfo));
    snprintf(progName, PMII_MAX_MPIEXEC_CMD_STR_LENGTH, 
                "mpiexec -pmiserver 1 -port %d -hide_console", portNo);
    if(!CreateProcess(NULL, progName, NULL, NULL, TRUE, 
                        NORMAL_PRIORITY_CLASS | CREATE_NO_WINDOW, NULL, NULL, &sInfo, &pInfo)){
        pmi_err_printf("Error creating mpiexec process...%d\n", GetLastError());
        pmi_err_printf("This singleton init program tried to access a feature which requires PM support\n");
        pmi_err_printf("eg: spawn, universe_size etc\n");
        pmi_err_printf("The program failed because mpiexec could not be located\n");
        return PMII_PROCESS_INVALID_HANDLE;
    }
    return pInfo.hProcess;
#else
#define PMII_MPIEXEC_CMDLINE_ARGV_SIZE 6
    int pid, rc;
    char *mpiexecArgv[PMII_MPIEXEC_CMDLINE_ARGV_SIZE];
	char port[16];
    pid = fork();
    if(pid < 0){
        pmi_err_printf("Error creating mpiexec process...\n");
        return PMII_PROCESS_INVALID_HANDLE;
    }
    else if(pid == 0){
		MPIU_Snprintf(port, sizeof(port), "%d", portNo);
        mpiexecArgv[0] = "mpiexec";
        mpiexecArgv[1] = "-pmiserver";
        mpiexecArgv[2] = "1";
        mpiexecArgv[3] = "-port";
        mpiexecArgv[4] = port;
        mpiexecArgv[5] = NULL;
        rc = execvp(mpiexecArgv[0], mpiexecArgv);
        pmi_err_printf("Error Singinit execv'ing mpiexec failed\n");
        pmi_err_printf("This singleton init program tried to access a feature which requires PM support\n");
        pmi_err_printf("eg: spawn, universe_size etc\n");
        pmi_err_printf("The program failed because mpiexec could not be located\n");
        exit(-1);
    }
    else{
        return pid;
    }
#endif
}

#define PMII_ERR_SETPRINTANDJUMP(msg, errcode) { pmi_err_printf("%s", msg); retval = errcode; goto fn_fail; }
#define PMII_MAX_ERR_MSG_LENGTH     100

static int PMIi_InitSingleton(void ){
    SMPDU_Sock_set_t singleton_client_set;
    SMPDU_Sock_t singleton_client_sock;
    smpd_context_t *p_singleton_context=NULL;
    char err_msg[PMII_MAX_ERR_MSG_LENGTH];
    int singleton_client_lport;
    int result, retval = PMI_SUCCESS;
    char rank_str[PMI_MAX_STR_VAL_LENGTH], size_str[PMI_MAX_STR_VAL_LENGTH];
    char str[PMI_MAX_STR_VAL_LENGTH];

    /* Enable singleton_init state machine tracing */
    /*
    smpd_process.verbose = SMPD_TRUE;
	smpd_process.dbg_state |= SMPD_DBG_STATE_ERROUT | SMPD_DBG_STATE_STDOUT | SMPD_DBG_STATE_TRACE;
    */

    result = SMPDU_Sock_create_set(&singleton_client_set);
    if(result != SMPD_SUCCESS){
        MPIU_Snprintf(err_msg, PMII_MAX_ERR_MSG_LENGTH, "SMPDU_Sock_create_set failed: unable to create a sock set, error: %d\n", result);
	    PMII_ERR_SETPRINTANDJUMP(err_msg, result);
    }

    /* Assign an ephemeral port */
    singleton_client_lport = 0;
    result = SMPDU_Sock_listen(singleton_client_set, NULL, &singleton_client_lport, &singleton_client_sock);
    if (result != SMPD_SUCCESS){
        MPIU_Snprintf(err_msg, PMII_MAX_ERR_MSG_LENGTH, "SMPDU_Sock_listen failed,\nsock error: %s\n", get_sock_error_string(result));
	    PMII_ERR_SETPRINTANDJUMP(err_msg, result);
    }

    result = smpd_create_context(SMPD_CONTEXT_SINGLETON_INIT_CLIENT, singleton_client_set, singleton_client_sock,
                                     -1, &p_singleton_context);
    if (result != SMPD_SUCCESS){
        MPIU_Snprintf(err_msg, PMII_MAX_ERR_MSG_LENGTH, "smpd_create_context failed, error = %d\n", result);
	    PMII_ERR_SETPRINTANDJUMP(err_msg, result);
    }

    result = SMPDU_Sock_set_user_ptr(singleton_client_sock, p_singleton_context);
    if (result != SMPD_SUCCESS){
        MPIU_Snprintf(err_msg, PMII_MAX_ERR_MSG_LENGTH, "SMPDU_Sock_set_user_ptr failed,\nsock error: %s\n", get_sock_error_string(result));
	    PMII_ERR_SETPRINTANDJUMP(err_msg, result);
    }

    p_singleton_context->state = SMPD_SINGLETON_CLIENT_LISTENING;

    /* Create an instance of mpiexec that will connect back and give us information about the PM to connect to */
    pmi_process.singleton_mpiexec_fd = launch_mpiexec_process(singleton_client_lport);
    if(pmi_process.singleton_mpiexec_fd == PMII_PROCESS_INVALID_HANDLE){
        result = -1;
        MPIU_Snprintf(err_msg, PMII_MAX_ERR_MSG_LENGTH, "launchMpiexecProcess failed\n");
	    PMII_ERR_SETPRINTANDJUMP(err_msg, result);
    }
    /* FIXME: Switch to PMI v2 to recognize non-MPICH2 mpiexecs */
    /* SMPD state machine will accept connection from mpiexec & get information about the PM */
    result = smpd_enter_at_state(singleton_client_set, SMPD_SINGLETON_CLIENT_LISTENING);
    if (result != SMPD_SUCCESS) {
        MPIU_Snprintf(err_msg, PMII_MAX_ERR_MSG_LENGTH, "smpd state machine failed, error = %d\n", result);
	    PMII_ERR_SETPRINTANDJUMP(err_msg, result);
    }
    /* SMPD state machine has set the PMI info for smpd_process */
    /* Now we have PMI_KVS, PMI_HOST and PMI_PORT info */
	if ((smpd_process.port > 0) && 
        (strlen(smpd_process.host) > 0) &&
        (strlen(smpd_process.kvs_name) > 0)){
        /* Save the current local KVS name */
        MPIU_Strncpy(pmi_process.kvs_name_singleton_nopm, pmi_process.kvs_name, PMI_MAX_KVS_NAME_LENGTH);
        /* Update the pmi process structs with the new remote KVS info */
        MPIU_Strncpy(pmi_process.kvs_name, smpd_process.kvs_name, PMI_MAX_KVS_NAME_LENGTH);
        MPIU_Strncpy(pmi_process.domain_name, smpd_process.domain_name, PMI_MAX_KVS_NAME_LENGTH);
        MPIU_Strncpy(pmi_process.host, smpd_process.host, PMI_MAX_HOST_NAME_LENGTH);
        MPIU_Strncpy(pmi_process.root_host, smpd_process.host, PMI_MAX_HOST_NAME_LENGTH);
        pmi_process.root_port = smpd_process.port;
        pmi_process.port = smpd_process.port;
        /*
        printf("Received:\nkvs_name = %s\nhost = %s\nport = %d\n",
                pmi_process.kvs_name, pmi_process.host, pmi_process.port); fflush(stdout);
        */

        smpd_process.id = 1;
        pmi_process.smpd_id = 1;
        pmi_process.smpd_key = 0;
        pmi_process.rpmi = PMI_TRUE;
        pmi_process.local_kvs = PMI_FALSE;
        pmi_process.iproc = 0;
        pmi_process.nproc = 1;

        smpd_process.is_singleton_client = SMPD_TRUE;

        result = SMPDU_Sock_create_set(&pmi_process.set);
	    if (result != SMPD_SUCCESS){
            MPIU_Snprintf(err_msg, PMII_MAX_ERR_MSG_LENGTH, "SMPDU_Sock_create_set failed: unable to create a sock set, error: %d\n", result);
    	    PMII_ERR_SETPRINTANDJUMP(err_msg, result);
		}

        /* Connect to PM */
		result = uPMI_ConnectToHost(pmi_process.root_host, pmi_process.root_port, SMPD_CONNECTING_RPMI);
		if (result != SMPD_SUCCESS){ 
            MPIU_Snprintf(err_msg, PMII_MAX_ERR_MSG_LENGTH, "uPMI_ConnectToHost failed: error: %s\n", result);
    	    PMII_ERR_SETPRINTANDJUMP(err_msg, result);
		}

        /* FIXME: Reduce size of rank_str & size_str */
        MPIU_Snprintf(rank_str, PMI_MAX_STR_VAL_LENGTH, "%d", pmi_process.iproc);
        MPIU_Snprintf(size_str, PMI_MAX_STR_VAL_LENGTH, "%d", pmi_process.nproc);

        result = pmi_create_post_command("init", pmi_process.kvs_name, rank_str, size_str);
        if (result != PMI_SUCCESS){
	        pmi_err_printf("PMIi_InitSingleton failed: unable to create an init command.\n");
	        return PMI_FAIL;
        }

        /* parse the result of the command */
        if (MPIU_Str_get_string_arg(pmi_process.context->read_cmd.cmd, "result", str, PMI_MAX_STR_VAL_LENGTH)
             != MPIU_STR_SUCCESS){
        	pmi_err_printf("PMIi_InitSingleton failed: no result string in the 'init' result command.\n");
	        return PMI_FAIL;
        }

        if (strncmp(str, SMPD_SUCCESS_STR, PMI_MAX_STR_VAL_LENGTH)){
	        pmi_err_printf("PMIi_InitSingleton failed: %s\n", str);
	        return PMI_FAIL;
        }

        /* Send info about the process to PM */
        result = pmi_create_post_command("proc_info", pmi_process.kvs_name, rank_str, size_str);
        if (result != PMI_SUCCESS){
	        pmi_dbg_printf("PMIi_InitSingleton failed: unable to create a 'proc_info' command.\n");
	        return PMI_FAIL;
        }

        /* parse the result of the command */
        if (MPIU_Str_get_string_arg(pmi_process.context->read_cmd.cmd, "result", str, PMI_MAX_STR_VAL_LENGTH)
             != MPIU_STR_SUCCESS){
        	pmi_err_printf("PMIi_InitSingleton failed: no result string in the 'proc_info' result command.\n");
	        return PMI_FAIL;
        }

        if (strncmp(str, SMPD_SUCCESS_STR, PMI_MAX_STR_VAL_LENGTH)){
	        pmi_err_printf("PMIi_InitSingleton failed: %s\n", str);
	        return PMI_FAIL;
        }

        pmi_process.init_finalized = PMI_INITIALIZED;
        /* Sync old local KVS with new remote KVS */
        if(pmi_rsync_localKVS(pmi_process.kvs_name_singleton_nopm, pmi_process.kvs_name)
                != PMI_SUCCESS){
            PMII_ERR_SETPRINTANDJUMP("Sync'ing local KVS in singleton proc to remote KVS in PM failed",
                PMI_FAIL);
        }

        /* Remove local KVS */
        if(pmi_destroy_localKVS() != PMI_SUCCESS){
            PMII_ERR_SETPRINTANDJUMP("Error removing local KVS in singleton proc\n", PMI_FAIL);
        }
    }
	else{
        MPIU_Snprintf(err_msg, PMII_MAX_ERR_MSG_LENGTH, 
            "No mechanism specified for connecting to the process manager - host %s or port %d provided.\n", 
            pmi_process.host, pmi_process.port);
	    PMII_ERR_SETPRINTANDJUMP(err_msg, result);
	}

fn_exit:
    if(singleton_client_set){
        result = SMPDU_Sock_destroy_set(singleton_client_set);
        if(result != SMPD_SUCCESS){
            pmi_err_printf("SMPDU_Sock_destroy_set failed: unable to destroy a sock set, error: %d\n", result);
        }
    }
    /* Make sure we return the error code set within the funcn */
    return retval;
fn_fail:
    /* FIXME : Make sure the newly created mpiexec process is also killed in the case of an error */
	/* FIXME : On failure do we have a local KVS ? */
    if(p_singleton_context){
        result = smpd_free_context(p_singleton_context);
        if(result != SMPD_SUCCESS){
            pmi_err_printf("smpd_free_context failed, error = %d\n", result);
        }
    }
    goto fn_exit;
}

int iPMI_Initialized(int *initialized)
{
    if (initialized == NULL)
	return PMI_ERR_INVALID_ARG;
    if (pmi_process.init_finalized == PMI_INITIALIZED)
    {
	*initialized = PMI_TRUE;
    }
    else
    {
	*initialized = PMI_FALSE;
    }
    return PMI_SUCCESS;
}

static int parse_clique(const char *str_orig)
{
    int count, i;
    char *str, *token;
    int first, last;

    /* count clique */
    count = 0;
    str = MPIU_Strdup(str_orig);
    if (str == NULL)
	return PMI_FAIL;
    token = strtok(str, ",");
    while (token)
    {
	first = atoi(token);
	while (isdigit(*token))
	    token++;
	if (*token == '\0')
	    count++;
	else
	{
	    if (*token == '.')
	    {
		token++;
		token++;
		last = atoi(token);
		count += last - first + 1;
	    }
	    else
	    {
		pmi_err_printf("unexpected clique token: '%s'\n", token);
		MPIU_Free(str);
		return PMI_FAIL;
	    }
	}
	token = strtok(NULL, ",");
    }
    MPIU_Free(str);

    /* allocate array */
    pmi_process.clique_ranks = (int*)MPIU_Malloc(count * sizeof(int));
    if (pmi_process.clique_ranks == NULL)
	return PMI_FAIL;
    pmi_process.clique_size = count;
    
    /* populate array */
    count = 0;
    str = MPIU_Strdup(str_orig);
    if (str == NULL)
	return PMI_FAIL;
    token = strtok(str, ",");
    while (token)
    {
	first = atoi(token);
	while (isdigit(*token))
	    token++;
	if (*token == '\0')
	{
	    pmi_process.clique_ranks[count] = first;
	    count++;
	}
	else
	{
	    if (*token == '.')
	    {
		token++;
		token++;
		last = atoi(token);
		for (i=first; i<=last; i++)
		{
		    pmi_process.clique_ranks[count] = i;
		    count++;
		}
	    }
	    else
	    {
		pmi_err_printf("unexpected clique token: '%s'\n", token);
		MPIU_Free(str);
		return PMI_FAIL;
	    }
	}
	token = strtok(NULL, ",");
    }
    MPIU_Free(str);

    /*
    printf("clique: %d [", pmi_process.iproc);
    for (i=0; i<pmi_process.clique_size; i++)
    {
	printf("%d,", pmi_process.clique_ranks[i]);
    }
    printf("]\n");
    fflush(stdout);
    */
    return PMI_SUCCESS;
}


static int rPMI_Init(int *spawned)
{
    char *p;
    int result;
    char rank_str[100], size_str[100];
    char str[1024];

    if (spawned == NULL)
	return PMI_ERR_INVALID_ARG;

    /* Enable state machine tracing 
    smpd_process.verbose = SMPD_TRUE;
    smpd_process.dbg_state |= SMPD_DBG_STATE_ERROUT | SMPD_DBG_STATE_STDOUT | SMPD_DBG_STATE_TRACE;
    */

    /* initialize to defaults */
    smpd_process.id = 1;
    pmi_process.smpd_id = 1;
    pmi_process.rpmi = PMI_TRUE;
    pmi_process.iproc = 0;
    pmi_process.nproc = 1;

    p = getenv("PMI_ROOT_HOST");
    if (p == NULL)
    {
	pmi_err_printf("unable to initialize the rPMI library: no PMI_ROOT_HOST specified.\n");
	return PMI_FAIL;
    }
    strncpy(pmi_process.root_host, p, 100);

    p = getenv("PMI_ROOT_PORT");
    if (p == NULL)
    {
	/* set to default port? */
	pmi_err_printf("unable to initialize the rPMI library: no PMI_ROOT_PORT specified.\n");
	return PMI_FAIL;
    }
    pmi_process.root_port = atoi(p);
    if (pmi_process.root_port < 1)
    {
	pmi_err_printf("invalid root port specified: %s\n", p);
	return PMI_FAIL;
    }
    smpd_process.port = pmi_process.root_port;
    strcpy(smpd_process.host, pmi_process.root_host);

    p = getenv("PMI_SPAWN");
    if (p)
    {
	*spawned = atoi(p);
    }
    else
    {
	*spawned = 0;
    }

    p = getenv("PMI_KVS");
    if (p != NULL)
    {
	/* use specified kvs name */
	strncpy(pmi_process.kvs_name, p, PMI_MAX_KVS_NAME_LENGTH);
	strncpy(smpd_process.kvs_name, p, PMI_MAX_KVS_NAME_LENGTH);
    }
    else
    {
	/* use default kvs name */
	strncpy(pmi_process.kvs_name, "default_mpich_kvs_name", PMI_MAX_KVS_NAME_LENGTH);
	strncpy(smpd_process.kvs_name, "default_mpich_kvs_name", PMI_MAX_KVS_NAME_LENGTH);
    }

    p = getenv("PMI_DOMAIN");
    if (p != NULL)
    {
	strncpy(pmi_process.domain_name, p, PMI_MAX_KVS_NAME_LENGTH);
	strncpy(smpd_process.domain_name, p, PMI_MAX_KVS_NAME_LENGTH);
    }
    else
    {
	strncpy(pmi_process.domain_name, "mpich2", PMI_MAX_KVS_NAME_LENGTH);
	strncpy(smpd_process.domain_name, "mpich2", PMI_MAX_KVS_NAME_LENGTH);
    }

    p = getenv("PMI_RANK");
    if (p != NULL)
    {
	pmi_process.iproc = atoi(p);
	if (pmi_process.iproc < 0)
	{
	    pmi_err_printf("invalid rank %d\n", pmi_process.iproc);
	    return PMI_FAIL;
	}
    }

    p = getenv("PMI_SIZE");
    if (p != NULL)
    {
	pmi_process.nproc = atoi(p);
	if (pmi_process.nproc < 1)
	{
	    pmi_err_printf("invalid size %d\n", pmi_process.nproc);
	    return PMI_FAIL;
	}
    }
    smpd_process.nproc = pmi_process.nproc;
#ifdef SINGLE_PROCESS_OPTIMIZATION
/* leave this code #ifdef'd out so we can test rPMI stuff with one process */
    if (pmi_process.nproc == 1)
    {
	pmi_process.local_kvs = PMI_TRUE;
	result = smpd_dbs_init();
	if (result != SMPD_SUCCESS)
	{
	    pmi_err_printf("unable to initialize the local dbs engine.\n");
	    return PMI_FAIL;
	}
	result = smpd_dbs_create(pmi_process.kvs_name);
	if (result != SMPD_SUCCESS)
	{
	    pmi_err_printf("unable to create the process group kvs\n");
	    return PMI_FAIL;
	}
	pmi_process.init_finalized = PMI_INITIALIZED;
	return PMI_SUCCESS;
    }
#endif

    p = getenv("PMI_CLIQUE");
    if (p != NULL)
    {
	parse_clique(p);
    }

    /*
    printf("PMI_ROOT_HOST=%s PMI_ROOT_PORT=%s PMI_RANK=%s PMI_SIZE=%s PMI_KVS=%s PMI_CLIQUE=%s\n",
	getenv("PMI_ROOT_HOST"), getenv("PMI_ROOT_PORT"), getenv("PMI_RANK"), getenv("PMI_SIZE"),
	getenv("PMI_KVS"), getenv("PMI_CLIQUE"));
    fflush(stdout);
    */

    if (pmi_process.iproc == 0)
    {
	p = getenv("PMI_ROOT_LOCAL");
	if (p && strcmp(p, "1") == 0)
	{
#ifdef HAVE_WINDOWS_H
	    pmi_process.hRootThreadReadyEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	    if (pmi_process.hRootThreadReadyEvent == NULL)
	    {
		pmi_err_printf("unable to create the root listener synchronization event, error: %d\n", GetLastError());
		return PMI_FAIL;
	    }
	    pmi_process.hRootThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)root_smpd, NULL, 0, NULL);
	    if (pmi_process.hRootThread == NULL)
	    {
		pmi_err_printf("unable to create the root listener thread: error %d\n", GetLastError());
		return PMI_FAIL;
	    }
	    if (WaitForSingleObject(pmi_process.hRootThreadReadyEvent, 60000) != WAIT_OBJECT_0)
	    {
		pmi_err_printf("the root process thread failed to initialize.\n");
		return PMI_FAIL;
	    }
#else
	    result = fork();
	    if (result == -1)
	    {
		pmi_err_printf("unable to fork the root listener, errno %d\n", errno);
		return PMI_FAIL;
	    }
	    if (result == 0)
	    {
		root_smpd(NULL);
		exit(0);
	    }
	    pmi_process.root_pid = result;
#endif
	}
    }

    /* connect to the root */

    result = SMPDU_Sock_create_set(&pmi_process.set);
    if (result != SMPD_SUCCESS)
    {
	pmi_err_printf("PMI_Init failed: unable to create a sock set, error: %d\n", result);
	return PMI_FAIL;
    }

    result = uPMI_ConnectToHost(pmi_process.root_host, pmi_process.root_port, SMPD_CONNECTING_RPMI);
    if (result != SMPD_SUCCESS)
    {
	pmi_err_printf("PMI_Init failed.\n");
	return PMI_FAIL;
    }

    pmi_process.init_finalized = PMI_INITIALIZED;

    sprintf(rank_str, "%d", pmi_process.iproc);
    sprintf(size_str, "%d", pmi_process.nproc);
    result = pmi_create_post_command("init", pmi_process.kvs_name, rank_str, size_str);
    if (result != PMI_SUCCESS)
    {
	pmi_err_printf("PMI_Init failed: unable to create an init command.\n");
	return PMI_FAIL;
    }

    /* parse the result of the command */
    if (MPIU_Str_get_string_arg(pmi_process.context->read_cmd.cmd, "result", str, 1024) != MPIU_STR_SUCCESS)
    {
	pmi_err_printf("PMI_Init failed: no result string in the result command.\n");
	return PMI_FAIL;
    }
    if (strcmp(str, SMPD_SUCCESS_STR))
    {
	pmi_err_printf("PMI_Init failed: %s\n", str);
	return PMI_FAIL;
    }

    return PMI_SUCCESS;
}

static int rPMI_Finalize()
{
    int result;
    char rank_str[100];
    char str[1024];
#ifndef HAVE_WINDOWS_H
    int status;
#endif

    if (pmi_process.init_finalized == PMI_FINALIZED)
	return PMI_SUCCESS;

    if(pmi_process.init_finalized < PMI_INITIALIZED)
    return PMI_SUCCESS;

    if (pmi_process.local_kvs)
    {
	if(pmi_destroy_localKVS() != PMI_SUCCESS){
        pmi_dbg_printf("Failed to destroy local KVS\n");
    }
    if(pmi_process.singleton_mpiexec_fd != PMII_PROCESS_INVALID_HANDLE){
#ifdef HAVE_WINDOWS_H
        WaitForSingleObject(pmi_process.singleton_mpiexec_fd, INFINITE);
#else
        waitpid(pmi_process.singleton_mpiexec_fd, &status, WUNTRACED);
#endif	
    }

	result = SMPDU_Sock_finalize();
	pmi_process.init_finalized = PMI_FINALIZED;
	return PMI_SUCCESS;
    }

    sprintf(rank_str, "%d", pmi_process.iproc);
    result = pmi_create_post_command("finalize", pmi_process.kvs_name, rank_str, NULL);
    if (result != PMI_SUCCESS)
    {
	pmi_err_printf("PMI_Finalize failed: unable to create an finalize command.\n");
	return PMI_FAIL;
    }

    /* parse the result of the command */
    if (MPIU_Str_get_string_arg(pmi_process.context->read_cmd.cmd, "result", str, 1024) != MPIU_STR_SUCCESS)
    {
	pmi_err_printf("PMI_Finalize failed: no result string in the result command.\n");
	return PMI_FAIL;
    }
    if (strcmp(str, SMPD_SUCCESS_STR))
    {
	pmi_err_printf("PMI_Finalize failed: %s\n", str);
	return PMI_FAIL;
    }

    if (pmi_process.iproc == 0)
    {
	/* the root process tells the root to exit when all the pmi contexts have exited */
	result = pmi_create_post_command("exit_on_done", NULL, NULL, NULL);
	if (result != PMI_SUCCESS)
	{
	    pmi_err_printf("exit_on_done command failed.\n");
	    return PMI_FAIL;
	}
	/*printf("exit_on_done command returned successfully.\n");fflush(stdout);*/
    }

    /*printf("entering finalize pmi_barrier.\n");fflush(stdout);*/
    PMI_Barrier();
    /*printf("after finalize pmi_barrier, posting done command.\n");fflush(stdout);*/

    /* post a done command to close the pmi context */
    result = pmi_create_post_command("done", NULL, NULL, NULL);
    if (result != PMI_SUCCESS)
    {
	pmi_err_printf("failed.\n");
	return PMI_FAIL;
    }

    if (pmi_process.iproc == 0)
    {
#ifdef HAVE_WINDOWS_H
	WaitForSingleObject(pmi_process.hRootThread, INFINITE);
    if(pmi_process.singleton_mpiexec_fd != PMII_PROCESS_INVALID_HANDLE){
        WaitForSingleObject(pmi_process.singleton_mpiexec_fd, INFINITE);
    }
#else
	waitpid(pmi_process.root_pid, &status, WUNTRACED);
    if(pmi_process.singleton_mpiexec_fd != PMII_PROCESS_INVALID_HANDLE){
        waitpid(pmi_process.singleton_mpiexec_fd, &status, WUNTRACED);
    }
#endif
    }

    /*if (pmi_process.sock != SMPDU_SOCK_INVALID_SOCK)*/
    {
	result = SMPDU_Sock_finalize();
	if (result != SMPD_SUCCESS)
	{
	    /*pmi_err_printf("SMPDU_Sock_finalize failed, error: %d\n", result);*/
	}
    }

    pmi_process.init_finalized = PMI_FINALIZED;

    return PMI_SUCCESS;
}

int iPMI_Init(int *spawned)
{
    char *p;
    int result;
    char rank_str[100], size_str[100];
    char str[1024];

    if (spawned == NULL){
	return PMI_ERR_INVALID_ARG;
    }

    /* Enable smpd state machine tracing */
    /*
    smpd_process.verbose = SMPD_TRUE;
	smpd_process.dbg_state |= SMPD_DBG_STATE_ERROUT | SMPD_DBG_STATE_STDOUT | SMPD_DBG_STATE_TRACE;
    */

    pmi_init_printf();

    /* don't allow pmi_init to be called more than once */
    if (pmi_process.init_finalized == PMI_INITIALIZED)
	return PMI_SUCCESS;

    /* initialize to defaults */

    result = SMPDU_Sock_init();
    if (result != SMPD_SUCCESS)
    {
	pmi_err_printf("SMPDU_Sock_init failed,\nsock error: %s\n", get_sock_error_string(result));
	return PMI_FAIL;
    }

    result = smpd_init_process();
    if (result != SMPD_SUCCESS)
    {
	pmi_err_printf("unable to initialize the smpd global process structure.\n");
	return PMI_FAIL;
    }

    p = getenv("PMI_ROOT_HOST");
    if (p != NULL)
    {
	return rPMI_Init(spawned);
    }

    pmi_process.iproc = 0;
    pmi_process.nproc = 1;

    p = getenv("PMI_SPAWN");
    if (p)
    {
	*spawned = atoi(p);
    }
    else
    {
	*spawned = 0;
    }

    p = getenv("PMI_APPNUM");
    if (p)
    {
	pmi_process.appnum = atoi(p);
    }
    else
    {
	pmi_process.appnum = 0;
    }

    /* Determine If singleton */
    p = getenv("PMI_SMPD_FD");
    if( p == NULL){
        p = getenv("PMI_HOST");
        if( p == NULL){
            /* FIXME: Do we need a check for PMI_KVS to determine if 
             * client is singleton ?
             */
            p = getenv("PMI_KVS");
            if(p == NULL){
                /* Assume singleton. 
                * Setup the PMI service when required i.e., later 
                */
                pmi_process.init_finalized = 
                    PMI_SINGLETON_INIT_BUT_NO_PM;
                /* Rank & Nprocs initialized by default above*/
                /* Create a local KVS which will be used until
                    spawn(), universe_size, kvs_get() is called*/
                if(pmi_create_localKVS() == PMI_SUCCESS){
                    return PMI_SUCCESS;
                }
                else{
                    pmi_err_printf("Unable to create local KVS\n");
                    return PMI_FAIL;
                }
            }
        }
    }
    else{
        /* decode PMI_SMPD_FD */
#ifdef HAVE_WINDOWS_H
	    pmi_process.smpd_fd = smpd_decode_handle(p);
#else
	    pmi_process.smpd_fd = (SMPDU_SOCK_NATIVE_FD)atoi(p);
#endif
        if(pmi_process.smpd_fd <= 0){
            /* FIXME: hack - Is there a better way ? */
            /* mpiexec sets smpd_fd<=0 to distinguish itself from
             * a singleton MPI process
             */
            /* FIXME: Get rid of this hack - we already create 
             * local KVS for all singleton clients by default
             */
            pmi_process.smpd_fd = 0;
            putenv("PMI_SMPD_FD=");
        }
    }

    p = getenv("PMI_KVS");
    if (p != NULL)
    {
	strncpy(pmi_process.kvs_name, p, PMI_MAX_KVS_NAME_LENGTH);
    }
    else
    {
        /* mpiexec/smpd don't set PMI_KVS */
        if(pmi_create_localKVS() == PMI_SUCCESS){
	        pmi_process.init_finalized = PMI_INITIALIZED;
	        return PMI_SUCCESS;
        }
        else{
            pmi_err_printf("unable to create local KVS\n");
            return PMI_FAIL;
        }
    }

    p = getenv("PMI_DOMAIN");
    if (p != NULL)
    {
	strncpy(pmi_process.domain_name, p, PMI_MAX_KVS_NAME_LENGTH);
    }
    else
    {
	strncpy(pmi_process.domain_name, "mpich2", PMI_MAX_KVS_NAME_LENGTH);
    }

    p = getenv("PMI_RANK");
    if (p != NULL)
    {
	pmi_process.iproc = atoi(p);
	if (pmi_process.iproc < 0)
	{
	    pmi_err_printf("invalid rank %d, setting to 0\n", pmi_process.iproc);
	    pmi_process.iproc = 0;
	}
    }

    p = getenv("PMI_SIZE");
    if (p != NULL)
    {
	pmi_process.nproc = atoi(p);
	if (pmi_process.nproc < 1)
	{
	    pmi_err_printf("invalid size %d, setting to 1\n", pmi_process.nproc);
	    pmi_process.nproc = 1;
	}
    }

    p = getenv("PMI_SMPD_ID");
    if (p != NULL){
        pmi_process.smpd_id = atoi(p);
	    smpd_process.id = pmi_process.smpd_id;
    }

    p = getenv("PMI_SMPD_KEY");
    if (p != NULL)
    {
	pmi_process.smpd_key = atoi(p);
    }

    p = getenv("PMI_SMPD_FD");
    if (p != NULL)
    {
	    result = SMPDU_Sock_create_set(&pmi_process.set);
	    if (result != SMPD_SUCCESS)
	    {
	    pmi_err_printf("PMI_Init failed: unable to create a sock set, error:\n%s\n",
		get_sock_error_string(result));
	    return PMI_FAIL;
	    }
        /* pmi_process.smpd_fd is decoded when checking for Singleton Init */
	    result = SMPDU_Sock_native_to_sock(pmi_process.set, pmi_process.smpd_fd, NULL, &pmi_process.sock);
	    if (result != SMPD_SUCCESS)
	    {
	    pmi_err_printf("SMPDU_Sock_native_to_sock failed, error %s\n", get_sock_error_string(result));
	    return PMI_FAIL;
	    }
	    result = smpd_create_context(SMPD_CONTEXT_PMI, pmi_process.set, pmi_process.sock, pmi_process.smpd_id, &pmi_process.context);
	    if (result != SMPD_SUCCESS)
	    {
	    pmi_err_printf("unable to create a pmi context.\n");
	    return PMI_FAIL;
	    }
    }
    else
    {
	p = getenv("PMI_HOST");
	if (p != NULL)
	{
	    strncpy(pmi_process.host, p, PMI_MAX_HOST_NAME_LENGTH);
	    p = getenv("PMI_PORT");
	    if (p != NULL)
	    {
		pmi_process.port = atoi(p);

		result = SMPDU_Sock_create_set(&pmi_process.set);
		if (result != SMPD_SUCCESS)
		{
		    pmi_err_printf("PMI_Init failed: unable to create a sock set, error: %d\n", result);
		    return PMI_FAIL;
		}

		result = uPMI_ConnectToHost(pmi_process.host, pmi_process.port, SMPD_CONNECTING_PMI);
		if (result != SMPD_SUCCESS)
		{
		    pmi_err_printf("PMI_Init failed.\n");
		    return PMI_FAIL;
		}
	    }
	    else
	    {
		pmi_err_printf("No mechanism specified for connecting to the process manager - host %s but no port provided.\n", pmi_process.host);
		return PMI_FAIL;
	    }
	}
	else
	{
        /* SINGLETON: Assume singleton here and initialize to SINGLETON_INIT_BUT_NO_PM 
         * Also set PMI_KVS & PMI_DOMAIN after this step...
         */
	    pmi_err_printf("No mechanism specified for connecting to the process manager.\n");
	    return PMI_FAIL;
	}
    }

    p = getenv("PMI_CLIQUE");
    if (p != NULL)
    {
	parse_clique(p);
    }
    /*
    printf("PMI_RANK=%s PMI_SIZE=%s PMI_KVS=%s PMI_SMPD_ID=%s PMI_SMPD_FD=%s PMI_SMPD_KEY=%s\n PMI_SPAWN=%s",
	getenv("PMI_RANK"), getenv("PMI_SIZE"), getenv("PMI_KVS"), getenv("PMI_SMPD_ID"),
	getenv("PMI_SMPD_FD"), getenv("PMI_SMPD_KEY"), getenv("PMI_SPAWN"));
    fflush(stdout);
    */

    pmi_process.init_finalized = PMI_INITIALIZED;

    sprintf(rank_str, "%d", pmi_process.iproc);
    sprintf(size_str, "%d", pmi_process.nproc);
    result = pmi_create_post_command("init", pmi_process.kvs_name, rank_str, size_str);
    if (result != PMI_SUCCESS)
    {
	pmi_err_printf("PMI_Init failed: unable to create an init command.\n");
	return PMI_FAIL;
    }

    /* parse the result of the command */
    if (MPIU_Str_get_string_arg(pmi_process.context->read_cmd.cmd, "result", str, 1024) != MPIU_STR_SUCCESS)
    {
	pmi_err_printf("PMI_Init failed: no result string in the result command.\n");
	return PMI_FAIL;
    }
    if (strcmp(str, SMPD_SUCCESS_STR))
    {
	pmi_err_printf("PMI_Init failed: %s\n", str);
	return PMI_FAIL;
    }

    /*
    if (*spawned && pmi_process.iproc == 0)
    {
	char key[1024], val[8192];
	key[0] = '\0';
	result = PMI_KVS_Iter_first(pmi_process.kvs_name, key, 1024, val, 8192);
	if (result != PMI_SUCCESS || key[0] == '\0')
	{
	    printf("No preput values in %s\n", pmi_process.kvs_name);
	}
	while (result == PMI_SUCCESS && key[0] != '\0')
	{
	    printf("PREPUT key=%s, val=%s\n", key, val);
	    result = PMI_KVS_Iter_next(pmi_process.kvs_name, key, 1024, val, 8192);
	}
	fflush(stdout);
    }
    iPMI_Barrier();
    */

    /*printf("iPMI_Init returning success.\n");fflush(stdout);*/
    return PMI_SUCCESS;
}

int iPMI_Finalize()
{
    int result;
    char rank_str[100];
    char str[1024];

    if (pmi_process.init_finalized == PMI_FINALIZED)
	return PMI_SUCCESS;

    if (pmi_process.rpmi)
    {
	return rPMI_Finalize();
    }

    if(pmi_process.init_finalized < PMI_INITIALIZED)
    return PMI_SUCCESS;

    if (pmi_process.local_kvs)
    {
	if(pmi_destroy_localKVS() != PMI_SUCCESS){
        pmi_dbg_printf("Failed to destroy local KVS\n");
    }
	result = SMPDU_Sock_finalize();
	pmi_process.init_finalized = PMI_FINALIZED;
	return PMI_SUCCESS;
    }

    sprintf(rank_str, "%d", pmi_process.iproc);
    result = pmi_create_post_command("finalize", pmi_process.kvs_name, rank_str, NULL);
    if (result != PMI_SUCCESS)
    {
	pmi_err_printf("PMI_Finalize failed: unable to create an finalize command.\n");
	goto fn_fail;
    }

    /* parse the result of the command */
    if (MPIU_Str_get_string_arg(pmi_process.context->read_cmd.cmd, "result", str, 1024) != MPIU_STR_SUCCESS)
    {
	pmi_err_printf("PMI_Finalize failed: no result string in the result command.\n");
	goto fn_fail;
    }
    if (strcmp(str, SMPD_SUCCESS_STR))
    {
	pmi_err_printf("PMI_Finalize failed: %s\n", str);
	goto fn_fail;
    }

    PMI_Barrier();

    /* post the done command and wait for the result */
    result = pmi_create_post_command("done", NULL, NULL, NULL);
    if (result != PMI_SUCCESS)
    {
	pmi_err_printf("failed.\n");
	goto fn_fail;
    }

    /*if (pmi_process.sock != SMPDU_SOCK_INVALID_SOCK)*/
    {
	result = SMPDU_Sock_finalize();
	if (result != SMPD_SUCCESS)
	{
	    /*pmi_err_printf("SMPDU_Sock_finalize failed,\nsock error: %s\n", get_sock_error_string(result));*/
	}
    }

    pmi_process.init_finalized = PMI_FINALIZED;
    /*printf("iPMI_Finalize success.\n");fflush(stdout);*/
    return PMI_SUCCESS;

fn_fail:
    /* set the state to finalized so PMI_Abort will not dereference mangled structures due to a failure */
    pmi_process.init_finalized = PMI_FINALIZED;
    return PMI_FAIL;
}

int iPMI_Abort(int exit_code, const char error_msg[])
{
    int result;
    smpd_command_t *cmd_ptr;

    /* flush any output before aborting */
    /* This doesn't work because it flushes output from the mpich dll but does not flush the main module's output */
    fflush(stdout);
    fflush(stderr);

    if (pmi_process.init_finalized == PMI_FINALIZED)
    {
	printf("PMI_Abort called after PMI_Finalize, error message:\n%s\n", error_msg);
	fflush(stdout);
#ifdef HAVE_WINDOWS_H
	/* ExitProcess(exit_code); */
    TerminateProcess(GetCurrentProcess(), exit_code);
#else
	exit(exit_code);
	return PMI_FAIL;
#endif
    }

    if (pmi_process.local_kvs)
    {
	if (smpd_process.verbose_abort_output)
	{
	    printf("\njob aborted:\n");
	    printf("process: node: exit code: error message:\n");
	    printf("0: localhost: %d", exit_code);
	    if (error_msg != NULL)
	    {
		printf(": %s", error_msg);
	    }
	    printf("\n");
	}
	else
	{
	    if (error_msg != NULL)
	    {
		printf("%s\n", error_msg);
	    }
	}
	fflush(stdout);
	if(pmi_destroy_localKVS() != PMI_SUCCESS){
        pmi_dbg_printf("Failed to destroy local KVS\n");
    }
	pmi_process.init_finalized = PMI_FINALIZED;
#ifdef HAVE_WINDOWS_H
	/* ExitProcess(exit_code); */
    TerminateProcess(GetCurrentProcess(), exit_code);
#else
	exit(exit_code);
	return PMI_FAIL;
#endif
    }
    if(pmi_process.init_finalized < PMI_INITIALIZED)
        return PMI_FAIL;

    result = smpd_create_command("abort_job", pmi_process.smpd_id, 0, SMPD_FALSE, &cmd_ptr);
    if (result != SMPD_SUCCESS)
    {
	pmi_err_printf("unable to create an abort command.\n");
	return PMI_FAIL;
    }

    result = smpd_add_command_arg(cmd_ptr, "name", pmi_process.kvs_name);
    if (result != SMPD_SUCCESS)
    {
	pmi_err_printf("unable to add the kvs name('%s') to the abort command.\n", pmi_process.kvs_name);
	return PMI_FAIL;
    }

    result = smpd_add_command_int_arg(cmd_ptr, "rank", pmi_process.iproc);
    if (result != SMPD_SUCCESS)
    {
	pmi_err_printf("unable to add the rank %d to the abort command.\n", pmi_process.iproc);
	return PMI_FAIL;
    }

    result = smpd_add_command_arg(cmd_ptr, "error", (char*)error_msg);
    if (result != SMPD_SUCCESS)
    {
	pmi_err_printf("unable to add the error message('%s') to the abort command.\n", error_msg);
	return PMI_FAIL;
    }

    result = smpd_add_command_int_arg(cmd_ptr, "exit_code", exit_code);
    if (result != SMPD_SUCCESS)
    {
	pmi_err_printf("unable to add the exit code(%d) to the abort command.\n", exit_code);
	return PMI_FAIL;
    }

    /* post the write of the command */
    result = smpd_post_write_command(pmi_process.context, cmd_ptr);
    if (result != SMPD_SUCCESS)
    {
	pmi_err_printf("unable to post a write of the abort command.\n");
	return PMI_FAIL;
    }

    /* and post a read for the result */
    /*
    result = smpd_post_read_command(pmi_process.context);
    if (result != SMPD_SUCCESS)
    {
	pmi_err_printf("unable to post a read of the next command on the pmi context.\n");
	return PMI_FAIL;
    }
    */

    /* let the state machine send the command and receive the result */
    result = smpd_enter_at_state(pmi_process.set, SMPD_WRITING_CMD);
    if (result != SMPD_SUCCESS)
    {
	pmi_err_printf("the state machine logic failed to handle the abort command.\n");
	return PMI_FAIL;
    }

    if(pmi_process.iproc == 0 && 
        pmi_process.singleton_mpiexec_fd != PMII_PROCESS_INVALID_HANDLE){
		int status;
#ifdef HAVE_WINDOWS_H
        WaitForSingleObject(pmi_process.singleton_mpiexec_fd, INFINITE);
#else
        waitpid(pmi_process.singleton_mpiexec_fd, &status, WUNTRACED);
#endif
    }

#ifdef HAVE_WINDOWS_H
    /* ExitProcess(exit_code); */
    TerminateProcess(GetCurrentProcess(), exit_code);
#else
    exit(exit_code);
    return PMI_FAIL;
#endif
}

int iPMI_Get_size(int *size)
{
    if (pmi_process.init_finalized == PMI_FINALIZED)
	return PMI_ERR_INIT;
    if (size == NULL)
	return PMI_ERR_INVALID_ARG;

    *size = pmi_process.nproc;

    return PMI_SUCCESS;
}

int iPMI_Get_rank(int *rank)
{
    if (pmi_process.init_finalized == PMI_FINALIZED)
	return PMI_ERR_INIT;
    if (rank == NULL)
	return PMI_ERR_INVALID_ARG;

    *rank = pmi_process.iproc;

    return PMI_SUCCESS;
}

int iPMI_Get_universe_size(int *size)
{
    if (pmi_process.init_finalized == PMI_FINALIZED)
	return PMI_ERR_INIT;
    /* Singleton init */
    if(pmi_process.init_finalized == PMI_SINGLETON_INIT_BUT_NO_PM){
        if(PMIi_InitSingleton() != PMI_SUCCESS){
            return PMI_ERR_INIT;
        }
    }
    if (size == NULL)
	return PMI_ERR_INVALID_ARG;

    *size = -1;

    return PMI_SUCCESS;
}

int iPMI_Get_appnum(int *appnum)
{
    if (pmi_process.init_finalized == PMI_FINALIZED)
	return PMI_ERR_INIT;
    if (appnum == NULL)
	return PMI_ERR_INVALID_ARG;

    *appnum = pmi_process.appnum;

    return PMI_SUCCESS;
}

int iPMI_Get_clique_size( int *size )
{
    if (pmi_process.init_finalized == PMI_FINALIZED)
	return PMI_ERR_INIT;
    if (size == NULL)
	return PMI_ERR_INVALID_ARG;

    if (pmi_process.clique_size == 0)
	*size = 1;
    else
	*size = pmi_process.clique_size;
    return PMI_SUCCESS;
}

int iPMI_Get_clique_ranks( int ranks[], int length )
{
    int i;

    if (pmi_process.init_finalized == PMI_FINALIZED)
	return PMI_ERR_INIT;
    if (ranks == NULL)
	return PMI_ERR_INVALID_ARG;
    if (length < pmi_process.clique_size)
	return PMI_ERR_INVALID_LENGTH;

    if (pmi_process.clique_size == 0)
    {
	*ranks = 0;
    }
    else
    {
	for (i=0; i<pmi_process.clique_size; i++)
	{
	    ranks[i] = pmi_process.clique_ranks[i];
	}
    }
    return PMI_SUCCESS;
}

int iPMI_Get_id( char id_str[], int length )
{
    return iPMI_KVS_Get_my_name(id_str, length);
}

int iPMI_Get_id_length_max(int *maxlen)
{
    return iPMI_KVS_Get_name_length_max(maxlen);
}

int iPMI_Get_kvs_domain_id(char id_str[], int length)
{
    if (pmi_process.init_finalized == PMI_FINALIZED)
	return PMI_ERR_INIT;
    if (id_str == NULL)
	return PMI_ERR_INVALID_ARG;
    if (length < PMI_MAX_KVS_NAME_LENGTH)
	return PMI_ERR_INVALID_LENGTH;

    strncpy(id_str, pmi_process.domain_name, length);

    return PMI_SUCCESS;
}

int iPMI_Barrier()
{
    int result;
    char count_str[20];
    char str[1024];
    
    if (pmi_process.init_finalized == PMI_FINALIZED)
	return PMI_ERR_INIT;

    if (pmi_process.nproc == 1)
	return PMI_SUCCESS;

    /*printf("entering barrier %d, %s\n", pmi_process.nproc, pmi_process.kvs_name);fflush(stdout);*/

    /* encode the size of the barrier */
    snprintf(count_str, 20, "%d", pmi_process.nproc);

    /* post the command and wait for the result */
    result = pmi_create_post_command("barrier", pmi_process.kvs_name, NULL, count_str);
    if (result != PMI_SUCCESS)
    {
	pmi_err_printf("PMI_Barrier failed.\n");
	return PMI_FAIL;
    }

    /* interpret the result */
    if (MPIU_Str_get_string_arg(pmi_process.context->read_cmd.cmd, "result", str, 1024) != MPIU_STR_SUCCESS)
    {
	pmi_err_printf("PMI_Barrier failed: no result string in the result command.\n");
	return PMI_FAIL;
    }
    if (strcmp(str, DBS_SUCCESS_STR))
    {
	pmi_err_printf("PMI_Barrier failed: '%s'\n", str);
	return PMI_FAIL;
    }

    /*printf("iPMI_Barrier success.\n");fflush(stdout);*/
    return PMI_SUCCESS;
}

int iPMI_KVS_Get_my_name(char kvsname[], int length)
{
    if (pmi_process.init_finalized == PMI_FINALIZED)
	return PMI_ERR_INIT;
    if (kvsname == NULL)
	return PMI_ERR_INVALID_ARG;
    if (length < PMI_MAX_KVS_NAME_LENGTH)
	return PMI_ERR_INVALID_LENGTH;

    strncpy(kvsname, pmi_process.kvs_name, length);

    /*
    printf("my kvs name is %s\n", kvsname);fflush(stdout);
    */

    return PMI_SUCCESS;
}

int iPMI_KVS_Get_name_length_max(int *maxlen)
{
    if (pmi_process.init_finalized == PMI_FINALIZED)
	return PMI_ERR_INIT;
    if (maxlen == NULL)
	return PMI_ERR_INVALID_ARG;
    *maxlen = PMI_MAX_KVS_NAME_LENGTH;
    return PMI_SUCCESS;
}

int iPMI_KVS_Get_key_length_max(int *maxlen)
{
    if (pmi_process.init_finalized == PMI_FINALIZED)
	return PMI_ERR_INIT;
    if (maxlen == NULL)
	return PMI_ERR_INVALID_ARG;
    *maxlen = PMI_MAX_KEY_LEN;
    return PMI_SUCCESS;
}

int iPMI_KVS_Get_value_length_max(int *maxlen)
{
    if (pmi_process.init_finalized == PMI_FINALIZED)
	return PMI_ERR_INIT;
    if (maxlen == NULL)
	return PMI_ERR_INVALID_ARG;
    *maxlen = PMI_MAX_VALUE_LEN;
    return PMI_SUCCESS;
}

int iPMI_KVS_Create(char kvsname[], int length)
{
    int result;
    char str[1024];

    if (pmi_process.init_finalized == PMI_FINALIZED)
	return PMI_ERR_INIT;
    if (kvsname == NULL)
	return PMI_ERR_INVALID_ARG;
    if (length < PMI_MAX_KVS_NAME_LENGTH)
	return PMI_ERR_INVALID_LENGTH;

    if(pmi_process.init_finalized == PMI_SINGLETON_INIT_BUT_NO_PM){
        if(PMIi_InitSingleton() != PMI_SUCCESS){
            return PMI_ERR_INIT;
        }
    }

    if (pmi_process.local_kvs)
    {
	result = smpd_dbs_create(kvsname);
	return (result == SMPD_SUCCESS) ? PMI_SUCCESS : PMI_FAIL;
    }

    result = pmi_create_post_command("dbcreate", NULL, NULL, NULL);
    if (result != PMI_SUCCESS)
    {
	pmi_err_printf("PMI_KVS_Create failed: unable to create a pmi kvs space.\n");
	return PMI_FAIL;
    }

    /* parse the result of the command */
    if (MPIU_Str_get_string_arg(pmi_process.context->read_cmd.cmd, "result", str, 1024) != MPIU_STR_SUCCESS)
    {
	pmi_err_printf("PMI_KVS_Create failed: no result string in the result command.\n");
	return PMI_FAIL;
    }
    if (strcmp(str, DBS_SUCCESS_STR))
    {
	pmi_err_printf("PMI_KVS_Create failed: %s\n", str);
	return PMI_FAIL;
    }
    if (MPIU_Str_get_string_arg(pmi_process.context->read_cmd.cmd, "name", str, 1024) != MPIU_STR_SUCCESS)
    {
	pmi_err_printf("PMI_KVS_Create failed: no kvs name in the dbcreate result command.\n");
	return PMI_FAIL;
    }
    strncpy(kvsname, str, PMI_MAX_KVS_NAME_LENGTH);

    /*printf("iPMI_KVS_Create success.\n");fflush(stdout);*/
    return PMI_SUCCESS;
}

int iPMI_KVS_Destroy(const char kvsname[])
{
    int result;
    char str[1024];

    if (pmi_process.init_finalized == PMI_FINALIZED)
	return PMI_ERR_INIT;
    if (kvsname == NULL)
	return PMI_ERR_INVALID_ARG;

    if (pmi_process.local_kvs)
    {
	result = smpd_dbs_destroy(kvsname);
	return (result == SMPD_SUCCESS) ? PMI_SUCCESS : PMI_FAIL;
    }
    else{
        /* FIXME: Test only for singleton init proc */
        int len = 0;
        if((len = strlen(kvsname)) > PMI_MAX_KVS_NAME_LENGTH){
            return PMI_ERR_INVALID_LENGTH;
        }
        /* Is the destroy req for stale kvsname before
         * singleton init ?
         */
        if(strncmp(kvsname, pmi_process.kvs_name_singleton_nopm, len)
            == 0){
            return PMI_SUCCESS;
        }
    }

    result = pmi_create_post_command("dbdestroy", kvsname, NULL, NULL);
    if (result != PMI_SUCCESS)
    {
	pmi_err_printf("PMI_KVS_Destroy failed: unable to destroy the pmi kvs space named '%s'.\n", kvsname);
	return PMI_FAIL;
    }

    /* parse the result of the command */
    if (MPIU_Str_get_string_arg(pmi_process.context->read_cmd.cmd, "result", str, 1024) != MPIU_STR_SUCCESS)
    {
	pmi_err_printf("PMI_KVS_Destroy failed: no result string in the result command.\n");
	return PMI_FAIL;
    }
    if (strcmp(str, DBS_SUCCESS_STR))
    {
	pmi_err_printf("PMI_KVS_Destroy failed: %s\n", str);
	return PMI_FAIL;
    }

    return PMI_SUCCESS;
}

int iPMI_KVS_Put(const char kvsname[], const char key[], const char value[])
{
    int result;
    char str[1024];
    const char *kvsname_ = NULL;

    if (pmi_process.init_finalized == PMI_FINALIZED)
	return PMI_ERR_INIT;
    if (kvsname == NULL)
	return PMI_ERR_INVALID_ARG;
    if (key == NULL)
	return PMI_ERR_INVALID_KEY;
    if (value == NULL)
	return PMI_ERR_INVALID_VAL;

    kvsname_ = kvsname;
    /*printf("putting <%s><%s><%s>\n", kvsname, key, value);fflush(stdout);*/

    if (pmi_process.local_kvs)
    {
	result = smpd_dbs_put(kvsname_, key, value);
	return (result == SMPD_SUCCESS) ? PMI_SUCCESS : PMI_FAIL;
    }
    else{
        int len = 0;
        if((len = strlen(kvsname)) > PMI_MAX_KVS_NAME_LENGTH){
            return PMI_ERR_INVALID_LENGTH;
        }
        /* Update kvsname if the caller has the stale kvsname before
         * singleton init
         */
        if(strncmp(kvsname, pmi_process.kvs_name_singleton_nopm, len)
            == 0){
            kvsname_ = pmi_process.kvs_name;
        }
    }

    result = pmi_create_post_command("dbput", kvsname_, key, value);
    if (result != PMI_SUCCESS)
    {
	pmi_err_printf("PMI_KVS_Put failed: unable to put '%s:%s:%s'\n", kvsname_, key, value);
	return PMI_FAIL;
    }

    /* parse the result of the command */
    if (MPIU_Str_get_string_arg(pmi_process.context->read_cmd.cmd, "result", str, 1024) != MPIU_STR_SUCCESS)
    {
	pmi_err_printf("PMI_KVS_Put failed: no result string in the result command.\n");
	return PMI_FAIL;
    }
    if (strcmp(str, DBS_SUCCESS_STR))
    {
	pmi_err_printf("PMI_KVS_Put failed: '%s'\n", str);
	return PMI_FAIL;
    }

    /*printf("iPMI_KVS_Put success.\n");fflush(stdout);*/
    return PMI_SUCCESS;
}

int iPMI_KVS_Commit(const char kvsname[])
{
    if (pmi_process.init_finalized == PMI_FINALIZED)
	return PMI_ERR_INIT;
    if (kvsname == NULL)
	return PMI_ERR_INVALID_ARG;

    if (pmi_process.local_kvs)
    {
	return PMI_SUCCESS;
    }

    /* Make the puts return when the commands are written but not acknowledged.
       Then have this function wait until all outstanding puts are acknowledged.
       */

    return PMI_SUCCESS;
}

int iPMI_KVS_Get(const char kvsname[], const char key[], char value[], int length)
{
    int result;
    char str[1024];
    const char *kvsname_ = NULL;

    if (pmi_process.init_finalized == PMI_FINALIZED)
	return PMI_ERR_INIT;

    if (kvsname == NULL)
	return PMI_ERR_INVALID_ARG;
    if (key == NULL)
	return PMI_ERR_INVALID_KEY;
    if (value == NULL)
	return PMI_ERR_INVALID_VAL;

    kvsname_ = kvsname;

	/* We need singleton init only for spawn, universe_size
    if(pmi_process.init_finalized == PMI_SINGLETON_INIT_BUT_NO_PM){
       if(PMIi_InitSingleton() != PMI_SUCCESS){
            return PMI_ERR_INIT;
        }
     }
	*/

    if (pmi_process.local_kvs)
    {
		result = smpd_dbs_get(kvsname_, key, value);
		return (result == SMPD_SUCCESS) ? PMI_SUCCESS : PMI_FAIL;
    }
    else{
        int len = 0;
        if((len = strlen(kvsname)) > PMI_MAX_KVS_NAME_LENGTH){
            return PMI_ERR_INVALID_LENGTH;
        }
        /* Update kvsname if the caller has the stale kvsname before
         * singleton init
         */
        if(strncmp(kvsname, pmi_process.kvs_name_singleton_nopm, len)
            == 0){
            kvsname_ = pmi_process.kvs_name;
        }
    }

    result = pmi_create_post_command("dbget", kvsname_, key, NULL);
    if (result != PMI_SUCCESS)
    {
	pmi_err_printf("PMI_KVS_Get failed: unable to get '%s:%s'\n", kvsname_, key);
	return PMI_FAIL;
    }

    /* parse the result of the command */
    if (MPIU_Str_get_string_arg(pmi_process.context->read_cmd.cmd, "result", str, 1024) != MPIU_STR_SUCCESS)
    {
	pmi_err_printf("PMI_KVS_Get failed: no result string in the result command.\n");
	return PMI_FAIL;
    }
    if (strcmp(str, DBS_SUCCESS_STR))
    {
        /* Unable to find the keyval in db. The caller should handle this error */
	pmi_dbg_printf("PMI_KVS_Get failed: '%s'\n", str);
	return PMI_FAIL;
    }
    if (MPIU_Str_get_string_arg(pmi_process.context->read_cmd.cmd, "value", value, length) != MPIU_STR_SUCCESS)
    {
	pmi_err_printf("PMI_KVS_Get failed: no value in the result command for the get: '%s'\n", pmi_process.context->read_cmd.cmd);
	return PMI_FAIL;
    }

    /*
    printf("iPMI_KVS_Get success.\n");fflush(stdout);
    printf("get <%s><%s><%s>\n", kvsname, key, value);
    fflush(stdout);
    */
    return PMI_SUCCESS;
}

int iPMI_KVS_Iter_first(const char kvsname[], char key[], int key_len, char value[], int val_len)
{
    int result;
    char str[1024];
    const char *kvsname_ = NULL;

    if (pmi_process.init_finalized == PMI_FINALIZED)
	return PMI_ERR_INIT;
    if (kvsname == NULL)
	return PMI_ERR_INVALID_ARG;
    if (key == NULL)
	return PMI_ERR_INVALID_KEY;
    if (key_len < PMI_MAX_KEY_LEN)
	return PMI_ERR_INVALID_KEY_LENGTH;
    if (value == NULL)
	return PMI_ERR_INVALID_VAL;
    if (val_len < PMI_MAX_VALUE_LEN)
	return PMI_ERR_INVALID_VAL_LENGTH;

    kvsname_ = kvsname;
    if (pmi_process.local_kvs)
    {
	result = smpd_dbs_first(kvsname_, key, value);
	return (result == SMPD_SUCCESS) ? PMI_SUCCESS : PMI_FAIL;
    }
    else{
        int len = 0;
        if((len = strlen(kvsname)) > PMI_MAX_KVS_NAME_LENGTH){
            return PMI_ERR_INVALID_LENGTH;
        }
        /* Update kvsname if the caller has the stale kvsname before
         * singleton init
         */
        if(strncmp(kvsname, pmi_process.kvs_name_singleton_nopm, len)
            == 0){
            kvsname_ = pmi_process.kvs_name;
        }
    }

    result = pmi_create_post_command("dbfirst", kvsname_, NULL, NULL);
    if (result != PMI_SUCCESS)
    {
	pmi_err_printf("PMI_KVS_Iter_first failed: unable to get the first key/value pair from '%s'\n", kvsname_);
	return PMI_FAIL;
    }

    /* parse the result of the command */
    if (MPIU_Str_get_string_arg(pmi_process.context->read_cmd.cmd, "result", str, 1024) != MPIU_STR_SUCCESS)
    {
	pmi_err_printf("PMI_KVS_Iter_first failed: no result string in the result command.\n");
	return PMI_FAIL;
    }
    if (strcmp(str, DBS_SUCCESS_STR))
    {
	pmi_err_printf("PMI_KVS_Iter_first failed: %s\n", str);
	return PMI_FAIL;
    }
    if (MPIU_Str_get_string_arg(pmi_process.context->read_cmd.cmd, "key", str, PMI_MAX_KEY_LEN) != MPIU_STR_SUCCESS)
    {
	pmi_err_printf("PMI_KVS_Iter_first failed: no key in the result command for the pmi iter_first: '%s'\n", pmi_process.context->read_cmd.cmd);
	return PMI_FAIL;
    }
    if (strcmp(str, DBS_END_STR) == 0)
    {
	*key = '\0';
	*value = '\0';
	return PMI_SUCCESS;
    }
    strcpy(key, str);
    if (MPIU_Str_get_string_arg(pmi_process.context->read_cmd.cmd, "value", value, PMI_MAX_VALUE_LEN) != MPIU_STR_SUCCESS)
    {
	pmi_err_printf("PMI_KVS_Iter_first failed: no value in the result command for the pmi iter_first: '%s'\n", pmi_process.context->read_cmd.cmd);
	return PMI_FAIL;
    }

    return PMI_SUCCESS;
}

int iPMI_KVS_Iter_next(const char kvsname[], char key[], int key_len, char value[], int val_len)
{
    int result;
    char str[1024];
    const char *kvsname_ = NULL;

    if (pmi_process.init_finalized == PMI_FINALIZED)
	return PMI_ERR_INIT;
    if (kvsname == NULL)
	return PMI_ERR_INVALID_ARG;
    if (key == NULL)
	return PMI_ERR_INVALID_KEY;
    if (key_len < PMI_MAX_KEY_LEN)
	return PMI_ERR_INVALID_KEY_LENGTH;
    if (value == NULL)
	return PMI_ERR_INVALID_VAL;
    if (val_len < PMI_MAX_VALUE_LEN)
	return PMI_ERR_INVALID_VAL_LENGTH;

    kvsname_ = kvsname;

    if (pmi_process.local_kvs)
    {
	result = smpd_dbs_next(kvsname_, key, value);
	return (result == SMPD_SUCCESS) ? PMI_SUCCESS : PMI_FAIL;
    }
    else{
        int len = 0;
        if((len = strlen(kvsname)) > PMI_MAX_KVS_NAME_LENGTH){
            return PMI_ERR_INVALID_LENGTH;
        }
        /* Update kvsname if the caller has the stale kvsname before
         * singleton init
         */
        if(strncmp(kvsname, pmi_process.kvs_name_singleton_nopm, len)
            == 0){
            kvsname_ = pmi_process.kvs_name;
        }
    }

    result = pmi_create_post_command("dbnext", kvsname_, NULL, NULL);
    if (result != PMI_SUCCESS)
    {
	pmi_err_printf("PMI_KVS_Iter_next failed: unable to get the next key/value pair from '%s'\n", kvsname_);
	return PMI_FAIL;
    }

    /* parse the result of the command */
    if (MPIU_Str_get_string_arg(pmi_process.context->read_cmd.cmd, "result", str, 1024) != MPIU_STR_SUCCESS)
    {
	pmi_err_printf("PMI_KVS_Iter_next failed: no result string in the result command.\n");
	return PMI_FAIL;
    }
    if (strcmp(str, DBS_SUCCESS_STR))
    {
	pmi_err_printf("PMI_KVS_Iter_next failed: %s\n", str);
	return PMI_FAIL;
    }
    if (MPIU_Str_get_string_arg(pmi_process.context->read_cmd.cmd, "key", str, PMI_MAX_KEY_LEN) != MPIU_STR_SUCCESS)
    {
	pmi_err_printf("PMI_KVS_Iter_next failed: no key in the result command for the pmi iter_next: '%s'\n", pmi_process.context->read_cmd.cmd);
	return PMI_FAIL;
    }
    if (strcmp(str, DBS_END_STR) == 0)
    {
	*key = '\0';
	*value = '\0';
	return PMI_SUCCESS;
    }
    strcpy(key, str);
    if (MPIU_Str_get_string_arg(pmi_process.context->read_cmd.cmd, "value", value, PMI_MAX_VALUE_LEN) != MPIU_STR_SUCCESS)
    {
	pmi_err_printf("PMI_KVS_Iter_next failed: no value in the result command for the pmi iter_next: '%s'\n", pmi_process.context->read_cmd.cmd);
	return PMI_FAIL;
    }

    return PMI_SUCCESS;
}

int iPMI_Spawn_multiple(int count,
                       const char * cmds[],
                       const char ** argvs[],
                       const int maxprocs[],
                       const int cinfo_keyval_sizes[],
                       const PMI_keyval_t * info_keyval_vectors[],
                       int preput_keyval_size,
                       const PMI_keyval_t preput_keyval_vector[],
                       int errors[])
{
    int result;
    smpd_command_t *cmd_ptr;
    int dest = 0;
    char buffer[SMPD_MAX_CMD_LENGTH];
    char keyval_buf[SMPD_MAX_CMD_LENGTH];
    char key[100];
    char *iter, *iter2;
    int i, j, maxlen, maxlen2;
    int path_specified = 0, wdir_specified = 0;
    char path[SMPD_MAX_PATH_LENGTH] = "";
    int *info_keyval_sizes;
    int total_num_processes;
    int appnum = 0;

    if (pmi_process.init_finalized == PMI_FINALIZED)
	return PMI_ERR_INIT;
    if(pmi_process.init_finalized == PMI_SINGLETON_INIT_BUT_NO_PM){
        if(PMIi_InitSingleton() != PMI_SUCCESS){
            return PMI_ERR_INIT;
        }
    }

    if (count < 1 || cmds == NULL || maxprocs == NULL || preput_keyval_size < 0)
	return PMI_ERR_INVALID_ARG;

    if (pmi_process.local_kvs)
    {
	return PMI_FAIL;
    }

    /*printf("creating spawn command.\n");fflush(stdout);*/
    result = smpd_create_command("spawn", pmi_process.smpd_id, dest, SMPD_TRUE, &cmd_ptr);
    if (result != SMPD_SUCCESS)
    {
	pmi_err_printf("unable to create a spawn command.\n");
	return PMI_FAIL;
    }
    result = smpd_add_command_int_arg(cmd_ptr, "ctx_key", pmi_process.smpd_key);
    if (result != SMPD_SUCCESS)
    {
	pmi_err_printf("unable to add the key to the spawn command.\n");
	return PMI_FAIL;
    }

    /* add the number of commands */
    result = smpd_add_command_int_arg(cmd_ptr, "ncmds", count);
    if (result != SMPD_SUCCESS)
    {
	pmi_err_printf("unable to add the ncmds field to the spawn command.\n");
	return PMI_FAIL;
    }
    /* add the commands and their argv arrays */
    for (i=0; i<count; i++)
    {
	sprintf(key, "cmd%d", i);
#ifdef HAVE_WINDOWS_H
	if (strlen(cmds[i]) > 2)
	{
	    if (cmds[i][0] == '.' && cmds[i][1] == '/')
	    {
		result = smpd_add_command_arg(cmd_ptr, key, (char*)&cmds[i][2]);
	    }
	    else
	    {
		result = smpd_add_command_arg(cmd_ptr, key, (char*)cmds[i]);
	    }
	}
	else
	{
	    result = smpd_add_command_arg(cmd_ptr, key, (char*)cmds[i]);
	}
#else
	result = smpd_add_command_arg(cmd_ptr, key, (char*)cmds[i]);
#endif
	if (result != SMPD_SUCCESS)
	{
	    pmi_err_printf("unable to add %s(%s) to the spawn command.\n", key, cmds[i]);
	    return PMI_FAIL;
	}
	if (argvs)
	{
	    buffer[0] = '\0';
	    iter = buffer;
	    maxlen = SMPD_MAX_CMD_LENGTH;
	    if (argvs[i] != NULL)
	    {
		for (j=0; argvs[i][j] != NULL; j++)
		{
		    result = MPIU_Str_add_string(&iter, &maxlen, argvs[i][j]);
		}
		if (iter > buffer)
		{
		    iter--;
		    *iter = '\0'; /* erase the trailing space */
		}
	    }
	    sprintf(key, "argv%d", i);
	    result = smpd_add_command_arg(cmd_ptr, key, buffer);
	    if (result != SMPD_SUCCESS)
	    {
		pmi_err_printf("unable to add %s(%s) to the spawn command.\n", key, buffer);
		return PMI_FAIL;
	    }
	}
    }
    /* add the maxprocs array and count the total number of processes */
    total_num_processes = 0;
    buffer[0] = '\0';
    for (i=0; i<count; i++)
    {
	total_num_processes += maxprocs[i];
	if (i < count-1)
	    sprintf(key, "%d ", maxprocs[i]);
	else
	    sprintf(key, "%d", maxprocs[i]);
	strcat(buffer, key);
    }
    result = smpd_add_command_arg(cmd_ptr, "maxprocs", buffer);
    if (result != SMPD_SUCCESS)
    {
	pmi_err_printf("unable to add maxprocs(%s) to the spawn command.\n", buffer);
	return PMI_FAIL;
    }

#ifdef HAVE_WINDOWS_H
    {
	HMODULE hModule;
	char exe_path[SMPD_MAX_PATH_LENGTH];
	char *iter;
	int length;

	GetCurrentDirectory(SMPD_MAX_PATH_LENGTH, path);
	hModule = GetModuleHandle(NULL);
	if (GetModuleFileName(hModule, exe_path, SMPD_MAX_PATH_LENGTH)) 
	{
	    iter = strrchr(exe_path, '\\');
	    if (iter != NULL)
	    {
		if (iter == (exe_path + 2) && *(iter-1) == ':')
		{
		    /* leave the \ if the path is at the root, like c:\foo.exe */
		    iter++;
		}
		*iter = '\0'; /* erase the file name leaving only the path */
	    }
	    length = (int)strlen(path);
	    iter = &path[length];
	    MPIU_Snprintf(iter, SMPD_MAX_PATH_LENGTH-length, ";%s", exe_path);
	}
    }
#else
    getcwd(path, SMPD_MAX_PATH_LENGTH);
#endif

    /* create a copy of the sizes so we can change the values locally */
    info_keyval_sizes = (int*)MPIU_Malloc(count * sizeof(int));
    if (info_keyval_sizes == NULL)
    {
	pmi_err_printf("unable to allocate an array of kevval sizes.\n");
	return PMI_FAIL;
    }
    for (i=0; i<count; i++)
    {
	info_keyval_sizes[i] = cinfo_keyval_sizes[i];
    }

    /* add the keyvals */
    if (info_keyval_sizes && info_keyval_vectors){
        for (i=0; i<count; i++){
            path_specified = 0;
            wdir_specified = 0;
            buffer[0] = '\0';
            iter = buffer;
            maxlen = SMPD_MAX_CMD_LENGTH;
	    
            for (j=0; j<info_keyval_sizes[i]; j++){
                keyval_buf[0] = '\0';
                iter2 = keyval_buf;
                maxlen2 = SMPD_MAX_CMD_LENGTH;
                if (strcmp(info_keyval_vectors[i][j].key, "path") == 0){
                    size_t val2len;
                    char *val2;
                    val2len = sizeof(char) * strlen(info_keyval_vectors[i][j].val) + 1 + strlen(path) + 1;
                    val2 = (char*)MPIU_Malloc(val2len);
                    if (val2 == NULL){
                        pmi_err_printf("unable to allocate memory for the path key.\n");
                        return PMI_FAIL;
                    }
                    /*printf("creating path %d: <%s>;<%s>\n", val2len, info_keyval_vectors[i][j].val, path);fflush(stdout);*/
                    MPIU_Snprintf(val2, val2len, "%s;%s", info_keyval_vectors[i][j].val, path);
                    result = MPIU_Str_add_string_arg(&iter2, &maxlen2, info_keyval_vectors[i][j].key, val2);
                    if (result != MPIU_STR_SUCCESS){
                        pmi_err_printf("unable to add %s=%s to the spawn command.\n", info_keyval_vectors[i][j].key, val2);
                        MPIU_Free(val2);
                        return PMI_FAIL;
                    }
                    MPIU_Free(val2);
                    path_specified = 1;
                }
                else{
                    if(strcmp(info_keyval_vectors[i][j].key, "wdir") == 0){
                        wdir_specified = 1;
                    }
                    result = MPIU_Str_add_string_arg(&iter2, &maxlen2, info_keyval_vectors[i][j].key, info_keyval_vectors[i][j].val);
                    if (result != MPIU_STR_SUCCESS){
                        pmi_err_printf("unable to add %s=%s to the spawn command.\n", info_keyval_vectors[i][j].key, info_keyval_vectors[i][j].val);
                        return PMI_FAIL;
                    }
                }
                if (iter2 > keyval_buf){
                    iter2--;
                    *iter2 = '\0'; /* remove the trailing space */
                }
                sprintf(key, "%d", j);
                result = MPIU_Str_add_string_arg(&iter, &maxlen, key, keyval_buf);
                if (result != MPIU_STR_SUCCESS){
                    pmi_err_printf("unable to add %s=%s to the spawn command.\n", key, keyval_buf);
                    return PMI_FAIL;
                }
            }
            /* add the current directory as the default path if a path has not been specified */
            if (!path_specified){
                keyval_buf[0] = '\0';
                iter2 = keyval_buf;
                maxlen2 = SMPD_MAX_CMD_LENGTH;
                result = MPIU_Str_add_string_arg(&iter2, &maxlen2, "path", path);
                iter2--;
                *iter2 = '\0';
                sprintf(key, "%d", j++);
                result = MPIU_Str_add_string_arg(&iter, &maxlen, key, keyval_buf);
                if (result != MPIU_STR_SUCCESS){
                    pmi_err_printf("unable to add %s=%s to the spawn command.\n", key, keyval_buf);
                    return PMI_FAIL;
                }
                info_keyval_sizes[i]++;
            }
            if(!wdir_specified){
                char wdir[SMPD_MAX_DIR_LENGTH];
                if(getcwd(wdir, SMPD_MAX_DIR_LENGTH)){
                    keyval_buf[0] = '\0';
                    iter2 = keyval_buf;
                    maxlen2 = SMPD_MAX_CMD_LENGTH;
                    result = MPIU_Str_add_string_arg(&iter2, &maxlen2, "wdir", wdir);
                    if(result != MPIU_STR_SUCCESS){
                        pmi_err_printf("Unable to add wdir to keyval_buf\n");
                        return PMI_FAIL;
                    }
                    *(--iter2) = '\0';
                    sprintf(key, "%d", j++);
                    result = MPIU_Str_add_string_arg(&iter, &maxlen, key, keyval_buf);
                    if(result != MPIU_STR_SUCCESS){
                        pmi_err_printf("unable to add %s=%s to the spawn command\n", key, keyval_buf);
                        return PMI_FAIL;
                    }
                    info_keyval_sizes[i]++;
                }
            }
#ifdef HAVE_WINDOWS_H
            /* FIXME: We don't support user environment infos for spawn() */
            if(pmi_process.rpmi == PMI_TRUE){
                /* Add channel environment for rpmi/singleton_init procs */
                char *env, env_str[SMPD_MAX_ENV_LENGTH];
                env = getenv("MPICH2_CHANNEL");
                if(env != NULL){
                    snprintf(env_str, SMPD_MAX_ENV_LENGTH, "MPICH2_CHANNEL=%s", env);
                    keyval_buf[0] = '\0';
                    iter2 = keyval_buf;
                    maxlen2 = SMPD_MAX_CMD_LENGTH;
                    result = MPIU_Str_add_string_arg(&iter2, &maxlen2, "env", env_str);
                    iter2--;
                    *iter2 = '\0';
                    sprintf(key, "%d", j++);
                    result = MPIU_Str_add_string_arg(&iter, &maxlen, key, keyval_buf);
                    if (result != MPIU_STR_SUCCESS){
                        pmi_err_printf("unable to add %s=%s to the spawn command.\n", key, keyval_buf);
                        return PMI_FAIL;
                    }
                    info_keyval_sizes[i]++;
                }
            }
#endif
            if (iter != buffer){
                iter--;
                *iter = '\0'; /* remove the trailing space */
            }
            sprintf(key, "keyvals%d", i);
            result = smpd_add_command_arg(cmd_ptr, key, buffer);
            if (result != SMPD_SUCCESS){
                pmi_err_printf("unable to add %s(%s) to the spawn command.\n", key, buffer);
                return PMI_FAIL;
            }
        }
    }
    else
    {
	if (!info_keyval_sizes)
	{
	    buffer[0] = '\0';
	    for (i=0; i<count; i++)
	    {
		if (i < count-1)
		    strcat(buffer, "1 ");
		else
		    strcat(buffer, "1");
	    }
	    result = smpd_add_command_arg(cmd_ptr, "nkeyvals", buffer);
	    if (result != SMPD_SUCCESS)
	    {
		pmi_err_printf("unable to add nkeyvals(%s) to the spawn command.\n", buffer);
		return PMI_FAIL;
	    }
	}
	for (i=0; i<count; i++)
	{
	    buffer[0] = '\0';
	    iter = buffer;
	    maxlen = SMPD_MAX_CMD_LENGTH;
	    /* add the current directory as the default path if a path has not been specified */
	    keyval_buf[0] = '\0';
	    iter2 = keyval_buf;
	    maxlen2 = SMPD_MAX_CMD_LENGTH;
	    result = MPIU_Str_add_string_arg(&iter2, &maxlen2, "path", path);
	    iter2--;
	    *iter2 = '\0';
	    strcpy(key, "0");
	    result = MPIU_Str_add_string_arg(&iter, &maxlen, key, keyval_buf);
	    if (result != MPIU_STR_SUCCESS)
	    {
		pmi_err_printf("unable to add %s=%s to the spawn command.\n", key, keyval_buf);
		return PMI_FAIL;
	    }
	    sprintf(key, "keyvals%d", i);
	    result = smpd_add_command_arg(cmd_ptr, key, buffer);
	    if (result != SMPD_SUCCESS)
	    {
		pmi_err_printf("unable to add %s(%s) to the spawn command.\n", key, buffer);
		return PMI_FAIL;
	    }
	}
    }

    /* add the keyval sizes array */
    if (info_keyval_sizes)
    {
	buffer[0] = '\0';
	for (i=0; i<count; i++)
	{
	    if (i < count-1)
		sprintf(key, "%d ", info_keyval_sizes[i] > 0 ? info_keyval_sizes[i] : 1);
	    else
		sprintf(key, "%d", info_keyval_sizes[i] > 0 ? info_keyval_sizes[i] : 1);
	    strcat(buffer, key);
	}
	result = smpd_add_command_arg(cmd_ptr, "nkeyvals", buffer);
	if (result != SMPD_SUCCESS)
	{
	    pmi_err_printf("unable to add nkeyvals(%s) to the spawn command.\n", buffer);
	    return PMI_FAIL;
	}
    }

    MPIU_Free(info_keyval_sizes);

    /* add the pre-put keyvals */
    result = smpd_add_command_int_arg(cmd_ptr, "npreput", preput_keyval_size);
    if (result != SMPD_SUCCESS)
    {
	pmi_err_printf("unable to add npreput=%d to the spawn command.\n", preput_keyval_size);
	return PMI_FAIL;
    }
    if (preput_keyval_size > 0 && preput_keyval_vector)
    {
	buffer[0] = '\0';
	iter = buffer;
	maxlen = SMPD_MAX_CMD_LENGTH;
	for (i=0; i<preput_keyval_size; i++)
	{
	    keyval_buf[0] = '\0';
	    iter2 = keyval_buf;
	    maxlen2 = SMPD_MAX_CMD_LENGTH;
	    result = MPIU_Str_add_string_arg(&iter2, &maxlen2, preput_keyval_vector[i].key, preput_keyval_vector[i].val);
	    if (result != MPIU_STR_SUCCESS)
	    {
		pmi_err_printf("unable to add %s=%s to the spawn command.\n", preput_keyval_vector[i].key, preput_keyval_vector[i].val);
		return PMI_FAIL;
	    }
	    if (iter2 > keyval_buf)
	    {
		iter2--;
		*iter2 = '\0'; /* remove the trailing space */
	    }
	    sprintf(key, "%d", i);
	    result = MPIU_Str_add_string_arg(&iter, &maxlen, key, keyval_buf);
	    if (result != MPIU_STR_SUCCESS)
	    {
		pmi_err_printf("unable to add %s=%s to the spawn command.\n", key, keyval_buf);
		return PMI_FAIL;
	    }
	}
	result = smpd_add_command_arg(cmd_ptr, "preput", buffer);
	if (result != SMPD_SUCCESS)
	{
	    pmi_err_printf("unable to add preput(%s) to the spawn command.\n", buffer);
	    return PMI_FAIL;
	}
    }	

    /*printf("spawn command: <%s>\n", cmd_ptr->cmd);*/

    /* post the write of the command */
    /*
    printf("posting write of spawn command to %s context, sock %d: '%s'\n",
	smpd_get_context_str(pmi_process.context), SMPDU_Sock_get_sock_id(pmi_process.context->sock), cmd_ptr->cmd);
    fflush(stdout);
    */
    
    result = smpd_post_write_command(pmi_process.context, cmd_ptr);
    if (result != SMPD_SUCCESS)
    {
	pmi_err_printf("unable to post a write of the spawn command.\n");
	return PMI_FAIL;
    }

    /* post a read for the result*/
    result = smpd_post_read_command(pmi_process.context);
    if (result != SMPD_SUCCESS)
    {
	pmi_err_printf("unable to post a read of the next command on the pmi context.\n");
	return PMI_FAIL;
    }

    /* let the state machine send the command and receive the result */
    result = smpd_enter_at_state(pmi_process.set, SMPD_WRITING_CMD);
    if (result != SMPD_SUCCESS)
    {
	/*printf("PMI_Spawn_multiple returning failure.\n");fflush(stdout);*/
	pmi_err_printf("the state machine logic failed to get the result of the spawn command.\n");
	return PMI_FAIL;
    }

    for (i=0; i<total_num_processes; i++)
    {
	errors[i] = PMI_SUCCESS;
    }
    /*printf("PMI_Spawn_multiple returning success.\n");fflush(stdout);*/
    return PMI_SUCCESS;
}

int iPMI_Parse_option(int num_args, char *args[], int *num_parsed, PMI_keyval_t **keyvalp, int *size)
{
    if (num_args < 1)
	return PMI_ERR_INVALID_NUM_ARGS;
    if (args == NULL)
	return PMI_ERR_INVALID_ARGS;
    if (num_parsed == NULL)
	return PMI_ERR_INVALID_NUM_PARSED;
    if (keyvalp == NULL)
	return PMI_ERR_INVALID_KEYVALP;
    if (size == NULL)
	return PMI_ERR_INVALID_SIZE;
    *num_parsed = 0;
    *keyvalp = NULL;
    *size = 0;
    return PMI_SUCCESS;
}

int iPMI_Args_to_keyval(int *argcp, char *((*argvp)[]), PMI_keyval_t **keyvalp, int *size)
{
    if (argcp == NULL || argvp == NULL || keyvalp == NULL || size == NULL)
	return PMI_ERR_INVALID_ARG;
    return PMI_SUCCESS;
}

int iPMI_Free_keyvals(PMI_keyval_t keyvalp[], int size)
{
    if (keyvalp == NULL || size < 0)
	return PMI_ERR_INVALID_ARG;
    if (size == 0)
	return PMI_SUCCESS;
    /* free stuff */
    return PMI_SUCCESS;
}

static char * namepub_kvs = NULL;
static int setup_name_service()
{
    int result;
    char *pmi_namepub_kvs;

    if (namepub_kvs != NULL)
    {
	/* FIXME: Should it be an error to call setup_name_service twice? */
	MPIU_Free(namepub_kvs);
    }

    namepub_kvs = (char*)MPIU_Malloc(PMI_MAX_KVS_NAME_LENGTH);
    if (!namepub_kvs)
    {
	pmi_err_printf("unable to allocate memory for the name publisher kvs.\n");
	return PMI_FAIL;
    }

    pmi_namepub_kvs = getenv("PMI_NAMEPUB_KVS");
    if (pmi_namepub_kvs)
    {
	strncpy(namepub_kvs, pmi_namepub_kvs, PMI_MAX_KVS_NAME_LENGTH);
    }
    else
    {
	/*result = PMI_KVS_Create(namepub_kvs, PMI_MAX_KVS_NAME_LENGTH);*/
	result = iPMI_Get_kvs_domain_id(namepub_kvs, PMI_MAX_KVS_NAME_LENGTH);
	if (result != PMI_SUCCESS)
	{
	    pmi_err_printf("unable to get the name publisher kvs name.\n");
	    return result;
	}
    }

    /*printf("namepub kvs: <%s>\n", namepub_kvs);fflush(stdout);*/
    return PMI_SUCCESS;
}

int iPMI_Publish_name( const char service_name[], const char port[] )
{
    int result;
    if (service_name == NULL || port == NULL)
	return PMI_ERR_INVALID_ARG;
    if (namepub_kvs == NULL)
    {
	result = setup_name_service();
	if (result != PMI_SUCCESS)
	    return result;
    }
    /*printf("publish kvs: <%s>\n", namepub_kvs);fflush(stdout);*/
    result = iPMI_KVS_Put(namepub_kvs, service_name, port);
    if (result != PMI_SUCCESS)
    {
	pmi_err_printf("unable to put the service name and port into the name publisher kvs.\n");
	return result;
    }
    result = iPMI_KVS_Commit(namepub_kvs);
    if (result != PMI_SUCCESS)
    {
	pmi_err_printf("unable to commit the name publisher kvs.\n");
	return result;
    }
    return PMI_SUCCESS;
}

int iPMI_Unpublish_name( const char service_name[] )
{
    int result;
    if (service_name == NULL)
	return PMI_ERR_INVALID_ARG;
    if (namepub_kvs == NULL)
    {
	result = setup_name_service();
	if (result != PMI_SUCCESS)
	    return result;
    }
    /*printf("unpublish kvs: <%s>\n", namepub_kvs);fflush(stdout);*/
    /* This assumes you can put the same key more than once which breaks the PMI specification */
    result = iPMI_KVS_Put(namepub_kvs, service_name, "");
    if (result != PMI_SUCCESS)
    {
	pmi_err_printf("unable to put the blank service name and port into the name publisher kvs.\n");
	return result;
    }
    result = iPMI_KVS_Commit(namepub_kvs);
    if (result != PMI_SUCCESS)
    {
	pmi_err_printf("unable to commit the name publisher kvs.\n");
	return result;
    }
    return PMI_SUCCESS;
}

int iPMI_Lookup_name( const char service_name[], char port[] )
{
    int result;
    if (service_name == NULL || port == NULL)
	return PMI_ERR_INVALID_ARG;
    if (namepub_kvs == NULL)
    {
	result = setup_name_service();
	if (result != PMI_SUCCESS)
	    return result;
    }
    /*printf("lookup kvs: <%s>\n", namepub_kvs);fflush(stdout);*/
    silence = 1;
    result = iPMI_KVS_Get(namepub_kvs, service_name, port, MPI_MAX_PORT_NAME);
    silence = 0;
    if (result != PMI_SUCCESS)
    {
	/* fail silently */
	/*pmi_err_printf("unable to get the service name and port from the name publisher kvs.\n");*/
	return result;
    }

    if (port[0] == '\0')
    {
	return MPI_ERR_NAME;
    }
    return PMI_SUCCESS;
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

int PMIX_Start_root_smpd(int nproc, char *host, int len, int *port)
{
#ifdef HAVE_WINDOWS_H
    DWORD dwLength = len;
#else
    int pipe_fd[2];
    int result;
#endif

    pmi_process.nproc = nproc;

#ifdef HAVE_WINDOWS_H
    pmi_process.hRootThreadReadyEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (pmi_process.hRootThreadReadyEvent == NULL)
    {
	pmi_err_printf("unable to create the root listener synchronization event, error: %d\n", GetLastError());
	return PMI_FAIL;
    }
    pmi_process.hRootThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)root_smpd, NULL, 0, NULL);
    if (pmi_process.hRootThread == NULL)
    {
	pmi_err_printf("unable to create the root listener thread: error %d\n", GetLastError());
	return PMI_FAIL;
    }
    if (WaitForSingleObject(pmi_process.hRootThreadReadyEvent, 60000) != WAIT_OBJECT_0)
    {
	pmi_err_printf("the root process thread failed to initialize.\n");
	return PMI_FAIL;
    }
    /*GetComputerName(host, &dwLength);*/
    GetComputerNameEx(ComputerNameDnsFullyQualified, host, &dwLength);
#else
    pipe(pipe_fd);
    result = fork();
    if (result == -1)
    {
	pmi_err_printf("unable to fork the root listener, errno %d\n", errno);
	return PMI_FAIL;
    }
    if (result == 0)
    {
	close(pipe_fd[0]); /* close the read end of the pipe */
	result = root_smpd(&pipe_fd[1]);
	exit(result);
    }

    /* close the write end of the pipe */
    close(pipe_fd[1]);
    /* read the port from the root_smpd process */
    readbuf(pipe_fd[0], &pmi_process.root_port, sizeof(int));
    /* read the kvs name */
    readbuf(pipe_fd[0], smpd_process.kvs_name, SMPD_MAX_DBS_NAME_LEN);
    /* close the read end of the pipe */
    close(pipe_fd[0]);
    pmi_process.root_pid = result;
    gethostname(host, len);
#endif

    *port = pmi_process.root_port;

    return PMI_SUCCESS;
}

int PMIX_Stop_root_smpd()
{
#ifdef HAVE_WINDOWS_H
    DWORD result;
#else
    int status;
#endif

#ifdef HAVE_WINDOWS_H
    result = WaitForSingleObject(pmi_process.hRootThread, INFINITE);
    if (result != WAIT_OBJECT_0)
    {
	return PMI_FAIL;
    }
#else
    kill(pmi_process.root_pid, SIGKILL);
    /*
    if (waitpid(pmi_process.root_pid, &status, WUNTRACED) == -1)
    {
	return PMI_FAIL;
    }
    */
#endif
    return PMI_SUCCESS;
}

/* FIXME: Why is this func defined here ? 
 * - shouldn't this be in smpd_util*.lib ?
 */
static int root_smpd(void *p)
{
    int result;
    SMPDU_Sock_set_t set;
    SMPDU_Sock_t listener;
    smpd_process_group_t *pg;
    int i;
#ifndef HAVE_WINDOWS_H
    int send_kvs = 0;
    int pipe_fd;
#endif

    /* unreferenced parameter */
    SMPD_UNREFERENCED_ARG(p);

    smpd_process.id = 1;
    smpd_process.root_smpd = SMPD_FALSE;
    smpd_process.map0to1 = SMPD_TRUE;

    result = SMPDU_Sock_create_set(&set);
    if (result != SMPD_SUCCESS)
    {
	pmi_mpi_err_printf(result, "SMPDU_Sock_create_set failed.\n");
	return PMI_FAIL;
    }
    smpd_process.set = set;
    smpd_dbg_printf("created a set for the listener: %d\n", SMPDU_Sock_get_sock_set_id(set));
    result = SMPDU_Sock_listen(set, NULL, &pmi_process.root_port, &listener); 
    if (result != SMPD_SUCCESS)
    {
	pmi_mpi_err_printf(result, "SMPDU_Sock_listen failed.\n");
	return PMI_FAIL;
    }
    smpd_dbg_printf("smpd listening on port %d\n", pmi_process.root_port);

    result = smpd_create_context(SMPD_CONTEXT_LISTENER, set, listener, -1, &smpd_process.listener_context);
    if (result != SMPD_SUCCESS)
    {
	pmi_err_printf("unable to create a context for the smpd listener.\n");
	return PMI_FAIL;
    }
    result = SMPDU_Sock_set_user_ptr(listener, smpd_process.listener_context);
    if (result != SMPD_SUCCESS)
    {
	pmi_mpi_err_printf(result, "SMPDU_Sock_set_user_ptr failed.\n");
	return PMI_FAIL;
    }
    smpd_process.listener_context->state = SMPD_SMPD_LISTENING;

    smpd_dbs_init();
    smpd_process.have_dbs = SMPD_TRUE;
    if (smpd_process.kvs_name[0] != '\0')
    {
	result = smpd_dbs_create_name_in(smpd_process.kvs_name);
    }
    else
    {
	result = smpd_dbs_create(smpd_process.kvs_name);
#ifndef HAVE_WINDOWS_H
	send_kvs = 1;
#endif
    }
    if (result != SMPD_DBS_SUCCESS)
    {
	pmi_err_printf("unable to create a kvs database: name = <%s>.\n", smpd_process.kvs_name);
	return PMI_FAIL;
    }

    /* Set up the process group */
    /* initialize a new process group structure */
    pg = (smpd_process_group_t*)MPIU_Malloc(sizeof(smpd_process_group_t));
    if (pg == NULL)
    {
	pmi_err_printf("unable to allocate memory for a process group structure.\n");
	return PMI_FAIL;
    }
    pg->aborted = SMPD_FALSE;
    pg->any_init_received = SMPD_FALSE;
    pg->any_noinit_process_exited = SMPD_FALSE;
    strncpy(pg->kvs, smpd_process.kvs_name, SMPD_MAX_DBS_NAME_LEN);
    pg->num_procs = pmi_process.nproc;
    pg->processes = (smpd_exit_process_t*)MPIU_Malloc(pmi_process.nproc * sizeof(smpd_exit_process_t));
    if (pg->processes == NULL)
    {
	pmi_err_printf("unable to allocate an array of %d process exit structures.\n", pmi_process.nproc);
	return PMI_FAIL;
    }
    for (i=0; i<pmi_process.nproc; i++)
    {
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
    SetEvent(pmi_process.hRootThreadReadyEvent);
#else
    if (p != NULL)
    {
	pipe_fd = *(int*)p;
	/* send the root port back over the pipe */
	writebuf(pipe_fd, &pmi_process.root_port, sizeof(int));
	if (send_kvs)
	{
	    writebuf(pipe_fd, smpd_process.kvs_name, SMPD_MAX_DBS_NAME_LEN);
	}  
	close(pipe_fd);
    }
#endif

    result = smpd_enter_at_state(set, SMPD_SMPD_LISTENING);
    if (result != SMPD_SUCCESS)
    {
	pmi_err_printf("root_smpd state machine failed.\n");
	return PMI_FAIL;
    }

    result = SMPDU_Sock_destroy_set(set);
    if (result != SMPD_SUCCESS)
    {
	pmi_mpi_err_printf(result, "unable to destroy the set.\n");
    }

    return PMI_SUCCESS;
}
