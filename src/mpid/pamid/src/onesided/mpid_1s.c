/* begin_generated_IBM_copyright_prolog                             */
/*                                                                  */
/* This is an automatically generated copyright prolog.             */
/* After initializing,  DO NOT MODIFY OR MOVE                       */
/*  --------------------------------------------------------------- */
/* Licensed Materials - Property of IBM                             */
/* Blue Gene/Q 5765-PER 5765-PRP                                    */
/*                                                                  */
/* (C) Copyright IBM Corp. 2011, 2012 All Rights Reserved           */
/* US Government Users Restricted Rights -                          */
/* Use, duplication, or disclosure restricted                       */
/* by GSA ADP Schedule Contract with IBM Corp.                      */
/*                                                                  */
/*  --------------------------------------------------------------- */
/*                                                                  */
/* end_generated_IBM_copyright_prolog                               */
/*  (C)Copyright IBM Corp.  2007, 2011  */
/**
 * \file src/onesided/mpid_1s.c
 * \brief ???
 */
#include "mpidi_onesided.h"


void
MPIDI_Win_DoneCB(pami_context_t  context,
                 void          * cookie,
                 pami_result_t   result)
{
  MPIDI_Win_request *req = (MPIDI_Win_request*)cookie;
  ++req->win->mpid.sync.complete;
  ++req->origin.completed;

  if ((req->buffer_free) && (req->type == MPIDI_WIN_REQUEST_GET))
    {
      if (req->origin.completed == req->target.dt.num_contig)
        {
          int mpi_errno;
          mpi_errno = MPIR_Localcopy(req->buffer,
                                     req->origin.dt.size,
                                     MPI_CHAR,
                                     req->origin.addr,
                                     req->origin.count,
                                     req->origin.datatype);
          MPID_assert(mpi_errno == MPI_SUCCESS);
          MPID_Datatype_release(req->origin.dt.pointer);
          MPIU_Free(req->buffer);
          MPIU_Free(req->user_buffer);
          req->buffer_free = 0;
        }
    }


    if ((req->origin.completed == req->target.dt.num_contig) || 
        ((req->type >= MPIDI_WIN_REQUEST_COMPARE_AND_SWAP) && 
         (req->origin.completed == req->origin.dt.num_contig)))
    {
      MPID_Request * req_handle = req->req_handle;

      if (req->buffer_free) {
          MPIU_Free(req->buffer);
          MPIU_Free(req->user_buffer);
          req->buffer_free = 0;
      }
      MPIDI_Win_datatype_unmap(&req->target.dt);
      if (req->accum_headers)
          MPIU_Free(req->accum_headers);

      if (!((req->type > MPIDI_WIN_REQUEST_GET_ACCUMULATE) && (req->type <=MPIDI_WIN_REQUEST_RGET_ACCUMULATE)))
          MPIU_Free(req);

      if(req_handle) {

          /* The instant this completion counter is set to zero another thread
           * may notice the change and begin freeing request resources. The
           * thread executing the code in this function must not touch any
           * portion of the request structure after decrementing the completion
           * counter.
           *
           * See MPID_Request_release_inline()
           */
          MPID_cc_set(req_handle->cc_ptr, 0);
      }
    }
  if ((MPIDI_Process.typed_onesided == 1) && (!req->target.dt.contig || !req->origin.dt.contig)) {
    /* We used the PAMI_Rput/Rget_typed call and added a ref so any MPI_Type_free before the context
     * executes the put/get would not free the MPID_Datatype, which would also free the associated PAMI datatype
     * which was still needed for communication -- that communication has completed, so now release the ref
     * in the callback to allow the MPID_Datatype to be freed.
     */
    MPID_Datatype_release(req->origin.dt.pointer);
    MPID_Datatype_release(req->target.dt.pointer);
  }
  MPIDI_Progress_signal();
}


void
MPIDI_Win_datatype_basic(int              count,
                         MPI_Datatype     datatype,
                         MPIDI_Datatype * dt)
{
  MPIDI_Datatype_get_info(dt->count = count,
                          dt->type  = datatype,
                          dt->contig,
                          dt->size,
                          dt->pointer,
                          dt->true_lb);
  TRACE_ERR("DT=%08x  DTp=%p  count=%d  contig=%d  size=%zu  true_lb=%zu\n",
            dt->type, dt->pointer, dt->count, dt->contig, (size_t)dt->size, (size_t)dt->true_lb);
}


void
MPIDI_Win_datatype_map(MPIDI_Datatype * dt)
{
  if (dt->contig)
    {
      dt->num_contig = 1;
      dt->map = &dt->__map;
      dt->map[0].DLOOP_VECTOR_BUF = (void*)(size_t)dt->true_lb;
      dt->map[0].DLOOP_VECTOR_LEN = dt->size;
    }
  else
    {
      unsigned map_size = dt->pointer->max_contig_blocks*dt->count + 1;
      dt->num_contig = map_size;
      dt->map = (DLOOP_VECTOR*)MPIU_Malloc(map_size * sizeof(DLOOP_VECTOR));
      MPID_assert(dt->map != NULL);

      DLOOP_Offset last = dt->pointer->size*dt->count;
      MPID_Segment seg;
      MPID_Segment_init(NULL, dt->count, dt->type, &seg, 0);
      MPID_Segment_pack_vector(&seg, 0, &last, dt->map, &dt->num_contig);
      MPID_assert((unsigned)dt->num_contig <= map_size);
#ifdef TRACE_ON
      TRACE_ERR("dt->pointer->size=%d  num_contig:  orig=%u  new=%d\n", dt->pointer->size, map_size, dt->num_contig);
      int i;
      for(i=0; i<dt->num_contig; ++i)
        TRACE_ERR("     %d:  BUF=%zu  LEN=%zu\n", i, (size_t)dt->map[i].DLOOP_VECTOR_BUF, (size_t)dt->map[i].DLOOP_VECTOR_LEN);
#endif
    }
}


void
MPIDI_Win_datatype_unmap(MPIDI_Datatype * dt)
{
  if (dt->map != &dt->__map)
    MPIU_Free(dt->map);;
}
