/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpidi_ch3_impl.h"

#ifdef USE_SINGLE_MSG_QUEUE
#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/msg.h>
#endif

#ifdef HAVE_WINDOWS_H
#include <winsock2.h>
#include <windows.h>
#endif

/* STATES:NO WARNINGS */

/* FIXME: Are these needed here (these were defined within an unrelated 
   ifdef in mpidimpl.h) */
#ifdef HAVE_TIME_H
#include <time.h>
#endif
#ifdef HAVE_UUID_UUID_H
#include <uuid/uuid.h>
#endif

/* FIXME: These routines allocate memory but rarely free it.  They need
   a finalize handler to remove the allocated memory.  A clear description
   of the purpose of these routines and their scope of use would make that
   easier. */

/* FIXME: These routines should *first* decide on the method of shared
   memory and queue support, then implement the Bootstrap interface in
   terms of those routines, if necessary using large (but single)
   ifdef regions rather than the constant ifdef/endif code used here.
   In addition, common service routines should be defined so that the
   overall code can simply */

/* FIXME: These routines need a description and explanation.  
   For example, where is the bootstrap q pointer exported?  Is there
   more than one bootstrap queue?  */

#ifdef HAVE_WINDOWS_H

typedef struct bootstrap_msg
{
    int length;
    char buffer[1024];
    struct bootstrap_msg *next;
} bootstrap_msg;

#endif /* HAVE_WINDOWS_H */

typedef struct MPIDI_CH3I_BootstrapQ_struct
{
    char name[MPIDI_BOOTSTRAP_NAME_LEN];
#ifdef MPIDI_CH3_USES_SHM_NAME
    char shm_name[MPIDI_BOOTSTRAP_NAME_LEN];
#endif
#ifdef USE_SINGLE_MSG_QUEUE
    int pid;
    int id;
#elif defined(USE_SYSV_MQ) || defined(USE_POSIX_MQ)
    int id;
#elif defined (HAVE_WINDOWS_H)
    bootstrap_msg *msg_list;
    HWND hWnd;
    HANDLE hMsgThread;
    HANDLE hReadyEvent;
    HANDLE hMutex;
    HANDLE hMessageArrivedEvent;
    int error;
#else
#error *** No bootstrapping queue capability specified ***
#endif
    struct MPIDI_CH3I_BootstrapQ_struct *next;
} MPIDI_CH3I_BootstrapQ_struct;

static MPIDI_CH3I_BootstrapQ_struct * g_queue_list = NULL;

#ifdef HAVE_WINDOWS_H

#undef FUNCNAME
#define FUNCNAME GetNextBootstrapMsg
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int GetNextBootstrapMsg(MPIDI_CH3I_BootstrapQ queue, bootstrap_msg ** msg_ptr, BOOL blocking)
{
    MPIDI_STATE_DECL(MPID_STATE_GET_NEXT_BOOTSTRAP_MSG);
    MPIDI_FUNC_ENTER(MPID_STATE_GET_NEXT_BOOTSTRAP_MSG);
    while (WaitForSingleObject(queue->hMessageArrivedEvent, blocking ? INFINITE : 0) == WAIT_OBJECT_0)
    {
	WaitForSingleObject(queue->hMutex, INFINITE);
	if (queue->msg_list)
	{
	    *msg_ptr = queue->msg_list;
	    queue->msg_list = queue->msg_list->next;
	    if (queue->msg_list == NULL)
		ResetEvent(queue->hMessageArrivedEvent);
	    ReleaseMutex(queue->hMutex);
	    MPIDI_FUNC_EXIT(MPID_STATE_GET_NEXT_BOOTSTRAP_MSG);
	    return MPI_SUCCESS;
	}
	ReleaseMutex(queue->hMutex);
    }
    *msg_ptr = NULL;
    MPIDI_FUNC_EXIT(MPID_STATE_GET_NEXT_BOOTSTRAP_MSG);
    return MPI_SUCCESS;
}

static MPIDI_CH3I_BootstrapQ s_queue = NULL;

