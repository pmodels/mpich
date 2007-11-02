/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "socketimpl.h"
#include "sock.h"

#if (WITH_SOCK_TYPE == SOCK_IOCP)

#include <mswsock.h>
#include <stdio.h>

typedef enum SOCK_TYPE { SOCK_INVALID, SOCK_LISTENER, SOCK_SOCKET } SOCK_TYPE;

typedef int SOCK_STATE;
#define SOCK_ACCEPTING  0x0001
#define SOCK_ACCEPTED   0x0002
#define SOCK_CONNECTING 0x0004
#define SOCK_READING    0x0008
#define SOCK_WRITING    0x0010

typedef struct sock_buffer
{
    int use_iov;
    DWORD num_bytes;
    OVERLAPPED ovl;
    void *buffer;
    int bufflen;
    SOCK_IOV iov[SOCK_IOV_MAXLEN];
    int iovlen;
    int index;
    int total;
    int (*progress_update)(int,void*);
} sock_buffer;

typedef struct sock_state_t
{
    SOCK_TYPE type;
    SOCK_STATE state;
    SOCKET sock;
    sock_set_t set;
    int closing;
    int pending_operations;
    /* listener/accept structures */
    SOCKET listen_sock;
    char accept_buffer[sizeof(struct sockaddr_in)*2+32];
    /* read and write structures */
    sock_buffer read;
    sock_buffer write;
    /* user pointer */
    void *user_ptr;
} sock_state_t;

#define DEFAULT_NUM_RETRIES 10

static int g_connection_attempts = DEFAULT_NUM_RETRIES;
static int g_num_cp_threads = 2;


/* utility allocator functions */

typedef struct BlockAllocator_struct * BlockAllocator;

BlockAllocator BlockAllocInit(unsigned int blocksize, int count, int incrementsize, void *(* alloc_fn)(unsigned int size), void (* free_fn)(void *p));
int BlockAllocFinalize(BlockAllocator *p);
void * BlockAlloc(BlockAllocator p);
int BlockFree(BlockAllocator p, void *pBlock);

struct BlockAllocator_struct
{
    void **pNextFree;
    void *(* alloc_fn)(size_t size);
    void (* free_fn)(void *p);
    struct BlockAllocator_struct *pNextAllocation;
    unsigned int nBlockSize;
    int nCount, nIncrementSize;
#ifdef WITH_ALLOCATOR_LOCKING
    MPIDU_Lock_t lock;
#endif
};

static int g_nLockSpinCount = 100;

#ifdef WITH_ALLOCATOR_LOCKING

typedef volatile long MPIDU_Lock_t;

#include <errno.h>
#ifdef HAVE_WINDOWS_H
#include <winsock2.h>
#include <windows.h>
#endif

static inline void MPIDU_Init_lock( MPIDU_Lock_t *lock )
{
    *(lock) = 0;
}

static inline void MPIDU_Lock( MPIDU_Lock_t *lock )
{
    int i;
    for (;;)
    {
        for (i=0; i<g_nLockSpinCount; i++)
        {
            if (*lock == 0)
            {
#ifdef HAVE_INTERLOCKEDEXCHANGE
                if (InterlockedExchange((LPLONG)lock, 1) == 0)
                {
                    /*printf("lock %x\n", lock);fflush(stdout);*/
                    MPID_PROFILE_OUT(MPIDU_BUSY_LOCK);
                    return;
                }
#elif defined(HAVE_COMPARE_AND_SWAP)
                if (compare_and_swap(lock, 0, 1) == 1)
                {
                    MPID_PROFILE_OUT(MPIDU_BUSY_LOCK);
                    return;
                }
#else
#error Atomic memory operation needed to implement busy locks
#endif
            }
        }
        MPIDU_Yield();
    }
}

static inline void MPIDU_Unlock( MPIDU_Lock_t *lock )
{
    *(lock) = 0;
}

static inline void MPIDU_Busy_wait( MPIDU_Lock_t *lock )
{
    int i;
    for (;;)
    {
        for (i=0; i<g_nLockSpinCount; i++)
            if (!*lock)
            {
                return;
            }
        MPIDU_Yield();
    }
}

static inline void MPIDU_Free_lock( MPIDU_Lock_t *lock )
{
}

/*@
   MPIDU_Compare_swap - 

   Parameters:
+  void **dest
.  void *new_val
.  void *compare_val
.  MPIDU_Lock_t *lock
-  void **original_val

   Notes:
@*/
static inline int MPIDU_Compare_swap( void **dest, void *new_val, void *compare_val,            
                        MPIDU_Lock_t *lock, void **original_val )
{
    /* dest = pointer to value to be checked (address size)
       new_val = value to set dest to if *dest == compare_val
       original_val = value of dest prior to this operation */

#ifdef HAVE_NT_LOCKS
    /* *original_val = (void*)InterlockedCompareExchange(dest, new_val, compare_val); */
    *original_val = InterlockedCompareExchangePointer(dest, new_val, compare_val);
#elif defined(HAVE_COMPARE_AND_SWAP)
    if (compare_and_swap((volatile long *)dest, (long)compare_val, (long)new_val))
        *original_val = new_val;
#else
#error Locking functions not defined
#endif

    return 0;
}
#endif /* WITH_ALLOCATOR_LOCKING */

