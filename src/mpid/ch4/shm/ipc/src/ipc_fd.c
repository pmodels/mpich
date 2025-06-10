#include "mpidimpl.h"
#include "mpidu_init_shm.h"
#include "ipc_noinline.h"
#include "ipc_types.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

/* This is defined as the max socket name length sun_path in sockaddr_un */
#define SOCK_MAX_STR_LEN 108

static int MPIDI_IPC_mpi_socks_init(MPIR_Comm * node_comm, pid_t * all_pids, int *fd_socks);
static int MPIDI_IPC_mpi_fd_init(MPIR_Comm * node_comm);
static int MPIDI_IPC_mpi_fd_cleanup(int local_size, int local_rank, pid_t * all_pids,
                                    int *fd_socks);
#ifdef MPL_HAVE_ZE
static int MPIDI_IPC_mpi_ze_fd_setup(int local_size, int local_rank, int *fd_socks);
static int MPIDI_IPC_mpi_fd_send(int sock, int fd, void *payload, size_t payload_len);
static int MPIDI_IPC_mpi_fd_recv(int sock, int *fd, void *payload, size_t payload_len, int flags);
#endif

static int ipc_fd_initialized = 0;

#define SET_SOCK_NAME(sock_name, pid, local_rank, remote_rank) \
    do { \
        snprintf(sock_name, SOCK_MAX_STR_LEN, "/tmp/mpich-ipc-fd-sock-%d:%d-%d", pid, local_rank, remote_rank); \
    } while (0)

