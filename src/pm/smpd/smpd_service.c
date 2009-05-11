/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include "smpd_service.h"
#include <ntsecapi.h>

VOID WINAPI smpd_service_ctrl(DWORD dwCtrlCode);
static LPTSTR smpd_get_last_error_text( LPTSTR lpszBuf, DWORD dwSize );

/*
  FUNCTION: smpd_service_main

  PURPOSE: To perform actual initialization of the service

  PARAMETERS:
    dwArgc   - number of command line arguments
    lpszArgv - array of command line arguments

  RETURN VALUE:
    none

  COMMENTS:
    This routine performs the service initialization and then calls
    the user defined smpd_entry_point() routine to perform majority
    of the work.
*/
void smpd_service_main(int argc, char *argv[])
{
    SMPD_UNREFERENCED_ARG(argc);
    SMPD_UNREFERENCED_ARG(argv);

    /* register our service control handler: */
    smpd_process.sshStatusHandle = RegisterServiceCtrlHandler( TEXT(SMPD_SERVICE_NAME), smpd_service_ctrl);
    
    if (!smpd_process.sshStatusHandle)
	return;
    
    /* SERVICE_STATUS members that don't change in example */
    smpd_process.ssStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    smpd_process.ssStatus.dwServiceSpecificExitCode = 0;

    /* report the status to the service control manager. */
    if (!smpd_report_status_to_sc_mgr(SERVICE_START_PENDING, NO_ERROR, 3000))
    {
	smpd_report_status_to_sc_mgr(SERVICE_STOPPED, NO_ERROR, 0);
	return;
    }

    smpd_entry_point();
    smpd_clear_process_registry();

    /* try to report the stopped status to the service control manager. */
    if (smpd_process.sshStatusHandle)
    {
	smpd_report_status_to_sc_mgr(SERVICE_STOPPED, 0, 0);
    }
}



/*
  FUNCTION: smpd_service_ctrl

  PURPOSE: This function is called by the SCM whenever
           ControlService() is called on this service.

  PARAMETERS:
    dwCtrlCode - type of control requested

  RETURN VALUE:
    none

  COMMENTS:
*/
VOID WINAPI smpd_service_ctrl(DWORD dwCtrlCode)
{
    /* Handle the requested control code. */
    switch(dwCtrlCode)
    {
    case SERVICE_CONTROL_CONTINUE:
	/* Notifies a paused service that it should resume. */
	smpd_process.ssStatus.dwCurrentState = SERVICE_RUNNING;
	break;
    case SERVICE_CONTROL_INTERROGATE:
	/* Notifies a service that it should report its current status information to the service control manager. */
	break;
    case SERVICE_CONTROL_NETBINDADD:
	/* Notifies a network service that there is a new component for binding. The service should bind to the new component. */
	break;
    case SERVICE_CONTROL_NETBINDDISABLE:
	/* Notifies a network service that one of its bindings has been disabled. The service should reread its binding information and remove the binding. */
	break;
    case SERVICE_CONTROL_NETBINDENABLE:
	/* Notifies a network service that a disabled binding has been enabled. The service should reread its binding information and add the new binding. */
	break;
    case SERVICE_CONTROL_NETBINDREMOVE:
	/* Notifies a network service that a component for binding has been removed. The service should reread its binding information and unbind from the removed component. */
	break;
    case SERVICE_CONTROL_PARAMCHANGE:
	/* Notifies a service that its startup parameters have changed. The service should reread its startup parameters. */
	break;
    case SERVICE_CONTROL_PAUSE:
	/* Notifies a service that it should pause. */
	smpd_process.ssStatus.dwCurrentState = SERVICE_PAUSE_PENDING;
	break;
    case SERVICE_CONTROL_SHUTDOWN:
	/* Notifies a service that the system is shutting down so the service can perform cleanup tasks. */
	break;
    case SERVICE_CONTROL_STOP:
	/* Stop the service. */
	smpd_process.ssStatus.dwCurrentState = SERVICE_STOP_PENDING;
	smpd_service_stop();
	break;
    default:
	/* invalid control code */
	break;
	
    }
    smpd_report_status_to_sc_mgr(smpd_process.ssStatus.dwCurrentState, NO_ERROR, 0);
}



