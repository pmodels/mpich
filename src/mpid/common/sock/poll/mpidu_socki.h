/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */

/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#if !defined(SOCKI_H_INCLUDED)
#define SOCKI_H_INCLUDED

#include "mpichconf.h"

#if defined(HAVE_SYS_UIO_H)
#include <sys/uio.h>
#endif
#if defined(HAVE_LIMITS_H)
#include <limits.h>
#endif
#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

#if defined(HAVE_GETHOSTNAME) && defined(NEEDS_GETHOSTNAME_DECL) && !defined(gethostname)
int gethostname(char *name, size_t len);
# endif

#ifndef SSIZE_MAX
/* SSIZE_MAX is the maximum amount of data that we expect to be able
   to read from a socket at one time.  If this is not defined, we
   guess that the value is 64k */
/* FIXME!!! */
#define SSIZE_MAX 65536
#endif
#define MPIDU_SOCK_INFINITE_TIME   -1
#define MPIDU_SOCK_INVALID_SOCK    NULL
#define MPIDU_SOCK_INVALID_SET     NULL
#define MPIDU_SOCK_SIZE_MAX	   SSIZE_MAX
#define MPIDU_SOCK_NATIVE_FD       int

typedef struct MPIDU_Sock_set * MPIDU_Sock_set_t;
typedef struct MPIDU_Sock * MPIDU_Sock_t;
typedef size_t MPIDU_Sock_size_t;

#define MPIDU_SOCKI_STATE_LIST \
MPID_STATE_MPIDU_SOCKI_READ, \
MPID_STATE_MPIDU_SOCKI_WRITE, \
MPID_STATE_MPIDU_SOCKI_SOCK_ALLOC, \
MPID_STATE_MPIDU_SOCKI_SOCK_FREE, \
MPID_STATE_MPIDU_SOCKI_EVENT_ENQUEUE, \
MPID_STATE_MPIDU_SOCKI_EVENT_DEQUEUE, \
MPID_STATE_MPIDU_SOCKI_ADJUST_IOV, \
MPID_STATE_READ, \
MPID_STATE_READV, \
MPID_STATE_WRITE, \
MPID_STATE_WRITEV, \
MPID_STATE_POLL,

#endif /* !defined(SOCKI_H_INCLUDED) */
