/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPID_NEM_INLINE_H_INCLUDED
#define MPID_NEM_INLINE_H_INCLUDED

#include "my_papi_defs.h"
#include "mpl.h"
#include "mpidi_nem_statistics.h"
#include "mpit.h"

extern int MPID_nem_lmt_shm_pending;
extern MPID_nem_cell_ptr_t MPID_nem_prefetched_cell;

static inline int MPID_nem_mpich_send_header (void* buf, int size, MPIDI_VC_t *vc, int *again);
static inline int MPID_nem_mpich_sendv (struct iovec **iov, int *n_iov, MPIDI_VC_t *vc, int *again);
static inline void MPID_nem_mpich_dequeue_fastbox (int local_rank);
static inline void MPID_nem_mpich_enqueue_fastbox (int local_rank);
static inline int MPID_nem_mpich_sendv_header (struct iovec **iov, int *n_iov, MPIDI_VC_t *vc, int *again);
static inline int MPID_nem_recv_seqno_matches (MPID_nem_queue_ptr_t qhead);
static inline int MPID_nem_mpich_test_recv (MPID_nem_cell_ptr_t *cell, int *in_fbox, int in_blocking_progress);
static inline int MPID_nem_mpich_blocking_recv (MPID_nem_cell_ptr_t *cell, int *in_fbox, int completions);
static inline int MPID_nem_mpich_test_recv_wait (MPID_nem_cell_ptr_t *cell, int *in_fbox, int timeout);
static inline int MPID_nem_mpich_release_cell (MPID_nem_cell_ptr_t cell, MPIDI_VC_t *vc);
static inline void MPID_nem_mpich_send_seg_header (void *buf, MPI_Aint count, MPI_Datatype datatype,
                                                   intptr_t *msg_offset,
                                                   intptr_t msgsize, void *header, intptr_t header_sz,
                                                   MPIDI_VC_t *vc, int *again);
static inline void MPID_nem_mpich_send_seg (void *buf, MPI_Aint count, MPI_Datatype datatype,
                                            intptr_t *msg_offset, intptr_t msgsize,
                                                    MPIDI_VC_t *vc, int *again);


/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_POLLS_BEFORE_YIELD
      category    : NEMESIS
      type        : int
      default     : 1000
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        When MPICH is in a busy waiting loop, it will periodically
        call a function to yield the processor.  This cvar sets
        the number of loops before the yield function is called.  A
        value of 0 disables yielding.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

/* -------------------------------------------------------------------------- */

/*
 * MPIU_Busy_wait()
 *
 * Call this in every busy wait loop to periodically yield the processor.  The
 * MPIR_CVAR_POLLS_BEFORE_YIELD parameter can be used to adjust the number of
 * times MPIU_Busy_wait is called before the yield function is called.
 */
#ifdef USE_NOTHING_FOR_YIELD
/* optimize if we're not yielding the processor */
#define MPIU_Busy_wait() do {} while (0)
#else
/* MT: Updating the static int poll_count variable isn't thread safe and will
   need to be changed for fine-grained multithreading.  A possible alternative
   is to make it a global thread-local variable. */
#define MPIU_Busy_wait() do {                                   \
        if (MPIR_CVAR_POLLS_BEFORE_YIELD) {                    \
            static int poll_count_ = 0;                         \
            if (poll_count_ >= MPIR_CVAR_POLLS_BEFORE_YIELD) { \
                poll_count_ = 0;                                \
                MPL_sched_yield();                              \
            } else {                                            \
                ++poll_count_;                                  \
            }                                                   \
        }                                                       \
    } while (0)
#endif

/* evaluates to TRUE if it is safe to block on recv operations in the progress
 * loop, FALSE otherwise */
#define MPID_nem_safe_to_block_recv()           \
    (!MPID_nem_local_lmt_pending &&             \
     !MPIDI_CH3I_shm_active_send &&             \
     !MPIDI_CH3I_Sendq_head(MPIDI_CH3I_shm_sendq) &&       \
     !MPIDU_Sched_are_pending() &&              \
     MPIR_Coll_safe_to_block() &&  \
     !MPIDI_RMA_Win_active_list_head)

static inline int
MPID_nem_mpich_send_header (void* buf, int size, MPIDI_VC_t *vc, int *again)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_nem_cell_ptr_t el;
    int my_rank;
    MPIDI_CH3I_VC *vc_ch = &vc->ch;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_NEM_MPICH_SEND_HEADER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_NEM_MPICH_SEND_HEADER);

    /*DO_PAPI (PAPI_reset (PAPI_EventSet)); */

    MPIR_Assert (size == sizeof(MPIDI_CH3_Pkt_t));
    MPIR_Assert (vc_ch->is_local);

    my_rank = MPID_nem_mem_region.rank;

#ifdef USE_FASTBOX
    {
	MPID_nem_fastbox_t *pbox = vc_ch->fbox_out;
        MPID_nem_cell_t *cell_ptr = MPID_NEM_FBOX_TO_CELL(pbox);

        /* _is_full contains acquire barrier */
        if (MPID_nem_fbox_is_full(pbox))
            goto usequeue_l;

        cell_ptr->header.source  = MPID_nem_mem_region.local_rank;
        cell_ptr->header.datalen = size;
        cell_ptr->header.seqno   = vc_ch->send_seqno++;
        
        MPL_DBG_STMT (MPIDI_CH3_DBG_CHANNEL, VERBOSE, cell_ptr->header.type = MPID_NEM_PKT_MPICH_HEAD);
        
        MPIR_Memcpy((void *)cell_ptr->payload, buf, size);

        MPL_atomic_release_store_int(&pbox->flag, 1);

        MPL_DBG_MSG (MPIDI_CH3_DBG_CHANNEL, VERBOSE, "--> Sent fbox ");
        MPL_DBG_STMT (MPIDI_CH3_DBG_CHANNEL, VERBOSE, MPID_nem_dbg_dump_cell (cell_ptr));
        
        goto return_success;
    }
 usequeue_l:
    MPIR_T_PVAR_COUNTER_INC_VAR(NEM, &MPID_nem_fbox_fall_back_to_queue_count[MPID_nem_mem_region.local_ranks[vc->lpid]], 1);