int MPIDI_FD_comm_bootstrap(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;

    if (MPL_gpu_info.ipc_handle_type != MPL_GPU_IPC_HANDLE_SHAREABLE_FD) {
        goto fn_exit;
    }

    if (MPIR_CVAR_CH4_IPC_ZE_SHAREABLE_HANDLE != MPIR_CVAR_CH4_IPC_ZE_SHAREABLE_HANDLE_drmfd) {
        goto fn_exit;
    }

    MPIR_Comm *node_comm = MPIR_Comm_get_node_comm(comm);

    if (node_comm->local_size == 1) {
        goto fn_exit;
    }

    int already_initialized;
    mpi_errno = MPIR_Allreduce_impl(&ipc_fd_initialized, &already_initialized, 1, MPIR_INT_INTERNAL,
                                    MPI_MAX, node_comm, MPIR_ERR_NONE);
    MPIR_ERR_CHECK(mpi_errno);

    if (already_initialized) {
        /* we can't do repeated fd sharing */
        mpi_errno = MPI_ERR_OTHER;
        MPIR_ERR_CHECK(mpi_errno);
    }

    mpi_errno = MPIDI_IPC_mpi_fd_init(node_comm);
    MPIR_ERR_CHECK(mpi_errno);

    ipc_fd_initialized = 1;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int MPIDI_IPC_mpi_fd_cleanup(int local_size, int local_rank, pid_t * all_pids, int *fd_socks)
{
    int mpi_errno = MPI_SUCCESS;
    int sock_err;
    char sock_name[SOCK_MAX_STR_LEN];
    char strerrbuf[MPIR_STRERROR_BUF_SIZE] ATTRIBUTE((unused));
    int i;

    /* Close sockets */
    for (i = 0; i < local_size; ++i) {
        if (fd_socks[i] != -1) {
            sock_err = close(fd_socks[i]);
            MPIR_ERR_CHKANDJUMP2(sock_err == -1, mpi_errno, MPI_ERR_OTHER, "**sock_close",
                                 "**sock_close %s %d",
                                 MPIR_Strerror(errno, strerrbuf, MPIR_STRERROR_BUF_SIZE), errno);
        }

        if (local_rank != i) {
            SET_SOCK_NAME(sock_name, all_pids[local_rank], local_rank, i);
            sock_err = unlink(sock_name);
            MPIR_ERR_CHKANDJUMP2(sock_err == -1, mpi_errno, MPI_ERR_OTHER, "**sock_unlink",
                                 "**sock_unlink %s %d",
                                 MPIR_Strerror(errno, strerrbuf, MPIR_STRERROR_BUF_SIZE), errno);
        }
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int MPIDI_IPC_mpi_socks_init(MPIR_Comm * node_comm, pid_t * all_pids, int *fd_socks)
{
    int mpi_errno = MPI_SUCCESS;

    int local_size = node_comm->local_size;
    int local_rank = node_comm->rank;

    /* Create servers for lower ranks */
    for (int j = 0; j < local_rank; j++) {
        int sock_err;
        int sock;
        struct sockaddr_un sockaddr;
        char sock_name[SOCK_MAX_STR_LEN];
        char strerrbuf[MPIR_STRERROR_BUF_SIZE] ATTRIBUTE((unused));

        /* Create the local socket name */
        SET_SOCK_NAME(sock_name, all_pids[local_rank], local_rank, j);

        /* Create a socket for local rank i */
        sock = socket(AF_UNIX, SOCK_STREAM, 0);
        MPIR_ERR_CHKANDJUMP2(sock == -1, mpi_errno, MPI_ERR_OTHER, "**sock_create",
                             "**sock_create %s %d",
                             MPIR_Strerror(errno, strerrbuf, MPIR_STRERROR_BUF_SIZE), errno);

        int enable = 1;
        setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
        memset(&sockaddr, 0, sizeof(sockaddr));
        sockaddr.sun_family = AF_UNIX;
        strcpy(sockaddr.sun_path, sock_name);

        sock_err = bind(sock, (struct sockaddr *) &sockaddr, sizeof(sockaddr));
        MPIR_ERR_CHKANDJUMP2(sock_err == -1, mpi_errno, MPI_ERR_OTHER, "**bind", "**bind %s %d",
                             MPIR_Strerror(errno, strerrbuf, MPIR_STRERROR_BUF_SIZE), errno);
        fd_socks[j] = sock;

        /* Listen to the socket to accept a connection to the other process */
        sock_err = listen(sock, 3);
        MPIR_ERR_CHKANDJUMP2(sock_err == -1, mpi_errno, MPI_ERR_OTHER, "**listen",
                             "**listen %s %d", MPIR_Strerror(errno, strerrbuf,
                                                             MPIR_STRERROR_BUF_SIZE), errno);
    }

    mpi_errno = MPIR_Barrier_impl(node_comm, MPIR_ERR_NONE);
    MPIR_ERR_CHECK(mpi_errno);

    /* create clients for higher ranks */
    for (int i = local_rank + 1; i < local_size; ++i) {
        int sock_err;
        char sock_name[SOCK_MAX_STR_LEN];
        char remote_sock_name[SOCK_MAX_STR_LEN];
        struct sockaddr_un sockaddr, remote_sockaddr;
        char strerrbuf[MPIR_STRERROR_BUF_SIZE] ATTRIBUTE((unused));

        /* Create the remote socket name */
        SET_SOCK_NAME(remote_sock_name, all_pids[i], i, local_rank);

        /* Create the local socket name */
        SET_SOCK_NAME(sock_name, all_pids[local_rank], local_rank, i);

        /* Create a socket for local rank j */
        fd_socks[i] = socket(AF_UNIX, SOCK_STREAM, 0);
        MPIR_ERR_CHKANDJUMP2(fd_socks[i] == -1, mpi_errno, MPI_ERR_OTHER,
                             "**sock_create", "**sock_create %s %d",
                             MPIR_Strerror(errno, strerrbuf, MPIR_STRERROR_BUF_SIZE), errno);

        int enable = 1;
        setsockopt(fd_socks[i], SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
        memset(&sockaddr, 0, sizeof(sockaddr));
        sockaddr.sun_family = AF_UNIX;
        strcpy(sockaddr.sun_path, sock_name);

        sock_err = bind(fd_socks[i], (struct sockaddr *) &sockaddr, sizeof(sockaddr));
        MPIR_ERR_CHKANDJUMP2(sock_err == -1, mpi_errno, MPI_ERR_OTHER, "**bind",
                             "**bind %s %d", MPIR_Strerror(errno, strerrbuf,
                                                           MPIR_STRERROR_BUF_SIZE), errno);

        /* Connect to remote socket for local rank j */
        memset(&remote_sockaddr, 0, sizeof(sockaddr));
        remote_sockaddr.sun_family = AF_UNIX;
        strcpy(remote_sockaddr.sun_path, remote_sock_name);

        sock_err = connect(fd_socks[i], (struct sockaddr *) &remote_sockaddr, sizeof(sockaddr));
        MPIR_ERR_CHKANDJUMP2(sock_err < 0, mpi_errno, MPI_ERR_OTHER, "**sock_connect",
                             "sock_connect %d %s", errno, MPIR_Strerror(errno, strerrbuf,
                                                                        MPIR_STRERROR_BUF_SIZE));
    }

    /* Servers accept connections */
    for (int j = 0; j < local_rank; j++) {
        int sock;
        struct sockaddr_un sockaddr;
        char strerrbuf[MPIR_STRERROR_BUF_SIZE] ATTRIBUTE((unused));

        sock = fd_socks[j];
        memset(&sockaddr, 0, sizeof(sockaddr));
        socklen_t len = sizeof(sockaddr);
        fd_socks[j] = accept(sock, (struct sockaddr *) &sockaddr, &len);
        MPIR_ERR_CHKANDJUMP2(fd_socks[j] == -1, mpi_errno, MPI_ERR_OTHER,
                             "**sock_accept", "**sock_accept %s %d",
                             MPIR_Strerror(errno, strerrbuf, MPIR_STRERROR_BUF_SIZE), errno);
        int enable = 1;
        setsockopt(fd_socks[j], SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
        close(sock);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    /* TODO: cleanup */
    goto fn_exit;
}

static int MPIDI_IPC_mpi_fd_init(MPIR_Comm * node_comm)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_CHKLMEM_DECL();

    int local_size = node_comm->local_size;
    int local_rank = node_comm->rank;

    /* Allgather pids -- used to locally generate named-socket locations */
    pid_t *all_pids;
    MPIR_CHKLMEM_MALLOC(all_pids, local_size * sizeof(pid_t));

    all_pids[local_rank] = getpid();
    mpi_errno = MPIR_Allgather_impl(MPI_IN_PLACE, sizeof(pid_t), MPIR_BYTE_INTERNAL,
                                    all_pids, sizeof(pid_t), MPIR_BYTE_INTERNAL,
                                    node_comm, MPIR_ERR_NONE);
    MPIR_ERR_CHECK(mpi_errno);

    /* Initialize fd_socks */
    int *fd_socks;
    MPIR_CHKLMEM_MALLOC(fd_socks, local_size * sizeof(pid_t));
    for (int i = 0; i < local_size; i++) {
        fd_socks[i] = -1;
    }

    mpi_errno = MPIDI_IPC_mpi_socks_init(node_comm, all_pids, fd_socks);
    MPIR_ERR_CHECK(mpi_errno);

    /* Only ZE will set MPL_GPU_IPC_HANDLE_SHAREABLE_FD and reach here. */
#ifdef MPL_HAVE_ZE
    mpi_errno = MPIDI_IPC_mpi_ze_fd_setup(local_size, local_rank, fd_socks);
    MPIR_ERR_CHECK(mpi_errno);
#else
    MPIR_Assert(0);
#endif

    /* Cleanup the sockets upon completion of init - they are no longer needed */
    /* close client first, and then server to avoid the TIME_WAIT */
    if (node_comm->rank == 0) {
        mpi_errno = MPIDI_IPC_mpi_fd_cleanup(local_size, local_rank, all_pids, fd_socks);
        MPIR_ERR_CHECK(mpi_errno);
        mpi_errno = MPIR_Barrier_impl(node_comm, MPIR_ERR_NONE);
        MPIR_ERR_CHECK(mpi_errno);
    } else {
        mpi_errno = MPIR_Barrier_impl(node_comm, MPIR_ERR_NONE);
        MPIR_ERR_CHECK(mpi_errno);
        mpi_errno = MPIDI_IPC_mpi_fd_cleanup(local_size, local_rank, all_pids, fd_socks);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#ifdef MPL_HAVE_ZE

static int MPIDI_IPC_mpi_ze_fd_setup(int local_size, int local_rank, int *fd_socks)
{
    int mpi_errno = MPI_SUCCESS;

    int num_fds, i, r, mpl_err = MPL_SUCCESS;
    int *fds, *bdfs;

    /* Get the number of ze devices */
    mpl_err = MPL_ze_init_device_fds(&num_fds, NULL, NULL);
    MPIR_ERR_CHKANDJUMP(mpl_err != MPL_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                        "**mpl_ze_init_device_fds");

    fds = (int *) MPL_malloc(num_fds * sizeof(int), MPL_MEM_OTHER);
    MPIR_ERR_CHKANDJUMP(!fds, mpi_errno, MPI_ERR_OTHER, "**nomem");

    bdfs = (int *) MPL_malloc(num_fds * 4 * sizeof(int), MPL_MEM_OTHER);
    MPIR_ERR_CHKANDJUMP(!bdfs, mpi_errno, MPI_ERR_OTHER, "**nomem");

    if (local_rank == 0) {
        /* Setup the device fds */
        mpl_err = MPL_ze_init_device_fds(&num_fds, fds, bdfs);
        MPIR_ERR_CHKANDJUMP(mpl_err != MPL_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                            "**mpl_ze_init_device_fds");

        /* Send the fds to all other local processes */
        for (r = 1; r < local_size; r++) {
            MPIDI_IPC_mpi_fd_send(fd_socks[r], fds[0], bdfs, 4 * num_fds * sizeof(int));
            for (i = 1; i < num_fds; i++) {
                MPIDI_IPC_mpi_fd_send(fd_socks[r], fds[i], NULL, 0);
            }
        }
    } else {
        /* Receive the fds from local process 0 */
        MPIDI_IPC_mpi_fd_recv(fd_socks[0], fds, bdfs, 4 * num_fds * sizeof(int), 0);
        for (i = 1; i < num_fds; i++) {
            MPIDI_IPC_mpi_fd_recv(fd_socks[0], fds + i, NULL, 0, 0);
        }
    }

    /* Save the fds in MPL */
    MPL_ze_set_fds(num_fds, fds, bdfs);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int MPIDI_IPC_mpi_fd_send(int sock, int fd, void *payload, size_t payload_len)
{
    int mpi_errno = MPI_SUCCESS;
    int sock_err;
    struct msghdr msg;
    struct cmsghdr *cmsg;
    struct iovec iov;
    char ctrl_buf[CMSG_SPACE(sizeof(fd))];
    char strerrbuf[MPIR_STRERROR_BUF_SIZE] ATTRIBUTE((unused));
    char empty_buf;

    /* Setup a payload if provided */
    if (payload == NULL) {
        iov.iov_base = &empty_buf;
        iov.iov_len = 1;
    } else {
        iov.iov_base = payload;
        iov.iov_len = payload_len;
    }

    /* Setup the message header */
    memset(&msg, 0, sizeof(msg));

    msg.msg_control = ctrl_buf;
    msg.msg_controllen = CMSG_LEN(sizeof(fd));
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

    /* Setup the control message, which includes the fd */
    cmsg = CMSG_FIRSTHDR(&msg);
    cmsg->cmsg_len = CMSG_LEN(sizeof(fd));
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    memcpy(CMSG_DATA(cmsg), &fd, sizeof(fd));

    /* Send the message with the fd */
    sock_err = sendmsg(sock, &msg, 0);
    MPIR_ERR_CHKANDJUMP2(sock_err == -1, mpi_errno, MPI_ERR_OTHER, "**sendmsg", "**sendmsg %s %d",
                         MPIR_Strerror(errno, strerrbuf, MPIR_STRERROR_BUF_SIZE), errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int MPIDI_IPC_mpi_fd_recv(int sock, int *fd, void *payload, size_t payload_len, int flags)
{
    int mpi_errno = MPI_SUCCESS;
    int sock_err;
    struct msghdr msg;
    struct cmsghdr *cmsg;
    struct iovec iov;
    char ctrl_buf[CMSG_SPACE(sizeof(*fd))];
    char strerrbuf[MPIR_STRERROR_BUF_SIZE] ATTRIBUTE((unused));
    char empty_buf;
    bool trunc_check;

    /* Setup a payload if provided */
    if (payload == NULL) {
        iov.iov_base = &empty_buf;
        iov.iov_len = 1;
    } else {
        iov.iov_base = payload;
        iov.iov_len = payload_len;
    }

    /* Setup the message header */
    memset(&msg, 0, sizeof(msg));

    msg.msg_control = ctrl_buf;
    msg.msg_controllen = CMSG_LEN(sizeof(*fd));
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

    if (flags & MSG_PEEK) {
        /* Use regular recv for MSG_PEEK so that an FD isn't created if it's not the matching message */
        sock_err = recv(sock, iov.iov_base, iov.iov_len, flags);
        MPIR_ERR_CHKANDJUMP2(sock_err == -1, mpi_errno, MPI_ERR_OTHER, "**recv", "**recv %s %d",
                             MPIR_Strerror(errno, strerrbuf, MPIR_STRERROR_BUF_SIZE), errno);
    } else {
        /* Receive the message with the fd */
        sock_err = recvmsg(sock, &msg, flags);
        MPIR_ERR_CHKANDJUMP2(sock_err == -1, mpi_errno, MPI_ERR_OTHER, "**recvmsg",
                             "**recvmsg %s %d", MPIR_Strerror(errno, strerrbuf,
                                                              MPIR_STRERROR_BUF_SIZE), errno);
    }

    trunc_check = ((flags & MSG_PEEK) == 0) && ((msg.msg_flags & MSG_TRUNC) ||
                                                (msg.msg_flags & MSG_CTRUNC));
    MPIR_ERR_CHKANDJUMP(trunc_check, mpi_errno, MPI_ERR_OTHER, "**truncate");

    if ((flags & MSG_PEEK) == 0) {
        for (cmsg = CMSG_FIRSTHDR(&msg); cmsg != NULL; cmsg = CMSG_NXTHDR(&msg, cmsg)) {
            if (cmsg->cmsg_len == CMSG_LEN(sizeof(*fd)) &&
                cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == SCM_RIGHTS) {
                memcpy(fd, CMSG_DATA(cmsg), sizeof(int));
                break;
            }
        }
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif /* MPL_HAVE_ZE */
