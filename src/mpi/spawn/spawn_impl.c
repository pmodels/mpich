/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "namepub.h"


#ifdef HAVE_ERRNO_H
#include <errno.h>      /* needed for read/write error codes */
#endif

#ifdef HAVE_WINDOWS_H
#define SOCKET_EINTR        WSAEINTR
#else
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#define SOCKET_EINTR        EINTR
#endif

static int MPIR_fd_send(int fd, void *buffer, int length)
{
    int result, num_bytes;

    while (length) {
        /* The expectation is that the length of a join message will fit
         * in an int.  For Unixes that define send as returning ssize_t,
         * we can safely cast this to an int. */
        num_bytes = (int) send(fd, buffer, length, 0);
        /* --BEGIN ERROR HANDLING-- */
        if (num_bytes == -1) {
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
            buffer = (char *) buffer + num_bytes;
        }
    }
    return 0;
}

static int MPIR_fd_recv(int fd, void *buffer, int length)
{
    int result, num_bytes;

    while (length) {
        /* See discussion on send above for the cast to int. */
        num_bytes = (int) recv(fd, buffer, length, 0);
        /* --BEGIN ERROR HANDLING-- */
        if (num_bytes == -1) {
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
            buffer = (char *) buffer + num_bytes;
        }
    }
    return 0;
}

int MPIR_Comm_join_impl(int fd, MPIR_Comm ** p_intercomm_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    int err;
    MPIR_Comm *intercomm_ptr;
    char *local_port, *remote_port;
    MPIR_CHKLMEM_DECL(2);

    MPIR_CHKLMEM_MALLOC(local_port, char *, MPI_MAX_PORT_NAME, mpi_errno, "local port name",
                        MPL_MEM_DYNAMIC);
    MPIR_CHKLMEM_MALLOC(remote_port, char *, MPI_MAX_PORT_NAME, mpi_errno, "remote port name",
                        MPL_MEM_DYNAMIC);

    MPL_VG_MEM_INIT(local_port, MPI_MAX_PORT_NAME * sizeof(char));

    mpi_errno = MPIR_Open_port_impl(NULL, local_port);
    MPIR_ERR_CHKANDJUMP((mpi_errno != MPI_SUCCESS), mpi_errno, MPI_ERR_OTHER, "**openportfailed");

    err = MPIR_fd_send(fd, local_port, MPI_MAX_PORT_NAME);
    MPIR_ERR_CHKANDJUMP1((err != 0), mpi_errno, MPI_ERR_INTERN, "**join_send", "**join_send %d",
                         err);

    err = MPIR_fd_recv(fd, remote_port, MPI_MAX_PORT_NAME);
    MPIR_ERR_CHKANDJUMP1((err != 0), mpi_errno, MPI_ERR_INTERN, "**join_recv", "**join_recv %d",
                         err);

    MPIR_ERR_CHKANDJUMP2((strcmp(local_port, remote_port) == 0), mpi_errno, MPI_ERR_INTERN,
                         "**join_portname", "**join_portname %s %s", local_port, remote_port);

    if (strcmp(local_port, remote_port) < 0) {
        MPIR_Comm *comm_self_ptr;
        MPIR_Comm_get_ptr(MPI_COMM_SELF, comm_self_ptr);
        mpi_errno = MPIR_Comm_accept_impl(local_port, NULL, 0, comm_self_ptr, &intercomm_ptr);
        MPIR_ERR_CHECK(mpi_errno);
    } else {
        MPIR_Comm *comm_self_ptr;
        MPIR_Comm_get_ptr(MPI_COMM_SELF, comm_self_ptr);
        mpi_errno = MPIR_Comm_connect_impl(remote_port, NULL, 0, comm_self_ptr, &intercomm_ptr);
        MPIR_ERR_CHECK(mpi_errno);
    }

    mpi_errno = MPIR_Close_port_impl(local_port);
    MPIR_ERR_CHECK(mpi_errno);

    *p_intercomm_ptr = intercomm_ptr;

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Comm_spawn_impl(const char *command, char *argv[], int maxprocs, MPIR_Info * info_ptr,
                         int root, MPIR_Comm * comm_ptr, MPIR_Comm ** p_intercomm_ptr,
                         int array_of_errcodes[])
{
    return MPID_Comm_spawn_multiple(1, (char **) &command, &argv, &maxprocs, &info_ptr, root,
                                    comm_ptr, p_intercomm_ptr, array_of_errcodes);
}

int MPIR_Comm_spawn_multiple_impl(int count, char *array_of_commands[], char **array_of_argv[],
                                  const int array_of_maxprocs[], MPIR_Info * array_of_info_ptrs[],
                                  int root, MPIR_Comm * comm_ptr, MPIR_Comm ** p_intercomm_ptr,
                                  int array_of_errcodes[])
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPID_Comm_spawn_multiple(count, array_of_commands,
                                         array_of_argv,
                                         array_of_maxprocs,
                                         array_of_info_ptrs, root,
                                         comm_ptr, p_intercomm_ptr, array_of_errcodes);
    return mpi_errno;
}

int MPIR_Open_port_impl(MPIR_Info * info_ptr, char *port_name)
{
    return MPID_Open_port(info_ptr, port_name);
}

