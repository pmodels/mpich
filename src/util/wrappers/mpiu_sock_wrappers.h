/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef MPIU_SOCK_WRAPPERS_H_INCLUDED
#define MPIU_SOCK_WRAPPERS_H_INCLUDED

/* Currently the SOCK wrapper is very thin and does not
 * perform extensive error checking on args.
 */

/* FIXME: Replace wrapper funcs implemented as complex
 * macros with static inline funcs
 */
/* FIXME: Add more comments */
/* FIXME: Ensure this header is only loaded when needed */

#include "mpl.h"
#include "mpichconf.h"
#include "mpi.h"
#include "mpierror.h"
#include "mpierrs.h"
#include "mpimem.h"
#include "mpidbg.h"
#include "mpiutil.h"

#ifdef USE_NT_SOCK

    typedef SOCKET MPIU_SOCKW_Sockfd_t;
    typedef int socklen_t;
    typedef u_short in_port_t;
    typedef int ssize_t;

#   define MPIU_SOCKW_SOCKFD_INVALID    INVALID_SOCKET
#   define MPIU_SOCKW_EINTR WSAEINTR

#   undef FUNCNAME
#   define FUNCNAME MPIU_SOCKW_Inet_addr
#   undef FCNAME
#   define FCNAME MPL_QUOTE(FUNCNAME)
#   define MPIU_SOCKW_Inet_addr(ipv4_dd_str, inaddr) (              \
        ((*(inaddr) = inet_addr(ipv4_dd_str))                       \
            != INADDR_NONE)                                         \
        ? MPI_SUCCESS                                               \
        : MPIR_Err_create_code(MPI_SUCCESS,                         \
                MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER,    \
                "**inet_addr", "**inet_addr %s %d",                 \
                MPIU_OSW_Strerror(MPIU_OSW_Get_errno()),             \
                MPIU_OSW_Get_errno())                                \
    )

#   undef FUNCNAME
#   define FUNCNAME MPIU_SOCKW_init
#   undef FCNAME
#   define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIU_SOCKW_Init(void )
{
    WSADATA wsaData;
    /* Init winsock */
    if(WSAStartup(MAKEWORD(2,2), &wsaData) != 0){
        int err;
        err = MPIU_OSW_Get_errno();
        return MPIR_Err_create_code(MPI_SUCCESS, 
            MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER,
            "**wsastartup", "**wsastartup %s %d",
            MPIU_OSW_Strerror(err), err);
    }
    else{
        return MPI_SUCCESS;
    }
}

#   define MPIU_SOCKW_Sockfd_is_valid(sockfd)    (                  \
        ((sockfd) != INVALID_SOCKET) ? 1 : 0                        \
    )

#   undef FUNCNAME
#   define FUNCNAME MPIU_SOCKW_Finalize
#   undef FCNAME
#   define FCNAME MPL_QUOTE(FUNCNAME)
#   define MPIU_SOCKW_Finalize()    (                               \
        (WSACleanup() == 0)                                         \
        ? MPI_SUCCESS                                               \
        : MPIR_Err_create_code(MPI_SUCCESS,                         \
                MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER,    \
                "**wsacleanup", "**wsacleanup %s %d",               \
                MPIU_OSW_Strerror(MPIU_OSW_Get_errno()),             \
                MPIU_OSW_Get_errno())                                \
    )

#   undef FUNCNAME
#   define FUNCNAME MPIU_SOCKW_Sock_open
#   undef FCNAME
#   define FCNAME MPL_QUOTE(FUNCNAME)
#   define MPIU_SOCKW_Sock_open(domain, type, protocol, sock_ptr) ( \
        ((*sock_ptr = WSASocket(domain, type, protocol, NULL, 0,    \
            WSA_FLAG_OVERLAPPED)) != INVALID_SOCKET)                \
        ? MPI_SUCCESS                                               \
        : MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, \
            __LINE__, MPI_ERR_OTHER, "**sock_create",               \
            "**sock_create %s %d",                                  \
            MPIU_OSW_Strerror(MPIU_OSW_Get_errno()),                 \
            MPIU_OSW_Get_errno())                                    \
    )

#   undef FUNCNAME
#   define FUNCNAME MPIU_SOCKW_Sock_close
#   undef FCNAME
#   define FCNAME MPL_QUOTE(FUNCNAME)
#   define MPIU_SOCKW_Sock_close(sock)    (                         \
        (closesocket(sock) != SOCKET_ERROR)                         \
        ? MPI_SUCCESS                                               \
        : MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, \
            __LINE__, MPI_ERR_OTHER, "**sock_close",                \
            "**sock_close %s %d",                                   \
            MPIU_OSW_Strerror(MPIU_OSW_Get_errno()),                 \
            MPIU_OSW_Get_errno())                                    \
    )

#   undef FUNCNAME
#   define FUNCNAME MPIU_SOCKW_Bind
#   undef FCNAME
#   define FCNAME MPL_QUOTE(FUNCNAME)
#   define MPIU_SOCKW_Bind(sock, addr, addr_len) (                  \
        (bind(sock, (struct sockaddr *)addr, addr_len)              \
            != SOCKET_ERROR)                                        \
        ? MPI_SUCCESS                                               \
        : MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, \
            __LINE__, MPI_ERR_OTHER, "**bind",                      \
            "**bind %s %d",                                         \
            MPIU_OSW_Strerror(MPIU_OSW_Get_errno()),                 \
            MPIU_OSW_Get_errno())                                    \
    )

