/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef CH4_TYPES_H_INCLUDED
#define CH4_TYPES_H_INCLUDED

#include <mpidimpl.h>
#include <stdio.h>
#include "mpir_cvars.h"
#include "mpidu_genq.h"

/* Macros and inlines */
#define MPIDIU_MAP_NOT_FOUND      ((void*)(-1UL))

#define MPIDIU_REQUEST_POOL_NUM_CELLS_PER_CHUNK (1024)
#define MPIDIU_REQUEST_POOL_CELL_SIZE (256)

/* Flags for MPIDI_Progress_test
 *
 * Flags argument allows to control execution of different parts of progress function,
 * for aims of prioritization of different transports and reentrant-safety of progress call.
 *
 * MPIDI_PROGRESS_HOOKS - enables progress on progress hooks. Hooks may invoke upper-level logic internally,
 *      that's why MPIDI_Progress_test call with MPIDI_PROGRESS_HOOKS set isn't reentrant safe, and shouldn't be called from netmod's fallback logic.
 * MPIDI_PROGRESS_NM and MPIDI_PROGRESS_SHM enables progress on transports only, and guarantee reentrant-safety.
 */
#define MPIDI_PROGRESS_HOOKS  (1)
#define MPIDI_PROGRESS_NM     (1<<1)
#define MPIDI_PROGRESS_SHM    (1<<2)
#define MPIDI_PROGRESS_NM_LOCKLESS     (1<<3)   /* to support lockless MT model */

#define MPIDI_PROGRESS_ALL (MPIDI_PROGRESS_HOOKS|MPIDI_PROGRESS_NM|MPIDI_PROGRESS_SHM)

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
    int flags;
    MPIR_Request *sreq_ptr;
    MPI_Aint data_sz;
    MPI_Aint rndv_hdr_sz;
} MPIDIG_hdr_t;

typedef struct MPIDIG_send_cts_msg_t {
    MPIR_Request *sreq_ptr;
    MPIR_Request *rreq_ptr;
} MPIDIG_send_cts_msg_t;

typedef struct MPIDIG_send_data_msg_t {
    MPIR_Request *rreq_ptr;
} MPIDIG_send_data_msg_t;

typedef struct MPIDIG_ssend_ack_msg_t {
    MPIR_Request *sreq_ptr;
} MPIDIG_ssend_ack_msg_t;

typedef struct MPIDIG_part_send_init_msg_t {
    int src_rank;
    int tag;
    MPIR_Context_id_t context_id;
    MPIR_Request *sreq_ptr;
    MPI_Aint data_sz;           /* size of entire send data */
} MPIDIG_part_send_init_msg_t;

typedef struct MPIDIG_part_cts_msg_t {
    MPIR_Request *sreq_ptr;
    MPIR_Request *rreq_ptr;
} MPIDIG_part_cts_msg_t;

typedef struct MPIDIG_part_send_data_msg_t {
    MPIR_Request *rreq_ptr;
} MPIDIG_part_send_data_msg_t;

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
    MPI_Aint origin_data_sz;
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
    MPI_Aint target_data_sz;
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
    MPI_Aint result_data_sz;
    int n_iov;
    int flattened_sz;
} MPIDIG_acc_req_msg_t;

typedef struct MPIDIG_acc_req_msg_t MPIDIG_get_acc_req_msg_t;

typedef struct MPIDIG_acc_ack_msg_t {
    MPIR_Request *req_ptr;
} MPIDIG_acc_ack_msg_t;

typedef MPIDIG_acc_ack_msg_t MPIDIG_get_acc_ack_msg_t;

typedef struct {
    MPIR_OBJECT_HEADER;
    int size;
    MPIDI_av_entry_t table[];
} MPIDI_av_table_t;

typedef struct {
    int max_n_avts;
    int n_avts;
    int n_free;
    MPIDI_av_table_t *av_table0;
    MPIDI_av_table_t **av_tables;
} MPIDIU_avt_manager;

#define MPIDIU_get_av_table(avtid) (MPIDI_global.avt_mgr.av_tables[(avtid)])
#define MPIDIU_get_av(avtid, lpid) (MPIDI_global.avt_mgr.av_tables[(avtid)]->table[(lpid)])

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

#define MPIDIU_THREAD_SCHED_LIST_MUTEX    MPIDI_global.m[3]
#define MPIDIU_THREAD_TSP_QUEUE_MUTEX     MPIDI_global.m[4]
#ifdef HAVE_HCOLL
#define MPIDIU_THREAD_HCOLL_MUTEX         MPIDI_global.m[5]
#endif

/* Protects dynamic process tag, connection_id, avtable etc. */
#define MPIDIU_THREAD_DYNPROC_MUTEX       MPIDI_global.m[6]

/* Protects alloc_mem hash */
#define MPIDIU_THREAD_ALLOC_MEM_MUTEX     MPIDI_global.m[7]

#define MAX_CH4_MUTEXES 8

extern MPID_Thread_mutex_t MPIR_THREAD_VCI_HANDLE_POOL_MUTEXES[REQUEST_POOL_MAX];

/* per-VCI structure -- using union to force minimum size */
typedef struct MPIDI_per_vci {
    MPID_Thread_mutex_t lock;
    /* The progress counts are mostly accessed in a VCI critical section and thus updated in a
     * relaxed manner.  MPL_atomic_int_t is used here only for MPIDI_set_progress_vci() and
     * MPIDI_set_progress_vci_n(), which access these progress counts outside a VCI critical
     * section. */
    MPL_atomic_int_t progress_count;

    MPIR_Request *posted_list;
    MPIR_Request *unexp_list;
    MPIDU_genq_private_pool_t request_pool;
    MPIDU_genq_private_pool_t pack_buf_pool;

    MPIDIG_req_ext_t *cmpl_list;
    MPL_atomic_uint64_t exp_seq_no;
    MPL_atomic_uint64_t nxt_seq_no;

    bool allocated;
    char pad MPL_ATTR_ALIGNED(MPL_CACHELINE_SIZE);
} MPIDI_per_vci_t;

#define MPIDI_VCI(i) MPIDI_global.per_vci[i]

typedef struct MPIDI_CH4_Global_t {
    int pname_set;
    int pname_len;
    char pname[MPI_MAX_PROCESSOR_NAME];
    char parent_port[MPIDI_MAX_KVS_VALUE_LEN];
    int is_initialized;
    MPIDIU_avt_manager avt_mgr;
    MPIR_Commops MPIR_Comm_fns_store;
    MPID_Thread_mutex_t m[MAX_CH4_MUTEXES];
    MPIDIU_map_t *win_map;

    MPIR_Request *part_posted_list;
    MPIR_Request *part_unexp_list;

#ifdef HAVE_SIGNAL
    void (*prev_sighandler) (int);
    volatile int sigusr1_count;
    int my_sigusr1_count;
#endif

    int n_vcis;                 /* num of vcis used for implicit hashing */
    int n_reserved_vcis;        /* num of reserved vcis */
    int n_total_vcis;           /* total num of vcis, must > n_vcis + n_reserved_vcis */
    MPIDI_per_vci_t per_vci[MPIDI_CH4_MAX_VCIS];

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
