/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "mpidu_sock.h"

#define MPL_QUOTE(A) MPL_QUOTE2(A)
#define MPL_QUOTE2(A) #A

#define SOCKI_TCP_BUFFER_SIZE       32*1024
#define SOCKI_DESCRIPTION_LENGTH    256
#define SOCKI_NUM_PREPOSTED_ACCEPTS 32

typedef enum SOCKI_TYPE { SOCKI_INVALID, SOCKI_LISTENER, SOCKI_SOCKET, SOCKI_WAKEUP } SOCKI_TYPE;

typedef int SOCKI_STATE;
#define SOCKI_ACCEPTING  0x0001
#define SOCKI_CONNECTING 0x0004
#define SOCKI_READING    0x0008
#define SOCKI_WRITING    0x0010

typedef struct sock_buffer
{
    DWORD num_bytes;
    OVERLAPPED ovl;
    MPL_IOV tiov;
#ifdef USE_SOCK_IOV_COPY
    MPL_IOV iov[MPL_IOV_MAXLEN];
#else
    MPL_IOV *iov;
#endif
    int iovlen;
    int index;
    MPIU_Size_t total;
    MPIDU_Sock_progress_update_func_t progress_update;
} sock_buffer;

typedef struct sock_state_t
{
    SOCKI_TYPE type;
    SOCKI_STATE state;
    SOCKET sock;
    MPIDU_Sock_set_t set;
    int closing;
    int pending_operations;
    /* listener/accept structures */
    SOCKET listen_sock;
    char accept_buffer[sizeof(struct sockaddr_in)*2+32];
    /* read and write structures */
    sock_buffer read;
    sock_buffer write;
    /* connect structures */
    char *cur_host;
    char host_description[SOCKI_DESCRIPTION_LENGTH];
    /* user pointer */
    void *user_ptr;
    /* internal list */
    struct sock_state_t *list, *next;
    int accepted;
    int listener_closed;
    /* timing variables for completion notification */
    /*
    double rt1, rt2;
    double wt1, wt2;
    double ct1, ct2;
    */
    struct sock_state_t *next_sock;
} sock_state_t;

static int g_num_cp_threads = 2;
static int g_socket_buffer_size = SOCKI_TCP_BUFFER_SIZE;
static int g_socket_rbuffer_size = SOCKI_TCP_BUFFER_SIZE;
static int g_socket_sbuffer_size = SOCKI_TCP_BUFFER_SIZE;
static int g_init_called = 0;
static int g_num_posted_accepts = SOCKI_NUM_PREPOSTED_ACCEPTS;

static sock_state_t *g_sock_list = NULL;

/* empty structure used to wake up a sock_wait call */
sock_state_t g_wakeup_state;

static void translate_error(int error, char *msg, char *prepend)
{
    HLOCAL str;
    int num_bytes;
    num_bytes = FormatMessage(
	FORMAT_MESSAGE_FROM_SYSTEM |
	FORMAT_MESSAGE_ALLOCATE_BUFFER,
	0,
	error,
	MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),
	(LPTSTR) &str,
	0,0);
    if (num_bytes == 0)
    {
	if (prepend != NULL)
	    MPIU_Strncpy(msg, prepend, 1024);
	else
	    *msg = '\0';
    }
    else
    {
	if (prepend == NULL)
	    memcpy(msg, str, num_bytes+1);
	else
	    MPL_snprintf(msg, 1024, "%s%s", prepend, (const char*)str);
	LocalFree(str);
	strtok(msg, "\r\n");
    }
}

static char *get_error_string(int error_code)
{
    /* obviously not thread safe to store a message like this */
    static char error_msg[1024];
    translate_error(error_code, error_msg, NULL);
    return error_msg;
}

#undef FUNCNAME
#define FUNCNAME easy_create
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int easy_create_ranged(SOCKET *sock, int port, unsigned long addr)
{
    int mpi_errno=MPI_SUCCESS;
    /*struct linger linger;*/
    int optval, len;
    SOCKET temp_sock;
    SOCKADDR_IN sockAddr;
    int use_range = 0;

    /* create the non-blocking socket */
    temp_sock = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
    if (temp_sock == INVALID_SOCKET)
    {
	mpi_errno = WSAGetLastError();
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**socket", "**socket %s %d", get_error_string(mpi_errno), mpi_errno);
	return mpi_errno;
    }
    
    MPIR_ERR_CHKANDJUMP(MPIR_CVAR_CH3_PORT_RANGE.low < 0 || MPIR_CVAR_CH3_PORT_RANGE.low > MPIR_CVAR_CH3_PORT_RANGE.high, mpi_errno, MPI_ERR_OTHER, "**badportrange");
    if (port == 0 && MPIR_CVAR_CH3_PORT_RANGE.low != 0 && MPIR_CVAR_CH3_PORT_RANGE.high != 0)
    {
	use_range = 1;
	port = MPIR_CVAR_CH3_PORT_RANGE.low;
    }

    memset(&sockAddr,0,sizeof(sockAddr));
    
    sockAddr.sin_family = AF_INET;
    sockAddr.sin_addr.s_addr = addr;
    sockAddr.sin_port = htons((unsigned short)port);

    for (;;)
    {
	if (bind(temp_sock, (SOCKADDR*)&sockAddr, sizeof(sockAddr)) == SOCKET_ERROR)
	{
	    if (use_range)
	    {
		port++;
		if (port > MPIR_CVAR_CH3_PORT_RANGE.high)
		{
		    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**socket", 0);
		    return mpi_errno;
		}
		sockAddr.sin_port = htons((unsigned short)port);
	    }
	    else
	    {
		mpi_errno = WSAGetLastError();
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**socket", "**socket %s %d", get_error_string(mpi_errno), mpi_errno);
		return mpi_errno;
	    }
	}
	else
	{
	    break;
	}
    }

    /* Set the linger on close option */
    /*
    linger.l_onoff = 1 ;
    linger.l_linger = 60;
    setsockopt(temp_sock, SOL_SOCKET, SO_LINGER, (char*)&linger, sizeof(linger));
    */

    /* set the socket to non-blocking */
    /*
    optval = TRUE;
    ioctlsocket(temp_sock, FIONBIO, &optval);
    */

    /* set the socket buffers */
    len = sizeof(int);
    if (!getsockopt(temp_sock, SOL_SOCKET, SO_RCVBUF, (char*)&optval, &len))
    {
	optval = g_socket_rbuffer_size;
	setsockopt(temp_sock, SOL_SOCKET, SO_RCVBUF, (char*)&optval, sizeof(int));
    }
    len = sizeof(int);
    if (!getsockopt(temp_sock, SOL_SOCKET, SO_SNDBUF, (char*)&optval, &len))
    {
	optval = g_socket_sbuffer_size;
	setsockopt(temp_sock, SOL_SOCKET, SO_SNDBUF, (char*)&optval, sizeof(int));
    }

    /* prevent the socket from being inherited by child processes */
    if (!DuplicateHandle(
	GetCurrentProcess(), (HANDLE)temp_sock,
	GetCurrentProcess(), (HANDLE*)sock,
	0, FALSE, DUPLICATE_CLOSE_SOURCE | DUPLICATE_SAME_ACCESS))
    {
	mpi_errno = GetLastError();
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**duphandle", "**duphandle %s %d", get_error_string(mpi_errno), mpi_errno);
	return mpi_errno;
    }

    /* Set the no-delay option */
    setsockopt(*sock, IPPROTO_TCP, TCP_NODELAY, (char *)&optval, sizeof(optval));

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME easy_create
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int easy_create(SOCKET *sock, int port, unsigned long addr)
{
    int mpi_errno;
    /*struct linger linger;*/
    int optval, len;
    SOCKET temp_sock;
    SOCKADDR_IN sockAddr;

    /* create the non-blocking socket */
    temp_sock = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
    if (temp_sock == INVALID_SOCKET)
    {
	mpi_errno = WSAGetLastError();
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**socket", "**socket %s %d", get_error_string(mpi_errno), mpi_errno);
	return mpi_errno;
    }
    
    memset(&sockAddr,0,sizeof(sockAddr));
    
    sockAddr.sin_family = AF_INET;
    sockAddr.sin_addr.s_addr = addr;
    sockAddr.sin_port = htons((unsigned short)port);

    if (bind(temp_sock, (SOCKADDR*)&sockAddr, sizeof(sockAddr)) == SOCKET_ERROR)
    {
	mpi_errno = WSAGetLastError();
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**socket", "**socket %s %d", get_error_string(mpi_errno), mpi_errno);
	return mpi_errno;
    }
    
    /* Set the linger on close option */
    /*
    linger.l_onoff = 1 ;
    linger.l_linger = 60;
    setsockopt(temp_sock, SOL_SOCKET, SO_LINGER, (char*)&linger, sizeof(linger));
    */

    /* set the socket to non-blocking */
    /*
    optval = TRUE;
    ioctlsocket(temp_sock, FIONBIO, &optval);
    */

    /* set the socket buffers */
    len = sizeof(int);
    if (!getsockopt(temp_sock, SOL_SOCKET, SO_RCVBUF, (char*)&optval, &len))
    {
	optval = g_socket_rbuffer_size;
	setsockopt(temp_sock, SOL_SOCKET, SO_RCVBUF, (char*)&optval, sizeof(int));
    }
    len = sizeof(int);
    if (!getsockopt(temp_sock, SOL_SOCKET, SO_SNDBUF, (char*)&optval, &len))
    {
	optval = g_socket_sbuffer_size;
	setsockopt(temp_sock, SOL_SOCKET, SO_SNDBUF, (char*)&optval, sizeof(int));
    }

    /* prevent the socket from being inherited by child processes */
    if (!DuplicateHandle(
	GetCurrentProcess(), (HANDLE)temp_sock,
	GetCurrentProcess(), (HANDLE*)sock,
	0, FALSE, DUPLICATE_CLOSE_SOURCE | DUPLICATE_SAME_ACCESS))
    {
	mpi_errno = GetLastError();
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**duphandle", "**duphandle %s %d", get_error_string(mpi_errno), mpi_errno);
	return mpi_errno;
    }

    /* Set the no-delay option */
    setsockopt(*sock, IPPROTO_TCP, TCP_NODELAY, (char *)&optval, sizeof(optval));

    return MPI_SUCCESS;
}

static inline int easy_get_sock_info(SOCKET sock, char *name, int *port)
{
    struct sockaddr_in addr;
    int name_len = sizeof(addr);
    DWORD len = 100;

    getsockname(sock, (struct sockaddr*)&addr, &name_len);
    *port = ntohs(addr.sin_port);
    /*GetComputerName(name, &len);*/
    GetComputerNameEx(ComputerNameDnsFullyQualified, name, &len);
    /*gethostname(name, 100);*/

    return 0;
}

static inline void init_state_struct(sock_state_t *p)
{
    p->listen_sock = INVALID_SOCKET;
    p->sock = INVALID_SOCKET;
    p->set = INVALID_HANDLE_VALUE;
    p->user_ptr = NULL;
    p->type = 0;
    p->state = 0;
    p->closing = FALSE;
    p->pending_operations = 0;
    p->read.total = 0;
    p->read.num_bytes = 0;
    p->read.tiov.MPL_IOV_BUF = NULL;
#ifndef USE_SOCK_IOV_COPY
    p->read.iov = NULL;
#endif
    p->read.iovlen = 0;
    p->read.ovl.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    p->read.ovl.Offset = 0;
    p->read.ovl.OffsetHigh = 0;
    p->read.progress_update = NULL;
    p->write.total = 0;
    p->write.num_bytes = 0;
    p->write.tiov.MPL_IOV_BUF = NULL;
#ifndef USE_SOCK_IOV_COPY
    p->write.iov = NULL;
#endif
    p->write.iovlen = 0;
    p->write.ovl.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    p->write.ovl.Offset = 0;
    p->write.ovl.OffsetHigh = 0;
    p->write.progress_update = NULL;
    p->list = NULL;
    p->next = NULL;
    p->accepted = 0;
    p->listener_closed = 0;
    /*
    p->bogus_t1 = 0;
    p->bogus_t2 = 0;
    */
    p->next_sock = g_sock_list;
    g_sock_list = p;
}

#undef FUNCNAME
#define FUNCNAME post_next_accept
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int post_next_accept(sock_state_t * context)
{
    int mpi_errno;
    context->state = SOCKI_ACCEPTING;
    context->accepted = 0;
    /*printf("posting an accept.\n");fflush(stdout);*/
    context->sock = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
    if (context->sock == INVALID_SOCKET)
    {
	mpi_errno = WSAGetLastError();
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**fail", "**fail %s %d", get_error_string(mpi_errno), mpi_errno);
	return mpi_errno;
    }
    if (!AcceptEx(
	context->listen_sock, 
	context->sock, 
	context->accept_buffer, 
	0, 
	sizeof(struct sockaddr_in)+16, 
	sizeof(struct sockaddr_in)+16, 
	&context->read.num_bytes,
	&context->read.ovl))
    {
	mpi_errno = WSAGetLastError();
	if (mpi_errno == ERROR_IO_PENDING)
	    return MPI_SUCCESS;
	/*MPL_error_printf("AcceptEx failed with error %d\n", error);fflush(stdout);*/
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**fail", "**fail %s %d", get_error_string(mpi_errno), mpi_errno);
	return mpi_errno;
    }
    return MPI_SUCCESS;
}

/* sock functions */

#undef FUNCNAME
#define FUNCNAME MPIDU_Sock_init
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDU_Sock_init()
{
    int mpi_errno;
    char *szNum, *szRange;
    WSADATA wsaData;
    int err;
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_SOCK_INIT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_SOCK_INIT);

    if (g_init_called)
    {
	g_init_called++;
	/*printf("sock init %d\n", g_init_called);fflush(stdout);*/
	/*
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_INIT, "**sock_init", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_INIT);
	return mpi_errno;
	*/
	return MPI_SUCCESS;
    }

    /* Start the Winsock dll */
    if ((err = WSAStartup(MAKEWORD(2, 0), &wsaData)) != 0)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**wsasock", "**wsasock %s %d", get_error_string(err), err);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_INIT);
	return mpi_errno;
    }

    /* get the socket buffer size */
    szNum = getenv("MPICH_SOCKET_BUFFER_SIZE");
    if (szNum != NULL)
    {
	g_socket_buffer_size = atoi(szNum);
	if (g_socket_buffer_size < 0)
	    g_socket_buffer_size = SOCKI_TCP_BUFFER_SIZE;
    }

    g_socket_rbuffer_size = g_socket_buffer_size;
    g_socket_sbuffer_size = g_socket_buffer_size;

    szNum = getenv("MPICH_SOCKET_RBUFFER_SIZE");
    if (szNum != NULL)
    {
	g_socket_rbuffer_size = atoi(szNum);
	if (g_socket_rbuffer_size < 0)
	    g_socket_rbuffer_size = g_socket_buffer_size;
    }

    szNum = getenv("MPICH_SOCKET_SBUFFER_SIZE");
    if (szNum != NULL)
    {
	g_socket_sbuffer_size = atoi(szNum);
	if (g_socket_sbuffer_size < 0)
	    g_socket_sbuffer_size = g_socket_buffer_size;
    }

    /* get the number of accepts to pre-post */
    szNum = getenv("MPICH_SOCKET_NUM_PREPOSTED_ACCEPTS");
    if (szNum != NULL)
    {
	g_num_posted_accepts = atoi(szNum);
	if (g_num_posted_accepts < 1)
	    g_num_posted_accepts = SOCKI_NUM_PREPOSTED_ACCEPTS;
    }

    init_state_struct(&g_wakeup_state);
    g_wakeup_state.type = SOCKI_WAKEUP;

    g_init_called = 1;
    /*printf("sock init %d\n", g_init_called);fflush(stdout);*/

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_INIT);
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPIDU_Sock_finalize
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDU_Sock_finalize()
{
    int mpi_errno;
    sock_state_t *iter;
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_SOCK_FINALIZE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_SOCK_FINALIZE);
    if (!g_init_called)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_INIT, "**sock_init", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_FINALIZE);
	return mpi_errno;
    }
    g_init_called--;
    if (g_init_called == 0)
    {
	iter = g_sock_list;
	while (iter)
	{
	    if (iter->sock != INVALID_SOCKET)
	    {
		/*printf("sock %d not closed before finalize\n", iter->sock);fflush(stdout);*/
		closesocket(iter->sock);
		iter->sock = INVALID_SOCKET;
	    }
	    iter = iter->next_sock;
	    if (iter == g_sock_list)
	    {
		/* catch loops */
		/*printf("sock list has a loop\n");fflush(stdout);*/
		break;
	    }
	}
	WSACleanup();
    }
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_FINALIZE);
    return MPI_SUCCESS;
}

