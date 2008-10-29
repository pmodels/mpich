/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef HYDRA_H_INCLUDED
#define HYDRA_H_INCLUDED

#include <stdio.h>
#include "mpibase.h"
#include "hydra_config.h"

#if defined HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */

#if defined HAVE_STDLIB_H
#include <stdlib.h>
#endif /* HAVE_STDLIB_H */

#if defined HAVE_STRING_H
#include <string.h>
#endif /* HAVE_STRING_H */

#if defined HAVE_STRINGS_H
#include <strings.h>
#endif /* HAVE_STRINGS_H */

#if defined HAVE_STDARG_H
#include <stdarg.h>
#endif /* HAVE_STDARG_H */

#if defined HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif /* HAVE_SYS_TYPES_H */

#include <errno.h>
/* FIXME: Is time.h available everywhere? We should probably have
 * multiple timer options like MPICH2. */
#include <time.h>
#include "mpimem.h"

#if defined MAXHOSTNAMELEN
#define MAX_HOSTNAME_LEN MAXHOSTNAMELEN
#else
#define MAX_HOSTNAME_LEN 256
#endif /* MAXHOSTNAMELEN */

#if defined MANUAL_EXTERN_ENVIRON
extern char ** environ;
#endif /* MANUAL_EXTERN_ENVIRON */

#if defined HAVE_DEBUGGING_SUPPORT
#define HYD_FUNC_ENTER() \
    { \
	printf("[%s] Entering function %s\n", FUNCNAME, __LINE__);	\
    }
#define HYD_FUNC_EXIT() \
    { \
	printf("[%s] Exiting function %s", FUNCNAME, __LINE__);	\
    }
#else
#define HYDU_FUNC_ENTER() {}
#define HYDU_FUNC_EXIT() {}
#endif /* HAVE_DEBUGGING_SUPPORT */

/* FIXME: __VA_ARGS__ is not portable. This needs to be fixed
 * eventually. */
#define HYDU_Error_printf(fmt, ...)		\
    { \
	fprintf(stderr, "%s (%d): ", FUNCNAME, __LINE__); \
	MPIU_Error_printf(fmt, ## __VA_ARGS__); \
    }

typedef enum {
    HYD_NO_MEM,
    HYD_SOCK_ERROR,
    HYD_INVALID_PARAM,
    HYD_INTERNAL_ERROR,
    HYD_SUCCESS = 0
} HYD_Status;

#endif /* HYDRA_H_INCLUDED */
