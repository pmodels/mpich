/* -*- Mode: C; c-basic-offset:4 ; -*- */

/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#if !defined(SOCKI_H_INCLUDED)
#define SOCKI_H_INCLUDED

#include "smpdu_socki_conf.h"

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
#define SMPDU_SOCK_INFINITE_TIME   -1
#define SMPDU_SOCK_INVALID_SOCK    NULL
#define SMPDU_SOCK_INVALID_SET     NULL
#define SMPDU_SOCK_SIZE_MAX	   SSIZE_MAX
#define SMPDU_SOCK_NATIVE_FD       int

typedef struct SMPDU_Sock_set * SMPDU_Sock_set_t;
typedef struct SMPDU_Sock * SMPDU_Sock_t;
typedef size_t SMPDU_Sock_size_t;

#define SMPDU_SOCKI_STATE_LIST \
SMPD_STATE_SMPDU_SOCKI_READ, \
SMPD_STATE_SMPDU_SOCKI_WRITE, \
SMPD_STATE_SMPDU_SOCKI_SOCK_ALLOC, \
SMPD_STATE_SMPDU_SOCKI_SOCK_FREE, \
SMPD_STATE_SMPDU_SOCKI_EVENT_ENQUEUE, \
SMPD_STATE_SMPDU_SOCKI_EVENT_DEQUEUE, \
SMPD_STATE_SMPDU_SOCKI_ADJUST_IOV, \
SMPD_STATE_READ, \
SMPD_STATE_READV, \
SMPD_STATE_WRITE, \
SMPD_STATE_WRITEV, \
SMPD_STATE_POLL,

#endif /* !defined(SOCKI_H_INCLUDED) */