/* Bind to 1st port available in the range [low_port, high_port] */
#   undef FUNCNAME
#   define FUNCNAME MPIU_SOCKW_Bind_port_range
#   undef FCNAME
#   define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIU_SOCKW_Bind_port_range(
    MPIU_SOCKW_Sockfd_t sock, struct sockaddr_in *sin, 
    unsigned short int low_port, unsigned short int high_port)
{
    unsigned short int cur_port=0;
    int mpi_errno = MPI_SUCCESS;
    int done = 0;

    MPIU_Assert(sin);

    for(cur_port = low_port; cur_port <= high_port; cur_port++){
        (sin)->sin_port = htons(cur_port);
        if(bind(sock, (struct sockaddr *)(sin), 
            sizeof(struct sockaddr_in)) != SOCKET_ERROR){
            done = 1;
            break;
        }
        else{
            int err;
            err = MPIU_OSW_Get_errno();
            MPIR_ERR_CHKANDJUMP2( ((err != WSAEADDRINUSE) &&
                (err != WSAEADDRNOTAVAIL)), mpi_errno, MPI_ERR_OTHER,
                "**bind", "**bind %s %d", MPIU_OSW_Strerror(err), err);
        }
    }
    MPIR_ERR_CHKANDJUMP2(!done, mpi_errno, MPI_ERR_OTHER,
        "**bindportrange", "**bindportrange %d %d", low_port, high_port);

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
} 

#   undef FUNCNAME
#   define FUNCNAME MPIU_SOCKW_Listen
#   undef FCNAME
#   define FCNAME MPL_QUOTE(FUNCNAME)
#   define MPIU_SOCKW_Listen(sock, backlog)  (                      \
        (listen(sock, backlog) != SOCKET_ERROR)                     \
        ? MPI_SUCCESS                                               \
        : MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, \
            __LINE__, MPI_ERR_OTHER, "**listen",                    \
            "**listen %s %d",                                       \
            MPIU_OSW_Strerror(MPIU_OSW_Get_errno()),                 \
            MPIU_OSW_Get_errno())                                    \
    )

#   undef FUNCNAME
#   define FUNCNAME MPIU_SOCKW_Accept
#   undef FCNAME
#   define FCNAME MPL_QUOTE(FUNCNAME)
#   define MPIU_SOCKW_Accept(sock, addr, addr_len,                  \
        new_sock_ptr)(                                              \
        ((*new_sock_ptr = accept(sock, addr, addr_len))             \
            != INVALID_SOCKET)                                      \
        ? MPI_SUCCESS                                               \
        :(                                                          \
            (MPIU_OSW_Get_errno() == WSAEWOULDBLOCK)                 \
            ? MPI_SUCCESS                                           \
            : MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL,     \
                FCNAME, __LINE__, MPI_ERR_OTHER, "**sock_accept",   \
                "**sock_accept %s %d",                              \
                MPIU_OSW_Strerror(MPIU_OSW_Get_errno()),             \
                MPIU_OSW_Get_errno())                                \
        )                                                           \
    )

#   undef FUNCNAME
#   define FUNCNAME MPIU_SOCKW_Connect
#   undef FCNAME
#   define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIU_SOCKW_Connect(
    MPIU_SOCKW_Sockfd_t sockfd, struct sockaddr *addr, 
    int addr_len, int *is_pending)
{
    int mpi_errno = MPI_SUCCESS;

    if(connect(sockfd, addr, addr_len) == SOCKET_ERROR){
        int err;
        err = MPIU_OSW_Get_errno();

        MPIR_ERR_CHKANDJUMP2((err != WSAEWOULDBLOCK), mpi_errno,
            MPI_ERR_OTHER, "**sock_connect", "**sock_connect %s %d",
            MPIU_OSW_Strerror(err), err);

        MPIU_Assert(is_pending);
        *is_pending = 1;
    }
fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#   undef FUNCNAME
#   define FUNCNAME MPIU_SOCKW_Read
#   undef FCNAME
#   define FCNAME MPL_QUOTE(FUNCNAME)
#   define MPIU_SOCKW_Read(sock, buf, buf_len, nb_rd_ptr)(          \
        ((*nb_rd_ptr = recv(sock, buf, buf_len, 0x0))               \
            != SOCKET_ERROR)                                        \
        ? MPI_SUCCESS                                               \
        :(                                                          \
            ((MPIU_OSW_Get_errno() == WSAEWOULDBLOCK)               \
                || (MPIU_OSW_Get_errno() == WSAEINTR))              \
            ? MPI_SUCCESS                                           \
            : MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL,     \
                FCNAME, __LINE__, MPI_ERR_OTHER,                    \
                "**sock_read", "**sock_read %s %d",                 \
                MPIU_OSW_Strerror(MPIU_OSW_Get_errno()),            \
                MPIU_OSW_Get_errno())                               \
        )                                                           \
    )

