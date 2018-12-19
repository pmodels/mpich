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
#define MPIDI_CH4U_MAP_NOT_FOUND      ((void*)(-1UL))

#define MAX_PROGRESS_HOOKS 4

/* VNI attributes */
enum {
    MPIDI_VNI_TX = 0x1,         /* Can send */
    MPIDI_VNI_RX = 0x2, /* Can receive */
};

#define MPIDI_CH4I_BUF_POOL_NUM (1024)
#define MPIDI_CH4I_BUF_POOL_SZ (256)

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
    MPIDI_CH4U_SEND = 0,        /* Eager send */

    MPIDI_CH4U_SEND_LONG_REQ,   /* Rendezvous send RTS (request to send) */
    MPIDI_CH4U_SEND_LONG_ACK,   /* Rendezvous send CTS (clear to send) */
    MPIDI_CH4U_SEND_LONG_LMT,   /* Rendezvous send LMT */

    MPIDI_CH4U_SSEND_REQ,
    MPIDI_CH4U_SSEND_ACK,

    MPIDI_CH4U_PUT_REQ,
    MPIDI_CH4U_PUT_ACK,
    MPIDI_CH4U_PUT_IOV_REQ,
    MPIDI_CH4U_PUT_DAT_REQ,
    MPIDI_CH4U_PUT_IOV_ACK,

    MPIDI_CH4U_GET_REQ,
    MPIDI_CH4U_GET_ACK,

    MPIDI_CH4U_ACC_REQ,
    MPIDI_CH4U_ACC_ACK,
    MPIDI_CH4U_ACC_IOV_REQ,
    MPIDI_CH4U_ACC_DAT_REQ,
    MPIDI_CH4U_ACC_IOV_ACK,

    MPIDI_CH4U_GET_ACC_REQ,
    MPIDI_CH4U_GET_ACC_ACK,
    MPIDI_CH4U_GET_ACC_IOV_REQ,
    MPIDI_CH4U_GET_ACC_DAT_REQ,
    MPIDI_CH4U_GET_ACC_IOV_ACK,

    MPIDI_CH4U_CSWAP_REQ,
    MPIDI_CH4U_CSWAP_ACK,
    MPIDI_CH4U_FETCH_OP,

    MPIDI_CH4U_WIN_COMPLETE,
    MPIDI_CH4U_WIN_POST,
    MPIDI_CH4U_WIN_LOCK,
    MPIDI_CH4U_WIN_LOCK_ACK,
    MPIDI_CH4U_WIN_UNLOCK,
    MPIDI_CH4U_WIN_UNLOCK_ACK,
    MPIDI_CH4U_WIN_LOCKALL,
    MPIDI_CH4U_WIN_LOCKALL_ACK,
    MPIDI_CH4U_WIN_UNLOCKALL,
    MPIDI_CH4U_WIN_UNLOCKALL_ACK,

    MPIDI_CH4U_COMM_ABORT
};

enum {
    MPIDI_CH4U_EPOTYPE_NONE = 0,          /**< No epoch in affect */
    MPIDI_CH4U_EPOTYPE_LOCK = 1,          /**< MPI_Win_lock access epoch */
    MPIDI_CH4U_EPOTYPE_START = 2,         /**< MPI_Win_start access epoch */
    MPIDI_CH4U_EPOTYPE_POST = 3,          /**< MPI_Win_post exposure epoch */
    MPIDI_CH4U_EPOTYPE_FENCE = 4,         /**< MPI_Win_fence access/exposure epoch */
    MPIDI_CH4U_EPOTYPE_REFENCE = 5,       /**< MPI_Win_fence possible access/exposure epoch */
    MPIDI_CH4U_EPOTYPE_LOCK_ALL = 6       /**< MPI_Win_lock_all access epoch */
};

/* Enum for calling types between netmod and shm */
enum {
    MPIDI_CH4R_NETMOD = 0,
    MPIDI_CH4R_SHM = 1
};

