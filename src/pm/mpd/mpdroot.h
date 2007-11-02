/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* get configured options */
#include "mpdconf.h"

#include <stdio.h>
#ifdef STDC_HEADERS
#  include <stdlib.h>
#  include <string.h>
#  include <stdarg.h>
#endif
#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif
#ifdef HAVE_CRYPT_H
#  include <crypt.h>
#endif
#include <ctype.h>
#include <sys/types.h>
#ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>
#endif
#include <sys/socket.h>
#ifdef HAVE_SYS_WAIT_H
#  include <sys/wait.h>
#endif
#include <sys/un.h>
#include <sys/stat.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <errno.h>
#include <strings.h>
#include <signal.h>
#ifdef HAVE_FCNTL_H
#  include <fcntl.h>
#endif
#include <pwd.h>
#include <syslog.h>

#define NAME_LEN 512

