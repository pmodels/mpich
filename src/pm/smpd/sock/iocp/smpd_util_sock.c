/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/*
#include "mpiimpl.h"
*/
#include "smpd.h"
#include "smpd_util_sock.h"

#define SMPDI_QUOTE(A) SMPDI_QUOTE2(A)
#define SMPDI_QUOTE2(A) #A

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
    SMPD_IOV tiov;
#ifdef USE_SOCK_IOV_COPY
    SMPD_IOV iov[SMPD_IOV_MAXLEN];
#else
    SMPD_IOV *iov;
#endif
    int iovlen;
    int index;
    SMPDU_Sock_size_t total;
    SMPDU_Sock_progress_update_func_t progress_update;
} sock_buffer;

typedef struct sock_state_t
{
    SOCKI_TYPE type;
    SOCKI_STATE state;
    SOCKET sock;
    SMPDU_Sock_set_t set;
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
static int g_min_port = 0;
static int g_max_port = 0;

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
    if (num_bytes == 0){
	    if (prepend != NULL){
	        MPIU_Strncpy(msg, prepend, 1024);
        }
	    else{
	        *msg = '\0';
        }
    }
    else{
	    if (prepend == NULL){
	        memcpy(msg, str, num_bytes+1);
        }
	    else{
	        MPIU_Snprintf(msg, 1024, "%s%s", prepend, (const char*)str);
        }
	    LocalFree(str);
	    strtok(msg, "\r\n");
    }
}

static char *get_err_string(int error_code)
{
    /* obviously not thread safe to store a message like this */
    static char error_msg[1024];
    translate_error(error_code, error_msg, NULL);
    return error_msg;
}

