/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2014 Mellanox Technologies, Inc.
 *
 */



#ifndef MX_MODULE_IMPL_H
#define MX_MODULE_IMPL_H
#include "mpid_nem_impl.h"
#include "pmi.h"
#include <mxm/api/mxm_api.h>


#if !defined(MXM_VERSION) || (MXM_API < MXM_VERSION(3,0))
#error "Unsupported MXM version, version 3.0 or above required"
#endif


int MPID_nem_mxm_init(MPIDI_PG_t * pg_p, int pg_rank, char **bc_val_p, int *val_max_sz_p);
int MPID_nem_mxm_finalize(void);
int MPID_nem_mxm_poll(int in_blocking_progress);
int MPID_nem_mxm_get_business_card(int my_rank, char **bc_val_p, int *val_max_sz_p);
int MPID_nem_mxm_connect_to_root(const char *business_card, MPIDI_VC_t * new_vc);
int MPID_nem_mxm_vc_init(MPIDI_VC_t * vc);
int MPID_nem_mxm_vc_destroy(MPIDI_VC_t * vc);
int MPID_nem_mxm_vc_terminate(MPIDI_VC_t * vc);
int MPID_nem_mxm_get_ordering(int *ordering);

/* alternate interface */
int MPID_nem_mxm_iSendContig(MPIDI_VC_t * vc, MPID_Request * sreq, void *hdr, MPIDI_msg_sz_t hdr_sz,
                             void *data, MPIDI_msg_sz_t data_sz);
int MPID_nem_mxm_iStartContigMsg(MPIDI_VC_t * vc, void *hdr, MPIDI_msg_sz_t hdr_sz, void *data,
                                 MPIDI_msg_sz_t data_sz, MPID_Request ** sreq_ptr);
int MPID_nem_mxm_SendNoncontig(MPIDI_VC_t * vc, MPID_Request * sreq, void *header,
                               MPIDI_msg_sz_t hdr_sz);

/* direct interface */
int MPID_nem_mxm_recv(MPIDI_VC_t * vc, MPID_Request * rreq);
int MPID_nem_mxm_send(MPIDI_VC_t * vc, const void *buf, MPI_Aint count, MPI_Datatype datatype,
                      int rank, int tag, MPID_Comm * comm, int context_offset,
                      MPID_Request ** sreq_p);
int MPID_nem_mxm_ssend(MPIDI_VC_t * vc, const void *buf, MPI_Aint count, MPI_Datatype datatype,
                       int rank, int tag, MPID_Comm * comm, int context_offset,
                       MPID_Request ** sreq_p);
int MPID_nem_mxm_isend(MPIDI_VC_t * vc, const void *buf, MPI_Aint count, MPI_Datatype datatype,
                       int rank, int tag, MPID_Comm * comm, int context_offset,
                       MPID_Request ** sreq_p);
int MPID_nem_mxm_issend(MPIDI_VC_t * vc, const void *buf, MPI_Aint count, MPI_Datatype datatype,
                        int rank, int tag, MPID_Comm * comm, int context_offset,
                        MPID_Request ** sreq_p);
int MPID_nem_mxm_cancel_send(MPIDI_VC_t * vc, MPID_Request * sreq);
int MPID_nem_mxm_cancel_recv(MPIDI_VC_t * vc, MPID_Request * rreq);
int MPID_nem_mxm_probe(MPIDI_VC_t * vc, int source, int tag, MPID_Comm * comm, int context_offset,
                       MPI_Status * status);
int MPID_nem_mxm_iprobe(MPIDI_VC_t * vc, int source, int tag, MPID_Comm * comm, int context_offset,
                        int *flag, MPI_Status * status);
int MPID_nem_mxm_improbe(MPIDI_VC_t * vc, int source, int tag, MPID_Comm * comm, int context_offset,
                         int *flag, MPID_Request ** message, MPI_Status * status);

int MPID_nem_mxm_anysource_iprobe(int tag, MPID_Comm * comm, int context_offset, int *flag,
                                  MPI_Status * status);
int MPID_nem_mxm_anysource_improbe(int tag, MPID_Comm * comm, int context_offset, int *flag,
                                   MPID_Request ** message, MPI_Status * status);

/* active message callback */
#define MXM_MPICH_HID_ADI_MSG         1
void MPID_nem_mxm_get_adi_msg(mxm_conn_h conn, mxm_imm_t imm, void *data,
                              size_t length, size_t offset, int last);

