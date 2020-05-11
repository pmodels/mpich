/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef IPC_PRE_H_INCLUDED
#define IPC_PRE_H_INCLUDED

#ifdef MPIDI_CH4_SHM_ENABLE_XPMEM
#include "../xpmem/xpmem_pre.h"
#endif

typedef enum {
    MPIDI_SHM_IPC_TYPE__XPMEM,
} MPIDI_SHM_IPC_type_t;

typedef union {
#ifdef MPIDI_CH4_SHM_ENABLE_XPMEM
    MPIDI_IPC_xpmem_win_t xpmem;
#endif
} MPIDI_IPC_win_t;

typedef struct {
    MPIDI_SHM_IPC_type_t ipc_type;
    union {
#ifdef MPIDI_CH4_SHM_ENABLE_XPMEM
        MPIDI_IPC_xpmem_am_request_t xpmem;
#endif
    } u;
} MPIDI_IPC_am_request_t;

#ifdef MPL_USE_DBG_LOGGING
extern MPL_dbg_class MPIDI_CH4_SHM_IPC_GENERAL;
#endif
#define IPC_TRACE(...) \
    MPL_DBG_MSG_FMT(MPIDI_CH4_SHM_IPC_GENERAL,VERBOSE,(MPL_DBG_FDEST, "IPC "__VA_ARGS__))

#define MPIDI_IPC_REQUEST(req, field)      ((req)->dev.ch4.am.shm_am.ipc.field)

#endif /* IPC_PRE_H_INCLUDED */
