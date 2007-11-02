/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MPID_NEM_QUEUE_H
#define MPID_NEM_QUEUE_H
#include "mpid_nem_datatypes.h"
#include "mpid_nem_defs.h"
#include "mpid_nem_atomics.h"

/*#define PAPI_MONITOR */
/* include "my_papi_defs.h" */

#define MPID_nem_dump_cell_mpich2(cell, index)  MPID_nem_dump_cell_mpich2__((cell),(index),__FILE__,__LINE__) 

void MPID_nem_dump_cell_mpich2__ ( MPID_nem_cell_ptr_t cell, int, char* ,int);
inline void   MPID_nem_dump_cell_mpich ( MPID_nem_cell_ptr_t cell, int);

inline void MPID_nem_cell_init( MPID_nem_cell_ptr_t cell);
static inline void MPID_nem_dump_queue( MPID_nem_queue_ptr_t q) MPIU_Assertp (0) /* not implemented */
inline void MPID_nem_queue_init( MPID_nem_queue_ptr_t );
int MPID_nem_network_poll (MPID_nem_poll_dir_t in_or_out);

/* inline void MPID_nem_rel_cell_init( MPID_nem_cell_ptr_t cell); */
/* static inline void MPID_nem_rel_dump_queue( MPID_nem_queue_ptr_t q){printf ("dump queue not implemented\n"); exit (-1);}; */
/* inline void MPID_nem_rel_queue_init( MPID_nem_queue_ptr_t ); */
/* void MPID_nem_rel_network_poll (MPID_nem_poll_dir_t in_or_out); */

#define MPID_NEM_USE_SHADOW_HEAD
#define MPID_NEM_USE_MACROS

/* MPID_NEM_SWAP_REL and MPID_NEM_CAS_REL_NULL are swap and compare-and-swap functions with strict type-checking */
static inline MPID_nem_cell_rel_ptr_t MPID_NEM_SWAP_REL (volatile MPID_nem_cell_rel_ptr_t *ptr, MPID_nem_cell_rel_ptr_t val)
{
    MPID_nem_cell_rel_ptr_t ret;
    ret.p = MPID_NEM_SWAP (&(ptr->p), val.p);
    return ret;
}

/* do a compare-and-swap with MPID_NEM_RELNULL */
static inline MPID_nem_cell_rel_ptr_t MPID_NEM_CAS_REL_NULL (volatile MPID_nem_cell_rel_ptr_t *ptr, MPID_nem_cell_rel_ptr_t oldv)
{
    MPID_nem_cell_rel_ptr_t ret;
    ret.p = MPID_NEM_CAS (&(ptr->p), oldv.p, MPID_NEM_REL_NULL);
    return ret;
}

#ifndef MPID_NEM_USE_MACROS
static inline void
MPID_nem_queue_enqueue (MPID_nem_queue_ptr_t qhead, MPID_nem_cell_ptr_t element)
{
    MPID_nem_cell_rel_ptr_t r_prev;
    MPID_nem_cell_rel_ptr_t r_element = MPID_NEM_ABS_TO_REL (element);

    r_prev = MPID_NEM_SWAP_REL (&(qhead->tail), r_element);

    if (MPID_NEM_IS_REL_NULL (r_prev))
    {
	qhead->head = r_element;
    }
    else
    {
	MPID_NEM_REL_TO_ABS (r_prev)->next = r_element;
    }
}
#else /*MPID_NEM_USE_MACROS */
#define MPID_nem_queue_enqueue(qhead, element) do {			\
    MPID_nem_cell_rel_ptr_t r_prev;					\
    MPID_nem_cell_rel_ptr_t r_element = MPID_NEM_ABS_TO_REL (element);	\
									\
    r_prev = MPID_NEM_SWAP_REL (&((qhead)->tail), r_element);		\
									\
    if (MPID_NEM_IS_REL_NULL (r_prev))					\
    {									\
	(qhead)->head = r_element;					\
    }									\
    else								\
    {									\
	MPID_NEM_REL_TO_ABS (r_prev)->next = r_element;			\
    }									\
} while (0)
#endif /*MPID_NEM_USE_MACROS */

/* #ifndef MPID_NEM_USE_MACROS */
/* static inline void */
/* MPID_nem_rel_queue_enqueue (MPID_nem_queue_ptr_t rel_qhead, MPID_nem_cell_ptr_t element) */
/* { */
/*     MPID_nem_queue_ptr_t qhead = MPID_NEM_REL_TO_ABS( rel_qhead ); */
/*     MPID_nem_cell_ptr_t   prev; */

