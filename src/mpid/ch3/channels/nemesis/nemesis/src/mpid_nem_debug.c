/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpid_nem_impl.h"
#include "mpid_nem_debug.h"

/* --BEGIN ERROR HANDLING-- */
void
MPID_nem_dbg_dump_cell (volatile struct MPID_nem_cell *cell)
{
    MPIU_DBG_MSG_D (ALL, TERSE, "  src = %6d", cell->pkt.mpich2.source);
    MPIU_DBG_MSG_D (ALL, TERSE, "  dst = %6d", cell->pkt.mpich2.dest);
    MPIU_DBG_MSG_D (ALL, TERSE, "  len = %6d", cell->pkt.mpich2.datalen);
    MPIU_DBG_MSG_D (ALL, TERSE, "  sqn = %6d", cell->pkt.mpich2.seqno);
    MPIU_DBG_MSG_D (ALL, TERSE, "  typ = %6d", cell->pkt.mpich2.type);
}
/* --END ERROR HANDLING-- */
