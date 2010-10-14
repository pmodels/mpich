/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef SMPD_H
#define SMPD_H

#include "mpichconf.h"
#include "smpdconf.h"

#ifdef HAVE_WINDOWS_H
#include <winsock2.h>
#include <windows.h>
#include <tchar.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <io.h>
#define SECURITY_WIN32
#include <security.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif
#ifdef HAVE_UUID_UUID_H
#include <uuid/uuid.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif
#ifdef USE_PTHREAD_STDIN_REDIRECTION
#include <pthread.h>
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif
#endif

#include "mpl.h"
#include "mpimem.h"
#include "smpd_database.h"
#include "pmi.h"

#include "smpd_version.h"
#include "smpd_iov.h"
#include "smpd_util_sock.h"

#define SMPD_VERSION_FAILURE "version_failure"

#define SMPD_LISTENER_PORT               8676

#define SMPD_SUCCESS                        0
#define SMPD_FAIL                          -1
#define SMPD_ERR_INVALID_USER              -2
#define SMPD_CLOSE                          2
#define SMPD_CONNECTED                      3
#define SMPD_EXIT                           4
#define SMPD_DBS_RETURN                     5
#define SMPD_ABORT                          6

/* #define SMPD_LAST_RETVAL                    7 */

/* FIXME: Use an enum instead
enum SMPD_RetVal{
    SMPD_ERR_INVALID_USER=-3,
    SMPD_ABORT,
    SMPD_FAIL,
    SMPD_SUCCESS,
    SMPD_DBS_RETURN,
    SMPD_CONNECTED,
    SMPD_CLOSE,
    SMPD_EXIT,
    SMPD_LAST_RETVAL
};
*/

typedef int SMPD_BOOL;
#define SMPD_TRUE                           1
#define SMPD_FALSE                          0
#define SMPD_INVALID_SETTING               -1

#define SMPD_SERVER_AUTHENTICATION          0
#define SMPD_CLIENT_AUTHENTICATION          1

#define SMPD_MAX_INT_LENGTH                12
#define SMPD_MAX_NAME_LENGTH              256
#define SMPD_MAX_VALUE_LENGTH            8192
#define SMPD_MAX_FILENAME                1024
#define SMPD_MAX_LINE_LENGTH            32768
#define SMPD_MAX_ARGC                    1024
/*#define SMPD_MAX_STDOUT_LENGTH           1024*/
#define SMPD_MAX_STDOUT_LENGTH           (SMPD_MAX_CMD_LENGTH - 100)
#define SMPD_MAX_SESSION_HEADER_LENGTH   1024
#define SMPD_MAX_ERROR_LEN               1024
#define SMPD_CMD_HDR_LENGTH                13
#define SMPD_MAX_CMD_LENGTH	        32768
#define SMPD_MAX_DBG_PRINTF_LENGTH      (1024 + SMPD_MAX_CMD_LENGTH)
#define SMPD_MAX_CMD_STR_LENGTH           100
#define SMPD_MAX_HOST_LENGTH	           64
#define SMPD_MAX_HOST_DESC_LENGTH        512
#define SMPD_MAX_FQ_NAME_LENGTH          1024
#define SMPD_MAX_NETBIOS_NAME_LENGTH     1024
#define SMPD_MAX_EXE_LENGTH              4096
#define SMPD_MAX_ENV_LENGTH              4096
#define SMPD_MAX_CLIQUE_LENGTH           8192
#define SMPD_MAX_DIR_LENGTH              1024
#define SMPD_MAX_PATH_LENGTH             1024
#define SMPD_MAX_ACCOUNT_LENGTH           100
#define SMPD_MAX_PASSWORD_LENGTH          200
#define SMPD_MAX_CRED_REQUEST_LENGTH      100
#define SMPD_MAX_PWD_REQUEST_LENGTH       100
#define SMPD_MAX_PORT_STR_LENGTH           20
#define SMPD_MAX_TO_STRING_INDENT          20
#define SMPD_STDIN_PACKET_SIZE           2048
#define SMPD_PASSPHRASE_MAX_LENGTH        256
#define SMPD_SALT_VALUE                   "14"
#define SMPD_SESSION_REQUEST_LEN          100
#define SMPD_AUTHENTICATION_STR_LEN       256
#define SMPD_AUTHENTICATION_REPLY_LENGTH  100
#define SMPD_AUTHENTICATION_REJECTED_STR  "FAIL"
#define SMPD_AUTHENTICATION_REJECTED_VERSION_STR "FAIL_VERSION"
#define SMPD_AUTHENTICATION_ACCEPTED_STR  "SUCCESS"
#define SMPD_SMPD_SESSION_STR             "smpd"
#define SMPD_PROCESS_SESSION_STR          "process"
#define SMPD_PMI_SESSION_STR              "pmi"
#define SMPD_DEFAULT_PASSPHRASE           "behappy"
#define SMPD_DEFAULT_PASSWORD             "gastroduodenostomy"
#define SMPD_REGISTRY_KEY                 "SOFTWARE\\MPICH\\SMPD"
#define SMPD_REGISTRY_CACHE_KEY           "SOFTWARE\\MPICH\\SMPD\\CACHE"
#define MPICH_REGISTRY_KEY                "SOFTWARE\\MPICH"
#define SMPD_CRED_REQUEST                 "credentials"
#define SMPD_CRED_REQUEST_JOB             "job"
#define SMPD_NO_CRED_REQUEST              "nocredentials"
#define SMPD_CRED_ACK_SSPI                "sspi"
#define SMPD_CRED_ACK_SSPI_JOB_KEY        "sspi_job"
#define SMPD_PWD_REQUEST                  "pwd"
#define SMPD_NO_PWD_REQUEST               "nopwd"
#define SMPD_SSPI_REQUEST                 "sspi"
#define SMPD_NO_RECONNECT_PORT_STR        "-1"
#define SMPD_SUCCESS_STR                  "SUCCESS"
#define SMPD_FAIL_STR                     "FAIL"
#define SMPD_OUTPUT_MUTEX_NAME            "SMPD_OUTPUT_MUTEX"
#define SMPD_DATA_MUTEX_NAME              "SMPD_DATA_MUTEX"
#define SMPD_DYNAMIC_HOSTS_KEY            "dynamic_hosts"
#define SMPD_PATH_SPEC                    "{SMPD_PATH}"
#define SMPD_PLAINTEXT_PREFIX             'y'
#define SMPD_ENCRYPTED_PREFIX             'x'
#define SMPD_SSPI_HEADER_LENGTH           SMPD_CMD_HDR_LENGTH
#define SMPD_SSPI_JOB_KEY_LENGTH          100
#define SMPD_SSPI_MAX_BUFFER_SIZE         (4*1024)
/*#define SMPD_SECURITY_PACKAGE             "Kerberos"*/
#define SMPD_SECURITY_PACKAGE             "Negotiate"
#define SMPD_ENV_OPTION_PREFIX            "SMPD_OPTION_"
#define SMPD_FREE_COOKIE           0xDDBEEFDD