typedef struct socki_host_name_t
{
    char host[256];
    struct socki_host_name_t *next;
} socki_host_name_t;

static int already_used_or_add(char *host, socki_host_name_t **list)
{
    socki_host_name_t *iter, *trailer;

    /* check if the host already has been used */
    iter = trailer = *list;
    while (iter)
    {
	if (strcmp(iter->host, host) == 0)
	{
	    return 1;
	}
	if (trailer != iter)
	    trailer = trailer->next;
	iter = iter->next;
    }

    /* the host has not been used so add a node for it */
    iter = (socki_host_name_t*)MPIU_Malloc(sizeof(socki_host_name_t));
    if (!iter)
    {
	/* if out of memory then treat it as not found */
	return 0;
    }
    MPIU_Strncpy(iter->host, host, 256);

    /* insert new hosts at the end of the list */
    if (trailer != NULL)
    {
        trailer->next = iter;
        iter->next = NULL;
    }
    else
    {
        iter->next = NULL;
        *list = iter;
    }
    /* insert new hosts at the beginning of the list                            
    iter->next = *list;                                                         
    *list = iter;                                                               
    */

    return 0;
}

static void socki_free_host_list(socki_host_name_t *list)
{
    socki_host_name_t *iter;
    while (list)
    {
	iter = list;
	list = list->next;
	MPIU_Free(iter);
    }
}

static int socki_get_host_list(char *hostname, socki_host_name_t **listp)
{
    int mpi_errno;
    struct addrinfo *res, *iter, hint;
    char host[256];
    socki_host_name_t *list = NULL;

    /* add the hostname to the beginning of the list */
    already_used_or_add(hostname, &list);

    hint.ai_flags = AI_PASSIVE | AI_CANONNAME;
    hint.ai_family = PF_UNSPEC;
    hint.ai_socktype = SOCK_STREAM;
    hint.ai_protocol = 0;
    hint.ai_addrlen = 0;
    hint.ai_canonname = NULL;
    hint.ai_addr = 0;
    hint.ai_next = NULL;
    if (getaddrinfo(hostname, NULL, NULL/*&hint*/, &res))
    {
        mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**getinfo", "**getinfo %s %d", strerror(errno), errno);
        return mpi_errno;
    }

    /* add the host names */
    iter = res;
    while (iter)
    {
        if (iter->ai_canonname)
        {
            already_used_or_add(iter->ai_canonname, &list);
        }
        else
        {
            switch (iter->ai_family)
            {
            case PF_INET:
            case PF_INET6:
                if (getnameinfo(iter->ai_addr, (socklen_t)iter->ai_addrlen, host, 256, NULL, 0, 0) == 0)
                {
                    already_used_or_add(host, &list);
                }
                break;
            }
        }
        iter = iter->ai_next;
    }
    /* add the names again, this time as ip addresses */
    iter = res;
    while (iter)
    {
        switch (iter->ai_family)
        {
        case PF_INET:
        case PF_INET6:
            if (getnameinfo(iter->ai_addr, (socklen_t)iter->ai_addrlen, host, 256, NULL, 0, NI_NUMERICHOST) == 0)
            {
                already_used_or_add(host, &list);
            }
            break;
        }
        iter = iter->ai_next;
    }
    if (res)
    {
        freeaddrinfo(res);
    }

    *listp = list;
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPIDU_Sock_hostname_to_host_description
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDU_Sock_hostname_to_host_description(char *hostname, char *host_description, int len)
{
    int mpi_errno = MPI_SUCCESS;
    int str_errno = MPIU_STR_SUCCESS;
    socki_host_name_t *iter, *list = NULL;
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_SOCK_HOSTNAME_TO_HOST_DESCRIPTION);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_SOCK_HOSTNAME_TO_HOST_DESCRIPTION);
    if (!g_init_called)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_INIT, "**sock_init", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_HOSTNAME_TO_HOST_DESCRIPTION);
	return mpi_errno;
    }

    mpi_errno = socki_get_host_list(hostname, &list);
    if (mpi_errno != MPI_SUCCESS)
    {
        mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
        goto fn_exit;
    }

    iter = list;
    while (iter)
    {
        MPIU_DBG_PRINTF(("adding host: %s\n", iter->host));
        str_errno = MPIU_Str_add_string(&host_description, &len, iter->host);
        MPIR_ERR_CHKANDJUMP(str_errno, mpi_errno, MPIDU_SOCK_ERR_NOMEM, "**desc_len");

        iter = iter->next;
    }

 fn_exit:
    socki_free_host_list(list);

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_HOSTNAME_TO_HOST_DESCRIPTION);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDU_Sock_get_host_description
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDU_Sock_get_host_description(int myRank, 
				    char * host_description, int len)
{
    int mpi_errno;
    char hostname[100];
    DWORD length = 100;
    char *env;
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_SOCK_GET_HOST_DESCRIPTION);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_SOCK_GET_HOST_DESCRIPTION);

    MPIU_UNREFERENCED_ARG(myRank);

    if (!g_init_called)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_INIT, "**sock_init", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_GET_HOST_DESCRIPTION);
	return mpi_errno;
    }

    env = getenv("MPICH_INTERFACE_HOSTNAME");
    if (env != NULL && *env != '\0')
    {
	MPIU_Strncpy(hostname, env, sizeof(hostname));
    }
    else
    {
	/*if (gethostname(hostname, 100) == SOCKET_ERROR)*/
	/*if (!GetComputerName(hostname, &length))*/
	if (!GetComputerNameEx(ComputerNameDnsFullyQualified, hostname, &length))
	{
	    mpi_errno = WSAGetLastError();
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**sock_gethost", "**sock_gethost %s %d", get_error_string(mpi_errno), mpi_errno);
	    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_GET_HOST_DESCRIPTION);
	    return mpi_errno;
	}
    }

    mpi_errno = MPIDU_Sock_hostname_to_host_description(hostname, host_description, len);
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
    }

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_GET_HOST_DESCRIPTION);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDU_Sock_create_set
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDU_Sock_create_set(MPIDU_Sock_set_t * set)
{
    int mpi_errno;
    HANDLE port;
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_SOCK_CREATE_SET);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_SOCK_CREATE_SET);
    if (!g_init_called)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_INIT, "**sock_init", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_CREATE_SET);
	return mpi_errno;
    }
    port = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, g_num_cp_threads);
    if (port != NULL)
    {
	*set = port;
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_CREATE_SET);
	return MPI_SUCCESS;
    }
    mpi_errno = GetLastError();
    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**iocp", "**iocp %s %d", get_error_string(mpi_errno), mpi_errno);
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_CREATE_SET);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDU_Sock_destroy_set
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDU_Sock_destroy_set(MPIDU_Sock_set_t set)
{
    int mpi_errno;
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_SOCK_DESTROY_SET);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_SOCK_DESTROY_SET);
    if (!g_init_called)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_INIT, "**sock_init", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_DESTROY_SET);
	return mpi_errno;
    }
    if (!CloseHandle(set))
    {
	mpi_errno = GetLastError();
	if (mpi_errno == ERROR_INVALID_HANDLE)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_BAD_SET, "**bad_set", 0);
	    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_DESTROY_SET);
	    return mpi_errno;
	}
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**fail", "**fail %s %d", get_error_string(mpi_errno), mpi_errno);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_DESTROY_SET);
	return mpi_errno;
    }
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_DESTROY_SET);
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPIDU_Sock_native_to_sock
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDU_Sock_native_to_sock(MPIDU_Sock_set_t set, MPIDU_SOCK_NATIVE_FD fd, void *user_ptr, MPIDU_Sock_t *sock_ptr)
{
    int mpi_errno;
    /*int ret_val;*/
    sock_state_t *sock_state;
    /*u_long optval;*/
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_SOCK_NATIVE_TO_SOCK);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_SOCK_NATIVE_TO_SOCK);
    if (!g_init_called)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_INIT, "**sock_init", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_NATIVE_TO_SOCK);
	return mpi_errno;
    }

    /* setup the structures */
    sock_state = (sock_state_t*)MPIU_Malloc(sizeof(sock_state_t));
    if (sock_state == NULL)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_NOMEM, "**nomem", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_NATIVE_TO_SOCK);
	return mpi_errno;
    }
    init_state_struct(sock_state);
    sock_state->sock = (SOCKET)fd;

    /* set the socket to non-blocking */
    /* leave the native handle in the state passed in?
    optval = TRUE;
    ioctlsocket(sock_state->sock, FIONBIO, &optval);
    */

    sock_state->user_ptr = user_ptr;
    sock_state->type = SOCKI_SOCKET;
    sock_state->state = 0;
    sock_state->set = set;

    /* associate the socket with the completion port */
    /*printf("CreateIOCompletionPort(%d, %p, %p, %d)\n", sock_state->sock, set, sock_state, g_num_cp_threads);fflush(stdout);*/
    if (CreateIoCompletionPort((HANDLE)sock_state->sock, set, (ULONG_PTR)sock_state, g_num_cp_threads) == NULL)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_NOMEM, "**nomem", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_NATIVE_TO_SOCK);
	return mpi_errno;
    }

    *sock_ptr = sock_state;

    /*printf("native socket %d\n", sock_state->sock);fflush(stdout);*/
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_NATIVE_TO_SOCK);
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPIDU_Sock_listen
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDU_Sock_listen(MPIDU_Sock_set_t set, void * user_ptr, int * port, MPIDU_Sock_t * sock)
{
    int mpi_errno;
    char host[100];
    sock_state_t * listen_state, **listener_copies;
    int i;
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_SOCK_LISTEN);
    MPIDI_STATE_DECL(MPID_STATE_MEMCPY);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_SOCK_LISTEN);
    if (!g_init_called)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_INIT, "**sock_init", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_LISTEN);
	return mpi_errno;
    }

    listen_state = (sock_state_t*)MPIU_Malloc(sizeof(sock_state_t));
    init_state_struct(listen_state);
    mpi_errno = easy_create_ranged(&listen_state->listen_sock, *port, INADDR_ANY);
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**sock_create", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_LISTEN);
	return mpi_errno;
    }
    if (listen(listen_state->listen_sock, SOMAXCONN) == SOCKET_ERROR)
    {
	mpi_errno = WSAGetLastError();
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**listen", "**listen %s %d", get_error_string(mpi_errno), mpi_errno);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_LISTEN);
	return mpi_errno;
    }
    if (CreateIoCompletionPort((HANDLE)listen_state->listen_sock, set, (ULONG_PTR)listen_state, g_num_cp_threads) == NULL)
    {
	mpi_errno = GetLastError();
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**iocp", "**iocp %s %d", get_error_string(mpi_errno), mpi_errno);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_LISTEN);
	return mpi_errno;
    }
    easy_get_sock_info(listen_state->listen_sock, host, port);
    listen_state->user_ptr = user_ptr;
    listen_state->type = SOCKI_LISTENER;
    listen_state->set = set;

    /* post the accept(s) last to make sure the listener state structure is completely initialized before
       a completion thread has the chance to satisfy the AcceptEx call */

    listener_copies = (sock_state_t**)MPIU_Malloc(g_num_posted_accepts * sizeof(sock_state_t*));
    for (i=0; i<g_num_posted_accepts; i++)
    {
	listener_copies[i] = (sock_state_t*)MPIU_Malloc(sizeof(sock_state_t));
	MPIDI_FUNC_ENTER(MPID_STATE_MEMCPY);
	memcpy(listener_copies[i], listen_state, sizeof(*listen_state));
	MPIDI_FUNC_EXIT(MPID_STATE_MEMCPY);
	if (i > 0)
	{
	    listener_copies[i]->next = listener_copies[i-1];
	}
	mpi_errno = post_next_accept(listener_copies[i]);
	if (mpi_errno != MPI_SUCCESS)
	{
	    MPIU_Free(listener_copies);
	    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**post_accept", 0);
	    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_LISTEN);
	    return mpi_errno;
	}
    }
    listen_state->list = listener_copies[g_num_posted_accepts-1];
    MPIU_Free(listener_copies);

    *sock = listen_state;
    /*printf("listening socket %d\n", listen_state->listen_sock);fflush(stdout);*/
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_LISTEN);
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPIDU_Sock_accept
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDU_Sock_accept(MPIDU_Sock_t listener_sock, MPIDU_Sock_set_t set, void * user_ptr, MPIDU_Sock_t * sock)
{
    int mpi_errno;
    BOOL b;
    /*struct linger linger;*/
    u_long optval;
    int len;
    sock_state_t *accept_state, *iter;
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_SOCK_ACCEPT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_SOCK_ACCEPT);
    if (!g_init_called)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_INIT, "**sock_init", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_ACCEPT);
	return mpi_errno;
    }

    accept_state = MPIU_Malloc(sizeof(sock_state_t));
    if (accept_state == NULL)
    {
	*sock = NULL;
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_NOMEM, "**nomem", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_ACCEPT);
	return mpi_errno;
    }
    init_state_struct(accept_state);

    accept_state->type = SOCKI_SOCKET;

    /* find the listener copy that satisfied the acceptex call and post another accept */
    iter = listener_sock->list;
    while (iter != NULL && iter->accepted == 0)
	iter = iter->next;
    if (iter == NULL)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**sock_nop_accept", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_ACCEPT);
	return mpi_errno;
    }
    accept_state->sock = iter->sock;
    mpi_errno = post_next_accept(iter);
    if (mpi_errno != MPI_SUCCESS)
    {
	*sock = NULL;
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**post_accept", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_ACCEPT);
	return mpi_errno;
    }

    /* finish the accept */
    setsockopt(accept_state->sock, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (char *)&listener_sock->listen_sock, sizeof(listener_sock->listen_sock));

    /* set the socket to non-blocking */
    optval = TRUE;
    ioctlsocket(accept_state->sock, FIONBIO, &optval);

    /* set the linger option */
    /*
    linger.l_onoff = 1;
    linger.l_linger = 60;
    setsockopt(accept_state->sock, SOL_SOCKET, SO_LINGER, (char*)&linger, sizeof(linger));
    */

    /* set the socket buffers */
    len = sizeof(int);
    if (!getsockopt(accept_state->sock, SOL_SOCKET, SO_RCVBUF, (char*)&optval, &len))
    {
	optval = g_socket_rbuffer_size;
	setsockopt(accept_state->sock, SOL_SOCKET, SO_RCVBUF, (char*)&optval, sizeof(int));
    }
    len = sizeof(int);
    if (!getsockopt(accept_state->sock, SOL_SOCKET, SO_SNDBUF, (char*)&optval, &len))
    {
	optval = g_socket_sbuffer_size;
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
    if (CreateIoCompletionPort((HANDLE)accept_state->sock, set, (ULONG_PTR)accept_state, g_num_cp_threads) == NULL)
    {
	mpi_errno = GetLastError();
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**iocp", "**iocp %s %d", get_error_string(mpi_errno), mpi_errno);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_ACCEPT);
	return mpi_errno;
    }

    accept_state->user_ptr = user_ptr;
    accept_state->set = set;
    *sock = accept_state;

    /*printf("accepted socket %d\n", accept_state->sock);fflush(stdout);*/
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_ACCEPT);
    return MPI_SUCCESS;
}

