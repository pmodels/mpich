/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef IPC_CONTROL_H_INCLUDED
#define IPC_CONTROL_H_INCLUDED

#include "shm_types.h"

int MPIDI_IPCI_send_contig_lmt_rts_cb(MPIDI_SHMI_ctrl_hdr_t * ctrl_hdr);
int MPIDI_IPCI_send_contig_lmt_fin_cb(MPIDI_SHMI_ctrl_hdr_t * ctrl_hdr);

#endif /* IPC_CONTROL_H_INCLUDED */