#endif /*USE_FASTBOX */

#ifdef PREFETCH_CELL
    DO_PAPI (PAPI_reset (PAPI_EventSet));
    el = MPID_nem_prefetched_cell;

    if (!el)
    {
	if (MPID_nem_queue_empty (MPID_nem_mem_region.my_freeQ))
	    goto return_again;
	
	MPID_nem_queue_dequeue (MPID_nem_mem_region.my_freeQ, &el);
    }
    DO_PAPI (PAPI_accum_var (PAPI_EventSet, PAPI_vvalues14));
#else /* PREFETCH_CELL */
    DO_PAPI (PAPI_reset (PAPI_EventSet));
    if (MPID_nem_queue_empty (MPID_nem_mem_region.my_freeQ))
    {
	goto return_again;
    }
    DO_PAPI (PAPI_accum_var (PAPI_EventSet, PAPI_vvalues14));

    DO_PAPI (PAPI_reset (PAPI_EventSet));
    MPID_nem_queue_dequeue (MPID_nem_mem_region.my_freeQ , &el);
    DO_PAPI (PAPI_accum_var (PAPI_EventSet, PAPI_vvalues10));
#endif /* PREFETCH_CELL */

    DO_PAPI (PAPI_reset (PAPI_EventSet));
    el->header.source  = my_rank;
    el->header.dest    = vc->lpid;
    el->header.datalen = size;
    el->header.seqno   = vc_ch->send_seqno++;
    MPL_DBG_STMT (MPIDI_CH3_DBG_CHANNEL, VERBOSE, el->header.type = MPID_NEM_PKT_MPICH_HEAD);
    
    MPIR_Memcpy((void *)el->payload, buf, size);
    DO_PAPI (PAPI_accum_var (PAPI_EventSet, PAPI_vvalues11));

    MPL_DBG_MSG (MPIDI_CH3_DBG_CHANNEL, VERBOSE, "--> Sent queue");
    MPL_DBG_STMT (MPIDI_CH3_DBG_CHANNEL, VERBOSE, MPID_nem_dbg_dump_cell (el));

    DO_PAPI (PAPI_reset (PAPI_EventSet));
    MPID_nem_queue_enqueue (vc_ch->recv_queue, el);
    /*MPID_nem_rel_dump_queue( vc_ch->recv_queue ); */
    DO_PAPI (PAPI_accum_var (PAPI_EventSet, PAPI_vvalues12));
    DO_PAPI (PAPI_reset (PAPI_EventSet));

#ifdef PREFETCH_CELL
    DO_PAPI (PAPI_reset (PAPI_EventSet));
    if (!MPID_nem_queue_empty (MPID_nem_mem_region.my_freeQ))
	MPID_nem_queue_dequeue (MPID_nem_mem_region.my_freeQ, &MPID_nem_prefetched_cell);
    else
	MPID_nem_prefetched_cell = 0;
    DO_PAPI (PAPI_accum_var (PAPI_EventSet, PAPI_vvalues10));
#endif /*PREFETCH_CELL */

    /*DO_PAPI (PAPI_accum_var (PAPI_EventSet, PAPI_vvalues14)); */

 return_success:
    *again = 0;
    goto fn_exit;
 return_again:
    *again = 1;
    goto fn_exit;
 fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_NEM_MPICH_SEND_HEADER);
    return mpi_errno;
}


/*
  int MPID_nem_mpich_sendv (struct iovec **iov, int *n_iov, MPIDI_VC_t *vc);

  sends iov to vc
  Non-blocking
  if iov specifies more than MPID_NEM_MPICH_DATA_LEN of data, the iov will be truncated, so that after MPID_nem_mpich_sendv returns,
  iov will describe unsent data
  sets again to 1 if it can't get a free cell, 0 otherwise
*/
static inline int
MPID_nem_mpich_sendv (struct iovec **iov, int *n_iov, MPIDI_VC_t *vc, int *again)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_nem_cell_ptr_t el;
    char *cell_buf;
    intptr_t payload_len;
    int my_rank;
    MPIDI_CH3I_VC *vc_ch = &vc->ch;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_NEM_MPICH_SENDV);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_NEM_MPICH_SENDV);

    MPIR_Assert (*n_iov > 0 && (*iov)->iov_len > 0);
    MPIR_Assert(vc_ch->is_local);

    DO_PAPI (PAPI_reset (PAPI_EventSet));

    my_rank = MPID_nem_mem_region.rank;
	
#ifdef PREFETCH_CELL
    el = MPID_nem_prefetched_cell;
    
    if (!el)
    {
	if (MPID_nem_queue_empty (MPID_nem_mem_region.my_freeQ))
	{
	    DO_PAPI (PAPI_accum_var (PAPI_EventSet, PAPI_vvalues5));
            goto return_again;
	}
	
	MPID_nem_queue_dequeue (MPID_nem_mem_region.my_freeQ, &el);
    }
