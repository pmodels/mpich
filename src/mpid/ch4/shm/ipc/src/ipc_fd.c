#include "mpidimpl.h"
#include "mpidu_init_shm.h"
#include "ipc_noinline.h"
#include "ipc_types.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

static int *MPIDI_IPCI_global_fd_socks;

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

    if ((flags & MSG_PEEK) == 0 && ((msg.msg_flags & MSG_TRUNC) || (msg.msg_flags & MSG_CTRUNC))) {
        mpi_errno = MPI_ERR_OTHER;
        goto fn_fail;
    }

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
