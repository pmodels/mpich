/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */

/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MPIDU_SOCKI_H_INCLUDED
#define MPIDU_SOCKI_H_INCLUDED

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
#define MPIDI_CH3I_SOCK_INFINITE_TIME   -1
#define MPIDI_CH3I_SOCK_INVALID_SOCK    NULL
#define MPIDI_CH3I_SOCK_INVALID_SET     NULL
#define MPIDI_CH3I_SOCK_SIZE_MAX	   SSIZE_MAX
#define MPIDI_CH3I_SOCK_NATIVE_FD       int

typedef struct MPIDI_CH3I_Sock_set * MPIDI_CH3I_Sock_set_t;
typedef struct MPIDI_CH3I_Sock * MPIDI_CH3I_Sock_t;
typedef size_t MPIDI_CH3I_Sock_size_t;

#define MPIDI_CH3I_SOCKI_STATE_LIST \
MPID_STATE_MPIDI_CH3I_SOCKI_READ, \
MPID_STATE_MPIDI_CH3I_SOCKI_WRITE, \
MPID_STATE_MPIDI_CH3I_SOCKI_SOCK_ALLOC, \
MPID_STATE_MPIDI_CH3I_SOCKI_SOCK_FREE, \
MPID_STATE_MPIDI_CH3I_SOCKI_EVENT_ENQUEUE, \
MPID_STATE_MPIDI_CH3I_SOCKI_EVENT_DEQUEUE, \
MPID_STATE_MPIDI_CH3I_SOCKI_ADJUST_IOV, \
MPID_STATE_READ, \
MPID_STATE_READV, \
MPID_STATE_WRITE, \
MPID_STATE_WRITEV, \
MPID_STATE_POLL,

#if defined (MPL_USE_DBG_LOGGING)
extern MPL_dbg_class MPIDI_CH3I_DBG_SOCK_CONNECT;
#endif /* MPL_USE_DBG_LOGGING */

#endif /* MPIDU_SOCKI_H_INCLUDED */
