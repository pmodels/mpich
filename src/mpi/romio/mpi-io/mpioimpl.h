/* -*- Mode: C; c-basic-offset:4 ; -*- */
/* 
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */


/* header file for MPI-IO implementation. not intended to be
   user-visible */ 

#ifndef MPIOIMPL_INCLUDE
#define MPIOIMPL_INCLUDE

#include "adio.h"
#include "mpio.h"

#ifdef ROMIO_INSIDE_MPICH2
#include "glue_romio.h"

#define MPIU_THREAD_CS_ENTER(name_,ctx_) MPIU_THREAD_CS_ENTER_##name_(ctx_)
#define MPIU_THREAD_CS_EXIT(name_,ctx_)  MPIU_THREAD_CS_EXIT_##name_(ctx_)
#define MPIU_THREAD_CS_ENTER_ALLFUNC(ctx_) MPIR_Ext_cs_enter_allfunc()
#define MPIU_THREAD_CS_EXIT_ALLFUNC(ctx_) MPIR_Ext_cs_exit_allfunc()

#else /* not ROMIO_INSIDE_MPICH2 */
/* Any MPI implementation that wishes to follow the thread-safety and
   error reporting features provided by MPICH2 must implement these 
   four functions.  Defining these as empty should not change the behavior 
   of correct programs */
#define MPIU_THREAD_CS_ENTER(x,y)
#define MPIU_THREAD_CS_EXIT(x,y)
#ifdef HAVE_WINDOWS_H
#define MPIU_UNREFERENCED_ARG(a) a
#else
#define MPIU_UNREFERENCED_ARG(a)
#endif
#endif /* ROMIO_INSIDE_MPICH2 */

/* info is a linked list of these structures */
struct MPIR_Info {
    int cookie;
    char *key, *value;
    struct MPIR_Info *next;
};

#define MPIR_INFO_COOKIE 5835657

MPI_Delete_function ADIOI_End_call;

/* common initialization routine */
void MPIR_MPIOInit(int * error_code);


#include "mpioprof.h"

#ifdef MPI_hpux
#  include "mpioinst.h"
#endif /* MPI_hpux */

#endif

