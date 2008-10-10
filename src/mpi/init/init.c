/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id: init.c,v 1.34 2007/08/03 21:02:32 buntinas Exp $
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "mpi_init.h"


/* -- Begin Profiling Symbol Block for routine MPI_Init */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Init = PMPI_Init
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Init  MPI_Init
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Init as PMPI_Init
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Init
#define MPI_Init PMPI_Init

/* Fortran logical values. extern'd in mpiimpl.h */
/* MPI_Fint MPIR_F_TRUE, MPIR_F_FALSE; */

/* Any internal routines can go here.  Make them static if possible */
#endif

#undef FUNCNAME
#define FUNCNAME MPI_Init

/*@
   MPI_Init - Initialize the MPI execution environment

   Input Parameters:
+  argc - Pointer to the number of arguments 
-  argv - Pointer to the argument vector

Thread and Signal Safety:
This routine must be called by one thread only.  That thread is called
the `main thread` and must be the thread that calls 'MPI_Finalize'.

Notes:
   The MPI standard does not say what a program can do before an 'MPI_INIT' or
   after an 'MPI_FINALIZE'.  In the MPICH implementation, you should do
   as little as possible.  In particular, avoid anything that changes the
   external state of the program, such as opening files, reading standard
   input or writing to standard output.

Notes for Fortran:
The Fortran binding for 'MPI_Init' has only the error return
.vb
    subroutine MPI_INIT( ierr )
    integer ierr
.ve

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_INIT

.seealso: MPI_Init_thread, MPI_Finalize
@*/
int MPI_Init( int *argc, char ***argv )
{
    static const char FCNAME[] = "MPI_Init";
    int mpi_errno = MPI_SUCCESS;
    int rc;
    int threadLevel;
    MPID_MPI_INIT_STATE_DECL(MPID_STATE_MPI_INIT);

    rc = MPID_Wtime_init();
#ifdef USE_DBG_LOGGING
    MPIU_DBG_PreInit( argc, argv, rc );
#endif

    MPID_CS_INITIALIZE();
    /* FIXME: Can we get away without locking every time.  Now, we
       need a MPID_CS_ENTER/EXIT around MPI_Init and MPI_Init_thread.
       Progress may be called within MPI_Init, e.g., by a spawned
       child process.  Within progress, the lock is released and
       reacquired when blocking.  If the lock isn't acquired before
       then, the release in progress is incorrect.  Furthermore, if we
       don't release the lock after progress, we'll deadlock the next
       time this process tries to acquire the lock.
       MPID_CS_ENTER/EXIT functions are used here instead of
       MPIU_THREAD_SINGLE_CS_ENTER/EXIT because
       MPIR_ThreadInfo.isThreaded hasn't been initialized yet.
    */
    MPID_CS_ENTER();
    
    MPID_MPI_INIT_FUNC_ENTER(MPID_STATE_MPI_INIT);
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            if (MPIR_Process.initialized != MPICH_PRE_INIT) {
                mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
						  "**inittwice", NULL );
	    }
            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ... */

#if (MPICH_THREAD_LEVEL == MPI_THREAD_MULTIPLE)
    /* If we support all thread levels, all the use of an environment variable to set the default
       thread level */
    {
	const char *str = 0;
	threadLevel = MPI_THREAD_SINGLE;
	if (MPIU_GetEnvStr( "MPICH_THREADLEVEL_DEFAULT", &str )) {
	    if (strcmp(str,"MULTIPLE") == 0 || strcmp(str,"multiple") == 0) {
		threadLevel = MPI_THREAD_MULTIPLE;
	    }
	    else if (strcmp(str,"SERIALIZED") == 0 || strcmp(str,"serialized") == 0) {
		threadLevel = MPI_THREAD_SERIALIZED;
	    }
	    else if (strcmp(str,"FUNNELED") == 0 || strcmp(str,"funneled") == 0) {
		threadLevel = MPI_THREAD_FUNNELED;
	    }
	    else if (strcmp(str,"SINGLE") == 0 || strcmp(str,"single") == 0) {
		threadLevel = MPI_THREAD_SINGLE;
	    }
	    else {
		MPIU_Error_printf( "Unrecognized thread level %s\n", str );
		exit(1);
	    }
	}
    }
#else 
    threadLevel = MPI_THREAD_SINGLE;
#endif
    
    mpi_errno = MPIR_Init_thread( argc, argv, threadLevel, (int *)0 );
    if (mpi_errno != MPI_SUCCESS) goto fn_fail;

    /* ... end of body of routine ... */
    
    MPID_MPI_INIT_FUNC_EXIT(MPID_STATE_MPI_INIT);
    MPID_CS_EXIT();
    return mpi_errno;
    
  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_REPORTING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, 
	    "**mpi_init", "**mpi_init %p %p", argc, argv);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
    MPID_CS_EXIT();
    MPID_CS_FINALIZE();
    return mpi_errno;
    /* --END ERROR HANDLING-- */
}
