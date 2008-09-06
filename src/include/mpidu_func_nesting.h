/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id: mpidu_func_nesting.h,v 1.3 2006/08/17 01:07:33 sankara Exp $
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#if !defined(MPIDU_FUNC_NESTING_H_INCLUDED)
#define MPIDU_FUNC_NESTING_H_INCLUDED

/* state declaration macros */
#define MPIR_STATE_DECL(a)		int a##_nest_level_in
#define MPID_MPI_STATE_DECL(a)		MPIR_STATE_DECL(a)
#define MPID_MPI_INIT_STATE_DECL(a)	MPIR_STATE_DECL(a)
#define MPID_MPI_FINALIZE_STATE_DECL(a)	MPIR_STATE_DECL(a)
#define MPIDI_STATE_DECL(a)		MPIR_STATE_DECL(a)
#define MPIDI_INIT_STATE_DECL(a)	MPIR_STATE_DECL(a)
#define MPIDI_FINALIZE_STATE_DECL(a)	MPIR_STATE_DECL(a)

/* function enter and exit macros */
#define MPIR_FUNC_ENTER(a)			\
{						\
    a##_nest_level_in = MPIR_Nest_value();	\
    MPIU_DBG_MSG(ROUTINE_ENTER,TYPICAL,"Entering " ## #a );\
}

#define MPIR_FUNC_EXIT(a)										\
{													\
    int nest_level_out = MPIR_Nest_value();								\
													\
    MPIU_DBG_MSG(ROUTINE_EXIT,TYPICAL,"Leaving " ## #a );\
    if (a##_nest_level_in != nest_level_out)								\
    {													\
	MPIU_Error_printf("Error in nesting level: file=%s, line=%d, nest_in=%d, nest_out=%d\n",	\
			  __FILE__, __LINE__, a##_nest_level_in, nest_level_out);			\
	exit(1);											\
    }													\
}

/* Tell the package to define the rest of the enter/exit macros in 
   terms of these */
#define NEEDS_FUNC_ENTER_EXIT_DEFS 1

#endif /* defined(MPIDU_FUNC_NESTING_H_INCLUDED) */
