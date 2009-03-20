/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef CSI_H_INCLUDED
#define CSI_H_INCLUDED

#include "hydra.h"

HYD_Status HYD_CSI_launch_procs(void);
HYD_Status HYD_CSI_wait_for_completion(void);
HYD_Status HYD_CSI_close_fd(int fd);
HYD_Status HYD_CSI_finalize(void);

#endif /* CSI_H_INCLUDED */