#undef FUNCNAME
#define FUNCNAME BootstrapQWndProc
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
LRESULT CALLBACK BootstrapQWndProc(
    HWND hwnd,        /* handle to window */
    UINT uMsg,        /* message identifier */
    WPARAM wParam,    /* first message parameter */
    LPARAM lParam)    /* second message parameter */
{
    PCOPYDATASTRUCT p;
    bootstrap_msg *bsmsg_ptr, *msg_iter;
    /*MPIDI_STATE_DECL(MPID_STATE_BOOTSTRAPQWNDPROC);
    MPIDI_FUNC_ENTER(MPID_STATE_BOOTSTRAPQWNDPROC);*/
    switch (uMsg)
    {
    case WM_DESTROY:
	PostQuitMessage(0);
	break;
    case WM_COPYDATA:
	/*printf("WM_COPYDATA received\n");fflush(stdout);*/
	p = (PCOPYDATASTRUCT) lParam;
	if (WaitForSingleObject(s_queue->hMutex, INFINITE) == WAIT_OBJECT_0)
	{
	    bsmsg_ptr = MPIU_Malloc(sizeof(bootstrap_msg));
	    if (bsmsg_ptr != NULL)
	    {
		MPIU_Memcpy(bsmsg_ptr->buffer, p->lpData, p->cbData);
		bsmsg_ptr->length = p->cbData;
		bsmsg_ptr->next = NULL;
		msg_iter = s_queue->msg_list;
		if (msg_iter == NULL)
		    s_queue->msg_list = bsmsg_ptr;
		else
		{
		    while (msg_iter->next != NULL)
			msg_iter = msg_iter->next;
		    msg_iter->next = bsmsg_ptr;
		}
		/*printf("bootstrap message received: %d bytes\n", bsmsg_ptr->length);fflush(stdout);*/
		SetEvent(s_queue->hMessageArrivedEvent);
	    }
	    else
	    {
		MPIU_Error_printf("MPIU_Malloc failed\n");
	    }
	    ReleaseMutex(s_queue->hMutex);
	}
	else
	{
	    MPIU_Error_printf("Error waiting for s_queue mutex, error %d\n", GetLastError());
	}
	/*MPIDI_FUNC_EXIT(MPID_STATE_BOOTSTRAPQWNDPROC);*/
	return 101;
    default:
	/*MPIDI_FUNC_EXIT(MPID_STATE_BOOTSTRAPQWNDPROC);*/
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    /*MPIDI_FUNC_EXIT(MPID_STATE_BOOTSTRAPQWNDPROC);*/
    return 0;
}

void MessageQueueThreadFn(MPIDI_CH3I_BootstrapQ_struct *queue)
{
    MSG msg;
    BOOL bRet;
    WNDCLASS wc;
    UUID guid;

    UuidCreate(&guid);
    MPIU_Snprintf(queue->name, MPIDI_BOOTSTRAP_NAME_LEN, "%08lX-%04X-%04x-%02X%02X-%02X%02X%02X%02X%02X%02X",
	guid.Data1, guid.Data2, guid.Data3,
	guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
	guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);

    wc.style = 0;
    wc.lpfnWndProc = (WNDPROC) BootstrapQWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = NULL;
    wc.hIcon = LoadIcon((HINSTANCE) NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor((HINSTANCE) NULL, IDC_ARROW);
    wc.hbrBackground = GetStockObject(WHITE_BRUSH);
    wc.lpszMenuName =  "MainMenu";
    wc.lpszClassName = queue->name;
    
    if (!RegisterClass(&wc))
    {
	queue->error = GetLastError();
	SetEvent(queue->hReadyEvent);
	return;
    }

    /* Create the hidden window. */
 
    queue->hWnd = CreateWindow(
	queue->name, /* window class */
	queue->name, /* window name */
	WS_OVERLAPPEDWINDOW,
	CW_USEDEFAULT,
	CW_USEDEFAULT,
	CW_USEDEFAULT,
	CW_USEDEFAULT,
	(HWND) HWND_MESSAGE, 
	(HMENU) NULL,
	0,
	(LPVOID) NULL); 
 
    if (!queue->hWnd)
    {
	queue->error = GetLastError();
	SetEvent(queue->hReadyEvent);
        return;
    }
 
    ShowWindow(queue->hWnd, SW_HIDE); 
 
    /* initialize the synchronization objects */
    queue->msg_list = NULL;
    queue->hMessageArrivedEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    queue->hMutex = CreateMutex(NULL, FALSE, NULL);

    /*MPIU_Assert(s_queue == NULL);*/ /* we can only handle one message queue */
    if (s_queue != NULL)
    {
	MPIU_Error_printf("Error: more than one message queue created\n");
	return;
    }
    s_queue = queue;

    /*printf("signalling queue is ready\n");fflush(stdout);*/
    /* signal that the queue is ready */
    SetEvent(queue->hReadyEvent);

    /* Start handling messages. */
    while( (bRet = GetMessage( &msg, NULL, 0, 0 )) != 0)
    { 
        if (bRet == -1)
        {
	    MPIU_Error_printf("MessageQueueThreadFn window received a WM_QUIT message, exiting...\n");
	    return;
        }
        else
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
	    /*printf("window message: %d\n", msg.message);fflush(stdout);*/
	}
    } 
}
#endif /* HAVE_WINDOWS_H */

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_BootstrapQ_create_unique_name
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_BootstrapQ_create_unique_name(char *name, int length)
{
#ifdef HAVE_WINDOWS_H
    UUID guid;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_CREATE_UNIQUE_NAME);
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_CREATE_UNIQUE_NAME);
    if (length < 40)
    {
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_CREATE_UNIQUE_NAME);
	return -1;
    }
    UuidCreate(&guid);
    MPIU_Snprintf(name, length, "%08lX-%04X-%04x-%02X%02X-%02X%02X%02X%02X%02X%02X",
	guid.Data1, guid.Data2, guid.Data3,
	guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
	guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
#elif defined(HAVE_UUID_GENERATE)
    uuid_t guid;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_CREATE_UNIQUE_NAME);
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_CREATE_UNIQUE_NAME);
    if (length < 40)
    {
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_CREATE_UNIQUE_NAME);
	return -1;
    }
    uuid_generate(guid);
    uuid_unparse(guid, name);
