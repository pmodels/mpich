/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2016 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#ifndef CH4_TYPES_H_INCLUDED
#define CH4_TYPES_H_INCLUDED

#include <mpidimpl.h>
#include <stdio.h>
#include "mpir_cvars.h"
#include "ch4i_workq_types.h"

/* Macros and inlines */
#define MPIDIU_MAP_NOT_FOUND      ((void*)(-1UL))

#define MAX_PROGRESS_HOOKS 4

/* VCI attributes */
enum {
    MPIDI_VCI_TX = 0x1,         /* Can send */
    MPIDI_VCI_RX = 0x2, /* Can receive */
};

#define MPIDIU_BUF_POOL_NUM (1024)
#define MPIDIU_BUF_POOL_SZ (256)

typedef int (*progress_func_ptr_t) (int *made_progress);
typedef struct progress_hook_slot {
    progress_func_ptr_t func_ptr;
    int active;
} progress_hook_slot_t;

/* Flags for MPIDI_Progress_test
 *
 * Flags argument allows to control execution of different parts of progress function,
 * for aims of prioritization of different transports and reentrant-safety of progress call.
 *
 * MPIDI_PROGRESS_HOOKS - enables progress on progress hooks. Hooks may invoke upper-level logic internaly,
 *      that's why MPIDI_Progress_test call with MPIDI_PROGRESS_HOOKS set isn't reentrant safe, and shouldn't be called from netmod's fallback logic.
 * MPIDI_PROGRESS_NM and MPIDI_PROGRESS_SHM enables progress on transports only, and guarantee reentrant-safety.
 */
#define MPIDI_PROGRESS_HOOKS  (1)
#define MPIDI_PROGRESS_NM     (1<<1)
#define MPIDI_PROGRESS_SHM    (1<<2)

#define MPIDI_PROGRESS_ALL (MPIDI_PROGRESS_HOOKS|MPIDI_PROGRESS_NM|MPIDI_PROGRESS_SHM)

enum {
    MPIDIG_SEND = 0,            /* Eager send */

    MPIDIG_SEND_LONG_REQ,       /* Rendezvous send RTS (request to send) */
    MPIDIG_SEND_LONG_ACK,       /* Rendezvous send CTS (clear to send) */
    MPIDIG_SEND_LONG_LMT,       /* Rendezvous send LMT */

    MPIDIG_SSEND_REQ,
    MPIDIG_SSEND_ACK,

    MPIDIG_PUT_REQ,
    MPIDIG_PUT_ACK,
    MPIDIG_PUT_IOV_REQ,
    MPIDIG_PUT_DAT_REQ,
    MPIDIG_PUT_IOV_ACK,

    MPIDIG_GET_REQ,
    MPIDIG_GET_ACK,

    MPIDIG_ACC_REQ,
    MPIDIG_ACC_ACK,
    MPIDIG_ACC_IOV_REQ,
    MPIDIG_ACC_DAT_REQ,
    MPIDIG_ACC_IOV_ACK,

    MPIDIG_GET_ACC_REQ,
    MPIDIG_GET_ACC_ACK,
    MPIDIG_GET_ACC_IOV_REQ,
    MPIDIG_GET_ACC_DAT_REQ,
    MPIDIG_GET_ACC_IOV_ACK,

    MPIDIG_CSWAP_REQ,
    MPIDIG_CSWAP_ACK,
    MPIDIG_FETCH_OP,

    MPIDIG_WIN_COMPLETE,
    MPIDIG_WIN_POST,
    MPIDIG_WIN_LOCK,
    MPIDIG_WIN_LOCK_ACK,
    MPIDIG_WIN_UNLOCK,
    MPIDIG_WIN_UNLOCK_ACK,
    MPIDIG_WIN_LOCKALL,
    MPIDIG_WIN_LOCKALL_ACK,
    MPIDIG_WIN_UNLOCKALL,
    MPIDIG_WIN_UNLOCKALL_ACK,

    MPIDIG_COMM_ABORT
};

