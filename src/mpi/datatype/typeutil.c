/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "datatype.h"

/* This is the utility file for datatypes that contains the basic datatype 
   items and storage management.  It also contains a temporary routine
   that is used by ROMIO to test to see if datatypes are contiguous */
#ifndef MPID_DATATYPE_PREALLOC 
#define MPID_DATATYPE_PREALLOC 8
#endif

/* Preallocated datatype objects */
MPID_Datatype MPID_Datatype_builtin[MPID_DATATYPE_N_BUILTIN + 1] = { {0} };
MPID_Datatype MPID_Datatype_direct[MPID_DATATYPE_PREALLOC] = { {0} };
MPIU_Object_alloc_t MPID_Datatype_mem = { 0, 0, 0, 0, MPID_DATATYPE, 
			      sizeof(MPID_Datatype), MPID_Datatype_direct,
					  MPID_DATATYPE_PREALLOC};

static int MPIR_Datatype_finalize(void *dummy );
static int MPIR_DatatypeAttrFinalizeCallback(void *dummy );

/* Call this routine to associate a MPID_Datatype with each predefined 
   datatype.  We do this with lazy initialization because many MPI 
   programs do not require anything except the predefined datatypes, and
   all of the necessary information about those is stored within the
   MPI_Datatype handle.  However, if the user wants to change the name
   (character string, set with MPI_Type_set_name) associated with a
   predefined name, then the structures must be allocated.
*/
/* FIXME does the order of this list need to correspond to anything in
   particular?  There are several lists of predefined types sprinkled throughout
   the codebase and it's unclear which (if any) of them must match exactly.
   [goodell@ 2009-03-17] */
static MPI_Datatype mpi_dtypes[] = {
    MPI_CHAR,
    MPI_UNSIGNED_CHAR,
    MPI_SIGNED_CHAR,
    MPI_BYTE,
    MPI_WCHAR,
    MPI_SHORT,
    MPI_UNSIGNED_SHORT,
    MPI_INT,
    MPI_UNSIGNED,
    MPI_LONG,
    MPI_UNSIGNED_LONG,
    MPI_FLOAT,
    MPI_DOUBLE,
    MPI_LONG_DOUBLE,
    MPI_LONG_LONG,
    MPI_UNSIGNED_LONG_LONG,
    MPI_PACKED,
    MPI_LB,
    MPI_UB,
    MPI_2INT,

    /* C99 types */
    MPI_INT8_T,
    MPI_INT16_T,
    MPI_INT32_T,
    MPI_INT64_T,
    MPI_UINT8_T,
    MPI_UINT16_T,
    MPI_UINT32_T,
    MPI_UINT64_T,
    MPI_C_BOOL,
    MPI_C_FLOAT_COMPLEX,
    MPI_C_DOUBLE_COMPLEX,
    MPI_C_LONG_DOUBLE_COMPLEX,

    /* address/offset/count types */
    MPI_AINT,
    MPI_OFFSET,
    MPI_COUNT,

    /* Fortran types */
    MPI_COMPLEX,
    MPI_DOUBLE_COMPLEX,
    MPI_LOGICAL,
    MPI_REAL,
    MPI_DOUBLE_PRECISION,
    MPI_INTEGER,
    MPI_2INTEGER,
#ifdef MPICH_DEFINE_2COMPLEX
    MPI_2COMPLEX,
    MPI_2DOUBLE_COMPLEX,
#endif
    MPI_2REAL,
    MPI_2DOUBLE_PRECISION,
    MPI_CHARACTER,
#ifdef HAVE_FORTRAN_BINDING
    /* Size-specific types; these are in section 10.2.4 (Extended Fortran 
       Support) as well as optional in MPI-1
    */
    MPI_REAL4,
    MPI_REAL8,
    MPI_REAL16,
    MPI_COMPLEX8,
    MPI_COMPLEX16,
    MPI_COMPLEX32,
    MPI_INTEGER1,
    MPI_INTEGER2,
    MPI_INTEGER4,
    MPI_INTEGER8,
    MPI_INTEGER16,
#endif
    /* This entry is a guaranteed end-of-list item */
    (MPI_Datatype) -1,
};

