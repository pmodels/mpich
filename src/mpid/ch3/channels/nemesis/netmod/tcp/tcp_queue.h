/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef TCP_QUEUE_H_INCLUDED
#define TCP_QUEUE_H_INCLUDED

/* Generic queue macros -- "next_field" should be set to the name of
   the next pointer field in the element (e.g., "ch.tcp_sendq_next") */

#define PRINT_QUEUE(qp, next_field) do {        \
    } while(0)       
        

#define GENERIC_Q_EMPTY(q) ((q).head == NULL)

#define GENERIC_Q_HEAD(q) ((q).head)

#define GENERIC_Q_ENQUEUE_EMPTY(qp, ep, next_field) do {        \
        MPIR_Assert (GENERIC_Q_EMPTY (*(qp)));                  \
        (qp)->head = (qp)->tail = ep;                           \
        (ep)->next_field = NULL;                                \
        PRINT_QUEUE (qp, next_field);                           \
    } while (0)

#define GENERIC_Q_ENQUEUE(qp, ep, next_field) do {              \
        if (GENERIC_Q_EMPTY (*(qp)))                            \
            GENERIC_Q_ENQUEUE_EMPTY (qp, ep, next_field);       \
        else                                                    \
        {                                                       \
            (qp)->tail->next_field = (qp)->tail = ep;           \
            (ep)->next_field = NULL;                            \
        }                                                       \
        PRINT_QUEUE (qp, next_field);                           \
    } while (0)

/* the _MULTIPLE routines assume that ep0 is the head and ep1 is the
   tail of a linked list of elements.  The list is inserted on the end
   of the queue. */
#define GENERIC_Q_ENQUEUE_EMPTY_MULTIPLE(qp, ep0, ep1, next_field) do { \
        MPIR_Assert (GENERIC_Q_EMPTY (*(qp)));                          \
        (qp)->head = ep0;                                               \
        (qp)->tail = ep1;                                               \
        (ep1)->next_field = NULL;                                       \
    } while (0)

#define GENERIC_Q_ENQUEUE_MULTIPLE(qp, ep0, ep1, next_field) do {               \
        if (GENERIC_Q_EMPTY (*(qp)))                                            \
            GENERIC_Q_ENQUEUE_EMPTY_MULTIPLE (qp, ep0, ep1, next_field);        \
        else                                                                    \
        {                                                                       \
            (qp)->tail->next_field = ep0;                                       \
            (qp)->tail = ep1;                                                   \
            (ep1)->next_field = NULL;                                           \
        }                                                                       \
    } while (0)


#define GENERIC_Q_DEQUEUE(qp, epp, next_field) do {     \
        MPIR_Assert (!GENERIC_Q_EMPTY (*(qp)));         \
        *(epp) = (qp)->head;                            \
        (qp)->head = (*(epp))->next_field;              \
        if ((qp)->head == NULL)                         \
            (qp)->tail = NULL;                          \
    } while (0)

/* remove the elements from the top of the queue starting with ep0 through ep1 */
#define GENERIC_Q_REMOVE_ELEMENTS(qp, ep0, ep1, next_field) do {        \
        MPIR_Assert (GENERIC_Q_HEAD (*(qp)) == (ep0));                  \
        (qp)->head = (ep1)->next_field;                                 \
        if ((qp)->head == NULL)                                         \
            (qp)->tail = NULL;                                          \
    } while (0)



/* Generic list macros */
#define GENERIC_L_EMPTY(q) ((q).head == NULL)

#define GENERIC_L_HEAD(q) ((q).head)

#define GENERIC_L_ADD_EMPTY(qp, ep, next_field, prev_field) do {        \
        MPIR_Assert (GENERIC_L_EMPTY (*(qp)));                          \
        (qp)->head = ep;                                                \
        (ep)->next_field = (ep)->prev_field = NULL;                     \
    } while (0)

#define GENERIC_L_ADD(qp, ep, next_field, prev_field) do {              \
        if (GENERIC_L_EMPTY (*(qp)))                                    \
            GENERIC_L_ADD_EMPTY (qp, ep, next_field, prev_field);       \
        else                                                            \
        {                                                               \
            (ep)->prev_field = NULL;                                    \
            (ep)->next_field = (qp)->head;                              \
            (qp)->head->prev_field = ep;                                \
            (qp)->head = ep;                                            \
        }                                                               \
    } while (0)

#define GENERIC_L_REMOVE(qp, ep, next_field, prev_field) do {   \
        MPIR_Assert (!GENERIC_L_EMPTY (*(qp)));                 \
        if ((ep)->prev_field)                                   \
            ((ep)->prev_field)->next_field = (ep)->next_field;  \
        else                                                    \
            (qp)->head = (ep)->next_field;                      \
        if ((ep)->next_field)                                   \
            ((ep)->next_field)->prev_field  = (ep)->prev_field; \
    } while (0)


/* Generic stack macros */
#define GENERIC_S_EMPTY(s) ((s).top == NULL)

#define GENERIC_S_TOP(s) ((s).top)

#define GENERIC_S_PUSH(sp, ep, next_field) do { \
        (ep)->next_field = (sp)->top;           \
        (sp)->top = ep;                         \
    } while (0)

/* PUSH_MULTIPLE pushes a linked list of elements onto the stack.  It
   assumes that ep0 is the head of the linked list and ep1 is at the tail */
#define GENERIC_S_PUSH_MULTIPLE(sp, ep0, ep1, next_field) do {  \
        (ep1)->next_field = (sp)->top;                          \
        (sp)->top = ep0;                                        \
    } while (0)

#define GENERIC_S_POP(sp, ep, next_field) do {  \
        *(ep) = (sp)->top;                      \
        (sp)->top = (*(ep))->next_field;        \
    } while (0)
#endif /* TCP_QUEUE_H_INCLUDED */
