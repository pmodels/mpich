/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id: MPIRun.cpp,v 1.2 2002/09/27 21:11:12 toonen Exp $
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include <stdio.h>
#include "MPICH_pwd.h"
#include "MPIJobDefs.h"
#include <conio.h>
#include <time.h>
#include "Translate_Error.h"
#include "localonly.h"
#include "GetOpt.h"
#include "LaunchProcess.h"
#include "global.h"
#include "WaitThread.h"
#include "mpd.h"
#include "mpdutil.h"
//#include "bsocket.h"
#include "RedirectIO.h"
#include <ctype.h>
#include <stdlib.h>

// Prototypes
void WaitForExitCommands();
void ExeToUnc(char *pszExe);

// Function name	: PrintOptions
// Description	    : 
// Return type		: void 
void PrintOptions()
{
    printf("\n");
    printf("Usage:\n");
    printf("   MPIRun -np #processes [options] executable [args ...]\n");
    printf("   MPIRun [options] configfile [args ...]\n");
    //printf("   MPIRun -localonly #processes [-mpirun options] exe [args ...]\n");
    printf("\n");
    printf("mpirun options:\n");
    printf("   -localonly\n");
    printf("   -env \"var1=val1|var2=val2|var3=val3...\"\n");
    printf("   -dir drive:\\my\\working\\directory\n");
    printf("   -logon\n");
    printf("   -map drive:\\\\host\\share\n");
    printf("\n");
    printf("Config file format:\n");
    printf("   >exe c:\\temp\\mpiprogram.exe\n");
    printf("     OR \\\\host\\share\\mpiprogram.exe\n");
    printf("   >[env var1=val1|var2=val2|var3=val3...]\n");
    printf("   >[dir drive:\\my\\working\\directory]\n");
    printf("   >[map drive:\\\\host\\share]\n");
    printf("   >[args arg1 arg2 ...]\n");
    printf("   >hosts\n");
    printf("   >hostname1 #procs [path\\mpiprogram.exe]\n");
    printf("   >hostname2 #procs [path\\mpiprogram.exe]\n");
    printf("   >hostname3 #procs [path\\mpiprogram.exe]\n");
    printf("   >...\n");
    printf("\n");
    printf("bracketed lines are optional\n");
    printf("\n");
    printf("For a list of all mpirun options, execute 'mpirun -help2'\n");
    printf("\n");
}

void PrintExtraOptions()
{
    printf("\n");
    printf("All options to mpirun:\n");
    printf("\n");
    printf("-np x\n");
    printf("  launch x processes\n");
    printf("-localonly x\n");
    printf("-np x -localonly\n");
    printf("  launch x processes on the local machine\n");
    printf("-machinefile filename\n");
    printf("  use a file to list the names of machines to launch on\n");
    printf("-map drive:\\\\host\\share\n");
    printf("  map a drive on all the nodes\n");
    printf("  this mapping will be removed when the processes exit\n");
    printf("-dir drive:\\my\\working\\directory\n");
    printf("  launch processes in the specified directory\n");
    printf("-env \"var1=val1|var2=val2|var3=val3...\"\n");
    printf("  set environment variables before launching the processes\n");
    printf("-logon\n");
    printf("  prompt for user account and password\n");
    printf("-tcp\n");
    printf("  use tcp instead of shared memory on the local machine\n");
    printf("-getphrase\n");
    printf("  prompt for the passphrase to access remote mpds\n");
    printf("-nocolor\n");
    printf("  don't use process specific output coloring\n");
    printf("-nompi\n");
    printf("  launch processes without the mpi startup mechanism\n");
    printf("-nodots\n");
    printf("  don't output dots while logging on the user\n");
    printf("-nomapping\n");
    printf("  don't try to map the current directory on the remote nodes\n");
    printf("-nopopup_debug\n");
    printf("  disable the system popup dialog if the process crashes\n");
    printf("-hosts n host1 host2 ... hostn\n");
    printf("-hosts n host1 m1 host2 m2 ... hostn mn\n");
    printf("  launch on the specified hosts\n");
    printf("  the number of processes = m1 + m2 + ... + mn\n");
}

// Function name	: ConnectReadMPDRegistry
// Description	    : 
// Return type		: bool 
// Argument         : char *pszHost
// Argument         : int nPort
// Argument         : char *pszPassPhrase
// Argument         : char *name
// Argument         : char *value
// Argument         : DWORD *length = NULL
bool ConnectReadMPDRegistry(char *pszHost, int nPort, char *pszPassPhrase, char *name, char *value, DWORD *length = NULL)
{
    int error;
    SOCKET sock;
    char pszStr[1024];

    if ((error = ConnectToMPD(pszHost, nPort, pszPassPhrase, &sock)) == 0)
    {
	sprintf(pszStr, "lget %s", name);
	WriteString(sock, pszStr);
	ReadString(sock, pszStr);
	if (strlen(pszStr))
	{
	    WriteString(sock, "done");
	    easy_closesocket(sock);
	    strcpy(value, pszStr);
	    if (length)
		*length = strlen(pszStr);
	    //printf("ConnectReadMPDRegistry successfully used to read the host entry:\n%s\n", pszStr);fflush(stdout);
	    return true;
	}
	WriteString(sock, "done");
	easy_closesocket(sock);
    }
    else
    {
	//printf("MPIRunLaunchProcess: Connect to %s failed, error %d\n", pszHost, error);fflush(stdout);
    }
    return false;
}

// Function name	: ReadMPDRegistry
// Description	    : 
// Return type		: bool 
// Argument         : char *name
// Argument         : char *value
// Argument         : DWORD *length = NULL
bool ReadMPDRegistry(char *name, char *value, DWORD *length /*= NULL*/)
{
    HKEY tkey;
    DWORD len, result;
    
    // Open the root key
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, MPD_REGISTRY_KEY,
	0, 
	KEY_READ,
	&tkey) != ERROR_SUCCESS)
    {
	//printf("Unable to open the MPD registry key, error %d\n", GetLastError());
	return false;
    }
    
    if (length == NULL)
	len = MAX_CMD_LENGTH;
    else
	len = *length;
    result = RegQueryValueEx(tkey, name, 0, NULL, (unsigned char *)value, &len);
    if (result != ERROR_SUCCESS)
    {
	//printf("Unable to read the mpd registry key '%s', error %d\n", name, GetLastError());
	RegCloseKey(tkey);
	return false;
    }
    if (length != NULL)
	*length = len;
    
    RegCloseKey(tkey);
    return true;
}

// Function name	: GetHostsFromRegistry
// Description	    : 
// Return type		: bool 
// Argument         : HostNode **list
bool GetHostsFromRegistry(HostNode **list)
{
    // Read the hosts entry
    char pszHosts[MAX_CMD_LENGTH+1];
    DWORD nLength = MAX_CMD_LENGTH;
    if (!ReadMPDRegistry("hosts", pszHosts, &nLength))
    {
	char localhost[100];
	gethostname(localhost, 100);
	if (!ConnectReadMPDRegistry(localhost, MPD_DEFAULT_PORT, MPD_DEFAULT_PASSPHRASE, "hosts", pszHosts, &nLength))
	    return false;
    }
    /*
    // If this code is used, the program fails at the delete 
    // statement at the end of the function.  I don't know why.
    char *pszHosts = NULL;
    DWORD num_bytes = 0;//1024*sizeof(char);
    if (!ReadMPDRegistry("hosts", NULL, &num_bytes))
	return false;
    pszHosts = new char[num_bytes+1];
    if (!ReadMPDRegistry("hosts", pszHosts, &num_bytes))
    {
	delete pszHosts;
	return false;
    }
    */
/*    
    char *token = NULL;
    token = strtok(pszHosts, "|");
    if (token != NULL)
    {
	HostNode *n, *l = new HostNode;
	
	// Make a list of the available nodes
	l->next = NULL;
	strncpy(l->host, token, MAX_HOST_LENGTH);
	l->host[MAX_HOST_LENGTH-1] = '\0';
	l->nSMPProcs = 1;
	n = l;
	while ((token = strtok(NULL, "|")) != NULL)
	{
	    n->next = new HostNode;
	    n = n->next;
	    n->next = NULL;
	    strncpy(n->host, token, MAX_HOST_LENGTH);
	    n->host[MAX_HOST_LENGTH-1] = '\0';
	    n->nSMPProcs = 1;
	}
	
	*list = l;
	
	//delete pszHosts;
	return true;
    }
    
    //delete pszHosts;
    return false;
*/
    QVS_Container *phosts;
    phosts = new QVS_Container(pszHosts);
    if (phosts->first(pszHosts, MAX_CMD_LENGTH))
    {
	HostNode *n, *l = new HostNode;
	
	l->next = NULL;
	strncpy(l->host, pszHosts, MAX_HOST_LENGTH);
	l->host[MAX_HOST_LENGTH-1] = '\0';
	l->nSMPProcs = 1;
	n = l;
	while (phosts->next(pszHosts, MAX_CMD_LENGTH))
	{
	    n->next = new HostNode;
	    n = n->next;
	    n->next = NULL;
	    strncpy(n->host, pszHosts, MAX_HOST_LENGTH);
	    n->host[MAX_HOST_LENGTH-1] = '\0';
	    n->nSMPProcs = 1;
	}
	*list = l;
	delete phosts;
	return true;
    }
    delete phosts;
    return false;
}

