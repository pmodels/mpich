/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/*
 * The routines in this file provide an event-driven I/O handler
 *
 * Each active fd has a handler associated with it.
 */
/* FIXME: Occasionally, data from stdout has been lost when the job is
   exiting.  I don't know whether data is being lost because the writer
   is discarding it or the reader (mpiexec) is failing to finish reading from
   all of the sockets before exiting.
 */
#include "mpichconf.h"
#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_POLL_H
#include <poll.h>
#endif
#ifdef HAVE_TIME_H
#include <time.h>
#endif
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif
#include "pmutil.h"
#include "ioloop.h"

/*
 * To simplify mapping fds back to their handlers, we store the handles
 * in an array such that the ith element of the array corresponds to the
 * fd with value i (e.g., for fd == 10, the [10] element of the array
 * has the information on the handler).  This isn't terrifically scalable,
 * but it makes this code fairly simple and this code isn't
 * performance sensitive.  "maxFD" is the maximum fd value seen; this
 * allows us to allocate a large array but usually only look at a small
 * part of it.
 */
#define MAXFD 4096
static IOHandle handlesByFD[MAXFD + 1];
static int maxFD = -1;

/*@
  MPIE_IORegister - Register a handler for an FD

Input Parameters:

  Notes:
  Keeps track of the largest fd seen (in 'maxFD').
  @*/
int MPIE_IORegister(int fd, int rdwr, int (*handler) (int, int, void *), void *extra_data)
{
    int i;

    if (fd > MAXFD) {
        /* Error; fd is too large */
        return 1;
    }

    /* Remember the largest set FD, and clear any FDs between this
     * fd and the last maximum */
    if (fd > maxFD) {
        for (i = maxFD + 1; i < fd; i++) {
            handlesByFD[i].fd = -1;
            handlesByFD[i].handler = 0;
        }
        maxFD = fd;
    }
    handlesByFD[fd].fd = fd;
    handlesByFD[fd].rdwr = rdwr;
    handlesByFD[fd].handler = handler;
    handlesByFD[fd].extra_data = extra_data;

    return 0;
}

/*@
  MPIE_IODeregister - Remove a handler for an FD

Input Parameters:
. fd - fd to deregister
  @*/
int MPIE_IODeregister(int fd)
{
    int i;
    int newMaxFd;

    if (fd > MAXFD) {
        /* Error; fd is too large */
        return 1;
    }
    if (fd > maxFD) {
        /* Error; fd is unknown */
        return 1;
    }

    /* Recompute the new maxfd */
    newMaxFd = -1;
    for (i = 0; i <= maxFD; i++) {
        if (handlesByFD[i].fd >= 0 && i > newMaxFd) {
            newMaxFd = i;
        }
    }
    maxFD = newMaxFd;

    handlesByFD[fd].fd = -1;
    handlesByFD[fd].rdwr = 0;
    handlesByFD[fd].handler = 0;
    handlesByFD[fd].extra_data = 0;

    return 0;
}

