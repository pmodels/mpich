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
#ifndef MPIDPRE_H_INCLUDED
#define MPIDPRE_H_INCLUDED

#if defined(HAVE_SYS_TYPES_H)
#include <sys/types.h>
#endif

#include "mpidu_dataloop.h"
#include "mpid_thread.h"
#include "mpid_sched.h"
#include "mpid_timers_fallback.h"
#include "netmodpre.h"
#include "shmpre.h"
#include "mpl_uthash.h"

typedef struct {
    union {
    MPIDI_NM_DT_DECL} netmod;
} MPIDI_Devdt_t;
#define MPID_DEV_DATATYPE_DECL   MPIDI_Devdt_t   dev;
#include "mpid_datatype_fallback.h"

typedef int MPID_Progress_state;

#define CH4_COMPILE_TIME_ASSERT(expr_)                                  \
  do { switch(0) { case 0: case (expr_): default: break; } } while (0)

typedef enum {
    MPIDI_PTYPE_RECV,
    MPIDI_PTYPE_SEND,
    MPIDI_PTYPE_BSEND,
    MPIDI_PTYPE_SSEND
} MPIDI_ptype;

#define MPIDI_CH4U_REQ_BUSY 		  (0x1)
#define MPIDI_CH4U_REQ_PEER_SSEND 	  (0x1 << 1)
#define MPIDI_CH4U_REQ_UNEXPECTED 	  (0x1 << 2)
#define MPIDI_CH4U_REQ_UNEXP_DQUED 	  (0x1 << 3)
#define MPIDI_CH4U_REQ_UNEXP_CLAIMED  (0x1 << 4)
#define MPIDI_CH4U_REQ_RCV_NON_CONTIG (0x1 << 5)
#define MPIDI_CH4U_REQ_MATCHED (0x1 << 6)
#define MPIDI_CH4U_REQ_LONG_RTS (0x1 << 7)

#define MPIDI_PARENT_PORT_KVSKEY "PARENT_ROOT_PORT_NAME"
#define MPIDI_MAX_KVS_VALUE_LEN  4096

typedef struct MPIDI_CH4U_sreq_t {
    /* persistent send fields */
} MPIDI_CH4U_sreq_t;

typedef struct MPIDI_CH4U_lreq_t {
    /* Long send fields */
    const void *src_buf;
    MPI_Count count;
    MPI_Datatype datatype;
    uint64_t msg_tag;
} MPIDI_CH4U_lreq_t;

typedef struct MPIDI_CH4U_rreq_t {
    /* mrecv fields */
    void *mrcv_buffer;
    uint64_t mrcv_count;
    MPI_Datatype mrcv_datatype;

    uint64_t ignore;
    uint64_t peer_req_ptr;
    uint64_t match_req;
    uint64_t request;

    struct MPIDI_CH4U_rreq_t *prev, *next;
} MPIDI_CH4U_rreq_t;

typedef struct MPIDI_CH4U_put_req_t {
    MPIR_Win *win_ptr;
    uint64_t preq_ptr;
    void *dt_iov;
    void *origin_addr;
    int origin_count;
    MPI_Datatype origin_datatype;
    int n_iov;
} MPIDI_CH4U_put_req_t;

typedef struct MPIDI_CH4U_get_req_t {
    MPIR_Win *win_ptr;
    uint64_t greq_ptr;
    uint64_t addr;
    MPI_Datatype datatype;
    int count;
    int n_iov;
    void *dt_iov;
} MPIDI_CH4U_get_req_t;

typedef struct MPIDI_CH4U_cswap_req_t {
    MPIR_Win *win_ptr;
    uint64_t creq_ptr;
    uint64_t addr;
    MPI_Datatype datatype;
    void *data;
    void *result_addr;
} MPIDI_CH4U_cswap_req_t;

typedef struct MPIDI_CH4U_acc_req_t {
    MPIR_Win *win_ptr;
    uint64_t req_ptr;
    MPI_Datatype origin_datatype;
    MPI_Datatype target_datatype;
    int origin_count;
    int target_count;
    int n_iov;
    void *target_addr;
    void *dt_iov;
    void *data;
    size_t data_sz;
    MPI_Op op;
    void *result_addr;
    int result_count;
    int do_get;
    void *origin_addr;
    MPI_Datatype result_datatype;
} MPIDI_CH4U_acc_req_t;