// Function name	: GetAvailableHosts
// Description	    : This function requires g_nHosts to have been previously set.
// Return type		: bool 
bool GetAvailableHosts()
{
    HostNode *list = NULL;
    
    //dbg_printf("finding available hosts\n");
    if (g_nHosts > 0)
    {
	if (GetHostsFromRegistry(&list))
	{
	    // Insert the first host into the list
	    g_pHosts = new HostNode;
	    strncpy(g_pHosts->host, list->host, MAX_HOST_LENGTH);
	    g_pHosts->host[MAX_HOST_LENGTH-1] = '\0';
	    strncpy(g_pHosts->exe, g_pszExe, MAX_CMD_LENGTH);
	    g_pHosts->exe[MAX_CMD_LENGTH-1] = '\0';
	    g_pHosts->nSMPProcs = 1;
	    g_pHosts->next = NULL;

	    // add the nodes to the target list, cycling if necessary
	    int num_left = g_nHosts-1;
	    HostNode *n = list->next, *target = g_pHosts;
	    if (n == NULL)
		n = list;
	    while (num_left)
	    {
		target->next = new HostNode;
		target = target->next;
		target->next = NULL;
		strncpy(target->host, n->host, MAX_HOST_LENGTH);
		target->host[MAX_HOST_LENGTH-1] = '\0';
		strncpy(target->exe, g_pHosts->exe, MAX_CMD_LENGTH);
		target->exe[MAX_CMD_LENGTH-1] = '\0';
		target->nSMPProcs = 1;
		
		n = n->next;
		if (n == NULL)
		    n = list;
		
		num_left--;
	    }
	    
	    // free the list
	    while (list)
	    {
		n = list;
		list = list->next;
		delete n;
	    }
	}
	else
	{
	    return false;
	}
    }
    return true;
}

// Function name	: GetHostsFromFile
// Description	    : 
// Return type		: bool 
// Argument         : char *pszFileName
bool GetHostsFromFile(char *pszFileName)
{
    FILE *fin;
    char buffer[1024] = "";
    char *pChar, *pChar2;
    HostNode *node = NULL, *list = NULL, *cur_node;

    fin = fopen(pszFileName, "r");
    if (fin == NULL)
    {
	printf("unable to open file '%s'\n", pszFileName);
	return false;
    }
    
    // Read the host names from the file
    while (fgets(buffer, 1024, fin))
    {
	pChar = buffer;
	
	// Advance over white space
	while (*pChar != '\0' && isspace(*pChar))
	    pChar++;
	if (*pChar == '#' || *pChar == '\0')
	    continue;
	
	// Trim trailing white space
	pChar2 = &buffer[strlen(buffer)-1];
	while (isspace(*pChar2) && (pChar >= pChar))
	{
	    *pChar2 = '\0';
	    pChar2--;
	}
	
	// If there is anything left on the line, consider it a host name
	if (strlen(pChar) > 0)
	{
	    node = new HostNode;
	    node->nSMPProcs = 1;
	    node->next = NULL;
	    node->exe[0] = '\0';
	    
	    // Copy the host name
	    pChar2 = node->host;
	    while (*pChar != '\0' && !isspace(*pChar))
	    {
		*pChar2 = *pChar;
		pChar++;
		pChar2++;
	    }
	    *pChar2 = '\0';
	    pChar2 = strtok(node->host, ":");
	    pChar2 = strtok(NULL, "\n");
	    if (pChar2 != NULL)
	    {
		node->nSMPProcs = atoi(pChar2);
		if (node->nSMPProcs < 1)
		    node->nSMPProcs = 1;
	    }
	    else
	    {
		// Advance over white space
		while (*pChar != '\0' && isspace(*pChar))
		    pChar++;
		// Get the number of SMP processes
		if (*pChar != '\0')
		{
		    node->nSMPProcs = atoi(pChar);
		    if (node->nSMPProcs < 1)
			node->nSMPProcs = 1;
		}
	    }

	    if (list == NULL)
	    {
		list = node;
		cur_node = node;
	    }
	    else
	    {
		cur_node->next = node;
		cur_node = node;
	    }
	}
    }

    fclose(fin);

    if (list == NULL)
	return false;

    // Allocate the first host node
    g_pHosts = new HostNode;
    int num_left = g_nHosts;
    HostNode *n = list, *target = g_pHosts;

    // add the nodes to the target list, cycling if necessary
    while (num_left)
    {
	target->next = NULL;
	strncpy(target->host, n->host, MAX_HOST_LENGTH);
	target->host[MAX_HOST_LENGTH-1] = '\0';
	strncpy(target->exe, g_pszExe, MAX_CMD_LENGTH);
	target->exe[MAX_CMD_LENGTH-1] = '\0';
	if (num_left <= n->nSMPProcs)
	{
	    target->nSMPProcs = num_left;
	    num_left = 0;
	}
	else
	{
	    target->nSMPProcs = n->nSMPProcs;
	    num_left = num_left - n->nSMPProcs;
	}

	if (num_left)
	{
	    target->next = new HostNode;
	    target = target->next;
	}

	n = n->next;
	if (n == NULL)
	    n = list;
    }
    
    // free the list
    while (list)
    {
	n = list;
	list = list->next;
	delete n;
    }

    return true;
}

// Function name	: ParseLineIntoHostNode
// Description	    : 
// Return type		: HostNode* 
// Argument         : char * line
HostNode* ParseLineIntoHostNode(char * line)
{
    char buffer[1024];
    char *pChar, *pChar2;
    HostNode *node = NULL;
    
    strncpy(buffer, line, 1024);
    buffer[1023] = '\0';
    pChar = buffer;
    
    // Advance over white space
    while (*pChar != '\0' && isspace(*pChar))
	pChar++;
    if (*pChar == '#' || *pChar == '\0')
	return NULL;
    
    // Trim trailing white space
    pChar2 = &buffer[strlen(buffer)-1];
    while (isspace(*pChar2) && (pChar >= pChar))
    {
	*pChar2 = '\0';
	pChar2--;
    }
    
    // If there is anything left on the line, consider it a host name
    if (strlen(pChar) > 0)
    {
	node = new HostNode;
	node->nSMPProcs = 1;
	node->next = NULL;
	node->exe[0] = '\0';
	
	// Copy the host name
	pChar2 = node->host;
	while (*pChar != '\0' && !isspace(*pChar))
	{
	    *pChar2 = *pChar;
	    pChar++;
	    pChar2++;
	}
	*pChar2 = '\0';
	
	// Advance over white space
	while (*pChar != '\0' && isspace(*pChar))
	    pChar++;
	// Get the number of SMP processes
	if (*pChar != '\0')
	{
	    node->nSMPProcs = atoi(pChar);
	    if (node->nSMPProcs < 1)
		node->nSMPProcs = 1;
	}
	// Advance over the number
	while (*pChar != '\0' && isdigit(*pChar))
	    pChar++;
	
	// Advance over white space
	while (*pChar != '\0' && isspace(*pChar))
	    pChar++;
	// Copy the executable
	if (*pChar != '\0')
	{
	    strncpy(node->exe, pChar, MAX_CMD_LENGTH);
	    node->exe[MAX_CMD_LENGTH-1] = '\0';
	    ExeToUnc(node->exe);
	}
    }
    
    return node;
}