/*
  FUNCTION: smpd_report_status_to_sc_mgr()

  PURPOSE: Sets the current status of the service and
           reports it to the Service Control Manager

  PARAMETERS:
    dwCurrentState - the state of the service
    dwWin32ExitCode - error code to report
    dwWaitHint - worst case estimate to next checkpoint

  RETURN VALUE:
    TRUE  - success
    FALSE - failure

  COMMENTS:
*/
SMPD_BOOL smpd_report_status_to_sc_mgr(DWORD dwCurrentState, DWORD dwWin32ExitCode, DWORD dwWaitHint)
{
    static DWORD dwCheckPoint = 1;
    SMPD_BOOL fResult = SMPD_TRUE;

    if (dwCurrentState == SERVICE_START_PENDING)
	smpd_process.ssStatus.dwControlsAccepted = 0;
    else
	smpd_process.ssStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;

    smpd_process.ssStatus.dwCurrentState = dwCurrentState;
    smpd_process.ssStatus.dwWin32ExitCode = dwWin32ExitCode;
    smpd_process.ssStatus.dwWaitHint = dwWaitHint;

    if ( ( dwCurrentState == SERVICE_RUNNING ) || ( dwCurrentState == SERVICE_STOPPED ) )
	smpd_process.ssStatus.dwCheckPoint = 0;
    else
	smpd_process.ssStatus.dwCheckPoint = dwCheckPoint++;

    /* Report the status of the service to the service control manager. */
    fResult = SetServiceStatus( smpd_process.sshStatusHandle, &smpd_process.ssStatus);
    if (!fResult)
    {
	smpd_add_error_to_message_log(TEXT("SetServiceStatus"));
    }
    return fResult;
}



/*
  FUNCTION: smpd_add_error_to_message_log(char *msg)

  PURPOSE: Allows any thread to log an error message

  PARAMETERS:
    lpszMsg - text for message

  RETURN VALUE:
    none

  COMMENTS:
*/
void smpd_add_error_to_message_log(char *msg)
{
    TCHAR   szMsg[256];
    HANDLE  hEventSource;
    LPTSTR  lpszStrings[2];
    DWORD dwErr;

    dwErr = GetLastError();

    /* Use event logging to log the error. */
    hEventSource = RegisterEventSource(NULL, TEXT(SMPD_SERVICE_NAME));

    _stprintf(szMsg, TEXT("%s error: %d"), TEXT(SMPD_SERVICE_NAME), dwErr);
    lpszStrings[0] = szMsg;
    lpszStrings[1] = msg;

    if (hEventSource != NULL) {
	ReportEvent(hEventSource, /* handle of event source */
	    EVENTLOG_ERROR_TYPE,  /* event type */
	    0,                    /* event category */
	    0,                    /* event ID */
	    NULL,                 /* current user's SID */
	    2,                    /* strings in lpszStrings */
	    0,                    /* no bytes of raw data */
	    (LPCTSTR*)lpszStrings,/* array of error strings */
	    NULL);                /* no raw data */

	(VOID) DeregisterEventSource(hEventSource);
    }
}

