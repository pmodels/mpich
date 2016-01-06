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

#endif /* !defined(MPIU_THREAD_MULTIPLE_H_INCLUDED) */