#elif defined(HAVE_TIME)
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_CREATE_UNIQUE_NAME);
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_CREATE_UNIQUE_NAME);
    /* rand is part of stdlib and returns an int, not an unsigned long */
    MPIU_Snprintf(name, 40, "%08X%08X%08X%08lX", rand(), rand(), rand(), (unsigned long)time(NULL));
#else
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_CREATE_UNIQUE_NAME);
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_CREATE_UNIQUE_NAME);
    MPIU_Snprintf(name, 40, "%08X%08X%08X%08X", rand(), rand(), rand(), rand());
#endif
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_CREATE_UNIQUE_NAME);
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_BootstrapQ_create_named
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_BootstrapQ_create_named(MPIDI_CH3I_BootstrapQ *queue_ptr, const char *name, const int initialize)
{
    int mpi_errno = MPI_SUCCESS;
#ifdef USE_MQSHM
    MPIDI_CH3I_BootstrapQ_struct *queue;
    int id;
#endif
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_CREATE_NAMED);
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_CREATE_NAMED);

#ifdef USE_MQSHM

    queue = (MPIDI_CH3I_BootstrapQ_struct*)
	MPIU_Malloc(sizeof(MPIDI_CH3I_BootstrapQ_struct));
    if (queue == NULL)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME,
				       __LINE__, MPI_ERR_OTHER, "**nomem", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_CREATE_NAMED);
	return mpi_errno;
    }
    queue->next = g_queue_list;
    g_queue_list = queue;

    /*printf("[%d] calling mqshm_create(%s) from BootstrapQ_create_named\n",
      MPIR_Process.comm_world->rank, name);fflush(stdout);*/
    mpi_errno = MPIDI_CH3I_mqshm_create(name, initialize, &id);
    if (mpi_errno != MPI_SUCCESS || id == -1)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME,
			       __LINE__, MPI_ERR_OTHER, "**mqshm_create", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_CREATE_NAMED);
	return mpi_errno;
    }
    queue->id = id;
    queue->pid = getpid();
    /*strcpy(queue->name, name);*/
    MPIU_Snprintf(queue->name, MPIDI_BOOTSTRAP_NAME_LEN, "%d", queue->pid);
#ifdef MPIDI_CH3_USES_SHM_NAME
    MPIU_Strncpy(queue->shm_name, name, MPIDI_BOOTSTRAP_NAME_LEN);
#endif

    *queue_ptr = queue;

    MPIU_DBG_PRINTF(("Created bootstrap queue, %d:%s\n", queue->id, queue->name));
#else
    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME,
				     __LINE__, MPI_ERR_OTHER, "**notimpl", 0);
#endif
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_CREATE_NAMED);
    return mpi_errno;
}

#ifdef USE_SINGLE_MSG_QUEUE

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_BootstrapQ_create
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_BootstrapQ_create(MPIDI_CH3I_BootstrapQ *queue_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_CH3I_BootstrapQ_struct *queue;
#ifndef USE_MQSHM
    int key;
    int id;
#endif
    int nb;
    struct msgbuf {
	long mtype;
	char data[BOOTSTRAP_MAX_MSG_SIZE];
    } msg;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_CREATE);
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_CREATE);

    if (g_queue_list)
    {
	*queue_ptr = g_queue_list;
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_CREATE);
	return MPI_SUCCESS;
    }

    queue = (MPIDI_CH3I_BootstrapQ_struct*)
	MPIU_Malloc(sizeof(MPIDI_CH3I_BootstrapQ_struct));
    if (queue == NULL)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME,
				       __LINE__, MPI_ERR_OTHER, "**nomem", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_CREATE);
	return mpi_errno;
    }
    queue->next = NULL;
    g_queue_list = queue;

#ifdef USE_MQSHM

    queue->id = -1;
    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**notimpl", 0);
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_CREATE);
    return mpi_errno;

#else

    key = MPICH_MSG_QUEUE_ID;
    id = msgget(key, IPC_CREAT | 0666);
    if (id == -1)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME,
		    __LINE__, MPI_ERR_OTHER, "**msgget", "**msgget %d", errno);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_CREATE);
	return mpi_errno;
    }
    queue->id = id;