#else /*PREFETCH_CELL     */
    if (MPID_nem_queue_empty (MPID_nem_mem_region.my_freeQ))
    {
	DO_PAPI (PAPI_accum_var (PAPI_EventSet, PAPI_vvalues5));
        goto return_again;
    }

    MPID_nem_queue_dequeue (MPID_nem_mem_region.my_freeQ , &el);
#endif /*PREFETCH_CELL     */

    payload_len = MPID_NEM_MPICH_DATA_LEN;
    cell_buf    = (char *) el->payload; /* cast away volatile */
    
    while (*n_iov && payload_len >= (*iov)->iov_len)
    {
	size_t _iov_len = (*iov)->iov_len;
	MPIR_Memcpy (cell_buf, (*iov)->iov_base, _iov_len);
	payload_len -= _iov_len;
	cell_buf += _iov_len;
	--(*n_iov);
	++(*iov);
    }
    
    if (*n_iov && payload_len > 0)
    {
	MPIR_Memcpy (cell_buf, (*iov)->iov_base, payload_len);
	(*iov)->iov_base = (char *)(*iov)->iov_base + payload_len;
	(*iov)->iov_len -= payload_len;
 	payload_len = 0;
    }

    el->header.source  = my_rank;
    el->header.dest    = vc->lpid;
    el->header.datalen = MPID_NEM_MPICH_DATA_LEN - payload_len;
    el->header.seqno   = vc_ch->send_seqno++;
    MPL_DBG_STMT (MPIDI_CH3_DBG_CHANNEL, VERBOSE, el->header.type = MPID_NEM_PKT_MPICH);

    MPL_DBG_MSG (MPIDI_CH3_DBG_CHANNEL, VERBOSE, "--> Sent queue");
    MPL_DBG_STMT (MPIDI_CH3_DBG_CHANNEL, VERBOSE, MPID_nem_dbg_dump_cell (el));

    MPID_nem_queue_enqueue (vc_ch->recv_queue, el);
    /*MPID_nem_rel_dump_queue( vc_ch->recv_queue ); */

#ifdef PREFETCH_CELL
    if (!MPID_nem_queue_empty (MPID_nem_mem_region.my_freeQ))
	MPID_nem_queue_dequeue (MPID_nem_mem_region.my_freeQ, &MPID_nem_prefetched_cell);
    else
	MPID_nem_prefetched_cell = 0;
#endif /*PREFETCH_CELL */
    DO_PAPI (PAPI_accum_var (PAPI_EventSet, PAPI_vvalues5));

    *again = 0;
    goto fn_exit;
 return_again:
    *again = 1;
    goto fn_exit;
 fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_NEM_MPICH_SENDV);
    return mpi_errno;
}

/* MPID_nem_mpich_sendv_header (struct iovec **iov, int *n_iov, int dest)
   same as above but first iov element is an MPICH header */
static inline int
MPID_nem_mpich_sendv_header (struct iovec **iov, int *n_iov, MPIDI_VC_t *vc, int *again)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_nem_cell_ptr_t el;
    char *cell_buf;
    intptr_t payload_len;
    int my_rank;
    MPIDI_CH3I_VC *vc_ch = &vc->ch;
    MPI_Aint buf_offset = 0;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_NEM_MPICH_SENDV_HEADER);
    
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_NEM_MPICH_SENDV_HEADER);

    MPIR_Assert(vc_ch->is_local);

    DO_PAPI (PAPI_reset (PAPI_EventSet));
    MPIR_Assert (*n_iov > 0 && (*iov)->iov_len == sizeof(MPIDI_CH3_Pkt_t));

    my_rank = MPID_nem_mem_region.rank;

#ifdef USE_FASTBOX
    if (*n_iov == 2 && (*iov)[1].iov_len + sizeof(MPIDI_CH3_Pkt_t) <= MPID_NEM_FBOX_DATALEN)
    {
	MPID_nem_fastbox_t *pbox = vc_ch->fbox_out;
        MPID_nem_cell_t *cell_ptr = MPID_NEM_FBOX_TO_CELL(pbox);

        if (MPID_nem_fbox_is_full(pbox))
            goto usequeue_l;

        cell_ptr->header.source  = MPID_nem_mem_region.local_rank;
        cell_ptr->header.datalen = (*iov)[1].iov_len + sizeof(MPIDI_CH3_Pkt_t);
        cell_ptr->header.seqno   = vc_ch->send_seqno++;
        MPL_DBG_STMT (MPIDI_CH3_DBG_CHANNEL, VERBOSE, cell_ptr->header.type = MPID_NEM_PKT_MPICH_HEAD);
        
        MPIR_Memcpy((void *)cell_ptr->payload, (*iov)[0].iov_base, (*iov)[0].iov_len);
        MPIR_Memcpy ((char *)cell_ptr->payload + (*iov)[0].iov_len, (*iov)[1].iov_base, (*iov)[1].iov_len);
        
        MPL_atomic_release_store_int(&pbox->flag, 1);
        *n_iov = 0;

        MPL_DBG_MSG (MPIDI_CH3_DBG_CHANNEL, VERBOSE, "--> Sent fbox ");
        MPL_DBG_STMT (MPIDI_CH3_DBG_CHANNEL, VERBOSE, MPID_nem_dbg_dump_cell (cell_ptr));
        
        goto return_success;
    }
 usequeue_l:
    MPIR_T_PVAR_COUNTER_INC_VAR(NEM, &MPID_nem_fbox_fall_back_to_queue_count[MPID_nem_mem_region.local_ranks[vc->lpid]], 1);

