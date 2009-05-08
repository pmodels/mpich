/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpid_nem_impl.h"


#define USE_MPICH2

/* --BEGIN ERROR HANDLING-- */
#undef FUNCNAME
#define FUNCNAME MPID_nem_dump_cell_mpich
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
void MPID_nem_dump_cell_mpich ( MPID_nem_cell_ptr_t cell, int master)
{
    MPID_nem_pkt_mpich2_t *mpkt     = (MPID_nem_pkt_mpich2_t *)&(cell->pkt.mpich2); /* cast away volatile */
    int              *cell_buf = (int *)(mpkt->payload);
    int               mark;
    MPID_nem_cell_rel_ptr_t rel_cell;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_DUMP_CELL_MPICH);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_DUMP_CELL_MPICH);

    if (master)
	mark = 111;
    else
	mark = 777;

    rel_cell = MPID_NEM_ABS_TO_REL(cell);
    fprintf(stdout,"Cell[%i  @ %p (rel %p), next @ %p (rel %p)]\n ",
	    mark,
	    cell,
	    OPA_load_ptr(&rel_cell.p),
	    MPID_NEM_REL_TO_ABS(cell->next),
	    OPA_load_ptr(&cell->next.p) );

    fprintf(stdout,"%i  [Source:%i] [dest : %i] [dlen : %i] [seqno : %i]\n",
	    mark,
	    mpkt->source,
	    mpkt->dest,
	    mpkt->datalen,
	    mpkt->seqno);

    if (cell_buf[0] == 0)
    {
	fprintf(stdout,"%i [type: MPIDI_CH3_PKT_EAGER_SEND ] [tag : %i] [dsz : %i]\n",
		mark,
		cell_buf[1],
		cell_buf[4]);
	{
	    int index;
	    char *buf = (char *)cell_buf;

	    for (index = 0 ; index < 40 ; index++)
		fprintf(stdout," -- %i ",buf[index]);
	    fprintf(stdout,"\n");
	}


    }
    else if (cell_buf[0] == 5)
	fprintf(stdout,"%i [type: MPIDI_CH3_PKT_RNDv_CLR_TO_SEND ] [tag : %i] [dsz : %i]\n",
		mark,
		cell_buf[1],
		cell_buf[4]);

    else
	fprintf(stdout,"%i [type:%i]\n", mark, cell_buf[0] );

    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_DUMP_CELL_MPICH);
}
/* --END ERROR HANDLING-- */

/* --BEGIN ERROR HANDLING-- */
/*inline */
#undef FUNCNAME
#define FUNCNAME MPID_nem_dump_cell_mpich2__
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
void MPID_nem_dump_cell_mpich2__ ( MPID_nem_cell_ptr_t cell, int master, char *file, int line)
{
    int mark;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_DUMP_CELL_MPICH2__);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_DUMP_CELL_MPICH2__);

    if (master)
        mark = 111;
    else
        mark = 777;

    fprintf(stderr,"%i called from file %s at line %i \n",mark,file,line);

    MPID_nem_dump_cell_mpich(cell,master);

    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_DUMP_CELL_MPICH2__);
}
/* --END ERROR HANDLING-- */

