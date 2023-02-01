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

static int *MPIDI_IPCI_global_fd_socks;
static pid_t *MPIDI_IPCI_global_fd_pids;

int MPIDI_FD_mpi_init_hook(void)
{
    int mpi_errno = MPI_SUCCESS;

    if (MPL_gpu_info.ipc_handle_type == MPL_GPU_IPC_HANDLE_SHAREABLE_FD) {
        bool use_drmfd = (MPIR_CVAR_CH4_IPC_ZE_SHAREABLE_HANDLE ==
                          MPIR_CVAR_CH4_IPC_ZE_SHAREABLE_HANDLE_drmfd);

        mpi_errno = MPIDI_IPC_mpi_fd_init(use_drmfd);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    MPIDI_FD_mpi_finalize_hook();
    goto fn_exit;
}

int MPIDI_FD_mpi_finalize_hook(void)
{
    int mpi_errno = MPI_SUCCESS;

    if (MPL_gpu_info.ipc_handle_type == MPL_GPU_IPC_HANDLE_SHAREABLE_FD) {
        bool use_drmfd = (MPIR_CVAR_CH4_IPC_ZE_SHAREABLE_HANDLE ==
                          MPIR_CVAR_CH4_IPC_ZE_SHAREABLE_HANDLE_drmfd);

        mpi_errno = MPIDI_IPC_mpi_fd_finalize(use_drmfd);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int MPIDI_IPC_mpi_fd_cleanup(void)
{
    int mpi_errno = MPI_SUCCESS;
    int sock_err;
    char sock_name[SOCK_MAX_STR_LEN];
    char strerrbuf[MPIR_STRERROR_BUF_SIZE] ATTRIBUTE((unused));
    int i;

    /* Close sockets */
    for (i = 0; i < MPIR_Process.local_size; ++i) {
        if (MPIDI_IPCI_global_fd_socks[i] != -1) {
            sock_err = close(MPIDI_IPCI_global_fd_socks[i]);
            MPIR_ERR_CHKANDJUMP2(sock_err == -1, mpi_errno, MPI_ERR_OTHER, "**sock_close",
                                 "**sock_close %s %d",
                                 MPIR_Strerror(errno, strerrbuf, MPIR_STRERROR_BUF_SIZE), errno);
        }

        if (MPIDI_IPCI_global_fd_pids[MPIR_Process.local_rank] != 0 && MPIR_Process.local_rank != i) {
            snprintf(sock_name, SOCK_MAX_STR_LEN, "/tmp/mpich-ipc-fd-sock-%d:%d-%d",
                     MPIDI_IPCI_global_fd_pids[MPIR_Process.local_rank],
                     MPIR_Process.local_rank, i);
            sock_err = unlink(sock_name);
            MPIR_ERR_CHKANDJUMP2(sock_err == -1, mpi_errno, MPI_ERR_OTHER, "**sock_unlink",
                                 "**sock_unlink %s %d",
                                 MPIR_Strerror(errno, strerrbuf, MPIR_STRERROR_BUF_SIZE), errno);
        }
    }

    MPL_free(MPIDI_IPCI_global_fd_pids);
    MPL_free(MPIDI_IPCI_global_fd_socks);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int MPIDI_IPC_mpi_ze_fd_setup(void)
{
    int mpi_errno = MPI_SUCCESS;

#if defined(MPL_HAVE_ZE)
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

    if (MPIR_Process.local_rank == 0) {
        /* Setup the device fds */
        mpl_err = MPL_ze_init_device_fds(&num_fds, fds, bdfs);
        MPIR_ERR_CHKANDJUMP(mpl_err != MPL_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                            "**mpl_ze_init_device_fds");

        /* Send the fds to all other local processes */
        for (r = 1; r < MPIR_Process.local_size; r++) {
            MPIDI_IPC_mpi_fd_send(r, fds[0], bdfs, 4 * num_fds * sizeof(int));
            for (i = 1; i < num_fds; i++) {
                MPIDI_IPC_mpi_fd_send(r, fds[i], NULL, 0);
            }
        }
    } else {
        /* Receive the fds from local process 0 */
        MPIDI_IPC_mpi_fd_recv(0, fds, bdfs, 4 * num_fds * sizeof(int), 0);
        for (i = 1; i < num_fds; i++) {
            MPIDI_IPC_mpi_fd_recv(0, fds + i, NULL, 0, 0);
        }
    }

    /* Save the fds in MPL */
    MPL_ze_set_fds(num_fds, fds, bdfs);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
#else
    return mpi_errno;
#endif
}

int MPIDI_IPC_mpi_socks_init(void)
{
    int mpi_errno = MPI_SUCCESS;
    int sock_err;
    int i, fd_count;
    unsigned int len;
    struct sockaddr_un sockaddr;
    char sock_name[SOCK_MAX_STR_LEN];
    char strerrbuf[MPIR_STRERROR_BUF_SIZE] ATTRIBUTE((unused));
    pid_t pid;
    int enable = 1;

    memset(&sockaddr, 0, sizeof(sockaddr));

    fd_count = MPIR_Process.local_size;
    MPIDI_IPCI_global_fd_socks = (int *) MPL_calloc(fd_count, sizeof(int), MPL_MEM_OTHER);
    MPIR_ERR_CHKANDJUMP(!MPIDI_IPCI_global_fd_socks, mpi_errno, MPI_ERR_OTHER, "**nomem");

    MPIDI_IPCI_global_fd_pids = (pid_t *) MPL_calloc(fd_count, sizeof(pid_t), MPL_MEM_OTHER);
    MPIR_ERR_CHKANDJUMP(!MPIDI_IPCI_global_fd_pids, mpi_errno, MPI_ERR_OTHER, "**nomem");

    for (i = 0; i < fd_count; i++) {
        MPIDI_IPCI_global_fd_socks[i] = -1;
    }

    pid = getpid();
    len = sizeof(sockaddr);

    /* Send PID in order to locally generate named-socket locations */
    mpi_errno = MPIDU_Init_shm_put(&pid, sizeof(pid_t));
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIDU_Init_shm_barrier();
    MPIR_ERR_CHECK(mpi_errno);

    for (i = 0; i < MPIR_Process.local_size; ++i) {
        mpi_errno = MPIDU_Init_shm_get(i, sizeof(pid_t), &MPIDI_IPCI_global_fd_pids[i]);
        MPIR_ERR_CHECK(mpi_errno);
    }

    /* Create servers for lower ranks */
    for (int j = 0; j < MPIR_Process.local_rank; j++) {
        int sock;

        /* Create the local socket name */
        snprintf(sock_name, SOCK_MAX_STR_LEN, "/tmp/mpich-ipc-fd-sock-%d:%d-%d", pid,
                 MPIR_Process.local_rank, j);

        /* Create a socket for local rank i */
        sock = socket(AF_UNIX, SOCK_STREAM, 0);
        MPIR_ERR_CHKANDJUMP2(sock == -1, mpi_errno, MPI_ERR_OTHER, "**sock_create",
                             "**sock_create %s %d",
                             MPIR_Strerror(errno, strerrbuf, MPIR_STRERROR_BUF_SIZE), errno);

        setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
        memset(&sockaddr, 0, sizeof(sockaddr));
        sockaddr.sun_family = AF_UNIX;
        strcpy(sockaddr.sun_path, sock_name);

        sock_err = bind(sock, (struct sockaddr *) &sockaddr, len);
        MPIR_ERR_CHKANDJUMP2(sock_err == -1, mpi_errno, MPI_ERR_OTHER, "**bind", "**bind %s %d",
                             MPIR_Strerror(errno, strerrbuf, MPIR_STRERROR_BUF_SIZE), errno);
        MPIDI_IPCI_global_fd_socks[j] = sock;

        /* Listen to the socket to accept a connection to the other process */
        sock_err = listen(sock, 3);
        MPIR_ERR_CHKANDJUMP2(sock_err == -1, mpi_errno, MPI_ERR_OTHER, "**listen",
                             "**listen %s %d", MPIR_Strerror(errno, strerrbuf,
                                                             MPIR_STRERROR_BUF_SIZE), errno);
    }

    mpi_errno = MPIDU_Init_shm_barrier();

    /* create clients for higher ranks */
    for (i = MPIR_Process.local_rank + 1; i < MPIR_Process.local_size; ++i) {
        char remote_sock_name[SOCK_MAX_STR_LEN];
        struct sockaddr_un remote_sockaddr;

        /* Create the remote socket name */
        snprintf(remote_sock_name, SOCK_MAX_STR_LEN, "/tmp/mpich-ipc-fd-sock-%d:%d-%d",
                 MPIDI_IPCI_global_fd_pids[i], i, MPIR_Process.local_rank);

        /* Create the local socket name */
        snprintf(sock_name, SOCK_MAX_STR_LEN, "/tmp/mpich-ipc-fd-sock-%d:%d-%d", pid,
                 MPIR_Process.local_rank, i);

        /* Create a socket for local rank j */
        MPIDI_IPCI_global_fd_socks[i] = socket(AF_UNIX, SOCK_STREAM, 0);
        MPIR_ERR_CHKANDJUMP2(MPIDI_IPCI_global_fd_socks[i] == -1, mpi_errno, MPI_ERR_OTHER,
                             "**sock_create", "**sock_create %s %d",
                             MPIR_Strerror(errno, strerrbuf, MPIR_STRERROR_BUF_SIZE), errno);

        setsockopt(MPIDI_IPCI_global_fd_socks[i], SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
        memset(&sockaddr, 0, sizeof(sockaddr));
        sockaddr.sun_family = AF_UNIX;
        strcpy(sockaddr.sun_path, sock_name);

        sock_err = bind(MPIDI_IPCI_global_fd_socks[i], (struct sockaddr *) &sockaddr, len);
        MPIR_ERR_CHKANDJUMP2(sock_err == -1, mpi_errno, MPI_ERR_OTHER, "**bind",
                             "**bind %s %d", MPIR_Strerror(errno, strerrbuf,
                                                           MPIR_STRERROR_BUF_SIZE), errno);

        /* Connect to remote socket for local rank j */
        memset(&remote_sockaddr, 0, sizeof(sockaddr));
        remote_sockaddr.sun_family = AF_UNIX;
        strcpy(remote_sockaddr.sun_path, remote_sock_name);

        sock_err =
            connect(MPIDI_IPCI_global_fd_socks[i], (struct sockaddr *) &remote_sockaddr, len);
        MPIR_ERR_CHKANDJUMP2(sock_err < 0, mpi_errno, MPI_ERR_OTHER, "**sock_connect",
                             "sock_connect %d %s", errno, MPIR_Strerror(errno, strerrbuf,
                                                                        MPIR_STRERROR_BUF_SIZE));
    }

    /* Servers accept connections */
    for (int j = 0; j < MPIR_Process.local_rank; j++) {
        int sock;
        sock = MPIDI_IPCI_global_fd_socks[j];
        memset(&sockaddr, 0, sizeof(sockaddr));
        MPIDI_IPCI_global_fd_socks[j] = accept(sock, (struct sockaddr *) &sockaddr, &len);
        MPIR_ERR_CHKANDJUMP2(MPIDI_IPCI_global_fd_socks[j] == -1, mpi_errno, MPI_ERR_OTHER,
                             "**sock_accept", "**sock_accept %s %d",
                             MPIR_Strerror(errno, strerrbuf, MPIR_STRERROR_BUF_SIZE), errno);
        setsockopt(MPIDI_IPCI_global_fd_socks[j], SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
        close(sock);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    MPIDI_IPC_mpi_fd_finalize(true);
    goto fn_exit;
}

int MPIDI_IPC_mpi_fd_init(bool use_drmfd)
{
    int mpi_errno = MPI_SUCCESS;

    if (!use_drmfd)
        goto fn_exit;

    mpi_errno = MPIDI_IPC_mpi_socks_init();
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIDI_IPC_mpi_ze_fd_setup();
    MPIR_ERR_CHECK(mpi_errno);

    /* Cleanup the sockets upon completion of init - they are no longer needed */
    /* close client first, and then server to avoid the TIME_WAIT */
    if (MPIR_Process.local_rank == 0) {
        mpi_errno = MPIDI_IPC_mpi_fd_cleanup();
        MPIR_ERR_CHECK(mpi_errno);
        mpi_errno = MPIDU_Init_shm_barrier();
        MPIR_ERR_CHECK(mpi_errno);
    } else {
        mpi_errno = MPIDU_Init_shm_barrier();
        MPIR_ERR_CHECK(mpi_errno);
        mpi_errno = MPIDI_IPC_mpi_fd_cleanup();
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_IPC_mpi_fd_finalize(bool use_drmfd)
{
    return MPI_SUCCESS;
}

int MPIDI_IPC_mpi_fd_send(int rank, int fd, void *payload, size_t payload_len)
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
    sock_err = sendmsg(MPIDI_IPCI_global_fd_socks[rank], &msg, 0);
    MPIR_ERR_CHKANDJUMP2(sock_err == -1, mpi_errno, MPI_ERR_OTHER, "**sendmsg", "**sendmsg %s %d",
                         MPIR_Strerror(errno, strerrbuf, MPIR_STRERROR_BUF_SIZE), errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_IPC_mpi_fd_recv(int rank, int *fd, void *payload, size_t payload_len, int flags)
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
        sock_err = recv(MPIDI_IPCI_global_fd_socks[rank], iov.iov_base, iov.iov_len, flags);
        MPIR_ERR_CHKANDJUMP2(sock_err == -1, mpi_errno, MPI_ERR_OTHER, "**recv", "**recv %s %d",
                             MPIR_Strerror(errno, strerrbuf, MPIR_STRERROR_BUF_SIZE), errno);
    } else {
        /* Receive the message with the fd */
        sock_err = recvmsg(MPIDI_IPCI_global_fd_socks[rank], &msg, flags);
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