/* any source management */
void MPID_nem_mxm_anysource_posted(MPID_Request * req);
int MPID_nem_mxm_anysource_matched(MPID_Request * req);

int _mxm_handle_sreq(MPID_Request * req);

/* List type as queue
 * Operations, initialization etc
 */
typedef struct list_item list_item_t;
struct list_item {
    list_item_t *next;
};

typedef struct list_head list_head_t;
struct list_head {
    list_item_t *head;
    list_item_t **ptail;
    int length;
};

static inline void list_init(list_head_t * list_head)
{
    list_head->head = NULL;
    list_head->ptail = &list_head->head;
    list_head->length = 0;
}

static inline int list_length(list_head_t * list_head)
{
    return list_head->length;
}

static inline int list_is_empty(list_head_t * list_head)
{
    return list_head->length == 0;
}

static inline void list_enqueue(list_head_t * list_head, list_item_t * list_item)
{
    *list_head->ptail = list_item;
    list_head->ptail = &list_item->next;
    ++list_head->length;
}

static inline list_item_t *list_dequeue(list_head_t * list_head)
{
    list_item_t *list_item;

    if (list_is_empty(list_head)) {
        return NULL;
    }

    list_item = list_head->head;
    list_head->head = list_item->next;
    --list_head->length;
    if (list_head->length == 0) {
        list_head->ptail = &list_head->head;
    }
    return list_item;
}


#define MXM_MPICH_MQ_ID 0x8888
#define MXM_MPICH_MAX_ADDR_SIZE 512
#define MXM_MPICH_ENDPOINT_KEY "endpoint_id"
#define MXM_MPICH_MAX_REQ 100
#define MXM_MPICH_MAX_IOV 3


/* The vc provides a generic buffer in which network modules can store
   private fields This removes all dependencies from the VC structure
   on the network module, facilitating dynamic module loading.
 */
typedef struct {
    mxm_conn_h mxm_conn;
    list_head_t free_queue;
} MPID_nem_mxm_ep_t;

typedef struct {
    MPIDI_VC_t *ctx;
    MPID_nem_mxm_ep_t *mxm_ep;
    int pending_sends;
} MPID_nem_mxm_vc_area;

/* macro for mxm private in VC */
#define VC_BASE(vcp) ((vcp) ? (MPID_nem_mxm_vc_area *)((vcp)->ch.netmod_area.padding) : NULL)

/* The req provides a generic buffer in which network modules can store
   private fields This removes all dependencies from the req structure
   on the network module, facilitating dynamic module loading. */
typedef struct {
    list_item_t queue;
    union {
        mxm_req_base_t base;
        mxm_send_req_t send;
        mxm_recv_req_t recv;
    } item;
} MPID_nem_mxm_req_t;

typedef struct {
    MPID_Request *ctx;
    MPID_nem_mxm_req_t *mxm_req;
    mxm_req_buffer_t *iov_buf;
    int iov_count;
    mxm_req_buffer_t tmp_buf[MXM_MPICH_MAX_IOV];
} MPID_nem_mxm_req_area;

/* macro for mxm private in REQ */
#define REQ_BASE(reqp) ((reqp) ? (MPID_nem_mxm_req_area *)((reqp)->ch.netmod_area.padding) : NULL)

typedef GENERIC_Q_DECL(struct MPID_Request) MPID_nem_mxm_reqq_t;
#define MPID_nem_mxm_queue_empty(q) GENERIC_Q_EMPTY (q)
#define MPID_nem_mxm_queue_head(q) GENERIC_Q_HEAD (q)
#define MPID_nem_mxm_queue_enqueue(qp, ep) do {                                           \
        /* add refcount so req doesn't get freed before it's dequeued */                \
        MPIR_Request_add_ref(ep);                                                       \
        MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST,                         \
                          "MPID_nem_mxm_queue_enqueue req=%p (handle=%#x), queue=%p",     \
                          ep, (ep)->handle, qp));                                       \
        GENERIC_Q_ENQUEUE (qp, ep, dev.next);                                           \
    } while (0)
#define MPID_nem_mxm_queue_dequeue(qp, ep)  do {                                          \
        GENERIC_Q_DEQUEUE (qp, ep, dev.next);                                           \
        MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST,                         \
                          "MPID_nem_mxm_queue_dequeuereq=%p (handle=%#x), queue=%p",      \
                          *(ep), *(ep) ? (*(ep))->handle : -1, qp));                    \
        MPID_Request_release(*(ep));                                                    \
    } while (0)