#   undef FUNCNAME
#   define FUNCNAME MPIU_SOCKW_Readv
#   undef FCNAME
#   define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIU_SOCKW_Readv(MPIU_SOCKW_Sockfd_t sock,
    MPL_IOV *iov, int iov_cnt, int *nb_rd_ptr)
{
    DWORD flags = 0;
    int err;
    int mpi_errno = MPI_SUCCESS;

    MPIU_Assert(nb_rd_ptr);
    if(WSARecv(sock, iov, iov_cnt, (LPDWORD )nb_rd_ptr, &flags, NULL, NULL)
        == SOCKET_ERROR){
        err = MPIU_OSW_Get_errno();
        MPIR_ERR_CHKANDJUMP2(!( (err == WSAEWOULDBLOCK) ||
            (err == WSA_IO_PENDING) || (err == WSAEINTR) ),
            mpi_errno, MPI_ERR_OTHER, "**sock_read",
            "**sock_read %s %d", MPIU_OSW_Strerror(err), err);

        *nb_rd_ptr = -1;
    }

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#   undef FUNCNAME
#   define FUNCNAME MPIU_SOCKW_Write
#   undef FCNAME
#   define FCNAME MPL_QUOTE(FUNCNAME)
#   define MPIU_SOCKW_Write(sock, buf, buf_len, nb_wr_ptr)(         \
        ((*nb_wr_ptr = send(sock, buf, buf_len, 0x0))               \
            != SOCKET_ERROR)                                        \
        ? MPI_SUCCESS                                               \
        :(                                                          \
            ((MPIU_OSW_Get_errno() == WSAEWOULDBLOCK)               \
                || (MPIU_OSW_Get_errno() == WSAEINTR))              \
            ? MPI_SUCCESS                                           \
            : MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL,     \
                FCNAME, __LINE__, MPI_ERR_OTHER,                    \
                "**sock_write", "**sock_write %s %d",               \
                MPIU_OSW_Strerror(MPIU_OSW_Get_errno()),            \
                MPIU_OSW_Get_errno())                               \
        )                                                           \
    )


#   undef FUNCNAME
#   define FUNCNAME MPIU_SOCKW_Writev
#   undef FCNAME
#   define FCNAME MPL_QUOTE(FUNCNAME)
#   define MPIU_SOCKW_Writev(sock, iov, iov_cnt, nb_wr_ptr)(        \
        (WSASend(sock, iov, iov_cnt, nb_wr_ptr, 0x0, NULL, NULL)    \
            != SOCKET_ERROR)                                        \
        ? MPI_SUCCESS                                               \
        :(                                                          \
            ((MPIU_OSW_Get_errno() == WSAEWOULDBLOCK)               \
                || (MPIU_OSW_Get_errno() == WSAEINTR))              \
            ? MPI_SUCCESS                                           \
            : MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL,     \
                FCNAME, __LINE__, MPI_ERR_OTHER,                    \
                "**sock_write", "**sock_write %s %d",               \
                MPIU_OSW_Strerror(MPIU_OSW_Get_errno()),            \
                MPIU_OSW_Get_errno())                               \
        )                                                           \
    )

#   undef FUNCNAME
#   define FUNCNAME MPIU_SOCKW_Sock_cntrl_nb
#   undef FCNAME
#   define FCNAME MPL_QUOTE(FUNCNAME)
#   define MPIU_SOCKW_Sock_cntrl_nb(sock, is_nb)(                   \
        (ioctlsocket(sock, FIONBIO, (u_long *)&is_nb)               \
            != SOCKET_ERROR)                                        \
        ? MPI_SUCCESS                                               \
        : MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, \
            __LINE__, MPI_ERR_OTHER, "**ioctlsocket",               \
            "**ioctlsocket %s %d",                                  \
            MPIU_OSW_Strerror(MPIU_OSW_Get_errno()),                \
            MPIU_OSW_Get_errno())                                   \
    )

#   undef FUNCNAME
#   define FUNCNAME MPIU_SOCKW_Sock_setopt
#   undef FCNAME
#   define FCNAME MPL_QUOTE(FUNCNAME)
#   define MPIU_SOCKW_Sock_setopt(sock, level, opt_name,opt_val_ptr,\
        opt_len) (                                                  \
        (setsockopt(sock, level, opt_name,                          \
                    (const char *)opt_val_ptr, opt_len)             \
                    != SOCKET_ERROR)                                \
        ? MPI_SUCCESS                                               \
        : MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, \
            __LINE__, MPI_ERR_OTHER, "**setsockopt",                \
            "**setsockopt %s %d",                                   \
            MPIU_OSW_Strerror(MPIU_OSW_Get_errno()),                \
            MPIU_OSW_Get_errno())                                   \
    )

#   undef FUNCNAME
#   define FUNCNAME MPIU_SOCKW_Sock_getopt
#   undef FCNAME
#   define FCNAME MPL_QUOTE(FUNCNAME)
#   define MPIU_SOCKW_Sock_getopt(sock, level, opt_name,opt_val_ptr,\
        opt_len_ptr) (                                              \
        (getsockopt(sock, level, opt_name, opt_val_ptr, opt_len_ptr)\
            != SOCKET_ERROR)                                        \
        ? MPI_SUCCESS                                               \
        : MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, \
            __LINE__, MPI_ERR_OTHER, "**getsockopt",                \
            "**getsockopt %s %d",                                   \
            MPIU_OSW_Strerror(MPIU_OSW_Get_errno()),                \
            MPIU_OSW_Get_errno())                                   \
    )