#endif /*USE_FASTBOX */
	
#ifdef PREFETCH_CELL
    el = MPID_nem_prefetched_cell;

    if (!el)
    {
	if (MPID_nem_queue_empty (MPID_nem_mem_region.my_freeQ))
	{
	    DO_PAPI (PAPI_accum_var (PAPI_EventSet, PAPI_vvalues5));
            goto return_again;
	}
	
	MPID_nem_queue_dequeue (MPID_nem_mem_region.my_freeQ, &el);
    }
#else /*PREFETCH_CELL    */
    if (MPID_nem_queue_empty (MPID_nem_mem_region.my_freeQ))
    {
	DO_PAPI (PAPI_accum_var (PAPI_EventSet, PAPI_vvalues5));
        goto return_again;
    }

    MPID_nem_queue_dequeue (MPID_nem_mem_region.my_freeQ, &el);
#endif /*PREFETCH_CELL */

    MPIR_Memcpy((void *)el->payload, (*iov)->iov_base, sizeof(MPIDI_CH3_Pkt_t));
    buf_offset += sizeof(MPIDI_CH3_Pkt_t);

    cell_buf = (char *)(el->payload) + buf_offset;
    ++(*iov);
    --(*n_iov);

    payload_len = MPID_NEM_MPICH_DATA_LEN - buf_offset;
    while (*n_iov && payload_len >= (*iov)->iov_len)
    {
	size_t _iov_len = (*iov)->iov_len;
	MPIR_Memcpy (cell_buf, (*iov)->iov_base, _iov_len);
	payload_len -= _iov_len;
	cell_buf += _iov_len;
	--(*n_iov);
	++(*iov);
    }
    
    if (*n_iov && payload_len > 0)
    {
	MPIR_Memcpy (cell_buf, (*iov)->iov_base, payload_len);
	(*iov)->iov_base = (char *)(*iov)->iov_base + payload_len;
	(*iov)->iov_len -= payload_len;
	payload_len = 0;
    }

    el->header.source  = my_rank;
    el->header.dest    = vc->lpid;
    el->header.datalen = MPID_NEM_MPICH_DATA_LEN - payload_len;
    el->header.seqno   = vc_ch->send_seqno++;
    MPL_DBG_STMT (MPIDI_CH3_DBG_CHANNEL, VERBOSE, el->header.type = MPID_NEM_PKT_MPICH_HEAD);

    MPL_DBG_MSG (MPIDI_CH3_DBG_CHANNEL, VERBOSE, "--> Sent queue");
    MPL_DBG_STMT (MPIDI_CH3_DBG_CHANNEL, VERBOSE, MPID_nem_dbg_dump_cell (el));

    MPID_nem_queue_enqueue (vc_ch->recv_queue, el);	
    /*MPID_nem_rel_dump_queue( vc_ch->recv_queue ); */

#ifdef PREFETCH_CELL
    if (!MPID_nem_queue_empty (MPID_nem_mem_region.my_freeQ))
	MPID_nem_queue_dequeue (MPID_nem_mem_region.my_freeQ, &MPID_nem_prefetched_cell);
    else
	MPID_nem_prefetched_cell = 0;
#endif /*PREFETCH_CELL */
    DO_PAPI (PAPI_accum_var (PAPI_EventSet, PAPI_vvalues5));

 return_success:
    *again = 0;
    goto fn_exit;
 return_again:
    *again = 1;
    goto fn_exit;
 fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_NEM_MPICH_SENDV_HEADER);
    return mpi_errno;
}

/* send the header and data described by the segment in one cell.  If
   there is no cell available, *again is set to 1.  If all of the data
   cannot be sent, *msg_offset is set to the index of the first
   unsent byte.
   Pre condition:  This must be the first packet of a message (i.e.,
                       *segment first == 0)
                   The destination process is local
   Post conditions:  the header has been sent iff *again == 0
                     if there is data to send (msgsize > 0) then
                         (the header has been sent iff any data has
                         been sent (i.e., *msg_offset > 0) )
                     i.e.: we will never send only the header
*/
static inline void
MPID_nem_mpich_send_seg_header (void *buf, MPI_Aint count, MPI_Datatype datatype,
                                intptr_t *msg_offset, intptr_t msgsize,
                                void *header, intptr_t header_sz, MPIDI_VC_t *vc, int *again)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_NEM_MPICH_SEND_SEG_HEADER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_NEM_MPICH_SEND_SEG_HEADER);

    MPID_nem_cell_ptr_t el;
    intptr_t datalen;
    int my_rank;
    MPIDI_CH3I_VC *vc_ch = &vc->ch;
    MPI_Aint buf_offset = 0;

    MPIR_Assert(vc_ch->is_local); /* netmods will have their own implementation */
    MPIR_Assert(header_sz <= sizeof(MPIDI_CH3_Pkt_t));
    
    
    DO_PAPI (PAPI_reset (PAPI_EventSet));

    my_rank = MPID_nem_mem_region.rank;

