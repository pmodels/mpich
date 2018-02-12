/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "mpii_bsend.h"
#include "bsendutil.h"

/*
 * Miscellaneous comments
 * By storing total_size along with "size available for messages", we
 * avoid any complexities associated with alignment, since we must
 * ensure that each KPIR_Bsend_data_t structure is properly aligned
 * (i.e., we can't simply do (sizeof(MPII_Bsend_data_t) + size) to get
 * total_size).
 *
 * Function Summary
 *   MPIR_Bsend_attach - Performs the work of MPI_Buffer_attach
 *   MPIR_Bsend_detach - Performs the work of MPI_Buffer_detach
 *   MPIR_Bsend_isend  - Essentially performs an MPI_Ibsend.  Returns
 *                an MPIR_Request that is also stored internally in the
 *                corresponding MPII_Bsend_data_t entry
 *   MPIR_Bsend_free_segment - Free a buffer that is no longer needed,
 *                merging with adjacent segments
 *   MPIR_Bsend_check_active - Check for completion of any pending sends
 *                for bsends (all bsends, both MPI_Ibsend and MPI_Bsend,
 *                are internally converted into Isends on the data
 *                in the Bsend buffer)
 *   MPIR_Bsend_retry_pending - Routine for future use to handle the
 *                case where an Isend cannot be initiated.
 *   MPIR_Bsend_find_buffer - Find a buffer in the bsend buffer large
 *                enough for the message.  However, does not acquire that
 *                buffer (see MPIR_Bsend_take_buffer)
 *   MPIR_Bsend_take_buffer - Find and acquire a buffer for a message
 *   MPIR_Bsend_finalize - Finalize handler when Bsend routines are used
 *   MPIR_Bsend_dump - Debugging routine to print the contents of the control
 *                information in the bsend buffer (the MPII_Bsend_data_t entries)
 */

#ifdef MPL_USE_DBG_LOGGING
static void MPIR_Bsend_dump(void);
#endif

#define BSENDDATA_HEADER_TRUE_SIZE (sizeof(MPII_Bsend_data_t) - sizeof(double))

/* BsendBuffer is the structure that describes the overall Bsend buffer */
/*
 * We use separate buffer and origbuffer because we may need to align
 * the buffer (we *could* always memcopy the header to an aligned region,
 * but it is simpler to just align it internally.  This does increase the
 * BSEND_OVERHEAD, but that is already relatively large.  We could instead
 * make sure that the initial header was set at an aligned location (
 * taking advantage of the "alignpad"), but this would require more changes.
 */
static struct BsendBuffer {
    void *buffer;               /* Pointer to the begining of the user-
                                 * provided buffer */
    size_t buffer_size;         /* Size of the user-provided buffer */
    void *origbuffer;           /* Pointer to the buffer provided by
                                 * the user */
    size_t origbuffer_size;     /* Size of the buffer as provided
                                 * by the user */
    MPII_Bsend_data_t *avail;   /* Pointer to the first available block
                                 * of space */
    MPII_Bsend_data_t *pending; /* Pointer to the first message that
                                 * could not be sent because of a
                                 * resource limit (e.g., no requests
                                 * available) */
    MPII_Bsend_data_t *active;  /* Pointer to the first active (sending)
                                 * message */
} BsendBuffer = {
0, 0, 0, 0, 0, 0, 0};

static int initialized = 0;     /* keep track of the first call to any
                                 * bsend routine */

/* Forward references */
static void MPIR_Bsend_retry_pending(void);
static int MPIR_Bsend_check_active(void);
static MPII_Bsend_data_t *MPIR_Bsend_find_buffer(size_t);
static void MPIR_Bsend_take_buffer(MPII_Bsend_data_t *, size_t);
static int MPIR_Bsend_finalize(void *);
static void MPIR_Bsend_free_segment(MPII_Bsend_data_t *);

/*
 * Attach a buffer.  This checks for the error conditions and then
 * initialized the avail buffer.
 */