#   undef FUNCNAME
#   define FUNCNAME MPIU_SOCKW_Sock_has_error
#   undef FCNAME
#   define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIU_SOCKW_Sock_has_error(MPIU_SOCKW_Sockfd_t sock)
{
    int has_error = 0;
    int opt_len = sizeof(int);
    if(getsockopt(sock, SOL_SOCKET, SO_ERROR, (char *)&has_error,
            &opt_len) != SOCKET_ERROR){
        return MPI_SUCCESS;
    }
    else{
        return MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL,
                    FCNAME, __LINE__, MPI_ERR_OTHER, "**getsockopt",
                    "**getsockopt %s %d",
                    MPIU_OSW_Strerror(MPIU_OSW_Get_errno()),
                    MPIU_OSW_Get_errno());
    }
}


#   define MPIU_SOCKW_FDSETCPY_(dst, src)    (dst = src)

typedef struct {
    MPIU_SOCKW_Sockfd_t sockfd;
    void *user_ptr;
    int event_flag;
} MPIU_SOCKW_Waitset_sock_hnd_;

typedef MPIU_SOCKW_Waitset_sock_hnd_ * MPIU_SOCKW_Waitset_sock_hnd_t;

typedef struct {
        fd_set  read_fds;
        fd_set  tmp_read_fds;
        int nread_fds;
        fd_set  write_fds;
        fd_set  tmp_write_fds;
        int nwrite_fds;
        fd_set  except_fds;
        fd_set  tmp_except_fds;
        int nexcept_fds;
        MPIU_SOCKW_Waitset_sock_hnd_ *fdset;
        int fdset_index;
        int fdset_cur_index;
        int nfds;
        int nevents;
    } MPIU_SOCKW_Waitset_;

typedef MPIU_SOCKW_Waitset_ * MPIU_SOCKW_Waitset_hnd_t;

#define MPIU_SOCKW_WAITSET_CURINDEX_INVALID_    -1
static inline void MPIU_SOCKW_Waitset_curindex_reset_(MPIU_SOCKW_Waitset_hnd_t waitset_hnd)
{
    (waitset_hnd)->fdset_cur_index = 0;
}

static inline void MPIU_SOCKW_Waitset_curindex_inc_(MPIU_SOCKW_Waitset_hnd_t waitset_hnd)
{
    if(++((waitset_hnd)->fdset_cur_index) >= (waitset_hnd)->nfds){
        MPIU_SOCKW_Waitset_curindex_reset_(waitset_hnd);
    }
}

/* FIXME: This is cheating... Expand dynamically - we need to take
 * care of updating sock Handles if we expand dynamically. 
 */
static inline void MPIU_SOCKW_Waitset_expand_(MPIU_SOCKW_Waitset_hnd_t waitset_hnd,
    int *index_ptr)
{
    MPIU_UNREFERENCED_ARG(index_ptr);
    (waitset_hnd)->nfds++;
    MPIU_Assert((waitset_hnd)->nfds < FD_SETSIZE);
}


static inline int MPIU_SOCKW_Waitset_freeindex_get_(MPIU_SOCKW_Waitset_hnd_t waitset_hnd)
{
    int i, findex;
    findex = MPIU_SOCKW_WAITSET_CURINDEX_INVALID_;
    for(i = 0; i < (waitset_hnd)->nfds; i++){
        if(((waitset_hnd)->fdset[i]).sockfd == MPIU_SOCKW_SOCKFD_INVALID){
            findex = i;
            break;
        }
    } /*
    if(findex == MPIU_SOCKW_WAITSET_CURINDEX_INVALID_){
        MPIU_SOCKW_Waitset_expand_(waitset_hnd);
        findex = i;
    }*/
    return findex;
}

#   define MPIU_SOCKW_TIMEVAL_INFINITE      -1
#   define MPIU_SOCKW_TIMEVAL_ZERO          0

typedef struct timeval MPIU_SOCKW_Timeval_t_;
typedef MPIU_SOCKW_Timeval_t_ *MPIU_SOCKW_Timeval_hnd_t;

#   define MPIU_SOCKW_Timeval_hnd_is_init_(hnd)  ((hnd) ? 1 : 0)
#   define MPIU_SOCKW_Waitset_hnd_is_init_(hnd)  ((hnd) ? 1 : 0)
#   define MPIU_SOCKW_Waitset_sock_hnd_is_init_(hnd)   ((hnd) ? 1 : 0)

#   undef FUNCNAME
#   define FUNCNAME MPIU_SOCKW_Timeval_hnd_init
#   undef FCNAME
#   define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIU_SOCKW_Timeval_hnd_init(
    MPIU_SOCKW_Timeval_hnd_t *hnd_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    MPIU_Assert(hnd_ptr);

    *hnd_ptr = (MPIU_SOCKW_Timeval_hnd_t)
                MPIU_Malloc(sizeof(MPIU_SOCKW_Timeval_t_));
    MPIR_ERR_CHKANDJUMP1( !(*hnd_ptr), mpi_errno, MPI_ERR_OTHER,
        "**nomem", "**nomem %s", "handle to timeval");

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

