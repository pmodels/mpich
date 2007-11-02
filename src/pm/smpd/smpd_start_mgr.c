/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "smpd.h"
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

#ifdef HAVE_WINDOWS_H

char *smpd_encode_handle(char *str, HANDLE h)
{
    sprintf(str, "%p", h);
    return str;
}

HANDLE smpd_decode_handle(char *str)
{
    HANDLE p;
    sscanf(str, "%p", &p);
    return p;
}

#undef FCNAME
#define FCNAME "smpd_start_win_mgr"
int smpd_start_win_mgr(smpd_context_t *context, SMPD_BOOL use_context_user_handle)
{
    int result;
    char dbg_str[20];
    char read_handle_str[20], write_handle_str[20];
    char domainaccount[SMPD_MAX_ACCOUNT_LENGTH], account[SMPD_MAX_ACCOUNT_LENGTH], domain[SMPD_MAX_ACCOUNT_LENGTH];
    char *pszDomain;
    char password[SMPD_MAX_PASSWORD_LENGTH];
    HANDLE user_handle = INVALID_HANDLE_VALUE;
    HANDLE job = INVALID_HANDLE_VALUE;
    int num_tries;
    char cmd[8192];
    PROCESS_INFORMATION pInfo;
    STARTUPINFO sInfo;
    SECURITY_ATTRIBUTES security;
    HANDLE hRead, hWrite, hReadRemote, hWriteRemote;
    DWORD num_read, num_written;

    smpd_enter_fn(FCNAME);

    /* initialize to null so in the sspi case garbage isn't sent to the manager */
    domainaccount[0] = '\0';
    account[0] = '\0';
    domain[0] = '\0';
    password[0] = '\0';

    /* start the manager */
    security.bInheritHandle = TRUE;
    security.lpSecurityDescriptor = NULL;
    security.nLength = sizeof(security);
    /* create a pipe to send the listening port information through */
    if (!CreatePipe(&hRead, &hWriteRemote, &security, 0))
    {
	smpd_err_printf("CreatePipe failed, error %d\n", GetLastError());
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    /* prevent the local read end of the pipe from being inherited */
    if (!DuplicateHandle(GetCurrentProcess(), hRead, GetCurrentProcess(), &hRead, 0, FALSE, DUPLICATE_CLOSE_SOURCE | DUPLICATE_SAME_ACCESS))
    {
	smpd_err_printf("Unable to duplicate the read end of the pipe, error %d\n", GetLastError());
	CloseHandle(hRead);
	CloseHandle(hWriteRemote);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    /* create a pipe to send the account information through */
    if (!CreatePipe(&hReadRemote, &hWrite, &security, 0))
    {
	smpd_err_printf("CreatePipe failed, error %d\n", GetLastError());
	CloseHandle(hRead);
	CloseHandle(hWriteRemote);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    /* prevent the local write end of the pipe from being inherited */
    if (!DuplicateHandle(GetCurrentProcess(), hWrite, GetCurrentProcess(), &hWrite, 0, FALSE, DUPLICATE_CLOSE_SOURCE | DUPLICATE_SAME_ACCESS))
    {
	smpd_err_printf("Unable to duplicate the read end of the pipe, error %d\n", GetLastError());
	CloseHandle(hRead);
	CloseHandle(hWrite);
	CloseHandle(hReadRemote);
	CloseHandle(hWriteRemote);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    /* encode the command line */
    if (smpd_process.dbg_state == SMPD_DBG_STATE_ALL)
	strcpy(dbg_str, "-d");
    else if (smpd_process.dbg_state)
	snprintf(dbg_str, 20, "-d %d", smpd_process.dbg_state);
    else
	dbg_str[0] = '\0';
    if (smpd_process.port != SMPD_LISTENER_PORT)
    {
	snprintf(cmd, 8192, "\"%s\" -p %d %s -mgr -read %s -write %s", smpd_process.pszExe, smpd_process.port, dbg_str,
	    smpd_encode_handle(read_handle_str, hReadRemote), 
	    smpd_encode_handle(write_handle_str, hWriteRemote));
    }
    else
    {
	snprintf(cmd, 8192, "\"%s\" %s -mgr -read %s -write %s", smpd_process.pszExe, dbg_str,
	    smpd_encode_handle(read_handle_str, hReadRemote), 
	    smpd_encode_handle(write_handle_str, hWriteRemote));
    }
    if (context->connect_to)
    {
	smpd_dbg_printf("starting command:%d: %s\n", context->connect_to->id, cmd);
    }
    else
    {
        smpd_dbg_printf("starting command: %s\n", cmd);
    }
    GetStartupInfo(&sInfo);
    if (smpd_process.bService)
    {
	if (use_context_user_handle)
	{
	    if (context->sspi_context == NULL)
	    {
		smpd_err_printf("use_context_user_handle set to TRUE but sspi_context == NULL\n");
		CloseHandle(hRead);
		CloseHandle(hWrite);
		CloseHandle(hReadRemote);
		CloseHandle(hWriteRemote);
		smpd_exit_fn(FCNAME);
		return SMPD_ERR_INVALID_USER;
	    }
	    user_handle = context->sspi_context->user_handle;
	    job = context->sspi_context->job;
	}
	else
	{
	    strcpy(domainaccount, context->account);
	    strcpy(password, context->password);
	    smpd_parse_account_domain(domainaccount, account, domain);
	    if (strlen(domain) < 1)
		pszDomain = NULL;
	    else
		pszDomain = domain;

	    result = smpd_get_user_handle(account, pszDomain, password, &user_handle);
	    if (user_handle == INVALID_HANDLE_VALUE)
	    {
		smpd_err_printf("smpd_get_user_handle failed, error %d.\n", result);
		CloseHandle(hRead);
		CloseHandle(hWrite);
		CloseHandle(hReadRemote);
		CloseHandle(hWriteRemote);
		smpd_exit_fn(FCNAME);
		return SMPD_ERR_INVALID_USER;
	    }
	}

	result = SMPD_SUCCESS;
	if (!ImpersonateLoggedOnUser(user_handle))
	{
	    result = GetLastError();
	    smpd_err_printf("ImpersonateLoggedOnUser failed, error %d\n", result);
	    CloseHandle(hRead);
	    CloseHandle(hWrite);
	    CloseHandle(hReadRemote);
	    CloseHandle(hWriteRemote);
	    smpd_exit_fn(FCNAME);
	    return SMPD_ERR_INVALID_USER;
	}
    }

    num_tries = 4;
    do
    {
	if (smpd_process.bService)
	{
	    smpd_dbg_printf("CreateProcessAsUser\n");
	    result = CreateProcessAsUser(
		user_handle,
		NULL, cmd, NULL, NULL, TRUE,
		CREATE_SUSPENDED,
		NULL, NULL, &sInfo, &pInfo);
	}
	else
	{
	    smpd_dbg_printf("CreateProcess\n");
	    result = CreateProcess(
		NULL, cmd, NULL, NULL, TRUE,
		CREATE_SUSPENDED,
		NULL, NULL, &sInfo, &pInfo);
	}

	if (result)
	{
	    result = SMPD_SUCCESS;
	    num_tries = 0;
	}
	else
	{
	    result = GetLastError();
	    if (result == ERROR_REQ_NOT_ACCEP)
	    {
		Sleep(1000);
		num_tries--;
		if (num_tries == 0)
		{
		    smpd_err_printf("%s failed, error %d - ERROR_REQ_NOT_ACCEP\n", smpd_process.bService ? "CreateProcessAsUser" : "CreateProcess", result);
		}
	    }
	    else
	    {
		smpd_err_printf("%s failed, error %d\n", smpd_process.bService ? "CreateProcessAsUser" : "CreateProcess", result);
		num_tries = 0;
	    }
	}
    } while (num_tries);

    if (job != INVALID_HANDLE_VALUE)
    {
	smpd_dbg_printf("assinging smpd manager to job\n");
	if (!AssignProcessToJobObject(job, pInfo.hProcess))
	{
	    smpd_err_printf("AssignProcessToJobObject failed: %d\n", GetLastError());
	    TerminateProcess(pInfo.hProcess, (UINT)-1);
	    result = SMPD_FAIL;
	}
	else
	{
	    ResumeThread(pInfo.hThread);
	}
    }
    else
    {
	ResumeThread(pInfo.hThread);
    }

    if (smpd_process.bService)
    {
	RevertToSelf();
	if (use_context_user_handle && context->sspi_context != NULL)
	{
	    if (context->sspi_context->close_handle)
	    {
		CloseHandle(context->sspi_context->user_handle);
		context->sspi_context->user_handle = INVALID_HANDLE_VALUE;
	    }
	}
	else
	{
	    CloseHandle(user_handle);
	}
    }
    if (result != SMPD_SUCCESS)
    {
	CloseHandle(hRead);
	CloseHandle(hWrite);
	CloseHandle(hReadRemote);
	CloseHandle(hWriteRemote);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    CloseHandle(pInfo.hThread);
    CloseHandle(pInfo.hProcess);
    CloseHandle(hReadRemote);
    CloseHandle(hWriteRemote);

    smpd_dbg_printf("smpd reading the port string from the manager\n");
    /* read the listener port from the pipe to the manager */
    if (!ReadFile(hRead, context->port_str, 20, &num_read, NULL))
    {
	smpd_err_printf("ReadFile() failed, error %d\n", GetLastError());
	CloseHandle(hRead);
	CloseHandle(hWrite);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    CloseHandle(hRead);
    if (num_read != 20)
    {
	smpd_err_printf("parital port string read, %d bytes of 20\n", num_read);
	CloseHandle(hWrite);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    /* send the account and password to the manager */
    smpd_dbg_printf("smpd sending the account to the manager\n");
    if (!WriteFile(hWrite, domainaccount, SMPD_MAX_ACCOUNT_LENGTH, &num_written, NULL))
    {
	smpd_err_printf("WriteFile('%s') failed to write the account, error %d\n", domainaccount, GetLastError());
	CloseHandle(hWrite);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    if (num_written != SMPD_MAX_ACCOUNT_LENGTH)
    {
	smpd_err_printf("parital account string written, %d bytes of %d\n", num_written, SMPD_MAX_ACCOUNT_LENGTH);
	CloseHandle(hWrite);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    smpd_dbg_printf("smpd sending the password to the manager\n");
    if (!WriteFile(hWrite, password, SMPD_MAX_PASSWORD_LENGTH, &num_written, NULL))
    {
	smpd_err_printf("WriteFile() failed to write the password, error %d\n", GetLastError());
	CloseHandle(hWrite);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    if (num_written != SMPD_MAX_PASSWORD_LENGTH)
    {
	smpd_err_printf("parital password string written, %d bytes of %d\n", num_written, SMPD_MAX_PASSWORD_LENGTH);
	CloseHandle(hWrite);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    smpd_dbg_printf("smpd sending the smpd passphrase to the manager\n");
    if (!WriteFile(hWrite, smpd_process.passphrase, SMPD_PASSPHRASE_MAX_LENGTH, &num_written, NULL))
    {
	smpd_err_printf("WriteFile() failed to write the passphrase, error %d\n", GetLastError());
	CloseHandle(hWrite);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    if (num_written != SMPD_PASSPHRASE_MAX_LENGTH)
    {
	smpd_err_printf("parital passphrase string written, %d bytes of %d\n", num_written, SMPD_PASSPHRASE_MAX_LENGTH);
	CloseHandle(hWrite);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    smpd_dbg_printf("closing the pipe to the manager\n");
    CloseHandle(hWrite);

    return SMPD_SUCCESS;
}

#else /* HAVE_WINDOWS_H */

#undef FCNAME
#define FCNAME "smpd_start_unx_mgr"
int smpd_start_unx_mgr(smpd_context_t *context)
{
    int result;

    result = fork();
    if (result == -1)
    {
	smpd_err_printf("fork failed, errno %d\n", errno);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    if (result == 0)
    {
	/* the child is not the root so clear the flag */
	smpd_process.root_smpd = SMPD_FALSE;
	/* the child cannot interact with the user so set these flags accordingly */
	smpd_process.noprompt = SMPD_TRUE;
	smpd_process.credentials_prompt = SMPD_FALSE;
    }
    return SMPD_SUCCESS;
}

#endif /* HAVE_WINDOWS_H */