static unsigned int GetIP(char *pszIP)
{
    unsigned int nIP;
    unsigned int a,b,c,d;
    if (pszIP == NULL)
	return 0;
    sscanf(pszIP, "%u.%u.%u.%u", &a, &b, &c, &d);
    /*printf("mask: %u.%u.%u.%u\n", a, b, c, d);fflush(stdout);*/
    nIP = (d << 24) | (c << 16) | (b << 8) | a;
    return nIP;
}

static unsigned int GetMask(char *pszMask)
{
    int i, nBits;
    unsigned int nMask = 0;
    unsigned int a,b,c,d;

    if (pszMask == NULL)
	return 0;

    if (strstr(pszMask, "."))
    {
	sscanf(pszMask, "%u.%u.%u.%u", &a, &b, &c, &d);
	/*printf("mask: %u.%u.%u.%u\n", a, b, c, d);fflush(stdout);*/
	nMask = (d << 24) | (c << 16) | (b << 8) | a;
    }
    else
    {
	nBits = atoi(pszMask);
	for (i=0; i<nBits; i++)
	{
	    nMask = nMask << 1;
	    nMask = nMask | 0x1;
	}
    }
    /*
    unsigned int a, b, c, d;
    a = ((unsigned char *)(&nMask))[0];
    b = ((unsigned char *)(&nMask))[1];
    c = ((unsigned char *)(&nMask))[2];
    d = ((unsigned char *)(&nMask))[3];
    printf("mask: %u.%u.%u.%u\n", a, b, c, d);fflush(stdout);
    */
    return nMask;
}