#undef FUNCNAME
#define FUNCNAME MPIR_Bsend_attach
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Bsend_attach(void *buffer, int buffer_size)
{
    MPII_Bsend_data_t *p;
    size_t offset, align_sz;

#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            if (BsendBuffer.buffer) {
                return MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                                            "MPIR_Bsend_attach", __LINE__, MPI_ERR_BUFFER,
                                            "**bufexists", 0);
            }
            if (buffer_size < MPI_BSEND_OVERHEAD) {
                /* MPI_ERR_OTHER is another valid choice for this error,
                 * but the Intel test wants MPI_ERR_BUFFER, and it seems
                 * to violate the principle of least surprise to not use
                 * MPI_ERR_BUFFER for errors with the Buffer */
                return MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                                            "MPIR_Bsend_attach", __LINE__, MPI_ERR_BUFFER,
                                            "**bsendbufsmall",
                                            "**bsendbufsmall %d %d", buffer_size,
                                            MPI_BSEND_OVERHEAD);
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    if (!initialized) {
        initialized = 1;
        MPIR_Add_finalize(MPIR_Bsend_finalize, (void *) 0, 10);
    }

    BsendBuffer.origbuffer = buffer;
    BsendBuffer.origbuffer_size = buffer_size;
    BsendBuffer.buffer = buffer;
    BsendBuffer.buffer_size = buffer_size;

    /* Make sure that the buffer that we use is aligned to align_sz.  Some other
     * code assumes pointer-alignment, and some code assumes double alignment.
     * Further, GCC 4.5.1 generates bad code on 32-bit platforms when this is
     * only 4-byte aligned (see #1149). */
    align_sz = MPL_MAX(sizeof(void *), sizeof(double));
    offset = ((size_t) buffer) % align_sz;
    if (offset) {
        offset = align_sz - offset;
        buffer = (char *) buffer + offset;
        BsendBuffer.buffer = buffer;
        BsendBuffer.buffer_size -= offset;
    }
    BsendBuffer.avail = buffer;
    BsendBuffer.pending = 0;
    BsendBuffer.active = 0;

    /* Set the first block */
    p = (MPII_Bsend_data_t *) buffer;
    p->size = buffer_size - BSENDDATA_HEADER_TRUE_SIZE;
    p->total_size = buffer_size;
    p->next = p->prev = NULL;
    p->msg.msgbuf = (char *) p + BSENDDATA_HEADER_TRUE_SIZE;

    return MPI_SUCCESS;
}

/*
 * Detach a buffer.  This routine must wait until any pending bsends
 * are complete.  Note that MPI specifies the type of the returned "size"
 * argument as an "int" (the definition predates that of ssize_t as a
 * standard type).
 */
#undef FUNCNAME
#define FUNCNAME MPIR_Bsend_detach
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Bsend_detach(void *bufferp, int *size)
{
    int mpi_errno = MPI_SUCCESS;

    if (BsendBuffer.pending) {
        /* FIXME: Process pending bsend requests in detach */
        return MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                                    "MPIR_Bsend_detach", __LINE__, MPI_ERR_OTHER, "**bsendpending",
                                    0);
    }
    if (BsendBuffer.active) {
        /* Loop through each active element and wait on it */
        MPII_Bsend_data_t *p = BsendBuffer.active;

        while (p) {
            mpi_errno = MPIR_Request_wait_and_complete(p->request);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);
            p = p->next;
        }
    }

/* Note that this works even when the buffer does not exist */
    *(void **) bufferp = BsendBuffer.origbuffer;
    /* This cast to int will work because the user must use an int to describe
     * the buffer size */
    *size = (int) BsendBuffer.origbuffer_size;
    BsendBuffer.origbuffer = NULL;
    BsendBuffer.origbuffer_size = 0;
    BsendBuffer.buffer = 0;
    BsendBuffer.buffer_size = 0;
    BsendBuffer.avail = 0;
    BsendBuffer.active = 0;
    BsendBuffer.pending = 0;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/*
 * Initiate an ibsend.  We'll used this for Bsend as well.
 */