static BlockAllocator BlockAllocInit(unsigned int blocksize, int count, int incrementsize, void *(* alloc_fn)(unsigned int size), void (* free_fn)(void *p))
{
    BlockAllocator p;
    void **ppVoid;
    int i;

    p = alloc_fn( sizeof(struct BlockAllocator_struct) + ((blocksize + sizeof(void**)) * count) );

    p->alloc_fn = alloc_fn;
    p->free_fn = free_fn;
    p->nIncrementSize = incrementsize;
    p->pNextAllocation = NULL;
    p->nCount = count;
    p->nBlockSize = blocksize;
    p->pNextFree = (void**)(p + 1);
#ifdef WITH_ALLOCATOR_LOCKING
    MPIDU_Init_lock(&p->lock);
#endif

    ppVoid = (void**)(p + 1);
    for (i=0; i<count-1; i++)
    {
	*ppVoid = (void*)((char*)ppVoid + sizeof(void**) + blocksize);
	ppVoid = *ppVoid;
    }
    *ppVoid = NULL;

    return p;
}

static int BlockAllocFinalize(BlockAllocator *p)
{
    if (*p == NULL)
	return 0;
    BlockAllocFinalize(&(*p)->pNextAllocation);
    if ((*p)->free_fn != NULL)
	(*p)->free_fn(*p);
    *p = NULL;
    return 0;
}

static void * BlockAlloc(BlockAllocator p)
{
    void *pVoid;
    
#ifdef WITH_ALLOCATOR_LOCKING
    MPIDU_Lock(&p->lock);
#endif

    pVoid = p->pNextFree + 1;
    
    if (*(p->pNextFree) == NULL)
    {
	BlockAllocator pIter = p;
	while (pIter->pNextAllocation != NULL)
	    pIter = pIter->pNextAllocation;
	pIter->pNextAllocation = BlockAllocInit(p->nBlockSize, p->nIncrementSize, p->nIncrementSize, p->alloc_fn, p->free_fn);
	p->pNextFree = pIter->pNextFree;
    }
    else
	p->pNextFree = *(p->pNextFree);

#ifdef WITH_ALLOCATOR_LOCKING
    MPIDU_Unlock(&p->lock);
#endif

    return pVoid;
}

static int BlockFree(BlockAllocator p, void *pBlock)
{
#ifdef WITH_ALLOCATOR_LOCKING
    MPIDU_Lock(&p->lock);
#endif

    ((void**)pBlock)--;
    *((void**)pBlock) = p->pNextFree;
    p->pNextFree = pBlock;

#ifdef WITH_ALLOCATOR_LOCKING
    MPIDU_Unlock(&p->lock);
#endif

    return 0;
}




/* utility socket functions */

static int easy_create(SOCKET *sock, int port, unsigned long addr)
{
    struct linger linger;
    int optval, len;
    SOCKET temp_sock;
    SOCKADDR_IN sockAddr;

    /* create the non-blocking socket */
    temp_sock = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
    if (temp_sock == INVALID_SOCKET)
	return SOCK_FAIL;
    
    memset(&sockAddr,0,sizeof(sockAddr));
    
    sockAddr.sin_family = AF_INET;
    sockAddr.sin_addr.s_addr = addr;
    sockAddr.sin_port = htons((unsigned short)port);

    if (bind(temp_sock, (SOCKADDR*)&sockAddr, sizeof(sockAddr)) == SOCKET_ERROR)
	    return SOCK_FAIL;
    
    /* Set the linger on close option */
    linger.l_onoff = 1 ;
    linger.l_linger = 60;
    setsockopt(temp_sock, SOL_SOCKET, SO_LINGER, (char*)&linger, sizeof(linger));

    /* set the socket buffers */
    len = sizeof(int);
    if (!getsockopt(temp_sock, SOL_SOCKET, SO_RCVBUF, (char*)&optval, &len))
    {
	optval = 64*1024;
	setsockopt(temp_sock, SOL_SOCKET, SO_RCVBUF, (char*)&optval, sizeof(int));
    }
    len = sizeof(int);
    if (!getsockopt(temp_sock, SOL_SOCKET, SO_SNDBUF, (char*)&optval, &len))
    {
	optval = 64*1024;
	setsockopt(temp_sock, SOL_SOCKET, SO_SNDBUF, (char*)&optval, sizeof(int));
    }

    /* prevent the socket from being inherited by child processes */
    DuplicateHandle(
	GetCurrentProcess(), (HANDLE)temp_sock,
	GetCurrentProcess(), (HANDLE*)sock,
	0, FALSE, DUPLICATE_CLOSE_SOURCE | DUPLICATE_SAME_ACCESS);

    /* Set the no-delay option */
    setsockopt(*sock, IPPROTO_TCP, TCP_NODELAY, (char *)&optval, sizeof(optval));

    return SOCK_SUCCESS;
}

