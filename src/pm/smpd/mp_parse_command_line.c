/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include "mpiexec.h"
#include "smpd.h"
#include <stdlib.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#if defined(HAVE_DIRECT_H) || defined(HAVE_WINDOWS_H)
#include <direct.h>
#endif
#ifdef HAVE_WINDOWS_H
#include <crtdbg.h>
#endif
#ifdef HAVE_CTYPE_H
#include <ctype.h>
#endif

/* The supported values right now are "newtcp" & "nd" */
#define SMPD_MAX_NEMESIS_NETMOD_LENGTH  10

void mp_print_options(void)
{
    printf("\n");
    printf("Usage:\n");
    printf("mpiexec -n <maxprocs> [options] executable [args ...]\n");
    printf("mpiexec [options] executable [args ...] : [options] exe [args] : ...\n");
    printf("mpiexec -configfile <configfile>\n");
    printf("\n");
    printf("options:\n");
    printf("\n");
    printf("standard:\n");
    printf("-n <maxprocs>\n");
    printf("-wdir <working directory>\n");
    printf("-configfile <filename> -\n");
    printf("       each line contains a complete set of mpiexec options\n");
    printf("       including the executable and arguments\n");
    printf("-host <hostname>\n");
    /*
    printf("-soft <Fortran90 triple> - acceptable number of processes up to maxprocs\n");
    printf("       a or a:b or a:b:c where\n");
    printf("       1) a = a\n");
    printf("       2) a:b = a, a+1, a+2, ..., b\n");
    printf("       3) a:b:c = a, a+c, a+2c, a+3c, ..., a+kc\n");
    printf("          where a+kc <= b if c>0\n");
    printf("                a+kc >= b if c<0\n");
    */
    printf("-path <search path for executable, ; separated>\n");
    /*printf("-arch <architecture> - sun, linux, rs6000, ...\n");*/
    printf("\n");
    printf("extensions:\n");
    printf("-env <variable value>\n");
    printf("-hosts <n host1 host2 ... hostn>\n");
    printf("-hosts <n host1 m1 host2 m2 ... hostn mn>\n");
    printf("-machinefile <filename> - one host per line, #commented\n");
    printf("-localonly <numprocs>\n");
    printf("-exitcodes - print the exit codes of processes as they exit\n");
    /*
    printf("-genvall - pass all env vars in current environment\n");
    printf("-genvnone - pass no env vars\n");
    */
    printf("-genvlist <list of env var names a,b,c,...> - pass current values of these vars\n");
    printf("-g<local arg name> - global version of local options\n");
    printf("  genv, gwdir, ghost, gpath, gmap\n");
    printf("-file <filename> - old mpich1 job configuration file\n");
    printf("\n");
    printf("examples:\n");
    printf("mpiexec -n 4 cpi\n");
    printf("mpiexec -n 1 -host foo master : -n 8 worker\n");
    printf("\n");
    printf("For a list of all mpiexec options, execute 'mpiexec -help2'\n");
}

void mp_print_extra_options(void)
{
    printf("\n");
    printf("All options to mpiexec:\n");
    printf("\n");
    printf("-n x\n");
    printf("-np x\n");
    printf("  launch x processes\n");
    printf("-localonly x\n");
    printf("-n x -localonly\n");
    printf("  launch x processes on the local machine\n");
    printf("-machinefile filename\n");
    printf("  use a file to list the names of machines to launch on\n");
    printf("-host hostname\n");
    printf("-hosts n host1 host2 ... hostn\n");
    printf("-hosts n host1 m1 host2 m2 ... hostn mn\n");
    printf("  launch on the specified hosts\n");
    printf("  In the second version the number of processes = m1 + m2 + ... + mn\n");
    printf("-binding proc_binding_scheme\n");
    printf("  Set the proc binding for each of the launched processes to a single core.\n");
    printf("  Currently \"auto\" and \"user\" are supported as the proc_binding_schemes \n"); 
    printf("-map drive:\\\\host\\share\n");
    printf("  map a drive on all the nodes\n");
    printf("  this mapping will be removed when the processes exit\n");
    printf("-mapall\n");
    printf("  map all of the current network drives\n");
    printf("  this mapping will be removed when the processes exit\n");
    printf("  (Available currently only on windows)\n");
    printf("-dir drive:\\my\\working\\directory\n");
    printf("-wdir /my/working/directory\n");
    printf("  launch processes in the specified directory\n");
    printf("-env var val\n");
    printf("  set environment variable before launching the processes\n");
    printf("-logon\n");
    printf("  prompt for user account and password\n");
    printf("-pwdfile filename\n");
    printf("  read the account and password from the file specified\n");
    printf("  put the account on the first line and the password on the second\n");
    /*
    printf("-nocolor\n");
    printf("  don't use process specific output coloring\n");
    */
    printf("-nompi\n");
    printf("  launch processes without the mpi startup mechanism\n");
    /*
    printf("-nomapping\n");
    printf("  don't try to map the current directory on the remote nodes\n");
    */
    printf("-nopopup_debug\n");
    printf("  disable the system popup dialog if the process crashes\n");
    /*
    printf("-dbg\n");
    printf("  catch unhandled exceptions\n");
    */
    printf("-exitcodes\n");
    printf("  print the process exit codes when each process exits.\n");
    printf("-noprompt\n");
    printf("  prevent mpiexec from prompting for user credentials.\n");
    printf("-priority class[:level]\n");
    printf("  set the process startup priority class and optionally level.\n");
    printf("  class = 0,1,2,3,4   = idle, below, normal, above, high\n");
    printf("  level = 0,1,2,3,4,5 = idle, lowest, below, normal, above, highest\n");
    printf("  the default is -priority 1:3\n");
    printf("-localroot\n");
    printf("  launch the root process directly from mpiexec if the host is local.\n");
    printf("  (This allows the root process to create windows and be debugged.)\n");
    printf("-port port\n");
    printf("-p port\n");
    printf("  specify the port that smpd is listening on.\n");
    printf("-phrase passphrase\n");
    printf("  specify the passphrase to authenticate connections to smpd with.\n");
    printf("-smpdfile filename\n");
    printf("  specify the file where the smpd options are stored including the passphrase.\n");
    /*
    printf("-soft Fortran90_triple\n");
    printf("  acceptable number of processes to launch up to maxprocs\n");
    */
    printf("-path search_path\n");
    printf("  search path for executable, ; separated\n");
    /*
    printf("-arch architecture\n");
    printf("  sun, linux, rs6000, ...\n");
    */
    printf("-register [-user n]\n");
    printf("  encrypt a user name and password to the Windows registry.\n");
    printf("  optionally specify a user slot index\n");
    printf("-remove [-user n]\n");
    printf("  delete the encrypted credentials from the Windows registry.\n");
    printf("  If no user index is specified then all entries are removed.\n");
    printf("-validate [-user n] [-host hostname]\n");
    printf("  validate the encrypted credentials for the current or specified host.\n");
    printf("  A specific user index can be specified otherwise index 0 is the default.\n");
    printf("-user n\n");
    printf("  use the registered user credentials from slot n to launch the job.\n");
    printf("-timeout seconds\n");
    printf("  timeout for the job.\n");
    printf("-plaintext\n");
    printf("  don't encrypt the data on the wire.\n");
    printf("-delegate\n");
    printf("  use passwordless delegation to launch processes\n");
    printf("-impersonate\n");
    printf("  use passwordless authentication to launch processes\n");
    printf("-add_job <job_name> <domain\\user>\n");
    printf("-add_job <job_name> <domain\\user> -host <hostname>\n");
    printf("  add a job key for the specified domain user on the local or specified host\n");
    printf("  requires administrator privileges\n");
    printf("-remove_job <name>\n");
    printf("-remove_job <name> -host <hostname>\n");
    printf("  remove a job key from the local or specified host\n");
    printf("  requires administrator privileges\n");
    printf("-associate_job <name>\n");
    printf("-associate_job <name> -host <hostname>\n");
    printf("  associate the current user's token with the specified job on the local or specified host\n");
    printf("-job <name>\n");
    printf("  launch the processes in the context of the specified job\n");
    printf("-whomai\n");
    printf("  print the current user name\n");
    printf("-l\n");
    printf("  prefix output with the process number. (This option is a lowercase L not the number one)\n");
}

#ifdef HAVE_WINDOWS_H

/* check to see if a path is on a network mapped drive and would need to be mapped on a remote system */
SMPD_BOOL NeedToMap(char *pszFullPath, char *pDrive, char *pszShare)
{
    DWORD dwResult;
    DWORD dwLength;
    char pBuffer[4096];
    REMOTE_NAME_INFO *info = (REMOTE_NAME_INFO*)pBuffer;
    char pszTemp[SMPD_MAX_EXE_LENGTH];

    if (*pszFullPath == '"')
    {
	strncpy(pszTemp, &pszFullPath[1], SMPD_MAX_EXE_LENGTH);
	pszTemp[SMPD_MAX_EXE_LENGTH-1] = '\0';
	if (pszTemp[strlen(pszTemp)-1] == '"')
	    pszTemp[strlen(pszTemp)-1] = '\0';
	pszFullPath = pszTemp;
    }
    dwLength = 4096;
    info->lpConnectionName = NULL;
    info->lpRemainingPath = NULL;
    info->lpUniversalName = NULL;
    dwResult = WNetGetUniversalName(pszFullPath, REMOTE_NAME_INFO_LEVEL, info, &dwLength);
    if (dwResult == NO_ERROR)
    {
	*pDrive = *pszFullPath;
	strcpy(pszShare, info->lpConnectionName);
	return SMPD_TRUE;
    }

    /*printf("WNetGetUniversalName: '%s'\n error %d\n", pszExe, dwResult);*/
    return SMPD_FALSE;
}

/* convert an executable name to a universal naming convention version so that it can be used on a remote system */
void ExeToUnc(char *pszExe, int length)
{
    DWORD dwResult;
    DWORD dwLength;
    char pBuffer[4096];
    REMOTE_NAME_INFO *info = (REMOTE_NAME_INFO*)pBuffer;
    char pszTemp[SMPD_MAX_EXE_LENGTH];
    SMPD_BOOL bQuoted = SMPD_FALSE;
    char *pszOriginal;

    pszOriginal = pszExe;

    if (*pszExe == '"')
    {
	bQuoted = SMPD_TRUE;
	strncpy(pszTemp, &pszExe[1], SMPD_MAX_EXE_LENGTH);
	pszTemp[SMPD_MAX_EXE_LENGTH-1] = '\0';
	if (pszTemp[strlen(pszTemp)-1] == '"')
	    pszTemp[strlen(pszTemp)-1] = '\0';
	pszExe = pszTemp;
    }
    dwLength = 4096;
    info->lpConnectionName = NULL;
    info->lpRemainingPath = NULL;
    info->lpUniversalName = NULL;
    dwResult = WNetGetUniversalName(pszExe, REMOTE_NAME_INFO_LEVEL, info, &dwLength);
    if (dwResult == NO_ERROR)
    {
	if (bQuoted)
	    snprintf(pszOriginal, length, "\"%s\"", info->lpUniversalName);
	else
	{
	    strncpy(pszOriginal, info->lpUniversalName, length);
	    pszOriginal[length-1] = '\0';
	}
    }
}

#endif

static int strip_args(int *argcp, char **argvp[], int n)
{
    int i;

    if (n+1 > (*argcp))
    {
	printf("Error: cannot strip %d args, only %d left.\n", n, (*argcp)-1);
	return SMPD_FAIL;
    }
    for (i=n+1; i<=(*argcp); i++)
    {
	(*argvp)[i-n] = (*argvp)[i];
    }
    (*argcp) -= n;
    return SMPD_SUCCESS;
}