/*     prev = MPID_NEM_SWAP (&(qhead->tail), element); */

/*     if (prev == MPID_NEM_REL_NULL) */
/*     { */
/* 	qhead->head = element; */
/*     } */
/*     else */
/*     { */
/*         (MPID_NEM_REL_TO_ABS(prev))->next = element; */
/*     } */
/* } */
/* #else /\*MPID_NEM_USE_MACROS *\/ */
/* #define MPID_nem_rel_queue_enqueue(rel_qhead, element) do {	\ */
/*     MPID_nem_queue_ptr_t qhead =				\ */
/* 	MPID_NEM_REL_TO_ABS((MPID_nem_queue_ptr_t)rel_qhead );	\ */
/*     MPID_nem_cell_ptr_t prev;					\ */
/* 								\ */
/*     prev = MPID_NEM_SWAP (&((qhead)->tail), (element));		\ */
/*     if (prev == MPID_NEM_REL_NULL)	\ */
/*     {								\ */
/* 	(qhead)->head = (element);				\ */
/*     }								\ */
/*     else							\ */
/*     {								\ */
/*         (MPID_NEM_REL_TO_ABS(prev))->next = (element);		\ */
/*     }								\ */
/* }while (0) */
/* #endif /\*MPID_NEM_USE_MACROS *\/ */

#ifndef MPID_NEM_USE_MACROS
static inline int
MPID_nem_queue_empty (MPID_nem_queue_ptr_t qhead)
{
#ifdef MPID_NEM_USE_SHADOW_HEAD
    if (MPID_NEM_IS_REL_NULL (qhead->my_head))
    {
	if (MPID_NEM_IS_REL_NULL (qhead->head))
	    return 1;
	else
	{
	    qhead->my_head = qhead->head;
	    MPID_NEM_SET_REL_NULL (qhead->head); /* reset it for next time */
	}
    }
    return 0;
#else
    return qhead->head == NULL;
#endif
}

#else /*MPID_NEM_USE_MACROS */
#ifdef MPID_NEM_USE_SHADOW_HEAD
#define MPID_nem_queue_empty(qhead) ({						\
    int __ret = 0;								\
    if (MPID_NEM_IS_REL_NULL (qhead->my_head))					\
    {										\
	if (MPID_NEM_IS_REL_NULL (qhead->head))					\
	    __ret = 1;								\
	else									\
	{									\
	    qhead->my_head = qhead->head;					\
	    MPID_NEM_SET_REL_NULL (qhead->head); /* reset it for next time */	\
	}									\
    }										\
    __ret;									\
})

#else
#define MPID_nem_queue_empty(qhead) (MPID_NEM_IS_REL_NULL (qhead)->head)
#endif /* MPID_NEM_USE_SHADOW_HEAD */
#endif /* MPID_NEM_USE_MACROS */


/* #ifndef MPID_NEM_USE_MACROS */
/* static inline int */
/* MPID_nem_rel_queue_empty ( MPID_nem_queue_ptr_t rel_qhead ) */
/* {      */
/*     MPID_nem_queue_ptr_t  qhead =  MPID_NEM_REL_TO_ABS( rel_qhead ); */