#undef FUNCNAME
#define FUNCNAME MPIDU_Sock_post_connect
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDU_Sock_post_connect(MPIDU_Sock_set_t set, void * user_ptr, char * host_description, int port, MPIDU_Sock_t * sock)
{
    int mpi_errno;
    int str_errno = MPIU_STR_SUCCESS;
    struct hostent *lphost;
    struct sockaddr_in sockAddr;
    sock_state_t *connect_state;
    u_long optval;
    char host[100];
    int i;
    int connected = 0;
    int connect_errno = MPI_SUCCESS;
    char pszNetMask[50];
    char *pEnv, *token;
    unsigned int nNicNet=0, nNicMask=0;
    int use_subnet = 0;
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_SOCK_POST_CONNECT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_SOCK_POST_CONNECT);
    if (!g_init_called)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_INIT, "**sock_init", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_POST_CONNECT);
	return mpi_errno;
    }
    if (strlen(host_description) > SOCKI_DESCRIPTION_LENGTH)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_NOMEM, "**nomem", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_POST_CONNECT);
	return mpi_errno;
    }

    memset(&sockAddr,0,sizeof(sockAddr));

    /* setup the structures */
    connect_state = (sock_state_t*)MPIU_Malloc(sizeof(sock_state_t));
    init_state_struct(connect_state);
    connect_state->cur_host = connect_state->host_description;
    MPIU_Strncpy(connect_state->host_description, host_description, SOCKI_DESCRIPTION_LENGTH);

    /* create a socket */
    mpi_errno = easy_create(&connect_state->sock, ADDR_ANY, INADDR_ANY);
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_INIT, "**sock_create", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_POST_CONNECT);
	return mpi_errno;
    }

    /* check to see if a subnet was specified through the environment */
    pEnv = getenv("MPICH_NETMASK");
    if (pEnv != NULL)
    {
	MPIU_Strncpy(pszNetMask, pEnv, 50);
	token = strtok(pszNetMask, "/");
	if (token != NULL)
	{
	    token = strtok(NULL, "\n");
	    if (token != NULL)
	    {
		nNicNet = GetIP(pszNetMask);
		nNicMask = GetMask(token);
		use_subnet = 1;
	    }
	}
    }

    while (!connected)
    {
	host[0] = '\0';
	str_errno = MPIU_Str_get_string(&connect_state->cur_host, host, 100);
	/*printf("got <%s> out of <%s>\n", host, connect_state->host_description);fflush(stdout);*/
	if (str_errno != MPIU_STR_SUCCESS)
	{
	    if (str_errno == MPIU_STR_NOMEM)
		mpi_errno = MPIR_Err_create_code(connect_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_NOMEM, "**nomem", 0);
	    else
		mpi_errno = MPIR_Err_create_code(connect_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**fail", "**fail %d", mpi_errno);
	    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_POST_CONNECT);
	    return mpi_errno;
	}
	if (host[0] == '\0')
	{
	    mpi_errno = MPIR_Err_create_code(connect_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**sock_connect", "**sock_connect %s %d %s %d", connect_state->host_description, port, "exhausted all endpoints", -1);
	    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_POST_CONNECT);
	    return mpi_errno;
	}

	sockAddr.sin_family = AF_INET;
	sockAddr.sin_addr.s_addr = inet_addr(host);

	if (sockAddr.sin_addr.s_addr == INADDR_NONE || sockAddr.sin_addr.s_addr == 0)
	{
	    lphost = gethostbyname(host);
	    if (lphost != NULL)
		sockAddr.sin_addr.s_addr = ((struct in_addr *)lphost->h_addr)->s_addr;
	    else
	    {
		mpi_errno = WSAGetLastError();
		connect_errno = MPIR_Err_create_code(connect_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**gethostbyname", "**gethostbyname %s %d", get_error_string(mpi_errno), mpi_errno);
		/*
		MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_POST_CONNECT);
		return mpi_errno;
		*/
		continue;
	    }
	}

	/* if a subnet was specified, make sure the currently extracted ip falls in the subnet */
	if (use_subnet)
	{
	    if ((sockAddr.sin_addr.s_addr & nNicMask) != nNicNet)
	    {
		/* this ip does not match, move to the next */
		continue;
	    }
	}

	sockAddr.sin_port = htons((u_short)port);

	/* connect */
	for (i=0; i<5; i++)
	{
	    /*printf("connecting to %s\n", host);fflush(stdout);*/
	    if (connect(connect_state->sock, (SOCKADDR*)&sockAddr, sizeof(sockAddr)) == SOCKET_ERROR)
	    {
		int random_time;
		int error = WSAGetLastError();
		if ((error != WSAECONNREFUSED && error != WSAETIMEDOUT) || i == 4)
		{
		    connect_errno = MPIR_Err_create_code(connect_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_INIT, "**sock_connect", "**sock_connect %s %d %s %d", host, port, get_error_string(error), error);
		    /*
		    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_POST_CONNECT);
		    return mpi_errno;
		    */
		    /* This code assumes that all errors other than WSAECONNREFUSED and WSAETIMEDOUT should not cause a connection retry */
		    /* FIXME: Is this correct for resource errors like WSAENOBUFS or an interrupted operation? */
		    /*        Should all errors cause a retry? or just WSAECONNREFUSED? or a subset of the possible errors? */
		    /*        The reason for not retrying on all errors is that it slows down connection time for multi-nic
		    /*        hosts that cannot be contacted on the first address listed. */
		    break;
		}
		/* Close the socket with an error and create a new one */
		if (closesocket(connect_state->sock) == SOCKET_ERROR)
		{
		    error = WSAGetLastError();
		    connect_errno = MPIR_Err_create_code(connect_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_INIT, "**sock_connect", "**sock_connect %s %d %s %d", host, port, get_error_string(error), error);
		    /*
		    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_POST_CONNECT);
		    return mpi_errno;
		    */
		    break;
		}
		connect_state->sock = INVALID_SOCKET;
		mpi_errno = easy_create(&connect_state->sock, ADDR_ANY, INADDR_ANY);
		if (mpi_errno != MPI_SUCCESS)
		{
		    /* Warning: Loss of information.  We have two error stacks, one in connect_errno and the other in mpi_errno, that cannot be joined given the current error code interface. */
		    /*connect_errno = MPIR_Err_create_code(connect_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_INIT, "**sock_create", 0);*/
		    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_INIT, "**sock_create", 0);
		    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_POST_CONNECT);
		    return mpi_errno;
		}
		random_time = (int)((double)rand() / (double)RAND_MAX * 250.0);
		Sleep(random_time);
	    }
	    else
	    {
		/*printf("connect to %s:%d succeeded.\n", host, port);fflush(stdout);*/
		connected = 1;
		break;
	    }
	}
    }

    /* set the socket to non-blocking */
    optval = TRUE;
    ioctlsocket(connect_state->sock, FIONBIO, &optval);

    connect_state->user_ptr = user_ptr;
    connect_state->type = SOCKI_SOCKET;
    connect_state->state = SOCKI_CONNECTING;
    connect_state->set = set;

    /* associate the socket with the completion port */
    if (CreateIoCompletionPort((HANDLE)connect_state->sock, set, (ULONG_PTR)connect_state, g_num_cp_threads) == NULL)
    {
	mpi_errno = GetLastError();
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_INIT, "**iocp", "**iocp %s %d", get_error_string(mpi_errno), mpi_errno);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_POST_CONNECT);
	return mpi_errno;
    }

    connect_state->pending_operations++;

    /* post a completion event so the sock_post_connect can be notified through sock_wait */
    PostQueuedCompletionStatus(set, 0, (ULONG_PTR)connect_state, &connect_state->write.ovl);

    *sock = connect_state;

    /*printf("connected socket %d\n", connect_state->sock);fflush(stdout);*/
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_POST_CONNECT);
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPIDU_Sock_set_user_ptr
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDU_Sock_set_user_ptr(MPIDU_Sock_t sock, void * user_ptr)
{
    int mpi_errno;
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_SOCK_SET_USER_PTR);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_SOCK_SET_USER_PTR);
    if (!g_init_called)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_INIT, "**sock_init", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_SET_USER_PTR);
	return mpi_errno;
    }
    if (sock == NULL)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_BAD_SOCK, "**bad_sock", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_SET_USER_PTR);
	return mpi_errno;
    }
    sock->user_ptr = user_ptr;
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_SET_USER_PTR);
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPIDU_Sock_post_close
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDU_Sock_post_close(MPIDU_Sock_t sock)
{
    int mpi_errno;
    SOCKET s, *sp;
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_SOCK_POST_CLOSE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_SOCK_POST_CLOSE);

    if (!g_init_called)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_INIT, "**sock_init", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_POST_CLOSE);
	return mpi_errno;
    }

    if (sock == NULL)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_BAD_SOCK, "**nullptr", "**nullptr %s", "sock");
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_POST_CLOSE);
	return mpi_errno;
    }
    if (sock->closing)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**pctwice", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_POST_CLOSE);
	return mpi_errno;
    }

    if (sock->type == SOCKI_LISTENER)
    {
	s = sock->listen_sock;
	sp = &sock->listen_sock;
    }
    else
    {
	s = sock->sock;
	sp = &sock->sock;
    }
    if (s == INVALID_SOCKET)
    {
	if (sock->type == SOCKI_LISTENER)
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_BAD_SOCK, "**bad_listenersock", 0);
	else
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_BAD_SOCK, "**bad_sock", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_POST_CLOSE);
	return mpi_errno;
    }

    if (sock->pending_operations != 0)
    {
	/*MPIU_Assert(sock->state != 0);*/ /* The state can be 0 if the operation was aborted */
#ifdef MPICH_DBG_OUTPUT
	if (sock->state & SOCKI_CONNECTING)
	    MPIU_DBG_PRINTF(("sock_post_close(%d) called while sock is connecting.\n", MPIDU_Sock_get_sock_id(sock)));
	if (sock->state & SOCKI_READING)
	{
	    int i, n = 0;
	    for (i=0; i<sock->read.iovlen; i++)
		n += sock->read.iov[i].MPL_IOV_LEN;
	    MPIU_DBG_PRINTF(("sock_post_close(%d) called while sock is reading: %d bytes out of %d, index %d, iovlen %d.\n",
		MPIDU_Sock_get_sock_id(sock), sock->read.total, n, sock->read.index, sock->read.iovlen));
	}
	if (sock->state & SOCKI_WRITING)
	{
	    int i, n = 0;
	    for (i=0; i<sock->write.iovlen; i++)
		n += sock->write.iov[i].MPL_IOV_LEN;
	    MPIU_DBG_PRINTF(("sock_post_close(%d) called while sock is writing: %d bytes out of %d, index %d, iovlen %d.\n",
		MPIDU_Sock_get_sock_id(sock), sock->write.total, n, sock->write.index, sock->write.iovlen));
	}
	fflush(stdout);
#endif
	/*
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_POST_CLOSE);
	return SOCK_ERR_OP_IN_PROGRESS;
	*/
	/* posting a close cancels all outstanding operations */
	/* It would be nice to cancel the outstanding reads or writes and then close the socket after handling the cancelled operations */
	/* But it cannot be done because CancelIo only cancels operations started by the current thread.  There is no way to cancel all operations. */
	/*CancelIo(sock->sock);*/
    }

    sock->closing = TRUE;
    /*sock->ct1 = PMPI_Wtime();*/
    if (sock->type != SOCKI_LISTENER) /* calling shutdown for a listening socket is not valid */
    {
	/* Mark the socket as non-writable */
	if (shutdown(s, SD_SEND) == SOCKET_ERROR)
	{
	    sock->pending_operations = 0;
	    /*printf("closing socket %d\n", s);fflush(stdout);*/
	    if (closesocket(s) == SOCKET_ERROR)
	    {
		mpi_errno = WSAGetLastError();
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**closesocket", "**closesocket %s %d", get_error_string(mpi_errno), mpi_errno);
		MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_POST_CLOSE);
		return mpi_errno;
	    }
	    *sp = INVALID_SOCKET;
	    if (!PostQueuedCompletionStatus(sock->set, 0, (ULONG_PTR)sock, NULL))
	    {
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_BAD_SOCK, "**fail", "**fail %d", GetLastError());
		MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_POST_CLOSE);
		return mpi_errno;
	    }
	    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_POST_CLOSE);
	    return MPI_SUCCESS;
	}
    }
    /* Cancel any outstanding operations */
    sock->pending_operations = 0;

    if (sock->type == SOCKI_SOCKET)
    {
	static char ch;
	mpi_errno = MPI_SUCCESS;
	if (sock->state ^ SOCKI_READING)
	{
	    /* If a read is not already posted, post a bogus one here. */
	    mpi_errno = MPIDU_Sock_post_read(sock, &ch, 1, 1, NULL);
	    /* ignore this posted read so wait will return an op_close */
	    sock->pending_operations = 0;
	}
	if (mpi_errno == MPI_SUCCESS)
	{
	    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_POST_CLOSE);
	    return MPI_SUCCESS;
	}
    }

    if (sock->type != SOCKI_LISTENER) /* calling shutdown for a listening socket is not valid */
    {
	/* Mark the socket as non-readable */
	if (shutdown(s, SD_RECEIVE) == SOCKET_ERROR)
	{
	    mpi_errno = WSAGetLastError();
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**shutdown", "**shutdown %s %d", get_error_string(mpi_errno), mpi_errno);
	    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_POST_CLOSE);
	    return mpi_errno;
	}
    }
    /* Close the socket and insert a completion status so wait will return an op_close */
    /*printf("closing socket %d\n", s);fflush(stdout);*/
    if (closesocket(s) == SOCKET_ERROR)
    {
	mpi_errno = WSAGetLastError();
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**closesocket", "**closesocket %s %d", get_error_string(mpi_errno), mpi_errno);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_POST_CLOSE);
	return mpi_errno;

    }
    *sp = INVALID_SOCKET;
    if (!PostQueuedCompletionStatus(sock->set, 0, (ULONG_PTR)sock, NULL))
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_BAD_SOCK, "**fail", "**fail %d", GetLastError());
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_POST_CLOSE);
	return mpi_errno;
    }
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_POST_CLOSE);
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPIDU_Sock_post_read
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDU_Sock_post_read(MPIDU_Sock_t sock, void * buf, MPIU_Size_t minbr, MPIU_Size_t maxbr,
                         MPIDU_Sock_progress_update_func_t fn)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_SOCK_POST_READ);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_SOCK_POST_READ);
    MPIU_UNREFERENCED_ARG(maxbr);
    sock->read.tiov.MPL_IOV_BUF = (MPL_IOV_BUF_CAST)buf;
    sock->read.tiov.MPL_IOV_LEN = minbr;
    mpi_errno = MPIDU_Sock_post_readv(sock, &sock->read.tiov, 1, fn);
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_POST_READ);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDU_Sock_post_readv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDU_Sock_post_readv(MPIDU_Sock_t sock, MPL_IOV * iov, int iov_n, MPIDU_Sock_progress_update_func_t fn)
{
    int iter;
    int mpi_errno = MPI_SUCCESS;
    int result;
#ifdef MPICH_DBG_OUTPUT
    int i;
#endif
    DWORD flags = 0;
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_SOCK_POST_READV);
#ifdef USE_SOCK_IOV_COPY
    MPIDI_STATE_DECL(MPID_STATE_MEMCPY);
#endif

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_SOCK_POST_READV);
    if (!g_init_called)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_INIT, "**sock_init", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_POST_READV);
	return mpi_errno;
    }
    /*sock->rt1 = PMPI_Wtime();*/
    /* strip any trailing empty buffers */
    while (iov_n && iov[iov_n-1].MPL_IOV_LEN == 0)
	iov_n--;
    sock->read.total = 0;
#ifdef USE_SOCK_IOV_COPY
    MPIDI_FUNC_ENTER(MPID_STATE_MEMCPY);
    memcpy(sock->read.iov, iov, sizeof(MPL_IOV) * n);
    MPIDI_FUNC_EXIT(MPID_STATE_MEMCPY);
#else
    sock->read.iov = iov;
#endif
    sock->read.iovlen = iov_n;
    sock->read.index = 0;
    sock->read.progress_update = fn;
    sock->state |= SOCKI_READING;
#ifdef MPICH_DBG_OUTPUT
    for (i=0; i<iov_n; i++)
    {
	MPIU_DBG_PRINTF(("sock_post_readv - iov[%d].len = %d\n", i, iov[i].MPL_IOV_LEN));
    }
#endif
    for (iter=0; iter<10; iter++)
    {
	if ((result = WSARecv(sock->sock, sock->read.iov, iov_n, &sock->read.num_bytes, &flags, &sock->read.ovl, NULL)) != SOCKET_ERROR)
	{
	    break;
	}

	mpi_errno = WSAGetLastError();
	if (mpi_errno == WSA_IO_PENDING)
	{
	    mpi_errno = MPI_SUCCESS;
	    break;
	}
	if (mpi_errno == WSAENOBUFS)
	{
	    WSABUF tmp;
	    tmp.buf = sock->read.iov[0].buf;
	    tmp.len = sock->read.iov[0].len;
	    MPIU_Assert(tmp.len > 0);
	    while (mpi_errno == WSAENOBUFS)
	    {
		/*printf("[%d] receiving %d bytes\n", __LINE__, tmp.len);fflush(stdout);*/
		if ((result = WSARecv(sock->sock, &tmp, 1, &sock->read.num_bytes, &flags, &sock->read.ovl, NULL)) != SOCKET_ERROR)
		{
		    mpi_errno = MPI_SUCCESS;
		    break;
		}
		mpi_errno = WSAGetLastError();
		if (mpi_errno == WSA_IO_PENDING)
		{
		    mpi_errno = MPI_SUCCESS;
		    break;
		}
		/*printf("[%d] reducing recv length from %d to %d\n", __LINE__, tmp.len, tmp.len / 2);fflush(stdout);*/
		tmp.len = tmp.len / 2;
		if (tmp.len == 0 && mpi_errno == WSAENOBUFS)
		{
		    break;
		}
	    }
	    if (mpi_errno == MPI_SUCCESS)
	    {
		break;
	    }
	}
	if (mpi_errno != WSAEWOULDBLOCK)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**fail", "**fail %s %d", get_error_string(mpi_errno), mpi_errno);
	    break;
	}
	Sleep(200);
    }
    if (mpi_errno == MPI_SUCCESS)
	sock->pending_operations++;
    else
	sock->state &= ~SOCKI_READING;
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_POST_READV);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDU_Sock_post_write
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDU_Sock_post_write(MPIDU_Sock_t sock, void * buf, MPIU_Size_t min, MPIU_Size_t max, MPIDU_Sock_progress_update_func_t fn)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_SOCK_POST_WRITE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_SOCK_POST_WRITE);
    MPIU_UNREFERENCED_ARG(max);
    sock->write.tiov.MPL_IOV_BUF = (MPL_IOV_BUF_CAST)buf;
    sock->write.tiov.MPL_IOV_LEN = min;
    mpi_errno = MPIDU_Sock_post_writev(sock, &sock->write.tiov, 1, fn);
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_POST_WRITE);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDU_Sock_post_writev
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDU_Sock_post_writev(MPIDU_Sock_t sock, MPL_IOV * iov, int iov_n, MPIDU_Sock_progress_update_func_t fn)
{
    int mpi_errno = MPI_SUCCESS;
    int iter;
#ifdef MPICH_DBG_OUTPUT
    int i;
#endif
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_SOCK_POST_WRITEV);
#ifdef USE_SOCK_IOV_COPY
    MPIDI_STATE_DECL(MPID_STATE_MEMCPY);
