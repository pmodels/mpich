/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef CH4_TYPES_H_INCLUDED
#define CH4_TYPES_H_INCLUDED

#include <mpidimpl.h>
#include <stdio.h>
#include "mpir_cvars.h"
#include "ch4i_workq_types.h"
#include "mpidu_genq.h"

/* Macros and inlines */
#define MPIDIU_MAP_NOT_FOUND      ((void*)(-1UL))

/* VCI attributes */
enum {
    MPIDI_VCI_TX = 0x1,         /* Can send */
    MPIDI_VCI_RX = 0x2, /* Can receive */
};

#define MPIDIU_REQUEST_POOL_NUM_CELLS_PER_CHUNK (1024)
#define MPIDIU_REQUEST_POOL_MAX_NUM_CELLS (257 * 1024)
#define MPIDIU_REQUEST_POOL_CELL_SIZE (256)

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
#define MPIDI_PROGRESS_WORKQ  (1<<3)

#define MPIDI_PROGRESS_ALL 0xff

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
    int error_bits;
} MPIDIG_hdr_t;

typedef struct MPIDIG_send_long_req_msg_t {
    MPIDIG_hdr_t hdr;
    size_t data_sz;             /* Message size in bytes */
    MPIR_Request *sreq_ptr;     /* Pointer value of the request object at the sender side */
} MPIDIG_send_long_req_msg_t;

typedef struct MPIDIG_send_long_ack_msg_t {
    MPIR_Request *sreq_ptr;
    MPIR_Request *rreq_ptr;
} MPIDIG_send_long_ack_msg_t;

typedef struct MPIDIG_send_long_lmt_msg_t {
    MPIR_Request *rreq_ptr;
} MPIDIG_send_long_lmt_msg_t;

typedef struct MPIDIG_ssend_req_msg_t {
    MPIDIG_hdr_t hdr;
    MPIR_Request *sreq_ptr;
} MPIDIG_ssend_req_msg_t;

typedef struct MPIDIG_ssend_ack_msg_t {
    MPIR_Request *sreq_ptr;
} MPIDIG_ssend_ack_msg_t;

typedef struct MPIDIG_win_cntrl_msg_t {
    uint64_t win_id;
    uint32_t origin_rank;
    int16_t lock_type;
} MPIDIG_win_cntrl_msg_t;

typedef struct MPIDIG_put_msg_t {
    int src_rank;
    uint64_t win_id;
    MPIR_Request *preq_ptr;
    MPI_Aint target_disp;
    MPI_Aint target_count;
    MPI_Aint target_datatype;
    MPI_Aint target_true_lb;
    int flattened_sz;
} MPIDIG_put_msg_t;

typedef struct MPIDIG_put_dt_ack_msg_t {
    int src_rank;
    MPIR_Request *target_preq_ptr;
    MPIR_Request *origin_preq_ptr;
} MPIDIG_put_dt_ack_msg_t;
typedef MPIDIG_put_dt_ack_msg_t MPIDIG_acc_dt_ack_msg_t;
typedef MPIDIG_put_dt_ack_msg_t MPIDIG_get_acc_dt_ack_msg_t;

typedef struct MPIDIG_put_dat_msg_t {
    MPIR_Request *preq_ptr;
} MPIDIG_put_dat_msg_t;
typedef MPIDIG_put_dat_msg_t MPIDIG_acc_dat_msg_t;
typedef MPIDIG_put_dat_msg_t MPIDIG_get_acc_dat_msg_t;

typedef struct MPIDIG_put_ack_msg_t {
    MPIR_Request *preq_ptr;
} MPIDIG_put_ack_msg_t;

typedef struct MPIDIG_get_msg_t {
    int src_rank;
    uint64_t win_id;
    MPIR_Request *greq_ptr;
    MPI_Aint target_disp;
    MPI_Aint target_count;
    MPI_Aint target_datatype;
    MPI_Aint target_true_lb;
    int flattened_sz;
} MPIDIG_get_msg_t;

typedef struct MPIDIG_get_ack_msg_t {
    MPIR_Request *greq_ptr;
} MPIDIG_get_ack_msg_t;

typedef struct MPIDIG_cswap_req_msg_t {
    int src_rank;
    uint64_t win_id;
    MPIR_Request *req_ptr;
    MPI_Aint target_disp;
    MPI_Datatype datatype;
} MPIDIG_cswap_req_msg_t;

typedef struct MPIDIG_cswap_ack_msg_t {
    MPIR_Request *req_ptr;
} MPIDIG_cswap_ack_msg_t;

typedef struct MPIDIG_acc_req_msg_t {
    int src_rank;
    uint64_t win_id;
    MPIR_Request *req_ptr;
    int origin_count;
    MPI_Datatype origin_datatype;
    int target_count;
    MPI_Datatype target_datatype;
    MPI_Op op;
    MPI_Aint target_disp;
    uint64_t result_data_sz;
    int n_iov;
    int flattened_sz;
} MPIDIG_acc_req_msg_t;