#define SMPD_DBG_STATE_STDOUT            0x01
#define SMPD_DBG_STATE_ERROUT            0x02
#define SMPD_DBG_STATE_LOGFILE           0x04
#define SMPD_DBG_STATE_PREPEND_RANK      0x08
#define SMPD_DBG_STATE_TRACE             0x10
#define SMPD_DBG_STATE_ALL               (SMPD_DBG_STATE_STDOUT | SMPD_DBG_STATE_ERROUT | SMPD_DBG_STATE_LOGFILE | SMPD_DBG_STATE_PREPEND_RANK | SMPD_DBG_STATE_TRACE)
#define SMPD_DBG_FILE_SIZE               (4*1024*1024)

#define SMPD_QUOTE_CHAR                   '\"'
#define SMPD_DELIM_CHAR                   '='
#define SMPD_DELIM_STR                    "="
#define SMPD_ESCAPE_CHAR                  '\\'
#define SMPD_HIDE_CHAR                    '*'
#define SMPD_SEPAR_CHAR                   ' '

#define DBS_SUCCESS_STR	                  "DBS_SUCCESS"
#define DBS_FAIL_STR	                  "DBS_FAIL"
#define DBS_END_STR	                  "DBS_END"

#define PMI_KVS_ID_KEY                    "PMI_KVS_ID"

#define SMPD_CONSOLE_STR_LENGTH         10*SMPD_MAX_CMD_LENGTH

#define SMPD_DEFAULT_TIMEOUT              120
#define SMPD_SHORT_TIMEOUT                 60
#define SMPD_DEFAULT_PRIORITY_CLASS         2
#define SMPD_DEFAULT_PRIORITY               3

#define SMPD_SINGLETON_MAX_KVS_NAME_LEN     100
#define SMPD_SINGLETON_MAX_HOST_NAME_LEN    100

#define SMPD_UNREFERENCED_ARG(a) a

#ifdef HAVE_WINDOWS_H
#define snprintf _snprintf
#define vsnprintf _vsnprintf
#define smpd_get_last_error GetLastError
#else
#define smpd_get_last_error() errno
#endif

/* FIXME: Do something sensible like mpitypedefs.h */
#define SMPDU_Size_t    int

#define SMPDU_Assert(a_)    {                                       \
    if(!(a_)){                                                      \
        smpd_err_printf("Assertion failed in file %s at line %d\n", \
            __FILE__, __LINE__);                                    \
        exit(-1);                                                   \
    }                                                               \
}

/* Some TCHAR utils for making our life easier */
#ifdef HAVE_WINDOWS_H
#ifdef _UNICODE
    #define SMPDU_TCSTOMBS  wcstombs
    #define SMPDU_MBSTOTCS  mbstowcs
#else
    #define SMPDU_TCSTOMBS  _tcsncpy
    #define SMPDU_MBSTOTCS  _tcsncpy
#endif
#endif

#ifdef HAVE_WINDOWS_H
#   define SMPDU_UNREFERENCED_ARG(a_)   a_
#else
#   define SMPDU_UNREFERENCED_ARG(a_)
#endif

#define SMPD_ERR_SETPRINTANDJUMP(msg, errcode) {smpd_err_printf("%s", msg); retval = errcode; goto fn_fail; }
#define SMPD_MAX_ERR_MSG_LENGTH 100