enum {
    MPIDIG_EPOTYPE_NONE = 0,          /**< No epoch in affect */
    MPIDIG_EPOTYPE_LOCK = 1,          /**< MPI_Win_lock access epoch */
    MPIDIG_EPOTYPE_START = 2,         /**< MPI_Win_start access epoch */
    MPIDIG_EPOTYPE_POST = 3,          /**< MPI_Win_post exposure epoch */
    MPIDIG_EPOTYPE_FENCE = 4,         /**< MPI_Win_fence access/exposure epoch */
    MPIDIG_EPOTYPE_REFENCE = 5,       /**< MPI_Win_fence possible access/exposure epoch */
    MPIDIG_EPOTYPE_LOCK_ALL = 6       /**< MPI_Win_lock_all access epoch */
};

/* Enum for calling types between netmod and shm */
enum {
    MPIDI_NETMOD = 0,
    MPIDI_SHM = 1
};

/* Enum for src buffer kind when computing accumulate op */
enum {
    MPIDIG_ACC_SRCBUF_DEFAULT = 0,
    MPIDIG_ACC_SRCBUF_PACKED = 1
};

typedef struct MPIDIG_hdr_t {
    int src_rank;
    int tag;
    MPIR_Context_id_t context_id;
} MPIDIG_hdr_t;

typedef struct MPIDIG_send_long_req_mst_t {
    MPIDIG_hdr_t hdr;
    size_t data_sz;             /* Message size in bytes */
    uint64_t sreq_ptr;          /* Pointer value of the request object at the sender side */
} MPIDIG_send_long_req_mst_t;

typedef struct MPIDIG_send_long_ack_msg_t {
    uint64_t sreq_ptr;
    uint64_t rreq_ptr;
} MPIDIG_send_long_ack_msg_t;

typedef struct MPIDIG_send_long_lmt_msg_t {
    uint64_t rreq_ptr;
} MPIDIG_send_long_lmt_msg_t;

typedef struct MPIDIG_ssend_req_msg_t {
    MPIDIG_hdr_t hdr;
    uint64_t sreq_ptr;
} MPIDIG_ssend_req_msg_t;

typedef struct MPIDIG_ssend_ack_msg_t {
    uint64_t sreq_ptr;
} MPIDIG_ssend_ack_msg_t;

typedef struct MPIDIG_win_cntrl_msg_t {
    uint64_t win_id;
    uint32_t origin_rank;
    int16_t lock_type;
} MPIDIG_win_cntrl_msg_t;

typedef struct MPIDIG_put_msg_t {
    int src_rank;
    uint64_t win_id;
    uint64_t preq_ptr;
    MPI_Aint target_disp;
    uint64_t count;
    MPI_Datatype datatype;
    int n_iov;
} MPIDIG_put_msg_t;

typedef struct MPIDIG_put_iov_ack_msg_t {
    int src_rank;
    uint64_t target_preq_ptr;
    uint64_t origin_preq_ptr;
} MPIDIG_put_iov_ack_msg_t;
typedef MPIDIG_put_iov_ack_msg_t MPIDIG_acc_iov_ack_msg_t;
typedef MPIDIG_put_iov_ack_msg_t MPIDIG_get_acc_iov_ack_msg_t;

typedef struct MPIDIG_put_dat_msg_t {
    uint64_t preq_ptr;
} MPIDIG_put_dat_msg_t;
typedef MPIDIG_put_dat_msg_t MPIDIG_acc_dat_msg_t;
typedef MPIDIG_put_dat_msg_t MPIDIG_get_acc_dat_msg_t;

typedef struct MPIDIG_put_ack_msg_t {
    uint64_t preq_ptr;
} MPIDIG_put_ack_msg_t;

typedef struct MPIDIG_get_msg_t {
    int src_rank;
    uint64_t win_id;
    uint64_t greq_ptr;
    MPI_Aint target_disp;
    uint64_t count;
    MPI_Datatype datatype;
    int n_iov;
} MPIDIG_get_msg_t;

typedef struct MPIDIG_get_ack_msg_t {
    uint64_t greq_ptr;
} MPIDIG_get_ack_msg_t;

typedef struct MPIDIG_cswap_req_msg_t {
    int src_rank;
    uint64_t win_id;
    uint64_t req_ptr;
    MPI_Aint target_disp;
    MPI_Datatype datatype;
} MPIDIG_cswap_req_msg_t;

typedef struct MPIDIG_cswap_ack_msg_t {
    uint64_t req_ptr;
} MPIDIG_cswap_ack_msg_t;