#endif /* USE_MQSHM */

    queue->pid = getpid();
    MPIU_Snprintf(queue->name, MPIDI_BOOTSTRAP_NAME_LEN, "%d", getpid());

    /* drain any stale messages in the queue */
    nb = 0;
    while (nb != -1)
    {
	msg.mtype = queue->pid;
	nb = msgrcv(queue->id, &msg, BOOTSTRAP_MAX_MSG_SIZE,
		    queue->pid, IPC_NOWAIT);
    }

    *queue_ptr = queue;

#ifndef USE_MQSHM
     MPIU_DBG_PRINTF(("Created bootstrap queue, %d -> %d:%s\n", key, queue->id, queue->name));
#else
    MPIU_DBG_PRINTF(("Created bootstrap queue, %d:%s\n", queue->id, queue->name));
#endif

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_CREATE);
    return MPI_SUCCESS;
}

#else

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_BootstrapQ_create
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_BootstrapQ_create(MPIDI_CH3I_BootstrapQ *queue_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_CH3I_BootstrapQ_struct *queue;
#if defined(USE_POSIX_MQ)
    char key[100];
    mode_t mode;
    struct mq_attr attr;
    mqd_t id;
#elif defined(USE_SYSV_MQ)
    int id, key;
#elif defined(HAVE_WINDOWS_H)
    DWORD dwThreadID;
#else
#error No bootstrap queue mechanism defined
#endif
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_CREATE);
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_CREATE);

    /* allocate a queue structure and add it to the global list */
    queue = (MPIDI_CH3I_BootstrapQ_struct*)
	MPIU_Malloc(sizeof(MPIDI_CH3I_BootstrapQ_struct));
    if (queue == NULL)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME,
				       __LINE__, MPI_ERR_OTHER, "**nomem", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_CREATE);
	return mpi_errno;
    }
    queue->next = g_queue_list;
    g_queue_list = queue;

#ifdef USE_POSIX_MQ

    srand(getpid());
    MPIU_Snprintf(key, 100, "%s%d", MPICH_MSG_QUEUE_NAME, rand());
    mode = 0666;
    memset(&attr, 0, sizeof(attr));
    attr.mq_maxmsg = BOOTSTRAP_MAX_NUM_MSGS;
    attr.mq_msgsize = BOOTSTRAP_MAX_MSG_SIZE;
    id = mq_open(key, O_CREAT | O_RDWR | O_NONBLOCK, mode, &attr);
    while (id == -1)
    {
	if (errno != EEXIST)
	{
	    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME,
		  __LINE__, MPI_ERR_OTHER, "**mq_open", "**mq_open %d", errno);
	    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_CREATE);
	    return mpi_errno;
	}
	MPIU_Snprintf(key, 100, "%s%d", MPICH_MSG_QUEUE_NAME, rand());
	id = mq_open(key, O_CREAT | O_RDWR | O_NONBLOCK, mode, &attr);
    }
    queue->id = id;
    MPIU_Strncpy(queue->name, key, MPIDI_BOOTSTRAP_NAME_LEN);
    MPIU_DBG_PRINTF(("created message queue: %s\n", queue->name));

#elif defined(USE_SYSV_MQ)

    srand(getpid());
    key = rand();
    id = msgget(key, IPC_CREAT | IPC_EXCL | 0666);
    while (id == -1)
    {
	if (errno != EEXIST)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL,
					     FCNAME, __LINE__, MPI_ERR_OTHER,
					     "**msgget", "**msgget %d", errno);
	    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_CREATE);
	    return mpi_errno;
	}
	key = rand();
	id = msgget(key, IPC_CREAT | IPC_EXCL | 0666);
    }
    queue->id = id;
    MPIU_Snprintf(queue->name, MPIDI_BOOTSTRAP_NAME_LEN, "%d", key);
    MPIU_DBG_PRINTF(("created message queue: %s\n", queue->name));

#elif defined(HAVE_WINDOWS_H)

    queue->hReadyEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    queue->hMsgThread = CreateThread(NULL, 0,
	  (LPTHREAD_START_ROUTINE)MessageQueueThreadFn, queue, 0, &dwThreadID);
    if (queue->hMsgThread == NULL)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL,
					 FCNAME, __LINE__, MPI_ERR_OTHER,
					 "**CreateThread", "**CreateThread %d",
					 GetLastError());
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_CREATE);
	return mpi_errno;
    }
    if (WaitForSingleObject(queue->hReadyEvent, 60000) != WAIT_OBJECT_0)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME,
				      __LINE__, MPI_ERR_OTHER, "**winwait", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_CREATE);
	return mpi_errno;
    }
    CloseHandle(queue->hReadyEvent);
    queue->hReadyEvent = NULL;

#else
#error No bootstrap queue mechanism defined
#endif

    *queue_ptr = queue;

    MPIU_DBG_PRINTF(("Created bootstrap queue: %s\n", queue->name));

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_CREATE);
    return MPI_SUCCESS;
}

