/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef SHMPRE_H_INCLUDED
#define SHMPRE_H_INCLUDED

/* *INDENT-OFF* */
#include "../shm/src/shm_pre.h"
/* *INDENT-ON* */

/* shm am status */
#define MPIDI_SHM_REQ_IPC_SEND_LMT (0x1)

#define MPIDI_SHM_REQUEST_AM_DECL    uint64_t status;                   \
                                     MPIDI_POSIX_am_request_t posix;    \
                                     MPIDI_IPC_am_request_t ipc;
#define MPIDI_SHM_REQUEST_DECL       MPIDI_POSIX_request_t posix;
#define MPIDI_SHM_COMM_DECL          MPIDI_POSIX_comm_t posix;
#define MPIDI_SHM_WIN_DECL           MPIDI_POSIX_win_t posix;

#define MPIDI_SHM_REQUEST(req, field)  ((req)->dev.ch4.am.shm_am.field)

#endif /* SHMPRE_H_INCLUDED */
