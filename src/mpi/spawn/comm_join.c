/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Comm_join */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Comm_join = PMPI_Comm_join
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Comm_join  MPI_Comm_join
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Comm_join as PMPI_Comm_join
#endif
/* -- End Profiling Symbol Block */

/* Prototypes for local functions */
PMPI_LOCAL int MPIR_fd_send(int, void *, int);
PMPI_LOCAL int MPIR_fd_recv(int, void *, int);

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Comm_join
#define MPI_Comm_join PMPI_Comm_join

#ifdef HAVE_ERRNO_H
#include <errno.h> /* needed for read/write error codes */
#endif

#ifdef HAVE_WINDOWS_H
#define SOCKET_EINTR	    WSAEINTR
#else
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#define SOCKET_EINTR	    EINTR
#endif

PMPI_LOCAL int MPIR_fd_send(int fd, void *buffer, int length)
{
    int result, num_bytes;

    while (length)
    {
	num_bytes = send(fd, buffer, length, 0);
	/* --BEGIN ERROR HANDLING-- */
	if (num_bytes == -1)
	{
#ifdef HAVE_WINDOWS_H
	    result = WSAGetLastError();
#else
	    result = errno;
#endif
	    if (result == SOCKET_EINTR)
                continue;
            else
                return result;
	}
	/* --END ERROR HANDLING-- */
        else {
            length -= num_bytes;
            buffer = (char*)buffer + num_bytes;
        }
    }
    return 0;
}

PMPI_LOCAL int MPIR_fd_recv(int fd, void *buffer, int length)
{
    int result, num_bytes;

    while (length)
    {
	num_bytes = recv(fd, buffer, length, 0);
	/* --BEGIN ERROR HANDLING-- */
	if (num_bytes == -1)
	{
#ifdef HAVE_WINDOWS_H
	    result = WSAGetLastError();
#else
	    result = errno;
#endif
	    if (result == SOCKET_EINTR)
                continue;
	    else
                return result;
	}
	/* --END ERROR HANDLING-- */
        else {
            length -= num_bytes;
            buffer = (char*)buffer + num_bytes;
        }
    }
    return 0;
}

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Comm_join

/*@
   MPI_Comm_join - Create a communicator by joining two processes connected by 
     a socket.

   Input Parameter:
. fd - socket file descriptor 

   Output Parameter:
. intercomm - new intercommunicator (handle) 

 Notes:
  The socket must be quiescent before 'MPI_COMM_JOIN' is called and after 
  'MPI_COMM_JOIN' returns. More specifically, on entry to 'MPI_COMM_JOIN', a 
  read on the socket will not read any data that was written to the socket 
  before the remote process called 'MPI_COMM_JOIN'.

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_ARG
@*/
int MPI_Comm_join(int fd, MPI_Comm *intercomm)
{
    static const char FCNAME[] = "MPI_Comm_join";
    int mpi_errno = MPI_SUCCESS, err;
    MPID_Comm *intercomm_ptr;
    char *local_port, *remote_port;
    MPIU_CHKLMEM_DECL(2);
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_COMM_JOIN);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPIU_THREAD_CS_ENTER(ALLFUNC,);
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_COMM_JOIN);

    /* ... body of routine ...  */
    
    MPIU_CHKLMEM_MALLOC(local_port, char *, MPI_MAX_PORT_NAME, mpi_errno, "local port name");
    MPIU_CHKLMEM_MALLOC(remote_port, char *, MPI_MAX_PORT_NAME, mpi_errno, "remote port name");
    
    mpi_errno = MPIR_Open_port_impl(NULL, local_port);
    MPIU_ERR_CHKANDJUMP((mpi_errno != MPI_SUCCESS), mpi_errno, MPI_ERR_OTHER, "**openportfailed");

    err = MPIR_fd_send(fd, local_port, MPI_MAX_PORT_NAME);
    MPIU_ERR_CHKANDJUMP1((err != 0), mpi_errno, MPI_ERR_INTERN, "**join_send", "**join_send %d", err);

    err = MPIR_fd_recv(fd, remote_port, MPI_MAX_PORT_NAME);
    MPIU_ERR_CHKANDJUMP1((err != 0), mpi_errno, MPI_ERR_INTERN, "**join_recv", "**join_recv %d", err);

    MPIU_ERR_CHKANDJUMP2((strcmp(local_port, remote_port) == 0), mpi_errno, MPI_ERR_INTERN, "**join_portname",
			 "**join_portname %s %s", local_port, remote_port);

    if (strcmp(local_port, remote_port) < 0) {
        MPID_Comm *comm_self_ptr;
        MPID_Comm_get_ptr( MPI_COMM_SELF, comm_self_ptr );
        mpi_errno = MPIR_Comm_accept_impl(local_port, NULL, 0, comm_self_ptr, &intercomm_ptr);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    } else {
        MPID_Comm *comm_self_ptr;
        MPID_Comm_get_ptr( MPI_COMM_SELF, comm_self_ptr );
        mpi_errno = MPIR_Comm_connect_impl(remote_port, NULL, 0, comm_self_ptr, &intercomm_ptr);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }

    mpi_errno = MPIR_Close_port_impl(local_port);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    MPIU_OBJ_PUBLISH_HANDLE(*intercomm, intercomm_ptr->handle);

    /* ... end of body of routine ... */

  fn_exit:
    MPIU_CHKLMEM_FREEALL();
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_COMM_JOIN);
    MPIU_THREAD_CS_EXIT(ALLFUNC,);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_comm_join",
	    "**mpi_comm_join %d %p", fd, intercomm);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( NULL, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