/* #ifdef MPID_NEM_USE_SHADOW_HEAD */
/*     if (qhead->my_head == MPID_NEM_REL_NULL) */
/*     { */
/* 	if (qhead->head == MPID_NEM_REL_NULL) */
/* 	    return 1; */
/* 	else */
/* 	{ */
/* 	    qhead->my_head = qhead->head; */
/* 	    qhead->head = MPID_NEM_REL_NULL; /\* reset it for next time *\/ */
/* 	} */
/*     } */
/*     return 0; */
/* #else /\*  MPID_NEM_USE_SHADOW_HEAD *\/ */
/*     return qhead->head == MPID_NEM_REL_NULL; */
/* #endif /\*   MPID_NEM_USE_SHADOW_HEAD *\/ */
/* } */
/* #else /\*MPID_NEM_USE_MACROS *\/ */
/* #ifdef MPID_NEM_USE_SHADOW_HEAD */
/* #define MPID_nem_rel_queue_empty(rel_qhead) ({                                 \ */
/*     MPID_nem_queue_ptr_t  qhead =  MPID_NEM_REL_TO_ABS( (MPID_nem_queue_ptr_t)rel_qhead ); \ */
/*     int             __ret = 0;			                      \ */
/*     if (qhead->my_head == MPID_NEM_REL_NULL)                \ */
/*     {						                      \ */
/* 	if (qhead->head == MPID_NEM_REL_NULL)               \ */
/* 	    __ret = 1;				                      \ */
/* 	else					                      \ */
/*         {					                      \ */
/* 	    qhead->my_head = qhead->head;	                      \ */
/* 	    qhead->head = MPID_NEM_REL_NULL;                \ */
/*         }					                      \ */
/*     }						                      \ */
/* 						                      \ */
/*     __ret;					                      \ */
/* }) */
/* #else /\*MPID_NEM_USE_SHADOW_HEAD *\/ */
/* #define MPID_nem_rel_queue_empty(qhead) ((MPID_NEM_REL_TO_ABS((qhead)))->head == MPID_NEM_REL_NULL) */
/* #endif /\* MPID_NEM_USE_SHADOW_HEAD *\/ */
/* #endif /\* MPID_NEM_USE_MACROS *\/ */

#ifndef MPID_NEM_USE_MACROS
/* Gets the head */
static inline void
MPID_nem_queue_dequeue (MPID_nem_queue_ptr_t qhead, MPID_nem_cell_ptr_t *e)
{
    register MPID_nem_cell_ptr_t _e;
    register MPID_nem_cell_rel_ptr_t _r_e;
    /*DO_PAPI (if (iter11) PAPI_accum_var (PAPI_EventSet, PAPI_vvalues11)); */
    /*    DO_PAPI (PAPI_reset (PAPI_EventSet)); */

#ifdef MPID_NEM_USE_SHADOW_HEAD
    _r_e = qhead->my_head;
#else /*SHADOW_HEAD */
    _r_e = qhead->head;
#endif /*SHADOWHEAD */

    _e = MPID_NEM_REL_TO_ABS (_r_e);
    if (!MPID_NEM_IS_REL_NULL(_e->next))
    {
#ifdef MPID_NEM_USE_SHADOW_HEAD
	qhead->my_head = _e->next;
#else /*MPID_NEM_USE_SHADOW_HEAD */
	qhead->head = _e->next;
#endif /*MPID_NEM_USE_SHADOW_HEAD */
    }
    else
    {
	MPID_nem_cell_rel_ptr_t old_tail;
      
#ifdef MPID_NEM_USE_SHADOW_HEAD
	MPID_NEM_SET_REL_NULL (qhead->my_head);	
#else /*MPID_NEM_USE_SHADOW_HEAD */
	MPID_NEM_SET_REL_NULL (qhead->head);	
#endif /*MPID_NEM_USE_SHADOW_HEAD */

	old_tail = MPID_NEM_CAS_REL_NULL (&(qhead->tail), _r_e);

	if (!MPID_NEM_REL_ARE_EQUAL (old_tail, _r_e))
	{
	    while (MPID_NEM_IS_REL_NULL (_e->next))
	    {
		SKIP;
	    }
#ifdef MPID_NEM_USE_SHADOW_HEAD
	    qhead->my_head = _e->next;
#else /*MPID_NEM_USE_SHADOW_HEAD */
	    qhead->head = _e->next;
#endif /*MPID_NEM_USE_SHADOW_HEAD */
	}
    }
    MPID_NEM_SET_REL_NULL (_e->next);
    *e = _e;
    /*    DO_PAPI (if (iter11) PAPI_accum_var (PAPI_EventSet, PAPI_vvalues11)); */
}

