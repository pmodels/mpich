/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#if !defined(MPL_ENV_H_INCLUDED)
#define MPL_ENV_H_INCLUDED

#include "mplconfig.h"

/* *INDENT-ON* */
#if defined(__cplusplus)
extern "C" {
#endif
/* *INDENT-OFF* */

/* Prototypes for the functions to provide uniform access to the environment */
int MPL_env2int(const char *envName, int *val);
int MPL_env2range(const char *envName, int *lowPtr, int *highPtr);
int MPL_env2bool(const char *envName, int *val);
int MPL_env2str(const char *envName, const char **val);
int MPL_env2double(const char *envName, double *val);

/* *INDENT-ON* */
#if defined(__cplusplus)
}
#endif
/* *INDENT-OFF* */

#endif /* !defined(MPL_ENV_H_INCLUDED) */