/* For portability with poll() timeval now has a milli sec 
 * time resolution - instead of a micro sec resolution
 */
#   undef FUNCNAME
#   define FUNCNAME MPIU_SOCKW_Timeval_hnd_set
#   undef FCNAME
#   define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIU_SOCKW_Timeval_hnd_set(
    MPIU_SOCKW_Timeval_hnd_t hnd, int tv_msec)
{
    int mpi_errno = MPI_SUCCESS;

    MPIU_Assert(MPIU_SOCKW_Timeval_hnd_is_init_(hnd));

    hnd->tv_sec = tv_msec/1000;
    hnd->tv_usec = (tv_msec % 1000) * 1000;

    return mpi_errno;
}

#   undef FUNCNAME
#   define FUNCNAME MPIU_SOCKW_Timeval_hnd_finalize
#   undef FCNAME
#   define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIU_SOCKW_Timeval_hnd_finalize(
    MPIU_SOCKW_Timeval_hnd_t *hnd_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    
    MPIU_Assert(hnd_ptr);

    if(MPIU_SOCKW_Timeval_hnd_is_init_(*hnd_ptr)){
        MPIU_Free(*hnd_ptr);
    }
    *hnd_ptr = NULL;

    return mpi_errno;
}

#   define MPIU_SOCKW_FLAG_CLR                      0x0

#   define MPIU_SOCKW_FLAG_SOCK_IS_READABLE_        0x1
#   define MPIU_SOCKW_FLAG_SOCK_IS_WRITEABLE_       0x10
#   define MPIU_SOCKW_FLAG_SOCK_HAS_EXCEPTIONS_     0x1000

#   define MPIU_SOCKW_FLAG_WAITON_RD    0x1
#   define MPIU_SOCKW_FLAG_WAITON_WR    0x10
#   define MPIU_SOCKW_FLAG_WAITON_EX    0x100


/* TODO:
    static inline int MPIU_SOCKW_Waitset_hndArr_create() 
*/

#   undef FUNCNAME
#   define FUNCNAME MPIU_SOCKW_Waitset_hnd_init
#   undef FCNAME
#   define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIU_SOCKW_Waitset_hnd_init(
            MPIU_SOCKW_Waitset_hnd_t *hnd_ptr, int nfds)
{
    int mpi_errno = MPI_SUCCESS;

    MPIU_Assert(hnd_ptr);
    MPIU_Assert(nfds <= FD_SETSIZE);

    if(nfds <= 0){
        nfds = FD_SETSIZE;
    }

    *hnd_ptr = (MPIU_SOCKW_Waitset_hnd_t) MPIU_Malloc(
            sizeof(MPIU_SOCKW_Waitset_));

    MPIR_ERR_CHKANDJUMP1(!(*hnd_ptr), mpi_errno, MPI_ERR_OTHER,
        "**nomem", "**nomem %s", "handle to waitset");

    (*hnd_ptr)->nevents = 0;
    FD_ZERO(&((*hnd_ptr)->read_fds));
    (*hnd_ptr)->nread_fds = 0;
    FD_ZERO(&((*hnd_ptr)->write_fds));
    (*hnd_ptr)->nwrite_fds = 0;
    FD_ZERO(&((*hnd_ptr)->except_fds));
    (*hnd_ptr)->nexcept_fds = 0;
    (*hnd_ptr)->nfds = nfds;
    (*hnd_ptr)->fdset_index = 0;
    MPIU_SOCKW_Waitset_curindex_reset_(*hnd_ptr);

    /* FIXME: Cheating - Expand dynamically */
    (*hnd_ptr)->fdset = (MPIU_SOCKW_Waitset_sock_hnd_ *)
        MPIU_Malloc(FD_SETSIZE * sizeof(MPIU_SOCKW_Waitset_sock_hnd_));

    MPIR_ERR_CHKANDJUMP1(!((*hnd_ptr)->fdset), mpi_errno, MPI_ERR_OTHER,
        "**nomem", "**nomem %s", "fdset array in waitSet");

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#   undef FUNCNAME
#   define FUNCNAME MPIU_SOCKW_Waitset_hnd_finalize
#   undef FCNAME
#   define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIU_SOCKW_Waitset_hnd_finalize(
            MPIU_SOCKW_Waitset_hnd_t *hnd_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    MPIU_Assert(hnd_ptr);

    if(MPIU_SOCKW_Waitset_hnd_is_init_(*hnd_ptr)){
        MPIU_Free((*hnd_ptr)->fdset);
        MPIU_Free(*hnd_ptr);
    }

    *hnd_ptr = NULL;
    return mpi_errno;
}

#   undef FUNCNAME
#   define FUNCNAME MPIU_SOCKW_Waitset_has_more_evnts
#   undef FCNAME
#   define FCNAME MPL_QUOTE(FUNCNAME)
#   define MPIU_SOCKW_Waitset_has_more_evnts(waitset_hnd) (           \
        waitset_hnd->nevents                                         \
    )