static inline int easy_get_sock_info(SOCKET sock, char *name, int *port)
{
    struct sockaddr_in addr;
    int name_len = sizeof(addr);

    getsockname(sock, (struct sockaddr*)&addr, &name_len);
    *port = ntohs(addr.sin_port);
    gethostname(name, 100);

    return 0;
}

static inline void init_state_struct(sock_state_t *p)
{
    p->listen_sock = SOCK_INVALID_SOCK;
    p->sock = SOCK_INVALID_SOCK;
    p->set = INVALID_HANDLE_VALUE;
    p->user_ptr = NULL;
    p->type = 0;
    p->state = 0;
    p->closing = FALSE;
    p->pending_operations = 0;
    p->read.total = 0;
    p->read.num_bytes = 0;
    p->read.buffer = NULL;
    /*p->read.iov = NULL;*/
    p->read.iovlen = 0;
    p->read.ovl.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    p->read.ovl.Offset = 0;
    p->read.ovl.OffsetHigh = 0;
    p->read.progress_update = NULL;
    p->write.total = 0;
    p->write.num_bytes = 0;
    p->write.buffer = NULL;
    /*p->write.iov = NULL;*/
    p->write.iovlen = 0;
    p->write.ovl.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    p->write.ovl.Offset = 0;
    p->write.ovl.OffsetHigh = 0;
    p->write.progress_update = NULL;
}

static inline int post_next_accept(sock_state_t * listen_state)
{
    int error;
    listen_state->state = SOCK_ACCEPTING;
    listen_state->sock = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
    if (listen_state->sock == SOCK_INVALID_SOCKET)
	return SOCK_FAIL;
    if (!AcceptEx(
	listen_state->listen_sock, 
	listen_state->sock, 
	listen_state->accept_buffer, 
	0, 
	sizeof(struct sockaddr_in)+16, 
	sizeof(struct sockaddr_in)+16, 
	&listen_state->read.num_bytes,
	&listen_state->read.ovl))
    {
	error = WSAGetLastError();
	if (error == ERROR_IO_PENDING)
	    return TRUE;
	printf("AcceptEx failed with error %d\n", error);
	return FALSE;
    }
    return TRUE;
}

/* sock functions */

static BlockAllocator g_StateAllocator;

int sock_init()
{
    char *szNum;
    WSADATA wsaData;
    int err;
    MPIDI_STATE_DECL(MPID_STATE_SOCK_INIT);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCK_INIT);

    /* Start the Winsock dll */
    if ((err = WSAStartup(MAKEWORD(2, 0), &wsaData)) != 0)
    {
	MPIDI_FUNC_EXIT(MPID_STATE_SOCK_INIT);
	return err;
    }

    /* get the connection retry value */
    szNum = getenv("SOCK_CONNECT_TRIES");
    if (szNum != NULL)
    {
	g_connection_attempts = atoi(szNum);
	if (g_connection_attempts < 1)
	    g_connection_attempts = DEFAULT_NUM_RETRIES;
    }

    g_StateAllocator = BlockAllocInit(sizeof(sock_state_t), 1000, 500, malloc, free);

    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_INIT);
    return SOCK_SUCCESS;
}

int sock_finalize()
{
    MPIDI_STATE_DECL(MPID_STATE_SOCK_FINALIZE);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCK_FINALIZE);
    WSACleanup();
    BlockAllocFinalize(&g_StateAllocator);
    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_FINALIZE);
    return SOCK_SUCCESS;
}

int sock_create_set(sock_set_t *set)
{
    HANDLE port;
    MPIDI_STATE_DECL(MPID_STATE_SOCK_CREATE_SET);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCK_CREATE_SET);
    port = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, g_num_cp_threads);
    if (port != NULL)
    {
	*set = port;
	MPIDI_FUNC_EXIT(MPID_STATE_SOCK_CREATE_SET);
	return SOCK_SUCCESS;
    }
    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_CREATE_SET);
    return SOCK_FAIL;
}

int sock_destroy_set(sock_set_t set)
{
    BOOL b;
    MPIDI_STATE_DECL(MPID_STATE_SOCK_DESTROY_SET);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCK_DESTROY_SET);
    b = CloseHandle(set);
    if (b == TRUE)
    {
	MPIDI_FUNC_EXIT(MPID_STATE_SOCK_DESTROY_SET);
	return SOCK_SUCCESS;
    }
    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_DESTROY_SET);
    return SOCK_FAIL;
}