typedef struct MPIDI_CH4U_req_ext_t {
    union {
        MPIDI_CH4U_sreq_t sreq;
        MPIDI_CH4U_lreq_t lreq;
        MPIDI_CH4U_rreq_t rreq;
        MPIDI_CH4U_put_req_t preq;
        MPIDI_CH4U_get_req_t greq;
        MPIDI_CH4U_cswap_req_t creq;
        MPIDI_CH4U_acc_req_t areq;
    };

    struct iovec *iov;
    void *cmpl_handler_fn;
    uint64_t seq_no;
    uint64_t request;
    uint64_t status;
    struct MPIDI_CH4U_req_ext_t *next, *prev;

} MPIDI_CH4U_req_ext_t;

typedef struct MPIDI_CH4U_req_t {
    union {
    MPIDI_NM_REQUEST_AM_DECL} netmod_am;
    MPIDI_CH4U_req_ext_t *req;
    MPIDI_ptype p_type;
    void *buffer;
    uint64_t count;
    uint64_t tag;
    int src_rank;
    MPI_Datatype datatype;
} MPIDI_CH4U_req_t;

typedef struct {
#ifdef MPIDI_CH4_EXCLUSIVE_SHM
    int is_local;
#endif
    /* Anysource handling. Netmod and shm specific requests are cross
     * referenced. This must be present all of the time to avoid lots of extra
     * ifdefs in the code. */
#ifdef MPIDI_BUILD_CH4_SHM
    struct MPIR_Request *anysource_partner_request;
#endif

    union {
        /* The first fields are used by the CH4U apis */
        MPIDI_CH4U_req_t ch4u;

        /* Used by the netmod direct apis */
        union {
        MPIDI_NM_REQUEST_DECL} netmod;

        union {
        MPIDI_SHM_REQUEST_DECL} shm;
    } ch4;
} MPIDI_Devreq_t;
#define MPIDI_REQUEST_HDR_SIZE              offsetof(struct MPIR_Request, dev.ch4.netmod)
#define MPIDI_REQUEST_CH4U_HDR_SIZE         offsetof(struct MPIR_Request, dev.ch4.netmod_am)
#define MPIDI_CH4I_REQUEST(req,field)       (((req)->dev).field)
#define MPIDI_CH4U_REQUEST(req,field)       (((req)->dev.ch4.ch4u).field)

#ifdef MPIDI_BUILD_CH4_SHM
#define MPIDI_CH4I_REQUEST_ANYSOURCE_PARTNER(req)  (((req)->dev).anysource_partner_request)
#else
#define MPIDI_CH4I_REQUEST_ANYSOURCE_PARTNER(req)  NULL
#endif

MPL_STATIC_INLINE_PREFIX void MPID_Request_create_hook(struct MPIR_Request *req);

MPL_STATIC_INLINE_PREFIX void MPID_Request_free_hook(struct MPIR_Request *req);

typedef struct MPIDI_CH4U_win_shared_info {
    uint32_t disp_unit;
    size_t size;
} __attribute__ ((packed)) MPIDI_CH4U_win_shared_info_t;

#define MPIDI_CH4I_ACCU_ORDER_RAR (1)
#define MPIDI_CH4I_ACCU_ORDER_RAW (1 << 1)
#define MPIDI_CH4I_ACCU_ORDER_WAR (1 << 2)
#define MPIDI_CH4I_ACCU_ORDER_WAW (1 << 3)

typedef enum {
    MPIDI_CH4I_ACCU_SAME_OP,
    MPIDI_CH4I_ACCU_SAME_OP_NO_OP
} MPIDI_CH4U_win_info_accumulate_ops;

typedef struct MPIDI_CH4U_win_info_args_t {
    int no_locks;
    int same_size;
    int accumulate_ordering;
    int alloc_shared_noncontig;
    MPIDI_CH4U_win_info_accumulate_ops accumulate_ops;
} MPIDI_CH4U_win_info_args_t;

struct MPIDI_CH4U_win_lock {
    struct MPIDI_CH4U_win_lock *next;
    int rank;
    uint16_t mtype;
    uint16_t type;
};

struct MPIDI_CH4U_win_queue {
    struct MPIDI_CH4U_win_lock *head;
    struct MPIDI_CH4U_win_lock *tail;
};

typedef struct MPIDI_CH4U_win_lock_info {
    unsigned peer;
    int lock_type;
    struct MPIR_Win *win;
    volatile unsigned done;
} MPIDI_CH4U_win_lock_info;