typedef struct MPIDIG_acc_req_msg_t MPIDIG_get_acc_req_msg_t;

typedef struct MPIDIG_acc_ack_msg_t {
    MPIR_Request *req_ptr;
} MPIDIG_acc_ack_msg_t;

typedef MPIDIG_acc_ack_msg_t MPIDIG_get_acc_ack_msg_t;

typedef struct MPIDIG_comm_req_list_t {
    MPIR_Comm *comm[2][4];
    MPIDIG_rreq_t *uelist[2][4];
} MPIDIG_comm_req_list_t;

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

/* Defining them in the global struct avoids duplicate of declarations and
 * definitions; However, it makes debugging a bit cryptic */
#define MPIDIU_THREAD_PROGRESS_MUTEX      MPIDI_global.m[0]
#define MPIDIU_THREAD_UTIL_MUTEX          MPIDI_global.m[1]

/* Protects MPIDIG global structures (e.g. global unexpected message queue) */
#define MPIDIU_THREAD_MPIDIG_GLOBAL_MUTEX MPIDI_global.m[2]

#define MPIDIU_THREAD_SCHED_LIST_MUTEX    MPIDI_global.m[3]
#define MPIDIU_THREAD_TSP_QUEUE_MUTEX     MPIDI_global.m[4]
#ifdef HAVE_LIBHCOLL
#define MPIDIU_THREAD_HCOLL_MUTEX         MPIDI_global.m[5]
#endif

/* Protects dynamic process tag, connection_id, avtable etc. */
#define MPIDIU_THREAD_DYNPROC_MUTEX       MPIDI_global.m[6]

#define MAX_CH4_MUTEXES 7

/* per-VCI structure -- using union to force minimum size */
typedef union MPIDI_vci {
    struct {
        int attr;
        MPID_Thread_mutex_t lock;
    } vci;
    char pad[MPL_CACHELINE_SIZE];
} MPIDI_vci_t;

#define MPIDI_VCI(i) MPIDI_global.vci[i].vci

typedef struct MPIDI_CH4_Global_t {
    MPIR_Request *request_test;
    MPIR_Comm *comm_test;
    int pname_set;
    int pname_len;
    char pname[MPI_MAX_PROCESSOR_NAME];
    char parent_port[MPIDI_MAX_KVS_VALUE_LEN];
    int is_initialized;
    MPIDIU_avt_manager avt_mgr;
    int is_ch4u_initialized;
    int **node_map, max_node_id;
    MPIDIG_comm_req_list_t *comm_req_lists;
    MPIR_Commops MPIR_Comm_fns_store;
    MPID_Thread_mutex_t m[MAX_CH4_MUTEXES];
    MPIDIU_map_t *win_map;
#ifndef MPIDI_CH4U_USE_PER_COMM_QUEUE
    MPIDIG_rreq_t *posted_list;
    MPIDIG_rreq_t *unexp_list;
#endif
    MPIDIG_req_ext_t *cmpl_list;
    MPL_atomic_uint64_t exp_seq_no;
    MPL_atomic_uint64_t nxt_seq_no;
    MPIDU_genq_private_pool_t request_pool;
    MPIDU_genq_private_pool_t unexp_pack_buf_pool;
#ifdef HAVE_SIGNAL
    void (*prev_sighandler) (int);
    volatile int sigusr1_count;
    int my_sigusr1_count;
#endif

    int n_vcis;
    MPIDI_vci_t vci[MPIDI_CH4_MAX_VCIS];
    int progress_counts[MPIDI_CH4_MAX_VCIS];

#if defined(MPIDI_CH4_USE_WORK_QUEUES)
    /* TODO: move into MPIDI_vci to have per-vci workqueue */
    MPIDI_workq_t workqueue;
#endif
    MPIDI_CH4_configurations_t settings;
    void *csel_root;

#ifndef MPIDI_CH4_DIRECT_NETMOD
    MPIDI_SHM_Global_t shm;
#endif
    MPIDI_NM_Global_t nm;
} MPIDI_CH4_Global_t;
extern MPIDI_CH4_Global_t MPIDI_global;
extern char MPIDI_coll_generic_json[];
#ifdef MPL_USE_DBG_LOGGING
extern MPL_dbg_class MPIDI_CH4_DBG_GENERAL;
extern MPL_dbg_class MPIDI_CH4_DBG_MAP;
extern MPL_dbg_class MPIDI_CH4_DBG_COMM;
extern MPL_dbg_class MPIDI_CH4_DBG_MEMORY;
#endif

#endif /* CH4_TYPES_H_INCLUDED */
