/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2017 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef HYDRA_BASE_H_INCLUDED
#define HYDRA_BASE_H_INCLUDED

/* hydra_config.h must come first, otherwise feature macros like _USE_GNU that
 * were defined by AC_USE_SYSTEM_EXTENSIONS will not be defined yet when mpl.h
 * indirectly includes features.h.  This leads to a mismatch between the
 * behavior determined by configure and the behavior actually caused by
 * "#include"ing unistd.h, for example. */
#include "hydra_config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>
#include <ctype.h>

#if defined NEEDS_POSIX_FOR_SIGACTION
#define _POSIX_SOURCE
#endif /* NEEDS_POSIX_FOR_SIGACTION */

#if defined HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */

#if defined HAVE_STRINGS_H
#include <strings.h>
#endif /* HAVE_STRINGS_H */

#if defined HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif /* HAVE_SYS_TYPES_H */

#if defined HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif /* HAVE_SYS_STAT_H */

#if defined HAVE_TIME_H
#include <time.h>
#endif /* HAVE_TIME_H */

#if defined HAVE_SYS_TIME_H
#include <sys/time.h>
#endif /* HAVE_SYS_TIME_H */

#if defined HAVE_IFADDRS_H
#include <ifaddrs.h>
#endif /* HAVE_IFADDRS_H */

#if defined HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif /* HAVE_ARPA_INET_H */

#ifdef HAVE_POLL_H
#include <poll.h>
#endif /* HAVE_POLL_H */

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif /* HAVE_FCNTL_H */

#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif /* HAVE_NETDB_H */

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif /* HAVE_NETINET_IN_H */

#ifdef HAVE_NETINET_TCP_H
#include <netinet/tcp.h>
#endif /* HAVE_NETINET_TCP_H */

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif /* HAVE_SYS_SOCKET_H */

#define MPL_VG_ENABLED 1

#include "mpl.h"
#include "mpl_uthash.h"

#if defined MANUAL_EXTERN_ENVIRON
extern char **environ;
#endif /* MANUAL_EXTERN_ENVIRON */

#if defined NEEDS_GETTIMEOFDAY_DECL
int gettimeofday(struct timeval *tv, struct timezone *tz);
#endif /* NEEDS_GETTIMEOFDAY_DECL */

#if defined(NEEDS_GETHOSTNAME_DECL)
int gethostname(char *name, size_t len);
#endif

#if defined NEEDS_GETPGID_DECL
pid_t getpgid(pid_t pid);
#endif /* NEEDS_GETPGID_DECL */

#if defined NEEDS_KILLPG_DECL
int killpg(int pgrp, int sig);
#endif /* NEEDS_KILLPG_DECL */

#ifdef NEEDS_STRSIGNAL_DECL
extern char *strsignal(int);
#endif

#if defined(MPL_HAVE_PUTENV) && defined(MPL_NEEDS_PUTENV_DECL)
extern int putenv(char *string);
#endif

#if defined MAXHOSTNAMELEN
#define HYD_MAX_HOSTNAME_LEN MAXHOSTNAMELEN
#else
#define HYD_MAX_HOSTNAME_LEN 256
#endif /* MAXHOSTNAMELEN */

#define HYDRA_MAX_PATH 4096

#define HYD_TMP_STRLEN  (16 * 1024)
#define HYD_NUM_TMP_STRINGS 1000

#define HYD_DEFAULT_RETRY_COUNT (10)
#define HYD_CONNECT_DELAY (10)

#define HYDRA_NAMESERVER_DEFAULT_PORT 6392

/* Disable for now; we might add something here in the future */
#define HYD_FUNC_ENTER()   do {} while (0)
#define HYD_FUNC_EXIT()    do {} while (0)

#if !defined HAVE_GETTIMEOFDAY
#error "hydra requires gettimeofday support"
#endif /* HAVE_GETTIMEOFDAY */

/* Status information */
typedef enum {
    HYD_SUCCESS = 0,
    HYD_ERR_TIMED_OUT,
    HYD_ERR_OUT_OF_MEMORY,
    HYD_ERR_SOCK,
    HYD_ERR_INVALID_PARAM,
    HYD_ERR_INTERNAL,
    HYD_ERR_PORT_IN_USE,
} HYD_status;

#endif /* HYDRA_BASE_H_INCLUDED */