#ifdef USE_FASTBOX
    if (sizeof(MPIDI_CH3_Pkt_t) + msgsize <= MPID_NEM_FBOX_DATALEN)
    {
	MPID_nem_fastbox_t *pbox = vc_ch->fbox_out;
        MPID_nem_cell_t *cell_ptr = MPID_NEM_FBOX_TO_CELL(pbox);

        /* Add a compiler time check on streaming unit size and FASTBOX size */
        MPIR_Static_assert((MPIDI_CH3U_Acc_stream_size > MPID_NEM_FBOX_DATALEN),
                           "RMA ACC Streaming unit size <= FASTBOX size in Nemesis.");

        /* NOTE: when FASTBOX is being used, streaming optimization is never triggered,
         * because streaming unit size is larger than FASTBOX size. In such case,
         * first offset (*msg_offset) is zero, and last offset (msgsize)
         * is the data size */
        MPIR_Assert(*msg_offset == 0);

        if (MPID_nem_fbox_is_full(pbox))
            goto usequeue_l;

	{
	    cell_ptr->header.source  = MPID_nem_mem_region.local_rank;
	    cell_ptr->header.datalen = sizeof(MPIDI_CH3_Pkt_t) + msgsize;
	    cell_ptr->header.seqno   = vc_ch->send_seqno++;
            MPL_DBG_STMT (MPIDI_CH3_DBG_CHANNEL, VERBOSE, cell_ptr->header.type = MPID_NEM_PKT_MPICH_HEAD);

            /* copy header */
            MPIR_Memcpy((void *)cell_ptr->payload, header, header_sz);
            
            /* copy data */
            MPI_Aint actual_pack_bytes;
            MPIR_Typerep_pack(buf, count, datatype, *msg_offset,
                           (char *)cell_ptr->payload + sizeof(MPIDI_CH3_Pkt_t),
                           msgsize - *msg_offset, &actual_pack_bytes);
            MPIR_Assert(actual_pack_bytes == msgsize - *msg_offset);

            MPL_atomic_release_store_int(&pbox->flag, 1);

            *msg_offset += actual_pack_bytes;;

	    MPL_DBG_MSG(MPIDI_CH3_DBG_CHANNEL, VERBOSE, "--> Sent fbox ");
	    MPL_DBG_STMT (MPIDI_CH3_DBG_CHANNEL, VERBOSE, MPID_nem_dbg_dump_cell (cell_ptr));

            goto return_success;
	}
    }
 usequeue_l:
    MPIR_T_PVAR_COUNTER_INC_VAR(NEM, &MPID_nem_fbox_fall_back_to_queue_count[MPID_nem_mem_region.local_ranks[vc->lpid]], 1);

#endif /*USE_FASTBOX */
	
#ifdef PREFETCH_CELL
    el = MPID_nem_prefetched_cell;

    if (!el)
    {
	if (MPID_nem_queue_empty (MPID_nem_mem_region.my_freeQ))
	{
	    DO_PAPI (PAPI_accum_var (PAPI_EventSet, PAPI_vvalues5));
            goto return_again;
	}
	
	MPID_nem_queue_dequeue (MPID_nem_mem_region.my_freeQ, &el);
    }
#else /*PREFETCH_CELL    */
    if (MPID_nem_queue_empty (MPID_nem_mem_region.my_freeQ))
    {
	DO_PAPI (PAPI_accum_var (PAPI_EventSet, PAPI_vvalues5));
        goto return_again;
    }

    MPID_nem_queue_dequeue (MPID_nem_mem_region.my_freeQ, &el);
#endif /*PREFETCH_CELL */

    /* copy header */
    MPIR_Memcpy((void *)el->payload, header, header_sz);
    
    buf_offset += sizeof(MPIDI_CH3_Pkt_t);

    /* copy data */
    MPI_Aint max_pack_bytes;
    if (msgsize - *msg_offset <= MPID_NEM_MPICH_DATA_LEN - buf_offset)
        max_pack_bytes = msgsize - *msg_offset;
    else
        max_pack_bytes = MPID_NEM_MPICH_DATA_LEN - buf_offset;

    MPI_Aint actual_pack_bytes;
    MPIR_Typerep_pack(buf, count, datatype, *msg_offset, (char *)el->payload + buf_offset,
                   max_pack_bytes, &actual_pack_bytes);
    datalen = buf_offset + actual_pack_bytes;
    *msg_offset += actual_pack_bytes;

    el->header.source  = my_rank;
    el->header.dest    = vc->lpid;
    el->header.datalen = datalen;
    el->header.seqno   = vc_ch->send_seqno++;
    MPL_DBG_STMT (MPIDI_CH3_DBG_CHANNEL, VERBOSE, el->header.type = MPID_NEM_PKT_MPICH_HEAD);

    MPL_DBG_MSG (MPIDI_CH3_DBG_CHANNEL, VERBOSE, "--> Sent queue");
    MPL_DBG_STMT (MPIDI_CH3_DBG_CHANNEL, VERBOSE, MPID_nem_dbg_dump_cell (el));

    MPID_nem_queue_enqueue (vc_ch->recv_queue, el);	

#ifdef PREFETCH_CELL
    if (!MPID_nem_queue_empty (MPID_nem_mem_region.my_freeQ))
	MPID_nem_queue_dequeue (MPID_nem_mem_region.my_freeQ, &MPID_nem_prefetched_cell);
    else
	MPID_nem_prefetched_cell = 0;
#endif /*PREFETCH_CELL */
    DO_PAPI (PAPI_accum_var (PAPI_EventSet, PAPI_vvalues5));

 return_success:
    *again = 0;
    goto fn_exit;
 return_again:
    *again = 1;
    goto fn_exit;
 fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_NEM_MPICH_SEND_SEG_HEADER);
    return;
}

/* similar to MPID_nem_mpich_send_seg_header, except there is no
   header to send.  This need not be the first packet of a message. */
