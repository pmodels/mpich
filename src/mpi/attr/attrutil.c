/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#ifdef BUILD_MPI_ABI
#include "mpi_abi_util.h"
#endif
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
MPII_Keyval MPII_Keyval_direct[MPID_KEYVAL_PREALLOC];

MPIR_Object_alloc_t MPII_Keyval_mem = { 0, 0, 0, 0, 0, 0, 0, MPIR_KEYVAL,
    sizeof(MPII_Keyval),
    MPII_Keyval_direct,
    MPID_KEYVAL_PREALLOC,
    {0}
};

/* Preallocated keyval objects */
MPIR_Attribute MPID_Attr_direct[MPIR_ATTR_PREALLOC];

MPIR_Object_alloc_t MPID_Attr_mem = { 0, 0, 0, 0, 0, 0, 0, MPIR_ATTR,
    sizeof(MPIR_Attribute),
    MPID_Attr_direct,
    MPIR_ATTR_PREALLOC,
    {0}
};

/* Provides a way to trap all attribute allocations when debugging leaks. */
MPIR_Attribute *MPID_Attr_alloc(void)
{
    MPIR_Attribute *attr = (MPIR_Attribute *) MPIR_Handle_obj_alloc(&MPID_Attr_mem);
    /* attributes don't have refcount semantics, but let's keep valgrind and
     * the debug logging pacified */
    MPIR_Assert(attr != NULL);
    MPIR_Object_set_ref(attr, 0);
    return attr;
}

void MPID_Attr_free(MPIR_Attribute * attr_ptr)
{
    MPIR_Handle_obj_free(&MPID_Attr_mem, attr_ptr);
}