#else /* MPID_NEM_USE_MACROS */
#ifdef MPID_NEM_USE_SHADOW_HEAD
#define MPID_nem_queue_dequeue(qhead, e) do {				\
    register MPID_nem_cell_ptr_t _e;					\
    register MPID_nem_cell_rel_ptr_t _r_e;				\
									\
    _r_e = (qhead)->my_head;						\
    _e = MPID_NEM_REL_TO_ABS (_r_e);					\
									\
    if (!MPID_NEM_IS_REL_NULL (_e->next))				\
    {									\
	(qhead)->my_head = _e->next;					\
    }									\
    else								\
    {									\
	MPID_nem_cell_rel_ptr_t old_tail;				\
									\
	MPID_NEM_SET_REL_NULL ((qhead)->my_head);			\
									\
	old_tail = MPID_NEM_CAS_REL_NULL (&((qhead)->tail), _r_e);	\
									\
	if (!MPID_NEM_REL_ARE_EQUAL (old_tail, _r_e))			\
	{								\
	    while (MPID_NEM_IS_REL_NULL (_e->next))			\
	    {								\
		SKIP;							\
	    }								\
	    (qhead)->my_head = _e->next;				\
	}								\
    }									\
    MPID_NEM_SET_REL_NULL (_e->next);					\
    *(e) = _e;								\
} while (0)                                             
#else /* MPID_NEM_USE_SHADOW_HEAD */
#define MPID_nem_queue_dequeue(qhead, e) do {				\
    register MPID_nem_cell_ptr_t _e;					\
    register MPID_nem_cell_rel_ptr_t _r_e;				\
									\
    _r_e = (qhead)->head;						\
									\
    _e = MPID_NEM_REL_TO_ABS (_r_e);					\
    if (!MPID_NEM_IS_REL_NULL (_e->next))				\
    {									\
	(qhead)->head = _e->next;					\
    }									\
    else								\
    {									\
	MPID_nem_cell_rel_ptr_t old_tail;				\
									\
	MPID_NEM_SET_REL_NULL ((qhead)->head);				\
									\
	old_tail = MPID_NEM_CAS_REL_NULL (&((qhead)->tail), _r_e);	\
									\
	if (!MPID_NEM_REL_ARE_EQUAL (old_tail, _r_e))			\
	{								\
	    while (MPID_NEM_IS_REL_NULL (_e->next))			\
	    {								\
		SKIP;							\
	    }								\
	    (qhead)->head = _e->next;					\
	}								\
    }									\
    MPID_NEM_SET_REL_NULL (_e->next);					\
    *(e) = _e;								\
} while (0)                                             

#endif /* MPID_NEM_USE_SHADOW_HEAD */
#endif /* MPID_NEM_USE_MACROS */


/* #ifndef MPID_NEM_USE_MACROS */
/* /\* Gets the head *\/ */
/* static inline void */
/* MPID_nem_rel_queue_dequeue (MPID_nem_queue_ptr_t rel_qhead, MPID_nem_cell_ptr_t *e) */
/* { */
/*     MPID_nem_queue_ptr_t  qhead = MPID_NEM_REL_TO_ABS( rel_qhead ); */
/*     register MPID_nem_cell_ptr_t _e;     */
/*     register MPID_nem_cell_ptr_t _e_abs; */
    
/*     /\*DO_PAPI (if (iter11) PAPI_accum_var (PAPI_EventSet, PAPI_vvalues11)); *\/ */
/*     /\*    DO_PAPI (PAPI_reset (PAPI_EventSet)); *\/ */

/* #ifdef MPID_NEM_USE_SHADOW_HEAD */
/*     (_e) = qhead->my_head; */
/* #else /\*SHADOW_HEAD *\/ */
/*     (_e) = qhead->head ; */
/* #endif /\*SHADOWHEAD *\/ */

/*     (_e_abs) = MPID_NEM_REL_TO_ABS( (_e) ); */
/*     if ((_e_abs)->next != MPID_NEM_REL_NULL) */
/*     { */
/* #ifdef MPID_NEM_USE_SHADOW_HEAD */
/* 	qhead->my_head = (_e_abs)->next; */
/* #else /\*MPID_NEM_USE_SHADOW_HEAD *\/ */
/* 	qhead->head     = (_e_abs)->next; */
/* #endif /\*MPID_NEM_USE_SHADOW_HEAD *\/ */
/*     } */
/*     else */
/*     { */
/* 	MPID_nem_cell_ptr_t old_tail; */
      
/* #ifdef MPID_NEM_USE_SHADOW_HEAD */
/* 	qhead->my_head = MPID_NEM_REL_NULL;	 */
/* #else /\*MPID_NEM_USE_SHADOW_HEAD *\/ */
/* 	qhead->head = MPID_NEM_REL_NULL;	 */
/* #endif /\*MPID_NEM_USE_SHADOW_HEAD *\/ */

/* 	old_tail = MPID_NEM_CAS (&(qhead->tail), (_e), MPID_NEM_REL_NULL); */