static inline void
MPID_nem_mpich_send_seg (void *buf, MPI_Aint count, MPI_Datatype datatype,
                         intptr_t *msg_offset, intptr_t msgsize, MPIDI_VC_t *vc, int *again)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_NEM_MPICH_SEND_SEG);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_NEM_MPICH_SEND_SEG);

    MPID_nem_cell_ptr_t el;
    intptr_t datalen;
    int my_rank;
    MPIDI_CH3I_VC *vc_ch = &vc->ch;

    MPIR_Assert(vc_ch->is_local); /* netmods will have their own implementation */
    
    DO_PAPI (PAPI_reset (PAPI_EventSet));

    my_rank = MPID_nem_mem_region.rank;
	
#ifdef PREFETCH_CELL
    el = MPID_nem_prefetched_cell;
    
    if (!el)
    {
	if (MPID_nem_queue_empty (MPID_nem_mem_region.my_freeQ))
	{
	    DO_PAPI (PAPI_accum_var (PAPI_EventSet, PAPI_vvalues5));
            goto return_again;
	}
	
	MPID_nem_queue_dequeue (MPID_nem_mem_region.my_freeQ, &el);
    }
#else /*PREFETCH_CELL    */
    if (MPID_nem_queue_empty (MPID_nem_mem_region.my_freeQ))
    {
	DO_PAPI (PAPI_accum_var (PAPI_EventSet, PAPI_vvalues5));
        goto return_again;
    }

    MPID_nem_queue_dequeue (MPID_nem_mem_region.my_freeQ, &el);
#endif /*PREFETCH_CELL */

    /* copy data */
    MPI_Aint max_pack_bytes;
    if (msgsize - *msg_offset <= MPID_NEM_MPICH_DATA_LEN)
        max_pack_bytes = msgsize - *msg_offset;
    else
        max_pack_bytes = MPID_NEM_MPICH_DATA_LEN;

    MPI_Aint actual_pack_bytes;
    MPIR_Typerep_pack(buf, count, datatype, *msg_offset, (char *)el->payload,
                   max_pack_bytes, &actual_pack_bytes);
    datalen = actual_pack_bytes;
    *msg_offset += actual_pack_bytes;
    
    el->header.source  = my_rank;
    el->header.dest    = vc->lpid;
    el->header.datalen = datalen;
    el->header.seqno   = vc_ch->send_seqno++;
    MPL_DBG_STMT (MPIDI_CH3_DBG_CHANNEL, VERBOSE, el->header.type = MPID_NEM_PKT_MPICH_HEAD);

    MPL_DBG_MSG (MPIDI_CH3_DBG_CHANNEL, VERBOSE, "--> Sent queue");
    MPL_DBG_STMT (MPIDI_CH3_DBG_CHANNEL, VERBOSE, MPID_nem_dbg_dump_cell (el));

    MPID_nem_queue_enqueue (vc_ch->recv_queue, el);	

#ifdef PREFETCH_CELL
    if (!MPID_nem_queue_empty (MPID_nem_mem_region.my_freeQ))
	MPID_nem_queue_dequeue (MPID_nem_mem_region.my_freeQ, &MPID_nem_prefetched_cell);
    else
	MPID_nem_prefetched_cell = 0;
#endif /*PREFETCH_CELL */
    DO_PAPI (PAPI_accum_var (PAPI_EventSet, PAPI_vvalues5));

    *again = 0;
    goto fn_exit;
 return_again:
    *again = 1;
    goto fn_exit;
 fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_NEM_MPICH_SEND_SEG_HEADER);
    return;
}

/*
  MPID_nem_mpich_dequeue_fastbox (int local_rank)
  decrements usage count on fastbox for process with local rank local_rank and
  dequeues it from fbox queue if usage is 0.
  This function is called whenever a receive for a process on this node is matched.
  Fastboxes on fbox queue are polled regularly for incoming messages.
*/
static inline void MPID_nem_mpich_dequeue_fastbox(int local_rank)
{
    MPID_nem_fboxq_elem_t *el;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_NEM_MPICH_DEQUEUE_FASTBOX);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_NEM_MPICH_DEQUEUE_FASTBOX);

    MPIR_Assert(local_rank < MPID_nem_mem_region.num_local);

    el = &MPID_nem_fboxq_elem_list[local_rank];    
    MPIR_Assert(el->fbox != NULL);

    MPIR_Assert(el->usage);

    --el->usage;
    if (el->usage == 0)
    {
	if (el->prev == NULL)
	    MPID_nem_fboxq_head = el->next;
	else
	    el->prev->next = el->next;

	if (el->next == NULL)
	    MPID_nem_fboxq_tail = el->prev;
	else
	    el->next->prev = el->prev;

	if (el == MPID_nem_curr_fboxq_elem)
	{
	    if (el->next == NULL)
		MPID_nem_curr_fboxq_elem = MPID_nem_fboxq_head;
	    else
		MPID_nem_curr_fboxq_elem = el->next;
	}
    }    
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_NEM_MPICH_DEQUEUE_FASTBOX);
}

