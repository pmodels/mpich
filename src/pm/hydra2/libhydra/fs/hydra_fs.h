/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2017 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef HYDRA_FS_H_INCLUDED
#define HYDRA_FS_H_INCLUDED

#include "hydra_base.h"

HYD_status HYD_find_in_path(const char *execname, char **path);
char *HYD_getcwd(void);
char *HYD_find_full_path(const char *execname);

#endif /* HYDRA_FS_H_INCLUDED */
