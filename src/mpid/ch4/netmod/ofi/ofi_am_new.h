/*
 *  (C) 2019 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef OFI_AM_NEW_H_INCLUDED
#define OFI_AM_NEW_H_INCLUDED

struct ofi_new_am_header {
    uint8_t handler_id;
    uint8_t header_size;
};

struct ofi_new_am_ctx {
    int zero;                   /* hack: use zero to distinguish from normal context (request object) */
    int completion_id;
    void *context;
};

/* Until old am usages are fully replaced, we need have both path working.
 * Use a static msg_hdr to hack. The new am path does not use msg_hdr.
 */
static MPIDI_OFI_am_header_t ofi_new_am_msg_hdr = {
    .handler_id = MPIDIG_NEW_AM
};

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_am_send_contig(int grank, int payload_type,
                                                        int handler_id, int completion_id,
                                                        int header_size, void *header,
                                                        int payload_size, void *payload,
                                                        void *context)
{
    int mpi_errno = MPI_SUCCESS;

    /* TODO: support payload_type, e.g. GPU payload */

    MPIR_Assert(sizeof(struct posix_new_am_header) <= MPIDI_AM_PADDING_SIZE);
    MPIR_Assert(handler_id < 256);
    MPIR_Assert(header_size < 256);
    /* FIXME: assert payload_size within eager limit */

    struct ofi_new_am_header *p_hdr = header;
    p_hdr->handler_id = handler_id;
    p_hdr->header_size = header_size;

    fi_addr_t addr = MPIDI_OFI_comm_to_phys(MPIR_Process.comm_world, grank);
    int buff_len = sizeof(msg_hdr) + header_size + payload_size;
    if (buff_len <= MPIDI_OFI_global.max_buffered_send) {
        char *buff = MPL_malloc(buff_len, MPL_MEM_OTHER);
        memcpy(buff, &msg_hdr, sizeof(msg_hdr));
        memcpy(buff + sizeof(msg_hdr), header, header_size);
        memcpy(buff + sizeof(msg_hdr) + header_size, payload, payload_size);

        MPIDI_OFI_CALL_RETRY_AM(fi_inject(MPIDI_OFI_global.ctx[0].tx, buff, buff_len, addr),
                                inject);
        MPL_free(buff);
        mpi_errno = MPIDI_am_send_complete(completion_id, context);
        MPIR_ERR_CHECK(mpi_errno);
    } else {
        struct iovec iov[3];
        int iov_count = 3;
        iov[0].iov_base = &ofi_new_am_msg_hdr;
        iov[0].iov_len = sizeof(MPIDI_OFI_am_header_t);
        iov[1].iov_base = header;
        iov[1].iov_len = header_size;
        iov[2].iov_base = payload;
        iov[2].iov_len = payload_size;

        struct ofi_new_am_ctx *p_ctx = MPL_malloc(sizeof(struct ofi_new_am_ctx));
        p_ctx->zero = 0;
        p_ctx->completion_id = completion_id;
        p_ctx->context = context;

        MPIDI_OFI_ASSERT_IOVEC_ALIGN(iov);
        MPIDI_OFI_CALL_RETRY_AM(fi_sendv(MPIDI_OFI_global.ctx[0].tx, iov, NULL, 3, addr, p_ctx),
                                sendv);
    }
  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* NOTE: called from ofi/ofi_events.c:MPIDI_OFI_dispatch_function */
MPL_STATIC_INLINE_PREFIX void progress_new_am_send_complete(void *ctx)
{
    struct ofi_new_am_ctx *p_ctx = ctx;
    MPIDI_am_send_complete(p_ctx->completion_id, p_ctx->context);
    MPL_free(p_ctx);
}

/* NOTE: called from ofi/ofi_am_impl.h:am_recv_event */
MPL_STATIC_INLINE_PREFIX void ofi_new_am_recv(void *data, int size)
{
    void *header = (char *) data + sizeof(MPIDI_OFI_am_header_t);
    struct posix_new_am_header *p_hdr = header;
    void *payload = (char *) header + p_hdr->header_size;
    int payload_size = size - sizeof(MPIDI_OFI_am_header_t) - p_hdr->header_size;

    /* potentially, switch handler_id and call handler inline */
    MPIDI_am_run_handler(p_hdr->handler_id, header, payload_size, payload);
}