typedef struct MPID_nem_mxm_module_t {
    char *runtime_version;
    const char *compiletime_version;
    mxm_context_opts_t *mxm_ctx_opts;
    mxm_ep_opts_t *mxm_ep_opts;
    mxm_h mxm_context;
    mxm_mq_h mxm_mq;
    mxm_ep_h mxm_ep;
    char mxm_ep_addr[MXM_MPICH_MAX_ADDR_SIZE];
    size_t mxm_ep_addr_size;
    int mxm_rank;
    int mxm_np;
    MPID_nem_mxm_ep_t *endpoint;
    list_head_t free_queue;
    MPID_nem_mxm_reqq_t sreq_queue;
    struct {
        int bulk_connect;       /* use bulk connect */
        int bulk_disconnect;    /* use bulk disconnect */
    } conf;
} MPID_nem_mxm_module_t;

extern MPID_nem_mxm_module_t *mxm_obj;

#define container_of(ptr, type, member) (type *)((char *)(ptr) - offsetof(type,member))
#define list_dequeue_mxm_req(head) \
    container_of(list_dequeue(head), MPID_nem_mxm_req_t, queue)
static inline void list_grow_mxm_req(list_head_t * list_head)
{
    MPID_nem_mxm_req_t *mxm_req = NULL;
    int count = MXM_MPICH_MAX_REQ;

    while (count--) {
        mxm_req = (MPID_nem_mxm_req_t *) MPIU_Malloc(sizeof(MPID_nem_mxm_req_t));
        list_enqueue(list_head, &mxm_req->queue);
    }
}

static inline void _mxm_barrier(void)
{
    int pmi_errno ATTRIBUTE((unused));

#ifdef USE_PMI2_API
    pmi_errno = PMI2_KVS_Fence();
#else
    pmi_errno = PMI_Barrier();
#endif
}

static inline void _mxm_to_mpi_status(mxm_error_t mxm_error, MPI_Status * mpi_status)
{
    switch (mxm_error) {
    case MXM_OK:
        mpi_status->MPI_ERROR = MPI_SUCCESS;
        break;
    case MXM_ERR_CANCELED:
        MPIR_STATUS_SET_CANCEL_BIT(*mpi_status, TRUE);
        mpi_status->MPI_ERROR = MPI_SUCCESS;
        break;
    case MXM_ERR_MESSAGE_TRUNCATED:
        mpi_status->MPI_ERROR = MPI_ERR_TRUNCATE;
        break;
    default:
        mpi_status->MPI_ERROR = MPI_ERR_INTERN;
        break;
    }
}

static inline int _mxm_req_test(mxm_req_base_t * req)
{
    return req->state == MXM_REQ_COMPLETED;
}

static inline void _mxm_progress_cb(void *user_data)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIDI_CH3_Progress_poke();
    MPIU_Assert(mpi_errno == MPI_SUCCESS);
}

static inline void _mxm_req_wait(mxm_req_base_t * req)
{
    mxm_wait_t mxm_wreq;

    mxm_wreq.req = req;
    mxm_wreq.state = MXM_REQ_COMPLETED;
    mxm_wreq.progress_cb = _mxm_progress_cb;
    mxm_wreq.progress_arg = NULL;

    mxm_wait(&mxm_wreq);
}

static inline int _mxm_eager_threshold(void)
{
    return 262144;
}

/*
 * Tag management section
 */
static inline mxm_tag_t _mxm_tag_mpi2mxm(int mpi_tag, MPIU_Context_id_t context_id)
{
    mxm_tag_t mxm_tag;

    mxm_tag = (mpi_tag == MPI_ANY_TAG ? 0 : mpi_tag) & 0x7fffffff;
    mxm_tag |= (context_id << 31) & 0x80000000;

    return mxm_tag;
}

static inline int _mxm_tag_mxm2mpi(mxm_tag_t mxm_tag)
{
    return (mxm_tag & 0x7fffffff);
}

static inline mxm_tag_t _mxm_tag_mask(int mpi_tag)
{
    return (mpi_tag == MPI_ANY_TAG ? 0x80000000U : ~(MPIR_TAG_PROC_FAILURE_BIT | MPIR_TAG_ERROR_BIT));
}