#undef FUNCNAME
#define FUNCNAME easy_create
#undef FCNAME
#define FCNAME SMPDI_QUOTE(FUNCNAME)
static int easy_create_ranged(SOCKET *sock, int port, unsigned long addr)
{
    int result;
    /*struct linger linger;*/
    int optval, len;
    SOCKET temp_sock;
    SOCKADDR_IN sockAddr;
    int use_range = 0;

    /* create the non-blocking socket */
    temp_sock = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
    if (temp_sock == INVALID_SOCKET){
	    result = WSAGetLastError();
	    /*
	    result = SMPDR_Err_create_code(SMPD_SUCCESS, SMPDR_ERR_RECOVERABLE, FCNAME, __LINE__, SMPDU_SOCK_ERR_FAIL, "**socket", "**socket %s %d", get_error_string(result), result);
	    */
	    smpd_err_printf("Error creating a socket, %s (%d) \n", get_err_string(result), result);
	    return result;
    }
    
    if (port == 0 && g_min_port != 0 && g_max_port != 0){
	    use_range = 1;
	    port = g_min_port;
    }

    memset(&sockAddr,0,sizeof(sockAddr));
    
    sockAddr.sin_family = AF_INET;
    sockAddr.sin_addr.s_addr = addr;
    sockAddr.sin_port = htons((unsigned short)port);

    for (;;){
	    if (bind(temp_sock, (SOCKADDR*)&sockAddr, sizeof(sockAddr)) == SOCKET_ERROR){
	        if (use_range){
		        port++;
		        if (port > g_max_port){
                    /*
		            result = SMPDR_Err_create_code(SMPD_SUCCESS, SMPDR_ERR_RECOVERABLE, FCNAME, __LINE__, SMPDU_SOCK_ERR_FAIL, "**socket", 0);
                    */
                    result = SMPD_FAIL;
                    smpd_err_printf("Error binding socket to given port range, %s (%d)\n", get_err_string(result), result);
		            return result;
		        }
		        sockAddr.sin_port = htons((unsigned short)port);
	        }
	        else{
		        result = WSAGetLastError();
                /*
		        result = SMPDR_Err_create_code(SMPD_SUCCESS, SMPDR_ERR_RECOVERABLE, FCNAME, __LINE__, SMPDU_SOCK_ERR_FAIL, "**socket", "**socket %s %d", get_error_string(result), result);
                */
                smpd_err_printf("Error binding socket to given port, %s (%d) \n", get_err_string(result), result);
		        return result;
	        }
	    }
	    else{
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
    if (!getsockopt(temp_sock, SOL_SOCKET, SO_RCVBUF, (char*)&optval, &len)){
	    optval = g_socket_rbuffer_size;
	    if(setsockopt(temp_sock, SOL_SOCKET, SO_RCVBUF, (char*)&optval, sizeof(int))
            == SOCKET_ERROR){
            result = WSAGetLastError();
            smpd_dbg_printf("Error setting SO_RCVBUF option for socket, %s (%d)\n", get_err_string(result), result);
            /* Don't fail on error in setting the SO_RCVBUF option */
        }
    }
    len = sizeof(int);
    if (!getsockopt(temp_sock, SOL_SOCKET, SO_SNDBUF, (char*)&optval, &len)){
	    optval = g_socket_sbuffer_size;
	    if(setsockopt(temp_sock, SOL_SOCKET, SO_SNDBUF, (char*)&optval, sizeof(int))
            == SOCKET_ERROR){
            result = WSAGetLastError();
            smpd_dbg_printf("Error setting SO_SNDBUF option for socket, %s (%d)\n", get_err_string(result), result);
            /* Don't fail on error in setting the SO_SNDBUF option */
        }
    }

    /* prevent the socket from being inherited by child processes */
    /* FIXME: Use WSADuplicateSocket() instead */
    if (!DuplicateHandle(
	        GetCurrentProcess(), (HANDLE)temp_sock,
	        GetCurrentProcess(), (HANDLE*)sock,
	        0, FALSE, DUPLICATE_CLOSE_SOURCE | DUPLICATE_SAME_ACCESS)){
	    result = GetLastError();
        /*
	    result = SMPDR_Err_create_code(SMPD_SUCCESS, SMPDR_ERR_RECOVERABLE, FCNAME, __LINE__, SMPDU_SOCK_ERR_FAIL, "**duphandle", "**duphandle %s %d", get_error_string(result), result);
        */
        smpd_err_printf("Error duplicating socket handle, %s (%d)\n", get_err_string(result), result);
	    return result;
    }

    /* Set the no-delay option */
    if(setsockopt(*sock, IPPROTO_TCP, TCP_NODELAY, (char *)&optval, sizeof(optval))
        == SOCKET_ERROR){
        result = WSAGetLastError();
        smpd_dbg_printf("Error setting TCP_NODELAY option for socket, %s (%d)\n", get_err_string(result), result);
        /* Don't fail on error in setting the TCP_NODELAY option */
    }

    return SMPD_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME easy_create
#undef FCNAME
#define FCNAME SMPDI_QUOTE(FUNCNAME)
static int easy_create(SOCKET *sock, int port, unsigned long addr)
{
    int result;
    /*struct linger linger;*/
    int optval, len;
    SOCKET temp_sock;
    SOCKADDR_IN sockAddr;

    /* create the non-blocking socket */
    temp_sock = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
    if (temp_sock == INVALID_SOCKET){
	    result = WSAGetLastError();
        /*
	    result = SMPDR_Err_create_code(SMPD_SUCCESS, SMPDR_ERR_RECOVERABLE, FCNAME, __LINE__, SMPDU_SOCK_ERR_FAIL, "**socket", "**socket %s %d", get_error_string(result), result);
        */
        smpd_err_printf("Error creating a socket, %s (%d)\n", get_err_string(result), result);
	    return result;
    }
    
    memset(&sockAddr,0,sizeof(sockAddr));
    
    sockAddr.sin_family = AF_INET;
    sockAddr.sin_addr.s_addr = addr;
    sockAddr.sin_port = htons((unsigned short)port);

    if (bind(temp_sock, (SOCKADDR*)&sockAddr, sizeof(sockAddr)) == SOCKET_ERROR){
	    result = WSAGetLastError();
        /*
	    result = SMPDR_Err_create_code(SMPD_SUCCESS, SMPDR_ERR_RECOVERABLE, FCNAME, __LINE__, SMPDU_SOCK_ERR_FAIL, "**socket", "**socket %s %d", get_error_string(result), result);
        */
        smpd_err_printf("Error binding socket to port, %s (%d)\n", get_err_string(result), result);
	    return result;
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
    if (!getsockopt(temp_sock, SOL_SOCKET, SO_RCVBUF, (char*)&optval, &len)){
	    optval = g_socket_rbuffer_size;
	    if(setsockopt(temp_sock, SOL_SOCKET, SO_RCVBUF, (char*)&optval, sizeof(int))
            == SOCKET_ERROR){
            result = WSAGetLastError();
            smpd_dbg_printf("Error setting SO_RCVBUF option for socket, %s (%d)\n", get_err_string(result), result);
            /* Don't fail on error in setting the SO_RCVBUF option */
        }
    }
    len = sizeof(int);
    if (!getsockopt(temp_sock, SOL_SOCKET, SO_SNDBUF, (char*)&optval, &len)){
	    optval = g_socket_sbuffer_size;
	    if(setsockopt(temp_sock, SOL_SOCKET, SO_SNDBUF, (char*)&optval, sizeof(int))
            == SOCKET_ERROR){
            result = WSAGetLastError();
            smpd_dbg_printf("Error setting SO_SNDBUF option for socket, %s (%d)\n", get_err_string(result), result);
            /* Don't fail on error in setting the SO_SNDBUF option */
        }
    }

    /* prevent the socket from being inherited by child processes */
    /* FIXME: Use WSADuplicateSocket instead */
    if (!DuplicateHandle(
	    GetCurrentProcess(), (HANDLE)temp_sock,
	    GetCurrentProcess(), (HANDLE*)sock,
	    0, FALSE, DUPLICATE_CLOSE_SOURCE | DUPLICATE_SAME_ACCESS)){
	    result = GetLastError();
        /*
	    result = SMPDR_Err_create_code(SMPD_SUCCESS, SMPDR_ERR_RECOVERABLE, FCNAME, __LINE__, SMPDU_SOCK_ERR_FAIL, "**duphandle", "**duphandle %s %d", get_error_string(result), result);
        */
        smpd_err_printf("Error duplicating socket handle, %s (%d)\n", get_err_string(result), result);
	    return result;
    }

    /* Set the no-delay option */
    if(setsockopt(*sock, IPPROTO_TCP, TCP_NODELAY, (char *)&optval, sizeof(optval))
        == SOCKET_ERROR){
        result = WSAGetLastError();
        smpd_dbg_printf("Error setting TCP_NODELAY option for socket, %s (%d)\n", get_err_string(result), result);
        /* Don't fail on error in setting the TCP_NODELAY option */
    }

    return SMPD_SUCCESS;
}

static inline int easy_get_sock_info(SOCKET sock, char *name, int *port)
{
    int result;
    struct sockaddr_in addr;
    int name_len = sizeof(addr);
    DWORD len = 100;

    if(getsockname(sock, (struct sockaddr*)&addr, &name_len)
        == SOCKET_ERROR){
        result = WSAGetLastError();
        smpd_err_printf("Error retreiving local name for socket, %s (%d)\n", get_err_string(result), result);
        return result;
    }
    *port = ntohs(addr.sin_port);
    /*GetComputerName(name, &len);*/
    if(!GetComputerNameEx(ComputerNameDnsFullyQualified, name, &len)){
        result = WSAGetLastError();
        smpd_err_printf("Error retreiving NetBios/DNS name for socket, %s (%d)\n", get_err_string(result), result);
        return result;
    }
    /*gethostname(name, 100);*/
    return SMPD_SUCCESS;
}

static inline void init_state_struct(sock_state_t *p)
{
    if(p == NULL){
        smpd_dbg_printf("Error initializing state struct: Invalid pointer to state struct\n");
        return;
    }
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
    p->read.tiov.SMPD_IOV_BUF = NULL;
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
    p->write.tiov.SMPD_IOV_BUF = NULL;
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
#define FCNAME SMPDI_QUOTE(FUNCNAME)
static inline int post_next_accept(sock_state_t * context)
{
    int result;
    context->state = SOCKI_ACCEPTING;
    context->accepted = 0;
    /*printf("posting an accept.\n");fflush(stdout);*/
    context->sock = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
    if (context->sock == INVALID_SOCKET){
        result = WSAGetLastError();
        smpd_err_printf("Error creating a socket, %s (%d)\n", get_err_string(result), result);
        return result;
    }
    if (!AcceptEx(
	    context->listen_sock, 
	    context->sock, 
	    context->accept_buffer, 
	    0, 
	    sizeof(struct sockaddr_in)+16, 
	    sizeof(struct sockaddr_in)+16, 
	    &context->read.num_bytes,
	    &context->read.ovl)){
	    
        result = WSAGetLastError();
	    if (result == ERROR_IO_PENDING){
	        return SMPD_SUCCESS;
        }
	    /*SMPDU_Error_printf("AcceptEx failed with error %d\n", error);fflush(stdout);
	    result = SMPDR_Err_create_code(SMPD_SUCCESS, SMPDR_ERR_RECOVERABLE, FCNAME, __LINE__, SMPDU_SOCK_ERR_FAIL, "**fail", "**fail %s %d", get_error_string(result), result);
        */
        smpd_err_printf("Error accepting a new connection, %s (%d)\n", get_err_string(result), result);
	    return result;
    }
    return SMPD_SUCCESS;
}

/* sock functions */

#undef FUNCNAME
#define FUNCNAME SMPDU_Sock_init
#undef FCNAME
#define FCNAME SMPDI_QUOTE(FUNCNAME)
int SMPDU_Sock_init(){
    int result;
    char *szNum, *szRange;
    WSADATA wsaData;
    int err;

    smpd_enter_fn(FCNAME);

    /* FIXME: Do we need this any more ? - PM sock util is single threaded */
    if (g_init_called){
	    g_init_called++;
	    return SMPD_SUCCESS;
    }

    /* Start the Winsock dll */
    if ((err = WSAStartup(MAKEWORD(2, 0), &wsaData)) != 0){
        /*
    	result = SMPDR_Err_create_code(SMPD_SUCCESS, SMPDR_ERR_FATAL, FCNAME, __LINE__, SMPDU_SOCK_ERR_FAIL, "**wsasock", "**wsasock %s %d", get_error_string(err), err);
    	smpd_exit_fn(FCNAME);
        */
        result = SMPD_FAIL;
        smpd_err_printf("Error initializing winsock, %s (%d)\n", get_err_string(result), result);
        smpd_exit_fn(FCNAME);
        return result;
    }

    /* get the socket buffer size */
    szNum = getenv("SMPD_SOCKET_BUFFER_SIZE");
    if (szNum != NULL){
	    g_socket_buffer_size = atoi(szNum);
	    if (g_socket_buffer_size < 0){
	        g_socket_buffer_size = SOCKI_TCP_BUFFER_SIZE;
        }
    }

    g_socket_rbuffer_size = g_socket_buffer_size;
    g_socket_sbuffer_size = g_socket_buffer_size;

    szNum = getenv("SMPD_SOCKET_RBUFFER_SIZE");
    if (szNum != NULL){
	    g_socket_rbuffer_size = atoi(szNum);
	    if (g_socket_rbuffer_size < 0){
	        g_socket_rbuffer_size = g_socket_buffer_size;
        }
    }

    szNum = getenv("SMPD_SOCKET_SBUFFER_SIZE");
    if (szNum != NULL){
	    g_socket_sbuffer_size = atoi(szNum);
	    if (g_socket_sbuffer_size < 0){
	        g_socket_sbuffer_size = g_socket_buffer_size;
        }
    }


    /* get the number of accepts to pre-post */
    szNum = getenv("SMPD_SOCKET_NUM_PREPOSTED_ACCEPTS");
    if (szNum != NULL){
	    g_num_posted_accepts = atoi(szNum);
	    if (g_num_posted_accepts < 1){
	        g_num_posted_accepts = SOCKI_NUM_PREPOSTED_ACCEPTS;
        }
    }

    /* check to see if a port range was specified */
    szRange = getenv("SMPD_PORT_RANGE");
    if (szRange != NULL){
	    szNum = strtok(szRange, ",.:"); /* tokenize min,max and min..max and min:max */
	    if (szNum){
	        g_min_port = atoi(szNum);
	        szNum = strtok(NULL, ",.:");
	        if (szNum){
		        g_max_port = atoi(szNum);
	        }
	    }
	    /* check for invalid values */
	    if (g_min_port < 0 || g_max_port < g_min_port){
	        g_min_port = g_max_port = 0;
	    }
	    /*
	    printf("using min_port = %d, max_port = %d\n", g_min_port, g_max_port);
	    fflush(stdout);
	    */
    }

    init_state_struct(&g_wakeup_state);
    g_wakeup_state.type = SOCKI_WAKEUP;

    g_init_called = 1;
    /*printf("sock init %d\n", g_init_called);fflush(stdout);*/

    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME SMPDU_Sock_finalize
#undef FCNAME
#define FCNAME SMPDI_QUOTE(FUNCNAME)
int SMPDU_Sock_finalize()
{
    sock_state_t *iter;

    smpd_enter_fn(FCNAME);
    if (!g_init_called){
        /*
	    result = SMPDR_Err_create_code(SMPD_SUCCESS, SMPDR_ERR_RECOVERABLE, FCNAME, __LINE__, SMPDU_SOCK_ERR_INIT, "**sock_init", 0);
	    smpd_exit_fn(FCNAME);
        */
        smpd_err_printf("Error finalizing sock, Trying to finalize sock before init\n");
        smpd_exit_fn(FCNAME);
        return SMPD_FAIL;

    }
    g_init_called--;
    if (g_init_called == 0){
	    iter = g_sock_list;
	    while (iter){
	        if (iter->sock != INVALID_SOCKET){
		        /*printf("sock %d not closed before finalize\n", iter->sock);fflush(stdout);*/
		        closesocket(iter->sock);
		        iter->sock = INVALID_SOCKET;
            }
	        iter = iter->next_sock;
	        if (iter == g_sock_list){
		        /* catch loops */
		        /*printf("sock list has a loop\n");fflush(stdout);*/
		        break;
	        }
	    }
	    WSACleanup();
    }
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

typedef struct socki_host_name_t{
    char host[256];
    struct socki_host_name_t *next;
} socki_host_name_t;

static int already_used_or_add(char *host, socki_host_name_t **list)
{
    socki_host_name_t *iter, *trailer;

    /* check if the host already has been used */
    iter = trailer = *list;
    while (iter){
	    if (strcmp(iter->host, host) == 0){
	        return 1;
	    }
	    if (trailer != iter){
	        trailer = trailer->next;
        }
	    iter = iter->next;
    }

    /* the host has not been used so add a node for it */
    iter = (socki_host_name_t*)MPIU_Malloc(sizeof(socki_host_name_t));
    if (!iter){
	    /* if out of memory then treat it as not found */
        smpd_dbg_printf("Adding host to host list failed : malloc() failed\n");
	    return SMPD_SUCCESS;
    }
    MPIU_Strncpy(iter->host, host, 256);

    /* insert new hosts at the end of the list */
    if (trailer != NULL){
        trailer->next = iter;
        iter->next = NULL;
    }
    else{
        iter->next = NULL;
        *list = iter;
    }
    /* insert new hosts at the beginning of the list                            
    iter->next = *list;                                                         
    *list = iter;                                                               
    */
    return SMPD_SUCCESS;
}

static void socki_free_host_list(socki_host_name_t *list)
{
    socki_host_name_t *iter;
    while (list){
	    iter = list;
	    list = list->next;
	    MPIU_Free(iter);
    }
}

static int socki_get_host_list(char *hostname, socki_host_name_t **listp)
{
    int result;
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
    if ((result = getaddrinfo(hostname, NULL, NULL/*&hint*/, &res))){
        /*
        result = SMPDR_Err_create_code(SMPD_SUCCESS, SMPDR_ERR_RECOVERABLE, FCNAME, __LINE__, SMPDU_SOCK_ERR_FAIL, "**getinfo", "**getinfo %s %d", strerror(errno), errno);
        */
        smpd_err_printf("Error translating ANSI hostname to an address, %s (%d)\n", get_err_string(result), result);
        return result;
    }

    /* add the host names */
    iter = res;
    while (iter){
        if (iter->ai_canonname){
            already_used_or_add(iter->ai_canonname, &list);
        }
        else{
            switch (iter->ai_family){
                case PF_INET:
                case PF_INET6:
                    if (getnameinfo(iter->ai_addr, (socklen_t)iter->ai_addrlen, host, 256, NULL, 0, 0) == 0){
                        already_used_or_add(host, &list);
                    }
                    break;
                default:
                    smpd_dbg_printf("Unknown socket family\n");
                    break;
            }
        }
        iter = iter->ai_next;
    }
    /* add the names again, this time as ip addresses */
    iter = res;
    while (iter){
        switch (iter->ai_family){
            case PF_INET:
            case PF_INET6:
                if (getnameinfo(iter->ai_addr, (socklen_t)iter->ai_addrlen, host, 256, NULL, 0, NI_NUMERICHOST) == 0){
                    already_used_or_add(host, &list);
                }
                break;
            default:
                smpd_dbg_printf("Unknown socket family\n");
                break;
        }
        iter = iter->ai_next;
    }
    if (res){
        freeaddrinfo(res);
    }

    *listp = list;
    return SMPD_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME SMPDU_Sock_hostname_to_host_description
#undef FCNAME
#define FCNAME SMPDI_QUOTE(FUNCNAME)
int SMPDU_Sock_hostname_to_host_description(char *hostname, char *host_description, int len)
{
    int result = SMPD_SUCCESS;
    socki_host_name_t *iter, *list = NULL;

    smpd_enter_fn(FCNAME);

    if (!g_init_called){
        /*
	    result = SMPDR_Err_create_code(SMPD_SUCCESS, SMPDR_ERR_RECOVERABLE, FCNAME, __LINE__, SMPDU_SOCK_ERR_INIT, "**sock_init", 0);
	    smpd_exit_fn(FCNAME);
        */
        smpd_err_printf("Attempting to use sock functions without initializing sock\n");
        smpd_exit_fn(FCNAME);
        return SMPD_FAIL;
    }

    result = socki_get_host_list(hostname, &list);
    if (result != SMPD_SUCCESS){
        /*
        result = SMPDR_Err_create_code(result, SMPDR_ERR_RECOVERABLE, FCNAME, __LINE__, SMPD_ERR_OTHER, "**fail", 0);
        */
        smpd_err_printf("Failed to retrieve host list\n");
        goto fn_exit;
    }

    iter = list;
    while (iter){
        smpd_dbg_printf("adding host: %s to host list\n", iter->host);
        result = MPIU_Str_add_string(&host_description, &len, iter->host);
        if (result != MPIU_STR_SUCCESS){
            /*
            result = SMPDR_Err_create_code(result, SMPDR_ERR_RECOVERABLE, FCNAME, __LINE__, SMPDU_SOCK_ERR_NOMEM, "**desc_len", 0);
            */
            smpd_err_printf("Unable to add host to host description\n");
            goto fn_exit;
        }
        iter = iter->next;
    }

 fn_exit:
    socki_free_host_list(list);

    smpd_exit_fn(FCNAME);
    return result;
}

#undef FUNCNAME
#define FUNCNAME SMPDU_Sock_get_host_description
#undef FCNAME
#define FCNAME SMPDI_QUOTE(FUNCNAME)
int SMPDU_Sock_get_host_description(int myRank, 
				    char * host_description, int len)
{
    int result;
    char hostname[100];
    DWORD length = 100;
    char *env;

    smpd_exit_fn(FCNAME);
    if (!g_init_called){
        /*
	    result = SMPDR_Err_create_code(SMPD_SUCCESS, SMPDR_ERR_RECOVERABLE, FCNAME, __LINE__, SMPDU_SOCK_ERR_INIT, "**sock_init", 0);
	    smpd_exit_fn(FCNAME);
        */
        smpd_err_printf("Attempting to use sock functions without initializing sock\n");
        smpd_exit_fn(FCNAME);
        return SMPD_FAIL;
    }

    env = getenv("SMPD_INTERFACE_HOSTNAME");
    if (env != NULL && *env != '\0'){
	    MPIU_Strncpy(hostname, env, sizeof(hostname));
    }
    else{
    	/*if (gethostname(hostname, 100) == SOCKET_ERROR)*/
	    /*if (!GetComputerName(hostname, &length))*/
	    if (!GetComputerNameEx(ComputerNameDnsFullyQualified, hostname, &length)){
	        result = WSAGetLastError();
            /*
	        result = SMPDR_Err_create_code(SMPD_SUCCESS, SMPDR_ERR_RECOVERABLE, FCNAME, __LINE__, SMPDU_SOCK_ERR_FAIL, "**sock_gethost", "**sock_gethost %s %d", get_error_string(result), result);
	        smpd_exit_fn(FCNAME);
            */
            smpd_err_printf("Error retrieving NetBios/DNS name for socket, %s (%d)\n", get_err_string(result), result);
            smpd_exit_fn(FCNAME);
	        return result;
	    }
    }

    result = SMPDU_Sock_hostname_to_host_description(hostname, host_description, len);
    if (result != SMPD_SUCCESS){
        /*
	    result = SMPDR_Err_create_code(result, SMPDR_ERR_RECOVERABLE, FCNAME, __LINE__, SMPD_ERR_OTHER, "**fail", 0);
        */
        smpd_err_printf("Error retrieving host description for hostname\n");
        smpd_exit_fn(FCNAME);
        return result;
    }

    smpd_exit_fn(FCNAME);
    return result;
}

#undef FUNCNAME
#define FUNCNAME SMPDU_Sock_create_set
#undef FCNAME
#define FCNAME SMPDI_QUOTE(FUNCNAME)
int SMPDU_Sock_create_set(SMPDU_Sock_set_t * set)
{
    int result;
    HANDLE port;

    smpd_enter_fn(FCNAME);

    if (!g_init_called){
        smpd_err_printf("Attempting to use sock functions without initializing sock\n");
        smpd_exit_fn(FCNAME);
        return SMPD_FAIL;
    }
    port = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, g_num_cp_threads);
    if (port != NULL){
	    *set = port;
    }
    else{
        result = GetLastError();
        /*
        result = SMPDR_Err_create_code(SMPD_SUCCESS, SMPDR_ERR_RECOVERABLE, FCNAME, __LINE__, SMPDU_SOCK_ERR_FAIL, "**iocp", "**iocp %s %d", get_error_string(result), result);
        smpd_exit_fn(FCNAME);
        */
        smpd_err_printf("Error creating IO Completion port, %s (%d)\n", get_err_string(result), result);
        smpd_exit_fn(FCNAME);
        return result;
    }
    return SMPD_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME SMPDU_Sock_destroy_set
#undef FCNAME
#define FCNAME SMPDI_QUOTE(FUNCNAME)
int SMPDU_Sock_destroy_set(SMPDU_Sock_set_t set)
{
    int result;
    smpd_enter_fn(FCNAME);
    if (!g_init_called){
        smpd_err_printf("Attempting to use sock functions without initializing sock\n");
        smpd_exit_fn(FCNAME);
        return SMPD_FAIL;
    }
    if (!CloseHandle(set)){
	    result = GetLastError();
        smpd_err_printf("Error closing handle, %s (%d)\n", get_err_string(result), result);
        smpd_exit_fn(FCNAME);
    	return result;
    }
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME SMPDU_Sock_native_to_sock
#undef FCNAME
#define FCNAME SMPDI_QUOTE(FUNCNAME)
int SMPDU_Sock_native_to_sock(SMPDU_Sock_set_t set, SMPDU_SOCK_NATIVE_FD fd, void *user_ptr, SMPDU_Sock_t *sock_ptr)
{
    int result;
    /*int ret_val;*/
    sock_state_t *sock_state;
    /*u_long optval;*/

    smpd_enter_fn(FCNAME);

    if (!g_init_called){
        smpd_err_printf("Attempting to use sock functions without initializing sock\n");
        smpd_exit_fn(FCNAME);
        return SMPD_FAIL;
    }

    /* setup the structures */
    sock_state = (sock_state_t*)MPIU_Malloc(sizeof(sock_state_t));
    if (sock_state == NULL){
        smpd_err_printf("Error allocating memory for sock state, no memory available\n");
        smpd_exit_fn(FCNAME);
        return SMPD_FAIL;
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
    if (CreateIoCompletionPort((HANDLE)sock_state->sock, set, (ULONG_PTR)sock_state, g_num_cp_threads) == NULL){
        result = GetLastError();
        smpd_err_printf("Error associating sock with IO Completion port, %s (%d)\n", get_err_string(result), result);
        smpd_exit_fn(FCNAME);
        return result;
    }

    *sock_ptr = sock_state;

    /*printf("native socket %d\n", sock_state->sock);fflush(stdout);*/
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME SMPDU_Sock_listen
#undef FCNAME
#define FCNAME SMPDI_QUOTE(FUNCNAME)
int SMPDU_Sock_listen(SMPDU_Sock_set_t set, void * user_ptr, int * port, SMPDU_Sock_t * sock)
{
    int result;
    char host[100];
    sock_state_t * listen_state, **listener_copies;
    int i;

    smpd_enter_fn(FCNAME);

    if (!g_init_called){
        smpd_err_printf("Attempting to use sock functions without initializing sock\n");
        smpd_exit_fn(FCNAME);
        return SMPD_FAIL;    }

    listen_state = (sock_state_t*)MPIU_Malloc(sizeof(sock_state_t));
    init_state_struct(listen_state);
    result = easy_create_ranged(&listen_state->listen_sock, *port, INADDR_ANY);
    if (result != SMPD_SUCCESS){
        smpd_err_printf("Error creating listen sock \n");
        smpd_exit_fn(FCNAME);
	    return result;
    }
    if (listen(listen_state->listen_sock, SOMAXCONN) == SOCKET_ERROR){
	    result = WSAGetLastError();
        smpd_err_printf("Error setting socket to listen state, %s (%d)\n", get_err_string(result), result);
        smpd_exit_fn(FCNAME);
	    return result;
    }
    if (CreateIoCompletionPort((HANDLE)listen_state->listen_sock, set, (ULONG_PTR)listen_state, g_num_cp_threads) == NULL){
        result = GetLastError();
        smpd_err_printf("Error associating sock with IO Completion port, %s (%d)\n", get_err_string(result), result);
        smpd_exit_fn(FCNAME);
        return result;
    }

    easy_get_sock_info(listen_state->listen_sock, host, port);
    listen_state->user_ptr = user_ptr;
    listen_state->type = SOCKI_LISTENER;
    listen_state->set = set;

    /* post the accept(s) last to make sure the listener state structure is completely initialized before
       a completion thread has the chance to satisfy the AcceptEx call */

    listener_copies = (sock_state_t**)MPIU_Malloc(g_num_posted_accepts * sizeof(sock_state_t*));
    for (i=0; i<g_num_posted_accepts; i++){
	    listener_copies[i] = (sock_state_t*)MPIU_Malloc(sizeof(sock_state_t));
	    memcpy(listener_copies[i], listen_state, sizeof(*listen_state));
	    if (i > 0){
	        listener_copies[i]->next = listener_copies[i-1];
	    }
    	result = post_next_accept(listener_copies[i]);
	    if (result != SMPD_SUCCESS){
	        MPIU_Free(listener_copies);
            smpd_err_printf("Posting next accept on socket failed\n");
            smpd_exit_fn(FCNAME);
	        return result;
	    }
    }

    listen_state->list = listener_copies[g_num_posted_accepts-1];
    MPIU_Free(listener_copies);

    *sock = listen_state;
    /*printf("listening socket %d\n", listen_state->listen_sock);fflush(stdout);*/
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME SMPDU_Sock_accept
#undef FCNAME
#define FCNAME SMPDI_QUOTE(FUNCNAME)
int SMPDU_Sock_accept(SMPDU_Sock_t listener_sock, SMPDU_Sock_set_t set, void * user_ptr, SMPDU_Sock_t * sock)
{
    int result;
    BOOL b;
    /*struct linger linger;*/
    u_long optval;
    int len;
    sock_state_t *accept_state, *iter;

    smpd_enter_fn(FCNAME);
    if (!g_init_called){
        smpd_err_printf("Attempting to use sock functions without initializing sock\n");
        smpd_exit_fn(FCNAME);
        return SMPD_FAIL;
    }

    accept_state = MPIU_Malloc(sizeof(sock_state_t));
    if (accept_state == NULL){
	    *sock = NULL;
        smpd_err_printf("Error allocating memory for sock state, no memory available\n");
        smpd_exit_fn(FCNAME);
        return SMPD_FAIL;
    }
    init_state_struct(accept_state);

    accept_state->type = SOCKI_SOCKET;

    /* find the listener copy that satisfied the acceptex call and post another accept */
    iter = listener_sock->list;
    while (iter != NULL && iter->accepted == 0)
	iter = iter->next;
    if (iter == NULL){
        smpd_err_printf("Error finding the listener copy that satisfied accept\n");
        smpd_exit_fn(FCNAME);
        return SMPD_FAIL;
    }
    accept_state->sock = iter->sock;
    result = post_next_accept(iter);
    if (result != SMPD_SUCCESS){
	    *sock = NULL;
        smpd_err_printf("Error posting next accept\n");
        smpd_exit_fn(FCNAME);
        return SMPD_FAIL;
    }

    /* finish the accept */
    
    if(setsockopt(accept_state->sock, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, 
        (char *)&listener_sock->listen_sock, sizeof(listener_sock->listen_sock)) 
        == SOCKET_ERROR){
        result = WSAGetLastError();
        smpd_dbg_printf("Error updating accept context, %s (%d)\n", get_err_string(result), result);
        /* Don't fail on error when updating SO_UPDATE_ACCEPT_CONTEXT */
    }

    /* set the socket to non-blocking */
    optval = TRUE;
    if(ioctlsocket(accept_state->sock, FIONBIO, &optval) == SOCKET_ERROR){
        result = WSAGetLastError();
        smpd_dbg_printf("Error setting socket to non-blocking, %s (%d)\n", get_err_string(result), result);
        /* Don't fail on error when setting  FIONBIO*/
    }

    /* set the linger option */
    /*
    linger.l_onoff = 1;
    linger.l_linger = 60;
    setsockopt(accept_state->sock, SOL_SOCKET, SO_LINGER, (char*)&linger, sizeof(linger));
    */

    /* set the socket buffers */
    len = sizeof(int);
    if (!getsockopt(accept_state->sock, SOL_SOCKET, SO_RCVBUF, (char*)&optval, &len)){
	    optval = g_socket_rbuffer_size;
	    if(setsockopt(accept_state->sock, SOL_SOCKET, SO_RCVBUF, (char*)&optval, sizeof(int))
                == SOCKET_ERROR){
            result = WSAGetLastError();
            smpd_dbg_printf("Error setting SO_RCVBUF on socket, %s (%d)\n", get_err_string(result), result);
            /* Don't fail on error when setting  SO_RCVBUF */
       }
    }
    len = sizeof(int);
    if (!getsockopt(accept_state->sock, SOL_SOCKET, SO_SNDBUF, (char*)&optval, &len)){
	    optval = g_socket_sbuffer_size;
	    if(setsockopt(accept_state->sock, SOL_SOCKET, SO_SNDBUF, (char*)&optval, sizeof(int))
            == SOCKET_ERROR){
            result = WSAGetLastError();
            smpd_dbg_printf("Error setting SO_SNDBUF on socket, %s (%d)\n", get_err_string(result), result);
            /* Don't fail on error when setting  SO_SNDBUF */
        }
    }

    /* set the no-delay option */
    b = TRUE;
    if(setsockopt(accept_state->sock, IPPROTO_TCP, TCP_NODELAY, (char*)&b, sizeof(BOOL))
        == SOCKET_ERROR){
        result = WSAGetLastError();
        smpd_dbg_printf("Error setting TCP_NODELAY on socket, %s (%d)\n", get_err_string(result), result);
        /* Don't fail on error when setting  TCP_NODELAY */
    }

    /* prevent the socket from being inherited by child processes */
    /* FIXME: Use WSADuplicateSocket() instead */
    if(!DuplicateHandle(
	        GetCurrentProcess(), (HANDLE)accept_state->sock,
	        GetCurrentProcess(), (HANDLE*)&accept_state->sock,
	        0, FALSE, DUPLICATE_CLOSE_SOURCE | DUPLICATE_SAME_ACCESS)){
        result = WSAGetLastError();
        smpd_err_printf("Error duplicating handle to socket, %s (%d)\n", get_err_string(result), result);
        return result;
    }

    /* associate the socket with the completion port */
    if (CreateIoCompletionPort((HANDLE)accept_state->sock, set, (ULONG_PTR)accept_state, g_num_cp_threads) == NULL){
        result = GetLastError();
        smpd_err_printf("Error associating sock with IO Completion port, %s (%d)\n", get_err_string(result), result);
        smpd_exit_fn(FCNAME);
        return result;
    }

    accept_state->user_ptr = user_ptr;
    accept_state->set = set;
    *sock = accept_state;

    /*printf("accepted socket %d\n", accept_state->sock);fflush(stdout);*/
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

static unsigned int GetIP(char *pszIP)
{
    unsigned int nIP;
    unsigned int a,b,c,d;
    if (pszIP == NULL){
	    return 0;
    }
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

    if (pszMask == NULL){
	    return 0;
    }

    if (strstr(pszMask, ".")){
	    sscanf(pszMask, "%u.%u.%u.%u", &a, &b, &c, &d);
	    /*printf("mask: %u.%u.%u.%u\n", a, b, c, d);fflush(stdout);*/
	    nMask = (d << 24) | (c << 16) | (b << 8) | a;
    }
    else{
	    nBits = atoi(pszMask);
	    for (i=0; i<nBits; i++){
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
#define FUNCNAME SMPDU_Sock_post_connect
#undef FCNAME
#define FCNAME SMPDI_QUOTE(FUNCNAME)
int SMPDU_Sock_post_connect(SMPDU_Sock_set_t set, void * user_ptr, char * host_description, int port, SMPDU_Sock_t * sock)
{
    int result;
    struct hostent *lphost;
    struct sockaddr_in sockAddr;
    sock_state_t *connect_state;
    u_long optval;
    char host[100];
    int i;
    int connected = 0;
    int connect_errno = SMPD_SUCCESS;
    char pszNetMask[50];
    char *pEnv, *token;
    unsigned int nNicNet=0, nNicMask=0;
    int use_subnet = 0;

    smpd_enter_fn(FCNAME);
    if (!g_init_called){
        smpd_err_printf("Attempting to use sock functions without initializing sock\n");
        smpd_exit_fn(FCNAME);
        return SMPD_FAIL;
    }
    if (strlen(host_description) > SOCKI_DESCRIPTION_LENGTH){
        smpd_err_printf("Host description length is greater than max length available (%d)\n",SOCKI_DESCRIPTION_LENGTH);
        smpd_exit_fn(FCNAME);
        return SMPD_FAIL;
    }

    memset(&sockAddr,0,sizeof(sockAddr));

    /* setup the structures */
    connect_state = (sock_state_t*)MPIU_Malloc(sizeof(sock_state_t));
    init_state_struct(connect_state);
    connect_state->cur_host = connect_state->host_description;
    MPIU_Strncpy(connect_state->host_description, host_description, SOCKI_DESCRIPTION_LENGTH);

    /* create a socket */
    result = easy_create(&connect_state->sock, ADDR_ANY, INADDR_ANY);
    if (result != SMPD_SUCCESS){
        smpd_err_printf("Error creating a socket\n");
        smpd_exit_fn(FCNAME);
        return result;
    }

    /* check to see if a subnet was specified through the environment */
    pEnv = getenv("SMPD_NETMASK");
    if (pEnv != NULL){
	    MPIU_Strncpy(pszNetMask, pEnv, 50);
	    token = strtok(pszNetMask, "/");
	    if (token != NULL){
	        token = strtok(NULL, "\n");
	        if (token != NULL){
		        nNicNet = GetIP(pszNetMask);
		        nNicMask = GetMask(token);
		        use_subnet = 1;
	        }
	    }
    }

    while (!connected){
	    host[0] = '\0';
	    result = MPIU_Str_get_string(&connect_state->cur_host, host, 100);
	    /*printf("got <%s> out of <%s>\n", host, connect_state->host_description);fflush(stdout);*/
	    if (result != MPIU_STR_SUCCESS){
            smpd_err_printf("Error while retrieving hostname from host description\n");
            smpd_exit_fn(FCNAME);
            return SMPD_FAIL;
    	}
	    if (host[0] == '\0'){
	        smpd_err_printf("Connect on sock (host=%s, port=%d) failed, exhaused all end points\n", connect_state->host_description, port);
            smpd_exit_fn(FCNAME);
	        return SMPD_FAIL;
	    }

	    sockAddr.sin_family = AF_INET;
	    sockAddr.sin_addr.s_addr = inet_addr(host);

	    if (sockAddr.sin_addr.s_addr == INADDR_NONE || sockAddr.sin_addr.s_addr == 0){
	        lphost = gethostbyname(host);
	        if (lphost != NULL){
		        sockAddr.sin_addr.s_addr = ((struct in_addr *)lphost->h_addr)->s_addr;
            }
	        else{
		        result = WSAGetLastError();
                smpd_dbg_printf("Error while retrieving host information, %s (%d)\n", get_err_string(result), result);
                /*
		        smpd_exit_fn(FCNAME);
		        return result;
		        */
		        continue;
            }
	    }

	    /* if a subnet was specified, make sure the currently extracted ip falls in the subnet */
	    if (use_subnet){
	        if ((sockAddr.sin_addr.s_addr & nNicMask) != nNicNet){
		        /* this ip does not match, move to the next */
		        continue;
	        }
	    }

	    sockAddr.sin_port = htons((u_short)port);

	    /* connect */
	    for (i=0; i<5; i++){
	        /*printf("connecting to %s\n", host);fflush(stdout);*/
	        if (connect(connect_state->sock, (SOCKADDR*)&sockAddr, sizeof(sockAddr)) == SOCKET_ERROR){
		        int random_time;
		        int error = WSAGetLastError();
		        if ((error != WSAECONNREFUSED && error != WSAETIMEDOUT) || i == 4){
		            /*
                    connect_errno = SMPDR_Err_create_code(connect_errno, SMPDR_ERR_RECOVERABLE, FCNAME, __LINE__, SMPDU_SOCK_ERR_INIT, "**sock_connect", "**sock_connect %s %d %s %d", host, port, get_error_string(error), error);
                    */
                    connect_errno = SMPD_FAIL;
                    smpd_err_printf("Error while connecting to host, %s (%d)\n", get_err_string(error), error);
		            /*
		            smpd_exit_fn(FCNAME);
		            return result;
		            */
		            /* This code assumes that all errors other than WSAECONNREFUSED and WSAETIMEDOUT should not cause a connection retry */
		            /* FIXME: Is this correct for resource errors like WSAENOBUFS or an interrupted operation? */
		            /*        Should all errors cause a retry? or just WSAECONNREFUSED? or a subset of the possible errors? */
		            /*        The reason for not retrying on all errors is that it slows down connection time for multi-nic
		            /*        hosts that cannot be contacted on the first address listed. */
		            break;
		        }
		        /* Close the socket with an error and create a new one */
		        if (closesocket(connect_state->sock) == SOCKET_ERROR){
		            error = WSAGetLastError();
                    connect_errno = SMPD_FAIL;
                    smpd_err_printf("Error while closing socket, %s (%d)\n", get_err_string(error), error);
                    /*
		            connect_errno = SMPDR_Err_create_code(connect_errno, SMPDR_ERR_RECOVERABLE, FCNAME, __LINE__, SMPDU_SOCK_ERR_INIT, "**sock_connect", "**sock_connect %s %d %s %d", host, port, get_error_string(error), error);
                    */
		            /*
		            smpd_exit_fn(FCNAME);
		            return result;
		            */
		            break;
		        }
		        connect_state->sock = INVALID_SOCKET;
		        result = easy_create(&connect_state->sock, ADDR_ANY, INADDR_ANY);
		        if (result != SMPD_SUCCESS){
		            /* Warning: Loss of information.  We have two error stacks, one in connect_errno and the other in result, that cannot be joined given the current error code interface. */
		            /*connect_errno = SMPDR_Err_create_code(connect_errno, SMPDR_ERR_RECOVERABLE, FCNAME, __LINE__, SMPDU_SOCK_ERR_INIT, "**sock_create", 0);*/
                    /*
		            result = SMPDR_Err_create_code(result, SMPDR_ERR_RECOVERABLE, FCNAME, __LINE__, SMPDU_SOCK_ERR_INIT, "**sock_create", 0);
		            smpd_exit_fn(FCNAME);
                    */
                    smpd_err_printf("Error creating a socket\n");
                    smpd_exit_fn(FCNAME);
		            return result;
		        }
		        random_time = (int)((double)rand() / (double)RAND_MAX * 250.0);
		        Sleep(random_time);
	        }
	        else{
		        /*printf("connect to %s:%d succeeded.\n", host, port);fflush(stdout);*/
		        connected = 1;
		        break;
            }
    	} /* for(;;) - retrying connect() */
    } /* while (!connected) */

    /* set the socket to non-blocking */
    optval = TRUE;
    if(ioctlsocket(connect_state->sock, FIONBIO, &optval) == SOCKET_ERROR){
        result = WSAGetLastError();
        smpd_dbg_printf("Error setting socket to non-blocking, %s (%d)\n", get_err_string(result), result);
    }

    connect_state->user_ptr = user_ptr;
    connect_state->type = SOCKI_SOCKET;
    connect_state->state = SOCKI_CONNECTING;
    connect_state->set = set;

    /* associate the socket with the completion port */
    if (CreateIoCompletionPort((HANDLE)connect_state->sock, set, (ULONG_PTR)connect_state, g_num_cp_threads) == NULL){
        result = GetLastError();
        smpd_err_printf("Error associating sock with IO Completion port, %s (%d)\n", get_err_string(result), result);
        smpd_exit_fn(FCNAME);
        return result;
    }

    connect_state->pending_operations++;

    /* post a completion event so the sock_post_connect can be notified through sock_wait */
    if(!PostQueuedCompletionStatus(set, 0, (ULONG_PTR)connect_state, &connect_state->write.ovl)){
        result = GetLastError();
        smpd_err_printf("Error posting an IO Completion packet on IOCP, %s (%d)\n", get_err_string(result), result);
        smpd_exit_fn(FCNAME);
        return result;
    }

    *sock = connect_state;

    /*printf("connected socket %d\n", connect_state->sock);fflush(stdout);*/
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME SMPDU_Sock_set_user_ptr
#undef FCNAME
#define FCNAME SMPDI_QUOTE(FUNCNAME)
int SMPDU_Sock_set_user_ptr(SMPDU_Sock_t sock, void * user_ptr)
{
    smpd_enter_fn(FCNAME);
    if (!g_init_called){
        smpd_err_printf("Attempting to use sock functions without initializing sock\n");
        smpd_exit_fn(FCNAME);
        return SMPD_FAIL;
    }
    if (sock == NULL){
        smpd_err_printf("Error while setting user ptr for sock, invalid sock\n");
        smpd_exit_fn(FCNAME);
        return SMPD_FAIL;
    }
    sock->user_ptr = user_ptr;
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME SMPDU_Sock_post_close
#undef FCNAME
#define FCNAME SMPDI_QUOTE(FUNCNAME)
int SMPDU_Sock_post_close(SMPDU_Sock_t sock){
    int result;
    SOCKET s, *sp;

    smpd_enter_fn(FCNAME);
    if (!g_init_called){
        smpd_err_printf("Attempting to use sock functions without initializing sock\n");
        smpd_exit_fn(FCNAME);
        return SMPD_FAIL;
    }

    if (sock == NULL){
        smpd_err_printf("Error posting a close on sock, invalid sock\n");
        smpd_exit_fn(FCNAME);
        return SMPD_FAIL;
    }
    if (sock->closing){
        smpd_err_printf("Posting close on a socket in closing state\n");
        smpd_exit_fn(FCNAME);
        return SMPD_FAIL;
    }

    if (sock->type == SOCKI_LISTENER){
	    s = sock->listen_sock;
	    sp = &sock->listen_sock;
    }
    else{
	    s = sock->sock;
	    sp = &sock->sock;
    }

    if(s == INVALID_SOCKET){
	    if (sock->type == SOCKI_LISTENER){
            smpd_err_printf("Error posting close on socket, Invalid listener socket\n");
        }
	    else{
            smpd_err_printf("Error posting close on socket, Invalid socket\n");
        }
        smpd_exit_fn(FCNAME);
        return SMPD_FAIL;
    }

    if (sock->pending_operations != 0){
	    /*SMPDU_Assert(sock->state != 0);*/ /* The state can be 0 if the operation was aborted */
#ifdef MPICH_DBG_OUTPUT
	    if (sock->state & SOCKI_CONNECTING){
	        smpd_dbg_printf("sock_post_close(%d) called while sock is connecting.\n", SMPDU_Sock_get_sock_id(sock));
        }
	    if (sock->state & SOCKI_READING){
	        int i, n = 0;
	        for (i=0; i<sock->read.iovlen; i++){
		        n += sock->read.iov[i].SMPD_IOV_LEN;
            }
	        smpd_dbg_printf("sock_post_close(%d) called while sock is reading: %d bytes out of %d, index %d, iovlen %d.\n",
    		    SMPDU_Sock_get_sock_id(sock), sock->read.total, n, sock->read.index, sock->read.iovlen);
	    }
	    if (sock->state & SOCKI_WRITING){
	        int i, n = 0;
	        for (i=0; i<sock->write.iovlen; i++){
		        n += sock->write.iov[i].SMPD_IOV_LEN;
            }
	        smpd_dbg_printf("sock_post_close(%d) called while sock is writing: %d bytes out of %d, index %d, iovlen %d.\n",
		        SMPDU_Sock_get_sock_id(sock), sock->write.total, n, sock->write.index, sock->write.iovlen);
	    }
	    fflush(stdout);
#endif
	    /*
	    smpd_exit_fn(FCNAME);
	    return SOCK_ERR_OP_IN_PROGRESS;
	    */
	    /* posting a close cancels all outstanding operations */
	    /* It would be nice to cancel the outstanding reads or writes and then close the socket after handling the cancelled operations */
	    /* But it cannot be done because CancelIo only cancels operations started by the current thread.  There is no way to cancel all operations. */
	    /*CancelIo(sock->sock);*/
    }

    sock->closing = TRUE;
    /*sock->ct1 = PMPI_Wtime();*/
    if (sock->type != SOCKI_LISTENER){ /* calling shutdown for a listening socket is not valid */
	    /* Mark the socket as non-writable */
	    if (shutdown(s, SD_SEND) == SOCKET_ERROR){
	        sock->pending_operations = 0;
	        /*printf("closing socket %d\n", s);fflush(stdout);*/
	        if (closesocket(s) == SOCKET_ERROR){
                result = WSAGetLastError();
                smpd_err_printf("Error closing socket, %s (%d)\n", get_err_string(result), result);
                smpd_exit_fn(FCNAME);
                return result;
	        }
	        *sp = INVALID_SOCKET;
	        if (!PostQueuedCompletionStatus(sock->set, 0, (ULONG_PTR)sock, NULL)){
                result = GetLastError();
                smpd_err_printf("Error posting an IO Completion packet on IOCP, %s (%d)\n", get_err_string(result), result);
                smpd_exit_fn(FCNAME);
                return result;
            }
            smpd_exit_fn(FCNAME);
	        return SMPD_SUCCESS;
	    }
    }
    /* Cancel any outstanding operations */
    sock->pending_operations = 0;

    if (sock->type == SOCKI_SOCKET){
	    static char ch;
	    result = SMPD_SUCCESS;
	    if (sock->state ^ SOCKI_READING){
	        /* If a read is not already posted, post a bogus one here. */
	        result = SMPDU_Sock_post_read(sock, &ch, 1, 1, NULL);
            /* FIXME: Why don't we check for errors during post ? */
	        /* ignore this posted read so wait will return an op_close */
	        sock->pending_operations = 0;
	    }
	    if (result == SMPD_SUCCESS){
            smpd_exit_fn(FCNAME);
	        return SMPD_SUCCESS;
	    }
    }

    if (sock->type != SOCKI_LISTENER){ /* calling shutdown for a listening socket is not valid */
	    /* Mark the socket as non-readable */
	    if (shutdown(s, SD_RECEIVE) == SOCKET_ERROR){
            result = WSAGetLastError();
            smpd_err_printf("Error during socket shutdown, %s (%d)\n", get_err_string(result), result);
            smpd_exit_fn(FCNAME);
            return result;
	    }
    }
    /* Close the socket and insert a completion status so wait will return an op_close */
    /*printf("closing socket %d\n", s);fflush(stdout);*/
    if (closesocket(s) == SOCKET_ERROR){
        result = WSAGetLastError();
        smpd_err_printf("Error closing socket, %s (%d)\n", get_err_string(result), result);
        smpd_exit_fn(FCNAME);
        return result;
    }
    *sp = INVALID_SOCKET;
    if (!PostQueuedCompletionStatus(sock->set, 0, (ULONG_PTR)sock, NULL)){
        result = GetLastError();
        smpd_err_printf("Error posting an IO Completion packet on IOCP, %s (%d)\n", get_err_string(result), result);
        smpd_exit_fn(FCNAME);
        return result;
    }
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME SMPDU_Sock_post_read
#undef FCNAME
#define FCNAME SMPDI_QUOTE(FUNCNAME)
int SMPDU_Sock_post_read(SMPDU_Sock_t sock, void * buf, SMPDU_Sock_size_t minbr, SMPDU_Sock_size_t maxbr,
                         SMPDU_Sock_progress_update_func_t fn)
{
    int result = SMPD_SUCCESS;
    smpd_enter_fn(FCNAME);
    SMPDU_UNREFERENCED_ARG(maxbr);
    sock->read.tiov.SMPD_IOV_BUF = (SMPD_IOV_BUF_CAST)buf;
    sock->read.tiov.SMPD_IOV_LEN = minbr;
    result = SMPDU_Sock_post_readv(sock, &sock->read.tiov, 1, fn);
    smpd_exit_fn(FCNAME);
    return result;
}

#undef FUNCNAME
#define FUNCNAME SMPDU_Sock_post_readv
#undef FCNAME
#define FCNAME SMPDI_QUOTE(FUNCNAME)
int SMPDU_Sock_post_readv(SMPDU_Sock_t sock, SMPD_IOV * iov, int iov_n, SMPDU_Sock_progress_update_func_t fn)
{
    int iter;
    int result = SMPD_SUCCESS;
    DWORD flags = 0;

    smpd_enter_fn(FCNAME);
    if (!g_init_called)
    {
        smpd_err_printf("Attempting to use sock functions without initializing sock\n");
        smpd_exit_fn(FCNAME);
        return SMPD_FAIL;
    }
    /*sock->rt1 = PMPI_Wtime();*/
    /* strip any trailing empty buffers */
    while (iov_n && iov[iov_n-1].SMPD_IOV_LEN == 0){
	    iov_n--;
    }
    sock->read.total = 0;
    sock->read.iov = iov;
    sock->read.iovlen = iov_n;
    sock->read.index = 0;
    sock->read.progress_update = fn;
    sock->state |= SOCKI_READING;

    for (iter=0; iter<10; iter++){
	    if ((result = WSARecv(sock->sock, sock->read.iov, iov_n, &sock->read.num_bytes, &flags, &sock->read.ovl, NULL))
                != SOCKET_ERROR){
	        break;
	    }

	    result = WSAGetLastError();
	    if (result == WSA_IO_PENDING){
	        result = SMPD_SUCCESS;
	        break;
	    }
	    if (result == WSAENOBUFS){
	        WSABUF tmp;
	        tmp.buf = sock->read.iov[0].buf;
	        tmp.len = sock->read.iov[0].len;
	        SMPDU_Assert(tmp.len > 0);
	        while (result == WSAENOBUFS){
		        /*printf("[%d] receiving %d bytes\n", __LINE__, tmp.len);fflush(stdout);*/
		        if ((result = WSARecv(sock->sock, &tmp, 1, &sock->read.num_bytes, &flags, &sock->read.ovl, NULL))
                    != SOCKET_ERROR){
		            result = SMPD_SUCCESS;
		            break;
		        }
		        result = WSAGetLastError();
		        if (result == WSA_IO_PENDING){
		            result = SMPD_SUCCESS;
		            break;
		        }
		        /*printf("[%d] reducing recv length from %d to %d\n", __LINE__, tmp.len, tmp.len / 2);fflush(stdout);*/
		        tmp.len = tmp.len / 2;
		        if (tmp.len == 0 && result == WSAENOBUFS){
		            break;
		        }
	        }
	        if (result == SMPD_SUCCESS){
		        break;
	        }
	    }
	    if (result != WSAEWOULDBLOCK){
            smpd_err_printf("Error posting readv, %s(%d)\n", get_err_string(result), result);
            break;
	    }
        /* FIXME: Wait on progress engine instead */
	    Sleep(200);
    }
    if (result == SMPD_SUCCESS){
	    sock->pending_operations++;
    }
    else{
	    sock->state &= ~SOCKI_READING;
    }
    smpd_exit_fn(FCNAME);
    return result;
}

#undef FUNCNAME
#define FUNCNAME SMPDU_Sock_post_write
#undef FCNAME
#define FCNAME SMPDI_QUOTE(FUNCNAME)
int SMPDU_Sock_post_write(SMPDU_Sock_t sock, void * buf, SMPDU_Sock_size_t min, SMPDU_Sock_size_t max, SMPDU_Sock_progress_update_func_t fn)
{
    int result = SMPD_SUCCESS;
    smpd_enter_fn(FCNAME);
    SMPDU_UNREFERENCED_ARG(max);
    sock->write.tiov.SMPD_IOV_BUF = (SMPD_IOV_BUF_CAST)buf;
    sock->write.tiov.SMPD_IOV_LEN = min;
    result = SMPDU_Sock_post_writev(sock, &sock->write.tiov, 1, fn);
    smpd_exit_fn(FCNAME);
    return result;
}

#undef FUNCNAME
#define FUNCNAME SMPDU_Sock_post_writev
#undef FCNAME
#define FCNAME SMPDI_QUOTE(FUNCNAME)
int SMPDU_Sock_post_writev(SMPDU_Sock_t sock, SMPD_IOV * iov, int iov_n, SMPDU_Sock_progress_update_func_t fn)
{
    int result = SMPD_SUCCESS;
    int iter;

    smpd_enter_fn(FCNAME);
    /*sock->wt1 = PMPI_Wtime();*/
    sock->write.total = 0;
    sock->write.iov = iov;
    sock->write.iovlen = iov_n;
    sock->write.index = 0;
    sock->write.progress_update = fn;
    sock->state |= SOCKI_WRITING;
    for (iter=0; iter<10; iter++){
	    if (WSASend(sock->sock, sock->write.iov, iov_n, &sock->write.num_bytes, 0, &sock->write.ovl, NULL)
            != SOCKET_ERROR){
	        break;
        }

	    result = WSAGetLastError();
	    if (result == WSA_IO_PENDING){
	        result = SMPD_SUCCESS;
	        break;
	    }
	    if (result == WSAENOBUFS){
	        WSABUF tmp;
	        tmp.buf = sock->write.iov[0].buf;
	        tmp.len = sock->write.iov[0].len;
	        while (result == WSAENOBUFS){
		        /*printf("[%d] sending %d bytes\n", __LINE__, tmp.len);fflush(stdout);*/
		        if (WSASend(sock->sock, &tmp, 1, &sock->write.num_bytes, 0, &sock->write.ovl, NULL) 
                    != SOCKET_ERROR){
		            result = SMPD_SUCCESS;
		            break;
		        }
		        result = WSAGetLastError();
		        if (result == WSA_IO_PENDING){
		            result = SMPD_SUCCESS;
		            break;
		        }
		        /*printf("[%d] reducing send length from %d to %d\n", __LINE__, tmp.len, tmp.len / 2);fflush(stdout);*/
		        tmp.len = tmp.len / 2;
		        if (tmp.len == 0){
                    smpd_err_printf("Error posting writev, %s(%d)\n", get_err_string(result), result);
		            break;
		        }
            }
	        if (result == SMPD_SUCCESS){
		        break;
	        }
	    }
	    if (result != WSAEWOULDBLOCK){
            smpd_err_printf("Error posting writev, %s(%d)\n", get_err_string(result), result);
            break;
	    }
	    Sleep(200);
    }
    if (result == SMPD_SUCCESS){
	    sock->pending_operations++;
    }
    else{
	    sock->state &= ~SOCKI_WRITING;
    }
    smpd_exit_fn(FCNAME);
    return result;
}

#undef FUNCNAME
#define FUNCNAME SMPDU_Sock_wait
#undef FCNAME
#define FCNAME SMPDI_QUOTE(FUNCNAME)
int SMPDU_Sock_wait(SMPDU_Sock_set_t set, int timeout, SMPDU_Sock_event_t * out)
{
    /*double t1, t2;*/
    int result;
    DWORD num_bytes;
    sock_state_t *sock, *iter;
    OVERLAPPED *ovl;
    DWORD dwFlags = 0;
    /* char error_msg[1024]; */

    smpd_enter_fn(FCNAME);

    for (;;){
	    /* initialize to NULL so we can compare the output of GetQueuedCompletionStatus */
	    sock = NULL;
	    ovl = NULL;
	    num_bytes = 0;
	    if (GetQueuedCompletionStatus(set, &num_bytes, (PULONG_PTR)&sock, &ovl, timeout)){
            /*
            printf("GQCS success\n");
            printf("sock->type = %d, sock->state = %d, sock->sock=%p, sock->closing=%d,sock->pendops=%d\n",
                    sock->type, sock->state, sock->sock, sock->closing, sock->pending_operations); fflush(stdout);
            */
    	    if (sock->type == SOCKI_SOCKET){
		        if (sock->closing && sock->pending_operations == 0){
        		    out->op_type = SMPDU_SOCK_OP_CLOSE;
		            out->num_bytes = 0;
		            out->error = SMPD_SUCCESS;
		            out->user_ptr = sock->user_ptr;
		            CloseHandle(sock->read.ovl.hEvent);
		            CloseHandle(sock->write.ovl.hEvent);
		            sock->read.ovl.hEvent = NULL;
		            sock->write.ovl.hEvent = NULL;
        		    if (sock->sock != INVALID_SOCKET){
			            /*printf("closing socket %d\n", sock->sock);fflush(stdout);*/
			            if (closesocket(sock->sock) == SOCKET_ERROR){
                            result = WSAGetLastError();
                            smpd_err_printf("Error closing socket, %s (%d)\n", get_err_string(result), result);
                            smpd_exit_fn(FCNAME);
                            return result;
			            }
			            sock->sock = INVALID_SOCKET;
		            }
		            smpd_exit_fn(FCNAME);
		            return SMPD_SUCCESS;
		        }
		        if (ovl == &sock->read.ovl){
		            if (num_bytes == 0){
            			out->op_type = SMPDU_SOCK_OP_READ;
            			out->num_bytes = sock->read.total;
			            out->error = /*SMPDU_SOCK_ERR_CONN_CLOSED*/ SMPD_FAIL;
			            out->user_ptr = sock->user_ptr;
			            sock->pending_operations--;
			            sock->state &= ~SOCKI_READING; /* remove the SOCKI_READING bit */
			            if (sock->closing && sock->pending_operations == 0){
            			    FlushFileBuffers((HANDLE)sock->sock);
			                if (shutdown(sock->sock, SD_BOTH) == SOCKET_ERROR){
                                result = WSAGetLastError();
                                smpd_err_printf("Error during socket shutdown, %s (%d)\n", get_err_string(result), result);
                                smpd_exit_fn(FCNAME);
                                return result;
			                }
            			    if (closesocket(sock->sock) == SOCKET_ERROR){
                                result = WSAGetLastError();
                                smpd_err_printf("Error closing socket, %s (%d)\n", get_err_string(result), result);
                                smpd_exit_fn(FCNAME);
                                return result;
			                }
			                sock->sock = INVALID_SOCKET;
			            }
			            smpd_exit_fn(FCNAME);
			            return SMPD_SUCCESS;
		            }
        		    sock->read.total += num_bytes;
		            while (num_bytes){
			            if (sock->read.iov[sock->read.index].SMPD_IOV_LEN <= num_bytes){
			                num_bytes -= sock->read.iov[sock->read.index].SMPD_IOV_LEN;
			                sock->read.index++;
			                sock->read.iovlen--;
			            }
			            else{
			                sock->read.iov[sock->read.index].SMPD_IOV_LEN -= num_bytes;
			                sock->read.iov[sock->read.index].SMPD_IOV_BUF = (SMPD_IOV_BUF_CAST)(
				            (char*)(sock->read.iov[sock->read.index].SMPD_IOV_BUF) + num_bytes);
			                num_bytes = 0;
			            }
		            }
		            if (sock->read.iovlen == 0){
            			out->op_type = SMPDU_SOCK_OP_READ;
			            out->num_bytes = sock->read.total;
			            out->error = SMPD_SUCCESS;
			            out->user_ptr = sock->user_ptr;
			            sock->pending_operations--;
			            sock->state &= ~SOCKI_READING; /* remove the SOCKI_READING bit */
			            if (sock->closing && sock->pending_operations == 0){
			                FlushFileBuffers((HANDLE)sock->sock);
            			    if (shutdown(sock->sock, SD_BOTH) == SOCKET_ERROR){
                                result = WSAGetLastError();
                                smpd_err_printf("Error during socket shutdown, %s (%d)\n", get_err_string(result), result);
                                smpd_exit_fn(FCNAME);
                                return result;
			                }
            			    if (closesocket(sock->sock) == SOCKET_ERROR){
                                result = WSAGetLastError();
                                smpd_err_printf("Error closing socket, %s (%d)\n", get_err_string(result), result);
                                smpd_exit_fn(FCNAME);
                                return result;
                            }
			                sock->sock = INVALID_SOCKET;
			            }
			            smpd_exit_fn(FCNAME);
			            return SMPD_SUCCESS;
		            }
        		    if (sock->read.progress_update != NULL){
			            sock->read.progress_update(num_bytes, sock->user_ptr);
                    }
        		    /* post a read of the remaining data */
		            if (WSARecv(sock->sock, &sock->read.iov[sock->read.index], sock->read.iovlen, &sock->read.num_bytes, &dwFlags, &sock->read.ovl, NULL)
                        == SOCKET_ERROR){
			            result = WSAGetLastError();
			            if (result == 0){
            			    out->op_type = SMPDU_SOCK_OP_READ;
			                out->num_bytes = sock->read.total;
            			    out->error = /*SMPDU_SOCK_ERR_CONN_CLOSED*/ SMPD_FAIL;
			                out->user_ptr = sock->user_ptr;
			                sock->pending_operations--;
			                sock->state &= ~SOCKI_READING;
			                if (sock->closing && sock->pending_operations == 0){
                				FlushFileBuffers((HANDLE)sock->sock);
				                if (shutdown(sock->sock, SD_BOTH) == SOCKET_ERROR){
                                    result = WSAGetLastError();
                                    smpd_err_printf("Error during socket shutdown, %s (%d)\n", get_err_string(result), result);
                                    smpd_exit_fn(FCNAME);
                                    return result;
				                }
				                if (closesocket(sock->sock) == SOCKET_ERROR){
                                    result = WSAGetLastError();
                                    smpd_err_printf("Error closing socket, %s (%d)\n", get_err_string(result), result);
                                    smpd_exit_fn(FCNAME);
                                    return result;
				                }
				                sock->sock = INVALID_SOCKET;
			                }
			                smpd_exit_fn(FCNAME);
			                return SMPD_SUCCESS;
			            }
			            if (result == WSAENOBUFS){
			                WSABUF tmp;
			                tmp.buf = sock->read.iov[sock->read.index].buf;
			                tmp.len = sock->read.iov[sock->read.index].len;
			                while (result == WSAENOBUFS){
                				if (WSARecv(sock->sock, &tmp, 1, &sock->read.num_bytes, &dwFlags, &sock->read.ovl, NULL) 
                                    != SOCKET_ERROR){
				                    result = WSA_IO_PENDING;
				                    break;
				                }
				                result = WSAGetLastError();
				                if (result == WSA_IO_PENDING){
				                    break;
				                }   
                				tmp.len = tmp.len / 2;
                				if (tmp.len == 0 && result == WSAENOBUFS){
				                    break;
				                }
			                }
			            }
			            if (result != WSA_IO_PENDING){
			                out->op_type = SMPDU_SOCK_OP_READ;
			                out->num_bytes = sock->read.total;
			                smpd_err_printf("Failed to read data on socket, %s (%d)\n", get_err_string(result), result);
			                out->user_ptr = sock->user_ptr;
			                sock->pending_operations--;
			                sock->state &= ~SOCKI_READING;
			                if (sock->closing && sock->pending_operations == 0){
                				FlushFileBuffers((HANDLE)sock->sock);
				                if (shutdown(sock->sock, SD_BOTH) == SOCKET_ERROR){
                                    result = WSAGetLastError();
                                    smpd_err_printf("Error during socket shutdown, %s (%d)\n", get_err_string(result), result);
                                    smpd_exit_fn(FCNAME);
                                    return result;
				                }
                				if (closesocket(sock->sock) == SOCKET_ERROR){
                                    result = WSAGetLastError();
                                    smpd_err_printf("Error closing socket, %s (%d)\n", get_err_string(result), result);
                                    smpd_exit_fn(FCNAME);
                                    return result;
				                }
				                sock->sock = INVALID_SOCKET;
			                }
			                smpd_exit_fn(FCNAME);
			                return SMPD_SUCCESS;
			            }
		            }
		        }
		        else if (ovl == &sock->write.ovl){
		            if (sock->state & SOCKI_CONNECTING){
			            /* insert code here to determine that the connect succeeded */
            			out->op_type = SMPDU_SOCK_OP_CONNECT;
			            out->num_bytes = 0;
			            out->error = SMPD_SUCCESS;
			            out->user_ptr = sock->user_ptr;
			            sock->pending_operations--;
			            sock->state ^= SOCKI_CONNECTING; /* remove the SOCKI_CONNECTING bit */
			            if (sock->closing && sock->pending_operations == 0){
            			    FlushFileBuffers((HANDLE)sock->sock);
			                if (shutdown(sock->sock, SD_BOTH) == SOCKET_ERROR){
                                result = WSAGetLastError();
                                smpd_err_printf("Error during socket shutdown, %s (%d)\n", get_err_string(result), result);
                                smpd_exit_fn(FCNAME);
                                return result;
			                }
            			    if (closesocket(sock->sock) == SOCKET_ERROR){
                                result = WSAGetLastError();
                                smpd_err_printf("Error closing socket, %s (%d)\n", get_err_string(result), result);
                                smpd_exit_fn(FCNAME);
                                return result;
			                }
			                sock->sock = INVALID_SOCKET;
			            }
			            smpd_exit_fn(FCNAME);
			            return SMPD_SUCCESS;
		            }
		            else{
			            if (num_bytes == 0){
            			    out->op_type = SMPDU_SOCK_OP_WRITE;
			                out->num_bytes = sock->write.total;
            			    out->error = /*SMPDU_SOCK_ERR_CONN_CLOSED*/ SMPD_FAIL;
			                out->user_ptr = sock->user_ptr;
			                sock->pending_operations--;
			                sock->state &= ~SOCKI_WRITING; /* remove the SOCKI_WRITING bit */
			                if (sock->closing && sock->pending_operations == 0){
                				FlushFileBuffers((HANDLE)sock->sock);
				                if (shutdown(sock->sock, SD_BOTH) == SOCKET_ERROR){
                                    result = WSAGetLastError();
                                    smpd_err_printf("Error during socket shutdown, %s (%d)\n", get_err_string(result), result);
                                    smpd_exit_fn(FCNAME);
                                    return result;
				                }
                				if (closesocket(sock->sock) == SOCKET_ERROR){
                                    result = WSAGetLastError();
                                    smpd_err_printf("Error closing socket, %s (%d)\n", get_err_string(result), result);
                                    smpd_exit_fn(FCNAME);
                                    return result;
				                }
				                sock->sock = INVALID_SOCKET;
			                }
			                smpd_exit_fn(FCNAME);
			                return SMPD_SUCCESS;
			            }
            			sock->write.total += num_bytes;
			            while (num_bytes){
			                if (sock->write.iov[sock->write.index].SMPD_IOV_LEN <= num_bytes){
                				num_bytes -= sock->write.iov[sock->write.index].SMPD_IOV_LEN;
				                sock->write.index++;
				                sock->write.iovlen--;
			                }
			                else{
                				sock->write.iov[sock->write.index].SMPD_IOV_LEN -= num_bytes;
				                sock->write.iov[sock->write.index].SMPD_IOV_BUF = (SMPD_IOV_BUF_CAST)(
				                (char*)(sock->write.iov[sock->write.index].SMPD_IOV_BUF) + num_bytes);
				                num_bytes = 0;
			                }
			            }
			            if (sock->write.iovlen == 0){
			                out->op_type = SMPDU_SOCK_OP_WRITE;
			                out->num_bytes = sock->write.total;
			                out->error = SMPD_SUCCESS;
			                out->user_ptr = sock->user_ptr;
			                sock->pending_operations--;
			                sock->state &= ~SOCKI_WRITING; /* remove the SOCKI_WRITING bit */
			                if (sock->closing && sock->pending_operations == 0){
				                FlushFileBuffers((HANDLE)sock->sock);
				                if (shutdown(sock->sock, SD_BOTH) == SOCKET_ERROR){
                                    result = WSAGetLastError();
                                    smpd_err_printf("Error during socket shutdown, %s (%d)\n", get_err_string(result), result);
                                    smpd_exit_fn(FCNAME);
                                    return result;
				                }
                				if (closesocket(sock->sock) == SOCKET_ERROR){
                                    result = WSAGetLastError();
                                    smpd_err_printf("Error closing socket, %s (%d)\n", get_err_string(result), result);
                                    smpd_exit_fn(FCNAME);
                                    return result;
				                }
				                sock->sock = INVALID_SOCKET;
			                }
			                smpd_exit_fn(FCNAME);
			                return SMPD_SUCCESS;
                        }
			            /* make the user upcall */
			            if (sock->write.progress_update != NULL){
			                sock->write.progress_update(num_bytes, sock->user_ptr);
                        }
            			if (WSASend(sock->sock, sock->write.iov, sock->write.iovlen, &sock->write.num_bytes, 0, &sock->write.ovl, NULL)
                                == SOCKET_ERROR){
			                result = WSAGetLastError();
			                if (result == 0){
				                out->op_type = SMPDU_SOCK_OP_WRITE;
				                out->num_bytes = sock->write.total;
				                /*printf("sock_wait returning %d bytes and socket closed\n", sock->write.total);*/
				                out->error = /*SMPDU_SOCK_ERR_CONN_CLOSED*/ SMPD_FAIL;
				                out->user_ptr = sock->user_ptr;
				                sock->pending_operations--;
				                sock->state &= ~SOCKI_WRITING;
				                if (sock->closing && sock->pending_operations == 0){
					                FlushFileBuffers((HANDLE)sock->sock);
					                if (shutdown(sock->sock, SD_BOTH) == SOCKET_ERROR){
                                        result = WSAGetLastError();
                                        smpd_err_printf("Error during socket shutdown, %s (%d)\n", get_err_string(result), result);
                                        smpd_exit_fn(FCNAME);
                                        return result;
									}
					                /*printf("closing socket %d\n", sock->sock);fflush(stdout);*/
					                if (closesocket(sock->sock) == SOCKET_ERROR){
                                        result = WSAGetLastError();
                                        smpd_err_printf("Error closing socket, %s (%d)\n", get_err_string(result), result);
                                        smpd_exit_fn(FCNAME);
                                        return result;
					                }
					                sock->sock = INVALID_SOCKET;
				                }
				                smpd_exit_fn(FCNAME);
				                return SMPD_SUCCESS;
			                }
				            if (result == WSAENOBUFS){
							    WSABUF tmp;
							    tmp.buf = sock->write.iov[0].buf;
							    tmp.len = sock->write.iov[0].len;
							    while (result == WSAENOBUFS){
                				    if (WSASend(sock->sock, &tmp, 1, &sock->write.num_bytes, 0, &sock->write.ovl, NULL)
                                            != SOCKET_ERROR){
                    					/* FIXME: does this data need to be handled immediately? */
                    					result = WSA_IO_PENDING;
					                    break;
				                    }
				                    result = WSAGetLastError();
				                    if (result == WSA_IO_PENDING){
					                    break;
				                    }
                				    tmp.len = tmp.len / 2;
				                }
			                }
			                if (result != WSA_IO_PENDING){
				                out->op_type = SMPDU_SOCK_OP_WRITE;
				                out->num_bytes = sock->write.total;
				                out->error = SMPD_FAIL;
				                out->user_ptr = sock->user_ptr;
				                sock->pending_operations--;
				                sock->state &= ~SOCKI_WRITING;
				                if (sock->closing && sock->pending_operations == 0){
					                FlushFileBuffers((HANDLE)sock->sock);
					                if (shutdown(sock->sock, SD_BOTH) == SOCKET_ERROR){
                                        result = WSAGetLastError();
                                        smpd_err_printf("Error during socket shutdown, %s (%d)\n", get_err_string(result), result);
                                        smpd_exit_fn(FCNAME);
                                        return result;
									}
					                /*printf("closing socket %d\n", sock->sock);fflush(stdout);*/
					                if (closesocket(sock->sock) == SOCKET_ERROR){
                                        result = WSAGetLastError();
                                        smpd_err_printf("Error closing socket, %s (%d)\n", get_err_string(result), result);
                                        smpd_exit_fn(FCNAME);
                                        return result;
					                }
					                sock->sock = INVALID_SOCKET;
				                }
				                smpd_exit_fn(FCNAME);
				                return SMPD_SUCCESS;
            			    }
			            }
		            }
		        }
		        else{
		            if (num_bytes == 0){
			            if ((sock->state & SOCKI_READING)){/* && sock-sock != INVALID_SOCKET) */
			                out->op_type = SMPDU_SOCK_OP_READ;
			                out->num_bytes = sock->read.total;
			                out->error = /*SMPDU_SOCK_ERR_CONN_CLOSED*/ SMPD_FAIL;
			                out->user_ptr = sock->user_ptr;
			                sock->state &= ~SOCKI_READING;
			                sock->pending_operations--;
			                smpd_exit_fn(FCNAME);
			                return SMPD_SUCCESS;
    			        }
	            		if ((sock->state & SOCKI_WRITING)){/* && sock->sock != INVALID_SOCKET) */
			                out->op_type = SMPDU_SOCK_OP_WRITE;
			                out->num_bytes = sock->write.total;
			                out->error = /*SMPDU_SOCK_ERR_CONN_CLOSED*/ SMPD_FAIL;
			                out->user_ptr = sock->user_ptr;
			                sock->state &= ~SOCKI_WRITING;
			                sock->pending_operations--;
			                smpd_exit_fn(FCNAME);
			                return SMPD_SUCCESS;
			            }
        		    }
		            if (sock->sock != INVALID_SOCKET){
			            smpd_err_printf("In sock_wait(), returned overlapped structure does not match the current read or write ovl: 0x%p\n", ovl);
			            smpd_exit_fn(FCNAME);
                        return SMPD_FAIL;
                    }
        		}
            }
	        else if (sock->type == SOCKI_WAKEUP){
		        out->op_type = SMPDU_SOCK_OP_WAKEUP;
		        out->num_bytes = 0;
		        out->error = SMPD_SUCCESS;
		        out->user_ptr = NULL;
		        smpd_exit_fn(FCNAME);
		        return SMPD_SUCCESS;
	        }
    	    else if (sock->type == SOCKI_LISTENER){
		        if (sock->closing && sock->pending_operations == 0){
		            if (!sock->listener_closed){
			            sock->listener_closed = 1;
			            out->op_type = SMPDU_SOCK_OP_CLOSE;
			            out->num_bytes = 0;
			            out->error = SMPD_SUCCESS;
			            out->user_ptr = sock->user_ptr;
			            CloseHandle(sock->read.ovl.hEvent);
			            CloseHandle(sock->write.ovl.hEvent);
			            sock->read.ovl.hEvent = NULL;
			            sock->write.ovl.hEvent = NULL;
			            smpd_exit_fn(FCNAME);
			            return SMPD_SUCCESS;
		            }
        		    else{
            			/* ignore multiple close operations caused by the outstanding accept operations */
			            continue;
		            }
		        }
		        iter = sock->list;
		        while (iter && &iter->read.ovl != ovl){
		            iter = iter->next;
                }
		        if (iter != NULL){
		            iter->accepted = 1;
                }
		        out->op_type = SMPDU_SOCK_OP_ACCEPT;
		        out->num_bytes = num_bytes;
		        out->error = SMPD_SUCCESS;
		        out->user_ptr = sock->user_ptr;
		        smpd_exit_fn(FCNAME);
		        return SMPD_SUCCESS;
	        }
	        else{
		        smpd_err_printf("sock type is not a SOCKET or a LISTENER, it's %d\n", sock->type);
		        smpd_exit_fn(FCNAME);
		        return SMPD_FAIL;
    	    }
	    }
	    else{
	        result = GetLastError();
            /*
            printf("GQCS failed, error = %d\n", result); fflush(stdout);
            if(sock != NULL){
                printf("sock->type = %d, sock->state = %d, sock->sock=%p, sock->closing=%d,sock->pendops=%d\n",
                        sock->type, sock->state, sock->sock, sock->closing, sock->pending_operations); fflush(stdout);
            }
            else{
                printf("sock is null\n"); fflush(stdout);
            }
            */
	        if (result == WAIT_TIMEOUT){
		        smpd_exit_fn(FCNAME);
		        return SMPD_FAIL;
	        }
            if (result == ERROR_OPERATION_ABORTED){
        		/* A thread exited causing GetQueuedCompletionStatus to return prematurely. */
		        if (sock != NULL && !sock->closing){
				    /* re-post the aborted operation */
		            if (sock->type == SOCKI_SOCKET){
			            if (ovl == &sock->read.ovl){
			                /*printf("repost readv of length %d\n", sock->read.iovlen);fflush(stdout);*/
			                result = SMPDU_Sock_post_readv(sock, sock->read.iov, sock->read.iovlen, NULL);
			                if (result != SMPD_SUCCESS){
				                /*sock->rt2 = PMPI_Wtime();*/
				                /*printf("[%d] time from post_read  to op_read : %.3f - sock %d\n", getpid(), sock->rt2 - sock->rt1, sock->sock);*/
				                out->op_type = SMPDU_SOCK_OP_READ;
				                out->num_bytes = 0;
				                out->error = SMPD_FAIL;
				                out->user_ptr = sock->user_ptr;
				                sock->state &= ~SOCKI_READING; /* remove the SOCKI_READING bit */
				                smpd_exit_fn(FCNAME);
				                return SMPD_SUCCESS;
				            }
			            }
			            else if (ovl == &sock->write.ovl){
				            /*printf("repost writev of length %d\n", sock->write.iovlen);fflush(stdout);*/
				            result = SMPDU_Sock_post_writev(sock, sock->write.iov, sock->write.iovlen, NULL);
				            if (result != SMPD_SUCCESS){
							    /*sock->wt2 = PMPI_Wtime();*/
							    /*printf("[%d] time from post_write to op_write: %.3f - sock %d\n", getpid(), sock->wt2 - sock->wt1, sock->sock);*/
							    out->op_type = SMPDU_SOCK_OP_WRITE;
							    out->num_bytes = 0;
							    out->error = SMPD_FAIL;
							    out->user_ptr = sock->user_ptr;
				                sock->state &= ~SOCKI_WRITING; /* remove the SOCKI_WRITING bit */
				                smpd_exit_fn(FCNAME);
				                return SMPD_SUCCESS;
			                }
			            }
			            else{
				            /*printf("aborted sock operation\n");fflush(stdout);*/
				            out->op_type = -1;
				            out->num_bytes = 0;
				            out->error = SMPD_FAIL;
				            out->user_ptr = sock->user_ptr;
				            smpd_exit_fn(FCNAME);
				            return SMPD_SUCCESS;
			            }
		            }
		            else if (sock->type == SOCKI_WAKEUP){
			            /*printf("aborted wakeup operation\n");fflush(stdout);*/
			            out->op_type = SMPDU_SOCK_OP_WAKEUP;
			            out->num_bytes = 0;
			            out->error = SMPD_FAIL;
			            out->user_ptr = sock->user_ptr;
			            smpd_exit_fn(FCNAME);
			            return SMPD_SUCCESS;
		            }
        		    else if (sock->type == SOCKI_LISTENER){
					    /*printf("aborted listener operation\n");fflush(stdout);*/
					    iter = sock->list;
					    while (iter && &iter->read.ovl != ovl){
						    iter = iter->next;
                        }
					    if (iter != NULL){
						    result = post_next_accept(iter);
						    if (result != SMPD_SUCCESS){
							    out->op_type = SMPDU_SOCK_OP_ACCEPT;
							    out->num_bytes = 0;
							    out->error = SMPD_FAIL;
							    out->user_ptr = sock->user_ptr;
							    smpd_exit_fn(FCNAME);
							    return SMPD_SUCCESS;
						    }
					    }
					    else{
				            /*printf("aborted unknown listener socket operation\n");fflush(stdout);*/
				            out->op_type = -1;
				            out->num_bytes = 0;
				            out->error = SMPD_FAIL;
				            out->user_ptr = sock->user_ptr;
				            smpd_exit_fn(FCNAME);
				            return SMPD_SUCCESS;
			            }
		            }
		            else{
			            /*printf("aborted unknown socket operation\n");fflush(stdout);*/
			            out->op_type = -1;
			            out->num_bytes = 0;
			            out->error = SMPD_FAIL;
			            out->user_ptr = sock->user_ptr;
			            smpd_exit_fn(FCNAME);
			            return SMPD_SUCCESS;
		            }
		            continue;
		        }
            }
	        smpd_dbg_printf("GetQueuedCompletionStatus failed, GetLastError: %d\n", result);
	        if (sock != NULL){
        		if (sock->type == SOCKI_SOCKET){
		            if (sock->closing){
			            out->op_type = SMPDU_SOCK_OP_CLOSE;
			            if (sock->sock != INVALID_SOCKET){
			                /*("closing socket %d\n", sock->sock);fflush(stdout);*/
			                if (closesocket(sock->sock) == SOCKET_ERROR){
                                result = WSAGetLastError();
                                smpd_err_printf("Error closing socket, %s (%d)\n", get_err_string(result), result);
                                smpd_exit_fn(FCNAME);
                                return result;
			                }
			                sock->sock = INVALID_SOCKET;
    		            }
    	            }
		            else{
			            if (ovl == &sock->read.ovl){
			                out->op_type = SMPDU_SOCK_OP_READ;
			                sock->state &= ~SOCKI_READING; /* remove the SOCKI_READING bit */
			            }
			            else if (ovl == &sock->write.ovl){
			                out->op_type = SMPDU_SOCK_OP_WRITE;
			                sock->state &= ~SOCKI_WRITING; /* remove the SOCKI_WRITING bit */
			            }
			            else{
			                /* Shoud we reset the state bits? */
			                sock->state &= ~SOCKI_READING; /* remove the SOCKI_READING bit */
			                sock->state &= ~SOCKI_WRITING; /* remove the SOCKI_WRITING bit */
			                out->op_type = -1;
			            }
        		    }
		            out->num_bytes = 0;
		            out->error = SMPD_FAIL;
		            out->user_ptr = sock->user_ptr;
		            smpd_exit_fn(FCNAME);
		            return SMPD_SUCCESS;
        		}
		        if (sock->type == SOCKI_LISTENER){
		            /* this only works if the aborted operation is reported before the close is reported
		               because the close will free the sock structure causing these dereferences bogus.
		               I need to reference count the sock */
		            if (sock->closing){
			            if (sock->listener_closed){
			                /* only return a close_op once for the main listener, not any of the copies */
			                continue;
			            }
			            else{
			                sock->listener_closed = 1;
			                out->op_type = SMPDU_SOCK_OP_CLOSE;
			                out->num_bytes = 0;
			                if (result == ERROR_OPERATION_ABORTED){
				                out->error = SMPD_SUCCESS;
                            }
			                else{
				                out->error = SMPD_FAIL;
                            }
			                out->user_ptr = sock->user_ptr;
			                smpd_exit_fn(FCNAME);
			                return SMPD_SUCCESS;
			            }
		            }
		            else{
			            /* Should we return a SOCK_OP_ACCEPT with an error if there is a failure on the listener? */
			            out->op_type = SMPDU_SOCK_OP_ACCEPT;
			            out->num_bytes = 0;
			            out->error = SMPD_FAIL;
			            out->user_ptr = sock->user_ptr;
			            smpd_exit_fn(FCNAME);
			            return SMPD_SUCCESS;
        		    }
		        }
	        }
	        smpd_exit_fn(FCNAME);
	        return SMPD_FAIL;
	    }
    }
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME SMPDU_Sock_wakeup
#undef FCNAME
#define FCNAME SMPDI_QUOTE(FUNCNAME)
int SMPDU_Sock_wakeup(SMPDU_Sock_set_t set)
{
    int result;
    /* post a completion event to wake up sock_wait */
    if(!PostQueuedCompletionStatus(set, 0, (ULONG_PTR)&g_wakeup_state, &g_wakeup_state.write.ovl)){
        result = GetLastError();
        smpd_dbg_printf("Error posting an IO Completion packet to wakeup sock, %s (%d)\n", get_err_string(result), result);
    }
    return SMPD_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME SMPDU_Sock_read
#undef FCNAME
#define FCNAME SMPDI_QUOTE(FUNCNAME)
int SMPDU_Sock_read(SMPDU_Sock_t sock, void * buf, SMPDU_Sock_size_t len, SMPDU_Sock_size_t * num_read)
{
    int result = SMPD_SUCCESS;
    SMPD_IOV iov;
    smpd_enter_fn(FCNAME);
    iov.buf = buf;
    iov.len = len;
    result = SMPDU_Sock_readv(sock, &iov, 1, num_read);
    smpd_exit_fn(FCNAME);
    return result;
}

#undef FUNCNAME
#define FUNCNAME SMPDU_Sock_readv
#undef FCNAME
#define FCNAME SMPDI_QUOTE(FUNCNAME)
int SMPDU_Sock_readv(SMPDU_Sock_t sock, SMPD_IOV * iov, int iov_n, SMPDU_Sock_size_t * num_read)
{
    int result = SMPD_SUCCESS;
    DWORD nFlags = 0;
    DWORD num_read_local;

    smpd_enter_fn(FCNAME);

    if (WSARecv(sock->sock, iov, iov_n, &num_read_local, &nFlags, NULL/*overlapped*/, NULL/*completion routine*/)
        == SOCKET_ERROR){
	    result = WSAGetLastError();
	    *num_read = 0;
	    if ((result == WSAEWOULDBLOCK) || (result == 0)){
	        result = SMPD_SUCCESS;
	    }
	    else{
	        if (result == WSAENOBUFS){
		        WSABUF tmp;
		        tmp.buf = iov[0].buf;
		        tmp.len = iov[0].len;
		        while (result == WSAENOBUFS){
        		    /*printf("[%d] receiving %d bytes\n", __LINE__, tmp.len);fflush(stdout);*/
		            if (WSARecv(sock->sock, &tmp, 1, &num_read_local, &nFlags, NULL, NULL) != SOCKET_ERROR){
			            /*printf("[%d] read %d bytes\n", __LINE__, num_read_local);fflush(stdout);*/
			            *num_read = num_read_local;
			            result = SMPD_SUCCESS;
			            break;
        		    }
		            /*printf("[%d] reducing send length from %d to %d\n", __LINE__, tmp.len, tmp.len / 2);fflush(stdout);*/
		            result = WSAGetLastError();
		            tmp.len = tmp.len / 2;
        		}
		        if (result != SMPD_SUCCESS){
		            if (result == WSAEWOULDBLOCK){
			            result = SMPD_SUCCESS;
		            }
		            else{
			            result = SMPD_FAIL;
		            }
		        }
	        }
	        else{
		        result = SMPD_FAIL;
	        }
	    }
    }
    else{
	    *num_read = num_read_local;
    }
    smpd_exit_fn(FCNAME);
    return result;
}

#undef FUNCNAME
#define FUNCNAME SMPDU_Sock_write
#undef FCNAME
#define FCNAME SMPDI_QUOTE(FUNCNAME)
int SMPDU_Sock_write(SMPDU_Sock_t sock, void * buf, SMPDU_Sock_size_t len, SMPDU_Sock_size_t * num_written)
{
    int result;
    SMPD_IOV iov;
    smpd_enter_fn(FCNAME);
    iov.len = len;
    iov.buf = buf;
    result = SMPDU_Sock_writev(sock, &iov, 1, num_written);
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME SMPDU_Sock_writev
#undef FCNAME
#define FCNAME SMPDI_QUOTE(FUNCNAME)
int SMPDU_Sock_writev(SMPDU_Sock_t sock, SMPD_IOV * iov, int iov_n, SMPDU_Sock_size_t * num_written)
{
    int result;
    DWORD num_written_local;

    smpd_enter_fn(FCNAME);
    if (WSASend(sock->sock, iov, iov_n, &num_written_local, 0, NULL/*overlapped*/, NULL/*completion routine*/)
        == SOCKET_ERROR){
	    result = WSAGetLastError();
	    *num_written = 0;
    	if (result == WSAENOBUFS){
	        WSABUF tmp;
	        tmp.buf = iov[0].buf;
	        tmp.len = iov[0].len;
	        while (result == WSAENOBUFS){
        		if (WSASend(sock->sock, &tmp, 1, &num_written_local, 0, NULL, NULL) != SOCKET_ERROR){
		            /*printf("[%d] wrote %d bytes\n", __LINE__, num_written_local);fflush(stdout);*/
		            *num_written = num_written_local;
		            result = SMPD_SUCCESS;
		            break;
		        }
		        result = WSAGetLastError();
		        /*printf("[%d] reducing send length from %d to %d\n", __LINE__, tmp.len, tmp.len / 2);fflush(stdout);*/
		        tmp.len = tmp.len / 2;
		        if (tmp.len == 0 && result == WSAENOBUFS){
		            break;
		        }
	        }
	    }
	    if (result && (result != WSAEWOULDBLOCK)){
	        result = SMPD_FAIL;
	        smpd_exit_fn(FCNAME);
	        return result;
	    }
    }
    else{
	    *num_written = num_written_local;
    }
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME SMPDU_Sock_get_sock_id
#undef FCNAME
#define FCNAME SMPDI_QUOTE(FUNCNAME)
int SMPDU_Sock_get_sock_id(SMPDU_Sock_t sock)
{
    int ret_val;
    smpd_enter_fn(FCNAME);
    if (sock == SMPDU_SOCK_INVALID_SOCK){
	    ret_val = -1;
    }
    else{
	    if (sock->type == SOCKI_LISTENER){
	        ret_val = (int)sock->listen_sock;
        }
	    else{
	        ret_val = (int)sock->sock;
        }
    }
    smpd_exit_fn(FCNAME);
    return ret_val;
}

#undef FUNCNAME
#define FUNCNAME SMPDU_Sock_get_sock_set_id
#undef FCNAME
#define FCNAME SMPDI_QUOTE(FUNCNAME)
int SMPDU_Sock_get_sock_set_id(SMPDU_Sock_set_t set)
{
    int ret_val;
    smpd_enter_fn(FCNAME);
    ret_val = PtrToInt(set);/*(int)set;*/
    smpd_exit_fn(FCNAME);
    return ret_val;
}


/* FIXME: Provide a rich set of error codes as in mpid/common/sock */
/*
#undef FUNCNAME
#define FUNCNAME SMPDU_Sock_get_error_class_string
#undef FCNAME
#define FCNAME SMPDI_QUOTE(FUNCNAME)
int SMPDU_Sock_get_error_class_string(int error, char *error_string, int length)
{
    SMPDI_STATE_DECL(SMPD_STATE_SMPDU_SOCK_GET_ERROR_CLASS_STRING);

    smpd_enter_fn(FCNAME);
    switch (SMPDR_ERR_GET_CLASS(error))
    {
    case SMPDU_SOCK_ERR_FAIL:
	MPIU_Strncpy(error_string, "generic socket failure", length);
	break;
    case SMPDU_SOCK_ERR_INIT:
	MPIU_Strncpy(error_string, "socket module not initialized", length);
	break;
    case SMPDU_SOCK_ERR_NOMEM:
	MPIU_Strncpy(error_string, "not enough memory to complete the socket operation", length);
	break;
    case SMPDU_SOCK_ERR_BAD_SET:
	MPIU_Strncpy(error_string, "invalid socket set", length);
	break;
    case SMPDU_SOCK_ERR_BAD_SOCK:
	MPIU_Strncpy(error_string, "invalid socket", length);
	break;
    case SMPDU_SOCK_ERR_BAD_HOST:
	MPIU_Strncpy(error_string, "host description buffer not large enough", length);
	break;
    case SMPDU_SOCK_ERR_BAD_HOSTNAME:
	MPIU_Strncpy(error_string, "invalid host name", length);
	break;
    case SMPDU_SOCK_ERR_BAD_PORT:
	MPIU_Strncpy(error_string, "invalid port", length);
	break;
    case SMPDU_SOCK_ERR_BAD_BUF:
	MPIU_Strncpy(error_string, "invalid buffer", length);
	break;
    case SMPDU_SOCK_ERR_BAD_LEN:
	MPIU_Strncpy(error_string, "invalid length", length);
	break;
    case SMPDU_SOCK_ERR_SOCK_CLOSED:
	MPIU_Strncpy(error_string, "socket closed", length);
	break;
    case SMPD_FAIL:
	MPIU_Strncpy(error_string, "socket connection closed", length);
	break;
    case SMPDU_SOCK_ERR_CONN_FAILED:
	MPIU_Strncpy(error_string, "socket connection failed", length);
	break;
    case SMPDU_SOCK_ERR_INPROGRESS:
	MPIU_Strncpy(error_string, "socket operation in progress", length);
	break;
    case SMPDU_SOCK_ERR_TIMEOUT:
	MPIU_Strncpy(error_string, "socket operation timed out", length);
	break;
    case SMPDU_SOCK_ERR_INTR:
	MPIU_Strncpy(error_string, "socket operation interrupted", length);
	break;
    case SMPDU_SOCK_ERR_NO_NEW_SOCK:
	MPIU_Strncpy(error_string, "no new connection available", length);
	break;
    default:
	MPIU_Snprintf(error_string, length, "unknown socket error %d", error);
	break;
    }
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}
*/