#define PARSE_ERR_NO_FILE  -1
#define PARSE_SUCCESS       0
// Function name	: ParseConfigFile
// Description	    : 
// Return type		: int 
// Argument         : char * filename
int ParseConfigFile(char * filename)
{
    FILE *fin;
    char buffer[1024] = "";

    //dbg_printf("parsing configuration file '%s'\n", filename);

    fin = fopen(filename, "r");
    if (fin == NULL)
    {
	return PARSE_ERR_NO_FILE;
    }
    
    while (fgets(buffer, 1024, fin))
    {
	// Check for the name of the executable
	if (strnicmp(buffer, "exe ", 4) == 0)
	{
	    char *pChar = &buffer[4];
	    while (isspace(*pChar))
		pChar++;
	    strncpy(g_pszExe, pChar, MAX_CMD_LENGTH);
	    g_pszExe[MAX_CMD_LENGTH-1] = '\0';
	    pChar = &g_pszExe[strlen(g_pszExe)-1];
	    while (isspace(*pChar) && (pChar >= g_pszExe))
	    {
		*pChar = '\0';
		pChar--;
	    }
	    ExeToUnc(g_pszExe);
	}
	else
	{
	    // Check for program arguments
	    if (strnicmp(buffer, "args ", 5) == 0)
	    {
		char *pChar = &buffer[5];
		while (isspace(*pChar))
		    pChar++;
		strncpy(g_pszArgs, pChar, MAX_CMD_LENGTH);
		g_pszArgs[MAX_CMD_LENGTH-1] = '\0';
		pChar = &g_pszArgs[strlen(g_pszArgs)-1];
		while (isspace(*pChar) && (pChar >= g_pszArgs))
		{
		    *pChar = '\0';
		    pChar--;
		}
	    }
	    else
	    {
		// Check for environment variables
		if (strnicmp(buffer, "env ", 4) == 0)
		{
		    char *pChar = &buffer[4];
		    while (isspace(*pChar))
			pChar++;
		    if (strlen(pChar) >= MAX_CMD_LENGTH)
		    {
			printf("Warning: environment variables truncated.\n");
			fflush(stdout);
		    }
		    strncpy(g_pszEnv, pChar, MAX_CMD_LENGTH);
		    g_pszEnv[MAX_CMD_LENGTH-1] = '\0';
		    pChar = &g_pszEnv[strlen(g_pszEnv)-1];
		    while (isspace(*pChar) && (pChar >= g_pszEnv))
		    {
			*pChar = '\0';
			pChar--;
		    }
		}
		else
		{
		    if (strnicmp(buffer, "map ", 4) == 0)
		    {
			char *pszMap;
			pszMap = &buffer[strlen(buffer)-1];
			while (isspace(*pszMap) && (pszMap >= buffer))
			{
			    *pszMap = '\0';
			    pszMap--;
			}
			pszMap = &buffer[4];
			while (isspace(*pszMap))
			    pszMap++;
			if (*pszMap != '\0' && strlen(pszMap) > 6 && pszMap[1] == ':')
			{
			    MapDriveNode *pNode = new MapDriveNode;
			    pNode->cDrive = pszMap[0];
			    strcpy(pNode->pszShare, &pszMap[2]);
			    pNode->pNext = g_pDriveMapList;
			    g_pDriveMapList = pNode;
			}
		    }
		    else
		    {
			if (strnicmp(buffer, "dir ", 4) == 0)
			{
			    char *pChar = &buffer[4];
			    while (isspace(*pChar))
				pChar++;
			    strcpy(g_pszDir, pChar);
			    pChar = &g_pszDir[strlen(g_pszDir)-1];
			    while (isspace(*pChar) && (pChar >= g_pszDir))
			    {
				*pChar = '\0';
				pChar--;
			    }
			}
			else
			{
			    // Check for hosts
			    if (strnicmp(buffer, "hosts", 5) == 0)
			    {
				g_nHosts = 0;
				g_pHosts = NULL;
				HostNode *node, dummy;
				dummy.next = NULL;
				node = &dummy;
				while (fgets(buffer, 1024, fin))
				{
				    node->next = ParseLineIntoHostNode(buffer);
				    if (node->next != NULL)
				    {
					node = node->next;
					g_nHosts++;
				    }
				}
				g_pHosts = dummy.next;
				
				fclose(fin);
				return PARSE_SUCCESS;
			    }
			}
		    }
		}
	    }
	}
    }
    fclose(fin);
    return PARSE_SUCCESS;
}

// Function name	: GetAccountAndPassword
// Description	    : Attempts to read the password from the registry, 
//	                  upon failure it requests the user to provide one
// Return type		: void 
void GetAccountAndPassword()
{
    char ch = 0;
    int index = 0;

    fprintf(stderr, "Mpd needs an account to launch processes with:\n");
    do
    {
	fprintf(stderr, "account (domain\\user): ");
	fflush(stderr);
	gets(g_pszAccount);
    } 
    while (strlen(g_pszAccount) == 0);
    
    fprintf(stderr, "password: ");
    fflush(stderr);
    
    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
    DWORD dwMode;
    if (!GetConsoleMode(hStdin, &dwMode))
	dwMode = ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT | ENABLE_MOUSE_INPUT;
    SetConsoleMode(hStdin, dwMode & ~ENABLE_ECHO_INPUT);
    gets(g_pszPassword);
    SetConsoleMode(hStdin, dwMode);
    
    fprintf(stderr, "\n");
}

// Function name	: GetMPDPassPhrase
// Description	    : 
// Return type		: void 
// Argument         : char *phrase
void GetMPDPassPhrase(char *phrase)
{
    fprintf(stderr, "mpd password: ");
    fflush(stderr);
    
    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
    DWORD dwMode;
    if (!GetConsoleMode(hStdin, &dwMode))
	dwMode = ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT | ENABLE_MOUSE_INPUT;
    SetConsoleMode(hStdin, dwMode & ~ENABLE_ECHO_INPUT);
    gets(phrase);
    SetConsoleMode(hStdin, dwMode);
    
    fprintf(stderr, "\n");
}

// Function name	: WaitToBreak
// Description	    : 
// Return type		: void 
void WaitToBreak()
{
    WaitForSingleObject(g_hBreakReadyEvent, INFINITE);
    if (easy_send(g_sockBreak, "x", 1) == SOCKET_ERROR)
	easy_send(g_sockStopIOSignalSocket, "x", 1);
}

// Function name	: CtrlHandlerRoutine
// Description	    : 
// Return type		: BOOL WINAPI 
// Argument         : DWORD dwCtrlType
static bool g_bFirst = true;
static HANDLE g_hLaunchThreadsRunning = CreateEvent(NULL, TRUE, TRUE, NULL);
BOOL WINAPI CtrlHandlerRoutine(DWORD dwCtrlType)
{
    bool bOK;

    // Don't abort while the launch threads are running because it can leave
    // processes running.
    if (WaitForSingleObject(g_hLaunchThreadsRunning, 0) == WAIT_OBJECT_0)
    {
	SetEvent(g_hAbortEvent);
	return TRUE;
    }

    // Hit Ctrl-C once and I'll try to kill the remote processes
    if (g_bFirst)
    {
	fprintf(stderr, "Soft break - attempting to kill processes\n(hit break again to do a hard abort)\n");
	
	// Signal all the threads to stop
	SetEvent(g_hAbortEvent);
	
	bOK = true;
	if (g_sockBreak != INVALID_SOCKET)
	{
	    // Send a break command to WaitForExitCommands
	    if (easy_send(g_sockBreak, "x", 1) == SOCKET_ERROR)
	    {
		printf("easy_send(break) failed, error %d\n", WSAGetLastError());
		bOK = false;
	    }
	}
	else
	{
	    // Start a thread to wait until a break can be issued.  This happens
	    // if you hit Ctrl-C before all the process threads have been created.
	    DWORD dwThreadId;
	    HANDLE hThread;
	    hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)WaitToBreak, NULL, 0, &dwThreadId);
	    if (hThread == NULL)
		bOK = false;
	    else
		CloseHandle(hThread);
	}
	if (!bOK)
	{
	    bOK = true;
	    // If you cannot issue a break signal, send a stop signal to the io threads
	    if (g_sockStopIOSignalSocket != INVALID_SOCKET)
	    {
		if (easy_send(g_sockStopIOSignalSocket, "x", 1) == SOCKET_ERROR)
		{
		    printf("easy_send(stop) failed, error %d\n", WSAGetLastError());
		    bOK =false;
		}
	    }
	    else
		bOK = false;
	    if (!bOK)
	    {
		if (g_bDoMultiColorOutput)
		{
		    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), g_ConsoleAttribute);
		}
		ExitProcess(1); // If you cannot issue either a break or stop signal, exit
	    }
	}

	g_bFirst = false;
	return TRUE;
    }
    
    fprintf(stderr, "aborting\n");

    // Hit Ctrl-C twice and I'll exit
    if (g_bDoMultiColorOutput)
    {
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), g_ConsoleAttribute);
    }

    // Issue a stop signal
    if (g_sockStopIOSignalSocket != INVALID_SOCKET)
    {
	if (easy_send(g_sockStopIOSignalSocket, "x", 1) == SOCKET_ERROR)
	    printf("easy_send(stop) failed, error %d\n", WSAGetLastError());
    }
    Sleep(2000); // Give a little time for the kill commands to get sent out?
    ExitProcess(1);
    return TRUE;
}

