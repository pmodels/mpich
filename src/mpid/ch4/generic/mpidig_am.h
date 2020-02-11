/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MPIDIG_AM_H_INCLUDED
#define MPIDIG_AM_H_INCLUDED

MPL_STATIC_INLINE_PREFIX MPIDIG_am_send(int grank, int handler_id, void *am_hdr,
                                        MPI_Aint am_hdr_size, void *buf, MPI_Aint count,
                                        MPI_Datatype datatype, MPIR_Request * sreq)
{
#ifdef MPIDI_CH4_DIRECT_NETMOD
    return MPIDI_NM_am_send(grank, handler_id, am_hdr, am_hdr_size, buf, count, datatype, sreq);
#else
    if (MPIDI_av_table0->table[grank].is_local) {
        return MPIDI_SHM_am_send(grank, handler_id, am_hdr, am_hdr_size, buf, count, datatype,
                                 sreq);
    } else {
        return MPIDI_NM_am_send(grank, handler_id, am_hdr, am_hdr_size, buf, count, datatype, sreq);
    }
#endif
}

MPL_STATIC_INLINE_PREFIX MPIDIG_am_send_hdr(int grank, int handler_id, void *am_hdr,
                                            MPI_Aint am_hdr_size)
{
    MPIDIG_am_send(grank, handler_id, am_hdr, am_hdr_size, NULL, 0, MPI_BYTE, NULL);
}

/* src is a global source rank, which is needed for sending replies
 * In synchronous mode, payload is supplied in its entirety, and req is set to NULL.
 * In asynchronous mode, payoad is NULL, and req is returned for persistence.
 */
typedef int (*MPIDIG_am_handler) (int src, void *am_hdr, void *payload,
                                  MPI_Aint payload_size, MPIR_Request ** req);

MPL_STATIC_INLINE_PREFIX MPIDIG_am_run_handler(int handler_id, int src, void *am_hdr,
                                               void *payload, MPI_Aint payload_size)
{
    return MPIDIG_global.target_msg_cbs[handler_id] (src, am_hdr, payload, payload_size, NULL);
}

MPL_STATIC_INLINE_PREFIX MPIDIG_am_run_handler_async(int handler_id, int src, void *am_hdr,
                                                     MPI_Aint total_payload_size,
                                                     MPIR_Request ** req)
{
    return MPIDIG_global.target_msg_cbs[handler_id] (src, am_hdr, NULL, total_payload_size, req);
}

#endif /* MPIDIG_AM_H_INCLUDED */
