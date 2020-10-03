/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPID_NEM_FBOX_H_INCLUDED
#define MPID_NEM_FBOX_H_INCLUDED

typedef struct MPID_nem_fboxq_elem
{
  int usage;
  struct MPID_nem_fboxq_elem *prev;
  struct MPID_nem_fboxq_elem *next;
  int grank;
  MPID_nem_fastbox_t *fbox;
} MPID_nem_fboxq_elem_t ;

extern MPID_nem_fboxq_elem_t *MPID_nem_fboxq_head;
extern MPID_nem_fboxq_elem_t *MPID_nem_fboxq_tail;
extern MPID_nem_fboxq_elem_t *MPID_nem_fboxq_elem_list;
extern MPID_nem_fboxq_elem_t *MPID_nem_fboxq_elem_list_last;
extern MPID_nem_fboxq_elem_t *MPID_nem_curr_fboxq_elem;
extern MPID_nem_fboxq_elem_t *MPID_nem_curr_fbox_all_poll;
extern unsigned short        *MPID_nem_recv_seqno;
/* int    MPID_nem_mpich_dequeue_fastbox (int local_rank); */
/* int    MPID_nem_mpich_enqueue_fastbox (int local_rank); */

static inline int poll_active_fboxes(MPID_nem_cell_ptr_t *cell)
{
    MPID_nem_fboxq_elem_t *orig_fboxq_elem;
    int found = FALSE;

    if (MPID_nem_fboxq_head != NULL)
    {
        orig_fboxq_elem = MPID_nem_curr_fboxq_elem;
        do
        {
            MPID_nem_fastbox_t *fbox;
            MPID_nem_cell_t *cell_ptr;

            fbox = MPID_nem_curr_fboxq_elem->fbox;
            MPIR_Assert(fbox != NULL);
            cell_ptr = MPID_NEM_FBOX_TO_CELL(fbox);
            if (MPL_atomic_acquire_load_int(&fbox->flag) &&
                cell_ptr->header.seqno == MPID_nem_recv_seqno[MPID_nem_curr_fboxq_elem->grank])
            {
                ++MPID_nem_recv_seqno[MPID_nem_curr_fboxq_elem->grank];
                *cell = cell_ptr;
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
    MPID_nem_fastbox_t *fbox;
    MPID_nem_cell_t *cell_ptr;
    int found = FALSE;

    do {
        fbox = MPID_nem_curr_fbox_all_poll->fbox;
        cell_ptr = MPID_NEM_FBOX_TO_CELL(fbox);
        if (fbox && MPL_atomic_acquire_load_int(&fbox->flag) &&
            cell_ptr->header.seqno == MPID_nem_recv_seqno[MPID_nem_curr_fbox_all_poll->grank]) {
            ++MPID_nem_recv_seqno[MPID_nem_curr_fbox_all_poll->grank];
            *cell = cell_ptr;
            found = TRUE;
            break;
        }
        ++MPID_nem_curr_fbox_all_poll;
        if (MPID_nem_curr_fbox_all_poll > MPID_nem_fboxq_elem_list_last)
            MPID_nem_curr_fbox_all_poll = MPID_nem_fboxq_elem_list;
    } while (MPID_nem_curr_fbox_all_poll != orig_fbox_el);

    return found;
}


#define poll_next_fbox(_cell, do_found)                                                             \
    do {                                                                                            \
        MPID_nem_fastbox_t *fbox;                                                               \
        MPID_nem_cell_t *cell_ptr;                                                                  \
                                                                                                    \
        fbox = MPID_nem_curr_fbox_all_poll->fbox;                                                   \
        cell_ptr = MPID_NEM_FBOX_TO_CELL(fbox);                                                     \
        if (fbox && MPL_atomic_acquire_load_int(&fbox->flag) &&                               \
            cell_ptr->header.seqno == MPID_nem_recv_seqno[MPID_nem_curr_fbox_all_poll->grank])      \
        {                                                                                           \
            ++MPID_nem_recv_seqno[MPID_nem_curr_fbox_all_poll->grank];                              \
            *(_cell) = cell_ptr;                                                                    \
            do_found;                                                                               \
        }                                                                                           \
        ++MPID_nem_curr_fbox_all_poll;                                                              \
        if (MPID_nem_curr_fbox_all_poll > MPID_nem_fboxq_elem_list_last)                            \
        MPID_nem_curr_fbox_all_poll = MPID_nem_fboxq_elem_list;                                     \
    } while(0)

#endif /* MPID_NEM_FBOX_H_INCLUDED */