typedef struct MPIDI_CH4U_win_sync_lock {
    struct {
        volatile unsigned locked;
        volatile unsigned allLocked;
    } remote;
    struct {
        struct MPIDI_CH4U_win_queue requested;
        int type;
        unsigned count;
    } local;
} MPIDI_CH4U_win_sync_lock;

typedef struct MPIDI_CH4U_win_sync_pscw {
    struct MPIR_Group *group;
    volatile unsigned count;
} MPIDI_CH4U_win_sync_pscw;

typedef struct MPIDI_CH4U_win_sync_t {
    volatile int origin_epoch_type;
    volatile int target_epoch_type;
    MPIDI_CH4U_win_sync_pscw sc, pw;
    MPIDI_CH4U_win_sync_lock lock;
} MPIDI_CH4U_win_sync_t;

typedef struct MPIDI_CH4U_win_t {
    uint64_t win_id;
    void *mmap_addr;
    int64_t mmap_sz;
    OPA_int_t outstanding_ops;
    MPI_Aint *sizes;
    MPIDI_CH4U_win_lock_info *lockQ;
    MPIDI_CH4U_win_sync_t sync;
    MPIDI_CH4U_win_info_args_t info_args;
    MPIDI_CH4U_win_shared_info_t *shared_table;
    MPL_UT_hash_handle hash_handle;
} MPIDI_CH4U_win_t;

typedef struct {
    MPIDI_CH4U_win_t ch4u;
    union {
    MPIDI_NM_WIN_DECL} netmod;
} MPIDI_Devwin_t;

#define MPIDI_CH4U_WIN(win,field)        (((win)->dev.ch4u).field)
#define MPIDI_CH4U_WINFO(win,rank) (MPIDI_CH4U_win_info_t*) &(MPIDI_CH4U_WIN(win, info_table)[rank])

typedef unsigned MPIDII_locality_t;

typedef struct MPIDI_CH4U_comm_t {
    MPIDI_CH4U_rreq_t *posted_list;
    MPIDI_CH4U_rreq_t *unexp_list;
    uint32_t window_instance;
} MPIDI_CH4U_comm_t;

#define MPIDII_CALC_STRIDE(rank, stride, blocksize, offset) \
    ((rank) / (blocksize) * ((stride) - (blocksize)) + (rank) + (offset))

#define MPIDII_CALC_STRIDE_SIMPLE(rank, stride, offset) \
    ((rank) * (stride) + (offset))

typedef enum {
    MPIDII_RANK_MAP_DIRECT,
    MPIDII_RANK_MAP_DIRECT_INTRA,
    MPIDII_RANK_MAP_OFFSET,
    MPIDII_RANK_MAP_OFFSET_INTRA,
    MPIDII_RANK_MAP_STRIDE,
    MPIDII_RANK_MAP_STRIDE_INTRA,
    MPIDII_RANK_MAP_STRIDE_BLOCK,
    MPIDII_RANK_MAP_STRIDE_BLOCK_INTRA,
    MPIDII_RANK_MAP_LUT,
    MPIDII_RANK_MAP_LUT_INTRA,
    MPIDII_RANK_MAP_MLUT,
    MPIDII_RANK_MAP_NONE
} MPIDII_rank_map_mode;

typedef int MPIDII_lpid_t;
typedef struct {
    int avtid;
    int lpid;
} MPIDII_gpid_t;

typedef struct {
    MPIR_OBJECT_HEADER;
    MPIDII_lpid_t lpid[0];
} MPIDII_rank_map_lut_t;

typedef struct {
    MPIR_OBJECT_HEADER;
    MPIDII_gpid_t gpid[0];
} MPIDII_rank_map_mlut_t;

typedef struct {
    MPIDII_rank_map_mode mode;
    int avtid;
    int size;

    union {
        int offset;
        struct {
            int offset;
            int stride;
            int blocksize;
        } stride;
    } reg;

    union {
        struct {
            MPIDII_rank_map_lut_t *t;
            MPIDII_lpid_t *lpid;
        } lut;
        struct {
            MPIDII_rank_map_mlut_t *t;
            MPIDII_gpid_t *gpid;
        } mlut;
    } irreg;
} MPIDII_rank_map_t;

