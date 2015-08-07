/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*  
 *  (C) 2004 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "create_f90_util.h"
#include "mpiimpl.h"

/* The MPI standard requires that the datatypes that are returned from
   the MPI_Create_f90_xxx be both predefined and return the r (and p
   if real/complex) with which it was created.  This file contains routines
   that keep track of the datatypes that have been created.  The interface
   that is used is a single routine that is passed as input the 
   chosen base function and combiner, along with r,p, and returns the
   appropriate datatype, creating it if necessary */

/* This gives the maximum number of distinct types returned by any one of the
   MPI_Type_create_f90_xxx routines */
#define MAX_F90_TYPES 64
typedef struct { int combiner; int r, p;
    MPI_Datatype d; } F90Predefined;
static int nAlloc = 0;
static F90Predefined f90Types[MAX_F90_TYPES];

static int MPIR_FreeF90Datatypes( void *d )
{
    int i;

    for (i=0; i<nAlloc; i++) {
        MPIR_Type_free_impl( &f90Types[i].d );
    }
    return 0;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Create_unnamed_predefined
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Create_unnamed_predefined( MPI_Datatype old, int combiner, 
				    int r, int p, 
				    MPI_Datatype *new_ptr )
{
    int i;
    int mpi_errno = MPI_SUCCESS;
    F90Predefined *type;

    *new_ptr = MPI_DATATYPE_NULL;

    /* Has this type been defined already? */
    for (i=0; i<nAlloc; i++) {
	type = &f90Types[i];
	if (type->combiner == combiner && type->r == r && type->p == p) {
	    *new_ptr = type->d;
	    return mpi_errno;
	}
    }

    /* Create a new type and remember it */
    if (nAlloc >= MAX_F90_TYPES) {
	return MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, 
				     "MPIF_Create_unnamed_predefined", __LINE__,
				     MPI_ERR_INTERN, "**f90typetoomany", 0 );
    }
    if (nAlloc == 0) {
	/* Install the finalize callback that frees these datatyeps.
	   Set the priority high enough that this will be executed 
	   before the handle allocation check */
	MPIR_Add_finalize( MPIR_FreeF90Datatypes, 0, 2 );
    }

    type           = &f90Types[nAlloc++];
    type->combiner = combiner;
    type->r        = r;
    type->p        = p;

    /* Create a contiguous type from one instance of the named type */
    mpi_errno = MPID_Type_contiguous( 1, old, &type->d );
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

    /* Initialize the contents data */
    {
	MPID_Datatype *new_dtp = NULL;
	int vals[2];
	int nvals=0;

	switch (combiner) {
	case MPI_COMBINER_F90_INTEGER:
	    nvals   = 1;
	    vals[0] = r;
	    break;

	case MPI_COMBINER_F90_REAL:
	case MPI_COMBINER_F90_COMPLEX:
	    nvals   = 2;
	    vals[0] = p;
	    vals[1] = r;
	    break;
	}

	MPID_Datatype_get_ptr( type->d, new_dtp );
	mpi_errno = MPID_Datatype_set_contents(new_dtp, combiner,
					       nvals, 0, 0, vals,
					       NULL, NULL );
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);

        /* FIXME should we be setting type->is_permanent=TRUE here too?  If so,
         * will the cleanup code handle it correctly and not freak out? */

#ifndef NDEBUG
        {
            MPI_Datatype old_basic = MPI_DATATYPE_NULL;
            MPI_Datatype new_basic = MPI_DATATYPE_NULL;
            /* we used MPID_Type_contiguous and then stomped it's contents
             * information, so make sure that the basic_type is usable by
             * MPID_Type_commit */
            MPID_Datatype_get_basic_type(old, old_basic);
            MPID_Datatype_get_basic_type(new_dtp->handle, new_basic);
            MPIU_Assert(new_basic == old_basic);
        }
#endif

        /* the MPI Standard requires that these types are pre-committed
         * (MPI-2.2, sec 16.2.5, pg 492) */
        mpi_errno = MPID_Type_commit(&type->d);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }

    *new_ptr       = type->d;

fn_fail:
    return mpi_errno;
}

/*
   The simple approach used here is to store (unordered) the precision and 
   range of each type that is created as a result of a user-call to
   one of the MPI_Type_create_f90_xxx routines.  The standard requires
   that the *same* handle be returned to allow simple == comparisons.
   A finalize callback is added to free up any remaining storage
*/

#if 0
/* Given the requested range and the length of the type in bytes,
   return the matching datatype */

int MPIR_Match_f90_int( int range, int length, MPI_Datatype *newtype )
{
    int i;

    /* Has this type been requested before? */
    for (i=0; i<nAllocInteger; i++) {
	if (f90IntTypes[i].r == range) {
	    *newtype = f90IntTypes[i].d;
	    return 0;
	}
    }
    
    /* No.  Try to add it */
    if (nAllocInteger >= MAX_F90_TYPES) {
	int line = -1;  /* Hack to suppress warning message from 
 			   extracterrmsgs, since this code should
			   reflect the routine from which it was called,
			   since the decomposition of the internal routine
			   is of no relevance to either the user or 
			   developer */
	return MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, 
				     "MPI_Type_create_f90_integer", __LINE__,
				     MPI_ERR_INTERN, "**f90typetoomany", 0 );
    }
    
    /* Temporary */
    *newtype = MPI_DATATYPE_NULL;

    mpi_errno = MPIR_Create_unnamed_predefined( , , r, -1, newtype );
    f90IntTypes[nAllocInteger].r   = range;
    f90IntTypes[nAllocInteger].p   = -1;
    f90IntTypes[nAllocInteger++].d = *newtype;
    
    return 0;
}

/* Build a new type */
static int MPIR_Create_unnamed_predefined( MPI_Datatype old, int combiner, 
					   int r, int p, MPI_Datatype *new_ptr )
{
    int mpi_errno;

    /* Create a contiguous type from one instance of the named type */
    mpi_errno = MPID_Type_contiguous( 1, old, new_ptr );

    /* Initialize the contents data */
    if (mpi_errno == MPI_SUCCESS) {
	MPID_Datatype *new_dtp;
	int vals[2];
	int nvals=0;

	switch (combiner) {
	case MPI_COMBINER_F90_INTEGER:
	    nvals   = 1;
	    vals[0] = r;
	    break;

	case MPI_COMBINER_F90_REAL:
	case MPI_COMBINER_F90_COMPLEX:
	    nvals   = 2;
	    vals[0] = p;
	    vals[1] = r;
	    break;
	}

	MPID_Datatype_get_ptr(*new_ptr, new_dtp);
	mpi_errno = MPID_Datatype_set_contents(new_dtp,
					       combiner,
					       nvals,
					       0,
					       0,
					       vals,
					       NULL,
					       NULL );
    }
    
    return mpi_errno;
}
#endif
