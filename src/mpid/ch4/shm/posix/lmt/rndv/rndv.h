/*
 *  (C) 2019 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2019 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_CH4_POSIX_LMT_PIPELINE_THRESHOLD
      category    : CH4
      type        : int
      default     : 131072
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        If the message is smaller than this threshold, copy the entire message into the pipeline
        copy buffer at once instead of pipelining. If it is larger than this message, copy it into
        the copy buffer using the size of each copy buffer segment as the chunk size (32KB is
        defined in rndv.h).

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

#include "mpiimpl.h"
#include "mpidu_generic_queue.h"
#include "posix_lmt.h"
#include "posix_am.h"

/* Describes ongoing operations so the progress engine can move them along. */
typedef struct lmt_rndv_wait_element {
    /* The function that needs to be called to progress this wait element. Probably will be a send
     * function or a receive function. */
    int (*progress) (MPIR_Request * req, int local_rank, bool * done);

    /* The request that describes the operation and has all of the information needed to make
     * progress on it. */
    MPIR_Request *req;

    /* Pointer to the next element in the list. */
    struct lmt_rndv_wait_element *next;
} lmt_rndv_wait_element_t;

typedef struct {
    lmt_rndv_wait_element_t *head, *tail;
} lmt_rndv_queue_t;

/* Ensures that the size of this int will be one cache line. */
typedef union {
    volatile int val;
    char padding[MPIDU_SHM_CACHE_LINE_LEN];
} lmt_rndv_cacheline_int_t;

#define MPIDI_LMT_NUM_BUFS 8
#define MPIDI_LMT_COPY_BUF_LEN (32 * 1024)
/* For smaller messages, use a smaller pipeline size to make the memcopies faster and get more
 * overlap. */
#define MPIDI_LMT_SMALL_PIPELINE_MAX (MPIDI_LMT_COPY_BUF_LEN / 2)

/* Struct to describe the metadata needed to perform a pipelined copy between two processes. */
typedef struct {
    /* Set if the sender is currently in the progress function for this buffer. */
    lmt_rndv_cacheline_int_t sender_present;

    /* Set if the receiver is currently in the progress function for this buffer. */
    lmt_rndv_cacheline_int_t receiver_present;

    /* The length of the data in each of the buffer chunks */
    lmt_rndv_cacheline_int_t len[MPIDI_LMT_NUM_BUFS];

    /* This buffer is used when there is some data that couldn't be copied out of the previous
     * buffer. */
    volatile char underflow_buf[MPIDU_SHM_CACHE_LINE_LEN];

    /* The buffers used for copying the data through the shared memory region. The are partitioned
     * as a series of copy buffers. */
    volatile char buf[MPIDI_LMT_NUM_BUFS][MPIDI_LMT_COPY_BUF_LEN];
} lmt_rndv_copy_buf_t;

/* An array of copy buffer pointers to be filled in on demand. */
extern lmt_rndv_copy_buf_t **rndv_send_copy_bufs;
extern lmt_rndv_copy_buf_t **rndv_recv_copy_bufs;
/* Handles for the copy buffers that can be serialized to send to the communication partner. */
extern MPL_shm_hnd_t *rndv_send_copy_buf_handles;
extern MPL_shm_hnd_t *rndv_recv_copy_buf_handles;
/* Array of queued operations between the current process and all other local processes. */
extern lmt_rndv_queue_t *rndv_send_queues;
extern lmt_rndv_queue_t *rndv_recv_queues;

int lmt_rndv_recv_progress(MPIR_Request * req, int local_rank, bool * done);
int lmt_rndv_send_progress(MPIR_Request * req, int local_rank, bool * done);
