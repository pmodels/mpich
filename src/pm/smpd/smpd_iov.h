#ifndef SMPD_IOV_H_INCLUDED
#define SMPD_IOV_H_INCLUDED

#include "smpdconf.h"
/* IOVs */
/* The basic channel interface uses IOVs */
#define SMPD_IOV_BUF_CAST void *
#ifdef HAVE_WINDOWS_H
#include <winsock2.h>
#define SMPD_IOV         WSABUF
#define SMPD_IOV_LEN     len
#define SMPD_IOV_BUF     buf
#else
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h> /* macs need sys/types.h before uio.h can be included */
#endif
#ifdef HAVE_SYS_UIO_H
#include <sys/uio.h>
#endif
#define SMPD_IOV         struct iovec
#define SMPD_IOV_LEN     iov_len
#define SMPD_IOV_BUF     iov_base
#endif
/* FIXME: How is IOV_LIMIT chosen? */
#define SMPD_IOV_LIMIT   16
#endif
