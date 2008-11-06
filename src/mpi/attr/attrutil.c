/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "attr.h"
/*
 * Keyvals.  These are handled just like the other opaque objects in MPICH
 * The predefined keyvals (and their associated attributes) are handled 
 * separately, without using the keyval 
 * storage
 */

#ifndef MPID_KEYVAL_PREALLOC 
#define MPID_KEYVAL_PREALLOC 16
#endif

/* Preallocated keyval objects */
MPID_Keyval MPID_Keyval_direct[MPID_KEYVAL_PREALLOC] = { {0} };
MPIU_Object_alloc_t MPID_Keyval_mem = { 0, 0, 0, 0, MPID_KEYVAL, 
					    sizeof(MPID_Keyval), 
					    MPID_Keyval_direct,
					    MPID_KEYVAL_PREALLOC, };

#ifndef MPID_ATTR_PREALLOC 
#define MPID_ATTR_PREALLOC 32
#endif

/* Preallocated keyval objects */
MPID_Attribute MPID_Attr_direct[MPID_ATTR_PREALLOC] = { {0} };
MPIU_Object_alloc_t MPID_Attr_mem = { 0, 0, 0, 0, MPID_ATTR, 
					    sizeof(MPID_Attribute), 
					    MPID_Attr_direct,
					    MPID_ATTR_PREALLOC, };

void MPID_Attr_free(MPID_Attribute *attr_ptr)
{
    MPIU_Handle_obj_free(&MPID_Attr_mem, attr_ptr);
}

/*
  This function deletes a single attribute.
  It is called by both the function to delete a list and attribute set/put 
  val.  Return the return code from the delete function; 0 if there is no
  delete function.

  Even though there are separate keyvals for communicators, types, and files,
  we can use the same function because the handle for these is always an int
  in MPICH2.  

  Note that this simply invokes the attribute delete function.  It does not
  remove the attribute from the list of attributes.
*/
int MPIR_Call_attr_delete( int handle, MPID_Attribute *attr_p )
{
    static const char FCNAME[] = "MPIR_Call_attr_delete";
    MPID_Delete_function delfn;
    MPID_Lang_t          language;
    int                  mpi_errno=0;
    int                  userErr=0;
    MPIU_THREADPRIV_DECL;

    MPIU_THREADPRIV_GET;

    MPIR_Nest_incr();
    
    delfn    = attr_p->keyval->delfn;
    language = attr_p->keyval->language;
    switch (language) {
    case MPID_LANG_C: 
	if (delfn.C_DeleteFunction) {
	    userErr = delfn.C_DeleteFunction( handle,
					      attr_p->keyval->handle, 
					      attr_p->value, 
					      attr_p->keyval->extra_state );
	    if (userErr != 0) {
		MPIU_ERR_SET1( mpi_errno, MPI_ERR_OTHER, 
			       "**user", "**userdel %d", userErr );
	    }
	}
	break;

#ifdef HAVE_CXX_BINDING
    case MPID_LANG_CXX: 
	if (delfn.C_DeleteFunction) {
	    int handleType = HANDLE_GET_MPI_KIND(handle);
	    userErr = (*MPIR_Process.cxx_call_delfn)( handleType,
						handle,
						attr_p->keyval->handle, 
						attr_p->value,
						attr_p->keyval->extra_state, 
				(void (*)(void)) delfn.C_DeleteFunction );
	    if (userErr != 0) {
		MPIU_ERR_SET1( mpi_errno, MPI_ERR_OTHER, 
			       "**user", "**userdel %d", userErr );
	    }
	}
	break;
#endif

#ifdef HAVE_FORTRAN_BINDING
    case MPID_LANG_FORTRAN: 
	{
	    MPI_Fint fhandle, fkeyval, fvalue, *fextra, ierr;
	    if (delfn.F77_DeleteFunction) {
		fhandle = (MPI_Fint) (handle);
		fkeyval = (MPI_Fint) (attr_p->keyval->handle);
		/* The following cast can lose data on systems whose
		   pointers are longer than integers.
		   We do this in two steps to keep compilers happy.
		   Note that the casts are consistent with the 
		   way in which the Fortran attributes are used (they
                   are MPI_Fint values, and we assume 
		   sizeof(MPI_Fint) <= sizeof(MPI_Aint).  
		   See also src/binding/f77/attr_getf.c . */
		fvalue  = (MPI_Fint) MPI_VOID_PTR_CAST_TO_MPI_AINT(attr_p->value);
		fextra  = (MPI_Fint*) (attr_p->keyval->extra_state);
		delfn.F77_DeleteFunction( &fhandle, &fkeyval, &fvalue, 
					  fextra, &ierr );
		if (ierr) userErr = (int)ierr;
		else      userErr = MPI_SUCCESS;
		if (userErr != 0) {
		    MPIU_ERR_SET1( mpi_errno, MPI_ERR_OTHER, 
				   "**user", "**userdel %d", userErr );
		}
	    }
	}
	break;
    case MPID_LANG_FORTRAN90: 
	{
	    MPI_Fint fhandle, fkeyval, ierr;
	    MPI_Aint fvalue, *fextra;
	    if (delfn.F90_DeleteFunction) {
		fhandle = (MPI_Fint) (handle);
		fkeyval = (MPI_Fint) (attr_p->keyval->handle);
		fvalue  = MPI_VOID_PTR_CAST_TO_MPI_AINT(attr_p->value);
		fextra  = (MPI_Aint*) (attr_p->keyval->extra_state );
		delfn.F90_DeleteFunction( &fhandle, &fkeyval, &fvalue, 
					  fextra, &ierr );
		if (ierr) userErr = (int)ierr;
		else      userErr = MPI_SUCCESS;
		if (userErr != 0) {
		    MPIU_ERR_SET1( mpi_errno, MPI_ERR_OTHER, 
				   "**user", "**userdel %d", userErr );
		}
	    }
	}
	break;
#endif
    }
    
    MPIR_Nest_decr();
    return mpi_errno;
}