#   undef FUNCNAME
#   define FUNCNAME MPIU_SOCKW_Waitset_wait
#   undef FCNAME
#   define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIU_SOCKW_Waitset_wait(
    MPIU_SOCKW_Waitset_hnd_t hnd, MPIU_SOCKW_Timeval_hnd_t timeout)
{
    int nfds=0;
    int mpi_errno = MPI_SUCCESS;

    MPIU_Assert(MPIU_SOCKW_Waitset_hnd_is_init_(hnd));
    MPIU_Assert(MPIU_SOCKW_Timeval_hnd_is_init_(timeout));

    MPIU_SOCKW_Waitset_curindex_reset_(hnd);
    FD_ZERO(&(hnd->tmp_read_fds));
    if(hnd->nread_fds > 0){
        MPIU_SOCKW_FDSETCPY_(hnd->tmp_read_fds, hnd->read_fds);
    }
    FD_ZERO(&(hnd->tmp_write_fds));
    if(hnd->nwrite_fds > 0){
        MPIU_SOCKW_FDSETCPY_(hnd->tmp_write_fds, hnd->write_fds);
    }
    FD_ZERO(&(hnd->tmp_except_fds));
    if(hnd->nexcept_fds > 0){
        MPIU_SOCKW_FDSETCPY_(hnd->tmp_except_fds, hnd->except_fds);
    }
    nfds = 0; /* ignored in windows */
    hnd->nevents = select(nfds,&(hnd->tmp_read_fds),
                            &(hnd->tmp_write_fds),
                            &(hnd->tmp_except_fds), timeout); 
    MPIR_ERR_CHKANDJUMP2((hnd->nevents == SOCKET_ERROR), mpi_errno,
        MPI_ERR_OTHER, "**select", "**select %s %d",
        MPIU_OSW_Strerror(MPIU_OSW_Get_errno()),MPIU_OSW_Get_errno());

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#   undef FUNCNAME
#   define FUNCNAME MPIU_SOCKW_Waitset_get_sock_evnts
#   undef FCNAME
#   define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIU_SOCKW_Waitset_get_sock_evnts_(
    MPIU_SOCKW_Waitset_hnd_t waitset_hnd, MPIU_SOCKW_Sockfd_t sock,
    int *flag_ptr)
{
    int has_events = 0;
    /*
    if(!MPIU_SOCKW_Waitset_hnd_is_init_(waitset_hnd)){
        return MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL,
                FCNAME, __LINE__, MPI_ERR_OTHER, "**arg",
                "**arg %s", "waitSet handle");
    }
    if(!flag_ptr){
        return MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL,
                FCNAME, __LINE__, MPI_ERR_OTHER, "**arg",
                "**arg %s", "pointer to event flag");
    }
    */    

    *flag_ptr = MPIU_SOCKW_FLAG_CLR;

    if(waitset_hnd->nread_fds && FD_ISSET(sock, &(waitset_hnd->tmp_read_fds))){
        waitset_hnd->nevents--;
        *flag_ptr |= MPIU_SOCKW_FLAG_SOCK_IS_READABLE_;
        has_events = 1;
    }
    if(waitset_hnd->nwrite_fds && FD_ISSET(sock, &(waitset_hnd->tmp_write_fds))){
        waitset_hnd->nevents--;
        *flag_ptr |= MPIU_SOCKW_FLAG_SOCK_IS_WRITEABLE_;
        has_events = 1;
    }
    if(waitset_hnd->nexcept_fds && FD_ISSET(sock, &(waitset_hnd->tmp_except_fds))){
        waitset_hnd->nevents--;
        *flag_ptr |= MPIU_SOCKW_FLAG_SOCK_HAS_EXCEPTIONS_;
        has_events = 1;
    }
    return has_events;
}

#   undef FUNCNAME
#   define FUNCNAME MPIU_SOCKW_Waitset_get_nxt_sock_with_evnt
#   undef FCNAME
#   define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIU_SOCKW_Waitset_get_nxt_sock_with_evnt(
    MPIU_SOCKW_Waitset_hnd_t waitset_hnd,
    MPIU_SOCKW_Waitset_sock_hnd_t *sock_hnd_ptr)
{
    int i;
    int mpi_errno = MPI_SUCCESS;

    MPIU_Assert(MPIU_SOCKW_Waitset_hnd_is_init_(waitset_hnd));
    MPIU_Assert(sock_hnd_ptr);

    *sock_hnd_ptr = NULL;

    for(i=0; i<waitset_hnd->nfds; i++){
        MPIU_SOCKW_Sockfd_t fd;
        fd = waitset_hnd->fdset[waitset_hnd->fdset_cur_index].sockfd;
        if(fd != MPIU_SOCKW_SOCKFD_INVALID){
            if(MPIU_SOCKW_Waitset_get_sock_evnts_(waitset_hnd, fd,
                &(waitset_hnd->fdset[waitset_hnd->fdset_cur_index].event_flag))){
                *sock_hnd_ptr = &(waitset_hnd->fdset[waitset_hnd->fdset_cur_index]);
                MPIU_SOCKW_Waitset_curindex_inc_(waitset_hnd);
                break;
            }
        }
        MPIU_SOCKW_Waitset_curindex_inc_(waitset_hnd);
    }

    return mpi_errno;
}

