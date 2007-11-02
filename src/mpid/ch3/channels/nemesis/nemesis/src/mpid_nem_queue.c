/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpid_nem_impl.h"


#define USE_MPICH2

inline void MPID_nem_cell_init( MPID_nem_cell_ptr_t cell)
{
    MPID_NEM_SET_REL_NULL (cell->next);
    memset ((void *)&cell->pkt, 0, sizeof (MPID_nem_pkt_header_t));
}

/* inline void MPID_nem_rel_cell_init( MPID_nem_cell_ptr_t cell) */
/* { */
/*   cell->next =  MPID_NEM_REL_NULL ; */
/*   memset (&cell->pkt, 0, 4*1024); */
/*   /\*  memset (&cell->pkt, 0, sizeof (MPID_nem_pkt_header_t)); *\/ */
/* } */

/* --BEGIN ERROR HANDLING-- */
inline void MPID_nem_dump_cell_mpich ( MPID_nem_cell_ptr_t cell, int master)
{
    MPID_nem_pkt_mpich2_t *mpkt     = (MPID_nem_pkt_mpich2_t *)&(cell->pkt.mpich2); /* cast away volatile */
    int              *cell_buf = (int *)(mpkt->payload);
    int               mark;

    if (master)
	mark = 111;
    else 
	mark = 777;

  
    fprintf(stdout,"Cell[%i  @ %p (rel %p), next @ %p (rel %p)]\n ",
	    mark,
	    cell,
	    MPID_NEM_ABS_TO_REL (cell).p, 
	    MPID_NEM_REL_TO_ABS (cell->next),
	    cell->next.p );
  
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

}
/* --END ERROR HANDLING-- */

/* --BEGIN ERROR HANDLING-- */
/*inline */
void MPID_nem_dump_cell_mpich2__ ( MPID_nem_cell_ptr_t cell, int master, char *file, int line)
{
    int mark;

    if (master)
        mark = 111;
    else
        mark = 777;
 
    fprintf(stderr,"%i called from file %s at line %i \n",mark,file,line);

    MPID_nem_dump_cell_mpich(cell,master); 
}
/* --END ERROR HANDLING-- */

/* inline void MPID_nem_rel_queue_init (MPID_nem_queue_ptr_t rel_qhead ) */
/* { */
/*     MPID_nem_queue_ptr_t qhead = MPID_NEM_REL_TO_ABS( rel_qhead ); */
/*     qhead->head    = MPID_NEM_REL_NULL; */
/*     qhead->my_head = MPID_NEM_REL_NULL; */
/*     qhead->tail    = MPID_NEM_REL_NULL; */
/* } */

inline void MPID_nem_queue_init (MPID_nem_queue_ptr_t qhead )
{
  MPID_NEM_SET_REL_NULL (qhead->head);
  MPID_NEM_SET_REL_NULL (qhead->my_head);
  MPID_NEM_SET_REL_NULL (qhead->tail);
}