/*
  FUNCTION: smpd_setup_service_restart( SC_HANDLE schService )

  PURPOSE: Setup the service to automatically restart if it has been down for 5 minutes

  PARAMETERS:
    schService - service handle

  RETURN VALUE:
    BOOL

  COMMENTS:
    code provided by Bradley, Peter C. (MIS/CFD) [bradlepc@pweh.com]
*/
static BOOL smpd_setup_service_restart( SC_HANDLE schService )
{
    SC_ACTION	actionList[3];
    SERVICE_FAILURE_ACTIONS schActionOptions;
    SERVICE_DESCRIPTION ds;
    HMODULE hModule;
    BOOL ( WINAPI * ChangeServiceConfig2_fn)(SC_HANDLE hService, DWORD dwInfoLevel, LPVOID lpInfo);

    hModule = GetModuleHandle("Advapi32");
    if (hModule == NULL)
	return FALSE;

    ChangeServiceConfig2_fn = (BOOL ( WINAPI *)(SC_HANDLE, DWORD, LPVOID))GetProcAddress(hModule, "ChangeServiceConfig2A");
    if (ChangeServiceConfig2_fn == NULL)
	return FALSE;

    /* Add a description string */
    ds.lpDescription = "Process manager service for MPICH2 applications";
    ChangeServiceConfig2_fn(schService, SERVICE_CONFIG_DESCRIPTION, &ds);

    /* The actions in this array are performed in order each time the service fails 
       within the specified reset period.
       This array attempts to restart mpd twice and then allow it to stay dead.
    */
    actionList[0].Type = SC_ACTION_RESTART;
    actionList[0].Delay = 0;
    actionList[1].Type = SC_ACTION_RESTART;
    actionList[1].Delay = 0;
    actionList[2].Type = SC_ACTION_NONE;
    actionList[2].Delay = 0;

    schActionOptions.dwResetPeriod = (DWORD) 300;  /* 5 minute reset */
    schActionOptions.lpRebootMsg = NULL;
    schActionOptions.lpCommand = NULL;
    schActionOptions.cActions = (DWORD) (sizeof actionList / sizeof actionList[0]);
    schActionOptions.lpsaActions = actionList;

    return ChangeServiceConfig2_fn(schService, SERVICE_CONFIG_FAILURE_ACTIONS, &schActionOptions);
}


/* Useful if we decide that smpd should not be installed on a domain controller */
BOOL Is_DomainController()
{
   OSVERSIONINFOEX osvi;
   DWORDLONG dwlConditionMask = 0;
 
   /* Initialize the OSVERSIONINFOEX structure. */
   ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
   osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
   osvi.dwMajorVersion = 5;
   osvi.wProductType = VER_NT_DOMAIN_CONTROLLER;
 
   /* Initialize the condition mask. */
   VER_SET_CONDITION(dwlConditionMask, VER_MAJORVERSION, VER_GREATER_EQUAL);
   VER_SET_CONDITION(dwlConditionMask, VER_PRODUCT_TYPE, VER_EQUAL);
 
   /* Perform the test. */
   return VerifyVersionInfo(&osvi, VER_MAJORVERSION | VER_PRODUCT_TYPE, dwlConditionMask);
}

/*
  FUNCTION: smpd_install_service()

  PURPOSE: Installs the service

  PARAMETERS:
    none

  RETURN VALUE:
    none

  COMMENTS:
*/

/* FIXME: Remove hardcoded values -- eg: Length of error Mesg, 256*/

#define SMPD_MAX_FILENAME_DECORATION (2 * sizeof(TCHAR))
#define SMPD_MAX_FILENAME_QUOTED (SMPD_MAX_FILENAME + SMPD_MAX_FILENAME_DECORATION)
void smpd_install_service(SMPD_BOOL interact, SMPD_BOOL bSetupRestart, SMPD_BOOL bSetupScp)
{
    SC_HANDLE   schService;
    SC_HANDLE   schSCManager;
    TCHAR szErr[256];
    TCHAR szPathQuoted[SMPD_MAX_FILENAME_QUOTED];
    LPTSTR pszPathQuoted;
    
    pszPathQuoted = szPathQuoted;
    /* The smpd module file name has to be quoted before passing to CreateService() --- refer MSDN doc for 
     * CreateService() 
     */
    _stprintf(pszPathQuoted, TEXT("\""));
    if ( GetModuleFileName( NULL, pszPathQuoted+1, SMPD_MAX_FILENAME ) == 0 )
    {
        _tprintf(TEXT("Unable to install %s.\n%s\n"), TEXT(SMPD_SERVICE_DISPLAY_NAME), smpd_get_last_error_text(szErr, 256));
	fflush(stdout);
        return;
    }
    
    _stprintf(pszPathQuoted + _tcslen(pszPathQuoted), TEXT("\""));

    schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if ( schSCManager )
    {
	DWORD type = SERVICE_WIN32_OWN_PROCESS;
	if (interact)
	    type = type | SERVICE_INTERACTIVE_PROCESS;
        schService = CreateService(
            schSCManager,               /* SCManager database */
            TEXT(SMPD_SERVICE_NAME),        /* name of service */
            TEXT(SMPD_SERVICE_DISPLAY_NAME), /* name to display */
            SERVICE_ALL_ACCESS,         /* desired access */
	    type,
	    SERVICE_AUTO_START,
            /*SERVICE_ERROR_NORMAL,*/       /* error control type */
	    SERVICE_ERROR_IGNORE,
            szPathQuoted,                /* service's binary */
            NULL,                       /* no load ordering group */
            NULL,                       /* no tag identifier */
            TEXT(""),                   /* dependencies */
            NULL,                       /* LocalSystem account if account==NULL */
            NULL);
	
        if ( schService )
        {
	    if (bSetupRestart)
		smpd_setup_service_restart( schService );

	    if (bSetupScp)
	    {
		smpd_register_spn(NULL, NULL, NULL);
	    }

	    /* Start the service */
	    if (StartService(schService, 0, NULL))
		_tprintf(TEXT("%s installed.\n"), TEXT(SMPD_SERVICE_DISPLAY_NAME) );
	    else
		_tprintf(TEXT("%s installed, but failed to start:\n%s.\n"), TEXT(SMPD_SERVICE_DISPLAY_NAME), smpd_get_last_error_text(szErr, 256) );
	    fflush(stdout);
            CloseServiceHandle(schService);
        }
        else
        {
            _tprintf(TEXT("CreateService failed:\n%s\n"), smpd_get_last_error_text(szErr, 256));
	    fflush(stdout);
        }
	
        CloseServiceHandle(schSCManager);
    }
    else
    {
        _tprintf(TEXT("OpenSCManager failed:\n%s\n"), smpd_get_last_error_text(szErr,256));
	fflush(stdout);
    }
}