// Function name	: PrintDots
// Description	    : 
// Return type		: void 
// Argument         : HANDLE hEvent
void PrintDots(HANDLE hEvent)
{
    if (WaitForSingleObject(hEvent, 3000) == WAIT_TIMEOUT)
    {
	printf(".");fflush(stdout);
	while (WaitForSingleObject(hEvent, 1000) == WAIT_TIMEOUT)
	{
	    printf(".");fflush(stdout);
	}
    }
    CloseHandle(hEvent);
}

/*
// Function name	: CheckMapping
// Description	    : 
// Return type		: bool 
// Argument         : char *pszExe
bool CheckMapping(char *pszExe)
{
    DWORD dwResult;
    DWORD dwLength;
    char pBuffer[4096];
    REMOTE_NAME_INFO *info = (REMOTE_NAME_INFO*)pBuffer;
    char pszTemp[MAX_CMD_LENGTH];

    if (*pszExe == '"')
    {
	strncpy(pszTemp, &pszExe[1], MAX_CMD_LENGTH);
	pszTemp[MAX_CMD_LENGTH-1] = '\0';
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
	printf("WNetGetUniversalName: '%s'\n unc: '%s'\n share: '%s'\n path: '%s'\n", 
	    pszExe, 
	    info->lpUniversalName, 
	    info->lpConnectionName, 
	    info->lpRemainingPath);
    }
    else
	printf("WNetGetUniversalName: '%s'\n error %d\n", pszExe, dwResult);
    return true;
}
*/

// Function name	: NeedToMap
// Description	    : 
// Return type		: bool 
// Argument         : char *pszFullPath
// Argument         : char *pDrive
// Argument         : char *pszShare
// Argument         : char *pszDir
bool NeedToMap(char *pszFullPath, char *pDrive, char *pszShare)//, char *pszDir)
{
    DWORD dwResult;
    DWORD dwLength;
    char pBuffer[4096];
    REMOTE_NAME_INFO *info = (REMOTE_NAME_INFO*)pBuffer;
    char pszTemp[MAX_CMD_LENGTH];

    if (*pszFullPath == '"')
    {
	strncpy(pszTemp, &pszFullPath[1], MAX_CMD_LENGTH);
	pszTemp[MAX_CMD_LENGTH-1] = '\0';
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
	/*
	printf("WNetGetUniversalName: '%s'\n unc: '%s'\n share: '%s'\n path: '%s'\n", 
	    pszFullPath, 
	    info->lpUniversalName, 
	    info->lpConnectionName, 
	    info->lpRemainingPath);
	*/
	*pDrive = *pszFullPath;
	strcpy(pszShare, info->lpConnectionName);
	//strcpy(pszDir, info->lpRemainingPath);
	return true;
    }

    //printf("WNetGetUniversalName: '%s'\n error %d\n", pszExe, dwResult);
    return false;
}

// Function name	: ExeToUnc
// Description	    : 
// Return type		: void 
// Argument         : char *pszExe
void ExeToUnc(char *pszExe)
{
    DWORD dwResult;
    DWORD dwLength;
    char pBuffer[4096];
    REMOTE_NAME_INFO *info = (REMOTE_NAME_INFO*)pBuffer;
    char pszTemp[MAX_CMD_LENGTH];
    bool bQuoted = false;
    char *pszOriginal;

    pszOriginal = pszExe;

    if (*pszExe == '"')
    {
	bQuoted = true;
	strncpy(pszTemp, &pszExe[1], MAX_CMD_LENGTH);
	pszTemp[MAX_CMD_LENGTH-1] = '\0';
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
	/*
	printf("WNetGetUniversalName: '%s'\n unc: '%s'\n share: '%s'\n path: '%s'\n", 
	    pszExe, 
	    info->lpUniversalName, 
	    info->lpConnectionName, 
	    info->lpRemainingPath);
	*/
	if (bQuoted)
	    sprintf(pszOriginal, "\"%s\"", info->lpUniversalName);
	else
	    strcpy(pszOriginal, info->lpUniversalName);
    }
}

static void StripArgs(int &argc, char **&argv, int n)
{
    if (n+1 > argc)
    {
	printf("Error: cannot strip %d args, only %d left.\n", n, argc-1);
    }
    for (int i=n+1; i<=argc; i++)
    {
	argv[i-n] = argv[i];
    }
    argc -= n;
}

static bool isnumber(char *str)
{
    int i, n = strlen(str);
    for (i=0; i<n; i++)
    {
	if (!isdigit(str[i]))
	    return false;
    }
    return true;
}

bool CreatePMIDatabase(char *pmi_host, int pmi_port, char *phrase, char *pmi_kvsname)
{
    int error;
    SOCKET sock;
    if ((error = ConnectToMPD(pmi_host, pmi_port, phrase, &sock)) == 0)
    {
	WriteString(sock, "dbcreate");
	ReadString(sock, pmi_kvsname);
	WriteString(sock, "done");
	easy_closesocket(sock);
	return true;
    }

    printf("Unable to connect to mpd at %s:%d\n", pmi_host, pmi_port);
    return false;
}

bool DestroyPMIDatabase(char *pmi_host, int pmi_port, char *phrase, char *pmi_kvsname)
{
    int error;
    SOCKET sock;
    char str[256];

    if ((error = ConnectToMPD(pmi_host, pmi_port, phrase, &sock)) == 0)
    {
	sprintf(str, "dbdestroy %s", pmi_kvsname);
	WriteString(sock, str);
	ReadString(sock, str);
	WriteString(sock, "done");
	easy_closesocket(sock);
	return true;
    }

    printf("Unable to connect to mpd at %s:%d\n", pmi_host, pmi_port);
    return false;
}