#undef FUNCNAME
#define FUNCNAME MPIR_Bsend_isend
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Bsend_isend(const void *buf, int count, MPI_Datatype dtype,
                     int dest, int tag, MPIR_Comm * comm_ptr,
                     MPII_Bsend_kind_t kind, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPII_Bsend_data_t *p;
    MPII_Bsend_msg_t *msg;
    MPI_Aint packsize;
    int pass;

    /* Find a free segment and copy the data into it.  If we could
     * have, we would already have used tBsend to send the message with
     * no copying.
     *
     * We may want to decide here whether we need to pack at all
     * or if we can just use (a MPIR_Memcpy) of the buffer.
     */


    /* We check the active buffer first.  This helps avoid storage
     * fragmentation */
    mpi_errno = MPIR_Bsend_check_active();
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    if (dtype != MPI_PACKED)
        MPIR_Pack_size_impl(count, dtype, &packsize);
    else
        packsize = count;

    MPL_DBG_MSG_D(MPIR_DBG_BSEND, TYPICAL, "looking for buffer of size " MPI_AINT_FMT_DEC_SPEC,
                  packsize);
    /*
     * Use two passes.  Each pass is the same; between the two passes,
     * attempt to complete any active requests, and start any pending
     * ones.  If the message can be initiated in the first pass,
     * do not perform the second pass.
     */
    for (pass = 0; pass < 2; pass++) {

        p = MPIR_Bsend_find_buffer(packsize);
        if (p) {
            MPL_DBG_MSG_FMT(MPIR_DBG_BSEND, TYPICAL, (MPL_DBG_FDEST,
                                                      "found buffer of size " MPI_AINT_FMT_DEC_SPEC
                                                      " with address %p", packsize, p));
            /* Found a segment */

            msg = &p->msg;

            /* Pack the data into the buffer */
            /* We may want to optimize for the special case of
             * either primative or contiguous types, and just
             * use MPIR_Memcpy and the provided datatype */
            msg->count = 0;
            if (dtype != MPI_PACKED) {
                mpi_errno =
                    MPIR_Pack_impl(buf, count, dtype, p->msg.msgbuf, packsize, &p->msg.count);
                if (mpi_errno)
                    MPIR_ERR_POP(mpi_errno);
            } else {
                MPIR_Memcpy(p->msg.msgbuf, buf, count);
                p->msg.count = count;
            }
            /* Try to send the message.  We must use MPID_Isend
             * because this call must not block */
            mpi_errno = MPID_Isend(msg->msgbuf, msg->count, MPI_PACKED,
                                   dest, tag, comm_ptr, MPIR_CONTEXT_INTRA_PT2PT, &p->request);
            MPIR_ERR_CHKINTERNAL(mpi_errno, mpi_errno, "Bsend internal error: isend returned err");
            /* If the error is "request not available", we should
             * put this on the pending list.  This will depend on
             * how we signal failure to send. */

            if (p->request) {
                MPL_DBG_MSG_FMT(MPIR_DBG_BSEND, TYPICAL,
                                (MPL_DBG_FDEST, "saving request %p in %p", p->request, p));
                /* An optimization is to check to see if the
                 * data has already been sent.  The original code
                 * to do this was commented out and probably did not match
                 * the current request internals */
                MPIR_Bsend_take_buffer(p, p->msg.count);
                p->kind = kind;
                *request = p->request;
            }
            break;
        }
        /* If we found a buffer or we're in the seccond pass, then break.
         * Note that the test on phere is redundant, as the code breaks
         * out of the loop in the test above if a block p is found. */
        if (p || pass == 1)
            break;
        MPL_DBG_MSG(MPIR_DBG_BSEND, TYPICAL, "Could not find storage, checking active");
        /* Try to complete some pending bsends */
        MPIR_Bsend_check_active();
        /* Give priority to any pending operations */
        MPIR_Bsend_retry_pending();
    }

    if (!p) {
        /* Return error for no buffer space found */
        /* Generate a traceback of the allocated space, explaining why
         * packsize could not be found */
        MPL_DBG_MSG(MPIR_DBG_BSEND, TYPICAL, "Could not find space; dumping arena");
        MPL_DBG_STMT(MPIR_DBG_BSEND, TYPICAL, MPIR_Bsend_dump());
        MPIR_ERR_SETANDJUMP2(mpi_errno, MPI_ERR_BUFFER, "**bufbsend", "**bufbsend %d %d", packsize,
                             BsendBuffer.buffer_size);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/*
 * The following routine looks up the segment used by request req
 * and frees it. The request is assumed to be completed. This routine
 * is called by only MPIR_Ibsend_free.
 */
#undef FUNCNAME
#define FUNCNAME MPIR_Bsend_free_seg
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Bsend_free_req_seg(MPIR_Request * req)
{
    int mpi_errno = MPI_ERR_INTERN;
    MPII_Bsend_data_t *active = BsendBuffer.active;

    MPL_DBG_MSG_P(MPIR_DBG_BSEND, TYPICAL, "Checking active starting at %p", active);
    while (active) {

        if (active->request == req) {
            MPIR_Bsend_free_segment(active);
            mpi_errno = MPI_SUCCESS;
        }

        active = active->next;;

        MPL_DBG_MSG_P(MPIR_DBG_BSEND, TYPICAL, "Next active is %p", active);
    }

    return mpi_errno;
}

/*
 * The following routines are used to manage the allocation of bsend segments
 * in the user buffer.  These routines handle, for example, merging segments
 * when an active segment that is adjacent to a free segment becomes free.
 *
 */

/* Add block p to the free list. Merge into adjacent blocks.  Used only
   within the check_active */

#undef FUNCNAME
#define FUNCNAME MPIR_Bsend_free_segment
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static void MPIR_Bsend_free_segment(MPII_Bsend_data_t * p)
{
    MPII_Bsend_data_t *prev = p->prev, *avail = BsendBuffer.avail, *avail_prev;

    MPL_DBG_MSG_FMT(MPIR_DBG_BSEND, TYPICAL, (MPL_DBG_FDEST,
                                              "Freeing bsend segment at %p of size %llu, next at %p",
                                              p, (unsigned long long) p->size,
                                              ((char *) p) + p->total_size));

    MPL_DBG_MSG_D(MPIR_DBG_BSEND, TYPICAL,
                  "At the begining of free_segment with size %llu:",
                  (unsigned long long) p->total_size);
    MPL_DBG_STMT(MPIR_DBG_BSEND, TYPICAL, MPIR_Bsend_dump());

    /* Remove the segment from the active list */
    if (prev) {
        MPL_DBG_MSG(MPIR_DBG_BSEND, TYPICAL, "free segment is within active list");
        prev->next = p->next;
    } else {
        /* p was at the head of the active list */
        MPL_DBG_MSG(MPIR_DBG_BSEND, TYPICAL, "free segment is head of active list");
        BsendBuffer.active = p->next;
        /* The next test sets the prev pointer to null */
    }
    if (p->next) {
        p->next->prev = prev;
    }

    MPL_DBG_STMT(MPIR_DBG_BSEND, VERBOSE, MPIR_Bsend_dump());

    /* Merge into the avail list */
    /* Find avail_prev, avail, such that p is between them.
     * either may be null if p is at either end of the list */
    avail_prev = 0;
    while (avail) {
        if (avail > p) {
            break;
        }
        avail_prev = avail;
        avail = avail->next;
    }

    /* Try to merge p with the next block */
    if (avail) {
        if ((char *) p + p->total_size == (char *) avail) {
            p->total_size += avail->total_size;
            p->size = p->total_size - BSENDDATA_HEADER_TRUE_SIZE;
            p->next = avail->next;
            if (avail->next)
                avail->next->prev = p;
            avail = 0;
        } else {
            p->next = avail;
            avail->prev = p;
        }
    } else {
        p->next = 0;
    }
    /* Try to merge p with the previous block */
    if (avail_prev) {
        if ((char *) avail_prev + avail_prev->total_size == (char *) p) {
            avail_prev->total_size += p->total_size;
            avail_prev->size = avail_prev->total_size - BSENDDATA_HEADER_TRUE_SIZE;
            avail_prev->next = p->next;
            if (p->next)
                p->next->prev = avail_prev;
        } else {
            avail_prev->next = p;
            p->prev = avail_prev;
        }
    } else {
        /* p is the new head of the list */
        BsendBuffer.avail = p;
        p->prev = 0;
    }

    MPL_DBG_MSG(MPIR_DBG_BSEND, TYPICAL, "At the end of free_segment:");
    MPL_DBG_STMT(MPIR_DBG_BSEND, TYPICAL, MPIR_Bsend_dump());
}

/*
 * The following routine tests for completion of active sends and
 * frees the related storage
 *
 * To make it easier to identify the source of the request, we keep
 * track of the type of MPI routine (ibsend, bsend, or bsend_init/start)
 * that created the bsend entry.
 */
#undef FUNCNAME
#define FUNCNAME MPIR_Bsend_check_active
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int MPIR_Bsend_check_active(void)
{
    int mpi_errno = MPI_SUCCESS;
    int active_flag;
    MPII_Bsend_data_t *active = BsendBuffer.active, *next_active;

    MPL_DBG_MSG_P(MPIR_DBG_BSEND, TYPICAL, "Checking active starting at %p", active);
    while (active) {
        int flag;

        next_active = active->next;

        if (active->kind == IBSEND) {
            /* We handle ibsend specially to allow for the user
             * to attempt and cancel the request. Also, to allow
             * for a cancel attempt (which must be attempted before
             * a successful test or wait), we only start
             * testing when the user has successfully released
             * the request (it is a grequest, the free call will do it) */
            flag = 0;
            /* XXX DJG FIXME-MT should we be checking this? */
            if (MPIR_Object_get_ref(active->request) == 1) {
                mpi_errno = MPID_Test(active->request, &flag, MPI_STATUS_IGNORE);
                if (mpi_errno)
                    MPIR_ERR_POP(mpi_errno);
            } else {
                /* We need to invoke the progress engine in case we
                 * need to advance other, incomplete communication.  */
                MPID_Progress_state progress_state;
                MPID_Progress_start(&progress_state);
                mpi_errno = MPID_Progress_test();
                MPID_Progress_end(&progress_state);
                if (mpi_errno)
                    MPIR_ERR_POP(mpi_errno);
            }
        } else {
            mpi_errno = MPID_Test(active->request, &flag, MPI_STATUS_IGNORE);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);
        }
        if (flag) {
            mpi_errno =
                MPIR_Request_completion_processing(active->request, MPI_STATUS_IGNORE,
                                                   &active_flag);
            MPIR_Request_free(active->request);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);
            /* We're done.  Remove this segment */
            MPL_DBG_MSG_P(MPIR_DBG_BSEND, TYPICAL, "Removing segment %p", active);
            MPIR_Bsend_free_segment(active);
        }
        active = next_active;
        MPL_DBG_MSG_P(MPIR_DBG_BSEND, TYPICAL, "Next active is %p", active);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/*
 * FIXME : For each pending item (that is, items that we couldn't even start
 * sending), try to get them going.
 */
static void MPIR_Bsend_retry_pending(void)
{
    MPII_Bsend_data_t *pending = BsendBuffer.pending, *next_pending;

    while (pending) {
        next_pending = pending->next;
        /* Retry sending this item */
        /* FIXME: Unimplemented retry of pending bsend operations */
        pending = next_pending;
    }
}

/*
 * Find a slot in the avail buffer that can hold size bytes.  Does *not*
 * remove the slot from the avail buffer (see MPIR_Bsend_take_buffer)
 */
static MPII_Bsend_data_t *MPIR_Bsend_find_buffer(size_t size)
{
    MPII_Bsend_data_t *p = BsendBuffer.avail;

    while (p) {
        if (p->size >= size) {
            return p;
        }
        p = p->next;
    }
    return 0;
}

/* This is the minimum number of bytes that a segment must be able to
   hold. */
#define MIN_BUFFER_BLOCK 8
/*
 * Carve off size bytes from buffer p and leave the remainder
 * on the avail list.  Handle the head/tail cases.
 * If there isn't enough left of p, remove the entire segment from
 * the avail list.
 */
static void MPIR_Bsend_take_buffer(MPII_Bsend_data_t * p, size_t size)
{
    MPII_Bsend_data_t *prev;
    size_t alloc_size;

    /* Compute the remaining size.  This must include any padding
     * that must be added to make the new block properly aligned */
    alloc_size = size;
    if (alloc_size & 0x7)
        alloc_size += (8 - (alloc_size & 0x7));
    /* alloc_size is the amount of space (out of size) that we will
     * allocate for this buffer. */

    MPL_DBG_MSG_FMT(MPIR_DBG_BSEND, TYPICAL, (MPL_DBG_FDEST,
                                              "Taking %lu bytes from a block with %llu bytes\n",
                                              alloc_size, (unsigned long long) p->total_size));

    /* Is there enough space left to create a new block? */
    if (alloc_size + BSENDDATA_HEADER_TRUE_SIZE + MIN_BUFFER_BLOCK <= p->size) {
        /* Yes, the available space (p->size) is large enough to
         * carve out a new block */
        MPII_Bsend_data_t *newp;

        MPL_DBG_MSG_P(MPIR_DBG_BSEND, TYPICAL, "Breaking block into used and allocated at %p", p);
        newp = (MPII_Bsend_data_t *) ((char *) p + BSENDDATA_HEADER_TRUE_SIZE + alloc_size);
        newp->total_size = p->total_size - alloc_size - BSENDDATA_HEADER_TRUE_SIZE;
        newp->size = newp->total_size - BSENDDATA_HEADER_TRUE_SIZE;
        newp->msg.msgbuf = (char *) newp + BSENDDATA_HEADER_TRUE_SIZE;

        /* Insert this new block after p (we'll remove p from the avail list
         * next) */
        newp->next = p->next;
        newp->prev = p;
        if (p->next) {
            p->next->prev = newp;
        }
        p->next = newp;
        p->total_size = (char *) newp - (char *) p;
        p->size = p->total_size - BSENDDATA_HEADER_TRUE_SIZE;

        MPL_DBG_MSG_FMT(MPIR_DBG_BSEND, TYPICAL, (MPL_DBG_FDEST,
                                                  "broken blocks p (%llu) and new (%llu)\n",
                                                  (unsigned long long) p->total_size,
                                                  (unsigned long long) newp->total_size));
    }

    /* Remove p from the avail list and add it to the active list */
    prev = p->prev;
    if (prev) {
        prev->next = p->next;
    } else {
        BsendBuffer.avail = p->next;
    }

    if (p->next) {
        p->next->prev = p->prev;
    }

    if (BsendBuffer.active) {
        BsendBuffer.active->prev = p;
    }
    p->next = BsendBuffer.active;
    p->prev = 0;
    BsendBuffer.active = p;

    MPL_DBG_MSG_P(MPIR_DBG_BSEND, VERBOSE, "segment %p now head of active", p);
    MPL_DBG_MSG(MPIR_DBG_BSEND, TYPICAL, "At end of take buffer");
    MPL_DBG_STMT(MPIR_DBG_BSEND, TYPICAL, MPIR_Bsend_dump());
}

static int MPIR_Bsend_finalize(void *p ATTRIBUTE((unused)))
{
    void *b;
    int s;

    MPL_UNREFERENCED_ARG(p);

    if (BsendBuffer.buffer) {
        /* Use detach to complete any communication */
        MPIR_Bsend_detach(&b, &s);
    }
    return 0;
}

/*
 * These routines are defined only if debug logging is enabled
 */
#ifdef MPL_USE_DBG_LOGGING
static void MPIR_Bsend_dump(void)
{
    MPII_Bsend_data_t *a = BsendBuffer.avail;

    MPL_DBG_MSG_D(MPIR_DBG_BSEND, TYPICAL, "Total size is %llu",
                  (unsigned long long) BsendBuffer.buffer_size);
    MPL_DBG_MSG(MPIR_DBG_BSEND, TYPICAL, "Avail list is:");
    while (a) {
        MPL_DBG_MSG_FMT(MPIR_DBG_BSEND, TYPICAL, (MPL_DBG_FDEST, "[%p] totalsize = %llu(%llx)",
                                                  a, (unsigned long long) a->total_size,
                                                  (unsigned long long) a->total_size));
        if (a == a->next) {
            MPL_DBG_MSG(MPIR_DBG_BSEND, TYPICAL, "@@@Corrupt list; avail block points at itself");
            break;
        }
        a = a->next;
    }

    MPL_DBG_MSG(MPIR_DBG_BSEND, TYPICAL, "Active list is:");
    a = BsendBuffer.active;
    while (a) {
        MPL_DBG_MSG_FMT(MPIR_DBG_BSEND, TYPICAL, (MPL_DBG_FDEST, "[%p] totalsize = %llu(%llx)",
                                                  a, (unsigned long long) a->total_size,
                                                  (unsigned long long) a->total_size));
        if (a == a->next) {
            MPL_DBG_MSG(MPIR_DBG_BSEND, TYPICAL, "@@@Corrupt list; active block points at itself");
            break;
        }
        a = a->next;
    }
    MPL_DBG_MSG(MPIR_DBG_BSEND, TYPICAL, "end of list");
}
#endif