#   undef FUNCNAME
#   define FUNCNAME MPIU_SOCKW_Waitset_add_sock
#   undef FCNAME
#   define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIU_SOCKW_Waitset_add_sock(
    MPIU_SOCKW_Waitset_hnd_t waitset_hnd, MPIU_SOCKW_Sockfd_t sock,
    int flag, void *user_ptr, MPIU_SOCKW_Waitset_sock_hnd_t *sock_hnd_ptr)
{
    int index= 0;
    int mpi_errno = MPI_SUCCESS;

    MPIU_Assert(MPIU_SOCKW_Waitset_hnd_is_init_(waitset_hnd));
    MPIU_Assert(sock_hnd_ptr);

    *sock_hnd_ptr = NULL;

    index = waitset_hnd->fdset_index;
    if(index >= waitset_hnd->nfds){
        index = MPIU_SOCKW_Waitset_freeindex_get_(waitset_hnd);
        /* FIXME: Try not to use MPIU_Assert(). Utils should not depend
         * on a device impl of assert */
        MPIU_Assert(index != MPIU_SOCKW_WAITSET_CURINDEX_INVALID_);
    }
    else{
        waitset_hnd->fdset_index++;
    }

    if(flag & MPIU_SOCKW_FLAG_WAITON_RD){
        FD_SET(sock, &(waitset_hnd->read_fds));
        waitset_hnd->nread_fds++;
    }
    if(flag & MPIU_SOCKW_FLAG_WAITON_WR){
        FD_SET(sock, &(waitset_hnd->write_fds));
        waitset_hnd->nwrite_fds++;
    }
    if(flag & MPIU_SOCKW_FLAG_WAITON_EX){
        FD_SET(sock, &(waitset_hnd->except_fds));
        waitset_hnd->nexcept_fds++;
    }

    (waitset_hnd->fdset[index]).sockfd = sock;
    (waitset_hnd->fdset[index]).event_flag = MPIU_SOCKW_FLAG_CLR;
    (waitset_hnd->fdset[index]).user_ptr = user_ptr;
    *sock_hnd_ptr = &(waitset_hnd->fdset[index]);

    return mpi_errno;
}

#   undef FUNCNAME
#   define FUNCNAME MPIU_SOCKW_Waitset_set_sock
#   undef FCNAME
#   define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIU_SOCKW_Waitset_set_sock(
    MPIU_SOCKW_Waitset_hnd_t waitset_hnd,
    MPIU_SOCKW_Waitset_sock_hnd_t sock_hnd, int flag)
{
    MPIU_SOCKW_Sockfd_t sock;
    int mpi_errno = MPI_SUCCESS;

    MPIU_Assert(MPIU_SOCKW_Waitset_hnd_is_init_(waitset_hnd));
    MPIU_Assert(MPIU_SOCKW_Waitset_sock_hnd_is_init_(sock_hnd));

    sock = sock_hnd->sockfd;
    if((flag & MPIU_SOCKW_FLAG_WAITON_RD) && 
        !FD_ISSET(sock, &(waitset_hnd->read_fds))){
        FD_SET(sock, &(waitset_hnd->read_fds));
        waitset_hnd->nread_fds++;
    }
    if((flag & MPIU_SOCKW_FLAG_WAITON_WR) &&
        !FD_ISSET(sock, &(waitset_hnd->write_fds))){
        FD_SET(sock, &(waitset_hnd->write_fds));
        waitset_hnd->nwrite_fds++;
    }
    if((flag & MPIU_SOCKW_FLAG_WAITON_EX) &&
        !FD_ISSET(sock, &(waitset_hnd->except_fds))){
        FD_SET(sock, &(waitset_hnd->except_fds));
        waitset_hnd->nexcept_fds++;
    }

    return mpi_errno;
}

#   undef FUNCNAME
#   define FUNCNAME MPIU_SOCKW_Waitset_clr_sock
#   undef FCNAME
#   define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIU_SOCKW_Waitset_clr_sock(
    MPIU_SOCKW_Waitset_hnd_t waitset_hnd,
    MPIU_SOCKW_Waitset_sock_hnd_t sock_hnd, int flag)
{
    MPIU_SOCKW_Sockfd_t sock;
    int mpi_errno = MPI_SUCCESS;

    MPIU_Assert(MPIU_SOCKW_Waitset_hnd_is_init_(waitset_hnd));
    MPIU_Assert(MPIU_SOCKW_Waitset_sock_hnd_is_init_(sock_hnd));

    sock = sock_hnd->sockfd;
    if((flag & MPIU_SOCKW_FLAG_WAITON_RD) && 
        FD_ISSET(sock, &(waitset_hnd->read_fds))){
        FD_CLR(sock, &(waitset_hnd->read_fds));
        waitset_hnd->nread_fds--;
    }
    if((flag & MPIU_SOCKW_FLAG_WAITON_WR) &&
        FD_ISSET(sock, &(waitset_hnd->write_fds))){
        FD_CLR(sock, &(waitset_hnd->write_fds));
        waitset_hnd->nwrite_fds--;
    }
    if((flag & MPIU_SOCKW_FLAG_WAITON_EX) &&
        FD_ISSET(sock, &(waitset_hnd->except_fds))){
        FD_CLR(sock, &(waitset_hnd->except_fds));
        waitset_hnd->nexcept_fds--;
    }

    return mpi_errno;
}