typedef struct MPIDI_CH4U_hdr_t {
    int src_rank;
    int tag;
    MPIR_Context_id_t context_id;
} MPIDI_CH4U_hdr_t;

typedef struct MPIDI_CH4U_send_long_req_msg_t {
    MPIDI_CH4U_hdr_t hdr;
    size_t data_sz;             /* Message size in bytes */
    uint64_t sreq_ptr;          /* Pointer value of the request object at the sender side */
} MPIDI_CH4U_send_long_req_msg_t;

typedef struct MPIDI_CH4U_send_long_ack_msg_t {
    uint64_t sreq_ptr;
    uint64_t rreq_ptr;
} MPIDI_CH4U_send_long_ack_msg_t;

typedef struct MPIDI_CH4U_send_long_lmt_msg_t {
    uint64_t rreq_ptr;
} MPIDI_CH4U_send_long_lmt_msg_t;

typedef struct MPIDI_CH4U_ssend_req_msg_t {
    MPIDI_CH4U_hdr_t hdr;
    uint64_t sreq_ptr;
} MPIDI_CH4U_ssend_req_msg_t;

typedef struct MPIDI_CH4U_ssend_ack_msg_t {
    uint64_t sreq_ptr;
} MPIDI_CH4U_ssend_ack_msg_t;

typedef struct MPIDI_CH4U_win_cntrl_msg_t {
    uint64_t win_id;
    uint32_t origin_rank;
    int16_t lock_type;
} MPIDI_CH4U_win_cntrl_msg_t;

typedef struct MPIDI_CH4U_put_msg_t {
    int src_rank;
    uint64_t win_id;
    uint64_t preq_ptr;
    MPI_Aint target_disp;
    uint64_t count;
    MPI_Datatype datatype;
    int n_iov;
} MPIDI_CH4U_put_msg_t;

typedef struct MPIDI_CH4U_put_iov_ack_msg_t {
    int src_rank;
    uint64_t target_preq_ptr;
    uint64_t origin_preq_ptr;
} MPIDI_CH4U_put_iov_ack_msg_t;
typedef MPIDI_CH4U_put_iov_ack_msg_t MPIDI_CH4U_acc_iov_ack_msg_t;
typedef MPIDI_CH4U_put_iov_ack_msg_t MPIDI_CH4U_get_acc_iov_ack_msg_t;

typedef struct MPIDI_CH4U_put_dat_msg_t {
    uint64_t preq_ptr;
} MPIDI_CH4U_put_dat_msg_t;
typedef MPIDI_CH4U_put_dat_msg_t MPIDI_CH4U_acc_dat_msg_t;
typedef MPIDI_CH4U_put_dat_msg_t MPIDI_CH4U_get_acc_dat_msg_t;

typedef struct MPIDI_CH4U_put_ack_msg_t {
    uint64_t preq_ptr;
} MPIDI_CH4U_put_ack_msg_t;

typedef struct MPIDI_CH4U_get_req_msg_t {
    int src_rank;
    uint64_t win_id;
    uint64_t greq_ptr;
    MPI_Aint target_disp;
    uint64_t count;
    MPI_Datatype datatype;
    int n_iov;
} MPIDI_CH4U_get_req_msg_t;

typedef struct MPIDI_CH4U_get_ack_msg_t {
    uint64_t greq_ptr;
} MPIDI_CH4U_get_ack_msg_t;

typedef struct MPIDI_CH4U_cswap_req_msg_t {
    int src_rank;
    uint64_t win_id;
    uint64_t req_ptr;
    MPI_Aint target_disp;
    MPI_Datatype datatype;
} MPIDI_CH4U_cswap_req_msg_t;

typedef struct MPIDI_CH4U_cswap_ack_msg_t {
    uint64_t req_ptr;
} MPIDI_CH4U_cswap_ack_msg_t;

typedef struct MPIDI_CH4U_acc_req_msg_t {
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
} MPIDI_CH4U_acc_req_msg_t;

