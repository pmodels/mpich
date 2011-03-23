/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 * Copyright © 2006-2011 Guillaume Mercier, Institut Polytechnique de
 * Bordeaux. All rights reserved. Permission is hereby granted to use,
 * reproduce, prepare derivative works, and to redistribute to others.
 */

#ifndef NMNAD_EXTENDED_INTERFACE_H
#define NMNAD_EXTENDED_INTERFACE_H

#include "mpid_nem_impl.h"

static inline int nm_sr_irecv_with_ref_tagged(nm_session_t p_session,
					      nm_gate_t p_gate, 
					      nm_tag_t tag, nm_tag_t mask,
					      void *data, uint32_t len,
					      nm_sr_request_t *p_request,
					      void *ref)
{
  nm_sr_recv_init(p_session, p_request);
  nm_sr_request_set_ref(p_session, p_request, ref);
  nm_sr_recv_unpack_data(p_session, p_request, data, len);
  if(ref != NULL)
    nm_sr_request_set_completion_queue(p_session, p_request);
  const int err = nm_sr_recv_irecv(p_session, p_request, p_gate, tag, mask);
  return err;
}

static inline int nm_sr_irecv_iov_with_ref_tagged(nm_session_t p_session,
						  nm_gate_t p_gate, 
						  nm_tag_t tag, nm_tag_t mask,
						  struct iovec *iov, int num_entries,
						  nm_sr_request_t *p_request, void *ref)
{
  nm_sr_recv_init(p_session, p_request);
  nm_sr_request_set_ref(p_session, p_request, ref);
  nm_sr_recv_unpack_iov(p_session, p_request, iov, num_entries);
  if(ref != NULL)
    nm_sr_request_set_completion_queue(p_session, p_request);
  const int err = nm_sr_recv_irecv(p_session, p_request, p_gate, tag, mask);
  return err;
}


static inline int nm_sr_issend_iov(nm_session_t p_session,
                                  nm_gate_t p_gate, nm_tag_t tag,
                                  const struct iovec *iov, int num_entries,
                                  nm_sr_request_t *p_request)
{
  nm_sr_send_init(p_session, p_request);
  nm_sr_send_pack_iov(p_session, p_request, iov, num_entries);
  const int err = nm_sr_send_issend(p_session, p_request, p_gate, tag);
  return err;
}

static inline int nm_sr_issend_iov_with_ref(nm_session_t p_session,
                                           nm_gate_t p_gate, nm_tag_t tag,
                                           const struct iovec *iov, int num_entries,
                                           nm_sr_request_t *p_request,
                                           void*ref)
{
  nm_sr_send_init(p_session, p_request);
  nm_sr_request_set_ref(p_session, p_request, ref);
  nm_sr_send_pack_iov(p_session, p_request, iov, num_entries);
  if(ref != NULL)
    nm_sr_request_set_completion_queue(p_session, p_request);
  const int err = nm_sr_send_issend(p_session, p_request, p_gate, tag);
  return err;
}


#endif