/*
  FUNCTION: smpd_remove_service(BOOL bErrorOnNotInstalled)

  PURPOSE: Stops and removes the service

  PARAMETERS:
    none

  RETURN VALUE:
    none

  COMMENTS:
*/
SMPD_BOOL smpd_remove_service(SMPD_BOOL bErrorOnNotInstalled)
{
    SMPD_BOOL   bRetVal = SMPD_FALSE;
    SC_HANDLE   schService;
    SC_HANDLE   schSCManager;
    TCHAR       szErr[256];

    schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS );
    if ( schSCManager )
    {
        schService = OpenService(schSCManager, TEXT(SMPD_SERVICE_NAME), SERVICE_ALL_ACCESS);

	if (schService)
        {
            /* try to stop the service */
            if ( ControlService( schService, SERVICE_CONTROL_STOP, &smpd_process.ssStatus ) )
            {
		_tprintf(TEXT("Stopping %s."), TEXT(SMPD_SERVICE_DISPLAY_NAME));
		fflush(stdout);
                Sleep( 1000 );
		
                while( QueryServiceStatus( schService, &smpd_process.ssStatus ) )
                {
                    if ( smpd_process.ssStatus.dwCurrentState == SERVICE_STOP_PENDING )
                    {
			_tprintf(TEXT("."));
			fflush(stdout);
                        Sleep( 250 );
                    }
                    else
                        break;
                }
		
                if ( smpd_process.ssStatus.dwCurrentState == SERVICE_STOPPED )
		{
                    _tprintf(TEXT("\n%s stopped.\n"), TEXT(SMPD_SERVICE_DISPLAY_NAME) );
		    fflush(stdout);
		}
                else
		{
                    _tprintf(TEXT("\n%s failed to stop.\n"), TEXT(SMPD_SERVICE_DISPLAY_NAME) );
		    fflush(stdout);
		}
		
            }
	    
	    /* Delete the registry entries for the service. */
	    RegDeleteKey(HKEY_LOCAL_MACHINE, "SOFTWARE\\MPICH\\SMPD");

	    /* now remove the service */
            if (DeleteService(schService))
	    {
                _tprintf(TEXT("%s removed.\n"), TEXT(SMPD_SERVICE_DISPLAY_NAME) );
		fflush(stdout);
		bRetVal = SMPD_TRUE;
	    }
            else
	    {
                _tprintf(TEXT("DeleteService failed:\n%s\n"), smpd_get_last_error_text(szErr,256));
		fflush(stdout);
	    }

	    CloseServiceHandle(schService);
        }
        else
	{
	    if (bErrorOnNotInstalled)
	    {
		_tprintf(TEXT("OpenService failed:\n%s\n"), smpd_get_last_error_text(szErr,256));
		fflush(stdout);
	    }
	    else
	    {
		bRetVal = SMPD_TRUE;
	    }
	}

	CloseServiceHandle(schSCManager);
    }
    else
    {
        _tprintf(TEXT("OpenSCManager failed:\n%s\n"), smpd_get_last_error_text(szErr,256));
	fflush(stdout);
    }
    return bRetVal;
}