static int listening = 0;
int sock_listen(sock_set_t set, void * user_ptr, int *port, sock_t *listener)
{
    char host[100];
    DWORD num_read = 0;
    sock_state_t * listen_state;
    MPIDI_STATE_DECL(MPID_STATE_SOCK_LISTEN);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCK_LISTEN);

    if (listening)
    {
	MPIDI_FUNC_EXIT(MPID_STATE_SOCK_LISTEN);
	return SOCK_FAIL;
    }
    listening = 1;

    listen_state = (sock_state_t*)BlockAlloc(g_StateAllocator);
    init_state_struct(listen_state);
    if (easy_create(&listen_state->listen_sock, *port, INADDR_ANY) == SOCKET_ERROR)
    {
	MPIDI_FUNC_EXIT(MPID_STATE_SOCK_LISTEN);
	return SOCK_FAIL;
    }
    if (listen(listen_state->listen_sock, SOMAXCONN) == SOCKET_ERROR)
    {
	MPIDI_FUNC_EXIT(MPID_STATE_SOCK_LISTEN);
	return SOCK_FAIL;
    }
    if (CreateIoCompletionPort((HANDLE)listen_state->listen_sock, set, (DWORD)listen_state, g_num_cp_threads) == NULL)
    {
	MPIDI_FUNC_EXIT(MPID_STATE_SOCK_LISTEN);
	return SOCK_FAIL;
    }
    easy_get_sock_info(listen_state->listen_sock, host, port);
    listen_state->user_ptr = user_ptr;
    listen_state->type = SOCK_LISTENER;
    /* do this last to make sure the listener state structure is completely initialized before
       a completion thread has the chance to satisfy the AcceptEx call */
    if (!post_next_accept(listen_state))
    {
	MPIDI_FUNC_EXIT(MPID_STATE_SOCK_LISTEN);
	return SOCK_FAIL;
    }
    *listener = listen_state;
    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_LISTEN);
    return SOCK_SUCCESS;
}

int sock_post_connect(sock_set_t set, void * user_ptr, char *host, int port, sock_t *connected)
{
    struct hostent *lphost;
    struct sockaddr_in sockAddr;
    sock_state_t *connect_state;
    MPIDI_STATE_DECL(MPID_STATE_SOCK_POST_CONNECT);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCK_POST_CONNECT);

    memset(&sockAddr,0,sizeof(sockAddr));

    /* setup the structures */
    connect_state = (sock_state_t*)BlockAlloc(g_StateAllocator);
    init_state_struct(connect_state);

    sockAddr.sin_family = AF_INET;
    sockAddr.sin_addr.s_addr = inet_addr(host);
    
    if (sockAddr.sin_addr.s_addr == INADDR_NONE || sockAddr.sin_addr.s_addr == 0)
    {
	lphost = gethostbyname(host);
	if (lphost != NULL)
	    sockAddr.sin_addr.s_addr = ((struct in_addr *)lphost->h_addr)->s_addr;
	else
	{
	    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_POST_CONNECT);
	    return SOCKET_ERROR;
	}
    }
    
    sockAddr.sin_port = htons((u_short)port);

    /* create a socket */
    if (easy_create(&connect_state->sock, ADDR_ANY, INADDR_ANY) == SOCKET_ERROR)
    {
	MPIDI_FUNC_EXIT(MPID_STATE_SOCK_POST_CONNECT);
	return SOCK_FAIL;
    }

    /* connect */
    if (connect(connect_state->sock, (SOCKADDR*)&sockAddr, sizeof(sockAddr)) == SOCKET_ERROR)
    {
	MPIDI_FUNC_EXIT(MPID_STATE_SOCK_POST_CONNECT);
	return SOCK_FAIL;
    }

    connect_state->user_ptr = user_ptr;
    connect_state->type = SOCK_SOCKET;
    connect_state->state = SOCK_CONNECTING;
    connect_state->set = set;

    /* associate the socket with the completion port */
    if (CreateIoCompletionPort((HANDLE)connect_state->sock, set, (DWORD)connect_state, g_num_cp_threads) == NULL)
    {
	MPIDI_FUNC_EXIT(MPID_STATE_SOCK_POST_CONNECT);
	return SOCK_FAIL;
    }

    connect_state->pending_operations++;

    /* post a completion event so the sock_post_connect can be notified through sock_wait */
    PostQueuedCompletionStatus(set, 0, (DWORD)connect_state, &connect_state->write.ovl);

    *connected = connect_state;

    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_POST_CONNECT);
    return SOCK_SUCCESS;
}