/*
 * Debugging section
 */

#define MXM_DEBUG   0
#define MXM_DEBUG_PREFIX   "MXM"


static inline void _dbg_mxm_out(int level,
                                FILE * output_id,
                                int cr,
                                const char *file_name,
                                const char *func_name, int line_no, const char *format, ...)
{
    va_list args;
    char str[200];
    int ret;

    if (level < MXM_DEBUG) {
        output_id = (output_id ? output_id : stderr);

        va_start(args, format);

        ret = vsnprintf(str, sizeof(str), format, args);
        assert(-1 != ret);

        if (cr) {
//            ret = fprintf(output_id, "[%s #%d] %s  %s:%s:%d", MXM_DEBUG_PREFIX, MPIR_Process.comm_world->rank, str, file_name, func_name, line_no);
            ret =
                fprintf(output_id, "[%s #%d] %s", MXM_DEBUG_PREFIX, MPIR_Process.comm_world->rank,
                        str);
        }
        else {
            ret = fprintf(output_id, "%s", str);
        }
        assert(-1 != ret);

        va_end(args);
    }
}

static inline void _dbg_mxm_hexdump(void *ptr, int buflen)
{
    unsigned char *buf = (unsigned char *) ptr;
    char *str = NULL;
    int len = 0;
    int cur_len = 0;
    int i, j;

    if (!ptr)
        return;

    len = 80 * (buflen / 16 + 1);
    str = (char *) MPIU_Malloc(len);
    for (i = 0; i < buflen; i += 16) {
        cur_len += MPL_snprintf(str + cur_len, len - cur_len, "%06x: ", i);
        for (j = 0; j < 16; j++)
            if (i + j < buflen)
                cur_len += MPL_snprintf(str + cur_len, len - cur_len, "%02x ", buf[i + j]);
            else
                cur_len += MPL_snprintf(str + cur_len, len - cur_len, "   ");
        cur_len += MPL_snprintf(str + cur_len, len - cur_len, " ");
        for (j = 0; j < 16; j++)
            if (i + j < buflen)
                cur_len +=
                    MPL_snprintf(str + cur_len, len - cur_len, "%c",
                                  isprint(buf[i + j]) ? buf[i + j] : '.');
        cur_len += MPL_snprintf(str + cur_len, len - cur_len, "\n");
    }
    _dbg_mxm_out(8, NULL, 1, NULL, NULL, -1, "%s", str);
    MPIU_Free(str);
}

static inline char *_tag_val_to_str(int tag, char *out, int max)
{
    if (tag == MPI_ANY_TAG) {
        MPIU_Strncpy(out, "MPI_ANY_TAG", max);
    }
    else {
        MPL_snprintf(out, max, "%d", tag);
    }
    return out;
}

static inline char *_rank_val_to_str(int rank, char *out, int max)
{
    if (rank == MPI_ANY_SOURCE) {
        MPIU_Strncpy(out, "MPI_ANY_SOURCE", max);
    }
    else {
        MPL_snprintf(out, max, "%d", rank);
    }
    return out;
}

static inline void _dbg_mxm_req(MPID_Request * req)
{
    char tag_buf[128];
    char rank_buf[128];

    if (req) {
        _dbg_mxm_out(10, NULL, 1, NULL, NULL, -1,
                     "[ctx=%#x rank=%d] req=%p ctx=%#x rank=%s tag=%s kind=%d\n",
                     MPIR_Process.comm_world->context_id, MPIR_Process.comm_world->rank,
                     req, req->dev.match.parts.context_id,
                     _rank_val_to_str(req->dev.match.parts.rank, rank_buf, sizeof(rank_buf)),
                     _tag_val_to_str(req->dev.match.parts.tag, tag_buf, sizeof(tag_buf)),
                     req->kind);
    }
}


#if defined(MXM_DEBUG) && (MXM_DEBUG > 0)
#define _dbg_mxm_out_buf(ptr, len)      _dbg_mxm_hexdump(ptr, len)
#define _dbg_mxm_out_req(req)           _dbg_mxm_req(req)
#define _dbg_mxm_output(level, ...)     _dbg_mxm_out(level, NULL, 1, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
#else
#define _dbg_mxm_out_buf(ptr, len)      ((void)0)
#define _dbg_mxm_out_req(req)           ((void)0)
#define _dbg_mxm_output(level, ...)     ((void)0)
#endif

#endif