/*
  MPID_nem_mpich_enqueue_fastbox (int local_rank)
  enqueues fastbox for process with local rank local_rank on fbox queue
  This function is called whenever a receive is posted for a process on this node.
  Fastboxes on fbox queue are polled regularly for incoming messages.
*/
static inline void MPID_nem_mpich_enqueue_fastbox(int local_rank)
{
    MPID_nem_fboxq_elem_t *el;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_NEM_MPICH_ENQUEUE_FASTBOX);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_NEM_MPICH_ENQUEUE_FASTBOX);

    MPIR_Assert(local_rank < MPID_nem_mem_region.num_local);

    el = &MPID_nem_fboxq_elem_list[local_rank];
    MPIR_Assert(el->fbox != NULL);

    if (el->usage)
    {
	++el->usage;
    }
    else
    {
	el->usage = 1;
	if (MPID_nem_fboxq_tail == NULL)
	{
	    el->prev = NULL;
	    MPID_nem_curr_fboxq_elem = MPID_nem_fboxq_head = el;
	}
	else
	{
	    el->prev = MPID_nem_fboxq_tail;
	    MPID_nem_fboxq_tail->next = el;
	}
	    
	el->next = NULL;
	MPID_nem_fboxq_tail = el;
    }
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_NEM_MPICH_ENQUEUE_FASTBOX);
}
/*
  MPID_nem_recv_seqno_matches (MPID_nem_queue_ptr_t qhead)
  check whether the sequence number for the cell at the head of qhead is the one
  expected from the sender of that cell
  We only check these for processes in COMM_WORLD (i.e. the ones initially allocated)
*/
static inline int
MPID_nem_recv_seqno_matches (MPID_nem_queue_ptr_t qhead)
{
    int source;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_NEM_RECV_SEQNO_MATCHES);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_NEM_RECV_SEQNO_MATCHES);

    MPID_nem_cell_ptr_t cell = MPID_nem_queue_head(qhead);
    source = cell->header.source;
    
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_NEM_RECV_SEQNO_MATCHES);
    return (cell->header.seqno == MPID_nem_recv_seqno[source]);
}

/*
  int MPID_nem_mpich_test_recv (MPID_nem_cell_ptr_t *cell, int *in_fbox);

  non-blocking receive
  sets cell to the received cell, or NULL if there is nothing to receive. in_fbox is true iff the cell was found in a fbox
  the cell must be released back to the subsystem with MPID_nem_mpich_release_cell() once the packet has been copied out
*/
static inline int
MPID_nem_mpich_test_recv(MPID_nem_cell_ptr_t *cell, int *in_fbox, int in_blocking_progress)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_NEM_MPICH_TEST_RECV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_NEM_MPICH_TEST_RECV);
    
    DO_PAPI (PAPI_reset (PAPI_EventSet));

#ifdef USE_FASTBOX
    if (poll_active_fboxes(cell)) goto fbox_l;
#endif/* USE_FASTBOX     */

    if (MPID_nem_num_netmods)
    {
	mpi_errno = MPID_nem_network_poll(in_blocking_progress);
        if (mpi_errno) MPIR_ERR_POP (mpi_errno);
    }

    if (MPID_nem_queue_empty (MPID_nem_mem_region.my_recvQ) || !MPID_nem_recv_seqno_matches (MPID_nem_mem_region.my_recvQ))
    {
#ifdef USE_FASTBOX
        /* check for messages from any process (even those from which
           we don't expect messages).  If we're nonblocking, check all
           fboxes at once, if we're in a blocking loop we'll keep
           iterating, so just check them one at a time. */
        if (!in_blocking_progress) {
            int found;
            found = poll_every_fbox(cell);
            if (found)
                goto fbox_l;
        } else {
            poll_next_fbox (cell, goto fbox_l);
        }
#endif/* USE_FASTBOX     */
	*cell = NULL;
	goto fn_exit;
    }
    
    MPID_nem_queue_dequeue (MPID_nem_mem_region.my_recvQ, cell);

    ++MPID_nem_recv_seqno[(*cell)->header.source];
    *in_fbox = 0;

 fn_exit:
    DO_PAPI (PAPI_accum_var (PAPI_EventSet, PAPI_vvalues6));
    
    MPL_DBG_STMT (MPIDI_CH3_DBG_CHANNEL, VERBOSE, {
	if (*cell)
	{
	    MPL_DBG_MSG_S (MPIDI_CH3_DBG_CHANNEL, VERBOSE, "<-- Recv %s", (*in_fbox) ? "fbox " : "queue");
	    MPL_DBG_STMT (MPIDI_CH3_DBG_CHANNEL, VERBOSE, MPID_nem_dbg_dump_cell (*cell));
	}
    });

 fn_fail:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_NEM_MPICH_TEST_RECV);
    return mpi_errno;

 fbox_l:
   *in_fbox = 1;
    goto fn_exit;

}

/*
  int MPID_nem_mpich_test_recv_wait (MPID_nem_cell_ptr_t *cell, int *in_fbox, int timeout);

  blocking receive with timeout
  waits up to timeout iterations to receive a cell
  sets cell to the received cell, or NULL if there is nothing to receive. in_fbox is true iff the cell was found in a fbox
  the cell must be released back to the subsystem with MPID_nem_mpich_release_cell() once the packet has been copied out
*/
static inline int
MPID_nem_mpich_test_recv_wait (MPID_nem_cell_ptr_t *cell, int *in_fbox, int timeout)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_NEM_MPICH_TEST_RECV_WAIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_NEM_MPICH_TEST_RECV_WAIT);
#ifdef USE_FASTBOX
    if (poll_active_fboxes(cell)) goto fbox_l;