// Function name	: main
// Description	    : 
// Return type		: void 
// Argument         : int argc
// Argument         : char *argv[]
int  main(int argc, char *argv[])
{
    int i;
    int iproc = 0;
    char pszEnv[MAX_CMD_LENGTH] = "";
    HANDLE *pThread = NULL;
    int nShmLow, nShmHigh;
    DWORD dwThreadID;
    bool bLogon = false;
    char pBuffer[MAX_CMD_LENGTH];
    char phrase[MPD_PASSPHRASE_MAX_LENGTH + 1];// = MPD_DEFAULT_PASSPHRASE;
    bool bLogonDots = true;
    HANDLE hStdout;
    char cMapDrive, pszMapShare[MAX_PATH];
    int nArgsToStrip;
    bool bRunLocal;
    char pszMachineFileName[MAX_PATH] = "";
    bool bUseMachineFile;
    bool bDoSMP;
    bool bPhraseNeeded;
    DWORD dwType;

    if (argc < 2)
    {
	PrintOptions();
	return 0;
    }

    SetConsoleCtrlHandler(CtrlHandlerRoutine, TRUE);

    DWORD pmi_host_length = MAX_HOST_LENGTH;
    GetComputerName(pmi_host, &pmi_host_length);

    // Set defaults
    g_bDoMultiColorOutput = true;
    bRunLocal = false;
    g_bNoMPI = false;
    bLogon = false;
    bLogonDots = true;
    GetCurrentDirectory(MAX_PATH, g_pszDir);
    bUseMachineFile = false;
    bDoSMP = true;
    phrase[0] = '\0';
    bPhraseNeeded = true;
    g_nHosts = 0;
    g_pHosts = NULL;

    // Parse mpirun options
    while (argv[1] && (argv[1][0] == '-' || argv[1][0] == '/'))
    {
	nArgsToStrip = 1;
	if (stricmp(&argv[1][1], "np") == 0)
	{
	    if (argc < 3)
	    {
		printf("Error: no number specified after -np option.\n");
		return 0;
	    }
	    g_nHosts = atoi(argv[2]);
	    if (g_nHosts < 1)
	    {
		printf("Error: must specify a number greater than 0 after the -np option\n");
		return 0;
	    }
	    nArgsToStrip = 2;
	}
	else if (stricmp(&argv[1][1], "localonly") == 0)
	{
	    bRunLocal = true;
	    if (argc > 2)
	    {
		if (isnumber(argv[2]))
		{
		    g_nHosts = atoi(argv[2]);
		    if (g_nHosts < 1)
		    {
			printf("Error: If you specify a number after -localonly option,\n        it must be greater than 0.\n");
			return 0;
		    }
		    nArgsToStrip = 2;
		}
	    }
	}
	else if (stricmp(&argv[1][1], "machinefile") == 0)
	{
	    if (argc < 3)
	    {
		printf("Error: no filename specified after -machinefile option.\n");
		return 0;
	    }
	    strcpy(pszMachineFileName, argv[2]);
	    bUseMachineFile = true;
	    nArgsToStrip = 2;
	}
	else if (stricmp(&argv[1][1], "map") == 0)
	{
	    if (argc < 3)
	    {
		printf("Error: no drive specified after -map option.\n");
		return 0;
	    }
	    if ((strlen(argv[2]) > 2) && argv[2][1] == ':')
	    {
		MapDriveNode *pNode = new MapDriveNode;
		pNode->cDrive = argv[2][0];
		strcpy(pNode->pszShare, &argv[2][2]);
		pNode->pNext = g_pDriveMapList;
		g_pDriveMapList = pNode;
	    }
	    nArgsToStrip = 2;
	}
	else if (stricmp(&argv[1][1], "dir") == 0)
	{
	    if (argc < 3)
	    {
		printf("Error: no directory after -dir option\n");
		return 0;
	    }
	    strcpy(g_pszDir, argv[2]);
	    nArgsToStrip = 2;
	}
	else if (stricmp(&argv[1][1], "env") == 0)
	{
	    if (argc < 3)
	    {
		printf("Error: no environment variables after -env option\n");
		return 0;
	    }
	    strncpy(g_pszEnv, argv[2], MAX_CMD_LENGTH);
	    g_pszEnv[MAX_CMD_LENGTH-1] = '\0';
	    if (strlen(argv[2]) >= MAX_CMD_LENGTH)
	    {
		printf("Warning: environment variables truncated.\n");
	    }
	    nArgsToStrip = 2;
	}
	else if (stricmp(&argv[1][1], "logon") == 0)
	{
	    bLogon = true;
	}
	else if (stricmp(&argv[1][1], "hosts") == 0)
	{
	    if (g_nHosts != 0)
	    {
		printf("Error: only one option is allowed to determine the number of processes.\n");
		printf("       -hosts cannot be used with -np or -localonly\n");
		return 0;
	    }
	    if (argc > 2)
	    {
		if (isnumber(argv[2]))
		{
		    g_nHosts = atoi(argv[2]);
		    if (g_nHosts < 1)
		    {
			printf("Error: You must specify a number greater than 0 after -hosts.\n");
			return 0;
		    }
		    nArgsToStrip = 2 + g_nHosts;
		    int index = 3;
		    for (i=0; i<g_nHosts; i++)
		    {
			if (index >= argc)
			{
			    printf("Error: missing host name after -hosts option.\n");
			    return 0;
			}
			HostNode *pNode = new HostNode;
			pNode->next = NULL;
			pNode->nSMPProcs = 1;
			pNode->exe[0] = '\0';
			strcpy(pNode->host, argv[index]);
			index++;
			if (argc > index)
			{
			    if (isnumber(argv[index]))
			    {
				pNode->nSMPProcs = atoi(argv[index]);
				index++;
				nArgsToStrip++;
			    }
			}
			if (g_pHosts == NULL)
			{
			    g_pHosts = pNode;
			}
			else
			{
			    HostNode *pIter = g_pHosts;
			    while (pIter->next)
				pIter = pIter->next;
			    pIter->next = pNode;
			}
		    }
		}
		else
		{
		    printf("Error: You must specify the number of hosts after the -hosts option.\n");
		    return 0;
		}
	    }
	    else
	    {
		printf("Error: not enough arguments.\n");
		return 0;
	    }
	}
	else if (stricmp(&argv[1][1], "tcp") == 0)
	{
	    bDoSMP = false;
	}
	else if (stricmp(&argv[1][1], "getphrase") == 0)
	{
	    GetMPDPassPhrase(phrase);
	    bPhraseNeeded = false;
	}
	else if (stricmp(&argv[1][1], "nocolor") == 0)
	{
	    g_bDoMultiColorOutput = false;
	}
	else if (stricmp(&argv[1][1], "nompi") == 0)
	{
	    g_bNoMPI = true;
	}
	else if (stricmp(&argv[1][1], "nodots") == 0)
	{
	    bLogonDots = false;
	}
	else if (stricmp(&argv[1][1], "nomapping") == 0)
	{
	    g_bNoDriveMapping = true;
	}
	else if (stricmp(&argv[1][1], "nopopup_debug") == 0)
	{
	    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
	}
	else if (stricmp(&argv[1][1], "help") == 0 || argv[1][1] == '?')
	{
	    PrintOptions();
	    return 0;
	}
	else if (stricmp(&argv[1][1], "help2") == 0)
	{
	    PrintExtraOptions();
	    return 0;
	}
	else
	{
	    printf("Unknown option: %s\n", argv[1]);
	}
	StripArgs(argc, argv, nArgsToStrip);
    }

    if (argc < 2)
    {
	printf("Error: no executable or configuration file specified\n");
	return 0;
    }

    // The next argument is the executable or a configuration file
    strncpy(g_pszExe, argv[1], MAX_CMD_LENGTH);
    g_pszExe[MAX_CMD_LENGTH-1] = '\0';

    // All the rest of the arguments are passed to the application
    g_pszArgs[0] = '\0';
    for (i = 2; i<argc; i++)
    {
	strncat(g_pszArgs, argv[i], MAX_CMD_LENGTH - 1 - strlen(g_pszArgs));
	if (i < argc-1)
	{
	    strncat(g_pszArgs, " ", MAX_CMD_LENGTH - 1 - strlen(g_pszArgs));
	}
    }

    if (g_nHosts == 0)
    {
	// If -np or -localonly options have not been specified, check if the first
	// parameter is an executable or a configuration file
	if (GetBinaryType(g_pszExe, &dwType) || (ParseConfigFile(g_pszExe) == PARSE_ERR_NO_FILE))
	{
	    g_nHosts = 1;
	    bRunLocal = true;
	}
    }

    // Fix up the executable name
    char pszTempExe[MAX_CMD_LENGTH], *namepart;
    if (g_pszExe[0] == '\\' && g_pszExe[1] == '\\')
    {
	strncpy(pszTempExe, g_pszExe, MAX_CMD_LENGTH);
	pszTempExe[MAX_CMD_LENGTH-1] = '\0';
    }
    else
	GetFullPathName(g_pszExe, MAX_PATH, pszTempExe, &namepart);
    // Quote the executable in case there are spaces in the path
    sprintf(g_pszExe, "\"%s\"", pszTempExe);

    easy_socket_init();

    if (!bRunLocal && g_pHosts == NULL)
    {
	// Save the original file name in case we end up running locally
	strncpy(pszTempExe, g_pszExe, MAX_CMD_LENGTH);
	pszTempExe[MAX_CMD_LENGTH-1] = '\0';

	// Convert the executable to its unc equivalent. This negates
	// the need to map network drives on remote machines just to locate
	// the executable.
	ExeToUnc(g_pszExe);

	// If we are not running locally and the hosts haven't been setup with a configuration file,
	// create the host list now
	if (bUseMachineFile)
	{
	    if (!GetHostsFromFile(pszMachineFileName))
	    {
		printf("Error parsing the machine file '%s'\n", pszMachineFileName);
		return 0;
	    }
	}
	else if (!GetAvailableHosts())
	{
	    strncpy(g_pszExe, pszTempExe, MAX_CMD_LENGTH);
	    g_pszExe[MAX_CMD_LENGTH-1] = '\0';
	    bRunLocal = true;
	}
    }

    // Setup multi-color output
    if (g_bDoMultiColorOutput)
    {
	char pszTemp[10];
	DWORD len = 10;
	if (ReadMPDRegistry("color", pszTemp, &len))
	{
	    g_bDoMultiColorOutput = (stricmp(pszTemp, "yes") == 0);
	}
    }
    if (g_bDoMultiColorOutput)
    {
	CONSOLE_SCREEN_BUFFER_INFO info;
	// Save the state of the console so it can be restored
	hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
	GetConsoleScreenBufferInfo(hStdout, &info);
	g_ConsoleAttribute = info.wAttributes;
    }
    
    // Check if the directory needs to be mapped on the remote machines
    if (NeedToMap(g_pszDir, &cMapDrive, pszMapShare))
    {
	MapDriveNode *pNode = new MapDriveNode;
	pNode->cDrive = cMapDrive;
	strcpy(pNode->pszShare, pszMapShare);
	pNode->pNext = g_pDriveMapList;
	g_pDriveMapList = pNode;
    }

    // If -getphrase was not specified, get the mpd passphrase from
    // the registry or use the default
    if (bPhraseNeeded)
    {
	if (!ReadMPDRegistry("phrase", phrase, NULL))
	{
	    strcpy(phrase, MPD_DEFAULT_PASSPHRASE);
	}
    }

    char pmi_port_str[100];
    DWORD port_str_length = 100;
    if (ReadMPDRegistry("port", pmi_port_str, &port_str_length))
    {
	pmi_port = atoi(pmi_port_str);
	if (pmi_port < 1)
	    pmi_port = MPD_DEFAULT_PORT;
    }
    CreatePMIDatabase(pmi_host, pmi_port, phrase, pmi_kvsname);

    if (bRunLocal)
    {
	RunLocal(bDoSMP);
	DestroyPMIDatabase(pmi_host, pmi_port, phrase, pmi_kvsname);
	easy_socket_finalize();
	return 0;
    }

    //dbg_printf("retrieving account information\n");
    if (bLogon)
	GetAccountAndPassword();
    else
    {
	char pszTemp[10] = "no";
	ReadMPDRegistry("SingleUser", pszTemp, NULL);
	if (stricmp(pszTemp, "yes"))
	{
	    if (bLogonDots)
	    {
		DWORD dwThreadId;
		HANDLE hEvent, hDotThread;
		hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		hDotThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)PrintDots, hEvent, 0, &dwThreadId);
		if (!ReadPasswordFromRegistry(g_pszAccount, g_pszPassword))
		{
		    SetEvent(hEvent);
		    GetAccountAndPassword();
		}
		else
		    SetEvent(hEvent);
		CloseHandle(hDotThread);
	    }
	    else
	    {
		if (!ReadPasswordFromRegistry(g_pszAccount, g_pszPassword))
		{
		    GetAccountAndPassword();
		}
	    }
	    bLogon = true;
	}
    }
    
    // Figure out how many processes to launch
    int nProc = 0;
    HostNode *n = g_pHosts;
    if (g_pHosts == NULL)
	nProc = g_nHosts;
    while (n)
    {
	nProc += n->nSMPProcs;
	n = n->next;
    }
    g_nNproc = nProc;
    
    // Set the environment variables common to all processes
    if (g_bNoMPI)
	pszEnv[0] = '\0';
    else
    {
	sprintf(pszEnv, "PMI_SIZE=%d|PMI_MPD=%s:%d|PMI_KVS=%s", nProc, pmi_host, pmi_port, pmi_kvsname);
    }
    
    // Allocate an array to hold handles to the LaunchProcess threads, sockets, ids, ranks, and forward host structures
    pThread = new HANDLE[nProc];
    g_pProcessSocket = new SOCKET[nProc];
    for (i=0; i<nProc; i++)
	g_pProcessSocket[i] = INVALID_SOCKET;
    g_pProcessLaunchId = new int[nProc];
    g_pLaunchIdToRank = new int [nProc];
    g_nNumProcessSockets = 0;
    g_pForwardHost = new ForwardHostStruct[nProc];
    for (i=0; i<nProc; i++)
	g_pForwardHost[i].nPort = 0;
    
    // Start the IO redirection thread
    HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    g_hRedirectIOListenThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)RedirectIOThread, hEvent, 0, &dwThreadID);
    if (g_hRedirectIOListenThread)
    {
	if (WaitForSingleObject(hEvent, 10000) != WAIT_OBJECT_0)
	{
	    printf("RedirectIOThread failed to initialize\n");
	    DestroyPMIDatabase(pmi_host, pmi_port, phrase, pmi_kvsname);
	    return 0;
	}
    }
    else
    {
	printf("Unable to create RedirectIOThread, error %d\n", GetLastError());
	DestroyPMIDatabase(pmi_host, pmi_port, phrase, pmi_kvsname);
	return 0;
    }
    CloseHandle(hEvent);

    strncpy(g_pForwardHost[0].pszHost, g_pszIOHost, MAX_HOST_LENGTH);
    g_pForwardHost[0].pszHost[MAX_HOST_LENGTH-1] = '\0';
    g_pForwardHost[0].nPort = g_nIOPort;
    //printf("io redirection: %s:%d\n", g_pForwardHost[0].pszHost, g_pForwardHost[0].nPort);fflush(stdout);

    // Launch the threads to launch the processes
    iproc = 0;
    while (g_pHosts)
    {
	nShmLow = iproc;
	nShmHigh = iproc + g_pHosts->nSMPProcs - 1;
	for (int i = 0; i<g_pHosts->nSMPProcs; i++)
	{
	    MPIRunLaunchProcessArg *arg = new MPIRunLaunchProcessArg;
	    sprintf(arg->pszIOHostPort, "%s:%d", g_pszIOHost, g_nIOPort);
	    strcpy(arg->pszPassPhrase, phrase);
	    arg->i = iproc;
	    arg->bLogon = bLogon;
	    if (bLogon)
	    {
		strcpy(arg->pszAccount, g_pszAccount);
		strcpy(arg->pszPassword, g_pszPassword);
	    }
	    else
	    {
		arg->pszAccount[0] = '\0';
		arg->pszPassword[0] = '\0';
	    }
	    if (strlen(g_pHosts->exe) > 0)
	    {
		strncpy(arg->pszCmdLine, g_pHosts->exe, MAX_CMD_LENGTH);
		arg->pszCmdLine[MAX_CMD_LENGTH-1] = '\0';
	    }
	    else
	    {
		strncpy(arg->pszCmdLine, g_pszExe, MAX_CMD_LENGTH);
		arg->pszCmdLine[MAX_CMD_LENGTH-1] = '\0';
	    }
	    if (strlen(g_pszArgs) > 0)
	    {
		strncat(arg->pszCmdLine, " ", MAX_CMD_LENGTH - 1 - strlen(arg->pszCmdLine));
		strncat(arg->pszCmdLine, g_pszArgs, MAX_CMD_LENGTH - 1 - strlen(arg->pszCmdLine));
	    }
	    strcpy(arg->pszDir, g_pszDir);
	    if (strlen(pszEnv) >= MAX_CMD_LENGTH)
	    {
		printf("Warning: environment variables truncated.\n");
		fflush(stdout);
	    }
	    strncpy(arg->pszEnv, pszEnv, MAX_CMD_LENGTH);
	    arg->pszEnv[MAX_CMD_LENGTH-1] = '\0';
	    strncpy(arg->pszHost, g_pHosts->host, MAX_HOST_LENGTH);
	    arg->pszHost[MAX_HOST_LENGTH-1] = '\0';

	    if (g_bNoMPI)
	    {
		if (strlen(g_pszEnv) >= MAX_CMD_LENGTH)
		{
		    printf("Warning: environment variables truncated.\n");
		    fflush(stdout);
		}
		strncpy(arg->pszEnv, g_pszEnv, MAX_CMD_LENGTH);
		arg->pszEnv[MAX_CMD_LENGTH-1] = '\0';
	    }
	    else
	    {
		sprintf(pBuffer, "PMI_RANK=%d|PMI_SHM_LOW=%d|PMI_SHM_HIGH=%d", iproc, nShmLow, nShmHigh);
		if (strlen(arg->pszEnv) > 0)
		    strncat(arg->pszEnv, "|", MAX_CMD_LENGTH - 1 - strlen(arg->pszEnv));
		if (strlen(pBuffer) + strlen(arg->pszEnv) >= MAX_CMD_LENGTH)
		{
		    printf("Warning: environment variables truncated.\n");
		    fflush(stdout);
		}
		strncat(arg->pszEnv, pBuffer, MAX_CMD_LENGTH - 1 - strlen(arg->pszEnv));
		
		if (strlen(g_pszEnv) > 0)
		{
		    if (strlen(arg->pszEnv) + strlen(g_pszEnv) + 1 >= MAX_CMD_LENGTH)
		    {
			printf("Warning: environment variables truncated.\n");
		    }
		    strncat(arg->pszEnv, "|", MAX_CMD_LENGTH - 1 - strlen(arg->pszEnv));
		    strncat(arg->pszEnv, g_pszEnv, MAX_CMD_LENGTH - 1 - strlen(arg->pszEnv));
		}
	    }
	    //printf("creating MPIRunLaunchProcess thread\n");fflush(stdout);
	    pThread[iproc] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)MPIRunLaunchProcess, arg, 0, &dwThreadID);
	    if (pThread[iproc] == NULL)
	    {
		printf("Unable to create LaunchProcess thread\n");fflush(stdout);
		// Signal launch threads to abort
		// Wait for them to return
		
		// ... insert code here

		// In the mean time, just exit
		if (g_bDoMultiColorOutput)
		{
		    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), g_ConsoleAttribute);
		}
		ExitProcess(1);
	    }
	    iproc++;
	}
	
	HostNode *n = g_pHosts;
	g_pHosts = g_pHosts->next;
	delete n;
    }

    //printf("Waiting for processes\n");fflush(stdout);
    // Wait for all the process launching threads to complete
    WaitForLotsOfObjects(nProc, pThread);
    for (i = 0; i<nProc; i++)
	CloseHandle(pThread[i]);
    delete pThread;
    pThread = NULL;

    if (WaitForSingleObject(g_hAbortEvent, 0) == WAIT_OBJECT_0)
    {
	char pszStr[100];
	for (i=0; i<nProc; i++)
	{
	    if (g_pProcessSocket[i] != INVALID_SOCKET)
	    {
		sprintf(pszStr, "kill %d", g_pProcessLaunchId[i]);
		WriteString(g_pProcessSocket[i], pszStr);
		if (!UnmapDrives(g_pProcessSocket[i]))
		{
		    printf("Drive unmappings failed\n");
		}
		sprintf(pszStr, "freeprocess %d", g_pProcessLaunchId[i]);
		WriteString(g_pProcessSocket[i], pszStr);
		WriteString(g_pProcessSocket[i], "done");
		easy_closesocket(g_pProcessSocket[i]);
	    }
	}
	ExitProcess(0);
    }
    // Note: If the user hits Ctrl-C between the above if statement and the following ResetEvent statement
    // nothing will happen and the user will have to hit Ctrl-C again.
    ResetEvent(g_hLaunchThreadsRunning);

    //printf("Waiting for exit codes\n");fflush(stdout);
    // Wait for the mpds to return the exit codes of all the processes
    WaitForExitCommands();

    delete g_pForwardHost;
    g_pForwardHost = NULL;

    // Signal the IO redirection thread to stop
    char ch = 0;
    easy_send(g_sockStopIOSignalSocket, &ch, 1);

    //printf("Waiting for redirection thread to exit\n");fflush(stdout);
    // Wait for the redirection thread to complete.  Kill it if it takes too long.
    if (WaitForSingleObject(g_hRedirectIOListenThread, 10000) != WAIT_OBJECT_0)
    {
	//printf("Terminating the IO redirection control thread\n");
	TerminateThread(g_hRedirectIOListenThread, 0);
    }
    CloseHandle(g_hRedirectIOListenThread);
    easy_closesocket(g_sockStopIOSignalSocket);
    CloseHandle(g_hAbortEvent);

    if (g_bDoMultiColorOutput)
    {
	SetConsoleTextAttribute(hStdout, g_ConsoleAttribute);
    }
    DestroyPMIDatabase(pmi_host, pmi_port, phrase, pmi_kvsname);
    easy_socket_finalize();

    delete g_pProcessSocket;
    delete g_pProcessLaunchId;
    delete g_pLaunchIdToRank;

    while (g_pDriveMapList)
    {
	MapDriveNode *pNode = g_pDriveMapList;
	g_pDriveMapList = g_pDriveMapList->pNext;
	delete pNode;
    }

    return 0;
}