typedef enum smpd_state_t
{
    SMPD_IDLE,
    SMPD_EXITING,
    SMPD_RESTARTING,
    SMPD_DONE,
    SMPD_CLOSING,
    SMPD_SMPD_LISTENING,
    SMPD_MGR_LISTENING,
    SMPD_PMI_LISTENING,
    SMPD_PMI_SERVER_LISTENING,
    SMPD_SINGLETON_CLIENT_LISTENING,
    SMPD_SINGLETON_MPIEXEC_CONNECTING,
    SMPD_SINGLETON_READING_PMI_INFO,
    SMPD_SINGLETON_WRITING_PMI_INFO,
    SMPD_SINGLETON_DONE,
    SMPD_MPIEXEC_CONNECTING_TREE,
    SMPD_MPIEXEC_CONNECTING_SMPD,
    SMPD_CONNECTING_RPMI,
    SMPD_CONNECTING_PMI,
    SMPD_CONNECTING,
    SMPD_RECONNECTING,
    SMPD_READING_CHALLENGE_STRING,
    SMPD_WRITING_CHALLENGE_STRING,
    SMPD_READING_CHALLENGE_RESPONSE,
    SMPD_WRITING_CHALLENGE_RESPONSE,
    SMPD_READING_CONNECT_RESULT,
    SMPD_WRITING_CONNECT_RESULT,
    SMPD_READING_STDIN,
    SMPD_WRITING_DATA_TO_STDIN,
    SMPD_READING_STDOUT,
    SMPD_READING_STDERR,
    SMPD_READING_CMD_HEADER,
    SMPD_READING_CMD,
    SMPD_WRITING_CMD,
    SMPD_READING_SESSION_REQUEST,
    SMPD_WRITING_SMPD_SESSION_REQUEST,
    SMPD_WRITING_PROCESS_SESSION_REQUEST,
    SMPD_WRITING_PMI_SESSION_REQUEST,
    SMPD_READING_PWD_REQUEST,
    SMPD_WRITING_PWD_REQUEST,
    SMPD_WRITING_NO_PWD_REQUEST,
    SMPD_WRITING_SSPI_REQUEST,
    SMPD_READING_SMPD_PASSWORD,
    SMPD_WRITING_SMPD_PASSWORD,
    SMPD_READING_CRED_REQUEST,
    SMPD_WRITING_CRED_REQUEST,
    SMPD_WRITING_NO_CRED_REQUEST,
    SMPD_READING_CRED_ACK,
    SMPD_WRITING_CRED_ACK_YES,
    SMPD_WRITING_CRED_ACK_NO,
    SMPD_READING_ACCOUNT,
    SMPD_WRITING_ACCOUNT,
    SMPD_READING_PASSWORD,
    SMPD_WRITING_PASSWORD,
    SMPD_READING_RECONNECT_REQUEST,
    SMPD_WRITING_RECONNECT_REQUEST,
    SMPD_WRITING_NO_RECONNECT_REQUEST,
    SMPD_READING_SESSION_HEADER,
    SMPD_WRITING_SESSION_HEADER,
    SMPD_READING_SMPD_RESULT,
    SMPD_WRITING_SESSION_ACCEPT,
    SMPD_WRITING_SESSION_REJECT,
    SMPD_READING_PROCESS_RESULT,
    SMPD_WRITING_PROCESS_SESSION_ACCEPT,
    SMPD_WRITING_PROCESS_SESSION_REJECT,
    SMPD_READING_TIMEOUT,
    SMPD_READING_MPIEXEC_ABORT,
    SMPD_READING_PMI_ID,
    SMPD_WRITING_PMI_ID,
    SMPD_READING_SSPI_HEADER,
    SMPD_READING_SSPI_BUFFER,
    SMPD_WRITING_SSPI_HEADER,
    SMPD_WRITING_SSPI_BUFFER,
    SMPD_WRITING_DELEGATE_REQUEST,
    SMPD_READING_DELEGATE_REQUEST_RESULT,
    SMPD_WRITING_DELEGATE_REQUEST_RESULT,
    SMPD_WRITING_IMPERSONATE_RESULT,
    SMPD_READING_IMPERSONATE_RESULT,
    SMPD_WRITING_CRED_ACK_SSPI,
    SMPD_WRITING_CRED_ACK_SSPI_JOB_KEY,
    SMPD_WRITING_SSPI_JOB_KEY,
    SMPD_READING_SSPI_JOB_KEY,
    SMPD_READING_CLIENT_SSPI_HEADER,
    SMPD_READING_CLIENT_SSPI_BUFFER,
    SMPD_WRITING_CLIENT_SSPI_HEADER,
    SMPD_WRITING_CLIENT_SSPI_BUFFER,
    SMPD_END_MARKER_NUM_STATES
} smpd_state_t;

typedef enum smpd_context_type_t
{
    SMPD_CONTEXT_INVALID,
    SMPD_CONTEXT_STDIN,
    SMPD_CONTEXT_MPIEXEC_STDIN,
    SMPD_CONTEXT_MPIEXEC_STDIN_RSH,
    SMPD_CONTEXT_STDOUT,
    SMPD_CONTEXT_STDOUT_RSH,
    SMPD_CONTEXT_STDERR,
    SMPD_CONTEXT_STDERR_RSH,
    SMPD_CONTEXT_PARENT,
    SMPD_CONTEXT_LEFT_CHILD,
    SMPD_CONTEXT_RIGHT_CHILD,
    SMPD_CONTEXT_CHILD,
    SMPD_CONTEXT_LISTENER,
    SMPD_CONTEXT_PMI_LISTENER,
    SMPD_CONTEXT_SMPD,
    SMPD_CONTEXT_PMI,
    SMPD_CONTEXT_TIMEOUT,
    SMPD_CONTEXT_MPIEXEC_ABORT,
    SMPD_CONTEXT_SINGLETON_INIT_CLIENT,
    SMPD_CONTEXT_SINGLETON_INIT_MPIEXEC,
    SMPD_CONTEXT_UNDETERMINED,
    SMPD_CONTEXT_FREED
} smpd_context_type_t;

typedef enum smpd_context_target_t
{
    SMPD_TARGET_UNDETERMINED,
    SMPD_TARGET_SMPD,
    SMPD_TARGET_PROCESS,
    SMPD_TARGET_PMI
} smpd_context_target_t;

typedef enum smpd_command_state_t
{
    SMPD_CMD_INVALID,
    SMPD_CMD_READING_HDR,
    SMPD_CMD_READING_CMD,
    SMPD_CMD_WRITING_CMD,
    SMPD_CMD_READY,
    SMPD_CMD_HANDLED
} smpd_command_state_t;

typedef struct smpd_command_t
{
    smpd_command_state_t state;
    char cmd_hdr_str[SMPD_CMD_HDR_LENGTH];
    char cmd_str[SMPD_MAX_CMD_STR_LENGTH];
    char cmd[SMPD_MAX_CMD_LENGTH];
    SMPD_IOV iov[2];
    int length;
    int src, dest, tag;
    int wait;
    int stdin_read_offset;
    struct smpd_context_t *context;
    struct smpd_command_t *next;
    int freed; /* debugging to see if freed more than once */
} smpd_command_t;

#ifdef HAVE_WINDOWS_H
typedef struct smpd_pwait_t
{
    HANDLE hProcess, hThread;
} smpd_pwait_t;
#else
typedef pid_t smpd_pwait_t;
#endif

typedef struct smpd_host_node_t
{
    int id, parent;
    char host[SMPD_MAX_HOST_LENGTH];
    char alt_host[SMPD_MAX_HOST_LENGTH];
    int nproc;
    SMPD_BOOL connected;
    int connect_cmd_tag;
    struct smpd_host_node_t *next;
    struct smpd_host_node_t *left;
    struct smpd_host_node_t *right;
} smpd_host_node_t;

struct smpd_spawn_context_t;

typedef struct smpd_sspi_client_context_t
{
    int id;
#ifdef HAVE_WINDOWS_H
    CtxtHandle context;
    CredHandle credential;
    TimeStamp expiration_time;
    HANDLE user_handle;
    HANDLE job;
    DWORD flags;
    SMPD_BOOL close_handle;
#endif
    char target[SMPD_MAX_NAME_LENGTH];
    void *buffer;
    int buffer_length;
    void *out_buffer;
    int out_buffer_length;
    int max_buffer_size;
    struct smpd_sspi_client_context_t *next;
} smpd_sspi_client_context_t;