#endif

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_SOCK_POST_WRITEV);
    /*sock->wt1 = PMPI_Wtime();*/
    sock->write.total = 0;
#ifdef USE_SOCK_IOV_COPY
    MPIDI_FUNC_ENTER(MPID_STATE_MEMCPY);
    memcpy(sock->write.iov, iov, sizeof(MPL_IOV) * iov_n);
    MPIDI_FUNC_EXIT(MPID_STATE_MEMCPY);
#else
    sock->write.iov = iov;
#endif
    sock->write.iovlen = iov_n;
    sock->write.index = 0;
    sock->write.progress_update = fn;
    sock->state |= SOCKI_WRITING;
#ifdef MPICH_DBG_OUTPUT
    for (i=0; i<iov_n; i++)
    {
	MPIU_DBG_PRINTF(("sock_post_writev - iov[%d].len = %d\n", i, iov[i].MPL_IOV_LEN));
    }
#endif
    for (iter=0; iter<10; iter++)
    {
	if (WSASend(sock->sock, sock->write.iov, iov_n, &sock->write.num_bytes, 0, &sock->write.ovl, NULL) != SOCKET_ERROR)
	    break;

	mpi_errno = WSAGetLastError();
	if (mpi_errno == WSA_IO_PENDING)
	{
	    mpi_errno = MPI_SUCCESS;
	    break;
	}
	if (mpi_errno == WSAENOBUFS)
	{
	    WSABUF tmp;
	    tmp.buf = sock->write.iov[0].buf;
	    tmp.len = sock->write.iov[0].len;
	    while (mpi_errno == WSAENOBUFS)
	    {
		/*printf("[%d] sending %d bytes\n", __LINE__, tmp.len);fflush(stdout);*/
		if (WSASend(sock->sock, &tmp, 1, &sock->write.num_bytes, 0, &sock->write.ovl, NULL) != SOCKET_ERROR)
		{
		    mpi_errno = MPI_SUCCESS;
		    break;
		}
		mpi_errno = WSAGetLastError();
		if (mpi_errno == WSA_IO_PENDING)
		{
		    mpi_errno = MPI_SUCCESS;
		    break;
		}
		/*printf("[%d] reducing send length from %d to %d\n", __LINE__, tmp.len, tmp.len / 2);fflush(stdout);*/
		tmp.len = tmp.len / 2;
		if (tmp.len == 0)
		{
		    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**fail", 0);
		    break;
		}
	    }
	    if (mpi_errno == MPI_SUCCESS)
	    {
		break;
	    }
	}
	if (mpi_errno != WSAEWOULDBLOCK)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**fail", "**fail %s %d", get_error_string(mpi_errno), mpi_errno);
	    break;
	}
	Sleep(200);
    }
    if (mpi_errno == MPI_SUCCESS)
	sock->pending_operations++;
    else
	sock->state &= ~SOCKI_WRITING;
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_POST_WRITEV);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDU_Sock_wait
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDU_Sock_wait(MPIDU_Sock_set_t set, int timeout, MPIDU_Sock_event_t * out)
{
    /*double t1, t2;*/
    int mpi_errno;
    DWORD num_bytes;
    sock_state_t *sock, *iter;
    OVERLAPPED *ovl;
    DWORD dwFlags = 0;
    char error_msg[1024];
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_SOCK_WAIT);
    MPIDI_STATE_DECL(MPID_STATE_GETQUEUEDCOMPLETIONSTATUS);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_SOCK_WAIT);

    for (;;) 
    {
#if defined(MPICH_IS_THREADED)
#       if (MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY_GLOBAL)
	{
            int err;
	    /* Release the lock so that other threads may make progress while this thread waits for something to do */
	    MPIU_DBG_MSG(THREAD,TYPICAL,"Exit global critical section");
	    MPIU_THREAD_CHECK_BEGIN;
	    MPID_Thread_mutex_unlock(&MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX, &err);
	    MPIU_THREAD_CHECK_END;
	}
#       else
#           error selected multi-threaded implementation is not supported
#       endif
#endif
	MPIDI_FUNC_ENTER(MPID_STATE_GETQUEUEDCOMPLETIONSTATUS);
	/* initialize to NULL so we can compare the output of GetQueuedCompletionStatus */
	sock = NULL;
	ovl = NULL;
	num_bytes = 0;
	/*t1 = PMPI_Wtime();*/
	if (GetQueuedCompletionStatus(set, &num_bytes, (PULONG_PTR)&sock, &ovl, timeout))
	{
	    /*t2 = PMPI_Wtime();*/
	    /*printf("[%d] GetQueuedCompletionStatus took %.3f seconds for sock: %d\n", getpid(), t2-t1, sock->sock);*/
	    MPIDI_FUNC_EXIT(MPID_STATE_GETQUEUEDCOMPLETIONSTATUS);
#if defined(MPICH_IS_THREADED)
#           if (MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY_GLOBAL)
	    {
                int err;
		/* Reaquire the lock before processing any of the information returned from GetQueuedCompletionStatus */
		MPIU_DBG_MSG(THREAD,TYPICAL,"Enter global critical section");
		MPIU_THREAD_CHECK_BEGIN;
		MPID_Thread_mutex_lock(&MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX, &err);
		MPIU_THREAD_CHECK_END;
	    }
#           else
#               error selected multi-threaded implementation is not supported
#           endif
#endif
	    if (sock->type == SOCKI_SOCKET)
	    {
		if (sock->closing && sock->pending_operations == 0)
		{
		    /*sock->ct2 = PMPI_Wtime();*/
		    /*printf("[%d] time from post_close to op_close: %.3f - sock %d\n", getpid(), sock->ct2 - sock->ct1, sock->sock);*/
		    /*printf("<1>");fflush(stdout);*/
		    out->op_type = MPIDU_SOCK_OP_CLOSE;
		    out->num_bytes = 0;
		    out->error = MPI_SUCCESS;
		    out->user_ptr = sock->user_ptr;
		    CloseHandle(sock->read.ovl.hEvent);
		    CloseHandle(sock->write.ovl.hEvent);
		    sock->read.ovl.hEvent = NULL;
		    sock->write.ovl.hEvent = NULL;
		    if (sock->sock != INVALID_SOCKET)
		    {
			/*printf("closing socket %d\n", sock->sock);fflush(stdout);*/
			if (closesocket(sock->sock) == SOCKET_ERROR)
			{
			    mpi_errno = WSAGetLastError();
			    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**closesocket", "**closesocket %s %d", get_error_string(mpi_errno), mpi_errno);
			    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_WAIT);
			    return mpi_errno;
			}
			sock->sock = INVALID_SOCKET;
		    }
		    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_WAIT);
		    return MPI_SUCCESS;
		}
		if (ovl == &sock->read.ovl)
		{
		    if (num_bytes == 0)
		    {
			/*sock->rt2 = PMPI_Wtime();*/
			/*printf("[%d] time from post_read  to op_read : %.3f - sock %d\n", getpid(), sock->rt2 - sock->rt1, sock->sock);*/
			/* socket closed */
			MPIU_DBG_PRINTF(("sock_wait readv returning %d bytes and EOF\n", sock->read.total));
			/*printf("sock_wait readv returning %d bytes and EOF\n", sock->read.total);*/
			out->op_type = MPIDU_SOCK_OP_READ;
			out->num_bytes = sock->read.total;
			out->error = MPIDU_SOCK_ERR_CONN_CLOSED;
			out->user_ptr = sock->user_ptr;
			sock->pending_operations--;
			sock->state &= ~SOCKI_READING; /* remove the SOCKI_READING bit */
			if (sock->closing && sock->pending_operations == 0)
			{
			    MPIU_DBG_PRINTF(("sock_wait: closing socket(%d) after iov read completed.\n", MPIDU_Sock_get_sock_id(sock)));
			    FlushFileBuffers((HANDLE)sock->sock);
			    if (shutdown(sock->sock, SD_BOTH) == SOCKET_ERROR)
			    {
				mpi_errno = WSAGetLastError();
				mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**shutdown", "**shutdown %s %d", get_error_string(mpi_errno), mpi_errno);
				MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_WAIT);
				return mpi_errno;
			    }
			    /*printf("closing socket %d\n", sock->sock);fflush(stdout);*/
			    if (closesocket(sock->sock) == SOCKET_ERROR)
			    {
				mpi_errno = WSAGetLastError();
				mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**closesocket", "**closesocket %s %d", get_error_string(mpi_errno), mpi_errno);
				MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_WAIT);
				return mpi_errno;
			    }
			    sock->sock = INVALID_SOCKET;
			}
			MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_WAIT);
			return MPI_SUCCESS;
		    }
		    MPIU_DBG_PRINTF(("sock_wait readv update: %d bytes\n", num_bytes));
		    sock->read.total += num_bytes;
		    while (num_bytes)
		    {
			if (sock->read.iov[sock->read.index].MPL_IOV_LEN <= num_bytes)
			{
			    num_bytes -= sock->read.iov[sock->read.index].MPL_IOV_LEN;
			    sock->read.index++;
			    sock->read.iovlen--;
			}
			else
			{
			    sock->read.iov[sock->read.index].MPL_IOV_LEN -= num_bytes;
			    sock->read.iov[sock->read.index].MPL_IOV_BUF = (MPL_IOV_BUF_CAST)(
				(char*)(sock->read.iov[sock->read.index].MPL_IOV_BUF) + num_bytes);
			    num_bytes = 0;
			}
		    }
		    if (sock->read.iovlen == 0)
		    {
			/*sock->rt2 = PMPI_Wtime();*/
			/*printf("[%d] time from post_read  to op_read : %.3f - sock %d\n", getpid(), sock->rt2 - sock->rt1, sock->sock);*/
			MPIU_DBG_PRINTF(("sock_wait readv %d bytes\n", sock->read.total));
			out->op_type = MPIDU_SOCK_OP_READ;
			out->num_bytes = sock->read.total;
			out->error = MPI_SUCCESS;
			out->user_ptr = sock->user_ptr;
			sock->pending_operations--;
			sock->state &= ~SOCKI_READING; /* remove the SOCKI_READING bit */
			if (sock->closing && sock->pending_operations == 0)
			{
			    MPIU_DBG_PRINTF(("sock_wait: closing socket(%d) after iov read completed.\n", MPIDU_Sock_get_sock_id(sock)));
			    FlushFileBuffers((HANDLE)sock->sock);
			    if (shutdown(sock->sock, SD_BOTH) == SOCKET_ERROR)
			    {
				mpi_errno = WSAGetLastError();
				mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**shutdown", "**shutdown %s %d", get_error_string(mpi_errno), mpi_errno);
				MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_WAIT);
				return mpi_errno;
			    }
			    /*printf("closing socket %d\n", sock->sock);fflush(stdout);*/
			    if (closesocket(sock->sock) == SOCKET_ERROR)
			    {
				mpi_errno = WSAGetLastError();
				mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**closesocket", "**closesocket %s %d", get_error_string(mpi_errno), mpi_errno);
				MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_WAIT);
				return mpi_errno;
			    }
			    sock->sock = INVALID_SOCKET;
			}
			MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_WAIT);
			return MPI_SUCCESS;
		    }
		    /* make the user upcall */
		    if (sock->read.progress_update != NULL)
			sock->read.progress_update(num_bytes, sock->user_ptr);
		    /* post a read of the remaining data */
		    /*WSARecv(sock->sock, sock->read.iov, sock->read.iovlen, &sock->read.num_bytes, &dwFlags, &sock->read.ovl, NULL);*/
		    if (WSARecv(sock->sock, &sock->read.iov[sock->read.index], sock->read.iovlen, &sock->read.num_bytes, &dwFlags, &sock->read.ovl, NULL) == SOCKET_ERROR)
		    {
			mpi_errno = WSAGetLastError();
			if (mpi_errno == 0)
			{
			    /*sock->rt2 = PMPI_Wtime();*/
			    /*printf("[%d] time from post_read  to op_read : %.3f - sock %d\n", getpid(), sock->rt2 - sock->rt1, sock->sock);*/
			    out->op_type = MPIDU_SOCK_OP_READ;
			    out->num_bytes = sock->read.total;
			    /*printf("sock_wait returning %d bytes and socket closed\n", sock->read.total);*/
			    out->error = MPIDU_SOCK_ERR_CONN_CLOSED;
			    out->user_ptr = sock->user_ptr;
			    sock->pending_operations--;
			    sock->state &= ~SOCKI_READING;
			    if (sock->closing && sock->pending_operations == 0)
			    {
				MPIU_DBG_PRINTF(("sock_wait: closing socket(%d) after iov read completed.\n", MPIDU_Sock_get_sock_id(sock)));
				FlushFileBuffers((HANDLE)sock->sock);
				if (shutdown(sock->sock, SD_BOTH) == SOCKET_ERROR)
				{
				    mpi_errno = WSAGetLastError();
				    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**shutdown", "**shutdown %s %d", get_error_string(mpi_errno), mpi_errno);
				    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_WAIT);
				    return mpi_errno;
				}
				/*printf("closing socket %d\n", sock->sock);fflush(stdout);*/
				if (closesocket(sock->sock) == SOCKET_ERROR)
				{
				    mpi_errno = WSAGetLastError();
				    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**closesocket", "**closesocket %s %d", get_error_string(mpi_errno), mpi_errno);
				    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_WAIT);
				    return mpi_errno;
				}
				sock->sock = INVALID_SOCKET;
			    }
			    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_WAIT);
			    return MPI_SUCCESS;
			}
			if (mpi_errno == WSAENOBUFS)
			{
			    WSABUF tmp;
			    tmp.buf = sock->read.iov[sock->read.index].buf;
			    tmp.len = sock->read.iov[sock->read.index].len;
			    while (mpi_errno == WSAENOBUFS)
			    {
				/*printf("[%d] receiving %d bytes\n", __LINE__, tmp.len);fflush(stdout);*/
				if (WSARecv(sock->sock, &tmp, 1, &sock->read.num_bytes, &dwFlags, &sock->read.ovl, NULL) != SOCKET_ERROR)
				{
				    mpi_errno = WSA_IO_PENDING;
				    break;
				}
				mpi_errno = WSAGetLastError();
				if (mpi_errno == WSA_IO_PENDING)
				{
				    break;
				}
				/*printf("[%d] reducing recv length from %d to %d\n", __LINE__, tmp.len, tmp.len / 2);fflush(stdout);*/
				tmp.len = tmp.len / 2;
				if (tmp.len == 0 && mpi_errno == WSAENOBUFS)
				{
				    break;
				}
			    }
			}
			if (mpi_errno != WSA_IO_PENDING)
			{
			    /*sock->rt2 = PMPI_Wtime();*/
			    /*printf("[%d] time from post_read  to op_read : %.3f - sock %d\n", getpid(), sock->rt2 - sock->rt1, sock->sock);*/
			    out->op_type = MPIDU_SOCK_OP_READ;
			    out->num_bytes = sock->read.total;
			    out->error = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**fail", "**fail %s %d", get_error_string(mpi_errno), mpi_errno);
			    out->user_ptr = sock->user_ptr;
			    sock->pending_operations--;
			    sock->state &= ~SOCKI_READING;
			    if (sock->closing && sock->pending_operations == 0)
			    {
				MPIU_DBG_PRINTF(("sock_wait: closing socket(%d) after iov read completed.\n", MPIDU_Sock_get_sock_id(sock)));
				FlushFileBuffers((HANDLE)sock->sock);
				if (shutdown(sock->sock, SD_BOTH) == SOCKET_ERROR)
				{
				    mpi_errno = WSAGetLastError();
				    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**shutdown", "**shutdown %s %d", get_error_string(mpi_errno), mpi_errno);
				    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_WAIT);
				    return mpi_errno;
				}
				/*printf("closing socket %d\n", sock->sock);fflush(stdout);*/
				if (closesocket(sock->sock) == SOCKET_ERROR)
				{
				    mpi_errno = WSAGetLastError();
				    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**closesocket", "**closesocket %s %d", get_error_string(mpi_errno), mpi_errno);
				    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_WAIT);
				    return mpi_errno;
				}
				sock->sock = INVALID_SOCKET;
			    }
			    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_WAIT);
			    return MPI_SUCCESS;
			}
		    }
		}
		else if (ovl == &sock->write.ovl)
		{
		    if (sock->state & SOCKI_CONNECTING)
		    {
			/* insert code here to determine that the connect succeeded */
			/* ... */
			out->op_type = MPIDU_SOCK_OP_CONNECT;
			out->num_bytes = 0;
			out->error = MPI_SUCCESS;
			out->user_ptr = sock->user_ptr;
			sock->pending_operations--;
			sock->state ^= SOCKI_CONNECTING; /* remove the SOCKI_CONNECTING bit */
			if (sock->closing && sock->pending_operations == 0)
			{
			    MPIU_DBG_PRINTF(("sock_wait: closing socket(%d) after connect completed.\n", MPIDU_Sock_get_sock_id(sock)));
			    FlushFileBuffers((HANDLE)sock->sock);
			    if (shutdown(sock->sock, SD_BOTH) == SOCKET_ERROR)
			    {
				mpi_errno = WSAGetLastError();
				mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**shutdown", "**shutdown %s %d", get_error_string(mpi_errno), mpi_errno);
				MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_WAIT);
				return mpi_errno;
			    }
			    /*printf("closing socket %d\n", sock->sock);fflush(stdout);*/
			    if (closesocket(sock->sock) == SOCKET_ERROR)
			    {
				mpi_errno = WSAGetLastError();
				mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**closesocket", "**closesocket %s %d", get_error_string(mpi_errno), mpi_errno);
				MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_WAIT);
				return mpi_errno;
			    }
			    sock->sock = INVALID_SOCKET;
			}
			MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_WAIT);
			return MPI_SUCCESS;
		    }
		    else
		    {
			if (num_bytes == 0)
			{
			    /*sock->wt2 = PMPI_Wtime();*/
			    /*printf("[%d] time from post_write to op_write: %.3f - sock %d\n", getpid(), sock->wt2 - sock->wt1, sock->sock);*/
			    /* socket closed */
			    MPIU_DBG_PRINTF(("sock_wait writev returning %d bytes and EOF\n", sock->write.total));
			    out->op_type = MPIDU_SOCK_OP_WRITE;
			    out->num_bytes = sock->write.total;
			    /*printf("sock_wait writev returning %d bytes and EOF\n", sock->write.total);*/
			    out->error = MPIDU_SOCK_ERR_CONN_CLOSED;
			    out->user_ptr = sock->user_ptr;
			    sock->pending_operations--;
			    sock->state &= ~SOCKI_WRITING; /* remove the SOCKI_WRITING bit */
			    if (sock->closing && sock->pending_operations == 0)
			    {
				MPIU_DBG_PRINTF(("sock_wait: closing socket(%d) after iov write completed.\n", MPIDU_Sock_get_sock_id(sock)));
				FlushFileBuffers((HANDLE)sock->sock);
				if (shutdown(sock->sock, SD_BOTH) == SOCKET_ERROR)
				{
				    mpi_errno = WSAGetLastError();
				    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**shutdown", "**shutdown %s %d", get_error_string(mpi_errno), mpi_errno);
				    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_WAIT);
				    return mpi_errno;
				}
				/*printf("closing socket %d\n", sock->sock);fflush(stdout);*/
				if (closesocket(sock->sock) == SOCKET_ERROR)
				{
				    mpi_errno = WSAGetLastError();
				    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**closesocket", "**closesocket %s %d", get_error_string(mpi_errno), mpi_errno);
				    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_WAIT);
				    return mpi_errno;
				}
				sock->sock = INVALID_SOCKET;
			    }
			    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_WAIT);
			    return MPI_SUCCESS;
			}
			MPIU_DBG_PRINTF(("sock_wait: write update, total = %d + %d = %d\n", sock->write.total, num_bytes, sock->write.total + num_bytes));
			sock->write.total += num_bytes;
			while (num_bytes)
			{
			    if (sock->write.iov[sock->write.index].MPL_IOV_LEN <= num_bytes)
			    {
				/*MPIU_DBG_PRINTF(("sock_wait: write.index %d, len %d\n", sock->write.index, 
				sock->write.iov[sock->write.index].MPL_IOV_LEN));*/
				num_bytes -= sock->write.iov[sock->write.index].MPL_IOV_LEN;
				sock->write.index++;
				sock->write.iovlen--;
			    }
			    else
			    {
				/*MPIU_DBG_PRINTF(("sock_wait: partial data written [%d].len = %d, num_bytes = %d\n", sock->write.index,
				sock->write.iov[sock->write.index].MPL_IOV_LEN, num_bytes));*/
				sock->write.iov[sock->write.index].MPL_IOV_LEN -= num_bytes;
				sock->write.iov[sock->write.index].MPL_IOV_BUF = (MPL_IOV_BUF_CAST)(
				    (char*)(sock->write.iov[sock->write.index].MPL_IOV_BUF) + num_bytes);
				num_bytes = 0;
			    }
			}
			if (sock->write.iovlen == 0)
			{
			    /*sock->wt2 = PMPI_Wtime();*/
			    /*printf("[%d] time from post_write to op_write: %.3f - sock %d\n", getpid(), sock->wt2 - sock->wt1, sock->sock);*/
			    if (sock->write.total > 0)
			    {
				MPIU_DBG_PRINTF(("sock_wait wrotev %d bytes\n", sock->write.total));
			    }
			    out->op_type = MPIDU_SOCK_OP_WRITE;
			    out->num_bytes = sock->write.total;
			    out->error = MPI_SUCCESS;
			    out->user_ptr = sock->user_ptr;
			    sock->pending_operations--;
			    sock->state &= ~SOCKI_WRITING; /* remove the SOCKI_WRITING bit */
			    if (sock->closing && sock->pending_operations == 0)
			    {
				MPIU_DBG_PRINTF(("sock_wait: closing socket(%d) after iov write completed.\n", MPIDU_Sock_get_sock_id(sock)));
				FlushFileBuffers((HANDLE)sock->sock);
				if (shutdown(sock->sock, SD_BOTH) == SOCKET_ERROR)
				{
				    mpi_errno = WSAGetLastError();
				    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**shutdown", "**shutdown %s %d", get_error_string(mpi_errno), mpi_errno);
				    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_WAIT);
				    return mpi_errno;
				}
				/*printf("closing socket %d\n", sock->sock);fflush(stdout);*/
				if (closesocket(sock->sock) == SOCKET_ERROR)
				{
				    mpi_errno = WSAGetLastError();
				    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**closesocket", "**closesocket %s %d", get_error_string(mpi_errno), mpi_errno);
				    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_WAIT);
				    return mpi_errno;
				}
				sock->sock = INVALID_SOCKET;
			    }
			    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_WAIT);
			    return MPI_SUCCESS;
			}
			/* make the user upcall */
			if (sock->write.progress_update != NULL)
			    sock->write.progress_update(num_bytes, sock->user_ptr);
			/* post a write of the remaining data */
			MPIU_DBG_PRINTF(("sock_wait: posting write of the remaining data, vec size %d\n", sock->write.iovlen));
			if (WSASend(sock->sock, sock->write.iov, sock->write.iovlen, &sock->write.num_bytes, 0, &sock->write.ovl, NULL) == SOCKET_ERROR)
			{
			    mpi_errno = WSAGetLastError();
			    if (mpi_errno == 0)
			    {
				/*sock->wt2 = PMPI_Wtime();*/
				/*printf("[%d] time from post_write to op_write: %.3f - sock %d\n", getpid(), sock->wt2 - sock->wt1, sock->sock);*/
				out->op_type = MPIDU_SOCK_OP_WRITE;
				out->num_bytes = sock->write.total;
				/*printf("sock_wait returning %d bytes and socket closed\n", sock->write.total);*/
				out->error = MPIDU_SOCK_ERR_CONN_CLOSED;
				out->user_ptr = sock->user_ptr;
				sock->pending_operations--;
				sock->state &= ~SOCKI_WRITING;
				if (sock->closing && sock->pending_operations == 0)
				{
				    MPIU_DBG_PRINTF(("sock_wait: closing socket(%d) after iov read completed.\n", MPIDU_Sock_get_sock_id(sock)));
				    FlushFileBuffers((HANDLE)sock->sock);
				    if (shutdown(sock->sock, SD_BOTH) == SOCKET_ERROR)
				    {
					mpi_errno = WSAGetLastError();
					mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**shutdown", "**shutdown %s %d", get_error_string(mpi_errno), mpi_errno);
					MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_WAIT);
					return mpi_errno;
				    }
				    /*printf("closing socket %d\n", sock->sock);fflush(stdout);*/
				    if (closesocket(sock->sock) == SOCKET_ERROR)
				    {
					mpi_errno = WSAGetLastError();
					mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**closesocket", "**closesocket %s %d", get_error_string(mpi_errno), mpi_errno);
					MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_WAIT);
					return mpi_errno;
				    }
				    sock->sock = INVALID_SOCKET;
				}
				MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_WAIT);
				return MPI_SUCCESS;
			    }
			    if (mpi_errno == WSAENOBUFS)
			    {
				WSABUF tmp;
				tmp.buf = sock->write.iov[0].buf;
				tmp.len = sock->write.iov[0].len;
				while (mpi_errno == WSAENOBUFS)
				{
				    /*printf("[%d] sending %d bytes\n", __LINE__, tmp.len);fflush(stdout);*/
				    if (WSASend(sock->sock, &tmp, 1, &sock->write.num_bytes, 0, &sock->write.ovl, NULL) != SOCKET_ERROR)
				    {
					/* FIXME: does this data need to be handled immediately? */
					mpi_errno = WSA_IO_PENDING;
					break;
				    }
				    mpi_errno = WSAGetLastError();
				    if (mpi_errno == WSA_IO_PENDING)
				    {
					break;
				    }
				    /*printf("[%d] reducing send length from %d to %d\n", __LINE__, tmp.len, tmp.len / 2);fflush(stdout);*/
				    tmp.len = tmp.len / 2;
				}
			    }
			    if (mpi_errno != WSA_IO_PENDING)
			    {
				/*sock->wt2 = PMPI_Wtime();*/
				/*printf("[%d] time from post_write to op_write: %.3f - sock %d\n", getpid(), sock->wt2 - sock->wt1, sock->sock);*/
				out->op_type = MPIDU_SOCK_OP_WRITE;
				out->num_bytes = sock->write.total;
				out->error = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**fail", "**fail %s %d", get_error_string(mpi_errno), mpi_errno);
				out->user_ptr = sock->user_ptr;
				sock->pending_operations--;
				sock->state &= ~SOCKI_WRITING;
				if (sock->closing && sock->pending_operations == 0)
				{
				    MPIU_DBG_PRINTF(("sock_wait: closing socket(%d) after iov read completed.\n", MPIDU_Sock_get_sock_id(sock)));
				    FlushFileBuffers((HANDLE)sock->sock);
				    if (shutdown(sock->sock, SD_BOTH) == SOCKET_ERROR)
				    {
					mpi_errno = WSAGetLastError();
					mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**shutdown", "**shutdown %s %d", get_error_string(mpi_errno), mpi_errno);
					MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_WAIT);
					return mpi_errno;
				    }
				    /*printf("closing socket %d\n", sock->sock);fflush(stdout);*/
				    if (closesocket(sock->sock) == SOCKET_ERROR)
				    {
					mpi_errno = WSAGetLastError();
					mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**closesocket", "**closesocket %s %d", get_error_string(mpi_errno), mpi_errno);
					MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_WAIT);
					return mpi_errno;
				    }
				    sock->sock = INVALID_SOCKET;
				}
				MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_WAIT);
				return MPI_SUCCESS;
			    }
			}
		    }
		}
		else
		{
		    if (num_bytes == 0)
		    {
			if ((sock->state & SOCKI_READING))/* && sock-sock != INVALID_SOCKET) */
			{
			    /*sock->rt2 = PMPI_Wtime();*/
			    /*printf("[%d] time from post_read  to op_read : %.3f - sock %d\n", getpid(), sock->rt2 - sock->rt1, sock->sock);*/
			    MPIU_DBG_PRINTF(("EOF with posted read on sock %d\n", sock->sock));
			    out->op_type = MPIDU_SOCK_OP_READ;
			    out->num_bytes = sock->read.total;
			    /*printf("sock_wait returning %d bytes and EOF\n", sock->read.total);*/
			    out->error = MPIDU_SOCK_ERR_CONN_CLOSED;
			    out->user_ptr = sock->user_ptr;
			    sock->state &= ~SOCKI_READING;
			    sock->pending_operations--;
			    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_WAIT);
			    return MPI_SUCCESS;
			}
			if ((sock->state & SOCKI_WRITING))/* && sock->sock != INVALID_SOCKET) */
			{
			    /*sock->wt2 = PMPI_Wtime();*/
			    /*printf("[%d] time from post_write to op_write: %.3f - sock %d\n", getpid(), sock->wt2 - sock->wt1, sock->sock);*/
			    MPIU_DBG_PRINTF(("EOF with posted write on sock %d\n", sock->sock));
			    out->op_type = MPIDU_SOCK_OP_WRITE;
			    out->num_bytes = sock->write.total;
			    /*printf("sock_wait returning %d bytes and EOF\n", sock->write.total);*/
			    out->error = MPIDU_SOCK_ERR_CONN_CLOSED;
			    out->user_ptr = sock->user_ptr;
			    sock->state &= ~SOCKI_WRITING;
			    sock->pending_operations--;
			    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_WAIT);
			    return MPI_SUCCESS;
			}
			MPIU_DBG_PRINTF(("ignoring EOF notification on unknown sock %d.\n", MPIDU_Sock_get_sock_id(sock)));
		    }

		    if (sock->sock != INVALID_SOCKET)
		    {
			MPIU_DBG_PRINTF(("unmatched ovl: pending: %d, state = %d\n", sock->pending_operations, sock->state));
			/*MPL_error_printf("In sock_wait(), returned overlapped structure does not match the current read or write ovl: 0x%x\n", ovl);*/
			MPL_snprintf(error_msg, 1024, "In sock_wait(), returned overlapped structure does not match the current read or write ovl: 0x%p\n", ovl);
			MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_WAIT);
			return MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**fail", "**fail %s", error_msg);
		    }
		    else
		    {
			MPIU_DBG_PRINTF(("ignoring notification on invalid sock.\n"));
		    }
		}
	    }
	    else if (sock->type == SOCKI_WAKEUP)
	    {
		out->op_type = MPIDU_SOCK_OP_WAKEUP;
		out->num_bytes = 0;
		out->error = MPI_SUCCESS;
		out->user_ptr = NULL;
		MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_WAIT);
		return MPI_SUCCESS;
	    }
	    else if (sock->type == SOCKI_LISTENER)
	    {
		if (sock->closing && sock->pending_operations == 0)
		{
		    if (!sock->listener_closed)
		    {
			/*sock->ct2 = PMPI_Wtime();*/
			/*printf("[%d] time from post_close to op_close: %.3f - sock %d\n", getpid(), sock->ct2 - sock->ct1, sock->sock);*/
			/* signal that the listener has been closed and prevent further posted accept failures from returning extra close_ops */
			sock->listener_closed = 1;
			/*printf("<2>");fflush(stdout);*/
			out->op_type = MPIDU_SOCK_OP_CLOSE;
			out->num_bytes = 0;
			out->error = MPI_SUCCESS;
			out->user_ptr = sock->user_ptr;
			CloseHandle(sock->read.ovl.hEvent);
			CloseHandle(sock->write.ovl.hEvent);
			sock->read.ovl.hEvent = NULL;
			sock->write.ovl.hEvent = NULL;
			MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_WAIT);
			return MPI_SUCCESS;
		    }
		    else
		    {
			/* ignore multiple close operations caused by the outstanding accept operations */
			continue;
		    }
		}
		iter = sock->list;
		while (iter && &iter->read.ovl != ovl)
		    iter = iter->next;
		if (iter != NULL)
		    iter->accepted = 1;
		out->op_type = MPIDU_SOCK_OP_ACCEPT;
		if (sock->listen_sock == INVALID_SOCKET)
		{
		    MPL_error_printf("returning MPIDU_SOCK_OP_ACCEPT with an INVALID_SOCKET for the listener\n");
		}
		out->num_bytes = num_bytes;
		out->error = MPI_SUCCESS;
		out->user_ptr = sock->user_ptr;
		MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_WAIT);
		return MPI_SUCCESS;
	    }
	    else
	    {
		/*MPL_error_printf("sock type is not a SOCKET or a LISTENER, it's %d\n", sock->type);*/
		MPL_snprintf(error_msg, 1024, "sock type is not a SOCKET or a LISTENER, it's %d\n", sock->type);
		MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_WAIT);
		return MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**fail", "**fail %s", error_msg);
	    }
	}
	else
	{
#if defined(MPICH_IS_THREADED)
#           if (MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY_GLOBAL)
	    {
                int err;
		/* Reaquire the lock before processing any of the information returned from GetQueuedCompletionStatus */
		MPIU_DBG_MSG(THREAD,TYPICAL,"Enter global critical section");
		MPIU_THREAD_CHECK_BEGIN;
		MPID_Thread_mutex_lock(&MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX, &err);
		MPIU_THREAD_CHECK_END;
	    }
#           else
#               error selected multi-threaded implementation is not supported
#           endif
#endif
	    /*MPIDI_FUNC_EXIT(MPID_STATE_GETQUEUEDCOMPLETIONSTATUS);*/ /* Maybe the logging will reset the last error? */
	    mpi_errno = GetLastError();
	    /*t2 = PMPI_Wtime();*/
	    /*printf("[%d] GetQueuedCompletionStatus took %.3f seconds for sock: %d\n", getpid(), t2-t1, sock != NULL ? sock->sock : 0);*/
	    MPIDI_FUNC_EXIT(MPID_STATE_GETQUEUEDCOMPLETIONSTATUS);
	    /* interpret error, return appropriate SOCK_ERR_... macro */
	    if (mpi_errno == WAIT_TIMEOUT)
	    {
		MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_WAIT);
		return MPIDU_SOCK_ERR_TIMEOUT;
	    }
	    if (mpi_errno == ERROR_OPERATION_ABORTED)
	    {
		/* A thread exited causing GetQueuedCompletionStatus to return prematurely. */
		if (sock != NULL && !sock->closing)
		{
		    /* re-post the aborted operation */
		    if (sock->type == SOCKI_SOCKET)
		    {
			if (ovl == &sock->read.ovl)
			{
			    /*printf("repost readv of length %d\n", sock->read.iovlen);fflush(stdout);*/
			    mpi_errno = MPIDU_Sock_post_readv(sock, sock->read.iov, sock->read.iovlen, NULL);
			    if (mpi_errno != MPI_SUCCESS)
			    {
				/*sock->rt2 = PMPI_Wtime();*/
				/*printf("[%d] time from post_read  to op_read : %.3f - sock %d\n", getpid(), sock->rt2 - sock->rt1, sock->sock);*/
				out->op_type = MPIDU_SOCK_OP_READ;
				out->num_bytes = 0;
				out->error = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**fail", "**fail %s", "Unable to re-post an aborted readv operation");
				out->user_ptr = sock->user_ptr;
				sock->state &= ~SOCKI_READING; /* remove the SOCKI_READING bit */
				MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_WAIT);
				return MPI_SUCCESS;
			    }
			}
			else if (ovl == &sock->write.ovl)
			{
			    /*printf("repost writev of length %d\n", sock->write.iovlen);fflush(stdout);*/
			    mpi_errno = MPIDU_Sock_post_writev(sock, sock->write.iov, sock->write.iovlen, NULL);
			    if (mpi_errno != MPI_SUCCESS)
			    {
				/*sock->wt2 = PMPI_Wtime();*/
				/*printf("[%d] time from post_write to op_write: %.3f - sock %d\n", getpid(), sock->wt2 - sock->wt1, sock->sock);*/
				out->op_type = MPIDU_SOCK_OP_WRITE;
				out->num_bytes = 0;
				out->error = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**fail", "**fail %s", "Unable to re-post an aborted writev operation");
				out->user_ptr = sock->user_ptr;
				sock->state &= ~SOCKI_WRITING; /* remove the SOCKI_WRITING bit */
				MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_WAIT);
				return MPI_SUCCESS;
			    }
			}
			else
			{
			    /*printf("aborted sock operation\n");fflush(stdout);*/
			    out->op_type = -1;
			    out->num_bytes = 0;
			    out->error = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**fail", "**fail %s", "Aborted socket operation");
			    out->user_ptr = sock->user_ptr;
			    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_WAIT);
			    return MPI_SUCCESS;
			}
		    }
		    else if (sock->type == SOCKI_WAKEUP)
		    {
			/*printf("aborted wakeup operation\n");fflush(stdout);*/
			out->op_type = MPIDU_SOCK_OP_WAKEUP;
			out->num_bytes = 0;
			out->error = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**fail", "**fail %s", "Aborted wakeup operation");
			out->user_ptr = sock->user_ptr;
			MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_WAIT);
			return MPI_SUCCESS;
		    }
		    else if (sock->type == SOCKI_LISTENER)
		    {
			/*printf("aborted listener operation\n");fflush(stdout);*/
			iter = sock->list;
			while (iter && &iter->read.ovl != ovl)
			    iter = iter->next;
			if (iter != NULL)
			{
			    mpi_errno = post_next_accept(iter);
			    if (mpi_errno != MPI_SUCCESS)
			    {
				out->op_type = MPIDU_SOCK_OP_ACCEPT;
				out->num_bytes = 0;
				out->error = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**fail", "**fail %s", "Unable to re-post an aborted accept operation");
				out->user_ptr = sock->user_ptr;
				MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_WAIT);
				return MPI_SUCCESS;
			    }
			}
			else
			{
			    /*printf("aborted unknown listener socket operation\n");fflush(stdout);*/
			    out->op_type = -1;
			    out->num_bytes = 0;
			    out->error = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**fail", "**fail %s", "Aborted unknown listener socket operation");
			    out->user_ptr = sock->user_ptr;
			    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_WAIT);
			    return MPI_SUCCESS;
			}
		    }
		    else
		    {
			/*printf("aborted unknown socket operation\n");fflush(stdout);*/
			out->op_type = -1;
			out->num_bytes = 0;
			out->error = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**fail", "**fail %s", "Aborted unknown socket operation");
			out->user_ptr = sock->user_ptr;
			MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_WAIT);
			return MPI_SUCCESS;
		    }
		    continue;
		}
	    }
	    MPIU_DBG_PRINTF(("GetQueuedCompletionStatus failed, GetLastError: %d\n", mpi_errno));
	    if (sock != NULL)
	    {
		MPIU_DBG_PRINTF(("GetQueuedCompletionStatus failed, sock != null\n"));
		if (sock->type == SOCKI_SOCKET)
		{
		    MPIU_DBG_PRINTF(("GetQueuedCompletionStatus failed, sock == SOCKI_SOCKET \n"));
		    if (sock->closing)
		    {
			/*sock->ct2 = PMPI_Wtime();*/
			/*printf("[%d] time from post_close to op_close: %.3f - sock %d\n", getpid(), sock->ct2 - sock->ct1, sock->sock);*/
			out->op_type = MPIDU_SOCK_OP_CLOSE;
			if (sock->sock != INVALID_SOCKET)
			{
			    /*("closing socket %d\n", sock->sock);fflush(stdout);*/
			    if (closesocket(sock->sock) == SOCKET_ERROR)
			    {
				mpi_errno = WSAGetLastError();
				mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**closesocket", "**closesocket %s %d", get_error_string(mpi_errno), mpi_errno);
				MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_WAIT);
				return mpi_errno;
			    }
			    sock->sock = INVALID_SOCKET;
			}
		    }
		    else
		    {
			if (ovl == &sock->read.ovl)
			{
			    /*sock->rt2 = PMPI_Wtime();*/
			    /*printf("[%d] time from post_read  to op_read : %.3f - sock %d\n", getpid(), sock->rt2 - sock->rt1, sock->sock);*/
			    out->op_type = MPIDU_SOCK_OP_READ;
			    sock->state &= ~SOCKI_READING; /* remove the SOCKI_READING bit */
			}
			else if (ovl == &sock->write.ovl)
			{
			    /*sock->wt2 = PMPI_Wtime();*/
			    /*printf("[%d] time from post_write to op_write: %.3f - sock %d\n", getpid(), sock->wt2 - sock->wt1, sock->sock);*/
			    out->op_type = MPIDU_SOCK_OP_WRITE;
			    sock->state &= ~SOCKI_WRITING; /* remove the SOCKI_WRITING bit */
			}
			else
			{
			    /* Shoud we reset the state bits? */
			    sock->state &= ~SOCKI_READING; /* remove the SOCKI_READING bit */
			    sock->state &= ~SOCKI_WRITING; /* remove the SOCKI_WRITING bit */
			    out->op_type = -1;
			}
		    }
		    out->num_bytes = 0;
		    out->error = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**fail", "**fail %s %d", get_error_string(mpi_errno), mpi_errno);
		    out->user_ptr = sock->user_ptr;
		    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_WAIT);
		    return MPI_SUCCESS;
		}
		if (sock->type == SOCKI_LISTENER)
		{
		    MPIU_DBG_PRINTF(("GetQueuedCompletionStatus failed, sock == SOCKI_LISTENER \n"));
		    /* this only works if the aborted operation is reported before the close is reported
		       because the close will free the sock structure causing these dereferences bogus.
		       I need to reference count the sock */
		    if (sock->closing)
		    {
			if (sock->listener_closed)
			{
			    /* only return a close_op once for the main listener, not any of the copies */
			    continue;
			}
			else
			{
			    /*sock->ct2 = PMPI_Wtime();*/
			    /*printf("[%d] time from post_close to op_close: %.3f - sock %d\n", getpid(), sock->ct2 - sock->ct1, sock->sock);*/
			    sock->listener_closed = 1;
			    /*printf("<3>");fflush(stdout);*/
			    out->op_type = MPIDU_SOCK_OP_CLOSE;
			    out->num_bytes = 0;
			    if (mpi_errno == ERROR_OPERATION_ABORTED)
				out->error = MPI_SUCCESS;
			    else
				out->error = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**fail", "**fail %s %d", get_error_string(mpi_errno), mpi_errno);
			    out->user_ptr = sock->user_ptr;
			    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_WAIT);
			    return MPI_SUCCESS;
			}
		    }
		    else
		    {
			/* Should we return a SOCK_OP_ACCEPT with an error if there is a failure on the listener? */
			out->op_type = MPIDU_SOCK_OP_ACCEPT;
			out->num_bytes = 0;
			out->error = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**fail", "**fail %s %d", get_error_string(mpi_errno), mpi_errno);
			out->user_ptr = sock->user_ptr;
			MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_WAIT);
			return MPI_SUCCESS;
		    }
		}
	    }

	    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_WAIT);
	    return MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**fail", "**fail %s %d", get_error_string(mpi_errno), mpi_errno);
	}
    }
    /* Unreachable code section - all sections exit/return before reaching this segment */
    /*
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_WAIT);
    return MPI_SUCCESS;
    */
}

