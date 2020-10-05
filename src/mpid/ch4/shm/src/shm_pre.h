/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef SHM_PRE_H_INCLUDED
#define SHM_PRE_H_INCLUDED

#include <mpi.h>

#include "../posix/posix_pre.h"
#include "../ipc/src/ipc_pre.h"

#include "shm_types.h"

typedef struct {
    MPIDI_POSIX_Global_t posix;
    MPIDI_SHMI_ctrl_cb ctrl_cbs[MPIDI_SHMI_CTRL_IDS_MAX];
} MPIDI_SHM_Global_t;

#endif /* SHM_PRE_H_INCLUDED */
