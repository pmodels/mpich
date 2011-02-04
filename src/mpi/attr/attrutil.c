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

/* Provides a way to trap all attribute allocations when debugging leaks. */
MPID_Attribute *MPID_Attr_alloc(void)
{
    MPID_Attribute *attr = (MPID_Attribute *)MPIU_Handle_obj_alloc(&MPID_Attr_mem);
    /* attributes don't have refcount semantics, but let's keep valgrind and
     * the debug logging pacified */
    MPIU_Object_set_ref(attr, 0);
    return attr;
}

void MPID_Attr_free(MPID_Attribute *attr_ptr)
{
    MPIU_Handle_obj_free(&MPID_Attr_mem, attr_ptr);
}

#undef FUNCNAME
#define FUNCNAME MPIR_Call_attr_delete
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
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
    int rc;
    int mpi_errno = MPI_SUCCESS;
    MPID_Keyval* kv = attr_p->keyval;

    if(kv->delfn.user_function == NULL)
        goto fn_exit;

    rc = kv->delfn.proxy(
                kv->delfn.user_function,
                handle,
                attr_p->keyval->handle,
                attr_p->attrType,
                (void *)(MPIR_Pint)attr_p->value,
                attr_p->keyval->extra_state
                );
    /* --BEGIN ERROR HANDLING-- */
    if(rc != 0){
#if MPICH_ERROR_MSG_LEVEL < MPICH_ERROR_MSG_ALL
	/* If rc is a valid error class, then return that.  
	   Note that it may be a dynamic error class */
	/* AMBIGUOUS: This is an ambiguity in the MPI standard: What is the
	   error value returned from the user-provided routine?  Particularly
	   with the MPI-2 feature of user-defined error codes, the 
	   user expectation is probably that the user-provided error code
	   is returned. */
	mpi_errno = rc;
#else
        mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**user", "**userdel %d", rc);
#endif
        goto fn_fail;
    }
    /* --END ERROR HANDLING-- */

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/*
  This function copies a single attribute.
  It is called by the function to copy a list of attribute
  Return the return code from the copy function; MPI_SUCCESS if there is
  no copy function.

  Even though there are separate keyvals for communicators, types, and files,
  we can use the same function because the handle for these is always an int
  in MPICH2.

  Note that this simply invokes the attribute copy function.
*/
#undef FUNCNAME
#define FUNCNAME MPIR_Call_attr_copy
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIR_Call_attr_copy( int handle, MPID_Attribute *attr_p, void** value_copy, int* flag)
{
    int mpi_errno = MPI_SUCCESS;
    int rc;
    MPID_Keyval* kv = attr_p->keyval;

    if(kv->copyfn.user_function == NULL)
        goto fn_exit;

    rc = kv->copyfn.proxy(
                kv->copyfn.user_function,
                handle,
                attr_p->keyval->handle,
                attr_p->keyval->extra_state,
                attr_p->attrType,
                (void *)(MPIR_Pint)attr_p->value,
                value_copy,
                flag
                );

    /* --BEGIN ERROR HANDLING-- */
    if(rc != 0){
#if MPICH_ERROR_MSG_LEVEL < MPICH_ERROR_MSG_ALL
	mpi_errno = rc;
#else
        mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**user", "**usercopy %d", rc);
#endif
        goto fn_fail;
    }
    /* --END ERROR HANDLING-- */
fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Attr_dup_list
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
/* Routine to duplicate an attribute list */
int MPIR_Attr_dup_list( int handle, MPID_Attribute *old_attrs, 
			MPID_Attribute **new_attr )
{
    MPID_Attribute *p, *new_p, **next_new_attr_ptr=new_attr;
    void* new_value = NULL;
    int mpi_errno = MPI_SUCCESS;

    for(p = old_attrs; p != NULL; p = p->next)
    {
        /* call the attribute copy function (if any) */
        int flag = 0;
        mpi_errno = MPIR_Call_attr_copy(
                        handle,
                        p,
                        &new_value,
                        &flag
                        );

        if(mpi_errno != MPI_SUCCESS)
            goto fn_fail;

        if(!flag)
            continue;
        /* If flag was returned as true, then insert this attribute into the 
	   new list (new_attr) */

        /* duplicate the attribute by creating new storage, copying the
        attribute value, and invoking the copy function */
        new_p = MPID_Attr_alloc();
        /* --BEGIN ERROR HANDLING-- */
        if (!new_p){
            mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
            goto fn_fail;
        }
        /* --END ERROR HANDLING-- */
        if(!*new_attr){
            *new_attr = new_p;
        }
        *(next_new_attr_ptr) = new_p;

        new_p->keyval = p->keyval;
        /* Remember that we need this keyval */
        MPIR_Keyval_add_ref(p->keyval);

        new_p->attrType         = p->attrType;
        new_p->pre_sentinal     = 0;
        new_p->value            = (MPID_AttrVal_t)(MPIR_Pint)new_value;
        new_p->post_sentinal    = 0;
        new_p->next             = 0;

        next_new_attr_ptr = &(new_p->next);
    } /* for(;;) */

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Attr_delete_list
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
/* Routine to delete an attribute list */
int MPIR_Attr_delete_list( int handle, MPID_Attribute **attr )
{
    MPID_Attribute *p, *new_p;
    int mpi_errno = MPI_SUCCESS;

    p = *attr;
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

    /* We must zero out the attribute list pointer or we could attempt to use it
       later.  This normally can't happen because the communicator usually
       disappears after a call to MPI_Comm_free.  But if the attribute keyval
       has an associated delete function that returns an error then we don't
       actually free the communicator despite having freed all the attributes
       associated with the communicator.

       This function is also used for Win and Type objects, but the idea is the
       same in those cases as well. */
    *attr = NULL;
    return mpi_errno;
}

int
MPIR_Attr_copy_c_proxy(
    MPI_Comm_copy_attr_function* user_function,
    int handle,
    int keyval,
    void* extra_state,
    MPIR_AttrType attrib_type,
    void* attrib,
    void** attrib_copy,
    int* flag
    )
{
    void *attrib_val = NULL;
    int ret;

    /* Make sure that the attribute value is delieverd as a pointer */
    if (MPIR_ATTR_KIND(attrib_type) == MPIR_ATTR_KIND(MPIR_ATTR_INT)){
        attrib_val = &attrib;
    }
    else{
        attrib_val = attrib;
    }

    /* user functions might call other MPI functions, so we need to
     * release the lock here. This is safe to do as ALLFUNC is not at
     * all recursive in our implementation. */
    MPIU_THREAD_CS_EXIT(ALLFUNC,);
    ret = user_function(handle, keyval, extra_state, attrib_val, attrib_copy, flag);
    MPIU_THREAD_CS_ENTER(ALLFUNC,);

    return ret;
}


int
MPIR_Attr_delete_c_proxy(
    MPI_Comm_delete_attr_function* user_function,
    int handle,
    int keyval,
    MPIR_AttrType attrib_type,
    void* attrib,
    void* extra_state
    )
{
    void *attrib_val = NULL;
    int ret;

    /* Make sure that the attribute value is delieverd as a pointer */
    if (MPIR_ATTR_KIND(attrib_type) == MPIR_ATTR_KIND(MPIR_ATTR_INT))
        attrib_val = &attrib;
    else
        attrib_val = attrib;

    /* user functions might call other MPI functions, so we need to
     * release the lock here. This is safe to do as ALLFUNC is not at
     * all recursive in our implementation. */
    MPIU_THREAD_CS_EXIT(ALLFUNC,);
    ret = user_function(handle, keyval, attrib_val, extra_state);
    MPIU_THREAD_CS_ENTER(ALLFUNC,);

    return ret;
}

/* FIXME: Missing routine description */
void
MPIR_Keyval_set_proxy(
    int keyval,
    MPID_Attr_copy_proxy copy_proxy,
    MPID_Attr_delete_proxy delete_proxy
    )
{
    MPID_Keyval*  keyval_ptr;
    MPID_Keyval_get_ptr( keyval, keyval_ptr );
    if(keyval_ptr == NULL)
        return;

    keyval_ptr->copyfn.proxy = copy_proxy;
    keyval_ptr->delfn.proxy = delete_proxy;
}