typedef struct MPIDIG_acc_req_msg_t {
    int src_rank;
    uint64_t win_id;
    uint64_t req_ptr;
    int origin_count;
    MPI_Datatype origin_datatype;
    int target_count;
    MPI_Datatype target_datatype;
    MPI_Op op;
    MPI_Aint target_disp;
    uint64_t result_data_sz;
    int n_iov;
} MPIDIG_acc_req_msg_t;

typedef struct MPIDIG_acc_req_msg_t MPIDIG_get_acc_req_msg_t;

typedef struct MPIDIG_acc_ack_msg_t {
    uint64_t req_ptr;
} MPIDIG_acc_ack_msg_t;

typedef MPIDIG_acc_ack_msg_t MPIDIG_get_acc_ack_msg_t;

typedef struct MPIDIG_comm_req_list_t {
    MPIR_Comm *comm[2][4];
    MPIDIG_rreq_t *uelist[2][4];
} MPIDIG_comm_req_list_t;

typedef struct MPIDIU_buf_pool_t {
    int size;
    int num;
    void *memory_region;
    struct MPIDIU_buf_pool_t *next;
    struct MPIDIU_buf_t *head;
    MPID_Thread_mutex_t lock;
} MPIDIU_buf_pool_t;

typedef struct MPIDIU_buf_t {
    struct MPIDIU_buf_t *next;
    MPIDIU_buf_pool_t *pool;
    char data[];
} MPIDIU_buf_t;

typedef struct {
    int mmapped_size;
    int max_n_avts;
    int n_avts;
    int next_avtid;
    int *free_avtid;
} MPIDIU_avt_manager;

typedef struct {
    uint64_t key;
    void *value;
    UT_hash_handle hh;          /* makes this structure hashable */
} MPIDIU_map_entry_t;

typedef struct MPIDIU_map_t {
    MPIDIU_map_entry_t *head;
} MPIDIU_map_t;

typedef struct {
    unsigned mt_model;
} MPIDI_CH4_configurations_t;

typedef struct MPIDI_CH4_Global_t {
    MPIR_Request *request_test;
    MPIR_Comm *comm_test;
    int pname_set;
    int pname_len;
    char pname[MPI_MAX_PROCESSOR_NAME];
    int is_initialized;
    MPIDIU_avt_manager avt_mgr;
    int is_ch4u_initialized;
    int **node_map, max_node_id;
    MPIDIG_comm_req_list_t *comm_req_lists;
    int registered_progress_hooks;
    MPIR_Commops MPIR_Comm_fns_store;
    progress_hook_slot_t progress_hooks[MAX_PROGRESS_HOOKS];
    MPID_Thread_mutex_t m[3];
    MPIDIU_map_t *win_map;
    char *jobid;
#ifndef MPIDI_CH4U_USE_PER_COMM_QUEUE
    MPIDIG_rreq_t *posted_list;
    MPIDIG_rreq_t *unexp_list;
#endif
    MPIDIG_req_ext_t *cmpl_list;
    OPA_int_t exp_seq_no;
    OPA_int_t nxt_seq_no;
    MPIDIU_buf_pool_t *buf_pool;
#ifdef HAVE_SIGNAL
    void (*prev_sighandler) (int);
    volatile int sigusr1_count;
    int my_sigusr1_count;
#endif
    OPA_int_t progress_count;

    MPID_Thread_mutex_t vci_lock;
#if defined(MPIDI_CH4_USE_WORK_QUEUES)
    MPIDI_workq_t workqueue;
#endif
    MPIDI_CH4_configurations_t settings;
} MPIDI_CH4_Global_t;
extern MPIDI_CH4_Global_t MPIDI_global;
#ifdef MPL_USE_DBG_LOGGING
extern MPL_dbg_class MPIDI_CH4_DBG_GENERAL;
extern MPL_dbg_class MPIDI_CH4_DBG_MAP;
extern MPL_dbg_class MPIDI_CH4_DBG_COMM;
extern MPL_dbg_class MPIDI_CH4_DBG_MEMORY;
#endif
#define MPIDIU_THREAD_PROGRESS_MUTEX  MPIDI_global.m[0]
#define MPIDIU_THREAD_PROGRESS_HOOK_MUTEX  MPIDI_global.m[1]
#define MPIDIU_THREAD_UTIL_MUTEX  MPIDI_global.m[2]

#endif /* CH4_TYPES_H_INCLUDED */