struct ProcessWaitAbortThreadArg
{
    SOCKET sockAbort;
    SOCKET sockStop;
    int n;
    SOCKET *pSocket;
};

// Function name	: ProcessWaitAbort
// Description	    : 
// Return type		: void 
// Argument         : ProcessWaitAbortThreadArg *pArg
void ProcessWaitAbort(ProcessWaitAbortThreadArg *pArg)
{
    int n, i;
    fd_set readset;

    FD_ZERO(&readset);
    FD_SET(pArg->sockAbort, &readset);
    FD_SET(pArg->sockStop, &readset);

    n = select(0, &readset, NULL, NULL, NULL);

    if (n == SOCKET_ERROR)
    {
	printf("bselect failed, error %d\n", WSAGetLastError());fflush(stdout);
	for (i=0; i<pArg->n; i++)
	{
	    easy_closesocket(pArg->pSocket[i]);
	}
	easy_closesocket(pArg->sockAbort);
	easy_closesocket(pArg->sockStop);
	return;
    }
    if (n == 0)
    {
	printf("ProcessWaitAbort: bselect returned zero sockets available\n");fflush(stdout);
	for (i=0; i<pArg->n; i++)
	{
	    easy_closesocket(pArg->pSocket[i]);
	}
	easy_closesocket(pArg->sockAbort);
	easy_closesocket(pArg->sockStop);
	return;
    }
    if (FD_ISSET(pArg->sockAbort, &readset))
    {
	for (i=0; i<pArg->n; i++)
	{
	    easy_send(pArg->pSocket[i], "x", 1);
	}
    }
    for (i=0; i<pArg->n; i++)
    {
	easy_closesocket(pArg->pSocket[i]);
    }
    easy_closesocket(pArg->sockAbort);
    easy_closesocket(pArg->sockStop);
}