typedef struct MPIDI_Devcomm_t {
    struct {
        /* The first fields are used by the CH4U apis */
        MPIDI_CH4U_comm_t ch4u;

        /* Used by the netmod direct apis */
        union {
        MPIDI_NM_COMM_DECL} netmod;

        union {
        MPIDI_SHM_COMM_DECL} shm;

        MPIDII_rank_map_t map;
        MPIDII_rank_map_t local_map;
    } ch4;
} MPIDI_Devcomm_t;
#define MPIDI_CH4U_COMM(comm,field) ((comm)->dev.ch4.ch4u).field
#define MPIDII_COMM(comm,field) ((comm)->dev.ch4).field


#define MPID_USE_NODE_IDS
typedef uint16_t MPID_Node_id_t;

typedef struct {
    union {
    MPIDI_NM_OP_DECL} netmod;
} MPIDI_Devop_t;

#define MPID_DEV_REQUEST_DECL    MPIDI_Devreq_t  dev;
#define MPID_DEV_WIN_DECL        MPIDI_Devwin_t  dev;
#define MPID_DEV_COMM_DECL       MPIDI_Devcomm_t dev;
#define MPID_DEV_OP_DECL         MPIDI_Devop_t   dev;

typedef struct {
    union {
    MPIDI_NM_ADDR_DECL} netmod;
#ifdef MPIDI_BUILD_CH4_LOCALITY_INFO
    MPIDII_locality_t is_local;
#endif
} MPIDII_av_entry_t;

typedef struct {
    MPIR_OBJECT_HEADER;
    int size;
    MPIDII_av_entry_t table[0];
} MPIDII_av_table_t;

extern MPIDII_av_table_t **MPIDII_av_table;
extern MPIDII_av_table_t *MPIDII_av_table0;

#define MPIDIU_get_av_table(avtid) (MPIDII_av_table[(avtid)])
#define MPIDIU_get_av(avtid, lpid) (MPIDII_av_table[(avtid)]->table[(lpid)])

#define MPIDIU_get_node_map(avtid)   (MPIDI_CH4_Global.node_map[(avtid)])

#define MPID_Progress_register_hook(fn_, id_) MPID_Progress_register(fn_, id_)
#define MPID_Progress_deregister_hook(id_) MPID_Progress_deregister(id_)
#define MPID_Progress_activate_hook(id_) MPID_Progress_activate(id_)
#define MPID_Progress_deactivate_hook(id_) MPID_Progress_deactivate(id_)

#define HAVE_DEV_COMM_HOOK
#define MPID_Comm_create_hook   MPIDI_Comm_create_hook
#define MPID_Comm_free_hook     MPIDI_Comm_free_hook

#define MPID_Type_create_hook   MPIDI_Type_create_hook
#define MPID_Type_free_hook     MPIDI_Type_free_hook

#define MPID_Op_create_hook     MPIDI_Op_create_hook
#define MPID_Op_free_hook       MPIDI_Op_free_hook

/* operation for (avtid, lpid) to/from "lpid64" */
/* hard code limit on number of live comm worlds. This should be fixed by future
 * LUPID patch */
#define MPIDIU_AVTID_BITS                    (8)
#define MPIDIU_LPID_BITS                     (24)
#define MPIDIU_LPID_MASK                     (0x00FFFFFFU)
#define MPIDIU_AVTID_MASK                    (0xFF000000U)
#define MPIDIU_NEW_AVT_MARK                  (0x80000000U)
#define MPIDIU_LUPID_CREATE(avtid, lpid)      (((avtid) << MPIDIU_LPID_BITS) | (lpid))
#define MPIDIU_LUPID_GET_AVTID(lupid)          ((((lupid) & MPIDIU_AVTID_MASK) >> MPIDIU_LPID_BITS))
#define MPIDIU_LUPID_GET_LPID(lupid)           (((lupid) & MPIDIU_LPID_MASK))
#define MPIDIU_LUPID_SET_NEW_AVT_MARK(lupid)   ((lupid) |= MPIDIU_NEW_AVT_MARK)
#define MPIDIU_LUPID_CLEAR_NEW_AVT_MARK(lupid) ((lupid) &= (~MPIDIU_NEW_AVT_MARK))
#define MPIDIU_LUPID_IS_NEW_AVT(lupid)         ((lupid) & MPIDIU_NEW_AVT_MARK)

#define MPIDII_DYNPROC_MASK                 (0x80000000U)

#define MPID_INTERCOMM_NO_DYNPROC(comm) \
    (MPIDII_COMM((comm),map).avtid == 0 && MPIDII_COMM((comm),local_map).avtid == 0)


#include "mpidu_pre.h"

#endif /* MPIDPRE_H_INCLUDED */