#endif/* USE_FASTBOX     */

    if (MPID_nem_num_netmods)
    {
	mpi_errno = MPID_nem_network_poll(TRUE /* blocking */);
        if (mpi_errno) MPIR_ERR_POP (mpi_errno);
    }

    while ((--timeout > 0) && (MPID_nem_queue_empty (MPID_nem_mem_region.my_recvQ) || !MPID_nem_recv_seqno_matches (MPID_nem_mem_region.my_recvQ)))
    {
#ifdef USE_FASTBOX
	poll_next_fbox (cell, goto fbox_l);
#endif/* USE_FASTBOX     */
	*cell = NULL;
	goto exit_l;
    }
    
    MPID_nem_queue_dequeue (MPID_nem_mem_region.my_recvQ, cell);

    ++MPID_nem_recv_seqno[(*cell)->header.source];
    *in_fbox = 0;
 exit_l:
    
    MPL_DBG_STMT (MPIDI_CH3_DBG_CHANNEL, VERBOSE, {
            if (*cell)
            {
                MPL_DBG_MSG_S (MPIDI_CH3_DBG_CHANNEL, VERBOSE, "<-- Recv %s", (*in_fbox) ? "fbox " : "queue");
                MPL_DBG_STMT (MPIDI_CH3_DBG_CHANNEL, VERBOSE, MPID_nem_dbg_dump_cell (*cell));
            }
        });

 fn_fail:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_NEM_MPICH_TEST_RECV_WAIT);
    return mpi_errno;

 fbox_l:
    *in_fbox = 1;
    goto exit_l;
}

/*
  int MPID_nem_mpich_blocking_recv (MPID_nem_cell_ptr_t *cell, int *in_fbox);

  blocking receive waits until there is something to receive, or then
  sets cell to the received cell. in_fbox is true iff the cell was
  found in a fbox the cell must be released back to the subsystem with
  MPID_nem_mpich_release_cell() once the packet has been copied out
*/
static inline int
MPID_nem_mpich_blocking_recv(MPID_nem_cell_ptr_t *cell, int *in_fbox, int completions)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_NEM_MPICH_BLOCKING_RECV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_NEM_MPICH_BLOCKING_RECV);
    DO_PAPI (PAPI_reset (PAPI_EventSet));

    /* We should never enter this function in a multithreaded app */
    MPIR_Assert(!MPIR_IS_THREADED);

#ifdef USE_FASTBOX
    if (poll_active_fboxes(cell)) goto fbox_l;
#endif /*USE_FASTBOX */

    if (MPID_nem_num_netmods)
    {
	mpi_errno = MPID_nem_network_poll(TRUE /* blocking */);
        if (mpi_errno) MPIR_ERR_POP (mpi_errno);
    }

    while (MPID_nem_queue_empty (MPID_nem_mem_region.my_recvQ) || !MPID_nem_recv_seqno_matches (MPID_nem_mem_region.my_recvQ))
    {
	DO_PAPI (PAPI_reset (PAPI_EventSet));

#ifdef USE_FASTBOX	
	poll_next_fbox (cell, goto fbox_l);
        if (poll_active_fboxes(cell)) goto fbox_l;
#endif /*USE_FASTBOX */

	if (MPID_nem_num_netmods)
	{            
	    mpi_errno = MPID_nem_network_poll(TRUE /* blocking */);
            if (mpi_errno) MPIR_ERR_POP (mpi_errno);

            if (!MPID_nem_safe_to_block_recv())
            {
                *cell = NULL;
                *in_fbox = 0;
                goto exit_l;
            }
	}

        if (completions != MPL_atomic_relaxed_load_int(&MPIDI_CH3I_progress_completion_count)) {
            *cell = NULL;
            *in_fbox = 0;
            goto exit_l;
        }
        MPIU_Busy_wait();
    }

    MPID_nem_queue_dequeue (MPID_nem_mem_region.my_recvQ, cell);

    ++MPID_nem_recv_seqno[(*cell)->header.source];
    *in_fbox = 0;

 exit_l:    

    DO_PAPI (PAPI_accum_var (PAPI_EventSet,PAPI_vvalues8));
    
    MPL_DBG_STMT (MPIDI_CH3_DBG_CHANNEL, VERBOSE, {
            if (*cell)
            {
                MPL_DBG_MSG_S (MPIDI_CH3_DBG_CHANNEL, VERBOSE, "<-- Recv %s", (*in_fbox) ? "fbox " : "queue");
                MPL_DBG_STMT (MPIDI_CH3_DBG_CHANNEL, VERBOSE, MPID_nem_dbg_dump_cell(*cell));
            }
        });

 fn_fail:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_NEM_MPICH_BLOCKING_RECV);
    return mpi_errno;

 fbox_l:
    *in_fbox = 1;
    goto exit_l;
}

/*
  int MPID_nem_mpich_release_cell (MPID_nem_cell_ptr_t cell, MPIDI_VC_t *vc);

  releases the cell back to the subsystem to be used for subsequent receives
*/
static inline int
MPID_nem_mpich_release_cell (MPID_nem_cell_ptr_t cell, MPIDI_VC_t *vc)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_NEM_MPICH_RELEASE_CELL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_NEM_MPICH_RELEASE_CELL);

    MPIDI_CH3I_VC *vc_ch = &vc->ch;
    DO_PAPI (PAPI_reset (PAPI_EventSet));
    MPID_nem_queue_enqueue (vc_ch->free_queue, cell);
    DO_PAPI (PAPI_accum_var (PAPI_EventSet,PAPI_vvalues9));

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_NEM_MPICH_RELEASE_CELL);
    return mpi_errno;
}

#endif /* MPID_NEM_INLINE_H_INCLUDED */