static SMPD_BOOL smpd_isnumber(char *str)
{
    size_t i, n = strlen(str);
    for (i=0; i<n; i++)
    {
	if (!isdigit(str[i]))
	    return SMPD_FALSE;
    }
    return SMPD_TRUE;
}

#ifdef HAVE_WINDOWS_H
static int mpiexec_assert_hook( int reportType, char *message, int *returnValue )
{
    SMPD_UNREFERENCED_ARG(reportType);

    fprintf(stderr, "%s", message);
    if (returnValue != NULL)
	exit(*returnValue);
    exit(-1);
}

/* This function reads the user binding option and sets the affinity map.
 * The user option is of the form "user:a,b,c" where a, b, c are integers denoting
 * the cores in the affinity map
 */
static int read_user_affinity_map(char *option)
{
    char *p = NULL, *q = NULL, *context = NULL;
    int i=0, map_sz = 0;

    if(smpd_process.affinity_map != NULL){
        printf("Error: duplicate user affinity map option\n");
        return SMPD_FAIL;
    }
    if(option == NULL){
        printf("Error: NULL user affinity option\n");
        return SMPD_FAIL;
    }

    /* option = "user:1,2,3" */
    p = option;
    p = strtok_s(p, ":", &context);
    if(p == NULL){
        printf("Error parsing user affinity map\n");
        return SMPD_FAIL;
    }
    p = strtok_s(NULL, ":", &context);
    if(p == NULL){
        printf("Error parsing user affinity map\n");
        return SMPD_FAIL;
    }

    /* p should now point to the affinity map separated by comma's */
    map_sz = 1;
    q = p;
    /* parse to find the number of elements in the map */
    while(*q != '\0'){
        if(*q == ','){
            map_sz++;
        }
        q++;
    }

    /* Allocate the mem for map */
    smpd_process.affinity_map = (int *)MPIU_Malloc(sizeof(int) * map_sz);
    if(smpd_process.affinity_map == NULL){
        printf("Unable to allocate memory for affinity map\n");
        return SMPD_FAIL;
    }
    smpd_process.affinity_map_sz = map_sz;

    context = NULL;
    p = strtok_s(p, ",", &context);
    i = 0;
    while(p != NULL){
        /* FIXME: We don't detect overflow case in atoi */
        smpd_process.affinity_map[i++] = atoi(p);
        p = strtok_s(NULL, ",", &context);
    }

    smpd_dbg_printf("The user affinity map is : [ ");
    for(i=0; i<map_sz; i++){
        smpd_dbg_printf(" %d ,",smpd_process.affinity_map[i]);
    }
    smpd_dbg_printf(" ] \n");

    return SMPD_SUCCESS;
}
#endif