/*
  FUNCTION: smpd_stop_service()

  PURPOSE: Stops the service

  PARAMETERS:
    none

  RETURN VALUE:
    none

  COMMENTS:
*/
void smpd_stop_service()
{
    SC_HANDLE   schService;
    SC_HANDLE   schSCManager;
    TCHAR szErr[256];
    
    schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if ( schSCManager )
    {
        schService = OpenService(schSCManager, TEXT(SMPD_SERVICE_NAME), SERVICE_ALL_ACCESS);

	if (schService)
        {
            /* try to stop the service */
            if ( ControlService( schService, SERVICE_CONTROL_STOP, &smpd_process.ssStatus ) )
            {
                _tprintf(TEXT("Stopping %s."), TEXT(SMPD_SERVICE_DISPLAY_NAME));
		fflush(stdout);
                Sleep( 1000 );
		
                while( QueryServiceStatus( schService, &smpd_process.ssStatus ) )
                {
                    if ( smpd_process.ssStatus.dwCurrentState == SERVICE_STOP_PENDING )
                    {
                        _tprintf(TEXT("."));
			fflush(stdout);
                        Sleep( 250 );
                    }
                    else
                        break;
                }

		if ( smpd_process.ssStatus.dwCurrentState == SERVICE_STOPPED )
		{
                    _tprintf(TEXT("\n%s stopped.\n"), TEXT(SMPD_SERVICE_DISPLAY_NAME) );
		    fflush(stdout);
		}
                else
		{
                    _tprintf(TEXT("\n%s failed to stop.\n"), TEXT(SMPD_SERVICE_DISPLAY_NAME) );
		    fflush(stdout);
		}
		
            }
	    
            CloseServiceHandle(schService);
        }
        else
	{
            _tprintf(TEXT("OpenService failed:\n%s\n"), smpd_get_last_error_text(szErr,256));
	    fflush(stdout);
	}
	
        CloseServiceHandle(schSCManager);
    }
    else
    {
        _tprintf(TEXT("OpenSCManager failed:\n%s\n"), smpd_get_last_error_text(szErr,256));
	fflush(stdout);
    }
}

/*
  FUNCTION: smpd_start_service()

  PURPOSE: Starts the service

  PARAMETERS:
    none

  RETURN VALUE:
    none

  COMMENTS:
*/
void smpd_start_service()
{
    SC_HANDLE   schService;
    SC_HANDLE   schSCManager;
    TCHAR szErr[256];

    schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if ( schSCManager )
    {
        schService = OpenService(schSCManager, TEXT(SMPD_SERVICE_NAME), SERVICE_ALL_ACCESS);

        if ( schService )
        {
	    /* Start the service */
	    if (StartService(schService, 0, NULL))
	    {
		_tprintf(TEXT("%s started.\n"), TEXT(SMPD_SERVICE_DISPLAY_NAME) );
		fflush(stdout);
	    }
	    else
	    {
		_tprintf(TEXT("%s failed to start.\n%s.\n"), TEXT(SMPD_SERVICE_DISPLAY_NAME), smpd_get_last_error_text(szErr, 256) );
		fflush(stdout);
	    }
            CloseServiceHandle(schService);
        }
        else
        {
            _tprintf(TEXT("OpenService failed:\n%s\n"), smpd_get_last_error_text(szErr,256));
	    fflush(stdout);
        }
	
        CloseServiceHandle(schSCManager);
    }
    else
    {
        _tprintf(TEXT("OpenSCManager failed:\n%s\n"), smpd_get_last_error_text(szErr,256));
	fflush(stdout);
    }
}