#endif

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_BootstrapQ_tostring
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_BootstrapQ_tostring(MPIDI_CH3I_BootstrapQ queue, char *name, int length)
{
    int mpi_errno;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_TOSTRING);
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_TOSTRING);
    
    if (queue == NULL)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_TOSTRING);
	return mpi_errno;
    }

    /*printf("[%d] queue->name = %s\n", MPIR_Process.comm_world->rank, queue->name);fflush(stdout);*/
#ifdef MPIDI_CH3_USES_SHM_NAME
    mpi_errno = MPIU_Snprintf(name, length, "%s:%s", queue->name, queue->shm_name);
    if (mpi_errno >= length)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_TOSTRING);
	return mpi_errno;
    }
#else
    mpi_errno = MPIU_Strncpy(name, queue->name, length);
    if (mpi_errno)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**arg", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_TOSTRING);
	return mpi_errno;
    }
#endif
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_TOSTRING);
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_BootstrapQ_unlink
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_BootstrapQ_unlink(MPIDI_CH3I_BootstrapQ queue)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_UNLINK);
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_UNLINK);
#ifdef USE_MQSHM
    mpi_errno = MPIDI_CH3I_mqshm_unlink(queue->id);
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**boot_unlink", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_UNLINK);
	return mpi_errno;
    }
#endif
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_UNLINK);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_BootstrapQ_destroy
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_BootstrapQ_destroy(MPIDI_CH3I_BootstrapQ queue)
{
    int mpi_errno = MPI_SUCCESS;
#ifdef USE_SINGLE_MSG_QUEUE
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_DESTROY);
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_DESTROY);
#ifdef USE_MQSHM
    mpi_errno = MPIDI_CH3I_mqshm_close(queue->id);
#endif
#elif defined(USE_POSIX_MQ)
    int result;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_DESTROY);
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_DESTROY);
    result = mq_close(queue->id);
    if (result == -1)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
					 FCNAME, __LINE__, MPI_ERR_OTHER,
					 "**mq_close", "**mq_close %d", errno);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_DESTROY);
	return mpi_errno;
    }
#elif defined(USE_SYSV_MQ)
    int result;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_DESTROY);
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_DESTROY);
    result = msgctl(queue->id, IPC_RMID, NULL);
    if (result == -1)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
					 FCNAME, __LINE__, MPI_ERR_OTHER,
					 "**msgctl", "**msgctl %d", errno);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_DESTROY);
	return mpi_errno;
    }
#elif defined(HAVE_WINDOWS_H)
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_DESTROY);
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_DESTROY);
    PostMessage(queue->hWnd, WM_DESTROY, 0, 0);
#else
#error No bootstrap queue mechanism defined
#endif
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_DESTROY);
    return MPI_SUCCESS;
}

#ifdef USE_SINGLE_MSG_QUEUE

/* FIXME: What does this routine do? */

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_BootstrapQ_attach
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_BootstrapQ_attach(char *name_full, MPIDI_CH3I_BootstrapQ * queue_ptr)
{
    int mpi_errno = MPI_SUCCESS;
#ifdef USE_SYSV_MQ
    int id, key;
#endif
    MPIDI_CH3I_BootstrapQ_struct *iter;
    char name[100];
#ifdef MPIDI_CH3_USES_SHM_NAME
    char shm_name[MPIDI_MAX_SHM_NAME_LENGTH] = "";
    char *token;
    MPIDI_CH3I_BootstrapQ_struct *matched_queue = NULL;
#endif
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_ATTACH);
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_ATTACH);

    MPIU_Strncpy(name, name_full, MPIDI_MAX_SHM_NAME_LENGTH);
#ifdef MPIDI_CH3_USES_SHM_NAME
    token = strtok(name, ":");
    if (token != NULL)
    {
	token = strtok(NULL, "");
	MPIU_Strncpy(shm_name, token, MPIDI_MAX_SHM_NAME_LENGTH);
    }
#endif

    /* check if this queue has already been attached to and return it if found */
    iter = g_queue_list;
    while (iter != NULL)
    {
	if (strcmp(iter->name, name) == 0)
	{
	    *queue_ptr = iter;
	    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_ATTACH);
	    return MPI_SUCCESS;
	}
	iter = iter->next;
    }

#ifdef MPIDI_CH3_USES_SHM_NAME
    /* search for another node with the same shm_name */
    iter = g_queue_list;
    while (iter != NULL)
    {
	if (strcmp(iter->shm_name, shm_name) == 0)
	{
	    matched_queue = iter;
	    break;
	}
	iter = iter->next;
    }
    if (matched_queue == NULL)
    {
	/* This is a queue this process hasn't seen before, so "create" it (attach to existing) */
	mpi_errno = MPIDI_CH3I_BootstrapQ_create_named(&matched_queue, shm_name, 0);
	if (mpi_errno != MPI_SUCCESS)
	{
	    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME,
				       __LINE__, MPI_ERR_OTHER, "**fail", 0);
	    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_ATTACH);
	    return mpi_errno;
	}
	/* change the pid field to match the remote process */
	MPIU_Strncpy(matched_queue->name, name, MPIDI_BOOTSTRAP_NAME_LEN);
	matched_queue->pid = atoi(name);
	*queue_ptr = matched_queue;
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_ATTACH);
	return mpi_errno;
    }