/*@
  MPIE_IOLoop - Handle all registered I/O

Input Parameters:
.  timeoutSeconds - Seconds until this routine should return with a
   timeout error.  If negative, no timeout.  If 0, return immediatedly
   after a nonblocking check for I/O.

   Return Value:
   Returns zero on success.  Returns 'IOLOOP_TIMEOUT' if the timeout
   is reached and 'IOLOOP_ERROR' on other errors.
@*/
int MPIE_IOLoop(int timeoutSeconds)
{
    int i, fd, nfds, npollfds, rc = 0, rc2;
    int (*handler) (int, int, void *);
    struct pollfd pollfds[MAXFD];
    int pollfd_to_fd[MAXFD];    /* map pollfd index back to fd */

    /* Loop on the fds, with the timeout */
    TimeoutInit(timeoutSeconds);
    while (1) {
        int timeout_ms;
        int time_left = TimeoutGetRemaining();
        if (time_left >= 1000000)
            timeout_ms = -1;    /* no timeout */
        else
            timeout_ms = time_left * 1000;

        /* Build the pollfd array from registered handlers */
        npollfds = 0;
        for (i = 0; i <= maxFD; i++) {
            if (handlesByFD[i].handler) {
                short events = 0;
                if (handlesByFD[i].rdwr & IO_READ)
                    events |= POLLIN;
                if (handlesByFD[i].rdwr & IO_WRITE)
                    events |= POLLOUT;
                if (events) {
                    pollfds[npollfds].fd = handlesByFD[i].fd;
                    pollfds[npollfds].events = events;
                    pollfds[npollfds].revents = 0;
                    pollfd_to_fd[npollfds] = handlesByFD[i].fd;
                    npollfds++;
                }
            }
        }
        if (npollfds == 0)
            break;

        MPIE_SYSCALL(nfds, poll, (pollfds, npollfds, timeout_ms));
        if (nfds < 0 && (errno == EINTR || errno == 0)) {
            /* Continuing through EINTR */
            /* We allow errno == 0 as a synonym for EINTR.  We've seen this
             * on Solaris; in addition, we set errno to 0 after a failed
             * waitpid in the process routines, and if the OS isn't careful,
             * the value of errno may get ECHILD instead of EINTR when the
             * signal handler returns (we suspect Linux of this problem),
             * which is why we have the signal handler in process.c reset
             * errno to 0 (we may need to allow ECHILD here (!)) */
            /* FIXME: an EINTR may also mean that a process has exited
             * (SIGCHILD).  If all processes have exited, we may want to
             * exit */
            DBG_PRINTF(("errno = EINTR in poll\n"));
            continue;
        }
        if (nfds < 0) {
            /* Serious error */
            MPL_internal_sys_error_printf("poll", errno, 0);
            break;
        }
        if (nfds == 0) {
            /* Timeout from poll */
            DBG_PRINTF(("Timeout in poll\n"));
            return IOLOOP_TIMEOUT;
        }
        /* nfds > 0 */
        DBG_PRINTF(("Found some fds to process (n = %d)\n", nfds));
        for (i = 0; i < npollfds; i++) {
            if (pollfds[i].revents == 0)
                continue;
            fd = pollfd_to_fd[i];
            if (pollfds[i].revents & (POLLOUT | POLLERR | POLLHUP)) {
                handler = handlesByFD[fd].handler;
                if (handler) {
                    rc = (*handler) (fd, IO_WRITE, handlesByFD[fd].extra_data);
                }
                if (rc == 1) {
                    MPIE_SYSCALL(rc2, close, (fd));
                    handlesByFD[fd].rdwr = 0;
                    continue;
                }
            }
            if (pollfds[i].revents & (POLLIN | POLLERR | POLLHUP)) {
                handler = handlesByFD[fd].handler;
                if (handler) {
                    rc = (*handler) (fd, IO_READ, handlesByFD[fd].extra_data);
                }
                if (rc == 1) {
                    MPIE_SYSCALL(rc2, close, (fd));
                    handlesByFD[fd].rdwr = 0;
                }
            }
        }
    }
    DBG_PRINTF(("Returning from IOLOOP handler\n"));
    return 0;
}


static int end_time = -1;       /* Time of timeout in seconds */

void TimeoutInit(int seconds)
{
    if (seconds > 0) {
#ifdef HAVE_TIME
        time_t t;
        t = time(NULL);
        end_time = seconds + (int) t;
#elif defined(HAVE_GETTIMEOFDAY)
        struct timeval tp;
        gettimeofday(&tp, NULL);
        end_time = seconds + tp.tv_sec;
#else
#error 'No timer available'
#endif
    } else {
        end_time = -1;
    }
}

/* Return remaining time in seconds */
int TimeoutGetRemaining(void)
{
    int time_left;
    if (end_time < 0) {
        /* Return a large, positive number */
        return 1000000;
    } else {
#ifdef HAVE_TIME
        time_t t;
        t = time(NULL);
        time_left = end_time - (int) t;
#elif defined(HAVE_GETTIMEOFDAY)
        struct timeval tp;
        gettimeofday(&tp, NULL);
        time_left = end_time - tp.tv_sec;
#else
#error 'No timer available'
#endif
    }
    if (time_left < 0)
        time_left = 0;
    return time_left;
}