/* Routine to duplicate an attribute list */
int MPIR_Attr_dup_list( int handle, MPID_Attribute *old_attrs, 
			MPID_Attribute **new_attr )
{
    static const char FCNAME[] = "MPIR_Attr_dup_list";
    MPID_Attribute     *p, *new_p, **next_new_attr_ptr = new_attr;
    MPID_Copy_function copyfn;
    MPID_Lang_t        language;
    void               *new_value = NULL;
    int                flag;
    int                mpi_errno = 0;
    int                userErr=0;
    MPIU_THREADPRIV_DECL;

    MPIU_THREADPRIV_GET;
    MPIR_Nest_incr();
    
    p = old_attrs;
    while (p) {
	/* Run the attribute delete function first */
/* Why is this here?
	mpi_errno = MPIR_Call_attr_delete( handle, p );
	if (mpi_errno)
	{
	    goto fn_exit;
	}
*/

	/* Now call the attribute copy function (if any) */
	copyfn   = p->keyval->copyfn;
	language = p->keyval->language;
	flag = 0;
	switch (language) {
	case MPID_LANG_C: 
	    if (copyfn.C_CopyFunction) {
		userErr = copyfn.C_CopyFunction( handle, 
						 p->keyval->handle, 
						 p->keyval->extra_state, 
						 p->value, &new_value, &flag );
		if (userErr != 0) {
		    MPIU_ERR_SET1( mpi_errno, MPI_ERR_OTHER, 
				   "**user", "**usercopy %d", userErr );
		}
	    }
	    break;
#ifdef HAVE_CXX_BINDING
	case MPID_LANG_CXX: 
	    if (copyfn.C_CopyFunction) {
		int handleType = HANDLE_GET_MPI_KIND(handle);
		userErr = (*MPIR_Process.cxx_call_copyfn)( 
				   handleType,
				   handle,
				   p->keyval->handle, 
				   p->keyval->extra_state, 
				   p->value, &new_value, &flag,
				   (void (*)(void)) copyfn.C_CopyFunction );
		if (userErr != 0) {
		    MPIU_ERR_SET1( mpi_errno, MPI_ERR_OTHER, 
				   "**user", "**usercopy %d", userErr );
		}
	    }
	    break;
#endif
#ifdef HAVE_FORTRAN_BINDING
	case MPID_LANG_FORTRAN: 
	    {
		if (copyfn.F77_CopyFunction) {
		    MPI_Fint fhandle, fkeyval, fvalue, *fextra, fflag, fnew, ierr;
		    fhandle = (MPI_Fint) (handle);
		    fkeyval = (MPI_Fint) (p->keyval->handle);
		    /* The following cast can lose data on systems whose
		       pointers are longer than integers */
		    fvalue  = (MPI_Fint) MPI_VOID_PTR_CAST_TO_MPI_AINT(p->value);
		    fextra  = (MPI_Fint*) (p->keyval->extra_state );
		    copyfn.F77_CopyFunction( &fhandle, &fkeyval, fextra,
					     &fvalue, &fnew, &fflag, &ierr );
		    if (ierr) userErr = (int)ierr;
		    flag      = fflag;
		    new_value = MPI_AINT_CAST_TO_VOID_PTR (MPI_Aint) fnew;
		    if (userErr != 0) {
			MPIU_ERR_SET1( mpi_errno, MPI_ERR_OTHER, 
				       "**user", "**usercopy %d", userErr );
		    }
		}
	    }
	    break;
	case MPID_LANG_FORTRAN90: 
	    {
		if (copyfn.F90_CopyFunction) {
		    MPI_Fint fhandle, fkeyval, fflag, ierr;
		    MPI_Aint fvalue, fnew, *fextra;
		    fhandle = (MPI_Fint) (handle);
		    fkeyval = (MPI_Fint) (p->keyval->handle);
		    fvalue  = MPI_VOID_PTR_CAST_TO_MPI_AINT(p->value);
		    fextra  = (MPI_Aint*) (p->keyval->extra_state );
		    copyfn.F90_CopyFunction( &fhandle, &fkeyval, fextra,
					     &fvalue, &fnew, &fflag, &ierr );
		    if (ierr) userErr = (int)ierr;
		    flag = fflag;
		    new_value = MPI_AINT_CAST_TO_VOID_PTR fnew;
		    if (userErr != 0) {
			MPIU_ERR_SET1( mpi_errno, MPI_ERR_OTHER, 
				       "**user", "**usercopy %d", userErr );
		    }
		}
	    }
	    break;
#endif
	}
	
	/* If flag was returned as true and there was no error, then
	   insert this attribute into the new list (new_attr) */
	if (flag && !mpi_errno) {
	    /* duplicate the attribute by creating new storage, copying the
	       attribute value, and invoking the copy function */
	    new_p = (MPID_Attribute *)MPIU_Handle_obj_alloc( &MPID_Attr_mem );
	    /* --BEGIN ERROR HANDLING-- */
	    if (!new_p) {
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
						 "MPIR_Attr_dup_list", __LINE__,
						  MPI_ERR_OTHER, "**nomem", 0 );
		goto fn_exit;
	    }
	    /* --END ERROR HANDLING-- */
	    if (!*new_attr) { 
		*new_attr = new_p;
	    }
	    new_p->keyval        = p->keyval;
	    *(next_new_attr_ptr) = new_p;
	    /* Remember that we need this keyval */
	    MPIR_Keyval_add_ref(p->keyval);
	    
	    new_p->pre_sentinal  = 0;
	    new_p->value 	 = new_value;
	    new_p->post_sentinal = 0;
	    new_p->next	         = 0;
	    next_new_attr_ptr    = &(new_p->next);
	}
	else if (mpi_errno)
	{ 
	    goto fn_exit;
	}

	p = p->next;
    }

  fn_exit:
    MPIR_Nest_decr();
    return mpi_errno;
}

