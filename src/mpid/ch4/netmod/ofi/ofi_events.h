/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef OFI_EVENTS_H_INCLUDED
#define OFI_EVENTS_H_INCLUDED

#include "ofi_impl.h"
#include "ofi_am_impl.h"
#include "ofi_am_events.h"
#include "ofi_control.h"
#include "utlist.h"

int MPIDI_OFI_send_event(struct fi_cq_tagged_entry *wc, MPIR_Request * sreq, int event_id);
int MPIDI_OFI_rma_done_event(struct fi_cq_tagged_entry *wc, MPIR_Request * in_req);
int MPIDI_OFI_get_huge_event(struct fi_cq_tagged_entry *wc, MPIR_Request * req);
int MPIDI_OFI_dispatch_function(struct fi_cq_tagged_entry *wc, MPIR_Request * req);
int MPIDI_OFI_get_buffered(struct fi_cq_tagged_entry *wc, ssize_t num);
int MPIDI_OFI_handle_cq_entries(struct fi_cq_tagged_entry *wc, ssize_t num);
int MPIDI_OFI_handle_cq_error(int vni_idx, ssize_t ret);

#endif /* OFI_EVENTS_H_INCLUDED */
