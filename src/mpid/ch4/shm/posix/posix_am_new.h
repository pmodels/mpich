/*
 *  (C) 2019 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef POSIX_AM_NEW_H_INCLUDED
#define POSIX_AM_NEW_H_INCLUDED

struct posix_new_am_header {
    uint8_t handler_id;
    uint8_t header_size;
};

/* Until old am usages are fully replaced, we need have both path working.
 * Use a static msg_hdr to hack. The new am path does not use msg_hdr.
 */
static MPIDI_POSIX_am_header_t posix_new_am_msg_hdr = {
    .handler_id = MPIDIG_NEW_AM
};

/* Until old am usages are fully replaced, we need have both path working.
 * Use a static msg_hdr to hack. The new am path does not use msg_hdr.
 */
static MPIDI_POSIX_am_header_t new_am_msg_hdr = {
    .handler_id = MPIDIG_NEW_AM
};

MPL_STATIC_INLINE_PREFIX int NEW_POSIX_am_enqueue(int grank, struct iovec *iov, int iov_count,
                                                  int completion_id, void *context);

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

    struct posix_new_am_header *p_hdr = header;
    p_hdr->handler_id = handler_id;
    p_hdr->header_size = header_size;

    struct iovec iov[2];
    int iov_count = 2;
    iov[0].iov_base = header;
    iov[0].iov_len = header_size;
    iov[1].iov_base = payload;
    iov[1].iov_len = payload_size;

    struct iovec *iov_ptr;
    iov_ptr = iov;

    if (payload_size == 0) {
        iov_count = 1;
    }

    /* If we already have messages in the postponed queue, this one will probably also end up being
     * queued so save some cycles and do it now. */
    if (unlikely(MPIDI_POSIX_global.postponed_queue)) {
        mpi_errno = NEW_POSIX_am_enqueue(grank, iov, iov_count, completion_id, context);
        MPIR_ERR_CHECK(mpi_errno);
        goto fn_exit;
    }

    int result = MPIDI_POSIX_OK;
    result = MPIDI_POSIX_eager_send(grank, &posix_new_am_msg_hdr, &iov, &iov_count);

    /* If the message was not completed, queue it to be sent later. */
    if (unlikely((MPIDI_POSIX_NOK == result) || iov_num_left)) {
        mpi_errno = NEW_POSIX_am_enqueue(grank, iov, iov_count, completion_id, context);
        MPIR_ERR_CHECK(mpi_errno);
        goto fn_exit;
    }

    /* If we made it here, the request has been completed and we can clean up the tracking
     * information and trigger the appropriate callbacks. */
    mpi_errno = MPIDI_am_send_complete(completion_id, context);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* Define MPIDI_POSIX_am_send_header and MPIDI_POSIX_am_send_noncontig */
MPIDI_AM_send_header_default(MPIDI_POSIX)
    MPIDI_AM_send_noncontig_default(MPIDI_POSIX)

/* internal functions */
struct am_queue_elem {
    int grank;
    struct iovec *iov;
    int iov_count;
    int completion_id;
    void *context;
    struct am_queue_elem *next;
};

/* TODO: add MPIDI_POSIX_global.am_send_queue */
MPL_STATIC_INLINE_PREFIX int NEW_POSIX_am_enqueue(int grank,
                                                  struct iovec *iov, int iov_count,
                                                  int completion_id, void *context)
{
    struct am_queue_elem *elem = malloc(sizeof(struct am_queue_elem));
    elem->grank = grank;
    elem->iov = iov;
    elem->iov_count = iov_count;
    elem->completion_id = completion_id;
    elem->context = context;
    DL_APPEND(MPIDI_POSIX_global.am_send_queue, elem);

    return MPI_SUCCESS;
}

/* NOTE: called from posix/progress.c:progress_send */
MPL_STATIC_INLINE_PREFIX void progress_new_am_send(void)
{
    if (MPIDI_POSIX_global.am_send_queue) {
        struct am_queue_elem *elem = MPIDI_POSIX_global.am_send_queue;
        int result = MPIDI_POSIX_eager_send(elem->grank, &posix_new_am_msg_hdr,
                                            &elem->iov, &elem->iov_count);
        if ((MPIDI_POSIX_NOK == result) || elem->iov_num) {
            return;
        }
        DL_DELETE(MPIDI_POSIX_global.am_send_queue, elem);

        if (elem->completion_id) {
            MPIDI_am_send_complete(elem->completion_id, elem->context);
        }
    }
}

/* NOTE: called from posix/progress.c:progress_recv */
MPL_STATIC_INLINE_PREFIX void progress_new_am_recv(void *data, int size)
{
    struct posix_new_am_header *p_hdr = data;
    void *header = data;
    void *payload = (char *) data + p_hdr->header_size;
    int payload_size = size - p_hdr->header_size;

    /* potentially, switch handler_id and call handler inline */
    MPIDI_am_run_handler(p_hdr->handler_id, header, payload_size, payload);
}

#endif /* POSIX_AM_NEW_H_INCLUDED */