int sock_accept(sock_t listener, sock_set_t set, void * user_ptr, sock_t *accepted)
{
    BOOL b;
    struct linger linger;
    int optval, len;
    sock_state_t *accept_state;
    MPIDI_STATE_DECL(MPID_STATE_SOCK_ACCEPT);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCK_ACCEPT);

    if (!(listener->state & SOCK_ACCEPTED))
    {
	*accepted = NULL;
	MPIDI_FUNC_EXIT(MPID_STATE_SOCK_ACCEPT);
	return SOCK_FAIL;
    }

    accept_state = BlockAlloc(g_StateAllocator);
    if (accept_state == NULL)
    {
	*accepted = NULL;
	MPIDI_FUNC_EXIT(MPID_STATE_SOCK_ACCEPT);
	return SOCK_FAIL;
    }
    init_state_struct(accept_state);

    accept_state->type = SOCK_SOCKET;

    accept_state->sock = listener->sock;
    if (!post_next_accept(listener))
    {
	*accepted = NULL;
	MPIDI_FUNC_EXIT(MPID_STATE_SOCK_ACCEPT);
	return SOCK_FAIL;
    }

    /* set the linger option */
    linger.l_onoff = 1;
    linger.l_linger = 60;
    setsockopt(accept_state->sock, SOL_SOCKET, SO_LINGER, (char*)&linger, sizeof(linger));

    /* set the socket buffers */
    len = sizeof(int);
    if (!getsockopt(accept_state->sock, SOL_SOCKET, SO_RCVBUF, (char*)&optval, &len))
    {
	optval = 64*1024;
	setsockopt(accept_state->sock, SOL_SOCKET, SO_RCVBUF, (char*)&optval, sizeof(int));
    }
    len = sizeof(int);
    if (!getsockopt(accept_state->sock, SOL_SOCKET, SO_SNDBUF, (char*)&optval, &len))
    {
	optval = 64*1024;
	setsockopt(accept_state->sock, SOL_SOCKET, SO_SNDBUF, (char*)&optval, sizeof(int));
    }

    /* set the no-delay option */
    b = TRUE;
    setsockopt(accept_state->sock, IPPROTO_TCP, TCP_NODELAY, (char*)&b, sizeof(BOOL));

    /* prevent the socket from being inherited by child processes */
    DuplicateHandle(
	GetCurrentProcess(), (HANDLE)accept_state->sock,
	GetCurrentProcess(), (HANDLE*)&accept_state->sock,
	0, FALSE, DUPLICATE_CLOSE_SOURCE | DUPLICATE_SAME_ACCESS);

    /* associate the socket with the completion port */
    if (CreateIoCompletionPort((HANDLE)accept_state->sock, set, (DWORD)accept_state, g_num_cp_threads) == NULL)
    {
	MPIDI_FUNC_EXIT(MPID_STATE_SOCK_ACCEPT);
	return SOCK_FAIL;
    }

    accept_state->user_ptr = user_ptr;
    accept_state->set = set;
    *accepted = accept_state;

    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_ACCEPT);
    return SOCK_SUCCESS;
}

int sock_post_close(sock_t sock)
{
    MPIDI_STATE_DECL(MPID_STATE_SOCK_POST_CLOSE);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCK_POST_CLOSE);

    sock->closing = TRUE;
    if (sock->pending_operations == 0)
    {
	shutdown(sock->sock, SD_BOTH);
	closesocket(sock->sock);
	sock->sock = SOCK_INVALID_SOCKET;
	PostQueuedCompletionStatus(sock->set, 0, (DWORD)sock, NULL);
    }
    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_POST_CLOSE);
    return SOCK_SUCCESS;
}