// Function name	: UnmapDrives
// Description	    : 
// Return type		: bool 
// Argument         : int sock
bool UnmapDrives(SOCKET sock)
{
    char pszStr[256];
    if (g_pDriveMapList && !g_bNoDriveMapping)
    {
	MapDriveNode *pNode = g_pDriveMapList;
	while (pNode)
	{
	    sprintf(pszStr, "unmap drive=%c", pNode->cDrive);
	    if (WriteString(sock, pszStr) == SOCKET_ERROR)
	    {
		printf("ERROR: Unable to send unmap command, Error %d", WSAGetLastError());
		easy_closesocket(sock);
		SetEvent(g_hAbortEvent);
		return false;
	    }
	    if (!ReadString(sock, pszStr))
	    {
		printf("ERROR: Unable to read the result of unmap command, Error %d", WSAGetLastError());
		easy_closesocket(sock);
		SetEvent(g_hAbortEvent);
		return false;
	    }
	    if (stricmp(pszStr, "SUCCESS"))
	    {
		printf("ERROR: Unable to unmap %c: %s\r\n%s", pNode->cDrive, pNode->pszShare, pszStr);
		easy_closesocket(sock);
		SetEvent(g_hAbortEvent);
		return false;
	    }
	    pNode = pNode->pNext;
	}
    }
    return true;
}

struct ProcessWaitThreadArg
{
    int n;
    SOCKET *pSocket;
    int *pId;
    int *pRank;
    SOCKET sockAbort;
};

// Function name	: ProcessWait
// Description	    : 
// Return type		: void 
// Argument         : ProcessWaitThreadArg *pArg
void ProcessWait(ProcessWaitThreadArg *pArg)
{
    int i, j, n;
    fd_set totalset, readset;
    char str[256];
    
    FD_ZERO(&totalset);
    
    FD_SET(pArg->sockAbort, &totalset);
    for (i=0; i<pArg->n; i++)
    {
	FD_SET(pArg->pSocket[i], &totalset);
    }
    
    while (pArg->n)
    {
	readset = totalset;
	n = select(0, &readset, NULL, NULL, NULL);
	if (n == SOCKET_ERROR)
	{
	    printf("select failed, error %d\n", WSAGetLastError());fflush(stdout);
	    for (i=0, j=0; i<pArg->n; i++, j++)
	    {
		while (pArg->pSocket[j] == INVALID_SOCKET)
		    j++;
		easy_closesocket(pArg->pSocket[j]);
	    }
	    return;
	}
	if (n == 0)
	{
	    printf("WaitForExitCommands: select returned zero sockets available");fflush(stdout);
	    for (i=0, j=0; i<pArg->n; i++, j++)
	    {
		while (pArg->pSocket[j] == INVALID_SOCKET)
		    j++;
		easy_closesocket(pArg->pSocket[j]);
	    }
	    return;
	}

	if (FD_ISSET(pArg->sockAbort, &readset))
	{
	    for (i=0; pArg->n > 0; i++)
	    {
		while (pArg->pSocket[i] == INVALID_SOCKET)
		    i++;
		sprintf(str, "kill %d", pArg->pId[i]);
		WriteString(pArg->pSocket[i], str);

		int nRank = pArg->pRank[i];
		if (g_nNproc > FORWARD_NPROC_THRESHOLD)
		{
		    if (nRank > 0 && (g_nNproc/2) > nRank)
		    {
			//printf("rank %d(%d) stopping forwarder\n", nRank, g_pProcessLaunchId[i]);fflush(stdout);
			sprintf(str, "stopforwarder port=%d abort=yes", g_pForwardHost[nRank].nPort);
			WriteString(pArg->pSocket[i], str);
		    }
		}

		sprintf(str, "freeprocess %d", pArg->pId[i]);
		WriteString(pArg->pSocket[i], str);
		WriteString(pArg->pSocket[i], "done");
		easy_closesocket(pArg->pSocket[i]);
		pArg->pSocket[i] = INVALID_SOCKET;
		pArg->n--;
	    }
	    return;
	}
	for (i=0; n>0; i++)
	{
	    while (pArg->pSocket[i] == INVALID_SOCKET)
		i++;
	    if (FD_ISSET(pArg->pSocket[i], &readset))
	    {
		if (!ReadString(pArg->pSocket[i], str))
		{
		    printf("Unable to read the result of the getexitcodewait command for process %d, error %d", i, WSAGetLastError());fflush(stdout);
		    return;
		}
		
		int nRank = pArg->pRank[i];
		if (g_nNproc > FORWARD_NPROC_THRESHOLD)
		{
		    if (nRank > 0 && (g_nNproc/2) > nRank)
		    {
			sprintf(str, "stopforwarder port=%d abort=no", g_pForwardHost[nRank].nPort);
			WriteString(pArg->pSocket[i], str);
		    }
		}
		
		UnmapDrives(pArg->pSocket[i]);

		sprintf(str, "freeprocess %d", pArg->pId[i]);
		WriteString(pArg->pSocket[i], str);
		
		WriteString(pArg->pSocket[i], "done");
		easy_closesocket(pArg->pSocket[i]);
		FD_CLR(pArg->pSocket[i], &totalset);
		pArg->pSocket[i] = INVALID_SOCKET;
		n--;
		pArg->n--;
	    }
	}
    }
}