/* 	if (old_tail != (_e)) */
/* 	{ */
/* 	    while ((_e_abs)->next == MPID_NEM_REL_NULL) */
/* 	    { */
/* 		SKIP; */
/* 	    } */
/* #ifdef MPID_NEM_USE_SHADOW_HEAD */
/* 	    qhead->my_head  = (_e_abs)->next; */
/* #else /\*MPID_NEM_USE_SHADOW_HEAD *\/ */
/* 	    qhead->head  = (_e_abs)->next; */
/* #endif /\*MPID_NEM_USE_SHADOW_HEAD *\/ */
/* 	} */
/*     } */
/*     (_e_abs)->next   = MPID_NEM_REL_NULL; */
/*     *e = _e; */
/*     /\*    DO_PAPI (if (iter11) PAPI_accum_var (PAPI_EventSet, PAPI_vvalues11)); *\/ */
/* } */

/* #else /\* MPID_NEM_USE_MACROS *\/ */
/* #ifdef MPID_NEM_USE_SHADOW_HEAD */
/* #define MPID_nem_rel_queue_dequeue(rel_qhead, e) do {					\ */
/*     MPID_nem_queue_ptr_t  qhead = MPID_NEM_REL_TO_ABS( (MPID_nem_queue_ptr_t)rel_qhead );            \ */
/*     register MPID_nem_cell_ptr_t _e;							\ */
/*     register MPID_nem_cell_ptr_t _e_abs;		       				\ */
/*     /\*    DO_PAPI (PAPI_reset (PAPI_EventSet));	*\/				\ */
/*     (_e)     = (qhead)->my_head;		     				\ */
/*     (_e_abs) = MPID_NEM_REL_TO_ABS( (_e) );                                              \ */
/*     if ((_e_abs)->next != MPID_NEM_REL_NULL)				\ */
/*     {										\ */
/* 	(qhead)->my_head  = (_e_abs)->next;			      		\ */
/*     }										\ */
/*     else									\ */
/*     {										\ */
/* 	MPID_nem_cell_ptr_t old_tail;							\ */
/* 										\ */
/* 	(qhead)->my_head  = MPID_NEM_REL_NULL;			\ */
/* 	old_tail = MPID_NEM_CAS (&((qhead)->tail), (_e), MPID_NEM_REL_NULL); 	\ */
/* 	if (old_tail != (_e))							\ */
/* 	{									\ */
/* 	    while ((_e_abs)->next == MPID_NEM_REL_NULL)		\ */
/* 	    {									\ */
/* 		SKIP;								\ */
/* 	    }									\ */
/* 	    (qhead)->my_head = (_e_abs)->next;					\ */
/* 	}									\ */
/*     }										\ */
/* 										\ */
/*     (_e_abs)->next   = MPID_NEM_REL_NULL;		      		\ */
/*     *(e) = _e;									\ */
/*     /\*    DO_PAPI (if (iter11) PAPI_accum (PAPI_EventSet, PAPI_values11));*\/	\ */
/*     /\*DO_PAPI (if (iter11) PAPI_accum (PAPI_EventSet, PAPI_values11)); *\/	\ */
/* } while (0)                                              */
/* #else /\* MPID_NEM_USE_SHADOW_HEAD *\/ */
/* #define MPID_nem_rel_queue_dequeue(rel_qhead, e) do {				        \ */
/*     MPID_nem_queue_ptr_t  qhead = MPID_NEM_REL_TO_ABS( (MPID_nem_queue_ptr_t)rel_qhead );            \ */
/*     register MPID_nem_cell_ptr_t _e;                                                   \ */
/*     register MPID_nem_cell_ptr_t _e_abs;		       		      	        \ */
/*     /\*    DO_PAPI (PAPI_reset (PAPI_EventSet));	*\/				\ */
/*     (_e)     = (qhead)->head;                                                   \ */
/*     (_e_abs) = MPID_NEM_REL_TO_ABS( (_e) );                                              \ */
/*     if ((_e_abs)->next != MPID_NEM_REL_NULL)				\ */
/*     {										\ */
/* 	(qhead)->head  = (_e_abs)->next;				      	\ */
/*     }										\ */
/*     else									\ */
/*     {										\ */
/* 	MPID_nem_cell_ptr_t old_tail;							\ */
/* 										\ */
/* 	(qhead)->head  = MPID_NEM_REL_NULL;	         	  	\ */
/* 	old_tail = MPID_NEM_CAS (&((qhead)->tail), (_e), MPID_NEM_REL_NULL);   \ */
/* 	if (old_tail != (_e))							\ */
/* 	{									\ */
/* 	    while ((_e_abs)->next == MPID_NEM_REL_NULL)     		\ */
/* 	    {									\ */
/* 		SKIP;								\ */
/* 	    }									\ */
/* 	    (qhead)->head = (_e_abs)->next;		      			\ */
/* 	}									\ */
/*     }										\ */
/* 										\ */
/*     (_e_abs)->next   = MPID_NEM_REL_NULL;			      	\ */
/*     *(e) = _e;									\ */
/*     /\*    DO_PAPI (if (iter11) PAPI_accum (PAPI_EventSet, PAPI_values11));*\/	\ */
/*     /\*DO_PAPI (if (iter11) PAPI_accum (PAPI_EventSet, PAPI_values11)); *\/	\ */
/* } while (0)                                              */