#undef FUNCNAME
#define FUNCNAME MPIDU_Sock_wakeup
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDU_Sock_wakeup(MPIDU_Sock_set_t set)
{
    /* post a completion event to wake up sock_wait */
    PostQueuedCompletionStatus(set, 0, (ULONG_PTR)&g_wakeup_state, &g_wakeup_state.write.ovl);
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPIDU_Sock_read
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDU_Sock_read(MPIDU_Sock_t sock, void * buf, MPIU_Size_t len, MPIU_Size_t * num_read)
{
    int mpi_errno = MPI_SUCCESS;
    MPL_IOV iov;
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_SOCK_READ);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_SOCK_READ);
    iov.buf = buf;
    iov.len = len;
    mpi_errno = MPIDU_Sock_readv(sock, &iov, 1, num_read);
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_READ);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDU_Sock_readv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDU_Sock_readv(MPIDU_Sock_t sock, MPL_IOV * iov, int iov_n, MPIU_Size_t * num_read)
{
    int mpi_errno = MPI_SUCCESS;
    DWORD nFlags = 0;
    DWORD num_read_local;
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_SOCK_READV);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_SOCK_READV);

    if (WSARecv(sock->sock, iov, iov_n, &num_read_local, &nFlags, NULL/*overlapped*/, NULL/*completion routine*/) == SOCKET_ERROR)
    {
	mpi_errno = WSAGetLastError();
	*num_read = 0;
	if ((mpi_errno == WSAEWOULDBLOCK) || (mpi_errno == 0))
	{
	    mpi_errno = MPI_SUCCESS;
	}
	else
	{
	    if (mpi_errno == WSAENOBUFS)
	    {
		WSABUF tmp;
		tmp.buf = iov[0].buf;
		tmp.len = iov[0].len;
		while (mpi_errno == WSAENOBUFS)
		{
		    /*printf("[%d] receiving %d bytes\n", __LINE__, tmp.len);fflush(stdout);*/
		    if (WSARecv(sock->sock, &tmp, 1, &num_read_local, &nFlags, NULL, NULL) != SOCKET_ERROR)
		    {
			/*printf("[%d] read %d bytes\n", __LINE__, num_read_local);fflush(stdout);*/
			*num_read = num_read_local;
			mpi_errno = MPI_SUCCESS;
			break;
		    }
		    /*printf("[%d] reducing send length from %d to %d\n", __LINE__, tmp.len, tmp.len / 2);fflush(stdout);*/
		    mpi_errno = WSAGetLastError();
		    tmp.len = tmp.len / 2;
		}
		if (mpi_errno != MPI_SUCCESS)
		{
		    if (mpi_errno == WSAEWOULDBLOCK)
		    {
			mpi_errno = MPI_SUCCESS;
		    }
		    else
		    {
			mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**fail", "**fail %s %d", get_error_string(mpi_errno), mpi_errno);
		    }
		}
	    }
	    else
	    {
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**fail", "**fail %s %d", get_error_string(mpi_errno), mpi_errno);
	    }
	}
    }
    else
    {
	*num_read = num_read_local;
    }
    MPIU_DBG_PRINTF(("sock_readv %d bytes\n", *num_read));
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_READV);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDU_Sock_write
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDU_Sock_write(MPIDU_Sock_t sock, void * buf, MPIU_Size_t len, MPIU_Size_t * num_written)
{
    int mpi_errno;
    MPL_IOV iov;
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_SOCK_WRITE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_SOCK_WRITE);
    iov.len = len;
    iov.buf = buf;
    mpi_errno = MPIDU_Sock_writev(sock, &iov, 1, num_written);
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_WRITE);
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPIDU_Sock_writev
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDU_Sock_writev(MPIDU_Sock_t sock, MPL_IOV * iov, int iov_n, MPIU_Size_t * num_written)
{
    int mpi_errno;
    DWORD num_written_local;
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_SOCK_WRITEV);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_SOCK_WRITEV);
    if (WSASend(sock->sock, iov, iov_n, &num_written_local, 0, NULL/*overlapped*/, NULL/*completion routine*/) == SOCKET_ERROR)
    {
	mpi_errno = WSAGetLastError();
	*num_written = 0;
	if (mpi_errno == WSAENOBUFS)
	{
	    WSABUF tmp;
	    tmp.buf = iov[0].buf;
	    tmp.len = iov[0].len;
	    while (mpi_errno == WSAENOBUFS)
	    {
		/*printf("[%d] sending %d bytes\n", __LINE__, tmp.len);fflush(stdout);*/
		if (WSASend(sock->sock, &tmp, 1, &num_written_local, 0, NULL, NULL) != SOCKET_ERROR)
		{
		    /*printf("[%d] wrote %d bytes\n", __LINE__, num_written_local);fflush(stdout);*/
		    *num_written = num_written_local;
		    mpi_errno = MPI_SUCCESS;
		    break;
		}
		mpi_errno = WSAGetLastError();
		/*printf("[%d] reducing send length from %d to %d\n", __LINE__, tmp.len, tmp.len / 2);fflush(stdout);*/
		tmp.len = tmp.len / 2;
		if (tmp.len == 0 && mpi_errno == WSAENOBUFS)
		{
		    break;
		}
	    }
	}
	if (mpi_errno && (mpi_errno != WSAEWOULDBLOCK))
	{
	    MPIU_DBG_PRINTF(("WSASend failed: error %d\n", mpi_errno));
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**fail", "**fail %s %d", get_error_string(mpi_errno), mpi_errno);
	    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_WRITEV);
	    return mpi_errno;
	}
    }
    else
    {
	*num_written = num_written_local;
    }
    MPIU_DBG_PRINTF(("sock_writev %d bytes\n", *num_written));

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_WRITEV);
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPIDU_Sock_get_sock_id
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDU_Sock_get_sock_id(MPIDU_Sock_t sock)
{
    int ret_val;
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_SOCK_GET_SOCK_ID);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_SOCK_GET_SOCK_ID);
    if (sock == MPIDU_SOCK_INVALID_SOCK)
	ret_val = -1;
    else
    {
	if (sock->type == SOCKI_LISTENER)
	    ret_val = (int)sock->listen_sock;
	else
	    ret_val = (int)sock->sock;
    }
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_GET_SOCK_ID);
    return ret_val;
}