// Function name	: WaitForExitCommands
// Description	    : 
// Return type		: void 
void WaitForExitCommands()
{
    if (g_nNumProcessSockets < FD_SETSIZE)
    {
	int i, n;
	fd_set totalset, readset;
	char str[256];
	SOCKET break_sock;
	
	MakeLoop(&break_sock, &g_sockBreak);
	SetEvent(g_hBreakReadyEvent);

	FD_ZERO(&totalset);
	
	FD_SET(break_sock, &totalset);
	for (i=0; i<g_nNumProcessSockets; i++)
	{
	    FD_SET(g_pProcessSocket[i], &totalset);
	}
	
	while (g_nNumProcessSockets)
	{
	    readset = totalset;
	    n = select(0, &readset, NULL, NULL, NULL);
	    if (n == SOCKET_ERROR)
	    {
		printf("WaitForExitCommands: select failed, error %d\n", WSAGetLastError());fflush(stdout);
		for (i=0; g_nNumProcessSockets > 0; i++)
		{
		    while (g_pProcessSocket[i] == INVALID_SOCKET)
			i++;
		    easy_closesocket(g_pProcessSocket[i]);
		    g_nNumProcessSockets--;
		}
		return;
	    }
	    if (n == 0)
	    {
		printf("WaitForExitCommands: select returned zero sockets available\n");fflush(stdout);
		for (i=0; g_nNumProcessSockets > 0; i++)
		{
		    while (g_pProcessSocket[i] == INVALID_SOCKET)
			i++;
		    easy_closesocket(g_pProcessSocket[i]);
		    g_nNumProcessSockets--;
		}
		return;
	    }
	    else
	    {
		if (FD_ISSET(break_sock, &readset))
		{
		    int num_read = easy_receive(break_sock, str, 1);
		    if (num_read == 0 || num_read == SOCKET_ERROR)
		    {
			FD_CLR(break_sock, &totalset);
		    }
		    else
		    {
			printf("Sending kill commands to launched processes\n");fflush(stdout);
			for (int j=0, i=0; i<g_nNumProcessSockets; i++, j++)
			{
			    while (g_pProcessSocket[j] == INVALID_SOCKET)
				j++;
			    sprintf(str, "kill %d", g_pProcessLaunchId[j]);
			    //printf("%s\n", str);fflush(stdout);
			    WriteString(g_pProcessSocket[j], str);
			}
		    }
		    n--;
		}
		for (i=0; n>0; i++)
		{
		    while (g_pProcessSocket[i] == INVALID_SOCKET)
			i++;
		    if (FD_ISSET(g_pProcessSocket[i], &readset))
		    {
			if (!ReadString(g_pProcessSocket[i], str))
			{
			    printf("Unable to read the result of the getexitcodewait command for process %d, error %d", i, WSAGetLastError());fflush(stdout);
			    return;
			}
			
			int nRank = g_pLaunchIdToRank[i];
			if (g_nNproc > FORWARD_NPROC_THRESHOLD)
			{
			    if (nRank > 0 && (g_nNproc/2) > nRank)
			    {
				//printf("rank %d(%d) stopping forwarder\n", nRank, g_pProcessLaunchId[i]);fflush(stdout);
				sprintf(str, "stopforwarder port=%d abort=no", g_pForwardHost[nRank].nPort);
				WriteString(g_pProcessSocket[i], str);
			    }
			}
			
			UnmapDrives(g_pProcessSocket[i]);

			sprintf(str, "freeprocess %d", g_pProcessLaunchId[i]);
			WriteString(g_pProcessSocket[i], str);

			WriteString(g_pProcessSocket[i], "done");
			easy_closesocket(g_pProcessSocket[i]);
			FD_CLR(g_pProcessSocket[i], &totalset);
			g_pProcessSocket[i] = INVALID_SOCKET;
			n--;
			g_nNumProcessSockets--;
			//printf("(E:%d)", g_pProcessLaunchId[i]);fflush(stdout);
		    }
		}
	    }
	}
	
	easy_closesocket(g_sockBreak);
	g_sockBreak = INVALID_SOCKET;
	delete g_pProcessSocket;
	delete g_pProcessLaunchId;
	delete g_pLaunchIdToRank;
	g_pProcessSocket = NULL;
	g_pProcessLaunchId = NULL;
	g_pLaunchIdToRank = NULL;
    }
    else
    {
	DWORD dwThreadID;
	int num = (g_nNumProcessSockets / (FD_SETSIZE-1)) + 1;
	HANDLE *hThread = new HANDLE[num];
	SOCKET *pAbortsock = new SOCKET[num];
	SOCKET sockStop;
	ProcessWaitThreadArg *arg = new ProcessWaitThreadArg[num];
	ProcessWaitAbortThreadArg *arg2 = new ProcessWaitAbortThreadArg;
        int i;
	for (i=0; i<num; i++)
	{
	    if (i == num-1)
		arg[i].n = g_nNumProcessSockets % (FD_SETSIZE-1);
	    else
		arg[i].n = (FD_SETSIZE-1);
	    arg[i].pSocket = &g_pProcessSocket[i*(FD_SETSIZE-1)];
	    arg[i].pId = &g_pProcessLaunchId[i*(FD_SETSIZE-1)];
	    arg[i].pRank = &g_pLaunchIdToRank[i*(FD_SETSIZE-1)];
	    MakeLoop(&arg[i].sockAbort, &pAbortsock[i]);
	}
	for (i=0; i<num; i++)
	{
	    hThread[i] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ProcessWait, &arg[i], 0, &dwThreadID);
	}
	MakeLoop(&arg2->sockAbort, &g_sockBreak);
	MakeLoop(&arg2->sockStop, &sockStop);

	HANDLE hWaitAbortThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ProcessWaitAbort, arg2, 0, &dwThreadID);

	SetEvent(g_hBreakReadyEvent);

	WaitForMultipleObjects(num, hThread, TRUE, INFINITE);
	for (i=0; i<num; i++)
	    CloseHandle(hThread[i]);
	delete hThread;
	delete arg;

	easy_send(sockStop, "x", 1);
	easy_closesocket(sockStop);
	WaitForSingleObject(hWaitAbortThread, 10000);
	delete pAbortsock;
	delete arg2;
	CloseHandle(hWaitAbortThread);

	easy_closesocket(g_sockBreak);
	g_sockBreak = INVALID_SOCKET;
	delete g_pProcessSocket;
	delete g_pProcessLaunchId;
	delete g_pLaunchIdToRank;
	g_pProcessSocket = NULL;
	g_pProcessLaunchId = NULL;
	g_pLaunchIdToRank = NULL;
    }
    //printf("WaitForExitCommands returning\n");fflush(stdout);
}