/* #endif /\* MPID_NEM_USE_SHADOW_HEAD *\/ */
/* #endif /\* MPID_NEM_USE_MACROS *\/ */

#if 0 /* Where are these functions used? */
#ifdef MPID_NEM_USE_SHADOW_HEAD
#undef FUNCNAME
#define FUNCNAME MPID_nem_queue_poll
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline 
int MPID_nem_queue_poll (MPID_nem_queue_ptr_t qhead, MPID_nem_poll_dir_t in_or_out)
{
    int mpi_errno = MPI_SUCCESS;
    
    mpi_errno = MPID_nem_network_poll (in_or_out);
    if (mpi_errno) MPIU_ERR_POP (mpi_errno);
    if (MPID_NEM_IS_REL_NULL (qhead->my_head))
    {
	while (MPID_NEM_IS_REL_NULL (qhead->head))
	{
	    mpi_errno = MPID_nem_network_poll (in_or_out);
            if (mpi_errno) MPIU_ERR_POP (mpi_errno);
	    sched_yield();
	}
	qhead->my_head = qhead->head;
	MPID_NEM_SET_REL_NULL (qhead->head);
    }
 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}
#else 
#undef FUNCNAME
#define FUNCNAME MPID_nem_queue_poll
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline int MPID_nem_queue_poll (MPID_nem_queue_ptr_t qhead, MPID_nem_poll_dir_t in_or_out)
{
    int mpi_errno = MPI_SUCCESS;
    
    mpi_errno = MPID_nem_network_poll (in_or_out);    
    if (mpi_errno) MPIU_ERR_POP (mpi_errno);
    while (MPID_NEM_IS_REL_NULL (qhead->head))
    {
	mpi_errno = MPID_nem_network_poll (in_or_out);
        if (mpi_errno) MPIU_ERR_POP (mpi_errno);
	sched_yield();
    }
    
 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}
#endif/* MPID_NEM_USE_SHADOW_HEAD */
#endif

/* #ifdef MPID_NEM_USE_SHADOW_HEAD */
/* static inline  */
/* void MPID_nem_rel_queue_poll (MPID_nem_queue_ptr_t rel_qhead, MPID_nem_poll_dir_t in_or_out) */
/* { */
/*     MPID_nem_queue_ptr_t  qhead = MPID_NEM_REL_TO_ABS( rel_qhead ); */

/*     MPID_nem_rel_network_poll (in_or_out); */
/*     if (qhead->my_head == MPID_NEM_REL_NULL) */
/*     { */
/* 	while (qhead->head == MPID_NEM_REL_NULL) */
/* 	{ */
/* 	    MPID_nem_rel_network_poll ( in_or_out ); */
/* 	    sched_yield(); */
/* 	} */
/* 	qhead->my_head = qhead->head;	 */
/* 	qhead->head = MPID_NEM_REL_NULL;    */
/*     } */
/* } */
/* #else  */
/* static inline void MPID_nem_rel_queue_poll (MPID_nem_queue_ptr_t rel_qhead, MPID_nem_poll_dir_t in_or_out) */
/* { */
/*     MPID_nem_queue_ptr_t  qhead = MPID_NEM_REL_TO_ABS( rel_qhead ); */

/*     MPID_nem_rel_network_poll ( in_or_out );     */
/*     while (qhead->head == MPID_NEM_REL_NULL) */
/*     { */
/* 	MPID_nem_rel_network_poll ( in_or_out ); */
/* 	sched_yield(); */
/*     } */
/* } */
/* #endif/\* MPID_NEM_USE_SHADOW_HEAD *\/ */

#endif /* MPID_NEM_QUEUE_H */