/*
  MPIR_Datatype_init()

  Main purpose of this function is to set up the following pair types:
  - MPI_FLOAT_INT
  - MPI_DOUBLE_INT
  - MPI_LONG_INT
  - MPI_SHORT_INT
  - MPI_LONG_DOUBLE_INT

  The assertions in this code ensure that:
  - this function is called before other types are allocated
  - there are enough spaces in the direct block to hold our types
  - we actually get the values we expect (otherwise errors regarding
    these types could be terribly difficult to track down!)

 */
static MPI_Datatype mpi_pairtypes[] = {
    MPI_FLOAT_INT,
    MPI_DOUBLE_INT,
    MPI_LONG_INT,
    MPI_SHORT_INT,
    MPI_LONG_DOUBLE_INT,
    (MPI_Datatype) -1
};

#undef FUNCNAME
#define FUNCNAME MPIR_Datatype_init
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Datatype_init(void)
{
    int i;
    int mpi_errno = MPI_SUCCESS;
    MPID_Datatype *ptr;

    MPIU_Assert(MPID_Datatype_mem.initialized == 0);
    MPIU_Assert(MPID_DATATYPE_PREALLOC >= 5);

    for (i=0; mpi_pairtypes[i] != (MPI_Datatype) -1; ++i) {
        /* types based on 'long long' and 'long double', may be disabled at
           configure time, and their values set to MPI_DATATYPE_NULL.  skip any
           such types. */
        if (mpi_pairtypes[i] == MPI_DATATYPE_NULL) continue;
        /* XXX: this allocation strategy isn't right if one or more of the
           pairtypes is MPI_DATATYPE_NULL.  in fact, the assert below will
           fail if any type other than the las in the list is equal to
           MPI_DATATYPE_NULL.  obviously, this should be fixed, but I need
           to talk to Rob R. first. -- BRT */
        /* XXX DJG it does work, but only because MPI_LONG_DOUBLE_INT is the
         * only one that is ever optional and it comes last */

        /* we use the _unsafe version because we are still in MPI_Init, before
         * multiple threads are permitted and possibly before support for
         * critical sections is entirely setup */
        ptr = (MPID_Datatype *)MPIU_Handle_obj_alloc_unsafe( &MPID_Datatype_mem );

        MPIU_Assert(ptr);
        MPIU_Assert(ptr->handle == mpi_pairtypes[i]);
        /* this is a redundant alternative to the previous statement */
        MPIU_Assert((void *) ptr == (void *) (MPID_Datatype_direct + HANDLE_INDEX(mpi_pairtypes[i])));

        mpi_errno = MPID_Type_create_pairtype(mpi_pairtypes[i], (MPID_Datatype *) ptr);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }

    MPIR_Add_finalize(MPIR_Datatype_finalize, 0,
                      MPIR_FINALIZE_CALLBACK_PRIO-1);

fn_fail:
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Datatype_finalize
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int MPIR_Datatype_finalize(void *dummy ATTRIBUTE((unused)) )
{
    int i;
    MPID_Datatype *dptr;

    for (i=0; mpi_pairtypes[i] != (MPI_Datatype) -1; i++) {
	if (mpi_pairtypes[i] != MPI_DATATYPE_NULL) {
	    MPID_Datatype_get_ptr(mpi_pairtypes[i], dptr);
	    MPID_Datatype_release(dptr);
	    mpi_pairtypes[i] = MPI_DATATYPE_NULL;
	}
    }
    return 0;
}

/* Called ONLY from MPIR_Datatype_init_names (type_get_name.c).  
   That routine calls it from within a single-init section to 
   ensure thread-safety. */

#undef FUNCNAME
#define FUNCNAME MPIR_Datatype_builtin_fillin
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Datatype_builtin_fillin(void)
{
    int mpi_errno = MPI_SUCCESS;
    int i;
    MPID_Datatype *dptr;
    MPI_Datatype  d = MPI_DATATYPE_NULL;
    static int is_init = 0;

    /* FIXME: This is actually an error, since this routine 
       should only be called once */
    if (is_init)
    {
	return MPI_SUCCESS;
    }

    if (!is_init) { 
	for (i=0; i<MPID_DATATYPE_N_BUILTIN; i++) {
	    /* Compute the index from the value of the handle */
	    d = mpi_dtypes[i];
	    if (d == -1) {
		/* At the end of mpi_dtypes */
		break;
	    }
	    /* Some of the size-specific types may be null, as might be types
	       based on 'long long' and 'long double' if those types were
	       disabled at configure time.  skip those cases. */
	    if (d == MPI_DATATYPE_NULL) continue;
	    
	    MPID_Datatype_get_ptr(d,dptr);
	    /* --BEGIN ERROR HANDLING-- */
	    if (dptr < MPID_Datatype_builtin || 
		dptr > MPID_Datatype_builtin + MPID_DATATYPE_N_BUILTIN)
		{
		    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS,
						     MPIR_ERR_FATAL, FCNAME,
						     __LINE__, MPI_ERR_INTERN,
						     "**typeinitbadmem", "**typeinitbadmem %d",
						     i);
		    return mpi_errno;
		}
	    /* --END ERROR HANDLING-- */
	    
	    /* dptr will point into MPID_Datatype_builtin */
	    dptr->handle	   = d;
	    dptr->is_permanent = 1;
	    dptr->is_contig	   = 1;
	    MPIU_Object_set_ref( dptr, 1 );
	    MPID_Datatype_get_size_macro(mpi_dtypes[i], dptr->size);
	    dptr->extent	   = dptr->size;
	    dptr->ub	   = dptr->size;
	    dptr->true_ub	   = dptr->size;
	    dptr->contents     = NULL; /* should never get referenced? */
	}
	/* --BEGIN ERROR HANDLING-- */
 	if (d != -1 && i < sizeof(mpi_dtypes)/sizeof(*mpi_dtypes) && mpi_dtypes[i] != -1) { 
	    /* We did not hit the end-of-list */
            mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL,
                                             FCNAME, __LINE__,
                                             MPI_ERR_INTERN, "**typeinitfail",
                                             "**typeinitfail %d", i-1);
            return mpi_errno;
	}
	/* --END ERROR HANDLING-- */
	is_init = 1;
    }
    return mpi_errno;
}