int MPIR_Close_port_impl(const char *port_name)
{
    return MPID_Close_port(port_name);
}

int MPIR_Comm_accept_impl(const char *port_name, MPIR_Info * info_ptr, int root,
                          MPIR_Comm * comm_ptr, MPIR_Comm ** newcomm_ptr)
{
    return MPID_Comm_accept(port_name, info_ptr, root, comm_ptr, newcomm_ptr);
}

int MPIR_Comm_connect_impl(const char *port_name, MPIR_Info * info_ptr, int root,
                           MPIR_Comm * comm_ptr, MPIR_Comm ** newcomm_ptr)
{
    return MPID_Comm_connect(port_name, info_ptr, root, comm_ptr, newcomm_ptr);
}

int MPIR_Comm_disconnect_impl(MPIR_Comm * comm_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    /*
     * Since outstanding I/O bumps the reference count on the communicator,
     * we wait until we hold the last reference count to
     * ensure that all communication has completed.  The reference count
     * is 1 when the communicator is created, and it is incremented
     * only for pending communication operations (and decremented when
     * those complete).
     */
    /* FIXME-MT should we be checking this? */
    if (MPIR_Object_get_ref(comm_ptr) > 1) {
        MPID_Progress_state progress_state;

        MPID_Progress_start(&progress_state);
        while (MPIR_Object_get_ref(comm_ptr) > 1) {
            mpi_errno = MPID_Progress_wait(&progress_state);
            /* --BEGIN ERROR HANDLING-- */
            if (mpi_errno != MPI_SUCCESS) {
                MPID_Progress_end(&progress_state);
                goto fn_fail;
            }
            /* --END ERROR HANDLING-- */
        }
        MPID_Progress_end(&progress_state);
    }

    mpi_errno = MPID_Comm_disconnect(comm_ptr);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* global for NAMEPUB_SERVICE */
MPID_NS_Handle MPIR_Namepub = 0;

int MPIR_Lookup_name_impl(const char *service_name, MPIR_Info * info_ptr, char *port_name)
{
    int mpi_errno = MPI_SUCCESS;
#ifdef HAVE_NAMEPUB_SERVICE
    if (!MPIR_Namepub) {
        mpi_errno = MPID_NS_Create(info_ptr, &MPIR_Namepub);
        /* FIXME: change **fail to something more meaningful */
        MPIR_ERR_CHKANDJUMP((mpi_errno != MPI_SUCCESS), mpi_errno, MPI_ERR_OTHER, "**fail");
        MPIR_Add_finalize((int (*)(void *)) MPID_NS_Free, &MPIR_Namepub, 9);
    }

    mpi_errno = MPID_NS_Lookup(MPIR_Namepub, info_ptr, (const char *) service_name, port_name);
    /* FIXME: change **fail to something more meaningful */
    /* Note: Jump on *any* error, not just errors other than MPI_ERR_NAME.
     * The usual MPI rules on errors apply - the error handler on the
     * communicator (file etc.) is invoked; MPI_COMM_WORLD is used
     * if there is no obvious communicator. A previous version of
     * this routine erroneously did not invoke the error handler
     * when the error was of class MPI_ERR_NAME. */
    MPIR_ERR_CHKANDJUMP(mpi_errno != MPI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**fail");
#else
    /* No name publishing service available */
    MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**nonamepub");
#endif

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Publish_name_impl(const char *service_name, MPIR_Info * info_ptr, const char *port_name)
{
    int mpi_errno = MPI_SUCCESS;

#ifdef HAVE_NAMEPUB_SERVICE
    {
        if (!MPIR_Namepub) {
            mpi_errno = MPID_NS_Create(info_ptr, &MPIR_Namepub);
            if (mpi_errno != MPI_SUCCESS)
                goto fn_fail;
            MPIR_Add_finalize((int (*)(void *)) MPID_NS_Free, &MPIR_Namepub, 9);
        }

        mpi_errno = MPID_NS_Publish(MPIR_Namepub, info_ptr,
                                    (const char *) service_name, (const char *) port_name);
        if (mpi_errno != MPI_SUCCESS)
            goto fn_fail;

    }
#else
    {
        /* No name publishing service available */
        MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**nonamepub");
    }
#endif

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Unpublish_name_impl(const char *service_name, MPIR_Info * info_ptr, const char *port_name)
{
    int mpi_errno = MPI_SUCCESS;

#ifdef HAVE_NAMEPUB_SERVICE
    {
        /* The standard leaves explicitly undefined what happens if the code
         * attempts to unpublish a name that is not published.  In this case,
         * MPI_Unpublish_name could be called before a name service structure
         * is allocated. */
        if (!MPIR_Namepub) {
            mpi_errno = MPID_NS_Create(info_ptr, &MPIR_Namepub);
            if (mpi_errno != MPI_SUCCESS)
                goto fn_fail;
            MPIR_Add_finalize((int (*)(void *)) MPID_NS_Free, &MPIR_Namepub, 9);
        }

        mpi_errno = MPID_NS_Unpublish(MPIR_Namepub, info_ptr, (const char *) service_name);
        if (mpi_errno != MPI_SUCCESS)
            goto fn_fail;

    }
#else
    {
        /* No name publishing service available */
        MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**nonamepub");
    }
#endif

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
