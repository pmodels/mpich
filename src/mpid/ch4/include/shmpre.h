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
#define MPIDI_SHM_REQ_XPMEM_SEND_LMT (0x1)
#define MPIDI_SHM_REQ_GPU_IPC_RECV   (0x2)

#ifdef MPIDI_CH4_SHM_ENABLE_XPMEM
#define MPIDI_SHM_XPMEM_WIN_DECL MPIDI_XPMEM_win_t xpmem;
#define MPIDI_SHM_XPMEM_REQUEST_AM_DECL MPIDI_XPMEM_am_request_t xpmem;
#else
#define MPIDI_SHM_XPMEM_WIN_DECL
#define MPIDI_SHM_XPMEM_REQUEST_AM_DECL
#endif

#ifdef MPIDI_CH4_SHM_ENABLE_GPU_IPC
#define MPIDI_SHM_GPU_REQUEST_AM_DECL MPIDI_GPU_am_request_t gpu;
#else
#define MPIDI_SHM_GPU_REQUEST_AM_DECL
#endif

#define MPIDI_SHM_REQUEST_AM_DECL    uint64_t status;                   \
                                     MPIDI_POSIX_am_request_t posix;    \
                                     MPIDI_SHM_XPMEM_REQUEST_AM_DECL;   \
                                     MPIDI_SHM_GPU_REQUEST_AM_DECL
#define MPIDI_SHM_REQUEST_DECL       MPIDI_POSIX_request_t posix;
#define MPIDI_SHM_COMM_DECL          MPIDI_POSIX_comm_t posix;
#define MPIDI_SHM_WIN_DECL           MPIDI_POSIX_win_t posix;   \
                                     MPIDI_SHM_XPMEM_WIN_DECL

#define MPIDI_SHM_REQUEST(req, field)  ((req)->dev.ch4.am.shm_am.field)

#endif /* SHMPRE_H_INCLUDED */
