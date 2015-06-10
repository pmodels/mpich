/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2012 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#include "ofi_impl.h"


MPID_nem_ofi_global_t gl_data;

/* ************************************************************************** */
/* Netmod Function Table                                                      */
/* ************************************************************************** */
MPIDI_Comm_ops_t _g_comm_ops = {
    MPID_nem_ofi_recv_posted,   /* recv_posted */

    MPID_nem_ofi_send,  /* send */
    MPID_nem_ofi_send,  /* rsend */
    MPID_nem_ofi_ssend, /* ssend */
    MPID_nem_ofi_isend, /* isend */
    MPID_nem_ofi_isend, /* irsend */
    MPID_nem_ofi_issend,        /* issend */

    NULL,       /* send_init */
    NULL,       /* bsend_init */
    NULL,       /* rsend_init */
    NULL,       /* ssend_init */
    NULL,       /* startall */

    MPID_nem_ofi_cancel_send,   /* cancel_send */
    MPID_nem_ofi_cancel_recv,   /* cancel_recv */

    NULL,       /* probe */
    MPID_nem_ofi_iprobe,        /* iprobe */
    MPID_nem_ofi_improbe        /* improbe */
};

MPID_nem_netmod_funcs_t MPIDI_nem_ofi_funcs = {
    MPID_nem_ofi_init,
    MPID_nem_ofi_finalize,
#ifdef ENABLE_CHECKPOINTING
    NULL,
    NULL,
    NULL,
#endif
    MPID_nem_ofi_poll,
    MPID_nem_ofi_get_business_card,
    MPID_nem_ofi_connect_to_root,
    MPID_nem_ofi_vc_init,
    MPID_nem_ofi_vc_destroy,
    MPID_nem_ofi_vc_terminate,
    MPID_nem_ofi_anysource_iprobe,
    MPID_nem_ofi_anysource_improbe,
    MPID_nem_ofi_get_ordering,
};
