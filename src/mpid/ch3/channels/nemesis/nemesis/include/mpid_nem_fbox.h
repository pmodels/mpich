/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MPID_NEM_FBOX_H
#define MPID_NEM_FBOX_H

typedef struct MPID_nem_fboxq_elem
{
  int usage;
  struct MPID_nem_fboxq_elem *prev;
  struct MPID_nem_fboxq_elem *next;
  int grank;
  MPID_nem_fbox_mpich2_t *fbox;
} MPID_nem_fboxq_elem_t ;

extern MPID_nem_fboxq_elem_t *MPID_nem_fboxq_head;
extern MPID_nem_fboxq_elem_t *MPID_nem_fboxq_tail;
extern MPID_nem_fboxq_elem_t *MPID_nem_fboxq_elem_list;
extern MPID_nem_fboxq_elem_t *MPID_nem_fboxq_elem_list_last;
extern MPID_nem_fboxq_elem_t *MPID_nem_curr_fboxq_elem;
extern MPID_nem_fboxq_elem_t *MPID_nem_curr_fbox_all_poll;
extern unsigned short        *MPID_nem_recv_seqno;
/* int    MPID_nem_mpich2_dequeue_fastbox (int local_rank); */
/* int    MPID_nem_mpich2_enqueue_fastbox (int local_rank); */

static inline int poll_active_fboxes(MPID_nem_cell_ptr_t *cell)
{
    MPID_nem_fboxq_elem_t *orig_fboxq_elem;
    int found = FALSE;

    if (MPID_nem_fboxq_head != NULL)
    {
        orig_fboxq_elem = MPID_nem_curr_fboxq_elem;
        do
        {
            MPID_nem_fbox_mpich2_t *fbox;

            fbox = MPID_nem_curr_fboxq_elem->fbox;
            MPIU_Assert(fbox != NULL);
            if (fbox->flag.value == 1 &&
                fbox->cell.pkt.mpich2.seqno == MPID_nem_recv_seqno[MPID_nem_curr_fboxq_elem->grank])
            {
                ++MPID_nem_recv_seqno[MPID_nem_curr_fboxq_elem->grank];
                *cell = &fbox->cell;
                found = TRUE;
                goto fn_exit;
            }
            MPID_nem_curr_fboxq_elem = MPID_nem_curr_fboxq_elem->next;
            if (MPID_nem_curr_fboxq_elem == NULL)
                MPID_nem_curr_fboxq_elem = MPID_nem_fboxq_head;
        }
        while (MPID_nem_curr_fboxq_elem != orig_fboxq_elem);
    }
    *cell = NULL;

fn_exit:
    return found;
}

static inline int poll_every_fbox(MPID_nem_cell_ptr_t *cell)
{
    MPID_nem_fboxq_elem_t *orig_fbox_el = MPID_nem_curr_fbox_all_poll;
    MPID_nem_fbox_mpich2_t *fbox;
    int found = FALSE;

    do {
        fbox = MPID_nem_curr_fbox_all_poll->fbox;
        if (fbox && fbox->flag.value == 1 &&
            fbox->cell.pkt.mpich2.seqno == MPID_nem_recv_seqno[MPID_nem_curr_fbox_all_poll->grank]) {
            ++MPID_nem_recv_seqno[MPID_nem_curr_fbox_all_poll->grank];
            *cell = &fbox->cell;
            found = TRUE;
            break;
        }
        ++MPID_nem_curr_fbox_all_poll;
        if (MPID_nem_curr_fbox_all_poll > MPID_nem_fboxq_elem_list_last)
            MPID_nem_curr_fbox_all_poll = MPID_nem_fboxq_elem_list;
    } while (MPID_nem_curr_fbox_all_poll != orig_fbox_el);

    return found;
}

     
#define poll_next_fbox(_cell, do_found) do {										\
    MPID_nem_fbox_mpich2_t *fbox;													\
															\
    fbox = MPID_nem_curr_fbox_all_poll->fbox;											\
    if (fbox && fbox->flag.value == 1 && fbox->cell.pkt.mpich2.seqno == MPID_nem_recv_seqno[MPID_nem_curr_fbox_all_poll->grank])	\
    {															\
	++MPID_nem_recv_seqno[MPID_nem_curr_fbox_all_poll->grank];									\
	*(_cell) = &fbox->cell;												\
	do_found;													\
    }															\
    ++MPID_nem_curr_fbox_all_poll;												\
    if (MPID_nem_curr_fbox_all_poll > MPID_nem_fboxq_elem_list_last)									\
	MPID_nem_curr_fbox_all_poll = MPID_nem_fboxq_elem_list;										\
} while(0)

#endif /* MPID_NEM_FBOX_H */