/*
  FUNCTION: smpd_get_last_error_text

  PURPOSE: copies error message text to string

  PARAMETERS:
    lpszBuf - destination buffer
    dwSize - size of buffer

  RETURN VALUE:
    destination buffer

  COMMENTS:
*/
static LPTSTR smpd_get_last_error_text( LPTSTR lpszBuf, DWORD dwSize )
{
    DWORD dwRet;
    LPTSTR lpszTemp = NULL;
    
    dwRet = FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |FORMAT_MESSAGE_ARGUMENT_ARRAY,
	NULL,
	GetLastError(),
	LANG_NEUTRAL,
	(LPTSTR)&lpszTemp,
	0,
	NULL );
    
    /* supplied buffer is not long enough */
    if ( !dwRet || ( (long)dwSize < (long)dwRet+14 ) )
        lpszBuf[0] = TEXT('\0');
    else
    {
        lpszTemp[lstrlen(lpszTemp)-2] = TEXT('\0');  /* remove cr and newline character */
        _stprintf( lpszBuf, TEXT("%s (error %d)"), lpszTemp, GetLastError() );
    }
    
    if ( lpszTemp )
        LocalFree((HLOCAL) lpszTemp );
    
    return lpszBuf;
}

/* A bomb thread can be used to guarantee that the service will exit when a stop command is processed */
void smpd_bomb_thread()
{
    if (WaitForSingleObject(smpd_process.hBombDiffuseEvent, (DWORD)10000) == WAIT_TIMEOUT)
    {
	smpd_dbg_printf("smpd_bomb_thread timed out, exiting.\n");
	ExitProcess((UINT)-1);
    }
}

/*
  FUNCTION: smpd_service_stop

  PURPOSE: Stops the service

  PARAMETERS:
    none

  RETURN VALUE:
    none

  COMMENTS:
    If a ServiceStop procedure is going to
    take longer than 3 seconds to execute,
    it should spawn a thread to execute the
    stop code, and return.  Otherwise, the
    ServiceControlManager will believe that
    the service has stopped responding.
*/    
void smpd_service_stop()
{
    SMPDU_Sock_set_t set;
    SMPDU_Sock_t sock;
    SMPDU_Sock_event_t event;
    char host[SMPD_MAX_HOST_LENGTH];
    int iter;
    DWORD dwThreadID;
    int result;

    for (iter=0; iter<10; iter++)
    {
	smpd_process.hBombThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)smpd_bomb_thread, NULL, 0, &dwThreadID);
	if (smpd_process.hBombThread != NULL)
	    break;
	Sleep(250);
    }

    /* stop the main thread */
    smpd_process.service_stop = SMPD_TRUE;
    smpd_get_hostname(host, SMPD_MAX_HOST_LENGTH);
    result = SMPDU_Sock_create_set(&set);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("SMPDU_Sock_create_set failed,\nsock error: %s\n", get_sock_error_string(result));
	SetEvent(smpd_process.hBombDiffuseEvent);
	WaitForSingleObject(smpd_process.hBombThread, (DWORD)3000);
	CloseHandle(smpd_process.hBombThread);
	ExitProcess((UINT)-1);
    }
    result = SMPDU_Sock_post_connect(set, NULL, host, smpd_process.port, &sock);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("Unable to connect to '%s:%d',\nsock error: %s\n",
	    smpd_process.host_list->host, smpd_process.port, get_sock_error_string(result));
	SetEvent(smpd_process.hBombDiffuseEvent);
	WaitForSingleObject(smpd_process.hBombThread, (DWORD)3000);
	CloseHandle(smpd_process.hBombThread);
	ExitProcess((UINT)-1);
    }
    result = SMPDU_Sock_wait(set, SMPDU_SOCK_INFINITE_TIME, &event);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("Unable to connect to '%s:%d',\nsock error: %s\n",
	    smpd_process.host_list->host, smpd_process.port, get_sock_error_string(result));
	SetEvent(smpd_process.hBombDiffuseEvent);
	WaitForSingleObject(smpd_process.hBombThread, (DWORD)3000);
	CloseHandle(smpd_process.hBombThread);
	ExitProcess((UINT)-1);
    }
}