#endif

    /* FIXME: This memory is not freed */
    iter = (MPIDI_CH3I_BootstrapQ_struct*)MPIU_Malloc(sizeof(MPIDI_CH3I_BootstrapQ_struct));
    if (iter == NULL)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME,
				       __LINE__, MPI_ERR_OTHER, "**nomem", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_ATTACH);
	return mpi_errno;
    }
    MPIU_Strncpy(iter->name, name, MPIDI_BOOTSTRAP_NAME_LEN);
    iter->pid = atoi(name);
#ifdef MPIDI_CH3_USES_SHM_NAME
    MPIU_Strncpy(iter->shm_name, shm_name, MPIDI_BOOTSTRAP_NAME_LEN);
    iter->id = matched_queue->id;
#else
    iter->id = g_queue_list->id;
#endif

    iter->next = g_queue_list;
    g_queue_list = iter;

    *queue_ptr = iter;
    /*printf("[%d] attached to message queue: %s\n", MPIR_Process.comm_world->rank, name);fflush(stdout);*/
    MPIU_DBG_PRINTF(("attached to message queue: %s\n", name));

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_ATTACH);
    return MPI_SUCCESS;
}

#else

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_BootstrapQ_attach
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_BootstrapQ_attach(char *name, MPIDI_CH3I_BootstrapQ * queue_ptr)
{
    int mpi_errno = MPI_SUCCESS;
#ifdef USE_MQSHM

    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_ATTACH);
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_ATTACH);
    /*printf("[%d] attaching to %s, returning q_ptr %p\n", MPIR_Process.comm_world->rank, name, g_queue_list);fflush(stdout);*/
    *queue_ptr = g_queue_list;

#elif defined(USE_POSIX_MQ)

    int id;
    MPIDI_CH3I_BootstrapQ_struct *iter = g_queue_list;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_ATTACH);
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_ATTACH);

    while (iter)
    {
	if (strcmp(iter->name, name) == 0)
	{
	    *queue_ptr = iter;
	    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_ATTACH);
	    return MPI_SUCCESS;
	}
	iter = iter->next;
    }
    iter = (MPIDI_CH3I_BootstrapQ_struct*)
	MPIU_Malloc(sizeof(MPIDI_CH3I_BootstrapQ_struct));
    if (iter == NULL)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME,
				       __LINE__, MPI_ERR_OTHER, "**nomem", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_ATTACH);
	return mpi_errno;
    }
    MPIU_Strncpy(iter->name, name, MPIDI_BOOTSTRAP_NAME_LEN);
    id = mq_open(name, O_RDWR | O_NONBLOCK);
    if (id == -1)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME,
					 __LINE__, MPI_ERR_OTHER,
					 "**mq_open", "**mq_open %d", errno);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_ATTACH);
	return mpi_errno;
    }
    iter->id = id;
    iter->next = g_queue_list;
    /*iter->next = NULL;*/
    *queue_ptr = iter;
    /*printf("[%d] attached to message queue: %s\n", MPIR_Process.comm_world->rank, name);fflush(stdout);*/

#elif defined(USE_SYSV_MQ)

    int id, key;
    MPIDI_CH3I_BootstrapQ_struct *iter = g_queue_list;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_ATTACH);
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_ATTACH);

    while (iter)
    {
	if (strcmp(iter->name, name) == 0)
	{
	    *queue_ptr = iter;
	    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_ATTACH);
	    return MPI_SUCCESS;
	}
	iter = iter->next;
    }
    iter = (MPIDI_CH3I_BootstrapQ_struct*)
	MPIU_Malloc(sizeof(MPIDI_CH3I_BootstrapQ_struct));
    if (iter == NULL)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME,
				       __LINE__, MPI_ERR_OTHER, "**nomem", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_ATTACH);
	return mpi_errno;
    }
    MPIU_Strncpy(iter->name, name, MPIDI_BOOTSTRAP_NAME_LEN);
    key = atoi(name);
    id = msgget(key, IPC_CREAT | 0666);
    if (id == -1)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME,
					 __LINE__, MPI_ERR_OTHER,
					 "**msgget", "**msgget %d", errno);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_ATTACH);
	return mpi_errno;
    }
    iter->id = id;
    iter->next = g_queue_list;
    /*iter->next = NULL;*/
    *queue_ptr = iter;
    /*printf("[%d] attached to message queue: %s\n", MPIR_Process.comm_world->rank, name);fflush(stdout);*/