#undef FUNCNAME
#define FUNCNAME MPIDU_Sock_get_sock_set_id
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDU_Sock_get_sock_set_id(MPIDU_Sock_set_t set)
{
    int ret_val;
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_SOCK_GET_SOCK_SET_ID);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_SOCK_GET_SOCK_SET_ID);
    ret_val = PtrToInt(set);/*(int)set;*/
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_GET_SOCK_SET_ID);
    return ret_val;
}

#undef FUNCNAME
#define FUNCNAME MPIDU_Sock_get_error_class_string
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDU_Sock_get_error_class_string(int error, char *error_string, int length)
{
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_SOCK_GET_ERROR_CLASS_STRING);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_SOCK_GET_ERROR_CLASS_STRING);
    switch (MPIR_ERR_GET_CLASS(error))
    {
    case MPIDU_SOCK_ERR_FAIL:
	MPIU_Strncpy(error_string, "generic socket failure", length);
	break;
    case MPIDU_SOCK_ERR_INIT:
	MPIU_Strncpy(error_string, "socket module not initialized", length);
	break;
    case MPIDU_SOCK_ERR_NOMEM:
	MPIU_Strncpy(error_string, "not enough memory to complete the socket operation", length);
	break;
    case MPIDU_SOCK_ERR_BAD_SET:
	MPIU_Strncpy(error_string, "invalid socket set", length);
	break;
    case MPIDU_SOCK_ERR_BAD_SOCK:
	MPIU_Strncpy(error_string, "invalid socket", length);
	break;
    case MPIDU_SOCK_ERR_BAD_HOST:
	MPIU_Strncpy(error_string, "host description buffer not large enough", length);
	break;
    case MPIDU_SOCK_ERR_BAD_HOSTNAME:
	MPIU_Strncpy(error_string, "invalid host name", length);
	break;
    case MPIDU_SOCK_ERR_BAD_PORT:
	MPIU_Strncpy(error_string, "invalid port", length);
	break;
    case MPIDU_SOCK_ERR_BAD_BUF:
	MPIU_Strncpy(error_string, "invalid buffer", length);
	break;
    case MPIDU_SOCK_ERR_BAD_LEN:
	MPIU_Strncpy(error_string, "invalid length", length);
	break;
    case MPIDU_SOCK_ERR_SOCK_CLOSED:
	MPIU_Strncpy(error_string, "socket closed", length);
	break;
    case MPIDU_SOCK_ERR_CONN_CLOSED:
	MPIU_Strncpy(error_string, "socket connection closed", length);
	break;
    case MPIDU_SOCK_ERR_CONN_FAILED:
	MPIU_Strncpy(error_string, "socket connection failed", length);
	break;
    case MPIDU_SOCK_ERR_INPROGRESS:
	MPIU_Strncpy(error_string, "socket operation in progress", length);
	break;
    case MPIDU_SOCK_ERR_TIMEOUT:
	MPIU_Strncpy(error_string, "socket operation timed out", length);
	break;
    case MPIDU_SOCK_ERR_INTR:
	MPIU_Strncpy(error_string, "socket operation interrupted", length);
	break;
    case MPIDU_SOCK_ERR_NO_NEW_SOCK:
	MPIU_Strncpy(error_string, "no new connection available", length);
	break;
    default:
	MPL_snprintf(error_string, length, "unknown socket error %d", error);
	break;
    }
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_GET_ERROR_CLASS_STRING);
    return MPI_SUCCESS;
}
