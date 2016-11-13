/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#if !defined(MPIU_THREAD_GLOBAL_H_INCLUDED)
#define MPIU_THREAD_GLOBAL_H_INCLUDED

/* GLOBAL locks are all real recursive ops */
#define MPIUI_THREAD_CS_ENTER_GLOBAL(mutex) MPIUI_THREAD_CS_ENTER_RECURSIVE("GLOBAL", mutex)
#define MPIUI_THREAD_CS_EXIT_GLOBAL(mutex) MPIUI_THREAD_CS_EXIT_RECURSIVE("GLOBAL", mutex)
#define MPIUI_THREAD_CS_YIELD_GLOBAL(mutex) MPIUI_THREAD_CS_YIELD_RECURSIVE("GLOBAL", mutex)

/* ALLGRAN locks are all real nonrecursive ops */
#define MPIUI_THREAD_CS_ENTER_ALLGRAN(mutex) MPIUI_THREAD_CS_ENTER_NONRECURSIVE("ALLGRAN", mutex)
#define MPIUI_THREAD_CS_EXIT_ALLGRAN(mutex) MPIUI_THREAD_CS_EXIT_NONRECURSIVE("ALLGRAN", mutex)
#define MPIUI_THREAD_CS_YIELD_ALLGRAN(mutex) MPIUI_THREAD_CS_YIELD_NONRECURSIVE("ALLGRAN", mutex)

/* POBJ locks are all NO-OPs */
#define MPIUI_THREAD_CS_ENTER_POBJ(mutex) do {} while (0)
#define MPIUI_THREAD_CS_EXIT_POBJ(mutex) do {} while (0)
#define MPIUI_THREAD_CS_YIELD_POBJ(mutex) do {} while (0)


/* define a type for the completion counter */
/* memory barriers aren't needed in this impl, because all access to completion
 * counters is done while holding the GLOBAL critical section */
typedef volatile int MPIU_cc_t;
#define MPIU_cc_get(cc_) (cc_)
#define MPIU_cc_set(cc_ptr_, val_) (*(cc_ptr_)) = (val_)
#define MPIU_cc_is_complete(cc_ptr_) (0 == *(cc_ptr_))

#define MPIU_cc_incr(cc_ptr_, was_incomplete_)  \
    do {                                        \
        *(was_incomplete_) = (*(cc_ptr_))++;    \
    } while (0)

#define MPIU_cc_decr(cc_ptr_, incomplete_)      \
    do {                                        \
        *(incomplete_) = --(*(cc_ptr_));        \
    } while (0)


/* "publishes" the obj with handle value (handle_) via the handle pointer
 * (hnd_lval_).  That is, it is a version of the following statement that fixes
 * memory consistency issues:
 *     (hnd_lval_) = (handle_);
 *
 * assumes that the following is always true: typeof(*hnd_lval_ptr_)==int
 */
/* This could potentially be generalized beyond MPI-handle objects, but we
 * should only take that step after seeing good evidence of its use.  A general
 * macro (that is portable to non-gcc compilers) will need type information to
 * make the appropriate volatile cast. */
/* Ideally _GLOBAL would use this too, but we don't want to count on OPA
 * availability in _GLOBAL mode.  Instead the GLOBAL critical section should be
 * used. */
#define MPIU_OBJ_PUBLISH_HANDLE(hnd_lval_, handle_)     \
    do {                                                \
        (hnd_lval_) = (handle_);                        \
    } while (0)

#endif /* !defined(MPIU_THREAD_MULTIPLE_H_INCLUDED) */