typedef struct MPIDI_CH4U_acc_req_msg_t MPIDI_CH4U_get_acc_req_msg_t;

typedef struct MPIDI_CH4U_acc_ack_msg_t {
    uint64_t req_ptr;
} MPIDI_CH4U_acc_ack_msg_t;

typedef MPIDI_CH4U_acc_ack_msg_t MPIDI_CH4U_get_acc_ack_msg_t;

typedef struct MPIDI_CH4U_comm_req_list_t {
    MPIR_Comm *comm[2][4];
    MPIDI_CH4U_rreq_t *uelist[2][4];
} MPIDI_CH4U_comm_req_list_t;

typedef struct MPIU_buf_pool_t {
    int size;
    int num;
    void *memory_region;
    struct MPIU_buf_pool_t *next;
    struct MPIU_buf_t *head;
    MPID_Thread_mutex_t lock;
} MPIU_buf_pool_t;

typedef struct MPIU_buf_t {
    struct MPIU_buf_t *next;
    MPIU_buf_pool_t *pool;
    char data[];
} MPIU_buf_t;

typedef struct {
    int mmapped_size;
    int max_n_avts;
    int n_avts;
    int next_avtid;
    int *free_avtid;
} MPIDI_CH4_avt_manager;

typedef struct {
    uint64_t key;
    void *value;
    UT_hash_handle hh;          /* makes this structure hashable */
} MPIDI_CH4U_map_entry_t;

typedef struct MPIDI_CH4U_map_t {
    MPIDI_CH4U_map_entry_t *head;
} MPIDI_CH4U_map_t;

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
    MPIDI_CH4_avt_manager avt_mgr;
    int is_ch4u_initialized;
    int **node_map, max_node_id;
    MPIDI_CH4U_comm_req_list_t *comm_req_lists;
    OPA_int_t active_progress_hooks;
    MPIR_Commops MPIR_Comm_fns_store;
    progress_hook_slot_t progress_hooks[MAX_PROGRESS_HOOKS];
    MPID_Thread_mutex_t m[3];
    MPIDI_CH4U_map_t *win_map;
    char *jobid;
#ifndef MPIDI_CH4U_USE_PER_COMM_QUEUE
    MPIDI_CH4U_rreq_t *posted_list;
    MPIDI_CH4U_rreq_t *unexp_list;
#endif
    MPIDI_CH4U_req_ext_t *cmpl_list;
    OPA_int_t exp_seq_no;
    OPA_int_t nxt_seq_no;
    MPIU_buf_pool_t *buf_pool;
#ifdef HAVE_SIGNAL
    void (*prev_sighandler) (int);
    volatile int sigusr1_count;
    int my_sigusr1_count;
#endif
    OPA_int_t progress_count;

    MPID_Thread_mutex_t vni_lock;
#if defined(MPIDI_CH4_USE_WORK_QUEUES)
    MPIDI_workq_t workqueue;
#endif
    MPIDI_CH4_configurations_t settings;
} MPIDI_CH4_Global_t;
extern MPIDI_CH4_Global_t MPIDI_CH4_Global;
#ifdef MPL_USE_DBG_LOGGING
extern MPL_dbg_class MPIDI_CH4_DBG_GENERAL;
extern MPL_dbg_class MPIDI_CH4_DBG_MAP;
extern MPL_dbg_class MPIDI_CH4_DBG_COMM;
extern MPL_dbg_class MPIDI_CH4_DBG_MEMORY;
#endif
#define MPIDI_CH4I_THREAD_PROGRESS_MUTEX  MPIDI_CH4_Global.m[0]
#define MPIDI_CH4I_THREAD_PROGRESS_HOOK_MUTEX  MPIDI_CH4_Global.m[1]
#define MPIDI_CH4I_THREAD_UTIL_MUTEX  MPIDI_CH4_Global.m[2]

#endif /* CH4_TYPES_H_INCLUDED */