typedef enum smpd_sspi_type_t
{
    SMPD_SSPI_IDENTIFY,
    SMPD_SSPI_IMPERSONATE,
    SMPD_SSPI_DELEGATE
} smpd_sspi_type_t;

typedef enum smpd_access_t
{
    SMPD_ACCESS_NONE,
    SMPD_ACCESS_ADMIN,
    SMPD_ACCESS_USER,
    SMPD_ACCESS_USER_PROCESS
} smpd_access_t;

typedef struct smpd_context_t
{
    smpd_context_type_t type;
    smpd_context_target_t target;
    smpd_access_t access;
    char host[SMPD_MAX_HOST_LENGTH];
    int id, rank;
    smpd_pwait_t wait;
    SMPDU_Sock_set_t set;
    SMPDU_Sock_t sock;
    smpd_state_t state;
    smpd_state_t read_state;
    smpd_command_t read_cmd;
    smpd_state_t write_state;
    smpd_command_t *write_list;
    smpd_command_t *wait_list;
    smpd_host_node_t *connect_to;
    struct smpd_spawn_context_t *spawn_context;
    char pszCrypt[SMPD_AUTHENTICATION_STR_LEN];
    char pszChallengeResponse[SMPD_AUTHENTICATION_STR_LEN];
    char port_str[SMPD_MAX_PORT_STR_LENGTH];
    char session[SMPD_SESSION_REQUEST_LEN];
    char pwd_request[SMPD_MAX_PWD_REQUEST_LENGTH];
    char cred_request[SMPD_MAX_CRED_REQUEST_LENGTH];
    char account[SMPD_MAX_ACCOUNT_LENGTH];
    char domain[SMPD_MAX_ACCOUNT_LENGTH];
    char full_domain[SMPD_MAX_ACCOUNT_LENGTH];
    char password[SMPD_MAX_PASSWORD_LENGTH];
    char encrypted_password[SMPD_MAX_PASSWORD_LENGTH];
    char smpd_pwd[SMPD_MAX_PASSWORD_LENGTH];
    char session_header[SMPD_MAX_SESSION_HEADER_LENGTH];
    /* FIXME: Remove this */
    char singleton_init_kvsname[SMPD_SINGLETON_MAX_KVS_NAME_LEN];
    char singleton_init_domainname[SMPD_SINGLETON_MAX_KVS_NAME_LEN];
    char singleton_init_hostname[SMPD_SINGLETON_MAX_HOST_NAME_LEN];
    int singleton_init_pm_port;
    /* FIXME: Remove this */
    int connect_return_id, connect_return_tag;
    struct smpd_process_t *process;
    char sspi_header[SMPD_SSPI_HEADER_LENGTH];
    char sspi_job_key[SMPD_SSPI_JOB_KEY_LENGTH];
    smpd_sspi_client_context_t *sspi_context;
    smpd_sspi_type_t sspi_type;
    SMPD_BOOL first_output_stdout, first_output_stderr;
    struct smpd_context_t *next;
} smpd_context_t;

typedef struct smpd_stdin_write_node_t
{
    char *buffer;
    int length;
    struct smpd_stdin_write_node_t *next;
} smpd_stdin_write_node_t;

typedef struct smpd_map_drive_node_t
{
    int ref_count;
    char drive;
    char share[SMPD_MAX_EXE_LENGTH];
    struct smpd_map_drive_node_t *next;
} smpd_map_drive_node_t;

typedef struct smpd_process_t
{
    int id;
    int num_valid_contexts;
    smpd_context_t *in, *out, *err, *pmi, *p_singleton_context;
    int context_refcount;
    int pid;
    char exe[SMPD_MAX_EXE_LENGTH];
    char env[SMPD_MAX_ENV_LENGTH];
    char dir[SMPD_MAX_DIR_LENGTH];
    char path[SMPD_MAX_PATH_LENGTH];
    char clique[SMPD_MAX_CLIQUE_LENGTH];
    int rank;
    int binding_proc;
    int nproc;
    smpd_pwait_t wait;
    int exitcode;
    char kvs_name[SMPD_MAX_DBS_NAME_LEN];
    char domain_name[SMPD_MAX_DBS_NAME_LEN];
    char err_msg[SMPD_MAX_ERROR_LEN];
    smpd_stdin_write_node_t *stdin_write_list;
    int spawned;
    SMPD_BOOL local_process;
    SMPD_BOOL is_singleton_client;
    smpd_map_drive_node_t *map_list;
    int appnum;
    struct smpd_process_t *next;
} smpd_process_t;

typedef struct smpd_launch_node_t
{
    char exe[SMPD_MAX_EXE_LENGTH];
    char args[SMPD_MAX_EXE_LENGTH];
    char *env, env_data[SMPD_MAX_ENV_LENGTH];
    char clique[SMPD_MAX_CLIQUE_LENGTH];
    char dir[SMPD_MAX_DIR_LENGTH];
    char path[SMPD_MAX_PATH_LENGTH];
    smpd_map_drive_node_t *map_list;
    int host_id;
    char hostname[SMPD_MAX_HOST_LENGTH];
    char alt_hostname[SMPD_MAX_HOST_LENGTH];
    int iproc;
    int binding_proc;
    int nproc;
    int appnum;
    int priority_class, priority_thread;
    struct smpd_launch_node_t *next, *prev;
} smpd_launch_node_t;

typedef struct smpd_spawn_context_t
{
    smpd_context_t *context;
    smpd_launch_node_t *launch_list;
    smpd_command_t *result_cmd;
    char kvs_name[SMPD_MAX_DBS_NAME_LEN];
    char domain_name[SMPD_MAX_DBS_NAME_LEN];
    int npreput;
    char preput[SMPD_MAX_CMD_LENGTH];
    int num_outstanding_launch_cmds;
} smpd_spawn_context_t;

