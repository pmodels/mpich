/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2019 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef CH4_AM_H_INCLUDED
#define CH4_AM_H_INCLUDED

/* Principle:
 * - do not assert delivery method such as how to pack active message header, leave it to device
 * - opaque header and payload, that is the only objects needed for handler
 * - opaque completion context, that is the only objects needed for send-side completion callback
 */

/* Transport interface:
 *
 * MPIDI_NM_am_send_contig(int target_rank, int payload_type,
 *                         int handler_id, int completion_id,
 *                         int header_size, void *header,
 *                         int payload_size, void *payload, void *context)
 *
 *     - Either pack header and payload as a single packet (e.g. payload small)
 *     - Or pack header and send payload as IOV or separately (e.g. payload big or payload is GPU)
 *     - msgid, header and payload are sent to target for running handlers
 *     - context is for send-completion callbacks
 *
 * Additional interface (for potential better efficiency). NM/SHM always can implement these by
 * falling back to send_contig.
 *
 * MPIDI_NM_am_send_header(int target_rank, int handler_id, int completion_id,
 *                         int header size, void *header, void *context)
 * MPIDI_NM_am_send_noncontig(int target_rank, void *data, int count, MPI_Datatype datatype,
 *                            int handler_id, int completion_id,
 *                            int header_size, void *header, void *context)
 *
 *     - the send_header allows NETMOD/SHMMOD bypass the check for payload
 *     - the send_noncontig allows NETMOD/SHMMOD a chance to do something more efficient for
 *       certain non-contig payload datatype
 */

/* MPIDI_NM_am_send_contig require a payload_type. This is used to mark when a payload is of
 * a special type, e.g. GPU memory.
 */

enum MPIDI_am_payload_type {
    MPIDI_AM_normal = 0,
    MPIDI_AM_GPU
};

/* In the function prototype, header refers to protocol header that is opaque to transport layer.
 * However, since transport layer will need add their own header, it appear undesirable.
 * To circumvent, we require all protocol header have MPIDI_AM_PADDING_SIZE bytes of padding space
 * at the beginning of each protocol header. This padding space is used by transport layer to pack
 * necessary transport-layer information.
 *
 * Example header structure (defined internally inside protocol):
 * struct rndv_header {
 *     char pad[MPIDI_AM_PADDING_SIZE];
 *     ...
 * };
 *
 * Transport-layer defines its own structure within MPIDI_AM_PADDING_SIZE limit. Typically it will
 * need store handler-id, header_size, transport_type etc.
 */

#define MPIDI_AM_PADDING_SIZE 8 /* Make it multiple of desired alignment to avoid waste */

/* Protocol-layer interfaces */
/* Completion callback is for asynchronous MPIDI_(NM/SHM)_am_send to call at completion.
 * A typical context is a request ptr, and a typical callback is to decr request's cc_ctr.
 *
 * Handler function is called by target when the active message is received.
 */
/* Is there need for handlers return int? */
typedef int (*MPIDI_am_handerler_fn) (void *header, int payload_size, void *payload);
typedef int (*MPIDI_am_completion_cb) (void *context);
int MPIDI_am_register_completion(MPIDI_am_completion_cb fn);
int MPIDI_am_register_handler(MPIDI_am_handerler_fn fn);

/* built-in handlers */
/* TODO: add MPIDI_AM_COMPLETION_REQUEST */
#define MPIDI_AM_COMPLETION_NULL 0      /* NOOP */
#define MPIDI_AM_COMPLETION_TMPBUF 1    /* Free temporary buffer and call next completion id */

#define MPIDI_AM_COMPLETION_NUM_RESERVED 2

typedef struct _tmpbuf_context {
    void *tmpbuf;
    void *next_context;
    int next_completion_id;
} MPIDI_AM_COMPLETION_TMPBUF_CTX_t;

/* Device-layer interfaces */
void MPIDI_am_send_complete(int id, void *context);
void MPIDI_am_run_handler(int id, void *header, int payload_size, void *payload);

/* DEFAULT MACROS for MPIDI_NM_am_send_header and MPIDI_NM_am_send_noncontig */
#define MPIDI_AM_send_header_default(_PREFIX) \
MPL_STATIC_INLINE_PREFIX int _PREFIX ## _am_send_header(int grank, \
                                                        int handler_id, int completion_id, \
                                                        int header_size, void *header, \
                                                        void *context) \
{ \
    return _PREFIX ## _am_send_contig(grank, 0, handler_id, completion_id, \
                                      header_size, header, 0, NULL, context); \
}

#define MPIDI_AM_send_noncontig_default(_PREFIX) \
MPL_STATIC_INLINE_PREFIX int _PREFIX ## _am_send_noncontig(int grank, void *data, \
                                                           int count, MPI_Datatype datatype, \
                                                           int handler_id, int completion_id, \
                                                           int header_size, void *header, \
                                                           void *context) \
{ \
    int mpi_errno = MPI_SUCCESS; \
    /* get datatype info */ \
    int dt_contig; \
    MPI_Aint data_sz; \
    MPIR_Datatype *dt_ptr; \
    MPI_Aint dt_true_lb; \
    MPIDI_Datatype_get_info(count, datatype, dt_contig, data_sz, dt_ptr, dt_true_lb); \
    /* pack datatype */ \
    char *pack_buffer = (char *) MPL_malloc(data_sz, MPL_MEM_OTHER); \
    MPIR_ERR_CHKANDJUMP1(pack_buffer == NULL, mpi_errno, \
                         MPI_ERR_OTHER, "**nomem", "**nomem %s", "Send Pack buffer alloc"); \
    MPI_Aint actual_pack_bytes; \
    mpi_errno = MPIR_Typerep_pack(data, count, datatype, 0, pack_buffer, data_sz, \
                                  &actual_pack_bytes); \
    MPIR_ERR_CHECK(mpi_errno); \
    MPIR_Assert(actual_pack_bytes == data_sz); \
    /* call send_contig */ \
    MPIDI_AM_COMPLETION_TMPBUF_CTX_t * tmpbuf_ctx; \
    tmpbuf_ctx = MPL_malloc(sizeof(MPIDI_AM_COMPLETION_TMPBUF_CTX_t)); \
    tmpbuf_ctx->tmpbuf = pack_buffer; \
    tmpbuf_ctx->next_context = context; \
    tmpbuf_ctx->next_completion_id = completion_id; \
    mpi_errno = _PREFIX ## _am_send_contig(grank, 0, handler_id, MPIDI_AM_COMPLETION_TMPBUF, \
                                           header_size, header, data_sz, pack_buffer, \
                                           tmpbuf_ctx); \
  fn_exit: \
    return mpi_errno; \
  fn_fail: \
    return fn_exit; \
}

#endif /* CH4_AM_H_INCLUDED */