/* This will eventually be removed once ROMIO knows more about MPICH */
void MPIR_Datatype_iscontig(MPI_Datatype datatype, int *flag)
{
    MPID_Datatype *datatype_ptr;
    if (HANDLE_GET_KIND(datatype) == HANDLE_KIND_BUILTIN)
        *flag = 1;
    else  {
        MPID_Datatype_get_ptr(datatype, datatype_ptr);
        *flag = datatype_ptr->is_contig;
    }
}

/* If an attribute is added to a predefined type, we free the attributes 
   in Finalize */
static int MPIR_DatatypeAttrFinalizeCallback(void *dummy ATTRIBUTE((unused)) )
{
    MPID_Datatype *dtype;
    int i, mpi_errno=MPI_SUCCESS;

    for (i=0; i<MPID_DATATYPE_N_BUILTIN; i++) {
	dtype = &MPID_Datatype_builtin[i];
	if (dtype && MPIR_Process.attr_free && dtype->attributes) {
	    mpi_errno = MPIR_Process.attr_free( dtype->handle, 
						&dtype->attributes );
	    /* During finalize, we ignore error returns from the free */
	}
    }
    return mpi_errno;
}

void MPIR_DatatypeAttrFinalize( void )
{
    static int called=0;

    /* FIXME: This needs to be make thread safe */
    if (!called) {
	called = 1;
	MPIR_Add_finalize(MPIR_DatatypeAttrFinalizeCallback, 0, 
			  MPIR_FINALIZE_CALLBACK_PRIO-1);
    }
}