typedef struct smpd_delayed_spawn_node_t
{
    smpd_context_t *context;
    smpd_command_t cmd;
    struct smpd_delayed_spawn_node_t *next;
} smpd_delayed_spawn_node_t;

typedef struct smpd_env_node_t
{
    char name[SMPD_MAX_NAME_LENGTH];
    char value[SMPD_MAX_VALUE_LENGTH];
    struct smpd_env_node_t *next;
} smpd_env_node_t;

typedef struct smpd_database_element_t
{
    char pszKey[SMPD_MAX_DBS_KEY_LEN];
    char pszValue[SMPD_MAX_DBS_VALUE_LEN];
    struct smpd_database_element_t *pNext;
} smpd_database_element_t;

typedef struct smpd_database_node_t
{
    char pszName[SMPD_MAX_DBS_NAME_LEN];
    smpd_database_element_t *pData, *pIter;
    struct smpd_database_node_t *pNext;
} smpd_database_node_t;

typedef smpd_database_element_t * smpd_dbsIter_t;

typedef struct smpd_barrier_in_t
{
    smpd_context_t *context;
    int dest;
    int cmd_tag;
    char ctx_key[100];
} smpd_barrier_in_t;

typedef struct smpd_barrier_node_t
{
    char name[SMPD_MAX_DBS_NAME_LEN];
    int count;
    int in;
    smpd_barrier_in_t *in_array;
    struct smpd_barrier_node_t *next;
} smpd_barrier_node_t;

typedef struct smpd_data_t
{
    char name[SMPD_MAX_NAME_LENGTH];
    char value[SMPD_MAX_VALUE_LENGTH];
    struct smpd_data_t *next;
} smpd_data_t;

typedef struct smpd_exit_process_t
{
    SMPD_BOOL init_called, finalize_called, suspended, exited;
    int node_id, exitcode;
    char host[SMPD_MAX_HOST_LENGTH];
    char ctx_key[100];
    char *errmsg;
    smpd_command_t *suspend_cmd;
} smpd_exit_process_t;

typedef struct smpd_process_group_t
{
    SMPD_BOOL aborted, any_noinit_process_exited, any_init_received;
    char kvs[SMPD_MAX_DBS_NAME_LEN];
    int num_procs, num_pending_suspends;
    smpd_exit_process_t *processes;
    struct smpd_process_group_t *next;
} smpd_process_group_t;

typedef enum smpd_builtin_commands_t
{
    SMPD_CMD_NONE,
    SMPD_CMD_SHUTDOWN,
    SMPD_CMD_RESTART,
    SMPD_CMD_VALIDATE,
    SMPD_CMD_DO_STATUS,
    SMPD_CMD_SET,
    SMPD_CMD_GET,
    SMPD_CMD_DELETE,
    SMPD_CMD_ADD_JOB,
    SMPD_CMD_ADD_JOB_AND_PASSWORD,
    SMPD_CMD_REMOVE_JOB,
    SMPD_CMD_ASSOCIATE_JOB,
    SMPD_CMD_VERSION
} smpd_builtin_commands_t;

/* If you add elements to the process structure you must add the appropriate
   initializer in smpd_connect.c where the global variable, smpd_process, lives */
typedef struct smpd_global_t
{
    smpd_state_t state;
    int  id, parent_id;
    int  level;
    smpd_context_t *left_context, *right_context, *parent_context, *context_list;
    smpd_context_t *listener_context;
    smpd_process_t *process_list;
    int  closing;
    int  root_smpd;
    SMPDU_Sock_set_t set;
    char host[SMPD_MAX_HOST_LENGTH];
    char pszExe[SMPD_MAX_EXE_LENGTH];
    int  bService;
    int  bNoTTY;
    SMPD_BOOL bPasswordProtect;
    char SMPDPassword[100];
    char passphrase[SMPD_PASSPHRASE_MAX_LENGTH];
    SMPD_BOOL logon;
    char UserAccount[SMPD_MAX_ACCOUNT_LENGTH];
    char UserPassword[SMPD_MAX_PASSWORD_LENGTH];
    int  cur_tag;
    int  dbg_state;
    char dbg_filename[SMPD_MAX_FILENAME];
    long dbg_file_size;
    int  have_dbs;
    char kvs_name[SMPD_MAX_DBS_NAME_LEN];
    char domain_name[SMPD_MAX_DBS_NAME_LEN];
#ifdef HAVE_WINDOWS_H
    HANDLE hCloseStdinThreadEvent;
    HANDLE hStdinThread;
#endif
#ifdef USE_PTHREAD_STDIN_REDIRECTION
    pthread_t stdin_thread;
    int stdin_read, stdin_write;
#endif
    int do_console;
    int port;
    SMPD_BOOL is_singleton_client;
    /* Port to connect back to a singleton process */
    int singleton_client_port;
    char console_host[SMPD_MAX_HOST_LENGTH];
    smpd_host_node_t *host_list;
    smpd_launch_node_t *launch_list;
    int credentials_prompt;
    int do_multi_color_output;
    int no_mpi;
    int output_exit_codes;
    int local_root;
    int use_iproot;
    int use_process_session;
    int nproc, nproc_launched, nproc_exited;
    SMPD_BOOL verbose;
#ifdef HAVE_WINDOWS_H
    /* set_affinity is TRUE if the user explicitly specifies MPI process binding */
    SMPD_BOOL set_affinity;
    /* The affinity map contains the user defined MPI process affinity map, if any */
    int *affinity_map;
    int affinity_map_sz;
#endif
    /*SMPD_BOOL shutdown, restart, validate, do_status;*/ /* built in commands */
    smpd_builtin_commands_t builtin_cmd;
#ifdef HAVE_WINDOWS_H
    BOOL bOutputInitialized;
    HANDLE hOutputMutex;
    HANDLE hLaunchProcessMutex;
#endif
#ifdef USE_WIN_MUTEX_PROTECT
    HANDLE hDBSMutex;
#endif
    smpd_database_node_t *pDatabase;
    smpd_database_node_t *pDatabaseIter;
    int nInitDBSRefCount;
    smpd_barrier_node_t *barrier_list;
#ifdef HAVE_WINDOWS_H
    SERVICE_STATUS ssStatus;
    SERVICE_STATUS_HANDLE sshStatusHandle;
    HANDLE hBombDiffuseEvent;
    HANDLE hBombThread;
#endif
    SMPD_BOOL service_stop;
    SMPD_BOOL noprompt;
    char smpd_filename[SMPD_MAX_FILENAME];
    SMPD_BOOL stdin_toall, stdin_redirecting;
    smpd_host_node_t *default_host_list, *cur_default_host;
    int cur_default_iproc;
#ifdef HAVE_WINDOWS_H
    HANDLE hSMPDDataMutex;
#endif
    char printf_buffer[SMPD_MAX_DBG_PRINTF_LENGTH];
    int state_machine_ret_val;
    SMPD_BOOL exit_on_done;
    int tree_parent;
    int tree_id;
    smpd_host_node_t *s_host_list, *s_cur_host;
    int s_cur_count;
    SMPD_BOOL use_inherited_handles;
    smpd_process_group_t *pg_list;
    SMPD_BOOL use_abort_exit_code;
    int abort_exit_code;
    SMPD_BOOL verbose_abort_output;
    int mpiexec_exit_code;
    SMPD_BOOL map0to1;
    SMPD_BOOL rsh_mpiexec;
    SMPD_BOOL mpiexec_inorder_launch;
    SMPD_BOOL mpiexec_run_local;
#ifdef HAVE_WINDOWS_H
    HANDLE timeout_thread;
#else
#ifdef HAVE_PTHREADS_H
    pthread_t timeout_thread;
#endif
#endif
    int timeout;
    SMPDU_Sock_t timeout_sock;
    SMPDU_Sock_t mpiexec_abort_sock;
    SMPD_BOOL use_pmi_server;
    char *mpiexec_argv0;
    char encrypt_prefix[SMPD_MAX_PASSWORD_LENGTH];
    SMPD_BOOL plaintext;
    SMPD_BOOL use_sspi, use_delegation, use_sspi_job_key;
    SMPD_BOOL use_ms_hpc;
#ifdef HAVE_WINDOWS_H
    PSecurityFunctionTable sec_fn;
#endif
    smpd_sspi_client_context_t *sspi_context_list;
    char job_key[SMPD_SSPI_JOB_KEY_LENGTH];
    char job_key_account[SMPD_MAX_ACCOUNT_LENGTH];
    char job_key_password[SMPD_MAX_PASSWORD_LENGTH];
    char key[SMPD_MAX_NAME_LENGTH];
    char val[SMPD_MAX_VALUE_LENGTH];
    SMPD_BOOL do_console_returns;
    char env_channel[10];
    char env_netmod[10];
    char env_dll[SMPD_MAX_FILENAME];
    char env_wrap_dll[SMPD_MAX_FILENAME];
    smpd_delayed_spawn_node_t *delayed_spawn_queue;
    SMPD_BOOL spawning;
    int user_index;
    SMPD_BOOL prefix_output;
} smpd_global_t;