#undef FCNAME
#define FCNAME "mp_parse_command_args"
int mp_parse_command_args(int *argcp, char **argvp[])
{
    int cur_rank;
    int affinity_map_index;
    int argc, next_argc;
    char **next_argv;
    char *exe_ptr;
    int num_args_to_strip;
    int nproc;
    char machine_file_name[SMPD_MAX_FILENAME];
    int use_machine_file = SMPD_FALSE;
    smpd_map_drive_node_t *map_node, *drive_map_list;
    smpd_map_drive_node_t *gmap_node, *gdrive_map_list;
    smpd_env_node_t *env_node, *env_list;
    smpd_env_node_t *genv_list;
    char *env_str, env_data[SMPD_MAX_ENV_LENGTH];
    char wdir[SMPD_MAX_DIR_LENGTH];
    char gwdir[SMPD_MAX_DIR_LENGTH];
    int use_debug_flag;
    char pwd_file_name[SMPD_MAX_FILENAME];
    int use_pwd_file;
    smpd_host_node_t *host_node_ptr, *host_list, *host_node_iter;
    smpd_host_node_t *ghost_list;
    int no_drive_mapping;
    int n_priority_class, n_priority;
    int index, i;
    char configfilename[SMPD_MAX_FILENAME];
    int use_configfile, delete_configfile;
    char exe[SMPD_MAX_EXE_LENGTH], *exe_iter;
    char exe_path[SMPD_MAX_EXE_LENGTH], *namepart;
    smpd_launch_node_t *launch_node, *launch_node_iter;
    int exe_len_remaining;
    char path[SMPD_MAX_PATH_LENGTH];
    char gpath[SMPD_MAX_PATH_LENGTH];
    char temp_password[SMPD_MAX_PASSWORD_LENGTH];
    FILE *fin_config = NULL;
    int result;
    int maxlen;
    int appnum = 0;
    char channel[SMPD_MAX_NAME_LENGTH] = "";
    /* smpd configured settings */
    char smpd_setting_tmp_buffer[20];
    char smpd_setting_channel[20] = "";
    char smpd_setting_internode_channel[20] = "";
    SMPD_BOOL smpd_setting_timeout = SMPD_INVALID_SETTING;
    SMPD_BOOL smpd_setting_priority_class = SMPD_INVALID_SETTING;
    SMPD_BOOL smpd_setting_priority = SMPD_INVALID_SETTING;
    char smpd_setting_path[SMPD_MAX_PATH_LENGTH] = "";
    SMPD_BOOL smpd_setting_localonly = SMPD_INVALID_SETTING;
    char nemesis_netmod[SMPD_MAX_NEMESIS_NETMOD_LENGTH];

#ifdef HAVE_WINDOWS_H
    int user_index;
    char user_index_str[20];
#endif

    smpd_enter_fn(FCNAME);

#ifdef HAVE_WINDOWS_H
    /* prevent mpiexec from bringing up an error message window if it crashes */
    _CrtSetReportMode( _CRT_ASSERT, _CRTDBG_MODE_FILE );
    _CrtSetReportFile( _CRT_ASSERT, _CRTDBG_FILE_STDERR );
    _CrtSetReportHook(mpiexec_assert_hook);

    /* check for windows specific arguments */
    if (*argcp > 1)
    {
	if (strcmp((*argvp)[1], "-register") == 0)
	{
        char register_filename[SMPD_MAX_FILENAME];
	    user_index = 0;
	    smpd_get_opt_int(argcp, argvp, "-user", &user_index);
	    if (user_index < 0)
	    {
		user_index = 0;
	    }
        register_filename[0] = '\0';
        if(smpd_get_opt_string(argcp, argvp, "-file", register_filename, SMPD_MAX_FILENAME))
        {
            smpd_dbg_printf("Registering username/password to a file\n");
        }
	    for (;;)
	    {
		smpd_get_account_and_password(smpd_process.UserAccount, smpd_process.UserPassword);
		fprintf(stderr, "confirm password: ");fflush(stderr);
		smpd_get_password(temp_password);
		if (strcmp(smpd_process.UserPassword, temp_password) == 0)
		    break;
		printf("passwords don't match, please try again.\n");
	    }
        if(strlen(register_filename) > 0)
        {
            if(smpd_save_cred_to_file(register_filename, smpd_process.UserAccount, smpd_process.UserPassword))
            {
                printf("Username/password encrypted and saved to registry file\n");
            }
            else
            {
                smpd_err_printf("Error saving username/password to registry file\n");
            }
            smpd_exit(0);
        }
	    if (smpd_save_password_to_registry(user_index, smpd_process.UserAccount, smpd_process.UserPassword, SMPD_TRUE)) 
	    {
		printf("Password encrypted into the Registry.\n");
		smpd_delete_cached_password();
	    }
	    else
	    {
		printf("Error: Unable to save encrypted password.\n");
	    }
	    fflush(stdout);
	    smpd_exit(0);
	}
	if ( (strcmp((*argvp)[1], "-remove") == 0) || (strcmp((*argvp)[1], "-unregister") == 0) )
	{
	    user_index = 0;
	    if (smpd_get_opt_string(argcp, argvp, "-user", user_index_str, 20))
	    {
		if (user_index_str[0] == 'a' && user_index_str[1] == 'l' && user_index_str[2] == 'l' && user_index_str[3] == '\0')
		{
		    user_index = -1;
		}
		else
		{
		    user_index = atoi(user_index_str);
		    if (user_index < 0)
		    {
			user_index = 0;
		    }
		}
	    }
	    if (smpd_delete_current_password_registry_entry(user_index))
	    {
		smpd_delete_cached_password();
		printf("Account and password removed from the Registry.\n");
	    }
	    else
	    {
		printf("ERROR: Unable to remove the encrypted password.\n");
	    }
	    fflush(stdout);
	    smpd_exit(0);
	}
	if (strcmp((*argvp)[1], "-validate") == 0)
	{
	    user_index = 0;
	    smpd_get_opt_int(argcp, argvp, "-user", &user_index);
	    if (user_index < 0)
	    {
		user_index = 0;
	    }
	    if (smpd_read_password_from_registry(user_index, smpd_process.UserAccount, smpd_process.UserPassword))
	    {
		if (!smpd_get_opt_string(argcp, argvp, "-host", smpd_process.console_host, SMPD_MAX_HOST_LENGTH))
		{
		    smpd_get_hostname(smpd_process.console_host, SMPD_MAX_HOST_LENGTH);
		}
		smpd_get_opt_int(argcp, argvp, "-port", &smpd_process.port);
		smpd_get_opt_string(argcp, argvp, "-phrase", smpd_process.passphrase, SMPD_PASSPHRASE_MAX_LENGTH);
		smpd_process.builtin_cmd = SMPD_CMD_VALIDATE;
		smpd_do_console();
	    }
	    else
	    {
		printf("FAIL: Unable to read the credentials from the registry.\n");fflush(stdout);
	    }
	    fflush(stdout);
	    smpd_exit(0);
	}
	if (strcmp((*argvp)[1], "-whoami") == 0)
	{
	    char username[100] = "";
	    ULONG len = 100;
	    if (GetUserNameEx(NameSamCompatible, username, &len))
	    {
		printf("%s\n", username);
	    }
	    else if (GetUserName(username, &len))
	    {
		printf("%s\n", username);
	    }
	    else
	    {
		printf("ERROR: Unable to determine the current username.\n");
	    }
	    fflush(stdout);
	    smpd_exit(0);
	}

	if (strcmp((*argvp)[1], "-add_job") == 0)
	{
	    if (smpd_get_opt(argcp, argvp, "-verbose"))
	    {
		smpd_process.verbose = SMPD_TRUE;
		smpd_process.dbg_state |= SMPD_DBG_STATE_ERROUT | SMPD_DBG_STATE_STDOUT | SMPD_DBG_STATE_TRACE;
	    }

	    if (smpd_get_opt_two_strings(argcp, argvp, "-add_job", smpd_process.job_key, SMPD_MAX_NAME_LENGTH, smpd_process.job_key_account, SMPD_MAX_ACCOUNT_LENGTH))
	    {
		if (!smpd_get_opt_string(argcp, argvp, "-host", smpd_process.console_host, SMPD_MAX_HOST_LENGTH))
		{
		    smpd_get_hostname(smpd_process.console_host, SMPD_MAX_HOST_LENGTH);
		}
		smpd_get_opt_int(argcp, argvp, "-port", &smpd_process.port);
		smpd_get_opt_string(argcp, argvp, "-phrase", smpd_process.passphrase, SMPD_PASSPHRASE_MAX_LENGTH);
		if (smpd_get_opt_string(argcp, argvp, "-password", smpd_process.job_key_password, SMPD_MAX_PASSWORD_LENGTH))
		{
		    smpd_process.builtin_cmd = SMPD_CMD_ADD_JOB_AND_PASSWORD;
		}
		else
		{
		    smpd_process.builtin_cmd = SMPD_CMD_ADD_JOB;
		}
		smpd_do_console();
		fflush(stdout);
		smpd_exit(0);
	    }
	    printf("Invalid number of arguments passed to -add_job <job_key> <user_account>\n");
	    fflush(stdout);
	    smpd_exit(-1);
	}

	if (strcmp((*argvp)[1], "-remove_job") == 0)
	{
	    if (smpd_get_opt(argcp, argvp, "-verbose"))
	    {
		smpd_process.verbose = SMPD_TRUE;
		smpd_process.dbg_state |= SMPD_DBG_STATE_ERROUT | SMPD_DBG_STATE_STDOUT | SMPD_DBG_STATE_TRACE;
	    }

	    if (smpd_get_opt_string(argcp, argvp, "-remove_job", smpd_process.job_key, SMPD_MAX_NAME_LENGTH))
	    {
		if (!smpd_get_opt_string(argcp, argvp, "-host", smpd_process.console_host, SMPD_MAX_HOST_LENGTH))
		{
		    smpd_get_hostname(smpd_process.console_host, SMPD_MAX_HOST_LENGTH);
		}
		smpd_get_opt_int(argcp, argvp, "-port", &smpd_process.port);
		smpd_get_opt_string(argcp, argvp, "-phrase", smpd_process.passphrase, SMPD_PASSPHRASE_MAX_LENGTH);
		smpd_process.builtin_cmd = SMPD_CMD_REMOVE_JOB;
		smpd_do_console();
		fflush(stdout);
		smpd_exit(0);
	    }
	    printf("Invalid number of arguments passed to -remove_job <job_key>\n");
	    fflush(stdout);
	    smpd_exit(-1);
	}

	if (strcmp((*argvp)[1], "-associate_job") == 0)
	{
	    if (smpd_get_opt(argcp, argvp, "-verbose"))
	    {
		smpd_process.verbose = SMPD_TRUE;
		smpd_process.dbg_state |= SMPD_DBG_STATE_ERROUT | SMPD_DBG_STATE_STDOUT | SMPD_DBG_STATE_TRACE;
	    }

	    if (smpd_get_opt_string(argcp, argvp, "-associate_job", smpd_process.job_key, SMPD_MAX_NAME_LENGTH))
	    {
		if (!smpd_get_opt_string(argcp, argvp, "-host", smpd_process.console_host, SMPD_MAX_HOST_LENGTH))
		{
		    smpd_get_hostname(smpd_process.console_host, SMPD_MAX_HOST_LENGTH);
		}
		smpd_get_opt_int(argcp, argvp, "-port", &smpd_process.port);
		smpd_get_opt_string(argcp, argvp, "-phrase", smpd_process.passphrase, SMPD_PASSPHRASE_MAX_LENGTH);
		smpd_process.builtin_cmd = SMPD_CMD_ASSOCIATE_JOB;
		smpd_do_console();
		fflush(stdout);
		smpd_exit(0);
	    }
	    printf("Invalid number of arguments passed to -associate_job <job_key>\n");
	    fflush(stdout);
	    smpd_exit(-1);
	}
    }
#endif

    if ((*argcp == 2) &&
	((strcmp((*argvp)[1], "-pmiserver") == 0) || (strcmp((*argvp)[1], "-pmi_server") == 0)))
    {
	smpd_err_printf("Error: No number of processes specified after the %s option\n", (*argvp)[1]);
	return SMPD_FAIL;
    }

    if (*argcp >= 3)
    {
	if ((strcmp((*argvp)[1], "-pmiserver") == 0) || (strcmp((*argvp)[1], "-pmi_server") == 0))
	{
	    char host[100];
	    int id;

        smpd_process.use_pmi_server = SMPD_TRUE;

	    if (smpd_get_opt(argcp, argvp, "-verbose"))
	    {
		smpd_process.verbose = SMPD_TRUE;
		smpd_process.dbg_state |= SMPD_DBG_STATE_ERROUT | SMPD_DBG_STATE_STDOUT | SMPD_DBG_STATE_TRACE;
	    }

        if(smpd_get_opt(argcp, argvp, "-hide_console")){
#ifdef HAVE_WINDOWS_H		
            FreeConsole();
#endif
        }

#ifdef HAVE_WINDOWS_H
        if(smpd_get_opt(argcp, argvp, "-impersonate"))
        {
            smpd_process.use_sspi = SMPD_TRUE;
            smpd_process.use_delegation = SMPD_FALSE;
        }
        if(smpd_get_opt(argcp, argvp, "-delegate"))
        {
            smpd_process.use_sspi = SMPD_TRUE;
            smpd_process.use_delegation = SMPD_TRUE;
        }
#endif
	    smpd_process.nproc = atoi((*argvp)[2]);
	    if (smpd_process.nproc < 1)
	    {
		smpd_err_printf("invalid number of processes: %s\n", (*argvp)[2]);
		return SMPD_FAIL;
	    }

	    /* set up the host list to connect to only the local host */
	    smpd_get_hostname(host, 100);
	    result = smpd_get_host_id(host, &id);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to get a id for host %s\n", host);
		return SMPD_FAIL;
	    }

        if (*argcp >= 5){
    	    smpd_process.singleton_client_port = atoi((*argvp)[4]);
            if(smpd_process.singleton_client_port < 1){
                smpd_err_printf("Invalid singleton client port = %d\n",
                                    smpd_process.singleton_client_port);
                return SMPD_FAIL;
            }
        }

	    /* Return without creating any launch_nodes.  This will result in an mpiexec connected to the local smpd
	     * and no processes launched.
	     */
	    return SMPD_SUCCESS;
	}
    }

    /* Get settings saved in smpd */
    /* These settings have the lowest priority.
     * First are settings on the command line,
     * second are settings from environment variables and
     * these are last.
     */
    result = smpd_get_smpd_data("channel", smpd_setting_channel, 20);
    result = smpd_get_smpd_data("internode_channel", smpd_setting_internode_channel, 20);
    smpd_setting_tmp_buffer[0] = '\0';
    result = smpd_get_smpd_data("timeout", smpd_setting_tmp_buffer, 20);
    if (result == SMPD_SUCCESS)
    {
	smpd_setting_timeout = atoi(smpd_setting_tmp_buffer);
	if (smpd_setting_timeout < 1)
	{
	    smpd_setting_timeout = SMPD_INVALID_SETTING;
	}
    }
    smpd_process.output_exit_codes = smpd_option_on("exitcodes");
    if (smpd_option_on("noprompt") == SMPD_TRUE)
    {
	smpd_process.noprompt = SMPD_TRUE;
	smpd_process.credentials_prompt = SMPD_FALSE;
    }
    env_str = getenv("MPIEXEC_NOPROMPT");
    if (env_str)
    {
	smpd_process.noprompt = SMPD_TRUE;
	smpd_process.credentials_prompt = SMPD_FALSE;
    }
    smpd_setting_tmp_buffer[0] = '\0';
    result = smpd_get_smpd_data("priority", smpd_setting_tmp_buffer, 20);
    if (result == SMPD_SUCCESS)
    {
	if (smpd_isnumbers_with_colon(smpd_setting_tmp_buffer))
	{
	    char *str;
	    smpd_setting_priority_class = atoi(smpd_setting_tmp_buffer); /* This assumes atoi will stop at the colon and return a number */
	    str = strchr(smpd_setting_tmp_buffer, ':');
	    if (str)
	    {
		str++;
		smpd_setting_priority = atoi(str);
	    }
	    else
	    {
		smpd_setting_priority = SMPD_DEFAULT_PRIORITY;
	    }
	    if (smpd_setting_priority_class < 0 || smpd_setting_priority_class > 4 || smpd_setting_priority < 0 || smpd_setting_priority > 5)
	    {
		/* ignore invalid priority settings */
		smpd_setting_priority_class = SMPD_INVALID_SETTING;
		smpd_setting_priority = SMPD_INVALID_SETTING;
	    }
	}
    }
    result = smpd_get_smpd_data("app_path", smpd_setting_path, SMPD_MAX_PATH_LENGTH);
    smpd_process.plaintext = smpd_option_on("plaintext");
    smpd_setting_localonly = smpd_option_on("localonly");
    result = smpd_get_smpd_data("port", smpd_setting_tmp_buffer, 20);
    if (result == SMPD_SUCCESS)
    {
	if (smpd_isnumber(smpd_setting_tmp_buffer))
	{
	    result = atoi(smpd_setting_tmp_buffer);
	    if (result != 0)
		smpd_process.port = result;
	}
    }

    env_str = getenv("MPIEXEC_SMPD_PORT");
    if (env_str)
    {
	if (smpd_isnumber(env_str))
	{
	    result = atoi(env_str);
	    if (result != 0)
		smpd_process.port = result;
	}
    }

    /* check for mpi options */
    /*
     * Required:
     * -n <maxprocs>
     * -host <hostname>
     * -soft <Fortran90 triple> - represents allowed number of processes up to maxprocs
     *        a or a:b or a:b:c where
     *        1) a = a
     *        2) a:b = a, a+1, a+2, ..., b
     *        3) a:b:c = a, a+c, a+2c, a+3c, ..., a+kc
     *           where a+kc <= b if c>0
     *                 a+kc >= b if c<0
     * -wdir <working directory>
     * -path <search path for executable>
     * -arch <architecture> - sun, linux, rs6000, ...
     * -configfile <filename> - each line contains a complete set of mpiexec options, #commented
     *
     * Extensions:
     * -env <variable=value>
     * -env <variable=value;variable2=value2;...>
     * -hosts <n host1 host2 ... hostn>
     * -hosts <n host1 m1 host2 m2 ... hostn mn>
     * -machinefile <filename> - one host per line, #commented
     * -localonly <numprocs>
     * -nompi - don't require processes to be SMPD processes (don't have to call SMPD_Init or PMI_Init)
     * -exitcodes - print the exit codes of processes as they exit
     * -verbose - same as setting environment variable to SMPD_DBG_OUTPUT=stdout
     * -quiet_abort - minimize the output when a job is aborted
     * -file - mpich1 job configuration file
     * 
     * Windows extensions:
     * -map <drive:\\host\share>
     * -pwdfile <filename> - account on the first line and password on the second
     * -nomapping - don't copy the current directory mapping on the remote nodes
     * -dbg - debug
     * -noprompt - don't prompt for user credentials, fail with an error message
     * -logon - force the prompt for user credentials
     * -priority <class[:level]> - set the process startup priority class and optionally level.
     *            class = 0,1,2,3,4   = idle, below, normal, above, high
     *            level = 0,1,2,3,4,5 = idle, lowest, below, normal, above, highest
     * -localroot - launch the root process without smpd if the host is local.
     *              (This allows the root process to create windows and be debugged.)
     *
     * Backwards compatibility
     * -np <numprocs>
     * -dir <working directory>
     */

    /* Get a list of hosts from a file or the registry to be used with the -n,-np options */
    smpd_get_default_hosts(); 

    cur_rank = 0;
    affinity_map_index = 0;
    gdrive_map_list = NULL;
    genv_list = NULL;
    gwdir[0] = '\0';
    gpath[0] = '\0';
    ghost_list = NULL;
    next_argc = *argcp;
    next_argv = *argvp + 1;
    exe_ptr = **argvp;
    do
    {
	/* calculate the current argc and find the next argv */
	argc = 1;
	while ( (*next_argv) != NULL && (**next_argv) != ':')
	{
	    argc++;
	    next_argc--;
	    next_argv++;
	}
	if ( (*next_argv) != NULL && (**next_argv) == ':')
	{
	    (*next_argv) = NULL;
	    next_argv++;
	}
	argcp = &argc;

	/* reset block global variables */
	use_configfile = SMPD_FALSE;
	delete_configfile = SMPD_FALSE;
configfile_loop:
	nproc = 0;
	drive_map_list = NULL;
	env_list = NULL;
	wdir[0] = '\0';
	use_debug_flag = SMPD_FALSE;
	use_pwd_file = SMPD_FALSE;
	host_list = NULL;
	no_drive_mapping = SMPD_FALSE;
	n_priority_class = (smpd_setting_priority_class == SMPD_INVALID_SETTING) ? SMPD_DEFAULT_PRIORITY_CLASS : smpd_setting_priority_class;
	n_priority = (smpd_setting_priority == SMPD_INVALID_SETTING) ? SMPD_DEFAULT_PRIORITY : smpd_setting_priority;
	use_machine_file = SMPD_FALSE;
	if (smpd_setting_path[0] != '\0')
	{
	    strncpy(path, smpd_setting_path, SMPD_MAX_PATH_LENGTH);
	}
	else
	{
	    path[0] = '\0';
	}

	/* Check for the -configfile option.  It must be the first and only option in a group. */
	if ((*argvp)[1] && (*argvp)[1][0] == '-')
	{
	    if ((*argvp)[1][1] == '-')
	    {
		/* double -- option provided, trim it to a single - */
		index = 2;
		while ((*argvp)[1][index] != '\0')
		{
		    (*argvp)[1][index-1] = (*argvp)[1][index];
		    index++;
		}
		(*argvp)[1][index-1] = '\0';
	    }
	    if (strcmp(&(*argvp)[1][1], "configfile") == 0)
	    {
		if (use_configfile)
		{
		    printf("Error: -configfile option is not valid from within a configuration file.\n");
		    smpd_exit_fn(FCNAME);
		    return SMPD_FAIL;
		}
		if (argc < 3)
		{
		    printf("Error: no filename specifed after -configfile option.\n");
		    smpd_exit_fn(FCNAME);
		    return SMPD_FAIL;
		}
		strncpy(configfilename, (*argvp)[2], SMPD_MAX_FILENAME);
		use_configfile = SMPD_TRUE;
		fin_config = fopen(configfilename, "r");
		if (fin_config == NULL)
		{
		    printf("Error: unable to open config file '%s'\n", configfilename);
		    smpd_exit_fn(FCNAME);
		    return SMPD_FAIL;
		}
		if (!smpd_get_argcv_from_file(fin_config, argcp, argvp))
		{
		    fclose(fin_config);
		    printf("Error: unable to parse config file '%s'\n", configfilename);
		    smpd_exit_fn(FCNAME);
		    return SMPD_FAIL;
		}
	    }
	    if (strcmp(&(*argvp)[1][1], "file") == 0)
	    {
		if (argc < 3)
		{
		    printf("Error: no filename specifed after -file option.\n");
		    smpd_exit_fn(FCNAME);
		    return SMPD_FAIL;
		}
		mp_parse_mpich1_configfile((*argvp)[2], configfilename, SMPD_MAX_FILENAME);
		delete_configfile = SMPD_TRUE;
		use_configfile = SMPD_TRUE;
		fin_config = fopen(configfilename, "r");
		if (fin_config == NULL)
		{
		    printf("Error: unable to open config file '%s'\n", configfilename);
		    smpd_exit_fn(FCNAME);
		    return SMPD_FAIL;
		}
		if (!smpd_get_argcv_from_file(fin_config, argcp, argvp))
		{
		    fclose(fin_config);
		    printf("Error: unable to parse config file '%s'\n", configfilename);
		    smpd_exit_fn(FCNAME);
		    return SMPD_FAIL;
		}
	    }
	}

	/* parse the current block */

	/* parse the mpiexec options */
	while ((*argvp)[1] && (*argvp)[1][0] == '-')
	{
	    if ((*argvp)[1][1] == '-')
	    {
		/* double -- option provided, trim it to a single - */
		index = 2;
		while ((*argvp)[1][index] != '\0')
		{
		    (*argvp)[1][index-1] = (*argvp)[1][index];
		    index++;
		}
		(*argvp)[1][index-1] = '\0';
	    }

	    num_args_to_strip = 1;
	    if ((strcmp(&(*argvp)[1][1], "np") == 0) || (strcmp(&(*argvp)[1][1], "n") == 0))
	    {
		if (nproc != 0)
		{
		    printf("Error: only one option is allowed to determine the number of processes.\n");
		    printf("       -hosts, -n, -np and -localonly x are mutually exclusive\n");
		    smpd_exit_fn(FCNAME);
		    return SMPD_FAIL;
		}
		if (argc < 3)
		{
		    printf("Error: no number specified after %s option.\n", (*argvp)[1]);
		    smpd_exit_fn(FCNAME);
		    return SMPD_FAIL;
		}
		nproc = atoi((*argvp)[2]);
		if (nproc < 1)
		{
		    printf("Error: must specify a number greater than 0 after the %s option\n", (*argvp)[1]);
		    smpd_exit_fn(FCNAME);
		    return SMPD_FAIL;
		}
		num_args_to_strip = 2;
	    }
	    else if (strcmp(&(*argvp)[1][1], "localonly") == 0)
	    {
		/* check to see if there is a number after the localonly option */
		if (argc > 2)
		{
		    if (smpd_isnumber((*argvp)[2]))
		    {
			if (nproc != 0)
			{
			    printf("Error: only one option is allowed to determine the number of processes.\n");
			    printf("       -hosts, -n, -np and -localonly x are mutually exclusive\n");
			    smpd_exit_fn(FCNAME);
			    return SMPD_FAIL;
			}
			nproc = atoi((*argvp)[2]);
			if (nproc < 1)
			{
			    printf("Error: If you specify a number after -localonly option,\n        it must be greater than 0.\n");
			    smpd_exit_fn(FCNAME);
			    return SMPD_FAIL;
			}
			num_args_to_strip = 2;
		    }
		}
		/* Use localroot to implement localonly */
		smpd_process.local_root = SMPD_TRUE;
		/* create a host list of one and set nproc to -1 to be replaced by nproc after parsing the block */
		host_list = (smpd_host_node_t*)MPIU_Malloc(sizeof(smpd_host_node_t));
		if (host_list == NULL)
		{
		    printf("failed to allocate memory for a host node.\n");
		    smpd_exit_fn(FCNAME);
		    return SMPD_FAIL;
		}
		host_list->next = NULL;
		host_list->left = NULL;
		host_list->right = NULL;
		host_list->connected = SMPD_FALSE;
		host_list->connect_cmd_tag = -1;
		host_list->connect_cmd_tag = -1;
		host_list->nproc = -1;
		host_list->alt_host[0] = '\0';
		smpd_get_hostname(host_list->host, SMPD_MAX_HOST_LENGTH);
	    }
	    else if (strcmp(&(*argvp)[1][1], "machinefile") == 0)
	    {
		if (smpd_process.s_host_list != NULL)
		{
		    printf("Error: -machinefile can only be specified once per section.\n");
		    smpd_exit_fn(FCNAME);
		    return SMPD_FAIL;
		}
		if (argc < 3)
		{
		    printf("Error: no filename specified after -machinefile option.\n");
		    smpd_exit_fn(FCNAME);
		    return SMPD_FAIL;
		}
		strncpy(machine_file_name, (*argvp)[2], SMPD_MAX_FILENAME);
		use_machine_file = SMPD_TRUE;
		smpd_parse_machine_file(machine_file_name);
		num_args_to_strip = 2;
	    }
#ifdef HAVE_WINDOWS_H
        else if (strcmp(&(*argvp)[1][1], "binding") == 0)
        {
            if(strcmp(&(*argvp)[2][0], "auto") == 0)
            {
                smpd_process.set_affinity = TRUE;
                smpd_process.affinity_map = NULL;
                smpd_process.affinity_map_sz = 0;
            }
            else if(strncmp(&(*argvp)[2][0], "user", 4) == 0)
            {
                smpd_process.set_affinity = TRUE;
                if(read_user_affinity_map(&(*argvp)[2][0]) != SMPD_SUCCESS)
                {
                    printf("Error parsing user binding scheme\n");
                    smpd_exit_fn(FCNAME);
                }
            }
            else
            {
                printf("Error: Process binding schemes supported are \"auto\", \"user\" \n");
                smpd_exit_fn(FCNAME);
                return SMPD_FAIL;
            }
            num_args_to_strip = 2;
        }
#endif
	    else if (strcmp(&(*argvp)[1][1], "map") == 0)
	    {
		if (argc < 3)
		{
		    printf("Error: no drive specified after -map option.\n");
		    smpd_exit_fn(FCNAME);
		    return SMPD_FAIL;
		}
		if (smpd_parse_map_string((*argvp)[2], &drive_map_list) != SMPD_SUCCESS)
		{
		    printf("Error: unable to parse the drive mapping option - '%s'\n", (*argvp)[2]);
		    smpd_exit_fn(FCNAME);
		    return SMPD_FAIL;
		}
		num_args_to_strip = 2;
	    }
        else if (strcmp(&(*argvp)[1][1], "mapall") == 0){
#ifdef HAVE_WINDOWS_H
            if(smpd_mapall(&drive_map_list) != SMPD_SUCCESS){
		    printf("Error: unable to map all network drives");
		    smpd_exit_fn(FCNAME);
		    return SMPD_FAIL;
            }
#endif
        num_args_to_strip = 1;
        }
	    else if (strcmp(&(*argvp)[1][1], "gmap") == 0)
	    {
		if (argc < 3)
		{
		    printf("Error: no drive specified after -gmap option.\n");
		    smpd_exit_fn(FCNAME);
		    return SMPD_FAIL;
		}
		if (smpd_parse_map_string((*argvp)[2], &gdrive_map_list) != SMPD_SUCCESS)
		{
		    printf("Error: unable to parse the drive mapping option - '%s'\n", (*argvp)[2]);
		    smpd_exit_fn(FCNAME);
		    return SMPD_FAIL;
		}
		num_args_to_strip = 2;
	    }
	    else if ( (strcmp(&(*argvp)[1][1], "dir") == 0) || (strcmp(&(*argvp)[1][1], "wdir") == 0) )
	    {
		if (argc < 3)
		{
		    printf("Error: no directory after %s option\n", (*argvp)[1]);
		    smpd_exit_fn(FCNAME);
		    return SMPD_FAIL;
		}
		strncpy(wdir, (*argvp)[2], SMPD_MAX_DIR_LENGTH);
		num_args_to_strip = 2;
	    }
	    else if ( (strcmp(&(*argvp)[1][1], "gdir") == 0) || (strcmp(&(*argvp)[1][1], "gwdir") == 0) )
	    {
		if (argc < 3)
		{
		    printf("Error: no directory after %s option\n", (*argvp)[1]);
		    smpd_exit_fn(FCNAME);
		    return SMPD_FAIL;
		}
		strncpy(gwdir, (*argvp)[2], SMPD_MAX_DIR_LENGTH);
		num_args_to_strip = 2;
	    }
	    else if (strcmp(&(*argvp)[1][1], "env") == 0)
	    {
		if (argc < 4)
		{
		    printf("Error: no environment variable after -env option\n");
		    smpd_exit_fn(FCNAME);
		    return SMPD_FAIL;
		}
		env_node = (smpd_env_node_t*)MPIU_Malloc(sizeof(smpd_env_node_t));
		if (env_node == NULL)
		{
		    printf("Error: malloc failed to allocate structure to hold an environment variable.\n");
		    smpd_exit_fn(FCNAME);
		    return SMPD_FAIL;
		}
		strncpy(env_node->name, (*argvp)[2], SMPD_MAX_NAME_LENGTH);
		strncpy(env_node->value, (*argvp)[3], SMPD_MAX_VALUE_LENGTH);
		env_node->next = env_list;
		env_list = env_node;
		if (strcmp(env_node->name, "MPI_DLL_NAME") == 0)
		{
		    MPIU_Strncpy(smpd_process.env_dll, env_node->value, SMPD_MAX_FILENAME);
		}
		if (strcmp(env_node->name, "MPI_WRAP_DLL_NAME") == 0)
		{
		    MPIU_Strncpy(smpd_process.env_wrap_dll, env_node->value, SMPD_MAX_FILENAME);
		}
#ifdef HAVE_WINDOWS_H
		if ((strcmp(env_node->name, "MPICH_CHOP_ERROR_STACK") == 0) && ((env_node->value[0] == '\0') || (env_node->value[0] == '-')))
		{
		    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
		    if (hConsole != INVALID_HANDLE_VALUE)
		    {
			CONSOLE_SCREEN_BUFFER_INFO info;
			if (GetConsoleScreenBufferInfo(hConsole, &info))
			{
			    /* The user chose default so set the value to the width of the current output console window */
			    snprintf(env_node->value, SMPD_MAX_VALUE_LENGTH, "%d", info.dwMaximumWindowSize.X);
			    /*printf("width = %d\n", info.dwMaximumWindowSize.X);*/
			}
		    }
		}
#endif
		num_args_to_strip = 3;
	    }
	    else if (strcmp(&(*argvp)[1][1], "envlist") == 0){
            char *str, *token, *value;
		    if (argc < 4){
		        printf("Error: no environment variable after -envlist option\n");
		        smpd_exit_fn(FCNAME);
		        return SMPD_FAIL;
		    }
		    str = MPIU_Strdup((*argvp)[2]);
		    if (str == NULL){
		        printf("Error: unable to allocate memory for copying envlist - '%s'\n", (*argvp)[2]);
		        smpd_exit_fn(FCNAME);
		        return SMPD_FAIL;
		    }
		    token = strtok(str, ",");
		    while (token){
		        value = getenv(token);
		        if (value != NULL){
			        env_node = (smpd_env_node_t*)MPIU_Malloc(sizeof(smpd_env_node_t));
			        if (env_node == NULL){
			            printf("Error: malloc failed to allocate structure to hold an environment variable.\n");
			            smpd_exit_fn(FCNAME);
			            return SMPD_FAIL;
			        }
			        strncpy(env_node->name, token, SMPD_MAX_NAME_LENGTH);
			        strncpy(env_node->value, value, SMPD_MAX_VALUE_LENGTH);
			        env_node->next = env_list;
			        env_list = env_node;
		        }
                else{
                    printf("Error: Cannot obtain value of env variable : %s\n", token);
                }
		        token = strtok(NULL, ",");
		    }
		    MPIU_Free(str);
		    num_args_to_strip = 2;
        }
	    else if (strcmp(&(*argvp)[1][1], "envnone") == 0){
            printf("-envnone option is not implemented\n");
            num_args_to_strip = 1;
        }
	    else if (strcmp(&(*argvp)[1][1], "genv") == 0)
	    {
		if (argc < 4)
		{
		    printf("Error: no environment variable after -genv option\n");
		    smpd_exit_fn(FCNAME);
		    return SMPD_FAIL;
		}
		env_node = (smpd_env_node_t*)MPIU_Malloc(sizeof(smpd_env_node_t));
		if (env_node == NULL)
		{
		    printf("Error: malloc failed to allocate structure to hold an environment variable.\n");
		    smpd_exit_fn(FCNAME);
		    return SMPD_FAIL;
		}
		strncpy(env_node->name, (*argvp)[2], SMPD_MAX_NAME_LENGTH);
		strncpy(env_node->value, (*argvp)[3], SMPD_MAX_VALUE_LENGTH);
		env_node->next = genv_list;
		genv_list = env_node;
#ifdef HAVE_WINDOWS_H
		if ((strcmp(env_node->name, "MPICH_CHOP_ERROR_STACK") == 0) && ((env_node->value[0] == '\0') || (env_node->value[0] == '-')))
		{
		    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
		    if (hConsole != INVALID_HANDLE_VALUE)
		    {
			CONSOLE_SCREEN_BUFFER_INFO info;
			if (GetConsoleScreenBufferInfo(hConsole, &info))
			{
			    /* The user chose default so set the value to the width of the current output console window */
			    snprintf(env_node->value, SMPD_MAX_VALUE_LENGTH, "%d", info.dwMaximumWindowSize.X);
			    /*printf("width = %d\n", info.dwMaximumWindowSize.X);*/
			}
		    }
		}
#endif
		num_args_to_strip = 3;
	    }
	    else if (strcmp(&(*argvp)[1][1], "genvall") == 0)
	    {
		printf("-genvall option not implemented\n");
        num_args_to_strip = 1;
	    }
	    else if (strcmp(&(*argvp)[1][1], "genvnone") == 0)
	    {
		printf("-genvnone option not implemented\n");
        num_args_to_strip = 1;
	    }
	    else if (strcmp(&(*argvp)[1][1], "genvlist") == 0)
	    {
		char *str, *token, *value;
		if (argc < 4)
		{
		    printf("Error: no environment variables after -genvlist option\n");
		    smpd_exit_fn(FCNAME);
		    return SMPD_FAIL;
		}
		str = MPIU_Strdup((*argvp)[2]);
		if (str == NULL)
		{
		    printf("Error: unable to allocate memory for a string - '%s'\n", (*argvp)[2]);
		    smpd_exit_fn(FCNAME);
		    return SMPD_FAIL;
		}
		token = strtok(str, ",");
		while (token)
		{
		    value = getenv(token);
		    if (value != NULL)
		    {
			env_node = (smpd_env_node_t*)MPIU_Malloc(sizeof(smpd_env_node_t));
			if (env_node == NULL)
			{
			    printf("Error: malloc failed to allocate structure to hold an environment variable.\n");
			    smpd_exit_fn(FCNAME);
			    return SMPD_FAIL;
			}
			strncpy(env_node->name, token, SMPD_MAX_NAME_LENGTH);
			strncpy(env_node->value, value, SMPD_MAX_VALUE_LENGTH);
			env_node->next = genv_list;
			genv_list = env_node;
		    }
		    token = strtok(NULL, ",");
		}
		MPIU_Free(str);
		num_args_to_strip = 2;
	    }
	    else if ( (strcmp(&(*argvp)[1][1], "logon") == 0) || (strcmp(&(*argvp)[1][1], "login") == 0) )
	    {
		smpd_process.logon = SMPD_TRUE;
	    }
	    else if ( (strcmp(&(*argvp)[1][1], "impersonate") == 0) || (strcmp(&(*argvp)[1][1], "impersonation") == 0) )
	    {
		smpd_process.use_sspi = SMPD_TRUE;
		smpd_process.use_delegation = SMPD_FALSE;
	    }
	    else if ( (strcmp(&(*argvp)[1][1], "delegate") == 0) || (strcmp(&(*argvp)[1][1], "delegation") == 0) )
	    {
		smpd_process.use_sspi = SMPD_TRUE;
		smpd_process.use_delegation = SMPD_TRUE;
	    }
	    else if (strcmp(&(*argvp)[1][1], "job") == 0)
	    {
		smpd_process.use_delegation = SMPD_FALSE;
		smpd_process.use_sspi_job_key = SMPD_TRUE;
		if (argc < 3)
		{
		    printf("Error: no job key specified after the -job option\n");
		    smpd_exit_fn(FCNAME);
		    return SMPD_FAIL;
		}
		strncpy(smpd_process.job_key, (*argvp)[2], SMPD_SSPI_JOB_KEY_LENGTH);
		num_args_to_strip = 2;
	    }
	    else if (strcmp(&(*argvp)[1][1], "dbg") == 0)
	    {
		use_debug_flag = SMPD_TRUE;
	    }
	    else if (strcmp(&(*argvp)[1][1], "pwdfile") == 0)
	    {
		use_pwd_file = SMPD_TRUE;
		if (argc < 3)
		{
		    printf("Error: no filename specified after -pwdfile option\n");
		    smpd_exit_fn(FCNAME);
		    return SMPD_FAIL;
		}
		strncpy(pwd_file_name, (*argvp)[2], SMPD_MAX_FILENAME);
		smpd_get_pwd_from_file(pwd_file_name);
		num_args_to_strip = 2;
	    }
#ifdef HAVE_WINDOWS_H
	    else if (strcmp(&(*argvp)[1][1], "registryfile") == 0)
	    {
        char reg_file_name[SMPD_MAX_FILENAME];
		if (argc < 3)
		{
		    printf("Error: no filename specified after -registryfile option\n");
		    smpd_exit_fn(FCNAME);
		    return SMPD_FAIL;
		}
		strncpy(reg_file_name, (*argvp)[2], SMPD_MAX_FILENAME);
        if(!smpd_read_cred_from_file(reg_file_name, smpd_process.UserAccount, SMPD_MAX_ACCOUNT_LENGTH, smpd_process.UserPassword, SMPD_MAX_PASSWORD_LENGTH)){
            printf("Error: Could not read credentials from registry file\n");
            smpd_exit_fn(FCNAME);
            return SMPD_FAIL;
        }
		num_args_to_strip = 2;
	    }
#endif
	    else if (strcmp(&(*argvp)[1][1], "configfile") == 0)
	    {
		printf("Error: The -configfile option must be the first and only option specified in a block.\n");
		smpd_exit_fn(FCNAME);
		return SMPD_FAIL;
		/*
		if (argc < 3)
		{
		    printf("Error: no filename specifed after -configfile option.\n");
		    smpd_exit_fn(FCNAME);
		    return SMPD_FAIL;
		}
		strncpy(configfilename, (*argvp)[2], SMPD_MAX_FILENAME);
		use_configfile = SMPD_TRUE;
		num_args_to_strip = 2;
		*/
	    }
	    else if (strcmp(&(*argvp)[1][1], "file") == 0)
	    {
		printf("Error: The -file option must be the first and only option specified in a block.\n");
		smpd_exit_fn(FCNAME);
		return SMPD_FAIL;
		/*
		if (argc < 3)
		{
		    printf("Error: no filename specifed after -file option.\n");
		    smpd_exit_fn(FCNAME);
		    return SMPD_FAIL;
		}
		strncpy(configfilename, (*argvp)[2], SMPD_MAX_FILENAME);
		use_configfile = SMPD_TRUE;
		num_args_to_strip = 2;
		*/
	    }
	    else if (strcmp(&(*argvp)[1][1], "host") == 0)
	    {
		if (argc < 3)
		{
		    printf("Error: no host specified after -host option.\n");
		    smpd_exit_fn(FCNAME);
		    return SMPD_FAIL;
		}
		if (host_list != NULL)
		{
		    printf("Error: -host option can only be specified once and it cannot be combined with -hosts.\n");
		    smpd_exit_fn(FCNAME);
		    return SMPD_FAIL;
		}
		/* create a host list of one and set nproc to -1 to be replaced by
		   nproc after parsing the block */
		host_list = (smpd_host_node_t*)MPIU_Malloc(sizeof(smpd_host_node_t));
		if (host_list == NULL)
		{
		    printf("failed to allocate memory for a host node.\n");
		    smpd_exit_fn(FCNAME);
		    return SMPD_FAIL;
		}
		host_list->next = NULL;
		host_list->left = NULL;
		host_list->right = NULL;
		host_list->connected = SMPD_FALSE;
		host_list->connect_cmd_tag = -1;
		host_list->nproc = -1;
		host_list->alt_host[0] = '\0';
		strncpy(host_list->host, (*argvp)[2], SMPD_MAX_HOST_LENGTH);
		num_args_to_strip = 2;
		smpd_add_host_to_default_list((*argvp)[2]);
	    }
	    else if (strcmp(&(*argvp)[1][1], "ghost") == 0)
	    {
		if (argc < 3)
		{
		    printf("Error: no host specified after -ghost option.\n");
		    smpd_exit_fn(FCNAME);
		    return SMPD_FAIL;
		}
		if (ghost_list != NULL)
		{
		    printf("Error: -ghost option can only be specified once.\n");
		    smpd_exit_fn(FCNAME);
		    return SMPD_FAIL;
		}
		/* create a host list of one and set nproc to -1 to be replaced by
		   nproc after parsing the block */
		ghost_list = (smpd_host_node_t*)MPIU_Malloc(sizeof(smpd_host_node_t));
		if (ghost_list == NULL)
		{
		    printf("failed to allocate memory for a host node.\n");
		    smpd_exit_fn(FCNAME);
		    return SMPD_FAIL;
		}
		ghost_list->next = NULL;
		ghost_list->left = NULL;
		ghost_list->right = NULL;
		ghost_list->connected = SMPD_FALSE;
		ghost_list->connect_cmd_tag = -1;
		ghost_list->nproc = -1;
		ghost_list->alt_host[0] = '\0';
		strncpy(ghost_list->host, (*argvp)[2], SMPD_MAX_HOST_LENGTH);
		num_args_to_strip = 2;
		smpd_add_host_to_default_list((*argvp)[2]);
	    }
#ifdef HAVE_WINDOWS_H
        else if (strcmp(&(*argvp)[1][1], "ms_hpc") == 0)
        {
            smpd_process.use_ms_hpc = SMPD_TRUE;
            /* Enable SSPI for authenticating PMs */
            smpd_process.use_sspi = SMPD_TRUE;
            smpd_process.use_delegation = SMPD_FALSE;
            /* Use mpiexec as PMI server */
            smpd_process.use_pmi_server = SMPD_TRUE;

            num_args_to_strip = 1;
        }
#endif
	    else if (strcmp(&(*argvp)[1][1], "hosts") == 0)
	    {
		if (nproc != 0)
		{
		    printf("Error: only one option is allowed to determine the number of processes.\n");
		    printf("       -hosts, -n, -np and -localonly x are mutually exclusive\n");
		    smpd_exit_fn(FCNAME);
		    return SMPD_FAIL;
		}
		if (host_list != NULL)
		{
		    printf("Error: -hosts option can only be called once and it cannot be combined with -host.\n");
		    smpd_exit_fn(FCNAME);
		    return SMPD_FAIL;
		}
		if (argc > 2)
		{
		    if (smpd_isnumber((*argvp)[2]))
		    {
			/* initially set nproc to be the number of hosts */
			nproc = atoi((*argvp)[2]);
			if (nproc < 1)
			{
			    printf("Error: You must specify a number greater than 0 after -hosts.\n");
			    smpd_exit_fn(FCNAME);
			    return SMPD_FAIL;
			}
			num_args_to_strip = 2 + nproc;
			index = 3;
			for (i=0; i<nproc; i++)
			{
			    if (index >= argc)
			    {
				printf("Error: missing host name after -hosts option.\n");
				smpd_exit_fn(FCNAME);
				return SMPD_FAIL;
			    }
			    host_node_ptr = (smpd_host_node_t*)MPIU_Malloc(sizeof(smpd_host_node_t));
			    if (host_node_ptr == NULL)
			    {
				printf("failed to allocate memory for a host node.\n");
				smpd_exit_fn(FCNAME);
				return SMPD_FAIL;
			    }
			    host_node_ptr->next = NULL;
			    host_node_ptr->left = NULL;
			    host_node_ptr->right = NULL;
			    host_node_ptr->connected = SMPD_FALSE;
			    host_node_ptr->connect_cmd_tag = -1;
			    host_node_ptr->nproc = 1;
			    host_node_ptr->alt_host[0] = '\0';
			    strncpy(host_node_ptr->host, (*argvp)[index], SMPD_MAX_HOST_LENGTH);
			    smpd_add_host_to_default_list((*argvp)[index]);
			    index++;
			    if (argc > index)
			    {
				if (smpd_isnumber((*argvp)[index]))
				{
				    host_node_ptr->nproc = atoi((*argvp)[index]);
				    index++;
				    num_args_to_strip++;
				}
			    }
			    if (host_list == NULL)
			    {
				host_list = host_node_ptr;
			    }
			    else
			    {
				host_node_iter = host_list;
				while (host_node_iter->next)
				    host_node_iter = host_node_iter->next;
				host_node_iter->next = host_node_ptr;
			    }
			}

			/* adjust nproc to be the actual number of processes */
			host_node_iter = host_list;
			nproc = 0;
			while (host_node_iter)
			{
			    nproc += host_node_iter->nproc;
			    host_node_iter = host_node_iter->next;
			}
		    }
		    else
		    {
			printf("Error: You must specify the number of hosts after the -hosts option.\n");
			smpd_exit_fn(FCNAME);
			return SMPD_FAIL;
		    }
		}
		else
		{
		    printf("Error: not enough arguments specified for -hosts option.\n");
		    smpd_exit_fn(FCNAME);
		    return SMPD_FAIL;
		}
	    }
	    else if (strcmp(&(*argvp)[1][1], "nocolor") == 0)
	    {
		smpd_process.do_multi_color_output = SMPD_FALSE;
	    }
	    else if (strcmp(&(*argvp)[1][1], "nompi") == 0)
	    {
		smpd_process.no_mpi = SMPD_TRUE;
	    }
	    else if (strcmp(&(*argvp)[1][1], "nomapping") == 0)
	    {
		no_drive_mapping = SMPD_TRUE;
	    }
	    else if (strcmp(&(*argvp)[1][1], "nopopup_debug") == 0)
	    {
#ifdef HAVE_WINDOWS_H
		SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);
#endif
	    }
	    else if (strcmp(&(*argvp)[1][1], "help") == 0 || (*argvp)[1][1] == '?')
	    {
		mp_print_options();
		exit(0);
	    }
	    else if (strcmp(&(*argvp)[1][1], "help2") == 0)
	    {
		mp_print_extra_options();
		exit(0);
	    }
	    else if (strcmp(&(*argvp)[1][1], "exitcodes") == 0)
	    {
		smpd_process.output_exit_codes = SMPD_TRUE;
	    }
	    else if (strcmp(&(*argvp)[1][1], "localroot") == 0)
	    {
		smpd_process.local_root = SMPD_TRUE;
	    }
	    else if (strcmp(&(*argvp)[1][1], "priority") == 0)
	    {
		if (argc < 3)
		{
		    printf("Error: you must specify a priority after the -priority option.\n");
		    smpd_exit_fn(FCNAME);
		    return SMPD_FAIL;
		}
		if (smpd_isnumbers_with_colon((*argvp)[2]))
		{
		    char *str;
		    n_priority_class = atoi((*argvp)[2]); /* This assumes atoi will stop at the colon and return a number */
		    str = strchr((*argvp)[2], ':');
		    if (str)
		    {
			str++;
			n_priority = atoi(str);
		    }
		    if (n_priority_class < 0 || n_priority_class > 4 || n_priority < 0 || n_priority > 5)
		    {
			printf("Error: priorities must be between 0-4:0-5\n");
			smpd_exit_fn(FCNAME);
			return SMPD_FAIL;
		    }
		}
		else
		{
		    printf("Error: you must specify a priority after the -priority option.\n");
		    smpd_exit_fn(FCNAME);
		    return SMPD_FAIL;
		}
		smpd_dbg_printf("priorities = %d:%d\n", n_priority_class, n_priority);
		num_args_to_strip = 2;
	    }
	    else if (strcmp(&(*argvp)[1][1], "iproot") == 0)
	    {
		smpd_process.use_iproot = SMPD_TRUE;
	    }
	    else if (strcmp(&(*argvp)[1][1], "noiproot") == 0)
	    {
		smpd_process.use_iproot = SMPD_FALSE;
	    }
	    else if (strcmp(&(*argvp)[1][1], "verbose") == 0)
	    {
		smpd_process.verbose = SMPD_TRUE;
		smpd_process.dbg_state |= SMPD_DBG_STATE_ERROUT | SMPD_DBG_STATE_STDOUT | SMPD_DBG_STATE_TRACE;
	    }
	    else if ( (strcmp(&(*argvp)[1][1], "p") == 0) || (strcmp(&(*argvp)[1][1], "port") == 0))
	    {
		if (argc > 2)
		{
		    if (smpd_isnumber((*argvp)[2]))
		    {
			smpd_process.port = atoi((*argvp)[2]);
		    }
		    else
		    {
			printf("Error: you must specify the port smpd is listening on after the %s option.\n", (*argvp)[1]);
			smpd_exit_fn(FCNAME);
			return SMPD_FAIL;
		    }
		}
		else
		{
		    printf("Error: you must specify the port smpd is listening on after the %s option.\n", (*argvp)[1]);
		    smpd_exit_fn(FCNAME);
		    return SMPD_FAIL;
		}
		num_args_to_strip = 2;
	    }
	    else if (strcmp(&(*argvp)[1][1], "path") == 0)
	    {
		if (argc < 3)
		{
		    printf("Error: no path specifed after -path option.\n");
		    smpd_exit_fn(FCNAME);
		    return SMPD_FAIL;
		}
		strncpy(path, (*argvp)[2], SMPD_MAX_PATH_LENGTH);
		num_args_to_strip = 2;
	    }
	    else if (strcmp(&(*argvp)[1][1], "gpath") == 0)
	    {
		if (argc < 3)
		{
		    printf("Error: no path specifed after -gpath option.\n");
		    smpd_exit_fn(FCNAME);
		    return SMPD_FAIL;
		}
		strncpy(gpath, (*argvp)[2], SMPD_MAX_PATH_LENGTH);
		num_args_to_strip = 2;
	    }
	    else if (strcmp(&(*argvp)[1][1], "noprompt") == 0)
	    {
		smpd_process.noprompt = SMPD_TRUE;
		smpd_process.credentials_prompt = SMPD_FALSE;
	    }
	    else if (strcmp(&(*argvp)[1][1], "phrase") == 0)
	    {
		if (argc < 3)
		{
		    printf("Error: no passphrase specified afterh -phrase option.\n");
		    smpd_exit_fn(FCNAME);
		    return SMPD_FAIL;
		}
		strncpy(smpd_process.passphrase, (*argvp)[2], SMPD_PASSPHRASE_MAX_LENGTH);
		num_args_to_strip = 2;
	    }
	    else if (strcmp(&(*argvp)[1][1], "smpdfile") == 0)
	    {
		if (argc < 3)
		{
		    printf("Error: no file name specified after -smpdfile option.\n");
		    smpd_exit_fn(FCNAME);
		    return SMPD_FAIL;
		}
		strncpy(smpd_process.smpd_filename, (*argvp)[2], SMPD_MAX_FILENAME);
		{
		    struct stat s;

		    if (stat(smpd_process.smpd_filename, &s) == 0)
		    {
			if (s.st_mode & 00077)
			{
			    printf("Error: .smpd file cannot be readable by anyone other than the current user.\n");
			    smpd_exit_fn(FCNAME);
			    return SMPD_FAIL;
			}
		    }
		}
		num_args_to_strip = 2;
	    }
	    else if (strcmp(&(*argvp)[1][1], "timeout") == 0)
	    {
		if (argc < 3)
		{
		    printf("Error: no timeout specified after -timeout option.\n");
		    smpd_exit_fn(FCNAME);
		    return SMPD_FAIL;
		}
		smpd_process.timeout = atoi((*argvp)[2]);
		if (smpd_process.timeout < 1)
		{
		    printf("Warning: invalid timeout specified, ignoring timeout value of '%s'\n", (*argvp)[2]);
		    smpd_process.timeout = -1;
		}
		num_args_to_strip = 2;
	    }
	    else if (strcmp(&(*argvp)[1][1], "hide_console") == 0)
	    {
#ifdef HAVE_WINDOWS_H
		FreeConsole();
#endif
	    }
	    else if (strcmp(&(*argvp)[1][1], "quiet_abort") == 0)
	    {
		smpd_process.verbose_abort_output = SMPD_FALSE;
	    }
	    else if ((strcmp(&(*argvp)[1][1], "rsh") == 0) || (strcmp(&(*argvp)[1][1], "ssh") == 0))
	    {
		smpd_process.rsh_mpiexec = SMPD_TRUE;
		if (smpd_process.mpiexec_inorder_launch == SMPD_FALSE)
		{
		    smpd_launch_node_t *temp_node, *ordered_list = NULL;
		    /* sort any existing reverse order nodes to be in order */
		    while (smpd_process.launch_list)
		    {
			temp_node = smpd_process.launch_list->next;
			smpd_process.launch_list->next = ordered_list;
			ordered_list = smpd_process.launch_list;
			smpd_process.launch_list = temp_node;
		    }
		    smpd_process.launch_list = ordered_list;
		}
		smpd_process.mpiexec_inorder_launch = SMPD_TRUE;
	    }
	    else if ((strcmp(&(*argvp)[1][1], "nosmpd") == 0) || (strcmp(&(*argvp)[1][1], "no_smpd") == 0) || (strcmp(&(*argvp)[1][1], "nopm") == 0))
	    {
		smpd_process.use_pmi_server = SMPD_FALSE;
	    }
	    else if (strcmp(&(*argvp)[1][1], "plaintext") == 0)
	    {
		smpd_process.plaintext = SMPD_TRUE;
	    }
	    else if (strcmp(&(*argvp)[1][1], "channel") == 0)
	    {
		if (argc < 3)
		{
		    printf("Error: no name specified after -channel option.\n");
		    smpd_exit_fn(FCNAME);
		    return SMPD_FAIL;
		}
		strncpy(channel, (*argvp)[2], SMPD_MAX_NAME_LENGTH);
        if((strcmp(channel, "shm") == 0) || (strcmp(channel, "ssm") == 0))
        {
            printf("WARNING: SHM & SSM channels are no longer available. Use the NEMESIS channel instead.\n");
            return SMPD_FAIL;
        }
		num_args_to_strip = 2;
	    }
	    else if (strcmp(&(*argvp)[1][1], "log") == 0)
	    {
		/* -log is a shortcut to create log files using the mpe wrapper dll */
		env_node = (smpd_env_node_t*)MPIU_Malloc(sizeof(smpd_env_node_t));
		if (env_node == NULL)
		{
		    printf("Error: malloc failed to allocate structure to hold an environment variable.\n");
		    smpd_exit_fn(FCNAME);
		    return SMPD_FAIL;
		}
		strncpy(env_node->name, "MPI_WRAP_DLL_NAME", SMPD_MAX_NAME_LENGTH);
		strncpy(env_node->value, "mpe", SMPD_MAX_VALUE_LENGTH);
		env_node->next = env_list;
		env_list = env_node;
	    }
#ifdef HAVE_WINDOWS_H
	    else if (strcmp(&(*argvp)[1][1], "user") == 0)
	    {
		if (argc < 3)
		{
		    printf("Error: no index specified after -user option.\n");
		    smpd_exit_fn(FCNAME);
		    return SMPD_FAIL;
		}
		smpd_process.user_index = atoi((*argvp)[2]);
		if (smpd_process.user_index < 0)
		    smpd_process.user_index = 0;
		num_args_to_strip = 2;
		smpd_delete_cached_password();
	    }
#endif
	    else if (strcmp(&(*argvp)[1][1], "l") == 0)
	    {
		smpd_process.prefix_output = SMPD_TRUE;
	    }
	    else
	    {
		printf("Unknown option: %s\n", (*argvp)[1]);
	    }
	    strip_args(argcp, argvp, num_args_to_strip);
	}

	/* check to see if a timeout is specified by the environment variable only if
	 * a timeout has not been specified on the command line
	 */
	if (smpd_process.timeout == -1)
	{
	    char *p = getenv("MPIEXEC_TIMEOUT");
	    if (p)
	    {
		smpd_process.timeout = atoi(p);
		if (smpd_process.timeout < 1)
		{
		    smpd_process.timeout = -1;
		}
	    }
	    else
	    {
		/* If a timeout is specified in the smpd settings but not in an env variable and not on the command line then use it. */
		if (smpd_setting_timeout != SMPD_INVALID_SETTING)
		{
		    smpd_process.timeout = smpd_setting_timeout;
		}
	    }
	}

	/* Check to see if the environment wants all processes to run locally.
	 * This is useful for test scripts.
	 */
	env_str = getenv("MPIEXEC_LOCALONLY");
	if (env_str == NULL && smpd_setting_localonly == SMPD_TRUE)
	{
	    smpd_setting_tmp_buffer[0] = '1';
	    smpd_setting_tmp_buffer[1] = '\0';
	    env_str = smpd_setting_tmp_buffer;
	}
	if (env_str != NULL)
	{
	    if (smpd_is_affirmative(env_str) || strcmp(env_str, "1") == 0)
	    {
#if 1
		/* This block creates a host list of one host to implement -localonly */

		if (host_list == NULL)
		{
		    /* Use localroot to implement localonly */
		    smpd_process.local_root = SMPD_TRUE;
		    /* create a host list of one and set nproc to -1 to be replaced by 
		       nproc after parsing the block */
		    host_list = (smpd_host_node_t*)MPIU_Malloc(sizeof(smpd_host_node_t));
		    if (host_list == NULL)
		    {
			printf("failed to allocate memory for a host node.\n");
			smpd_exit_fn(FCNAME);
			return SMPD_FAIL;
		    }
		    host_list->next = NULL;
		    host_list->left = NULL;
		    host_list->right = NULL;
		    host_list->connected = SMPD_FALSE;
		    host_list->connect_cmd_tag = -1;
		    host_list->nproc = -1;
		    host_list->alt_host[0] = '\0';
		    smpd_get_hostname(host_list->host, SMPD_MAX_HOST_LENGTH);
		}
		else
		{
		    smpd_dbg_printf("host_list not null, not using localonly\n");
		}
#else
		/* This block uses the rsh code to implement -localonly */

		smpd_process.mpiexec_run_local = SMPD_TRUE;
		smpd_process.rsh_mpiexec = SMPD_TRUE;
		if (smpd_process.mpiexec_inorder_launch == SMPD_FALSE)
		{
		    smpd_launch_node_t *temp_node, *ordered_list = NULL;
		    /* sort any existing reverse order nodes to be in order */
		    while (smpd_process.launch_list)
		    {
			temp_node = smpd_process.launch_list->next;
			smpd_process.launch_list->next = ordered_list;
			ordered_list = smpd_process.launch_list;
			smpd_process.launch_list = temp_node;
		    }
		    smpd_process.launch_list = ordered_list;
		}
		smpd_process.mpiexec_inorder_launch = SMPD_TRUE;
#endif
	    }
	    else
	    {
		smpd_dbg_printf("MPIEXEC_LOCALONLY env is not affirmative: '%s'\n", env_str);
	    }
	}

	/* remaining args are the executable and it's args */
	if (argc < 2)
	{
	    printf("Error: no executable specified\n");
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}

	exe_iter = exe;
	exe_len_remaining = SMPD_MAX_EXE_LENGTH;
	if (!((*argvp)[1][0] == '\\' && (*argvp)[1][1] == '\\') && (*argvp)[1][0] != '/' &&
	    !(strlen((*argvp)[1]) > 3 && (*argvp)[1][1] == ':' && (*argvp)[1][2] == '\\') )
	{
	    /* an absolute path was not specified so find the executable an save the path */
	    if (smpd_get_full_path_name((*argvp)[1], SMPD_MAX_EXE_LENGTH, exe_path, &namepart))
	    {
		if (path[0] != '\0')
		{
		    if (strlen(path) < SMPD_MAX_PATH_LENGTH)
		    {
			strcat(path, ";");
			strncat(path, exe_path, SMPD_MAX_PATH_LENGTH - strlen(path));
			path[SMPD_MAX_PATH_LENGTH-1] = '\0';
		    }
		}
		else
		{
		    strncpy(path, exe_path, SMPD_MAX_PATH_LENGTH);
		}
		result = MPIU_Str_add_string(&exe_iter, &exe_len_remaining, namepart);
		if (result != MPIU_STR_SUCCESS)
		{
		    printf("Error: insufficient buffer space for the command line.\n");
		    smpd_exit_fn(FCNAME);
		    return SMPD_FAIL;
		}
	    }
	    else
	    {
		result = MPIU_Str_add_string(&exe_iter, &exe_len_remaining, (*argvp)[1]);
		if (result != MPIU_STR_SUCCESS)
		{
		    printf("Error: insufficient buffer space for the command line.\n");
		    smpd_exit_fn(FCNAME);
		    return SMPD_FAIL;
		}
	    }
	}
	else
	{
	    /* an absolute path was specified */
#ifdef HAVE_WINDOWS_H
	    char *pTemp = (char*)MPIU_Malloc(SMPD_MAX_EXE_LENGTH);
	    if (pTemp == NULL)
	    {
		smpd_exit_fn(FCNAME);
		return SMPD_FAIL;
	    }
	    strncpy(pTemp, (*argvp)[1], SMPD_MAX_EXE_LENGTH);
	    pTemp[SMPD_MAX_EXE_LENGTH-1] = '\0';
	    ExeToUnc(pTemp, SMPD_MAX_EXE_LENGTH);
	    result = MPIU_Str_add_string(&exe_iter, &exe_len_remaining, pTemp);
	    MPIU_Free(pTemp);
	    if (result != MPIU_STR_SUCCESS)
	    {
		printf("Error: insufficient buffer space for the command line.\n");
		smpd_exit_fn(FCNAME);
		return SMPD_FAIL;
	    }
#else
	    result = MPIU_Str_add_string(&exe_iter, &exe_len_remaining, (*argvp)[1]);
	    if (result != MPIU_STR_SUCCESS)
	    {
		printf("Error: insufficient buffer space for the command line.\n");
		smpd_exit_fn(FCNAME);
		return SMPD_FAIL;
	    }
#endif
	}
	for (i=2; i<argc; i++)
	{
	    result = MPIU_Str_add_string(&exe_iter, &exe_len_remaining, (*argvp)[i]);
	}
	/* remove the trailing space */
	exe[strlen(exe)-1] = '\0';
	smpd_dbg_printf("handling executable:\n%s\n", exe);

	if (nproc == 0){
    /* By default assume "mpiexec foo" => "mpiexec -n 1 foo" */
        nproc = 1;
	}
	if (ghost_list != NULL && host_list == NULL && use_machine_file != SMPD_TRUE)
	{
	    host_list = (smpd_host_node_t*)MPIU_Malloc(sizeof(smpd_host_node_t));
	    if (host_list == NULL)
	    {
		printf("failed to allocate memory for a host node.\n");
		smpd_exit_fn(FCNAME);
		return SMPD_FAIL;
	    }
	    host_list->next = NULL;
	    host_list->left = NULL;
	    host_list->right = NULL;
	    host_list->connected = SMPD_FALSE;
	    host_list->connect_cmd_tag = -1;
	    host_list->nproc = -1;
	    host_list->alt_host[0] = '\0';
	    strncpy(host_list->host, ghost_list->host, SMPD_MAX_HOST_LENGTH);
	}
	if (host_list != NULL && host_list->nproc == -1)
	{
	    /* -host specified, replace nproc field */
	    host_list->nproc = nproc;
	}

    smpd_dbg_printf("Processing environment variables \n");
	/* add environment variables */
	env_data[0] = '\0';
	env_str = env_data;
	maxlen = SMPD_MAX_ENV_LENGTH;
	/* add and destroy the local environment variable list */
	while (env_list)
	{
	    MPIU_Str_add_string_arg(&env_str, &maxlen, env_list->name, env_list->value);
	    /*
	    env_str += snprintf(env_str,
		SMPD_MAX_ENV_LENGTH - (env_str - env_data),
		"%s=%s", env_list->name, env_list->value);
	    if (env_list->next)
	    {
		env_str += snprintf(env_str, SMPD_MAX_ENV_LENGTH - (env_str - env_data), ";");
	    }
	    */
	    env_node = env_list;
	    env_list = env_list->next;
	    MPIU_Free(env_node);
	}
	/* add the global environment variable list */
	env_node = genv_list;
	while (env_node)
	{
	    MPIU_Str_add_string_arg(&env_str, &maxlen, env_node->name, env_node->value);
	    env_node = env_node->next;
	}
	if (env_str > env_data)
	{
	    /* trim the trailing white space */
	    env_str--;
	    *env_str = '\0';
	}

    smpd_dbg_printf("Processing drive mappings\n");
	/* merge global drive mappings with the local drive mappings */
	gmap_node = gdrive_map_list;
	while (gmap_node)
	{
	    map_node = drive_map_list;
	    while (map_node)
	    {
		if (map_node->drive == gmap_node->drive)
		{
		    /* local option trumps the global option */
		    break;
		}
		if (map_node->next == NULL)
		{
		    /* add a copy of the global node to the end of the list */
		    map_node->next = (smpd_map_drive_node_t*)MPIU_Malloc(sizeof(smpd_map_drive_node_t));
		    if (map_node->next == NULL)
		    {
			printf("Error: malloc failed to allocate map structure.\n");
			smpd_exit_fn(FCNAME);
			return SMPD_FAIL;
		    }
		    map_node = map_node->next;
		    map_node->ref_count = 0;
		    map_node->drive = gmap_node->drive;
		    strncpy(map_node->share, gmap_node->share, SMPD_MAX_EXE_LENGTH);
		    map_node->next = NULL;
		    break;
		}
		map_node = map_node->next;
	    }
	    if (drive_map_list == NULL)
	    {
		map_node = (smpd_map_drive_node_t*)MPIU_Malloc(sizeof(smpd_map_drive_node_t));
		if (map_node == NULL)
		{
		    printf("Error: malloc failed to allocate map structure.\n");
		    smpd_exit_fn(FCNAME);
		    return SMPD_FAIL;
		}
		map_node->ref_count = 0;
		map_node->drive = gmap_node->drive;
		strncpy(map_node->share, gmap_node->share, SMPD_MAX_EXE_LENGTH);
		map_node->next = NULL;
		drive_map_list = map_node;
	    }
	    gmap_node = gmap_node->next;
	}

    smpd_dbg_printf("Creating launch nodes (%d)\n", nproc);
	for (i=0; i<nproc; i++)
	{
	    /* create a launch_node */
	    launch_node = (smpd_launch_node_t*)MPIU_Malloc(sizeof(smpd_launch_node_t));
	    if (launch_node == NULL)
	    {
		smpd_err_printf("unable to allocate a launch node structure.\n");
		smpd_exit_fn(FCNAME);
		return SMPD_FAIL;
	    }
	    launch_node->clique[0] = '\0';
	    smpd_get_next_host(&host_list, launch_node);
        smpd_dbg_printf("Adding host (%s) to launch list \n", launch_node->hostname);
	    launch_node->iproc = cur_rank++;
#ifdef HAVE_WINDOWS_H
        if(smpd_process.affinity_map_sz > 0){
            launch_node->binding_proc =
                smpd_process.affinity_map[affinity_map_index % smpd_process.affinity_map_sz];
            affinity_map_index++;
        }
        else{
            launch_node->binding_proc = -1;
        }
#endif
	    launch_node->appnum = appnum;
	    launch_node->priority_class = n_priority_class;
	    launch_node->priority_thread = n_priority;
	    launch_node->env = launch_node->env_data;
	    strcpy(launch_node->env_data, env_data);
	    if (launch_node->alt_hostname[0] != '\0')
	    {
		if (smpd_append_env_option(launch_node->env_data, SMPD_MAX_ENV_LENGTH, "MPICH_INTERFACE_HOSTNAME", launch_node->alt_hostname) != SMPD_SUCCESS)
		{
		    smpd_err_printf("unable to add the MPICH_INTERFACE_HOSTNAME environment variable to the launch command.\n");
		    smpd_exit_fn(FCNAME);
		    return SMPD_FAIL;
		}
	    }
	    if (wdir[0] != '\0')
	    {
		strcpy(launch_node->dir, wdir);
	    }
	    else
	    {
		if (gwdir[0] != '\0')
		{
		    strcpy(launch_node->dir, gwdir);
		}
		else
		{
		    launch_node->dir[0] = '\0';
		    getcwd(launch_node->dir, SMPD_MAX_DIR_LENGTH);
		}
	    }
	    if (path[0] != '\0')
	    {
		strcpy(launch_node->path, path);
		/* should the gpath be appended to the local path? */
	    }
	    else
	    {
		if (gpath[0] != '\0')
		{
		    strcpy(launch_node->path, gpath);
		}
		else
		{
		    launch_node->path[0] = '\0';
		}
	    }
	    launch_node->map_list = drive_map_list;
	    if (drive_map_list)
	    {
		/* ref count the list so when freeing the launch_node it can be known when to free the list */
		drive_map_list->ref_count++;
	    }
	    strcpy(launch_node->exe, exe);
	    launch_node->args[0] = '\0';
	    if (smpd_process.mpiexec_inorder_launch == SMPD_TRUE)
	    {
		/* insert the node in order */
		launch_node->next = NULL;
		if (smpd_process.launch_list == NULL)
		{
		    smpd_process.launch_list = launch_node;
		    launch_node->prev = NULL;
		}
		else
		{
		    launch_node_iter = smpd_process.launch_list;
		    while (launch_node_iter->next)
			launch_node_iter = launch_node_iter->next;
		    launch_node_iter->next = launch_node;
		    launch_node->prev = launch_node_iter;
		}
	    }
	    else
	    {
		/* insert the node in reverse order */
		launch_node->next = smpd_process.launch_list;
		if (smpd_process.launch_list)
		    smpd_process.launch_list->prev = launch_node;
		smpd_process.launch_list = launch_node;
		launch_node->prev = NULL;
	    }
	}

	/* advance the application number for each : separated command line block or line in a configuration file */
	appnum++;

	if (smpd_process.s_host_list)
	{
	    /* free the current host list */
	    while (smpd_process.s_host_list)
	    {
		host_node_iter = smpd_process.s_host_list;
		smpd_process.s_host_list = smpd_process.s_host_list->next;
		MPIU_Free(host_node_iter);
	    }
	}

	if (use_configfile)
	{
	    if (smpd_get_argcv_from_file(fin_config, argcp, argvp))
	    {
		/* FIXME: How do we tell the difference between an error parsing the file and parsing the last entry? */
		goto configfile_loop;
	    }
	    fclose(fin_config);
	    if (delete_configfile)
	    {
		unlink(configfilename);
	    }
	}

	/* move to the next block */
	*argvp = next_argv - 1;
	if (*next_argv)
	    **argvp = exe_ptr;
    } while (*next_argv);

    launch_node_iter = smpd_process.launch_list;
    while (launch_node_iter)
    {
        /* add nproc to all the launch nodes */
        launch_node_iter->nproc = cur_rank;
    	launch_node_iter = launch_node_iter->next;
    }

    /* create the cliques */
    smpd_create_cliques(smpd_process.launch_list);

    if (smpd_process.launch_list != NULL)
    {
	if ((channel[0] == '\0') && (smpd_setting_channel[0] != '\0'))
	{
	    /* channel specified in the smpd settings */
	    strncpy(channel, smpd_setting_channel, 20);
	}
	if (channel[0] != '\0')
	{
	    smpd_launch_node_t *iter;
	    /* If the user specified auto channel selection then set the channel here */
	    /* shm < 8 processes on one node
	    * ssm multiple nodes
	    */
	    if ((strcmp(channel, "auto") == 0) && (smpd_process.launch_list != NULL))
	    {
            strncpy(channel, "nemesis", SMPD_MAX_NAME_LENGTH);
	    }

        nemesis_netmod[0] = '\0';
#ifdef HAVE_WINDOWS_H
        /* See if there is a netmod specified with the channel
         * eg: -channel nemesis:newtcp | -channel nemesis:nd | -channel nemesis:none
         */
        if(strlen(channel) > 0){
            char seps[]=":";
            char *tok, *ctxt;

            tok = strtok_s(channel, seps, &ctxt);
            if(tok != NULL){
                tok = strtok_s(NULL, seps, &ctxt);
                if(tok != NULL){
                    MPIU_Strncpy(nemesis_netmod, tok, SMPD_MAX_NEMESIS_NETMOD_LENGTH);
                }
            }
        }
#endif

	    /* add the channel to the environment of each process */
	    iter = smpd_process.launch_list;
	    while (iter)
	    {
		maxlen = (int)strlen(iter->env);
		env_str = &iter->env[maxlen];
		maxlen = SMPD_MAX_ENV_LENGTH - maxlen - 1;

		if (maxlen > 15) /* At least 16 characters are needed to store MPICH2_CHANNEL=x */
		{
		    *env_str = ' ';
		    env_str++;
		    MPIU_Str_add_string_arg(&env_str, &maxlen, "MPICH2_CHANNEL", channel);
		    /* trim the trailing white space */
		    env_str--;
		    *env_str = '\0';
		}
		if ((strlen(nemesis_netmod) > 0) && (maxlen > 32)) /* At least 31 characters are needed to store " MPICH_NEMESIS_NETMOD=x" */
		{
		    *env_str = ' ';
		    env_str++;
		    MPIU_Str_add_string_arg(&env_str, &maxlen, "MPICH_NEMESIS_NETMOD", nemesis_netmod);
		    /* trim the trailing white space */
		    env_str--;
		    *env_str = '\0';
		}

		iter = iter->next;
	    }

	    /* Save the channel to be used by spawn commands */
	    MPIU_Strncpy(smpd_process.env_channel, channel, 10);
        if(strlen(nemesis_netmod) > 0){
            MPIU_Strncpy(smpd_process.env_netmod, nemesis_netmod, 10);
        }
	}
    }

    smpd_fix_up_host_tree(smpd_process.host_list);

    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}