/* Routine to delete an attribute list */
int MPIR_Attr_delete_list( int handle, MPID_Attribute *attr )
{
    static const char FCNAME[] = "MPIR_Attr_delete_list";
    MPID_Attribute *p, *new_p;
    int mpi_errno = MPI_SUCCESS;

    p = attr;
    while (p) {
	/* delete the attribute by first executing the delete routine, if any,
	   determine the the next attribute, and recover the attributes 
	   storage */
	new_p = p->next;
	
	/* Check the sentinals first */
	/* --BEGIN ERROR HANDLING-- */
	if (p->pre_sentinal != 0 || p->post_sentinal != 0) {
	    MPIU_ERR_SET(mpi_errno,MPI_ERR_OTHER,"**attrsentinal");
	    /* We could keep trying to free the attributes, but for now
	       we'll just bag it */
	    return mpi_errno;
	}
	/* --END ERROR HANDLING-- */
	/* For this attribute, find the delete function for the 
	   corresponding keyval */
	/* Still to do: capture any error returns but continue to 
	   process attributes */
	mpi_errno = MPIR_Call_attr_delete( handle, p );

	/* We must also remove the keyval reference.  If the keyval
	   was freed earlier (reducing the refcount), the actual 
	   release and free will happen here.  We must free the keyval
	   even if the attr delete failed, as we then remove the 
	   attribute.
	*/
	{
	    int in_use;
	    /* Decrement the use of the keyval */
	    MPIR_Keyval_release_ref( p->keyval, &in_use);
	    if (!in_use) {
		MPIU_Handle_obj_free( &MPID_Keyval_mem, p->keyval );
	    }
	}
	
	MPIU_Handle_obj_free( &MPID_Attr_mem, p );
	
	p = new_p;
    }
    return mpi_errno;
}

#ifdef HAVE_FORTRAN_BINDING
/* This routine is used by the Fortran binding to change the language of a
   keyval to Fortran 
*/
void MPIR_Keyval_set_fortran( int keyval )
{
    MPID_Keyval *keyval_ptr;

    MPID_Keyval_get_ptr( keyval, keyval_ptr );
    if (keyval_ptr) 
	keyval_ptr->language = MPID_LANG_FORTRAN;
}

/* 
   This routine is used by the Fortran binding to change the language of a
   keyval to Fortran90 (also used for Fortran77 when address-sized integers
   are used 
*/
void MPIR_Keyval_set_fortran90( int keyval )
{
    MPID_Keyval *keyval_ptr;

    MPID_Keyval_get_ptr( keyval, keyval_ptr );
    if (keyval_ptr) 
	keyval_ptr->language = MPID_LANG_FORTRAN90;
}
#endif
#ifdef HAVE_CXX_BINDING
/* This function allows the C++ interface to provide the routines that use
   a C interface that invoke the C++ attribute copy and delete functions.
*/
void MPIR_Keyval_set_cxx( int keyval, void (*delfn)(void), void (*copyfn)(void) )
{
    MPID_Keyval *keyval_ptr;

    MPID_Keyval_get_ptr( keyval, keyval_ptr );
    
    keyval_ptr->language	 = MPID_LANG_CXX;
    MPIR_Process.cxx_call_delfn	 = (int (*)(int, int, int, void *, void *, 
					    void (*)(void)))delfn;
    MPIR_Process.cxx_call_copyfn = (int (*)(int, int, int, void *, void *, 
					    void *, int *, 
					    void (*)(void)))copyfn;
}
#endif