/* FIXME: Cleanup this struct after we start using spn list
 * handles everywhere
 */
typedef struct smpd_host_spn_node_t
{
    char host[SMPD_MAX_NAME_LENGTH];
    char dnshost[SMPD_MAX_NAME_LENGTH];
    char fq_service_name[SMPD_MAX_FQ_NAME_LENGTH];
    char spn[SMPD_MAX_FQ_NAME_LENGTH];
    struct smpd_host_spn_node_t *next;
} smpd_host_spn_node_t;
typedef smpd_host_spn_node_t **smpd_spn_list_hnd_t;

#define SMPD_SPN_LIST_HND_IS_INIT(hnd) (((hnd) != NULL) && (*(hnd) != NULL))

extern smpd_global_t smpd_process;

/* MS HPC job utils are smpd utils & hence needs the smpd structures 
 * to be defined prior to including them.
 */
#ifdef HAVE_WINDOWS_H
#include "smpd_hpc_js_exports.h"
#endif

/* function prototypes */
#if defined(__cplusplus)
extern "C" {
#endif

int smpd_parse_command_args(int *argcp, char **argvp[]);
#ifdef HAVE_WINDOWS_H
char *smpd_encode_handle(char *str, HANDLE h);
HANDLE smpd_decode_handle(char *str);
#endif
void smpd_print_options(void);
int smpd_entry_point(void);
int smpd_enter_at_state(SMPDU_Sock_set_t set, smpd_state_t state);
int smpd_wait_process(smpd_pwait_t wait, int *exit_code_ptr);
int smpd_init_process(void);
int smpd_init_printf(void);
int smpd_finalize_printf(void);
int smpd_init_context(smpd_context_t *context, smpd_context_type_t type, SMPDU_Sock_set_t set, SMPDU_Sock_t sock, int id);
int smpd_init_command(smpd_command_t *cmd);
int smpd_create_context(smpd_context_type_t type, SMPDU_Sock_set_t set, SMPDU_Sock_t sock, int id, smpd_context_t **context_pptr);
int smpd_create_command(char *cmd_str, int src, int dest, int want_reply, smpd_command_t **cmd_pptr);
int smpd_create_command_copy(smpd_command_t *src_ptr, smpd_command_t **cmd_pptr);
int smpd_free_command(smpd_command_t *cmd_ptr);
int smpd_free_context(smpd_context_t *context);
int smpd_add_command_arg(smpd_command_t *cmd_ptr, char *param, char *value);
int smpd_add_command_int_arg(smpd_command_t *cmd_ptr, char *param, int value);
int smpd_add_command_binary_arg(smpd_command_t *cmd_ptr, char *param, void *buffer, int length);
int smpd_parse_command(smpd_command_t *cmd_ptr);
int smpd_post_read_command(smpd_context_t *context);
int smpd_post_write_command(smpd_context_t *context, smpd_command_t *cmd);
int smpd_package_command(smpd_command_t *cmd);
int smpd_read_string(SMPDU_Sock_t sock, char *str, int maxlen);
int smpd_write_string(SMPDU_Sock_t sock, char *str);
int smpd_read(SMPDU_Sock_t sock, void *buf, SMPDU_Sock_size_t len);
int smpd_write(SMPDU_Sock_t sock, void *buf, SMPDU_Sock_size_t len);
int smpd_dbg_printf(char *str, ...);
int smpd_err_printf(char *str, ...);

#ifdef HAVE_WINDOWS_H
    typedef int (*smpd_printf_fp_t) (char *str, ...);
    int smpd_tprintf_templ(smpd_printf_fp_t fp, TCHAR *str, ...);
    #define smpd_err_tprintf(str, ...) smpd_tprintf_templ(smpd_err_printf, str, __VA_ARGS__);
    #define smpd_dbg_tprintf(str, ...) smpd_tprintf_templ(smpd_dbg_printf, str, __VA_ARGS__);
#endif

int smpd_enter_fn(char *fcname);
int smpd_exit_fn(char *fcname);
SMPD_BOOL smpd_option_on(const char *option);
int smpd_get_user_data(const char *key, char *value, int value_len);
int smpd_get_smpd_data(const char *key, char *value, int value_len);
int smpd_get_user_data_default(const char *key, char *value, int value_len);
int smpd_get_smpd_data_default(const char *key, char *value, int value_len);
int smpd_set_user_data(const char *key, const char *value);
int smpd_set_smpd_data(const char *key, const char *value);
int smpd_delete_user_data(const char *key);
int smpd_delete_smpd_data(const char *key);
int smpd_getpid(void);
char * get_sock_error_string(int error);
void smpd_get_password(char *password);
void smpd_get_account_and_password(char *account, char *password);
int smpd_get_credentials_from_parent(SMPDU_Sock_set_t set, SMPDU_Sock_t sock);
int smpd_get_smpd_password_from_parent(SMPDU_Sock_set_t set, SMPDU_Sock_t sock);
int smpd_get_opt(int *argc, char ***argv, char * flag);
int smpd_get_opt_int(int *argc, char ***argv, char * flag, int *n);
int smpd_get_opt_long(int *argc, char ***argv, char * flag, long *n);
int smpd_get_opt_double(int *argc, char ***argv, char * flag, double *d);
int smpd_get_opt_string(int *argc, char ***argv, char * flag, char *str, int len);
int smpd_get_opt_two_strings(int *argc, char ***argv, char * flag, char *str1, int len1, char *str2, int len2);
int smpd_get_win_opt_string(int *argc, char ***argv, char * flag, char *str, int len);
#ifdef HAVE_WINDOWS_H
void smpd_parse_account_domain(const char *domain_account, char *account, char *domain);
int smpd_get_user_handle(char *account, char *domain, char *password, HANDLE *handle_ptr);
int smpd_get_user_name(char *account, char *domain, char *full_domain);
int smpd_make_socket_loop(SOCKET *pRead, SOCKET *pWrite);
int smpd_make_socket_loop_choose(SOCKET *pRead, int read_overlapped, SOCKET *pWrite, int write_overlapped);
#endif
int smpd_generate_session_header(char *str, int session_id);
int smpd_interpret_session_header(char *str);
int smpd_command_destination(int dest, smpd_context_t **dest_context);
int smpd_forward_command(smpd_context_t *src, smpd_context_t *dest);
int smpd_launch_process(smpd_process_t *process, int priorityClass, int priority, int dbg, SMPDU_Sock_set_t set);
int smpd_encode_buffer(char *dest, int dest_length, const char *src, int src_length, int *num_encoded);
int smpd_decode_buffer(const char *str, char *dest, int length, int *num_decoded);
int smpd_create_process_struct(int rank, smpd_process_t **process_ptr);
int smpd_free_process_struct(smpd_process_t *process);
char * smpd_get_context_str(smpd_context_t *context);
int smpd_gen_authentication_strings(char *phrase, char *append, char *crypted);
#ifdef HAVE_WINDOWS_H
int smpd_start_win_mgr(smpd_context_t *context, SMPD_BOOL use_context_user_handle);
#else
int smpd_start_unx_mgr(smpd_context_t *context);
#endif
#ifdef HAVE_WINDOWS_H
void smpd_stdin_thread(SOCKET hWrite);
#endif
#ifdef USE_PTHREAD_STDIN_REDIRECTION
void *smpd_pthread_stdin_thread(void *p);
int smpd_cancel_stdin_thread(void);
#endif
int smpd_handle_command(smpd_context_t *context);
int smpd_create_command_from_stdin(char *str, smpd_command_t **cmd_pptr);
int smpd_handle_barrier_command(smpd_context_t *context);
int smpd_post_abort_command(char *fmt, ...);
int smpd_kill_all_processes(void);
int smpd_exit(int exitcode);
#ifdef HAVE_WINDOWS_H
void smpd_translate_win_error(int error, char *msg, int maxlen, char *prepend, ...);
#else
typedef void smpd_sig_fn_t( int );
smpd_sig_fn_t *smpd_signal( int signo, smpd_sig_fn_t func );
#endif
SMPD_BOOL smpd_get_full_path_name(const char *exe, int maxlen, char *path, char **namepart);
SMPD_BOOL smpd_search_path(const char *path, const char *exe, int maxlen, char *str);
#ifdef HAVE_WINDOWS_H
int smpd_process_from_registry(smpd_process_t *process);
int smpd_process_to_registry(smpd_process_t *process, char *actual_exe);
int smpd_clear_process_registry(void);
int smpd_validate_process_registry(void);
SMPD_BOOL smpd_read_password_from_registry(int index, char *szAccount, char *szPassword);
SMPD_BOOL smpd_save_password_to_registry(int index, const char *szAccount, const char *szPassword, SMPD_BOOL persistent);
SMPD_BOOL smpd_save_cred_to_file(const char *filename, const char *szAccount, const char *szPassword);
SMPD_BOOL smpd_read_cred_from_file(const char *filename, char *szAccount, int acc_len, char *szPassword, int pass_len);
SMPD_BOOL smpd_delete_current_password_registry_entry(int index);
int smpd_cache_password(const char *account, const char *password);
SMPD_BOOL smpd_get_cached_password(char *account, char *password);
int smpd_delete_cached_password(void);
#endif
int smpd_do_console(void);
int smpd_restart(void);
SMPD_BOOL smpd_snprintf_update(char **str_pptr, int *len_ptr, char *str_format, ...);
const char * smpd_get_state_string(smpd_state_t state);
const char * smpd_get_cmd_state_string(smpd_command_state_t state);
SMPD_BOOL smpd_command_to_string(char **str_pptr, int *len_ptr, int indent, smpd_command_t *cmd_ptr);
SMPD_BOOL smpd_process_to_string(char **str_pptr, int *len_ptr, int indent, smpd_process_t *process);
SMPD_BOOL smpd_is_affirmative(const char *str);
int smpd_hide_string_arg(char *str, const char *flag);
int smpd_get_default_hosts(void);
int smpd_lock_smpd_data(void);
int smpd_unlock_smpd_data(void);
int smpd_insert_into_dynamic_hosts(void);
int smpd_remove_from_dynamic_hosts(void);
int smpd_get_pwd_from_file(char *file_name);
int smpd_get_next_hostname(char *host, char *alt_host);
SMPD_BOOL smpd_parse_machine_file(char *file_name);
int smpd_parse_hosts_string(const char *host_str);
int smpd_free_host_list(void );
int smpd_get_host_id(char *host, int *id_ptr);
int smpd_get_next_host(smpd_host_node_t **host_node_pptr, smpd_launch_node_t *launch_node);
SMPD_BOOL smpd_get_argcv_from_file(FILE *fin, int *argcp, char ***argvp);
int smpd_create_cliques(smpd_launch_node_t *list);
int smpd_handle_spawn_command(smpd_context_t *context);
int smpd_handle_abort_job_command(smpd_context_t *context);
int smpd_abort_job(char *name, int rank, char *fmt, ...);
int smpd_suspend_process(smpd_process_t *process);
int smpd_kill_process(smpd_process_t *process, int exit_code);
int smpd_handle_suspend_result(smpd_command_t *cmd, char *result_str);
int smpd_watch_processes(void);
int smpd_get_hostname(char *host, int length);
int PMIX_Start_root_smpd(int nproc, char *host, int len, int *port);
int PMIX_Stop_root_smpd(void);
int smpd_hash(char *input, int input_length, char *output, int output_length);
int smpd_encrypt_data(char *input, int input_length, char *output, int output_length);
int smpd_decrypt_data(char *input, int input_length, char *output, int *output_length);
int smpd_get_all_smpd_data(smpd_data_t **data);
int smpd_create_sspi_client_context(smpd_sspi_client_context_t **new_context);
int smpd_free_sspi_client_context(smpd_sspi_client_context_t **context);
int smpd_sspi_context_init(smpd_sspi_client_context_t **sspi_context_pptr, const char *host, int port, smpd_sspi_type_t type);
int smpd_sspi_context_iter(int sspi_id, void **sspi_buffer_pptr, int *length_ptr);
#ifdef HAVE_WINDOWS_H
int smpd_setup_scps_with_file(char *filename);
int smpd_setup_scp(TCHAR *hostname);
int smpd_remove_scp(TCHAR *hostname, smpd_spn_list_hnd_t hnd);
int smpd_remove_scps_with_file(char *filename, smpd_spn_list_hnd_t hnd);
#endif
int smpd_register_spn(const char *dc, const char *dn, const char *dh);
int smpd_lookup_spn(char *target, int length, const char * host, int port);
int smpd_lookup_spn_list(smpd_spn_list_hnd_t hnd, char *target, int length, const char * host, int port);
int smpd_spn_list_finalize(smpd_spn_list_hnd_t *spn_list_hnd);
int smpd_spn_list_init(smpd_spn_list_hnd_t *spn_list_hnd);
int smpd_spn_list_dbg_print(smpd_spn_list_hnd_t hnd);
SMPD_BOOL smpd_map_user_drives(char *pszMap, char *pszAccount, char *pszPassword, char *pszError, int maxerrlength);
SMPD_BOOL smpd_unmap_user_drives(char *pszMap);
void smpd_finalize_drive_maps(void);
int smpd_append_env_option(char *str, int maxlen, const char *env_name, const char *env_val);
#ifdef HAVE_WINDOWS_H
int smpd_get_network_drives(char *pMapList, int mapListLen);
int smpd_add_job_key(const char *key, const char *username, const char *domain, const char *full_domain);
int smpd_add_job_key_and_handle(const char *key, const char *username, const char *domain, const char *full_domain, HANDLE hUser);
int smpd_remove_job_key(const char *key);
int smpd_associate_job_key(const char *key, const char *username, const char *domain, const char *full_domain, HANDLE user_handle);
int smpd_lookup_job_key(const char *key, const char *username, HANDLE *user_handle, HANDLE *job_handle);
#endif
SMPD_BOOL smpd_verify_version(const char *challenge);
void smpd_fix_up_host_tree(smpd_host_node_t *node);
SMPD_BOOL smpd_isnumbers_with_colon(const char *str);
int smpd_add_host_to_default_list(const char *hostname);
int smpd_get_ccp_nodes(int *np, smpd_host_node_t **host_node_ptr_p);
int smpd_add_extended_host_to_default_list(const char *hostname, const char *alt_hostname, const int num_cpus);
int smpd_parse_map_string(const char *str, smpd_map_drive_node_t **list);
int smpd_delayed_spawn_enqueue(smpd_context_t *context);
int smpd_delayed_spawn_dequeue(smpd_context_t **context);
int smpd_handle_delayed_spawn_command(void);

#ifdef HAVE_WINDOWS_H
DWORD_PTR smpd_get_next_process_affinity_mask(void );
DWORD_PTR smpd_get_processor_affinity_mask(int proc_num);
BOOL smpd_init_affinity_table(void );

int smpd_hpc_js_rmk_init(smpd_hpc_js_handle_t *phnd);
int smpd_hpc_js_rmk_finalize(smpd_hpc_js_handle_t *phnd);
int smpd_hpc_js_rmk_alloc_nodes(smpd_hpc_js_handle_t hnd, smpd_launch_node_t *head);

int smpd_hpc_js_bs_init(smpd_hpc_js_handle_t hnd);
int smpd_hpc_js_bs_finalize(smpd_hpc_js_handle_t hnd);
int smpd_hpc_js_bs_launch_procs(smpd_hpc_js_handle_t hnd, smpd_launch_node_t *head);

#endif

#if defined(__cplusplus)
}
#endif

#endif