/*
  This function deletes a single attribute.
  It is called by both the function to delete a list and attribute set/put
  val.  Return the return code from the delete function; 0 if there is no
  delete function.

  Even though there are separate keyvals for communicators, types, and files,
  we can use the same function because the handle for these is always an int
  in MPICH.

  Note that this simply invokes the attribute delete function.  It does not
  remove the attribute from the list of attributes.
*/
int MPIR_Call_attr_delete(int handle, MPIR_Attribute * attr_p)
{
    int mpi_errno = MPI_SUCCESS, rc;
    MPII_Keyval *kv = attr_p->keyval;

    if (kv->delfn.user_function == NULL)
        goto fn_exit;

    /* user functions, as well as binding proxies, might call other MPI
     * functions, so we need to release the lock here. This is safe to do
     * as GLOBAL is not at all recursive in our implementation. */
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    rc = kv->delfn.proxy(kv->delfn.user_function,
                         handle,
                         attr_p->keyval->handle,
                         attr_p->attrType,
                         (void *) (intptr_t) attr_p->value, attr_p->keyval->extra_state);
    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    /* --BEGIN ERROR HANDLING-- */
    if (rc != 0) {
        MPIR_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**user", "**userdel %d", rc);
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
  in MPICH.

  Note that this simply invokes the attribute copy function.
*/
int MPIR_Call_attr_copy(int handle, MPIR_Attribute * attr_p, void **value_copy, int *flag)
{
    int mpi_errno = MPI_SUCCESS, rc;
    MPII_Keyval *kv = attr_p->keyval;

    if (kv->copyfn.user_function == NULL)
        goto fn_exit;

    /* user functions might call other MPI functions, so we need to
     * release the lock here. This is safe to do as GLOBAL is not at
     * all recursive in our implementation. */
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    rc = kv->copyfn.proxy(kv->copyfn.user_function,
                          handle,
                          attr_p->keyval->handle,
                          attr_p->keyval->extra_state,
                          attr_p->attrType, (void *) (intptr_t) attr_p->value, value_copy, flag);
    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);

    /* --BEGIN ERROR HANDLING-- */
    if (rc != 0) {
        MPIR_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**user", "**usercopy %d", rc);
        goto fn_fail;
    }
    /* --END ERROR HANDLING-- */
  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* Routine to duplicate an attribute list */
int MPIR_Attr_dup_list(int handle, MPIR_Attribute * old_attrs, MPIR_Attribute ** new_attr)
{
    int mpi_errno = MPI_SUCCESS, rc;
    MPIR_Attribute *p, *new_p, **next_new_attr_ptr = new_attr;
    void *new_value = NULL;

    for (p = old_attrs; p != NULL; p = p->next) {
        /* call the attribute copy function (if any) */
        int flag = 0;
        rc = MPIR_Call_attr_copy(handle, p, &new_value, &flag);
        if (rc != 0) {
            mpi_errno = rc;
            goto fn_fail;
        }

        if (!flag)
            continue;
        /* If flag was returned as true, then insert this attribute into the
         * new list (new_attr) */

        /* duplicate the attribute by creating new storage, copying the
         * attribute value, and invoking the copy function */
        new_p = MPID_Attr_alloc();
        /* --BEGIN ERROR HANDLING-- */
        if (!new_p) {
            MPIR_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**nomem");
            goto fn_fail;
        }
        /* --END ERROR HANDLING-- */
        if (!*new_attr) {
            *new_attr = new_p;
        }
        *(next_new_attr_ptr) = new_p;

        new_p->keyval = p->keyval;
        /* Remember that we need this keyval */
        MPII_Keyval_add_ref(p->keyval);

        new_p->attrType = p->attrType;
        new_p->pre_sentinal = 0;
        /* FIXME: This is not correct in some cases (size(MPI_Aint)>
         * sizeof(intptr_t)) */
        new_p->value = (MPII_Attr_val_t) (intptr_t) new_value;
        new_p->post_sentinal = 0;
        new_p->next = 0;

        next_new_attr_ptr = &(new_p->next);
    }   /* for(;;) */

  fn_exit:
    return mpi_errno;
  fn_fail:
    /* In case of any failure, remove the attributes duplicated so far
     * to prevent MPICH and user memory leaks.  We ignore any errors
     * from the cleanup, otherwise we would be masking the original
     * error from the user attribute copy operation. */
    (void) MPIR_Attr_delete_list(handle, new_attr);
    goto fn_exit;
}

/* Routine to delete an attribute list */
int MPIR_Attr_delete_list(int handle, MPIR_Attribute ** attr)
{
    int mpi_errno = MPI_SUCCESS, rc;
    MPIR_Attribute *p, *new_p;

    p = *attr;
    while (p) {
        /* delete the attribute by first executing the delete routine, if any,
         * determine the the next attribute, and recover the attributes
         * storage */
        new_p = p->next;

        /* Check the sentinels first */
        /* --BEGIN ERROR HANDLING-- */
        if (p->pre_sentinal != 0 || p->post_sentinal != 0) {
            MPIR_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**attrsentinal");
            /* We could keep trying to free the attributes, but for now
             * we'll just bag it */
            return mpi_errno;
        }
        /* --END ERROR HANDLING-- */
        /* For this attribute, find the delete function for the
         * corresponding keyval. If the delete function fails,
         * we record the last failure */
        rc = MPIR_Call_attr_delete(handle, p);
        if (rc != 0) {
            mpi_errno = rc;
        }

        /* We must also remove the keyval reference.  If the keyval
         * was freed earlier (reducing the refcount), the actual
         * release and free will happen here.  We must free the keyval
         * even if the attr delete failed, as we then remove the
         * attribute.
         */
        {
            int in_use;
            /* Decrement the use of the keyval */
            MPII_Keyval_release_ref(p->keyval, &in_use);
            if (!in_use) {
                MPIR_Handle_obj_free(&MPII_Keyval_mem, p->keyval);
            }
        }

        MPIR_Handle_obj_free(&MPID_Attr_mem, p);

        p = new_p;
    }

    /* We must zero out the attribute list pointer or we could attempt to use it
     * later.  This normally can't happen because the communicator usually
     * disappears after a call to MPI_Comm_free.  But if the attribute keyval
     * has an associated delete function that returns an error then we don't
     * actually free the communicator despite having freed all the attributes
     * associated with the communicator.
     *
     * This function is also used for Win and Type objects, but the idea is the
     * same in those cases as well. */
    *attr = NULL;
    return mpi_errno;
}

/* Note: proxy functions assumed to be outside critical section. This allows them
 * to call MPI functions directly. E.g. CXX bindings.
 */

int
MPII_Attr_copy_c_proxy(MPI_Comm_copy_attr_function * user_function,
                       int handle,
                       int keyval,
                       void *extra_state,
                       MPIR_Attr_type attrib_type, void *attrib, void **attrib_copy, int *flag)
{
    void *attrib_val = NULL;
    int ret;

    /* Make sure that the attribute value is delieverd as a pointer */
    if (MPII_ATTR_KIND(attrib_type) == MPII_ATTR_KIND(MPIR_ATTR_INT)) {
        attrib_val = &attrib;
    } else {
        attrib_val = attrib;
    }
#ifndef BUILD_MPI_ABI
    ret = user_function(handle, keyval, extra_state, attrib_val, attrib_copy, flag);
#else
    ret = user_function(ABI_Handle_from_mpi(handle), keyval, extra_state, attrib_val,
                        attrib_copy, flag);
#endif

    return ret;
}


int
MPII_Attr_delete_c_proxy(MPI_Comm_delete_attr_function * user_function,
                         int handle,
                         int keyval, MPIR_Attr_type attrib_type, void *attrib, void *extra_state)
{
    void *attrib_val = NULL;
    int ret;

    /* Make sure that the attribute value is delieverd as a pointer */
    if (MPII_ATTR_KIND(attrib_type) == MPII_ATTR_KIND(MPIR_ATTR_INT))
        attrib_val = &attrib;
    else
        attrib_val = attrib;
#ifndef BUILD_MPI_ABI
    ret = user_function(handle, keyval, attrib_val, extra_state);
#else
    ret = user_function(ABI_Handle_from_mpi(handle), keyval, attrib_val, extra_state);
#endif

    return ret;
}

/* FIXME: Missing routine description */
void
MPII_Keyval_set_proxy(int keyval,
                      MPII_Attr_copy_proxy copy_proxy, MPII_Attr_delete_proxy delete_proxy)
{
    MPII_Keyval *keyval_ptr;
    MPII_Keyval_get_ptr(keyval, keyval_ptr);
    if (keyval_ptr == NULL)
        return;

    keyval_ptr->copyfn.proxy = copy_proxy;
    keyval_ptr->delfn.proxy = delete_proxy;
}