#elif defined(HAVE_WINDOWS_H)

    MPIDI_CH3I_BootstrapQ_struct *iter = g_queue_list;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_ATTACH);
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_ATTACH);
    while (iter)
    {
	if (strcmp(iter->name, name) == 0)
	{
	    *queue_ptr = iter;
	    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_ATTACH);
	    return MPI_SUCCESS;
	}
	iter = iter->next;
    }
    iter = (MPIDI_CH3I_BootstrapQ_struct*)
	MPIU_Malloc(sizeof(MPIDI_CH3I_BootstrapQ_struct));
    if (iter == NULL)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME,
				       __LINE__, MPI_ERR_OTHER, "**nomem", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_ATTACH);
	return mpi_errno;
    }
    MPIU_Strncpy(iter->name, name, MPIDI_BOOTSTRAP_NAME_LEN);
    /*printf("looking for window %s\n", name);fflush(stdout);*/
    iter->hWnd = FindWindowEx(HWND_MESSAGE, NULL, name, name);
    /*if (iter->hWnd != NULL) { printf("FindWindowEx found the window\n"); fflush(stdout); }*/
    if (iter->hWnd == NULL)
    {
	iter->hWnd = FindWindow(name, name);
	/*if (iter->hWnd != NULL) { printf("FindWindow found the window\n"); fflush(stdout); }*/
    }
    if (iter->hWnd == NULL)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME,
					 __LINE__, MPI_ERR_OTHER,
					 "**FindWindowEx", "**FindWindowEx %d",
					 GetLastError());
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_ATTACH);
	return mpi_errno;
    }
    iter->hMutex = CreateMutex(NULL, FALSE, NULL);
    iter->hReadyEvent = NULL;
    iter->msg_list = NULL;
    iter->hMsgThread = NULL;
    iter->hMessageArrivedEvent = NULL;
    iter->next = g_queue_list;
    /*iter->next = NULL;*/
    *queue_ptr = iter;
#else
#error No bootstrap queue mechanism defined
#endif
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_ATTACH);
    return MPI_SUCCESS;
}

#endif

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_BootstrapQ_detach
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_BootstrapQ_detach(MPIDI_CH3I_BootstrapQ queue)
{
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_DETACH);
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_DETACH);
    /* remove the queue from the global list? */
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_DETACH);
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_BootstrapQ_send_msg
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_BootstrapQ_send_msg(MPIDI_CH3I_BootstrapQ queue, void *buffer, int length)
{
    int mpi_errno = MPI_SUCCESS;
#ifdef USE_MQSHM

    int num_sent = 0;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_SEND_MSG);
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_SEND_MSG);
    /*printf("[%d] calling mqshm_send from BootstrapQ_send_msg\n", MPIR_Process.comm_world->rank);fflush(stdout);*/
    mpi_errno = MPIDI_CH3I_mqshm_send(queue->id, buffer, length,
				      queue->pid, &num_sent, 1);
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME,
				   __LINE__, MPI_ERR_OTHER, "**mqshm_send", 0);
    }

#elif defined(USE_POSIX_MQ) || defined(USE_SYSV_MQ) || defined(USE_SINGLE_MSG_QUEUE)
    struct msgbuf {
	long mtype;
	char data[BOOTSTRAP_MAX_MSG_SIZE];
    } msg;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_SEND_MSG);
    MPIDI_STATE_DECL(MPID_STATE_MEMCPY);
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_SEND_MSG);

#ifdef MPICH_DBG_OUTPUT
    /*MPIU_Assert(length <= BOOTSTRAP_MAX_MSG_SIZE);*/
    if (length > BOOTSTRAP_MAX_MSG_SIZE)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME,
					 __LINE__, MPI_ERR_OTHER,
					 "**bootqmsg", "bootqmsg %d %d",
					 length, BOOTSTRAP_MAX_MSG_SIZE);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_SEND_MSG);
	return mpi_errno;
    }
#endif

#ifdef USE_SINGLE_MSG_QUEUE
    msg.mtype = queue->pid;
#else
    msg.mtype = 100;
#endif
    MPIDI_FUNC_ENTER(MPID_STATE_MEMCPY);
    MPIU_Memcpy(msg.data, buffer, length);
    MPIDI_FUNC_EXIT(MPID_STATE_MEMCPY);
    MPIU_DBG_PRINTF(("sending message %d on queue %d\n", msg.mtype, queue->id));
#ifdef USE_POSIX_MQ
    if (mq_send(queue->id, &msg, length, 0))
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME,
					 __LINE__, MPI_ERR_OTHER,
					 "**mq_send", "**mq_send %d", errno);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_SEND_MSG);
	return mpi_errno;
    }
#elif defined (USE_SYSV_MQ)
    if (msgsnd(queue->id, &msg, length, 0) == -1)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME,
					 __LINE__, MPI_ERR_OTHER,
					 "**msgsnd", "**msgsnd %d", errno);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_SEND_MSG);
	return mpi_errno;
    }
#else
#error No bootstrap queue mechanism defined
#endif
    MPIU_DBG_PRINTF(("message sent: %d bytes\n", length));

