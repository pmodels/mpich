/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2012 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

#include "scif_impl.h"

#undef FUNCNAME
#define FUNCNAME MPID_nem_scif_finalize
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_scif_finalize(void)
{
    int i, ret;
    scifconn_t *it_sc;

    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_SCIF_FINALIZE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_SCIF_FINALIZE);

    for (i = 0; i < MPID_nem_scif_nranks; ++i) {
        it_sc = &MPID_nem_scif_conns[i];
        if (it_sc->fd == -1) {
            continue;   /* no connection */
        }
        if (scif_close(it_sc->fd) == 0)
            it_sc->fd = -1;
    }
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_SCIF_FINALIZE);
    return MPI_SUCCESS;
}