int sock_wait(sock_set_t set, int millisecond_timeout, sock_event_t *out)
{
    int error;
    DWORD num_bytes;
    sock_state_t *sock;
    OVERLAPPED *ovl;
    DWORD dwFlags = 0;
    MPIDI_STATE_DECL(MPID_STATE_SOCK_WAIT);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCK_WAIT);

    for (;;) 
    {
	if (GetQueuedCompletionStatus(set, &num_bytes, (DWORD*)&sock, &ovl, millisecond_timeout))
	{
	    if (sock->type == SOCK_SOCKET)
	    {
		if (sock->closing && sock->pending_operations == 0)
		{
		    out->num_bytes = 0;
		    out->error = 0;
		    out->op_type = SOCK_OP_CLOSE;
		    out->user_ptr = sock->user_ptr;
		    /*BlockFree(g_sock_allocator, sock);*/
		    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_WAIT);
		    return SOCK_SUCCESS;
		}
		if (ovl == &sock->read.ovl)
		{
		    sock->read.total += num_bytes;
		    if (sock->read.use_iov)
		    {
			while (num_bytes)
			{
			    if (sock->read.iov[sock->read.index].SOCK_IOV_LEN <= num_bytes)
			    {
				num_bytes -= sock->read.iov[sock->read.index].SOCK_IOV_LEN;
				sock->read.index++;
				sock->read.iovlen--;
			    }
			    else
			    {
				sock->read.iov[sock->read.index].SOCK_IOV_LEN -= num_bytes;
				sock->read.iov[sock->read.index].SOCK_IOV_BUF = 
				    (char*)(sock->read.iov[sock->read.index].SOCK_IOV_BUF) + num_bytes;
				num_bytes = 0;
			    }
			}
			if (sock->read.iovlen == 0)
			{
			    out->num_bytes = sock->read.total;
			    out->op_type = SOCK_OP_READ;
			    out->user_ptr = sock->user_ptr;
			    sock->pending_operations--;
			    if (sock->closing && sock->pending_operations == 0)
			    {
				MPIU_DBG_PRINTF(("sock_wait: closing socket(%d) after iov read completed.\n", sock_getid(sock)));
				shutdown(sock->sock, SD_BOTH);
				closesocket(sock->sock);
				sock->sock = SOCK_INVALID_SOCKET;
			    }
			    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_WAIT);
			    return SOCK_SUCCESS;
			}
			/* make the user upcall */
			if (sock->read.progress_update != NULL)
			    sock->read.progress_update(num_bytes, sock->user_ptr);
			/* post a read of the remaining data */
			WSARecv(sock->sock, sock->read.iov, sock->read.iovlen, &sock->read.num_bytes, &dwFlags, &sock->read.ovl, NULL);
		    }
		    else
		    {
			sock->read.buffer = (char*)(sock->read.buffer) + num_bytes;
			sock->read.bufflen -= num_bytes;
			if (sock->read.bufflen == 0)
			{
			    out->num_bytes = sock->read.total;
			    out->op_type = SOCK_OP_READ;
			    out->user_ptr = sock->user_ptr;
			    sock->pending_operations--;
			    if (sock->closing && sock->pending_operations == 0)
			    {
				MPIU_DBG_PRINTF(("sock_wait: closing socket(%d) after simple read completed.\n", sock_getid(sock)));
				shutdown(sock->sock, SD_BOTH);
				closesocket(sock->sock);
				sock->sock = SOCK_INVALID_SOCKET;
			    }
			    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_WAIT);
			    return SOCK_SUCCESS;
			}
			/* make the user upcall */
			if (sock->read.progress_update != NULL)
			    sock->read.progress_update(num_bytes, sock->user_ptr);
			/* post a read of the remaining data */
			ReadFile((HANDLE)(sock->sock), sock->read.buffer, sock->read.bufflen, &sock->read.num_bytes, &sock->read.ovl);
		    }
		}
		else if (ovl == &sock->write.ovl)
		{
		    if (sock->state & SOCK_CONNECTING)
		    {
			/* insert code here to determine that the connect succeeded */
			/* ... */
			sock->state ^= SOCK_CONNECTING; /* remove the SOCK_CONNECTING bit */
			
			out->op_type = SOCK_OP_CONNECT;
			out->user_ptr = sock->user_ptr;
			sock->pending_operations--;
			if (sock->closing && sock->pending_operations == 0)
			{
			    MPIU_DBG_PRINTF(("sock_wait: closing socket(%d) after connect completed.\n", sock_getid(sock)));
			    shutdown(sock->sock, SD_BOTH);
			    closesocket(sock->sock);
			    sock->sock = SOCK_INVALID_SOCKET;
			}
			MPIDI_FUNC_EXIT(MPID_STATE_SOCK_WAIT);
			return SOCK_SUCCESS;
		    }
		    else
		    {
			/*MPIU_DBG_PRINTF(("sock_wait: write update, total = %d + %d = %d\n", sock->write.total, num_bytes, sock->write.total + num_bytes));*/
			sock->write.total += num_bytes;
			if (sock->write.use_iov)
			{
			    while (num_bytes)
			    {
				if (sock->write.iov[sock->write.index].SOCK_IOV_LEN <= num_bytes)
				{
				    /*MPIU_DBG_PRINTF(("sock_wait: write.index %d, len %d\n", sock->write.index, 
					sock->write.iov[sock->write.index].SOCK_IOV_LEN));*/
				    num_bytes -= sock->write.iov[sock->write.index].SOCK_IOV_LEN;
				    sock->write.index++;
				    sock->write.iovlen--;
				}
				else
				{
				    /*MPIU_DBG_PRINTF(("sock_wait: partial data written [%d].len = %d, num_bytes = %d\n", sock->write.index,
					sock->write.iov[sock->write.index].SOCK_IOV_LEN, num_bytes));*/
				    sock->write.iov[sock->write.index].SOCK_IOV_LEN -= num_bytes;
				    sock->write.iov[sock->write.index].SOCK_IOV_BUF =
					(char*)(sock->write.iov[sock->write.index].SOCK_IOV_BUF) + num_bytes;
				    num_bytes = 0;
				}
			    }
			    if (sock->write.iovlen == 0)
			    {
				out->num_bytes = sock->write.total;
				out->op_type = SOCK_OP_WRITE;
				out->user_ptr = sock->user_ptr;
				sock->pending_operations--;
				if (sock->closing && sock->pending_operations == 0)
				{
				    MPIU_DBG_PRINTF(("sock_wait: closing socket(%d) after iov write completed.\n", sock_getid(sock)));
				    shutdown(sock->sock, SD_BOTH);
				    closesocket(sock->sock);
				    sock->sock = SOCK_INVALID_SOCKET;
				}
				MPIDI_FUNC_EXIT(MPID_STATE_SOCK_WAIT);
				return SOCK_SUCCESS;
			    }
			    /* make the user upcall */
			    if (sock->write.progress_update != NULL)
				sock->write.progress_update(num_bytes, sock->user_ptr);
			    /* post a write of the remaining data */
			    MPIU_DBG_PRINTF(("sock_wait: posting write of the remaining data, vec size %d\n", sock->write.iovlen));
			    WSASend(sock->sock, sock->write.iov, sock->write.iovlen, &sock->write.num_bytes, 0, &sock->write.ovl, NULL);
			}
			else
			{
			    sock->write.buffer = (char*)(sock->write.buffer) + num_bytes;
			    sock->write.bufflen -= num_bytes;
			    if (sock->write.bufflen == 0)
			    {
				out->num_bytes = sock->write.total;
				out->op_type = SOCK_OP_WRITE;
				out->user_ptr = sock->user_ptr;
				sock->pending_operations--;
				if (sock->closing && sock->pending_operations == 0)
				{
				    MPIU_DBG_PRINTF(("sock_wait: closing socket(%d) after simple write completed.\n", sock_getid(sock)));
				    shutdown(sock->sock, SD_BOTH);
				    closesocket(sock->sock);
				    sock->sock = SOCK_INVALID_SOCKET;
				}
				MPIDI_FUNC_EXIT(MPID_STATE_SOCK_WAIT);
				return SOCK_SUCCESS;
			    }
			    /* make the user upcall */
			    if (sock->write.progress_update != NULL)
				sock->write.progress_update(num_bytes, sock->user_ptr);
			    /* post a write of the remaining data */
			    WriteFile((HANDLE)(sock->sock), sock->write.buffer, sock->write.bufflen, &sock->write.num_bytes, &sock->write.ovl);
			}
		    }
		}
		else
		{
		    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_WAIT);
		    return SOCK_FAIL;
		}
	    }
	    else if (sock->type == SOCK_LISTENER)
	    {
		sock->state |= SOCK_ACCEPTED;
		out->num_bytes = num_bytes;
		out->op_type = SOCK_OP_ACCEPT;
		out->user_ptr = sock->user_ptr;
		MPIDI_FUNC_EXIT(MPID_STATE_SOCK_WAIT);
		return SOCK_SUCCESS;
	    }
	    else
	    {
		MPIDI_FUNC_EXIT(MPID_STATE_SOCK_WAIT);
		return SOCK_FAIL;
	    }
	}
	else
	{
	    error = GetLastError();
	    /* interpret error, return appropriate SOCK_ERR_... macro */
	    if (error == WAIT_TIMEOUT)
	    {
		MPIDI_FUNC_EXIT(MPID_STATE_SOCK_WAIT);
		return SOCK_ERR_TIMEOUT;
	    }
	    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_WAIT);
	    return SOCK_FAIL;
	}
    }
    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_WAIT);
}