#   undef FUNCNAME
#   define FUNCNAME MPIU_SOCKW_Waitset_rem_sock
#   undef FCNAME
#   define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIU_SOCKW_Waitset_rem_sock(
    MPIU_SOCKW_Waitset_hnd_t waitset_hnd,
    MPIU_SOCKW_Waitset_sock_hnd_t *sock_hnd_ptr)
{
    MPIU_SOCKW_Sockfd_t sock;
    int mpi_errno = MPI_SUCCESS;

    MPIU_Assert(MPIU_SOCKW_Waitset_hnd_is_init_(waitset_hnd));
    MPIU_Assert(sock_hnd_ptr);
    MPIU_Assert(MPIU_SOCKW_Waitset_sock_hnd_is_init_(*sock_hnd_ptr));

    sock = (*sock_hnd_ptr)->sockfd;
    if(FD_ISSET(sock, &(waitset_hnd->read_fds))){
        FD_CLR(sock, &(waitset_hnd->read_fds));
        waitset_hnd->nread_fds--;
    }
    if(FD_ISSET(sock, &(waitset_hnd->write_fds))){
        FD_CLR(sock, &(waitset_hnd->write_fds));
        waitset_hnd->nwrite_fds--;
    }
    if(FD_ISSET(sock, &(waitset_hnd->except_fds))){
        FD_CLR(sock, &(waitset_hnd->except_fds));
        waitset_hnd->nexcept_fds--;
    }

    (*sock_hnd_ptr)->sockfd = MPIU_SOCKW_SOCKFD_INVALID;
    (*sock_hnd_ptr)->event_flag = MPIU_SOCKW_FLAG_CLR;
    (*sock_hnd_ptr)->user_ptr = NULL;
    *sock_hnd_ptr = NULL;

    return mpi_errno;
}

#   undef FUNCNAME
#   define FUNCNAME MPIU_SOCKW_Waitset_sock_hnd_get_sockfd
#   undef FCNAME
#   define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIU_SOCKW_Waitset_sock_hnd_get_sockfd(
    MPIU_SOCKW_Waitset_sock_hnd_t sock_hnd,
    MPIU_SOCKW_Sockfd_t *sockfd_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    MPIU_Assert(MPIU_SOCKW_Waitset_sock_hnd_is_init_(sock_hnd));
    MPIU_Assert(sockfd_ptr);

    *sockfd_ptr = sock_hnd->sockfd;

    return mpi_errno;
}

#   undef FUNCNAME
#   define FUNCNAME MPIU_SOCKW_Waitset_sock_hnd_get_user_ptr
#   undef FCNAME
#   define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIU_SOCKW_Waitset_sock_hnd_get_user_ptr(
    MPIU_SOCKW_Waitset_sock_hnd_t sock_hnd, void **userp_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    MPIU_Assert(MPIU_SOCKW_Waitset_sock_hnd_is_init_(sock_hnd));
    MPIU_Assert(userp_ptr);

    *userp_ptr = sock_hnd->user_ptr;

    return mpi_errno;
}

#   undef FUNCNAME
#   define FUNCNAME MPIU_SOCKW_Waitset_sock_hnd_set_user_ptr
#   undef FCNAME
#   define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIU_SOCKW_Waitset_sock_hnd_set_user_ptr(
    MPIU_SOCKW_Waitset_sock_hnd_t sock_hnd, void *user_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    MPIU_Assert(MPIU_SOCKW_Waitset_sock_hnd_is_init_(sock_hnd));

    sock_hnd->user_ptr = user_ptr;

    return mpi_errno;
}

#   undef FUNCNAME
#   define FUNCNAME MPIU_SOCKW_Waitset_is_sock_readable
#   undef FCNAME
#   define FCNAME MPL_QUOTE(FUNCNAME)
#   define MPIU_SOCKW_Waitset_is_sock_readable(sock_hnd)(              \
        (MPIU_SOCKW_Waitset_sock_hnd_is_init_(sock_hnd))                \
        ? (sock_hnd->event_flag & MPIU_SOCKW_FLAG_SOCK_IS_READABLE_) \
        : 0                                                         \
    )

#   undef FUNCNAME
#   define FUNCNAME MPIU_SOCKW_Waitset_is_sock_writeable
#   undef FCNAME
#   define FCNAME MPL_QUOTE(FUNCNAME)
#   define MPIU_SOCKW_Waitset_is_sock_writeable(sock_hnd)(             \
        (MPIU_SOCKW_Waitset_sock_hnd_is_init_(sock_hnd))                \
        ? (sock_hnd->event_flag & MPIU_SOCKW_FLAG_SOCK_IS_WRITEABLE_)\
        : 0                                                         \
    )

#else
    typedef int MPIU_SOCKW_Sockfd_t;
#   define MPIU_SOCKW_SOCKFD_INVALID    -1
#   define MPIU_SOCKW_EINTR EINTR

#endif /* USE_NT_SOCK */

#undef FUNCNAME
#undef FCNAME
#endif /* MPIU_SOCK_WRAPPERS_H_INCLUDED */
