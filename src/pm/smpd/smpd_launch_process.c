/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "smpd.h"
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_SYS_STAT_H
/* This is needed for umask */
#include <sys/stat.h>
#endif
#include <stdlib.h>
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif

#ifdef HAVE_WINDOWS_H

#define MAX_ERROR_LENGTH 512

#undef FCNAME
#define FCNAME "smpd_clear_process_registry"
int smpd_clear_process_registry()
{
    HKEY tkey;
    DWORD dwLen, result;
    int i;
    DWORD dwNumSubKeys, dwMaxSubKeyLen;
    char pid_str[256];
    char err_msg[MAX_ERROR_LENGTH] = "";

    smpd_enter_fn(FCNAME);

    result = RegOpenKeyEx(HKEY_LOCAL_MACHINE, SMPD_REGISTRY_KEY "\\process", 0, KEY_ALL_ACCESS, &tkey);
    if (result != ERROR_SUCCESS)
    {
	if (result != ERROR_PATH_NOT_FOUND)
	{
	    smpd_err_printf("Unable to open the " SMPD_REGISTRY_KEY "\\process registry key, error %d\n", result);
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
	return SMPD_SUCCESS;
    }

    result = RegQueryInfoKey(tkey, NULL, NULL, NULL, &dwNumSubKeys, &dwMaxSubKeyLen, NULL, NULL, NULL, NULL, NULL, NULL);
    if (result != ERROR_SUCCESS)
    {
	RegCloseKey(tkey);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    if (dwMaxSubKeyLen > 256)
    {
	smpd_err_printf("Error: Invalid process subkeys, max length is too large: %d\n", dwMaxSubKeyLen);
	RegCloseKey(tkey);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    if (dwNumSubKeys == 0)
    {
	result = RegCloseKey(tkey);
	if (result != ERROR_SUCCESS)
	{
	    smpd_translate_win_error(result, err_msg, MAX_ERROR_LENGTH, NULL);
	    smpd_err_printf("Error: RegCloseKey(HKEY_LOCAL_MACHINE\\" SMPD_REGISTRY_KEY "\\process) failed, %s\n", err_msg);
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
	result = RegDeleteKey(HKEY_LOCAL_MACHINE, SMPD_REGISTRY_KEY "\\process");
	if (result != ERROR_SUCCESS)
	{
	    smpd_translate_win_error(result, err_msg, MAX_ERROR_LENGTH, NULL);
	    smpd_err_printf("Error: Unable to remove the HKEY_LOCAL_MACHINE\\" SMPD_REGISTRY_KEY "\\process registry key, %s\n", err_msg);
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
	smpd_exit_fn(FCNAME);
	return SMPD_SUCCESS;
    }
    /* count backwards so keys can be removed */
    for (i=dwNumSubKeys-1; i>=0; i--)
    {
	dwLen = 256;
	result = RegEnumKeyEx(tkey, i, pid_str, &dwLen, NULL, NULL, NULL, NULL);
	if (result != ERROR_SUCCESS)
	{
	    smpd_err_printf("Error: Unable to enumerate the %d subkey in the " SMPD_REGISTRY_KEY "\\process registry key\n", i);
	    RegCloseKey(tkey);
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
	result = RegDeleteKey(tkey, pid_str);
	if (result != ERROR_SUCCESS)
	{
	    smpd_translate_win_error(result, err_msg, MAX_ERROR_LENGTH, NULL);
	    smpd_err_printf("Error: RegDeleteKey(HKEY_LOCAL_MACHINE\\" SMPD_REGISTRY_KEY "\\process\\%s) failed, %s\n", pid_str, err_msg);
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
    }
    result = RegCloseKey(tkey);
    if (result != ERROR_SUCCESS)
    {
	smpd_translate_win_error(result, err_msg, MAX_ERROR_LENGTH, NULL);
	smpd_err_printf("Error: RegCloseKey(HKEY_LOCAL_MACHINE\\" SMPD_REGISTRY_KEY ") failed, %s\n", err_msg);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    result = RegDeleteKey(HKEY_LOCAL_MACHINE, SMPD_REGISTRY_KEY "\\process");
    if (result != ERROR_SUCCESS)
    {
	smpd_translate_win_error(result, err_msg, MAX_ERROR_LENGTH, NULL);
	smpd_err_printf("Error: Unable to remove the HKEY_LOCAL_MACHINE\\" SMPD_REGISTRY_KEY "\\process registry key, %s\n", err_msg);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_validate_process_registry"
int smpd_validate_process_registry()
{
    int error;
    HKEY tkey;
    DWORD dwLen, result;
    int i;
    DWORD dwNumSubKeys, dwMaxSubKeyLen;
    char pid_str[100];
    int pid;
    HANDLE hTemp;
    char err_msg[MAX_ERROR_LENGTH] = "";

    smpd_enter_fn(FCNAME);

    result = RegOpenKeyEx(HKEY_LOCAL_MACHINE, SMPD_REGISTRY_KEY "\\process", 0, KEY_ALL_ACCESS, &tkey);
    if (result != ERROR_SUCCESS)
    {
	if (result != ERROR_PATH_NOT_FOUND)
	{
	    smpd_translate_win_error(result, err_msg, MAX_ERROR_LENGTH, NULL);
	    smpd_err_printf("Unable to open the smpd\\process registry key, error %d, %s\n", result, err_msg);
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
	return SMPD_SUCCESS;
    }

    result = RegQueryInfoKey(tkey, NULL, NULL, NULL, &dwNumSubKeys, &dwMaxSubKeyLen, NULL, NULL, NULL, NULL, NULL, NULL);
    if (result != ERROR_SUCCESS)
    {
	RegCloseKey(tkey);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    if (dwMaxSubKeyLen > 100)
    {
	smpd_err_printf("Error: Invalid process subkeys, max length is too large: %d\n", dwMaxSubKeyLen);
	RegCloseKey(tkey);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    if (dwNumSubKeys == 0)
    {
	RegCloseKey(tkey);
	smpd_exit_fn(FCNAME);
	return SMPD_SUCCESS;
    }
    /* count backwards so keys can be removed */
    for (i=dwNumSubKeys-1; i>=0; i--)
    {
	dwLen = 100;
	result = RegEnumKeyEx(tkey, i, pid_str, &dwLen, NULL, NULL, NULL, NULL);
	if (result != ERROR_SUCCESS)
	{
	    smpd_translate_win_error(result, err_msg, MAX_ERROR_LENGTH, NULL);
	    smpd_err_printf("Error: Unable to enumerate the %d subkey in the smpd\\process registry key, %s\n", i, err_msg);
	    RegCloseKey(tkey);
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
	pid = atoi(pid_str);
	printf("pid = %d\n", pid);fflush(stdout);
	hTemp = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);
	if (hTemp == NULL)
	{
	    error = GetLastError();
	    if (error == ERROR_INVALID_PARAMETER)
	    {
		RegDeleteKey(tkey, pid_str);
	    }
	    /*
	    else
	    {
		printf("error = %d\n", error);
	    }
	    */
	}
	else
	{
	    CloseHandle(hTemp);
	}
    }
    result = RegCloseKey(tkey);
    if (result != ERROR_SUCCESS)
    {
	smpd_translate_win_error(result, err_msg, MAX_ERROR_LENGTH, NULL);
	smpd_err_printf("Error: RegCloseKey(HKEY_LOCAL_MACHINE\\" SMPD_REGISTRY_KEY "\\process) failed, %s\n", err_msg);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_process_to_registry"
int smpd_process_to_registry(smpd_process_t *process, char *actual_exe)
{
    HKEY tkey;
    DWORD len, result;
    char name[1024];
    char err_msg[MAX_ERROR_LENGTH] = "";

    smpd_enter_fn(FCNAME);

    if (process == NULL)
    {
	smpd_dbg_printf("NULL process passed to smpd_process_to_registry.\n");
	return SMPD_FAIL;
    }

    len = snprintf(name, 1024, SMPD_REGISTRY_KEY "\\process\\%d", process->pid);
    if (len < 0 || len > 1023)
    {
	smpd_dbg_printf("unable to create a string of the registry key.\n");
	return SMPD_FAIL;
    }

    result = RegCreateKeyEx(HKEY_LOCAL_MACHINE, name,
	0, NULL, REG_OPTION_VOLATILE, KEY_ALL_ACCESS, NULL, &tkey, NULL);
    if (result != ERROR_SUCCESS)
    {
	smpd_translate_win_error(result, err_msg, MAX_ERROR_LENGTH, NULL);
	smpd_err_printf("Unable to open the HKEY_LOCAL_MACHINE\\%s registry key, error %d, %s\n", name, result, err_msg);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    len = (DWORD)(strlen(actual_exe)+1);
    result = RegSetValueEx(tkey, "exe", 0, REG_SZ, (const BYTE *)actual_exe, len);
    if (result != ERROR_SUCCESS)
    {
	smpd_translate_win_error(result, err_msg, MAX_ERROR_LENGTH, NULL);
	smpd_err_printf("Unable to write the process registry value 'exe:%s', error %d, %s\n", process->exe, result, err_msg);
	RegCloseKey(tkey);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    result = RegCloseKey(tkey);
    if (result != ERROR_SUCCESS)
    {
	smpd_translate_win_error(result, err_msg, MAX_ERROR_LENGTH, NULL);
	smpd_err_printf("Error: RegCloseKey(HKEY_LOCAL_MACHINE\\" SMPD_REGISTRY_KEY "\\process\\%d) failed, %s\n", process->pid, err_msg);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_process_from_registry"
int smpd_process_from_registry(smpd_process_t *process)
{
    DWORD len, result;
    char name[1024];
    char err_msg[MAX_ERROR_LENGTH] = "";

    smpd_enter_fn(FCNAME);

    if (process == NULL)
    {
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    len = snprintf(name, 1024, SMPD_REGISTRY_KEY "\\process\\%d", process->pid);
    if (len < 0 || len > 1023)
    {
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    result = RegDeleteKey(HKEY_LOCAL_MACHINE, name);
    if (result != ERROR_SUCCESS)
    {
	if (result == ERROR_FILE_NOT_FOUND || result == ERROR_PATH_NOT_FOUND)
	{
	    /* Key already deleted, return success */
	    smpd_exit_fn(FCNAME);
	    return SMPD_SUCCESS;
	}
	smpd_translate_win_error(result, err_msg, MAX_ERROR_LENGTH, NULL);
	smpd_err_printf("Unable to delete the HKEY_LOCAL_MACHINE\\%s registry key, error %d\n", name, result, err_msg);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_get_user_handle"
int smpd_get_user_handle(char *account, char *domain, char *password, HANDLE *handle_ptr)
{
    HANDLE hUser;
    int error;
    int num_tries = 3;

    smpd_enter_fn(FCNAME);

    if (domain)
    {
	smpd_dbg_printf("LogonUser(%s\\%s)\n", domain, account);
    }
    else
    {
	smpd_dbg_printf("LogonUser(%s)\n", account);
    }

    /* logon the user */
    while (!LogonUser(
	account,
	domain,
	password,
	LOGON32_LOGON_INTERACTIVE,
	LOGON32_PROVIDER_DEFAULT,
	&hUser))
    {
	error = GetLastError();
	if (error == ERROR_NO_LOGON_SERVERS)
	{
	    if (num_tries)
		Sleep(250);
	    else
	    {
		*handle_ptr = INVALID_HANDLE_VALUE;
		smpd_exit_fn(FCNAME);
		return error;
	    }
	    num_tries--;
	}
	else
	{
	    *handle_ptr = INVALID_HANDLE_VALUE;
	    smpd_exit_fn(FCNAME);
	    return error;
	}
    }

    *handle_ptr = hUser;
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_get_user_name"
int smpd_get_user_name(char *account, char *domain, char *full_domain)
{
    DWORD len;
    char name[SMPD_MAX_ACCOUNT_LENGTH];
    char *separator;
    size_t i;

    *account = '\0';
    if (domain != NULL)
	*domain = '\0';
    if (full_domain != NULL)
	*full_domain = '\0';

    len = 100;
    if (GetUserNameEx(NameSamCompatible, name, &len))
    {
	for (i=0; i<strlen(name); i++)
	{
	    name[i] = (char)(tolower(name[i]));
	}
	separator = strchr(name, '\\');
	if (separator)
	{
	    *separator = '\0';
	    separator++;
	}
	if (domain != NULL)
	    strcpy(domain, name);
	strcpy(account, separator);
    }

    if (full_domain != NULL)
    {
	len = 100;
	if (GetUserNameEx(NameDnsDomain, name, &len))
	{
	    for (i=0; i<strlen(name); i++)
	    {
		name[i] = (char)(tolower(name[i]));
	    }
	    separator = strchr(name, '\\');
	    if (separator)
	    {
		*separator = '\0';
		strcpy(full_domain, name);
	    }
	}
    }

    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "SetEnvironmentVariables"
static void SetEnvironmentVariables(char *bEnv)
{
    char name[SMPD_MAX_ENV_LENGTH], equals[3], value[SMPD_MAX_ENV_LENGTH];

    smpd_enter_fn(FCNAME);
    for (;;)
    {
	name[0] = '\0';
	equals[0] = '\0';
	value[0] = '\0';
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
	smpd_dbg_printf("setting environment variable: <%s> = <%s>\n", name, value);
	SetEnvironmentVariable(name, value);
    }
    smpd_exit_fn(FCNAME);
}

#undef FCNAME
#define FCNAME "RemoveEnvironmentVariables"
static void RemoveEnvironmentVariables(char *bEnv)
{
    char name[SMPD_MAX_ENV_LENGTH], equals[3], value[SMPD_MAX_ENV_LENGTH];

    smpd_enter_fn(FCNAME);
    for (;;)
    {
	name[0] = '\0';
	equals[0] = '\0';
	value[0] = '\0';
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
	/*smpd_dbg_printf("removing environment variable <%s>\n", name);*/
	SetEnvironmentVariable(name, NULL);
    }
    smpd_exit_fn(FCNAME);
}

#undef FCNAME
#define FCNAME "smpd_priority_class_to_win_class"
int smpd_priority_class_to_win_class(int *priorityClass)
{
    smpd_enter_fn(FCNAME);
    switch (*priorityClass)
    {
    case 0:
	*priorityClass = IDLE_PRIORITY_CLASS;
	break;
    case 1:
	*priorityClass = BELOW_NORMAL_PRIORITY_CLASS;
	break;
    case 2:
	*priorityClass = NORMAL_PRIORITY_CLASS;
	break;
    case 3:
	*priorityClass = ABOVE_NORMAL_PRIORITY_CLASS;
	break;
    case 4:
	*priorityClass = HIGH_PRIORITY_CLASS;
	break;
    default:
	*priorityClass = NORMAL_PRIORITY_CLASS;
	break;
    }
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_priority_to_win_priority"
int smpd_priority_to_win_priority(int *priority)
{
    smpd_enter_fn(FCNAME);
    switch (*priority)
    {
    case 0:
	*priority = THREAD_PRIORITY_IDLE;
	break;
    case 1:
	*priority = THREAD_PRIORITY_LOWEST;
	break;
    case 2:
	*priority = THREAD_PRIORITY_BELOW_NORMAL;
	break;
    case 3:
	*priority = THREAD_PRIORITY_NORMAL;
	break;
    case 4:
	*priority = THREAD_PRIORITY_ABOVE_NORMAL;
	break;
    case 5:
	*priority = THREAD_PRIORITY_HIGHEST;
	break;
    default:
	*priority = THREAD_PRIORITY_NORMAL;
	break;
    }
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

/* Windows code */

typedef struct smpd_piothread_arg_t
{
    HANDLE hIn;
    SOCKET hOut;
    int pid;
} smpd_piothread_arg_t;

typedef struct smpd_pinthread_arg_t
{
    SOCKET hIn;
    HANDLE hOut;
    int pid;
} smpd_pinthread_arg_t;

#undef FCNAME
#define FCNAME "smpd_easy_send"
static int smpd_easy_send(SOCKET sock, char *buffer, int length)
{
    int error;
    int num_sent, num_left;

    smpd_exit_fn(FCNAME);
    num_left = length;
    while (num_left)
    {
	while ((num_sent = send(sock, buffer, num_left, 0)) == SOCKET_ERROR)
	{
	    error = WSAGetLastError();
	    if (error == WSAEWOULDBLOCK)
	    {
		Sleep(0);
		continue;
	    }
	    if (error == WSAENOBUFS)
	    {
		/* If there is no buffer space available then split the buffer in half and send each piece separately.*/
		if (smpd_easy_send(sock, buffer, num_left/2) == SOCKET_ERROR)
		{
		    smpd_exit_fn(FCNAME);
		    return SOCKET_ERROR;
		}
		if (smpd_easy_send(sock, buffer+(num_left/2), num_left - (num_left/2)) == SOCKET_ERROR)
		{
		    smpd_exit_fn(FCNAME);
		    return SOCKET_ERROR;
		}
		smpd_exit_fn(FCNAME);
		return length;
	    }
	    WSASetLastError(error);
	    smpd_exit_fn(FCNAME);
	    return SOCKET_ERROR;
	}
	num_left = num_left - num_sent;
	buffer = buffer + num_sent;
    }
    smpd_exit_fn(FCNAME);
    return length;
}

int smpd_piothread(smpd_piothread_arg_t *p)
{
    char buffer[8192];
    DWORD num_read;
    HANDLE hIn;
    SOCKET hOut;
    DWORD error;
    char bogus_char;
    int pid;
    double t1, t2;

    hIn = p->hIn;
    hOut = p->hOut;
    pid = p->pid;
    MPIU_Free(p);
    p = NULL;

    smpd_dbg_printf("*** entering smpd_piothread pid:%d sock:%d ***\n", pid, hOut);
    for (;;)
    {
	num_read = 0;
	if (!ReadFile(hIn, buffer, 8192, &num_read, NULL))
	{
	    error = GetLastError();
	    /* If there was an error but some bytes were read, send those bytes before exiting */
	    if (num_read > 0)
	    {
		if (smpd_easy_send(hOut, buffer, num_read) == SOCKET_ERROR)
		{
		    smpd_dbg_printf("smpd_easy_send of %d bytes failed.\n", num_read);
		    break;
		}
	    }
	    smpd_dbg_printf("ReadFile failed, error %d\n", error);
	    break;
	}
	if (num_read < 1)
	{
	    smpd_dbg_printf("ReadFile returned %d bytes\n", num_read);
	    break;
	}
	/*smpd_dbg_printf("*** smpd_piothread read %d bytes ***\n", num_read);*/
	if (smpd_easy_send(hOut, buffer, num_read) == SOCKET_ERROR)
	{
	    smpd_dbg_printf("smpd_easy_send of %d bytes failed.\n", num_read);
	    break;
	}
	/*smpd_dbg_printf("*** smpd_piothread wrote %d bytes ***\n", num_read);*/
    }
    smpd_dbg_printf("*** smpd_piothread finishing pid:%d ***\n", pid);
    /*
    FlushFileBuffers((HANDLE)hOut);
    if (shutdown(hOut, SD_BOTH) == SOCKET_ERROR)
    {
	smpd_err_printf("shutdown failed, sock %d, error %d\n", hOut, WSAGetLastError());
    }
    if (closesocket(hOut) == SOCKET_ERROR)
    {
	smpd_err_printf("closesocket failed, sock %d, error %d\n", hOut, WSAGetLastError());
    }
    */
    if (shutdown(hOut, SD_SEND) == SOCKET_ERROR)
    {
	smpd_err_printf("shutdown failed, sock %d, error %d\n", hOut, WSAGetLastError());
    }
    t1 = PMPI_Wtime();
    recv(hOut, &bogus_char, 1, 0);
    if (closesocket(hOut) == SOCKET_ERROR)
    {
	smpd_err_printf("closesocket failed, sock %d, error %d\n", hOut, WSAGetLastError());
    }
    t2 = PMPI_Wtime();
    smpd_dbg_printf("closing output socket took %.3f seconds\n", t2-t1);
    CloseHandle(hIn);
    /*smpd_dbg_printf("*** exiting smpd_piothread ***\n");*/
    return 0;
}

/* one line at a time version */
int smpd_pinthread(smpd_pinthread_arg_t *p)
{
    char str [SMPD_MAX_CMD_LENGTH];
    int index;
    DWORD num_written;
    int num_read;
    SOCKET hIn;
    HANDLE hOut;
    int pid;
    /*int i;*/
    /*
    char bogus_char;
    double t1, t2;
    */

    hIn = p->hIn;
    hOut = p->hOut;
    pid = p->pid;
    MPIU_Free(p);
    p = NULL;

    smpd_dbg_printf("*** entering smpd_pinthread pid:%d sock:%d ***\n", pid, hIn);
    index = 0;
    for (;;)
    {
	num_read = recv(hIn, &str[index], 1, 0);
	if (num_read == SOCKET_ERROR || num_read == 0)
	{
	    if (num_read == SOCKET_ERROR && WSAGetLastError() == WSAEWOULDBLOCK)
	    {
		u_long optval = (u_long)TRUE;
		ioctlsocket(hIn, FIONBIO, &optval);
		continue;
	    }
	    if (index > 0)
	    {
		/* write any buffered data before exiting */
		if (!WriteFile(hOut, str, index, &num_written, NULL))
		{
		    smpd_dbg_printf("WriteFile failed, error %d\n", GetLastError());
		    break;
		}
	    }
	    if (num_read != 0)
	    {
		smpd_dbg_printf("recv from stdin socket failed, error %d.\n", WSAGetLastError());
	    }
	    break;
	}
	if (str[index] == '\n' || index == SMPD_MAX_CMD_LENGTH-1)
	{
	    smpd_dbg_printf("writing %d bytes to the process's stdin\n", index+1);
	    if (!WriteFile(hOut, str, index+1, &num_written, NULL))
	    {
		smpd_dbg_printf("WriteFile failed, error %d\n", GetLastError());
		break;
	    }
	    /*
	    smpd_dbg_printf("wrote: ");
	    for (i=0; i<=index; i++)
	    {
		smpd_dbg_printf("(%d)'%c'", (int)str[i], str[i]);
	    }
	    smpd_dbg_printf("\n");
	    */
	    index = 0;
	}
	else
	{
	    smpd_dbg_printf("read character(%d)'%c'\n", (int)str[index], str[index]);
	    index++;
	}
    }
    smpd_dbg_printf("*** smpd_pinthread finishing pid:%d ***\n", pid);
    FlushFileBuffers(hOut);
    if (shutdown(hIn, SD_BOTH) == SOCKET_ERROR)
    {
	smpd_err_printf("shutdown failed, sock %d, error %d\n", hIn, WSAGetLastError());
    }
    if (closesocket(hIn) == SOCKET_ERROR)
    {
	smpd_err_printf("closesocket failed, sock %d, error %d\n", hIn, WSAGetLastError());
    }
    /*
    if (shutdown(hIn, SD_SEND) == SOCKET_ERROR)
    {
	smpd_err_printf("shutdown failed, sock %d, error %d\n", hIn, WSAGetLastError());
    }
    t1 = PMPI_Wtime();
    recv(hIn, &bogus_char, 1, 0);
    if (closesocket(hIn) == SOCKET_ERROR)
    {
	smpd_err_printf("closesocket failed, sock %d, error %d\n", hIn, WSAGetLastError());
    }
    t2 = PMPI_Wtime();
    smpd_dbg_printf("closing input socket took %.3f seconds\n", t2-t1);
    */
    CloseHandle(hOut);
    /*smpd_dbg_printf("*** exiting smpd_pinthread ***\n");*/
    return 0;
}

#undef FCNAME
#define FCNAME "smpd_launch_process"
int smpd_launch_process(smpd_process_t *process, int priorityClass, int priority, int dbg, SMPDU_Sock_set_t set)
{
    HANDLE hStdin = INVALID_HANDLE_VALUE, hStdout = INVALID_HANDLE_VALUE, hStderr = INVALID_HANDLE_VALUE;
    SOCKET hSockStdinR = INVALID_SOCKET, hSockStdinW = INVALID_SOCKET;
    SOCKET hSockStdoutR = INVALID_SOCKET, hSockStdoutW = INVALID_SOCKET;
    SOCKET hSockStderrR = INVALID_SOCKET, hSockStderrW = INVALID_SOCKET;
    SOCKET hSockPmiR = INVALID_SOCKET, hSockPmiW = INVALID_SOCKET;
    HANDLE hPipeStdinR = NULL, hPipeStdinW = NULL;
    HANDLE hPipeStdoutR = NULL, hPipeStdoutW = NULL;
    HANDLE hPipeStderrR = NULL, hPipeStderrW = NULL;
    HANDLE hIn = INVALID_HANDLE_VALUE, hOut = INVALID_HANDLE_VALUE, hErr = INVALID_HANDLE_VALUE;
    STARTUPINFO saInfo;
    PROCESS_INFORMATION psInfo = { 0 };
    void *pEnv=NULL;
    char tSavedPath[MAX_PATH] = ".";
    DWORD launch_flag = 0;
    int nError = 0, result = 0;
    /*unsigned long blocking_flag = 0;*/
    SMPDU_Sock_t sock_in = SMPDU_SOCK_INVALID_SOCK, sock_out = SMPDU_SOCK_INVALID_SOCK, sock_err = SMPDU_SOCK_INVALID_SOCK, sock_pmi = SMPDU_SOCK_INVALID_SOCK;
    SECURITY_ATTRIBUTES saAttr;
    char str[8192], sock_str[20];
    BOOL bSuccess = TRUE;
    char *actual_exe, exe_data[SMPD_MAX_EXE_LENGTH];
    char *args;
    char temp_exe[SMPD_MAX_EXE_LENGTH];
    SMPDU_Sock_t sock_pmi_listener;
    smpd_context_t *listener_context;
    int listener_port = 0;
    char host_description[SMPD_MAX_HOST_DESC_LENGTH];
    char err_msg[MAX_ERROR_LENGTH] = "";

    smpd_enter_fn(FCNAME);

    /* Initialize the psInfo structure in case there is an error an we jump to CLEANUP before psInfo is set */
    psInfo.hProcess = INVALID_HANDLE_VALUE;

    /* resolve the executable name */
    if (process->path[0] != '\0')
    {
	args = process->exe;
	result = MPIU_Str_get_string(&args, temp_exe, SMPD_MAX_EXE_LENGTH);
	if (result != MPIU_STR_SUCCESS)
	{
	}
	smpd_dbg_printf("searching for '%s' in '%s'\n", temp_exe, process->path);
	if (smpd_search_path(process->path, temp_exe, SMPD_MAX_EXE_LENGTH, exe_data))
	{
	    if (args != NULL)
	    {
		strncat(exe_data, " ", SMPD_MAX_EXE_LENGTH - strlen(exe_data));
		strncat(exe_data, args, SMPD_MAX_EXE_LENGTH - strlen(exe_data));
		exe_data[SMPD_MAX_EXE_LENGTH-1] = '\0';
	    }
	    actual_exe = exe_data;
	}
	else
	{
	    actual_exe = process->exe;
	}
    }
    else
    {
	actual_exe = process->exe;
    }

    smpd_priority_class_to_win_class(&priorityClass);
    smpd_priority_to_win_priority(&priority);

    /* Save stdin, stdout, and stderr */
    result = WaitForSingleObject(smpd_process.hLaunchProcessMutex, INFINITE);
    if (result == WAIT_FAILED)
    {
	result = GetLastError();
	smpd_translate_win_error(result, err_msg, MAX_ERROR_LENGTH, NULL);
	smpd_err_printf("Error waiting for smpd_process.hLaunchProcessMutex %d, %s\n", result, err_msg);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    hStdin = GetStdHandle(STD_INPUT_HANDLE);
    hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
    hStderr = GetStdHandle(STD_ERROR_HANDLE);
    if (hStdin == INVALID_HANDLE_VALUE || hStdout == INVALID_HANDLE_VALUE  || hStderr == INVALID_HANDLE_VALUE)
    {
	nError = GetLastError(); /* This will only be correct if stderr failed */
	ReleaseMutex(smpd_process.hLaunchProcessMutex);
	smpd_translate_win_error(nError, err_msg, MAX_ERROR_LENGTH, NULL);
	smpd_err_printf("GetStdHandle failed, error %d, %s\n", nError, err_msg);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;;
    }

    /* Create sockets for stdin, stdout, and stderr */
    nError = smpd_make_socket_loop_choose(&hSockStdinR, SMPD_FALSE, &hSockStdinW, SMPD_TRUE);
    if (nError)
    {
	smpd_err_printf("smpd_make_socket_loop failed, error %d\n", nError);
	goto CLEANUP;
    }
    nError = smpd_make_socket_loop_choose(&hSockStdoutR, SMPD_TRUE, &hSockStdoutW, SMPD_FALSE);
    if (nError)
    {
	smpd_err_printf("smpd_make_socket_loop failed, error %d\n", nError);
	goto CLEANUP;
    }
    nError = smpd_make_socket_loop_choose(&hSockStderrR, SMPD_TRUE, &hSockStderrW, SMPD_FALSE);
    if (nError)
    {
	smpd_err_printf("smpd_make_socket_loop failed, error %d\n", nError);
	goto CLEANUP;
    }
    if (process->pmi != NULL)
    {
	if (smpd_process.use_inherited_handles)
	{
	    nError = smpd_make_socket_loop(&hSockPmiR, &hSockPmiW);
	    if (nError)
	    {
		smpd_err_printf("smpd_make_socket_loop failed, error %d\n", nError);
		goto CLEANUP;
	    }
	}
	else
	{
	    nError = SMPDU_Sock_listen(set, NULL, &listener_port, &sock_pmi_listener); 
	    if (nError != SMPD_SUCCESS)
	    {
		smpd_err_printf("SMPDU_Sock_listen failed,\nsock error: %s\n", get_sock_error_string(nError));
		goto CLEANUP;
	    }
	    smpd_dbg_printf("pmi listening on port %d\n", listener_port);

	    nError = smpd_create_context(SMPD_CONTEXT_PMI_LISTENER, set, sock_pmi_listener, -1, &listener_context);
	    if (nError != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to create a context for the pmi listener.\n");
		goto CLEANUP;
	    }
	    nError = SMPDU_Sock_set_user_ptr(sock_pmi_listener, listener_context);
	    if (nError != SMPD_SUCCESS)
	    {
		smpd_err_printf("SMPDU_Sock_set_user_ptr failed,\nsock error: %s\n", get_sock_error_string(nError));
		goto CLEANUP;
	    }
	    listener_context->state = SMPD_PMI_LISTENING;
	    /* Adding process rank since the interface for SMPDU_Sock_get_host_description changed */
	    nError = SMPDU_Sock_get_host_description(process->rank, host_description, SMPD_MAX_HOST_DESC_LENGTH);
	    if (nError != SMPD_SUCCESS)
	    {
		smpd_err_printf("SMPDU_Sock_get_host_description failed,\nsock error: %s\n", get_sock_error_string(nError));
		goto CLEANUP;
	    }
	    /* save the listener sock in the slot reserved for the client sock so we can match the listener
	    to the process structure when the client sock is accepted */
	    process->pmi->sock = sock_pmi_listener;
	}
    }

    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.lpSecurityDescriptor = NULL;
    saAttr.bInheritHandle = TRUE;

    /* Create the pipes for stdout, stderr */
    if (!CreatePipe(&hPipeStdinR, &hPipeStdinW, &saAttr, 0))
    {
	smpd_err_printf("CreatePipe(stdin) failed, error %d\n", GetLastError());
	goto CLEANUP;
    }
    if (!CreatePipe(&hPipeStdoutR, &hPipeStdoutW, &saAttr, 0))
    {
	smpd_err_printf("CreatePipe(stdout) failed, error %d\n", GetLastError());
	goto CLEANUP;
    }
    if (!CreatePipe(&hPipeStderrR, &hPipeStderrW, &saAttr, 0))
    {
	smpd_err_printf("CreatePipe(stderr) failed, error %d\n", GetLastError());
	goto CLEANUP;
    }

    /* Make the ends of the pipes that this process will use not inheritable */
    /*
    if (!DuplicateHandle(GetCurrentProcess(), (HANDLE)hSockStdinW, GetCurrentProcess(), &hIn, 
	0, FALSE, DUPLICATE_CLOSE_SOURCE | DUPLICATE_SAME_ACCESS))
    {
	nError = GetLastError();
	smpd_err_printf("DuplicateHandle failed, error %d\n", nError);
	goto CLEANUP;
    }
    */
    if (!DuplicateHandle(GetCurrentProcess(), (HANDLE)hPipeStdinW, GetCurrentProcess(), &hIn, 
	0, FALSE, DUPLICATE_CLOSE_SOURCE | DUPLICATE_SAME_ACCESS))
    {
	nError = GetLastError();
	smpd_err_printf("DuplicateHandle failed, error %d\n", nError);
	goto CLEANUP;
    }
    /*
    if (!DuplicateHandle(GetCurrentProcess(), (HANDLE)hSockStdoutR, GetCurrentProcess(), &hOut, 
	0, FALSE, DUPLICATE_CLOSE_SOURCE | DUPLICATE_SAME_ACCESS))
    {
	nError = GetLastError();
	smpd_err_printf("DuplicateHandle failed, error %d\n", nError);
	goto CLEANUP;
    }
    if (!DuplicateHandle(GetCurrentProcess(), (HANDLE)hSockStderrR, GetCurrentProcess(), &hErr, 
	0, FALSE, DUPLICATE_CLOSE_SOURCE | DUPLICATE_SAME_ACCESS))
    {
	nError = GetLastError();
	smpd_err_printf("DuplicateHandle failed, error %d\n", nError);
	goto CLEANUP;
    }
    */
    if (!DuplicateHandle(GetCurrentProcess(), (HANDLE)hPipeStdoutR, GetCurrentProcess(), &hOut, 
	0, FALSE, DUPLICATE_CLOSE_SOURCE | DUPLICATE_SAME_ACCESS))
    {
	nError = GetLastError();
	smpd_err_printf("DuplicateHandle failed, error %d\n", nError);
	goto CLEANUP;
    }
    if (!DuplicateHandle(GetCurrentProcess(), (HANDLE)hPipeStderrR, GetCurrentProcess(), &hErr, 
	0, FALSE, DUPLICATE_CLOSE_SOURCE | DUPLICATE_SAME_ACCESS))
    {
	nError = GetLastError();
	smpd_err_printf("DuplicateHandle failed, error %d\n", nError);
	goto CLEANUP;
    }
    if (process->pmi != NULL && smpd_process.use_inherited_handles)
    {
	if (!DuplicateHandle(GetCurrentProcess(), (HANDLE)hSockPmiR, GetCurrentProcess(), (LPHANDLE)&hSockPmiR, 
	    0, FALSE, DUPLICATE_CLOSE_SOURCE | DUPLICATE_SAME_ACCESS))
	{
	    nError = GetLastError();
	    smpd_err_printf("DuplicateHandle failed, error %d\n", nError);
	    goto CLEANUP;
	}
    }

    /* prevent the socket loops from being inherited */
    if (!DuplicateHandle(GetCurrentProcess(), (HANDLE)hSockStdinR, GetCurrentProcess(), (LPHANDLE)&hSockStdinR, 
	0, FALSE, DUPLICATE_CLOSE_SOURCE | DUPLICATE_SAME_ACCESS))
    {
	nError = GetLastError();
	smpd_err_printf("DuplicateHandle failed, error %d\n", nError);
	goto CLEANUP;
    }
    if (!DuplicateHandle(GetCurrentProcess(), (HANDLE)hSockStdoutR, GetCurrentProcess(), (LPHANDLE)&hSockStdoutR, 
	0, FALSE, DUPLICATE_CLOSE_SOURCE | DUPLICATE_SAME_ACCESS))
    {
	nError = GetLastError();
	smpd_err_printf("DuplicateHandle failed, error %d\n", nError);
	goto CLEANUP;
    }
    if (!DuplicateHandle(GetCurrentProcess(), (HANDLE)hSockStderrR, GetCurrentProcess(), (LPHANDLE)&hSockStderrR, 
	0, FALSE, DUPLICATE_CLOSE_SOURCE | DUPLICATE_SAME_ACCESS))
    {
	nError = GetLastError();
	smpd_err_printf("DuplicateHandle failed, error %d\n", nError);
	goto CLEANUP;
    }
    if (!DuplicateHandle(GetCurrentProcess(), (HANDLE)hSockStdinW, GetCurrentProcess(), (LPHANDLE)&hSockStdinW, 
	0, FALSE, DUPLICATE_CLOSE_SOURCE | DUPLICATE_SAME_ACCESS))
    {
	nError = GetLastError();
	smpd_err_printf("DuplicateHandle failed, error %d\n", nError);
	goto CLEANUP;
    }
    if (!DuplicateHandle(GetCurrentProcess(), (HANDLE)hSockStdoutW, GetCurrentProcess(), (LPHANDLE)&hSockStdoutW, 
	0, FALSE, DUPLICATE_CLOSE_SOURCE | DUPLICATE_SAME_ACCESS))
    {
	nError = GetLastError();
	smpd_err_printf("DuplicateHandle failed, error %d\n", nError);
	goto CLEANUP;
    }
    if (!DuplicateHandle(GetCurrentProcess(), (HANDLE)hSockStderrW, GetCurrentProcess(), (LPHANDLE)&hSockStderrW, 
	0, FALSE, DUPLICATE_CLOSE_SOURCE | DUPLICATE_SAME_ACCESS))
    {
	nError = GetLastError();
	smpd_err_printf("DuplicateHandle failed, error %d\n", nError);
	goto CLEANUP;
    }

    /* make the ends used by the spawned process blocking */
    /*
    blocking_flag = 0;
    ioctlsocket(hSockStdinR, FIONBIO, &blocking_flag);
    */
    /*
    blocking_flag = 0;
    ioctlsocket(hSockStdoutW, FIONBIO, &blocking_flag);
    blocking_flag = 0;
    ioctlsocket(hSockStderrW, FIONBIO, &blocking_flag);
    */

    /* Set stdin, stdout, and stderr to the ends of the pipe the created process will use */
    /*
    if (!SetStdHandle(STD_INPUT_HANDLE, (HANDLE)hSockStdinR))
    {
	nError = GetLastError();
	smpd_err_printf("SetStdHandle failed, error %d\n", nError);
	goto CLEANUP;
    }
    */
    if (!SetStdHandle(STD_INPUT_HANDLE, (HANDLE)hPipeStdinR))
    {
	nError = GetLastError();
	smpd_err_printf("SetStdHandle failed, error %d\n", nError);
	goto CLEANUP;
    }
    /*
    if (!SetStdHandle(STD_OUTPUT_HANDLE, (HANDLE)hSockStdoutW))
    {
	nError = GetLastError();
	smpd_err_printf("SetStdHandle failed, error %d\n", nError);
	goto RESTORE_CLEANUP;
    }
    if (!SetStdHandle(STD_ERROR_HANDLE, (HANDLE)hSockStderrW))
    {
	nError = GetLastError();
	smpd_err_printf("SetStdHandle failed, error %d\n", nError);
	goto RESTORE_CLEANUP;
    }
    */
    if (!SetStdHandle(STD_OUTPUT_HANDLE, (HANDLE)hPipeStdoutW))
    {
	nError = GetLastError();
	smpd_err_printf("SetStdHandle failed, error %d\n", nError);
	goto RESTORE_CLEANUP;
    }
    if (!SetStdHandle(STD_ERROR_HANDLE, (HANDLE)hPipeStderrW))
    {
	nError = GetLastError();
	smpd_err_printf("SetStdHandle failed, error %d\n", nError);
	goto RESTORE_CLEANUP;
    }

    /* Create the process */
    memset(&saInfo, 0, sizeof(STARTUPINFO));
    saInfo.cb = sizeof(STARTUPINFO);
    /*saInfo.hStdInput = (HANDLE)hSockStdinR;*/
    saInfo.hStdInput = (HANDLE)hPipeStdinR;
    /*
    saInfo.hStdOutput = (HANDLE)hSockStdoutW;
    saInfo.hStdError = (HANDLE)hSockStderrW;
    */
    saInfo.hStdOutput = (HANDLE)hPipeStdoutW;
    saInfo.hStdError = (HANDLE)hPipeStderrW;
    saInfo.dwFlags = STARTF_USESTDHANDLES;

    SetEnvironmentVariables(process->env);
    if (process->pmi != NULL)
    {
	sprintf(str, "%d", process->rank);
	smpd_dbg_printf("env: PMI_RANK=%s\n", str);
	SetEnvironmentVariable("PMI_RANK", str);
	sprintf(str, "%d", process->nproc);
	smpd_dbg_printf("env: PMI_SIZE=%s\n", str);
	SetEnvironmentVariable("PMI_SIZE", str);
	sprintf(str, "%s", process->kvs_name);
	smpd_dbg_printf("env: PMI_KVS=%s\n", str);
	SetEnvironmentVariable("PMI_KVS", str);
	sprintf(str, "%s", process->domain_name);
	smpd_dbg_printf("env: PMI_DOMAIN=%s\n", str);
	SetEnvironmentVariable("PMI_DOMAIN", str);
	if (smpd_process.use_inherited_handles)
	{
	    smpd_encode_handle(sock_str, (HANDLE)hSockPmiW);
	    sprintf(str, "%s", sock_str);
	    smpd_dbg_printf("env: PMI_SMPD_FD=%s\n", str);
	    SetEnvironmentVariable("PMI_SMPD_FD", str);
	}
	else
	{
	    smpd_dbg_printf("env: PMI_HOST=%s\n", host_description);
	    SetEnvironmentVariable("PMI_HOST", host_description);
	    sprintf(str, "%d", listener_port);
	    smpd_dbg_printf("env: PMI_PORT=%s\n", str);
	    SetEnvironmentVariable("PMI_PORT", str);
	}
	sprintf(str, "%d", smpd_process.id);
	smpd_dbg_printf("env: PMI_SMPD_ID=%s\n", str);
	SetEnvironmentVariable("PMI_SMPD_ID", str);
	sprintf(str, "%d", process->id);
	smpd_dbg_printf("env: PMI_SMPD_KEY=%s\n", str);
	SetEnvironmentVariable("PMI_SMPD_KEY", str);
	if (process->clique[0] != '\0')
	{
	    sprintf(str, "%s", process->clique);
	    smpd_dbg_printf("env: PMI_CLIQUE=%s\n", str);
	    SetEnvironmentVariable("PMI_CLIQUE", str);
	}
	sprintf(str, "%d", process->spawned);
	smpd_dbg_printf("env: PMI_SPAWN=%s\n", str);
	SetEnvironmentVariable("PMI_SPAWN", str);
	sprintf(str, "%d", process->appnum);
	smpd_dbg_printf("env: PMI_APPNUM=%s\n", str);
	SetEnvironmentVariable("PMI_APPNUM", str);
    }
    pEnv = GetEnvironmentStrings();

    GetCurrentDirectory(MAX_PATH, tSavedPath);
    SetCurrentDirectory(process->dir);

    launch_flag = 
	CREATE_SUSPENDED | /*CREATE_NEW_CONSOLE*/ /*DETACHED_PROCESS*/ CREATE_NO_WINDOW | priorityClass;
    if (dbg)
	launch_flag = launch_flag | DEBUG_PROCESS;

    smpd_dbg_printf("CreateProcess(%s)\n", actual_exe);
    psInfo.hProcess = INVALID_HANDLE_VALUE;
    if (CreateProcess(
	NULL,
	actual_exe,
	NULL, NULL, TRUE,
	launch_flag,
	pEnv,
	NULL,
	&saInfo, &psInfo))
    {
	SetThreadPriority(psInfo.hThread, priority);
	process->pid = psInfo.dwProcessId;
    }
    else
    {
	nError = GetLastError();
	smpd_err_printf("CreateProcess('%s') failed, error %d\n", process->exe, nError);
	smpd_translate_win_error(nError, process->err_msg, SMPD_MAX_ERROR_LEN, "CreateProcess(%s) on '%s' failed, error %d - ",
	    process->exe, smpd_process.host, nError);
	psInfo.hProcess = INVALID_HANDLE_VALUE;
	bSuccess = FALSE;
    }

#ifdef HAVE_WINDOWS_H
    if(smpd_process.set_affinity)
    {
        DWORD_PTR mask;
        if(process->binding_proc != -1){
            /* Get affinity mask corresponding to the binding proc */
            mask = smpd_get_processor_affinity_mask(process->binding_proc);
        }
        else{
            /* Get affinity mask decided by smpd - auto binding */
            mask = smpd_get_next_process_affinity_mask();
        }
        if(mask != NULL){
            /* FIXME: The return vals of these functions are not checked ! */
            smpd_dbg_printf("Setting the process/thread affinity (mask=%lu)\n", mask);
            SetProcessAffinityMask(psInfo.hProcess, mask);
            SetThreadAffinityMask(psInfo.hThread, mask);
		}
        else{
            smpd_dbg_printf("Unable to set process/thread affinity (mask=%lu)\n", mask);
        }
    }
#endif

    FreeEnvironmentStrings((TCHAR*)pEnv);
    SetCurrentDirectory(tSavedPath);
    RemoveEnvironmentVariables(process->env);
    if (process->pmi != NULL)
    {
	SetEnvironmentVariable("PMI_RANK", NULL);
	SetEnvironmentVariable("PMI_SIZE", NULL);
	SetEnvironmentVariable("PMI_KVS", NULL);
	SetEnvironmentVariable("PMI_DOMAIN", NULL);
	if (smpd_process.use_inherited_handles)
	{
	    SetEnvironmentVariable("PMI_SMPD_FD", NULL);
	}
	else
	{
	    SetEnvironmentVariable("PMI_HOST", NULL);
	    SetEnvironmentVariable("PMI_PORT", NULL);
	}
	SetEnvironmentVariable("PMI_SMPD_ID", NULL);
	SetEnvironmentVariable("PMI_SMPD_KEY", NULL);
	SetEnvironmentVariable("PMI_SPAWN", NULL);
    }

    if (bSuccess)
    {
	/* make sock structures out of the sockets */
	/*
	nError = SMPDU_Sock_native_to_sock(set, hIn, NULL, &sock_in);
	if (nError != SMPD_SUCCESS)
	{
	    smpd_err_printf("SMPDU_Sock_native_to_sock failed, error %s\n", get_sock_error_string(nError));
	}
	*/
	/*printf("native sock for stdin\n");fflush(stdout);*/
	nError = SMPDU_Sock_native_to_sock(set, (SMPDU_SOCK_NATIVE_FD)hSockStdinW, NULL, &sock_in);
	if (nError != SMPD_SUCCESS)
	{
	    smpd_err_printf("SMPDU_Sock_native_to_sock failed, error %s\n", get_sock_error_string(nError));
	}
	/*printf("native sock for stdout\n");fflush(stdout);*/
	nError = SMPDU_Sock_native_to_sock(set, (SMPDU_SOCK_NATIVE_FD)hSockStdoutR, NULL, &sock_out);
	if (nError != SMPD_SUCCESS)
	{
	    smpd_err_printf("SMPDU_Sock_native_to_sock failed, error %s\n", get_sock_error_string(nError));
	}
	/*printf("native sock for stderr\n");fflush(stdout);*/
	nError = SMPDU_Sock_native_to_sock(set, (SMPDU_SOCK_NATIVE_FD)hSockStderrR, NULL, &sock_err);
	if (nError != SMPD_SUCCESS)
	{
	    smpd_err_printf("SMPDU_Sock_native_to_sock failed, error %s\n", get_sock_error_string(nError));
	}
	if (process->pmi != NULL && smpd_process.use_inherited_handles)
	{
	    /*printf("native sock for pmi\n");fflush(stdout);*/
	    nError = SMPDU_Sock_native_to_sock(set, (SMPDU_SOCK_NATIVE_FD)hSockPmiR, NULL, &sock_pmi);
	    if (nError != SMPD_SUCCESS)
	    {
		smpd_err_printf("SMPDU_Sock_native_to_sock failed, error %s\n", get_sock_error_string(nError));
	    }
	}

	process->in->sock = sock_in;
	process->out->sock = sock_out;
	process->err->sock = sock_err;
	if (process->pmi != NULL && smpd_process.use_inherited_handles)
	    process->pmi->sock = sock_pmi;
	process->pid = process->in->id = process->out->id = process->err->id = psInfo.dwProcessId;
	SMPDU_Sock_set_user_ptr(sock_in, process->in);
	SMPDU_Sock_set_user_ptr(sock_out, process->out);
	SMPDU_Sock_set_user_ptr(sock_err, process->err);
	if (process->pmi != NULL && smpd_process.use_inherited_handles)
	    SMPDU_Sock_set_user_ptr(sock_pmi, process->pmi);
    }
    else
    {
	/* close all the sockets and handles allocated in this function */
	/*CloseHandle(hIn);*/
	CloseHandle((HANDLE)hSockStdinW);
	CloseHandle((HANDLE)hSockStdoutR);
	CloseHandle((HANDLE)hSockStderrR);
	if (process->pmi != NULL && smpd_process.use_inherited_handles)
	    CloseHandle((HANDLE)hSockPmiR);
    }

RESTORE_CLEANUP:
    /* Restore stdin, stdout, stderr */
    SetStdHandle(STD_INPUT_HANDLE, hStdin);
    SetStdHandle(STD_OUTPUT_HANDLE, hStdout);
    SetStdHandle(STD_ERROR_HANDLE, hStderr);

CLEANUP:
    ReleaseMutex(smpd_process.hLaunchProcessMutex);
    /*CloseHandle((HANDLE)hSockStdinR);*/
    CloseHandle((HANDLE)hPipeStdinR);
    /*
    CloseHandle((HANDLE)hSockStdoutW);
    CloseHandle((HANDLE)hSockStderrW);
    */
    CloseHandle((HANDLE)hPipeStdoutW);
    CloseHandle((HANDLE)hPipeStderrW);
    if (process->pmi != NULL && smpd_process.use_inherited_handles)
	CloseHandle((HANDLE)hSockPmiW);

    if (psInfo.hProcess != INVALID_HANDLE_VALUE)
    {
	HANDLE hThread;
	smpd_piothread_arg_t *arg_ptr;
	smpd_pinthread_arg_t *in_arg_ptr;

	in_arg_ptr = (smpd_pinthread_arg_t*)MPIU_Malloc(sizeof(smpd_pinthread_arg_t));
	in_arg_ptr->hIn = hSockStdinR;
	in_arg_ptr->hOut = hPipeStdinW;
	in_arg_ptr->pid = psInfo.dwProcessId;
	hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)smpd_pinthread, in_arg_ptr, 0, NULL);
	CloseHandle(hThread);
	arg_ptr = (smpd_piothread_arg_t*)MPIU_Malloc(sizeof(smpd_piothread_arg_t));
	arg_ptr->hIn = hOut;
	arg_ptr->hOut = hSockStdoutW;
	arg_ptr->pid = psInfo.dwProcessId;
	hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)smpd_piothread, arg_ptr, 0, NULL);
	CloseHandle(hThread);
	arg_ptr = (smpd_piothread_arg_t*)MPIU_Malloc(sizeof(smpd_piothread_arg_t));
	arg_ptr->hIn = hErr;
	arg_ptr->hOut = hSockStderrW;
	arg_ptr->pid = psInfo.dwProcessId;
	hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)smpd_piothread, arg_ptr, 0, NULL);
	CloseHandle(hThread);

	if (process->pmi != NULL && smpd_process.use_inherited_handles)
	    process->context_refcount = 3;
	else
	    process->context_refcount = 2;
	process->out->read_state = SMPD_READING_STDOUT;
	result = SMPDU_Sock_post_read(sock_out, process->out->read_cmd.cmd, 1, 1, NULL);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("posting first read from stdout context failed, sock error: %s\n",
		get_sock_error_string(result));
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
	process->err->read_state = SMPD_READING_STDERR;
	result = SMPDU_Sock_post_read(sock_err, process->err->read_cmd.cmd, 1, 1, NULL);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("posting first read from stderr context failed, sock error: %s\n",
		get_sock_error_string(result));
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
	if (process->pmi != NULL && smpd_process.use_inherited_handles)
	{
	    result = smpd_post_read_command(process->pmi);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to post a read of the first command on the pmi control channel.\n");
		smpd_exit_fn(FCNAME);
		return SMPD_FAIL;
	    }
	}
	process->wait.hProcess = process->in->wait.hProcess = process->out->wait.hProcess = process->err->wait.hProcess = psInfo.hProcess;
	process->wait.hThread = process->in->wait.hThread = process->out->wait.hThread = process->err->wait.hThread = psInfo.hThread;
	smpd_process_to_registry(process, actual_exe);
	ResumeThread(psInfo.hThread);
	smpd_exit_fn(FCNAME);
	return SMPD_SUCCESS;
    }

    smpd_exit_fn(FCNAME);
    return SMPD_FAIL;
}

#undef FCNAME
#define FCNAME "smpd_parse_account_domain"
void smpd_parse_account_domain(const char *domain_account, char *account, char *domain)
{
    const char *pCh;
    char *pCh2;

    smpd_enter_fn(FCNAME);

    if ((strchr(domain_account, '\\') == NULL) && (strchr(domain_account, '@') != NULL))
    {
	pCh = domain_account;
	pCh2 = account;
	while ((*pCh != '@') && (*pCh != '\0'))
	{
	    *pCh2 = *pCh;
	    pCh++;
	    pCh2++;
	}
	*pCh2 = '\0';
	if (*pCh == '@')
	{
	    pCh++;
	    strcpy(domain, pCh);
	}
	else
	{
	    domain[0] = '\0';
	}
    }
    else
    {
	pCh = domain_account;
	pCh2 = domain;
	while ((*pCh != '\\') && (*pCh != '\0'))
	{
	    *pCh2 = *pCh;
	    pCh++;
	    pCh2++;
	}
	if (*pCh == '\\')
	{
	    pCh++;
	    strcpy(account, pCh);
	    *pCh2 = '\0';
	}
	else
	{
	    strcpy(account, domain_account);
	    domain[0] = '\0';
	}
    }

    smpd_exit_fn(FCNAME);
}

#else

/* Unix code */

/*
static void set_environment_variables(char *bEnv)
{
    char name[1024]="", value[8192]="";
    char *pChar;
    
    pChar = name;
    while (*bEnv != '\0')
    {
	if (*bEnv == '=')
	{
	    *pChar = '\0';
	    pChar = value;
	}
	else
	{
	    if (*bEnv == ';')
	    {
		*pChar = '\0';
		pChar = name;
		smpd_dbg_printf("env: %s=%s\n", name, value);
		setenv(name, value, 1);
	    }
	    else
	    {
		*pChar = *bEnv;
		pChar++;
	    }
	}
	bEnv++;
    }
    *pChar = '\0';
    if (name[0] != '\0')
    {
	smpd_dbg_printf("env: %s=%s\n", name, value);
	setenv(name, value, 1);
    }
}
*/

#ifdef HAVE_SETENV
#undef FCNAME
#define FCNAME "set_environment_variables"
static void set_environment_variables(char *bEnv)
{
    char name[1024], equals[3], value[8192];

    smpd_enter_fn(FCNAME);
    while (1)
    {
	name[0] = '\0';
	equals[0] = '\0';
	value[0] = '\0';
	if (MPIU_Str_get_string(&bEnv, name, 1024) != MPIU_STR_SUCCESS)
	    break;
	if (name[0] == '\0')
	    break;
	if (MPIU_Str_get_string(&bEnv, equals, 3) != MPIU_STR_SUCCESS)
	    break;
	if (equals[0] == '\0')
	    break;
	if (MPIU_Str_get_string(&bEnv, value, 8192) != MPIU_STR_SUCCESS)
	    break;
	setenv(name, value, 1);
    }
    smpd_exit_fn(FCNAME);
}
#else
#undef FCNAME
#define FCNAME "get_env_size"
static int get_env_size(char *bEnv, int *count)
{
    char name[1024], equals[3], value[8192];
    int size = 0;

    smpd_enter_fn(FCNAME);
    while (1)
    {
	name[0] = '\0';
	equals[0] = '\0';
	value[0] = '\0';
	if (MPIU_Str_get_string(&bEnv, name, 1024) != MPIU_STR_SUCCESS)
	    break;
	if (name[0] == '\0')
	    break;
	if (MPIU_Str_get_string(&bEnv, equals, 3) != MPIU_STR_SUCCESS)
	    break;
	if (equals[0] == '\0')
	    break;
	if (MPIU_Str_get_string(&bEnv, value, 8192) != MPIU_STR_SUCCESS)
	    break;
	*count = *count + 1;
	size = size + strlen(name) + strlen(value) + 2; /* length of 'name=value\0' */
    }
    smpd_exit_fn(FCNAME);
    return size;
}

#undef FCNAME
#define FCNAME "add_environment_variables"
static void add_environment_variables(char *str, char **vars, char *bEnv)
{
    char name[1024], equals[3], value[8192];
    int i = 0;

    smpd_enter_fn(FCNAME);
    while (1)
    {
	name[0] = '\0';
	equals[0] = '\0';
	value[0] = '\0';
	if (MPIU_Str_get_string(&bEnv, name, 1024) != MPIU_STR_SUCCESS)
	    break;
	if (name[0] == '\0')
	    break;
	if (MPIU_Str_get_string(&bEnv, equals, 3) != MPIU_STR_SUCCESS)
	    break;
	if (equals[0] == '\0')
	    break;
	if (MPIU_Str_get_string(&bEnv, value, 8192) != MPIU_STR_SUCCESS)
	    break;
	vars[i] = str;
	str += sprintf(str, "%s=%s", name, value) + 1;
	i++;
    }
    smpd_exit_fn(FCNAME);
}
#endif

#ifdef USE_PTHREAD_STDIN_REDIRECTION
static void child_exited(int signo)
{
    pid_t pid;
    int status;
    smpd_process_t *iter;

    /*
     * For some reason on the Macs, stdout and stderr are not closed when the
     * process exits so we can't use the closing of the redirected stdout and
     * stderr sockets as indications that the process has exited.  So this
     * signal handler closes the stdout/err redirection socket on a SIGCHLD
     * signal to simulate that behavior.
     */
    if (signo == SIGCHLD)
    {
	smpd_dbg_printf("SIGCHLD received\n");
	for (;;)
	{
	    status = 0;
	    pid = waitpid(-1, &status, WNOHANG);
	    if (pid <= 0)
		break;

	    smpd_dbg_printf("SIGCHILD pid = %d\n", pid);
	    iter = smpd_process.process_list;
	    {
		while (iter != NULL)
		{
		    if (iter->wait == pid)
		    {
			if (WIFEXITED(status))
			{
			    iter->exitcode =  WEXITSTATUS(status);
			}
			else
			{
			    iter->exitcode = -123;
			}
			if (iter->out != NULL)
			{
			    if (iter->out->read_state == SMPD_READING_STDOUT)
			    {
				smpd_dbg_printf("closing stdout redirection\n");
				iter->out->state = SMPD_CLOSING;
				SMPDU_Sock_post_close(iter->out->sock);
			    }
			    else
			    {
				smpd_err_printf("iter->out->read_state = %d\n", iter->out->read_state);
			    }
			}
			if (iter->err != NULL)
			{
			    if (iter->err->read_state == SMPD_READING_STDERR)
			    {
				smpd_dbg_printf("closing stderr redirection\n");
				iter->err->state = SMPD_CLOSING;
				SMPDU_Sock_post_close(iter->err->sock);
			    }
			    else
			    {
				smpd_err_printf("iter->err->read_state = %d\n", iter->err->read_state);
			    }
			}
			break;
		    }
		    iter = iter->next;
		}
	    }
	}
    }
    else
    {
	smpd_dbg_printf("unexpected signal %d received\n", signo);
    }
}
#endif

#undef FCNAME
#define FCNAME "smpd_launch_process"
int smpd_launch_process(smpd_process_t *process, int priorityClass, int priority, int dbg, SMPDU_Sock_set_t set)
{
    int result;
    int stdin_pipe_fds[2], stdout_pipe_fds[2];
    int  stderr_pipe_fds[2], pmi_pipe_fds[2];
    int pid;
    SMPDU_Sock_t sock_in, sock_out, sock_err, sock_pmi;
    char args[SMPD_MAX_EXE_LENGTH];
    char *argv[1024];
    char *token;
    int i;
    char str[1024];
    char *str_iter;
    int total, num_chars;
    char *actual_exe, exe_data[SMPD_MAX_EXE_LENGTH];
    char *temp_str;
    char temp_exe[SMPD_MAX_EXE_LENGTH];
    smpd_command_t *cmd_ptr;
    static char *pPutEnv = NULL;
    char *pLastEnv, *env_iter;
    int env_count, env_size;
    char **pEnvArray;

    smpd_enter_fn(FCNAME);

#ifdef USE_PTHREAD_STDIN_REDIRECTION
    {
	/* On the Macs we must use a signal handler to determine when a process
	 * has exited.  On all other systems we use the closing of stdout and
	 * stderr to determine that a process has exited.
	 */
	static int sighandler_setup = 0;
	if (!sighandler_setup)
	{
	    smpd_dbg_printf("setting child_exited signal handler\n");
	    smpd_signal(SIGCHLD, child_exited);
	    sighandler_setup = 1;
	}
    }
#endif

    /* resolve the executable name */
    if (process->path[0] != '\0')
    {
	temp_str = process->exe;
	result = MPIU_Str_get_string(&temp_str, temp_exe, SMPD_MAX_EXE_LENGTH);
	if (result != MPIU_STR_SUCCESS)
	{
	}
	smpd_dbg_printf("searching for '%s' in '%s'\n", temp_exe, process->path);
	if (smpd_search_path(process->path, temp_exe, SMPD_MAX_EXE_LENGTH, exe_data))
	{
	    smpd_dbg_printf("found: '%s'\n", exe_data);
	    if (strstr(exe_data, " "))
	    {
		smpd_err_printf("Currently unable to handle paths with spaces in them.\n");
	    }
	    if (temp_str != NULL)
	    {
		strncat(exe_data, " ", SMPD_MAX_EXE_LENGTH - strlen(exe_data));
		strncat(exe_data, temp_str, SMPD_MAX_EXE_LENGTH - strlen(exe_data));
		exe_data[SMPD_MAX_EXE_LENGTH-1] = '\0';
	    }
	    actual_exe = exe_data;
	}
	else
	{
	    actual_exe = process->exe;
	}
    }
    else
    {
	actual_exe = process->exe;
    }
    
    /* create argv from the command */
    i = 0;
    total = 0;
    /*str_iter = process->exe;*/
    str_iter = actual_exe;
    while (str_iter && i<1024)
    {
	result = MPIU_Str_get_string(&str_iter, &args[total],
				   SMPD_MAX_EXE_LENGTH - total);
	argv[i] = &args[total];
	i++;
	total += strlen(&args[total])+1; /* move over the null termination */
    }
    argv[i] = NULL;

    /* create pipes for redirecting I/O */
    /*
    pipe(stdin_pipe_fds);
    pipe(stdout_pipe_fds);
    pipe(stderr_pipe_fds);
    */
    socketpair(AF_UNIX, SOCK_STREAM, 0, stdin_pipe_fds);
    socketpair(AF_UNIX, SOCK_STREAM, 0, stdout_pipe_fds);
    socketpair(AF_UNIX, SOCK_STREAM, 0, stderr_pipe_fds);
    if (process->pmi != NULL)
    {
	socketpair(AF_UNIX, SOCK_STREAM, 0, pmi_pipe_fds);
    }

    pid = fork();
    if (pid < 0)
    {
	smpd_err_printf("fork failed - error %d.\n", errno);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    if (pid == 0)
    {
	/* child process */
	smpd_dbg_printf("client is alive and about to exec '%s'\n", argv[0]);

#ifdef HAVE_SETENV
	if (process->pmi != NULL)
	{
	    sprintf(str, "%d", process->rank);
	    smpd_dbg_printf("env: PMI_RANK=%s\n", str);
	    setenv("PMI_RANK", str, 1);
	    sprintf(str, "%d", process->nproc);
	    smpd_dbg_printf("env: PMI_SIZE=%s\n", str);
	    setenv("PMI_SIZE", str, 1);
	    sprintf(str, "%s", process->kvs_name);
	    smpd_dbg_printf("env: PMI_KVS=%s\n", str);
	    setenv("PMI_KVS", str, 1);
	    sprintf(str, "%s", process->domain_name);
	    smpd_dbg_printf("env: PMI_DOMAIN=%s\n", str);
	    setenv("PMI_DOMAIN", str, 1);
	    sprintf(str, "%d", pmi_pipe_fds[1]);
	    smpd_dbg_printf("env: PMI_SMPD_FD=%s\n", str);
	    setenv("PMI_SMPD_FD", str, 1);
	    sprintf(str, "%d", smpd_process.id);
	    smpd_dbg_printf("env: PMI_SMPD_ID=%s\n", str);
	    setenv("PMI_SMPD_ID", str, 1);
	    sprintf(str, "%d", process->id);
	    smpd_dbg_printf("env: PMI_SMPD_KEY=%s\n", str);
	    setenv("PMI_SMPD_KEY", str, 1);
	    sprintf(str, "%d", process->spawned);
	    smpd_dbg_printf("env: PMI_SPAWN=%s\n", str);
	    setenv("PMI_SPAWN", str, 1);
	    sprintf(str, "%d", process->appnum);
	    smpd_dbg_printf("env: PMI_APPNUM=%s\n", str);
	    setenv("PMI_APPNUM", str, 1);
	    sprintf(str, "%s", process->clique);
	    smpd_dbg_printf("env: PMI_CLIQUE=%s\n", str);
	    setenv("PMI_CLIQUE", str, 1);
	}
	set_environment_variables(process->env);
#else
	pLastEnv = pPutEnv;
	env_count = 0;
	env_size = get_env_size(process->env, &env_count) + 1024;
	env_count += 10;
	pPutEnv = (char*)MPIU_Malloc(env_size * sizeof(char));
	pEnvArray = (char**)MPIU_Malloc(env_count * sizeof(char*));
	env_iter = pPutEnv;
	pEnvArray[0] = env_iter;
	env_iter += sprintf(env_iter, "PMI_RANK=%d", process->rank) + 1;
	pEnvArray[1] = env_iter;
	env_iter += sprintf(env_iter, "PMI_SIZE=%d", process->nproc) + 1;
	pEnvArray[2] = env_iter;
	env_iter += sprintf(env_iter, "PMI_KVS=%s", process->kvs_name) + 1;
	pEnvArray[3] = env_iter;
	env_iter += sprintf(env_iter, "PMI_DOMAIN=%s", process->domain_name) + 1;
	pEnvArray[4] = env_iter;
	env_iter += sprintf(env_iter, "PMI_SMPD_FD=%d", pmi_pipe_fds[1]) + 1;
	pEnvArray[5] = env_iter;
	env_iter += sprintf(env_iter, "PMI_SMPD_ID=%d", smpd_process.id) + 1;
	pEnvArray[6] = env_iter;
	env_iter += sprintf(env_iter, "PMI_SMPD_KEY=%d", process->id) + 1;
	pEnvArray[7] = env_iter;
	env_iter += sprintf(env_iter, "PMI_SPAWN=%d", process->spawned) + 1;
	pEnvArray[8] = env_iter;
	env_iter += sprintf(env_iter, "PMI_APPNUM=%d", process->appnum) + 1;
	pEnvArray[9] = env_iter;
	env_iter += sprintf(env_iter, "PMI_CLIQUE=%s", process->clique) + 1;
	add_environment_variables(env_iter, &pEnvArray[10], process->env);
	for (i=0; i<env_count; i++)
	{
	    result = putenv(pEnvArray[i]);
	    if (result != 0)
	    {
		smpd_err_printf("putenv failed: %d\n", errno);
	    }
	}
	if (pLastEnv != NULL)
	    MPIU_Free(pLastEnv);
#endif
	result = dup2(stdin_pipe_fds[0], 0);   /* dup a new stdin */
	if (result == -1)
	{
	    smpd_err_printf("dup2 stdin failed: %d\n", errno);
	}
	close(stdin_pipe_fds[0]);
	close(stdin_pipe_fds[1]);

	result = dup2(stdout_pipe_fds[1], 1);  /* dup a new stdout */
	if (result == -1)
	{
	    smpd_err_printf("dup2 stdout failed: %d\n", errno);
	}
	close(stdout_pipe_fds[0]);
	close(stdout_pipe_fds[1]);

	result = dup2(stderr_pipe_fds[1], 2);  /* dup a new stderr */
	if (result == -1)
	{
	    smpd_err_printf("dup2 stderr failed: %d\n", errno);
	}
	close(stderr_pipe_fds[0]);
	close(stderr_pipe_fds[1]);

	if (process->pmi != NULL)
	{
	    close(pmi_pipe_fds[0]); /* close the other end */
	}

	/* change the working directory */
	result = -1;
	if (process->dir[0] != '\0')
	    result = chdir( process->dir );
	if (result < 0)
	    chdir( getenv( "HOME" ) );

	/* reset the file mode creation mask */
	umask(0);

	result = execvp( argv[0], argv );

	result = errno;
	{
	    char myhostname[SMPD_MAX_HOST_LENGTH];
	    smpd_get_hostname(myhostname, SMPD_MAX_HOST_LENGTH);
	    snprintf(process->err_msg, SMPD_MAX_ERROR_LEN, "Unable to exec '%s' on %s.  Error %d - %s\n", process->exe, myhostname, result, strerror(result));
	    /*sprintf(process->err_msg, "Error %d - %s", result, strerror(result));*/
	}

	if (process->pmi != NULL)
	{
	    /* create the abort command */
	    result = smpd_create_command("abort", smpd_process.id, 0, SMPD_FALSE, &cmd_ptr);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to create an abort command in response to failed launch command: '%s'\n", process->exe);
		exit(-1);
	    }
	    /* launch process should provide a reason for the error, for now just return FAIL */
	    result = smpd_add_command_arg(cmd_ptr, "result", SMPD_FAIL_STR);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to add the result field to the result command in response to launch command: '%s'\n", process->exe);
		exit(-1);
	    }
	    if (process->err_msg[0] != '\0')
	    {
		result = smpd_add_command_arg(cmd_ptr, "error", process->err_msg);
		if (result != SMPD_SUCCESS)
		{
		    smpd_err_printf("unable to add the error field to the abort command in response to failed launch command: '%s'\n", process->exe);
		    exit(-1);
		}
	    }

	    /* send the result back */
	    smpd_package_command(cmd_ptr);
	    result = write(pmi_pipe_fds[1], cmd_ptr->cmd_hdr_str, SMPD_CMD_HDR_LENGTH);
	    if (result != SMPD_CMD_HDR_LENGTH)
	    {
		smpd_err_printf("unable to write the abort command header in response to failed launch command: '%s'\n", process->exe);
		exit(-1);
	    }
	    result = write(pmi_pipe_fds[1], cmd_ptr->cmd, cmd_ptr->length);
	    if (result != cmd_ptr->length)
	    {
		smpd_err_printf("unable to write the abort command in response to failed launch command: '%s'\n", process->exe);
		exit(-1);
	    }

	    /* send a closed message on the pmi socket? */
	}

	exit(result);
    }

    /* parent process */
    process->pid = pid;
    close(stdin_pipe_fds[0]);
    close(stdout_pipe_fds[1]);
    close(stderr_pipe_fds[1]);
    if (process->pmi != NULL)
    {
	close(pmi_pipe_fds[1]);
    }

    /* make sock structures out of the sockets */
    result = SMPDU_Sock_native_to_sock(set, stdin_pipe_fds[1], NULL, &sock_in);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("SMPDU_Sock_native_to_sock failed, error %s\n", get_sock_error_string(result));
    }
    result = SMPDU_Sock_native_to_sock(set, stdout_pipe_fds[0], NULL, &sock_out);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("SMPDU_Sock_native_to_sock failed, error %s\n", get_sock_error_string(result));
    }
    result = SMPDU_Sock_native_to_sock(set, stderr_pipe_fds[0], NULL, &sock_err);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("SMPDU_Sock_native_to_sock failed, error %s\n", get_sock_error_string(result));
    }
    if (process->pmi != NULL)
    {
	result = SMPDU_Sock_native_to_sock(set, pmi_pipe_fds[0], NULL, &sock_pmi);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("SMPDU_Sock_native_to_sock failed, error %s\n", get_sock_error_string(result));
	}
    }
    process->in->sock = sock_in;
    process->out->sock = sock_out;
    process->err->sock = sock_err;
    if (process->pmi != NULL)
    {
	process->pmi->sock = sock_pmi;
    }
    process->pid = process->in->id = process->out->id = process->err->id = pid;
    result = SMPDU_Sock_set_user_ptr(sock_in, process->in);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("SMPDU_Sock_set_user_ptr failed, error %s\n", get_sock_error_string(result));
    }
    result = SMPDU_Sock_set_user_ptr(sock_out, process->out);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("SMPDU_Sock_set_user_ptr failed, error %s\n", get_sock_error_string(result));
    }
    result = SMPDU_Sock_set_user_ptr(sock_err, process->err);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("SMPDU_Sock_set_user_ptr failed, error %s\n", get_sock_error_string(result));
    }
    if (process->pmi != NULL)
    {
	result = SMPDU_Sock_set_user_ptr(sock_pmi, process->pmi);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("SMPDU_Sock_set_user_ptr failed, error %s\n", get_sock_error_string(result));
	}
    }

    process->context_refcount = (process->pmi != NULL) ? 3 : 2;
    process->out->read_state = SMPD_READING_STDOUT;
    result = SMPDU_Sock_post_read(sock_out, process->out->read_cmd.cmd, 1, 1, NULL);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("posting first read from stdout context failed, sock error: %s\n",
	    get_sock_error_string(result));
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    process->err->read_state = SMPD_READING_STDERR;
    result = SMPDU_Sock_post_read(sock_err, process->err->read_cmd.cmd, 1, 1, NULL);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("posting first read from stderr context failed, sock error: %s\n",
	    get_sock_error_string(result));
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    if (process->pmi != NULL)
    {
	result = smpd_post_read_command(process->pmi);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to post a read of the first command on the pmi control context.\n");
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
    }
    process->wait = process->in->wait = process->out->wait = process->err->wait = pid;

    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#endif

#undef FCNAME
#define FCNAME "smpd_wait_process"
int smpd_wait_process(smpd_pwait_t wait, int *exit_code_ptr)
{
#ifdef HAVE_WINDOWS_H
    int result;
    DWORD exit_code;

    smpd_enter_fn(FCNAME);

    if (wait.hProcess == INVALID_HANDLE_VALUE || wait.hProcess == NULL)
    {
	smpd_dbg_printf("No process to wait for.\n");
	*exit_code_ptr = -1;
	smpd_exit_fn(FCNAME);
	return SMPD_SUCCESS;
    }
    if (WaitForSingleObject(wait.hProcess, INFINITE) != WAIT_OBJECT_0)
    {
	smpd_err_printf("WaitForSingleObject failed, error %d\n", GetLastError());
	*exit_code_ptr = -1;
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    result = GetExitCodeProcess(wait.hProcess, &exit_code);
    if (!result)
    {
	smpd_err_printf("GetExitCodeProcess failed, error %d\n", GetLastError());
	*exit_code_ptr = -1;
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    CloseHandle(wait.hProcess);
    CloseHandle(wait.hThread);

    *exit_code_ptr = exit_code;

    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
#else
    int status;
    smpd_pwait_t result;
    smpd_enter_fn(FCNAME);

    smpd_dbg_printf("waiting for process %d\n", wait);
    result = -1;
    while (result == -1)
    {
	result = waitpid(wait, &status, WUNTRACED);
	if (result == -1)
	{
	    switch (errno)
	    {
	    case EINTR:
		break;
	    case ECHILD:
#ifdef USE_PTHREAD_STDIN_REDIRECTION
		/* On the Macs where stdout/err redirection hangs a SIGCHLD 
		 * handler has been set up so ignore ECHILD errors.
		 */
#else
		smpd_err_printf("waitpid(%d) returned ECHILD\n", wait);
		*exit_code_ptr = -10;
#endif
		smpd_exit_fn(FCNAME);
		return SMPD_SUCCESS;
		break;
	    case EINVAL:
		smpd_err_printf("waitpid(%d) returned EINVAL\n", wait);
		*exit_code_ptr = -11;
		smpd_exit_fn(FCNAME);
		return SMPD_SUCCESS;
		break;
	    default:
		smpd_err_printf("waitpid(%d) returned %d\n", wait, errno);
		*exit_code_ptr = -12;
		smpd_exit_fn(FCNAME);
		return SMPD_SUCCESS;
		break;
	    }
	}
    }
    if (WIFEXITED(status))
    {
	*exit_code_ptr =  WEXITSTATUS(status);
    }
    else
    {
	smpd_err_printf("WIFEXITED(%d) failed, setting exit code to -1\n", wait);
	*exit_code_ptr = -1;
	if (WIFSIGNALED(status))
	{
	    *exit_code_ptr = -2;
	}
	if (WIFSTOPPED(status))
	{
	    *exit_code_ptr = -3;
	}
#ifdef WCOREDUMP
	if (WCOREDUMP(status))
	{
	    *exit_code_ptr = -4;
	}
#endif
    }

    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
#endif
}

#define SMPD_MAX_SUSPEND_RETRY_COUNT 4

#undef FCNAME
#define FCNAME "smpd_suspend_process"
int smpd_suspend_process(smpd_process_t *process)
{
#ifdef HAVE_WINDOWS_H
    int result = SMPD_SUCCESS;
    int retry_cnt = 0;
    smpd_enter_fn(FCNAME);

    do{
        if (SuspendThread(process->wait.hThread) == -1){
            int exit_code;

            /* Check if the thread is still active */
            if(!GetExitCodeThread(process->wait.hThread, &exit_code)){
                smpd_err_printf("Getting exit code for thread failed\n");
                break;
            }
            else{
                if(exit_code != STILL_ACTIVE){
                    smpd_err_printf("The thread to be suspended is no longer active, exit_code = %d\n", exit_code);
                    break;
                }
                else{
                    smpd_err_printf("The thread is active but cannot be suspended\n");
                }
            }

	        result = GetLastError();
	        smpd_err_printf("SuspendThread failed[%d times] with error %d for process %d:%s:'%s'\n",
	            retry_cnt, result, process->rank, process->kvs_name, process->exe);
        }
        else{
            break;
        }

        /* Ignore error and proceed if we fail to suspend */
        result = SMPD_SUCCESS;
        retry_cnt++;
    }while(retry_cnt < SMPD_MAX_SUSPEND_RETRY_COUNT);

    smpd_exit_fn(FCNAME);
    /* Ignore error */
    return SMPD_SUCCESS;
#else
    smpd_enter_fn(FCNAME);

    smpd_dbg_printf("stopping process %d\n", process->wait);
    kill(process->wait, SIGSTOP);

    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
#endif
}

#ifdef HAVE_WINDOWS_H
static BOOL SafeTerminateProcess(HANDLE hProcess, UINT uExitCode)
{
    DWORD dwTID, dwCode, dwErr = 0;
    HANDLE hProcessDup = INVALID_HANDLE_VALUE;
    HANDLE hRT = NULL;
    HINSTANCE hKernel = GetModuleHandle("Kernel32");
    BOOL bSuccess = FALSE;

    BOOL bDup = DuplicateHandle(GetCurrentProcess(),
	hProcess,
	GetCurrentProcess(),
	&hProcessDup,
	PROCESS_ALL_ACCESS,
	FALSE,
	0);

    if (GetExitCodeProcess((bDup) ? hProcessDup : hProcess, &dwCode) &&
	(dwCode == STILL_ACTIVE))
    {
	FARPROC pfnExitProc;

	pfnExitProc = GetProcAddress(hKernel, "ExitProcess");

	if (pfnExitProc)
	{
	    hRT = CreateRemoteThread((bDup) ? hProcessDup : hProcess,
		NULL,
		0,
		/*
		This relies on the probability that Kernel32.dll is mapped to the same place on all processes
		If it gets relocated, this function will produce spurious results
		*/
		(LPTHREAD_START_ROUTINE)pfnExitProc,
		UintToPtr(uExitCode)/*(LPVOID)uExitCode*/, 0, &dwTID);
	}
	
	if (hRT == NULL)
	    dwErr = GetLastError();
    }
    else
    {
	dwErr = ERROR_PROCESS_ABORTED;
    }

    if (hRT)
    {
	if (WaitForSingleObject((bDup) ? hProcessDup : hProcess, 30000) == WAIT_OBJECT_0)
	    bSuccess = TRUE;
	else
	{
	    dwErr = ERROR_TIMEOUT;
	    bSuccess = FALSE;
	}
	CloseHandle(hRT);
    }

    if (bDup)
	CloseHandle(hProcessDup);

    if (!bSuccess)
	SetLastError(dwErr);

    return bSuccess;
}
#endif

#undef FCNAME
#define FCNAME "smpd_kill_process"
int smpd_kill_process(smpd_process_t *process, int exit_code)
{
    int result = SMPD_SUCCESS;
#ifdef HAVE_WINDOWS_H
    smpd_enter_fn(FCNAME);

    smpd_process_from_registry(process);
    if (!SafeTerminateProcess(process->wait.hProcess, exit_code)){
        smpd_err_printf("unable terminate process safely. exit_code = %d\n", exit_code);
	    if (GetLastError() != ERROR_PROCESS_ABORTED){
            if(!TerminateProcess(process->wait.hProcess, exit_code)){
                if (GetLastError() != ERROR_PROCESS_ABORTED){
                    result = SMPD_FAIL;
                }
            }
	    }
    }
    smpd_exit_fn(FCNAME);
    return result;
#else
    int status;
    smpd_enter_fn(FCNAME);

    smpd_dbg_printf("killing process %d\n", process->wait);
    if(kill(process->wait, /*SIGTERM*/SIGKILL) == -1){
        smpd_exit_fn(FCNAME);
        return SMPD_FAIL;
    }
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
#endif
}

#undef FCNAME
#define FCNAME "smpd_kill_all_processes"
int smpd_kill_all_processes(void)
{
    smpd_process_t *iter;

    smpd_enter_fn(FCNAME);

    if (smpd_process.local_root)
    {
	/* the mpiexec process should not kill the smpd manager that it created for the -localroot option */
	smpd_exit_fn(FCNAME);
	return SMPD_SUCCESS;
    }

    if (smpd_process.rsh_mpiexec)
    {
	int i;
	int count = 0;
	iter = smpd_process.process_list;
	while (iter)
	{
	    count++;
	    iter = iter->next;
	}
	if (count > 0)
	{
	    smpd_pwait_t *wait_array;
	    wait_array = (smpd_pwait_t*)MPIU_Malloc(sizeof(smpd_pwait_t) * count);
	    for (i=0, iter = smpd_process.process_list; i<count; i++)
	    {
		wait_array[i] = iter->wait;
		iter = iter->next;
	    }
	    for (i=0; i<count; i++)
	    {
#ifdef HAVE_WINDOWS_H
		if (!SafeTerminateProcess(wait_array[i].hProcess, 123))
		{
		    if (GetLastError() != ERROR_PROCESS_ABORTED)
		    {
			TerminateProcess(wait_array[i].hProcess, 255);
		    }
		}
#else
		kill(wait_array[i], /*SIGTERM*/SIGKILL);
#endif
	    }
	}
    }
    else
    {
	iter = smpd_process.process_list;
	while (iter)
	{
#ifdef HAVE_WINDOWS_H
	    /*DWORD dwProcessId;*/
	    smpd_process_from_registry(iter);
	    /* For some reason break signals don't work on processes created by smpd_launch_process
	    printf("ctrl-c process: %s\n", iter->exe);fflush(stdout);
	    dwProcessId = GetProcessId(iter->wait.hProcess);
	    GenerateConsoleCtrlEvent(CTRL_C_EVENT, dwProcessId);
	    if (WaitForSingleObject(iter->wait.hProcess, 1000) != WAIT_OBJECT_0)
	    {
	    printf("breaking process: %s\n", iter->exe);fflush(stdout);
	    GenerateConsoleCtrlEvent(CTRL_BREAK_EVENT, dwProcessId);
	    if (WaitForSingleObject(iter->wait.hProcess, 1000) != WAIT_OBJECT_0)
	    {
	    */
	    if (!SafeTerminateProcess(iter->wait.hProcess, 123))
	    {
		if (GetLastError() != ERROR_PROCESS_ABORTED)
		{
		    TerminateProcess(iter->wait.hProcess, 255);
		}
	    }
	    /*
	    }
	    }
	    */
#else
	    kill(iter->wait, /*SIGTERM*/SIGKILL);
#endif
	    iter = iter->next;
	}
    }

    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_exit"
int smpd_exit(int exitcode)
{
    smpd_enter_fn(FCNAME);
    smpd_kill_all_processes();
#ifdef HAVE_WINDOWS_H
    smpd_finalize_drive_maps();
#endif
    smpd_finalize_printf();
    PMPI_Finalize();
    /* If we're exiting due to a user abort, use the exit code supplied by the abort call */
    if (smpd_process.use_abort_exit_code)
	exitcode = smpd_process.abort_exit_code;
#ifdef HAVE_WINDOWS_H
    if (smpd_process.hCloseStdinThreadEvent)
    {
	CloseHandle(smpd_process.hCloseStdinThreadEvent);
    }
    /* This is necessary because exit() can deadlock flushing file buffers while the stdin thread is running */
    ExitProcess(exitcode);
#else
    exit(exitcode);
    smpd_exit_fn(FCNAME);
    return SMPD_FAIL;
#endif
}