int sock_set_user_ptr(sock_t sock, void *user_ptr)
{
    MPIDI_STATE_DECL(MPID_STATE_SOCK_SET_USER_PTR);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCK_SET_USER_PTR);
    if (sock == NULL)
    {
	MPIDI_FUNC_EXIT(MPID_STATE_SOCK_SET_USER_PTR);
	return SOCK_FAIL;
    }
    sock->user_ptr = user_ptr;
    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_SET_USER_PTR);
    return SOCK_SUCCESS;
}

/* immediate functions */

int sock_read(sock_t sock, void *buf, int len, int *num_read)
{
    MPIDI_STATE_DECL(MPID_STATE_SOCK_READ);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCK_READ);
    *num_read = recv(sock->sock, buf, len, 0);
    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_READ);
    return SOCK_SUCCESS;
}

int sock_readv(sock_t sock, SOCK_IOV *iov, int n, int *num_read)
{
    DWORD nFlags = 0;
    MPIDI_STATE_DECL(MPID_STATE_SOCK_READV);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCK_READV);

    if (WSARecv(sock->sock, iov, n, num_read, &nFlags, NULL/*overlapped*/, NULL/*completion routine*/) == SOCKET_ERROR)
    {
	if (WSAGetLastError() != WSAEWOULDBLOCK)
	{
	    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_READV);
	    return SOCKET_ERROR;
	}
	*num_read = 0;
    }

    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_READV);
    return SOCK_SUCCESS;
}

int sock_write(sock_t sock, void *buf, int len, int *num_written)
{
    MPIDI_STATE_DECL(MPID_STATE_SOCK_WRITE);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCK_WRITE);
    *num_written = send(sock->sock, buf, len, 0);
    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_WRITE);
    return SOCK_SUCCESS;
}

int sock_writev(sock_t sock, SOCK_IOV *iov, int n, int *num_written)
{
    MPIDI_STATE_DECL(MPID_STATE_SOCK_WRITEV);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCK_WRITEV);
    if (n == 0)
    {
	MPIU_DBG_PRINTF(("empty vector passed into sock_writev\n"));
	MPIDI_FUNC_EXIT(MPID_STATE_SOCK_WRITEV);
	return 0;
    }
    if (WSASend(sock->sock, iov, n, num_written, 0, NULL/*overlapped*/, NULL/*completion routine*/) == SOCKET_ERROR)
    {
	if (WSAGetLastError() != WSAEWOULDBLOCK)
	{
	    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_WRITEV);
	    return SOCKET_ERROR;
	}
    }
    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_WRITEV);
    return SOCK_SUCCESS;
}