#elif defined(HAVE_WINDOWS_H)

    COPYDATASTRUCT data;
    LRESULT rc;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_SEND_MSG);
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_SEND_MSG);
    data.dwData = 0; /* immediate data field */
    data.cbData = length;
    data.lpData = buffer;
    rc = SendMessage(queue->hWnd, WM_COPYDATA, 0, (LPARAM)&data);
    /*printf("SendMessage returned %d\n", rc);fflush(stdout);*/

#endif

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_SEND_MSG);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_BootstrapQ_recv_msg
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_BootstrapQ_recv_msg(MPIDI_CH3I_BootstrapQ queue, void *buffer, int length, int *num_bytes_ptr, BOOL blocking)
{
    int mpi_errno = MPI_SUCCESS;
#ifdef USE_MQSHM

    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_RECV_MSG);
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_RECV_MSG);
    /*printf("[%d] calling mqshm_receive from BootstrapQ_recv_msg\n", MPIR_Process.comm_world->rank);fflush(stdout);*/
    mpi_errno = MPIDI_CH3I_mqshm_receive(queue->id, queue->pid, buffer, length,
					 num_bytes_ptr, blocking);
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME,
					 __LINE__, MPI_ERR_OTHER,
					 "**mqshm_receive", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_RECV_MSG);
	return mpi_errno;
    }

#elif defined(USE_POSIX_MQ) || defined(USE_SYSV_MQ) || defined(USE_SINGLE_MSG_QUEUE)
    int nb;
    struct msgbuf {
	long mtype;
	char data[BOOTSTRAP_MAX_MSG_SIZE];
    } msg;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_RECV_MSG);
    MPIDI_STATE_DECL(MPID_STATE_MEMCPY);
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_RECV_MSG);

#ifdef MPICH_DBG_OUTPUT
    /*MPIU_Assert(length <= BOOTSTRAP_MAX_MSG_SIZE);*/
    if (length > BOOTSTRAP_MAX_MSG_SIZE)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME,
					 __LINE__, MPI_ERR_OTHER,
					 "**bootqmsg", "bootqmsg %d %d",
					 length, BOOTSTRAP_MAX_MSG_SIZE);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_RECV_MSG);
	return mpi_errno;
    }
#endif

#ifdef USE_SINGLE_MSG_QUEUE
    msg.mtype = queue->pid;
    nb = msgrcv(queue->id, &msg, BOOTSTRAP_MAX_MSG_SIZE, queue->pid,
		blocking ? 0 : IPC_NOWAIT);
#else
#ifdef USE_POSIX_MQ
    nb = mq_receive(queue->id, &msg, BOOTSTRAP_MAX_MSG_SIZE, NULL);
#elif defined(USE_SYSV_MQ)
    msg.mtype = 100;
    nb = msgrcv(queue->id, &msg, BOOTSTRAP_MAX_MSG_SIZE, 0,
		blocking ? 0 : IPC_NOWAIT);
#else
#error No bootstrap queue mechanism defined
#endif
#endif
    if (nb == -1)
    {
	*num_bytes_ptr = 0;
	if (errno == EAGAIN || errno == ENOMSG)
	{
	    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_RECV_MSG);
	    return MPI_SUCCESS;
	}
#ifdef USE_POSIX_MQ
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME,
					 __LINE__, MPI_ERR_OTHER,
					 "**mq_receive", "**mq_receive %d",
					 errno);
#else
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME,
					 __LINE__, MPI_ERR_OTHER,
					 "**msgrcv", "**msgrcv %d", errno);
#endif
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_RECV_MSG);
	return mpi_errno;
    }
    MPIDI_FUNC_ENTER(MPID_STATE_MEMCPY);
    MPIU_Memcpy(buffer, msg.data, nb);
    MPIDI_FUNC_EXIT(MPID_STATE_MEMCPY);
    *num_bytes_ptr = nb;
    MPIU_DBG_PRINTF(("message %d received: %d bytes\n", msg.mtype, nb));
    
#elif defined(HAVE_WINDOWS_H)

    bootstrap_msg * msg;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_RECV_MSG);
    MPIDI_STATE_DECL(MPID_STATE_MEMCPY);
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_RECV_MSG);

    mpi_errno = GetNextBootstrapMsg(queue, &msg, blocking);
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME,
					 __LINE__, MPI_ERR_OTHER,
					 "**nextbootmsg", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_RECV_MSG);
	return mpi_errno;
    }
    if (msg)
    {
	MPIDI_FUNC_ENTER(MPID_STATE_MEMCPY);
	MPIU_Memcpy(buffer, msg->buffer, min(length, msg->length));
	MPIDI_FUNC_EXIT(MPID_STATE_MEMCPY);
	*num_bytes_ptr = min(length, msg->length);
	MPIU_Free(msg);
    }
    else
    {
	*num_bytes_ptr = 0;
    }
#endif
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_RECV_MSG);
    return mpi_errno;
}