/* non-blocking functions */

int sock_post_read(sock_t sock, void *buf, int len, int (*rfn)(int, void*))
{
    MPIDI_STATE_DECL(MPID_STATE_SOCK_POST_READ);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCK_POST_READ);
    sock->read.total = 0;
    sock->read.buffer = buf;
    sock->read.bufflen = len;
    sock->read.use_iov = FALSE;
    sock->read.progress_update = rfn;
    sock->state |= SOCK_READING;
    sock->pending_operations++;
    ReadFile((HANDLE)(sock->sock), buf, len, &sock->read.num_bytes, &sock->read.ovl);
    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_POST_READ);
    return SOCK_SUCCESS;
}

int sock_post_readv(sock_t sock, SOCK_IOV *iov, int n, int (*rfn)(int, void*))
{
    DWORD flags = 0;
    MPIDI_STATE_DECL(MPID_STATE_SOCK_POST_READV);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCK_POST_READV);
    sock->read.total = 0;
    /*sock->read.iov = iov;*/
    memcpy(sock->read.iov, iov, sizeof(SOCK_IOV) * n);
    sock->read.iovlen = n;
    sock->read.index = 0;
    sock->read.use_iov = TRUE;
    sock->read.progress_update = rfn;
    sock->state |= SOCK_READING;
    sock->pending_operations++;
    WSARecv(sock->sock, sock->read.iov, n, &sock->read.num_bytes, &flags, &sock->read.ovl, NULL);
    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_POST_READV);
    return SOCK_SUCCESS;
}

int sock_post_write(sock_t sock, void *buf, int len, int (*wfn)(int, void*))
{
    MPIDI_STATE_DECL(MPID_STATE_SOCK_POST_WRITE);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCK_POST_WRITE);
    sock->write.total = 0;
    sock->write.buffer = buf;
    sock->write.bufflen = len;
    sock->write.use_iov = FALSE;
    sock->write.progress_update = wfn;
    sock->state |= SOCK_WRITING;
    sock->pending_operations++;
    WriteFile((HANDLE)(sock->sock), buf, len, &sock->write.num_bytes, &sock->write.ovl);
    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_POST_WRITE);
    return SOCK_SUCCESS;
}

int sock_post_writev(sock_t sock, SOCK_IOV *iov, int n, int (*wfn)(int, void*))
{
    MPIDI_STATE_DECL(MPID_STATE_SOCK_POST_WRITEV);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCK_POST_WRITEV);
    sock->write.total = 0;
    /*sock->write.iov = iov;*/
    memcpy(sock->write.iov, iov, sizeof(SOCK_IOV) * n);
    sock->write.iovlen = n;
    sock->write.index = 0;
    sock->write.use_iov = TRUE;
    sock->write.progress_update = wfn;
    sock->state |= SOCK_WRITING;
    sock->pending_operations++;
    /*
    {
	char str[1024], *s = str;
	int i;
	s += sprintf(s, "sock_post_writev(");
	for (i=0; i<n; i++)
	    s += sprintf(s, "%d,", iov[i].SOCK_IOV_LEN);
	sprintf(s, ")\n");
	MPIU_DBG_PRINTF(("%s", str));
    }
    */
    WSASend(sock->sock, sock->write.iov, n, &sock->write.num_bytes, 0, &sock->write.ovl, NULL);
    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_POST_WRITEV);
    return SOCK_SUCCESS;
}

/* extended functions */

int sock_getid(sock_t sock)
{
    return (int)sock->sock;
}

int sock_easy_receive(sock_t sock, void *buf, int len, int *num_read)
{
    int error;
    int n;
    int total = 0;
    MPIDI_STATE_DECL(MPID_STATE_SOCK_EASY_RECEIVE);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCK_EASY_RECEIVE);

    while (len)
    {
	error = sock_read(sock, buf, len, &n);
	if (error != SOCK_SUCCESS)
	{
	    *num_read = total;
	    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_EASY_RECEIVE);
	    return error;
	}
	total += n;
	buf = (char*)buf + n;
	len -= n;
    }

    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_EASY_RECEIVE);
    return SOCK_SUCCESS;
}

int sock_easy_send(sock_t sock, void *buf, int len, int *num_written)
{
    int error;
    int n;
    int total = 0;
    MPIDI_STATE_DECL(MPID_STATE_SOCK_EASY_SEND);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCK_EASY_SEND);

    while (len)
    {
	error = sock_write(sock, buf, len, &n);
	if (error != SOCK_SUCCESS)
	{
	    *num_written = total;
	    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_EASY_SEND);
	    return error;
	}
	total += n;
	buf = (char*)buf + n;
	len -= n;
    }

    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_EASY_SEND);
    return SOCK_SUCCESS;
}

#endif /* WITH_SOCK_TYPE == SOCK_IOCP  */
