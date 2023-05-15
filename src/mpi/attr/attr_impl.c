/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* MPI_Comm_free_keyval, MPI_Type_free_keyval, and MPI_Win_free_keyval share
 * the same routine. */
void MPIR_free_keyval(MPII_Keyval * keyval_ptr)
{
    int in_use;

    if (!keyval_ptr->was_freed) {
        keyval_ptr->was_freed = 1;
        MPII_Keyval_release_ref(keyval_ptr, &in_use);
        if (!in_use) {
            MPIR_Handle_obj_free(&MPII_Keyval_mem, keyval_ptr);
        }
    }
}

int MPIR_Comm_free_keyval_impl(MPII_Keyval * keyval_ptr)
{
    MPIR_free_keyval(keyval_ptr);
    return MPI_SUCCESS;
}

int MPIR_Type_free_keyval_impl(MPII_Keyval * keyval_ptr)
{
    MPIR_free_keyval(keyval_ptr);
    return MPI_SUCCESS;
}

int MPIR_Win_free_keyval_impl(MPII_Keyval * keyval_ptr)
{
    MPIR_free_keyval(keyval_ptr);
    return MPI_SUCCESS;
}

int MPIR_Comm_create_keyval_impl(MPI_Comm_copy_attr_function * comm_copy_attr_fn,
                                 MPI_Comm_delete_attr_function * comm_delete_attr_fn,
                                 int *comm_keyval, void *extra_state)
{
    int mpi_errno = MPI_SUCCESS;
    MPII_Keyval *keyval_ptr;

    keyval_ptr = (MPII_Keyval *) MPIR_Handle_obj_alloc(&MPII_Keyval_mem);
    MPIR_ERR_CHKANDJUMP(!keyval_ptr, mpi_errno, MPI_ERR_OTHER, "**nomem");

    /* Initialize the attribute dup function */
    if (!MPIR_Process.attr_dup) {
        MPIR_Process.attr_dup = MPIR_Attr_dup_list;
        MPIR_Process.attr_free = MPIR_Attr_delete_list;
    }

    /* The handle encodes the keyval kind.  Modify it to have the correct
     * field */
    keyval_ptr->handle = (keyval_ptr->handle & ~(0x03c00000)) | (MPIR_COMM << 22);
    MPIR_Object_set_ref(keyval_ptr, 1);
    keyval_ptr->was_freed = 0;
    keyval_ptr->kind = MPIR_COMM;
    keyval_ptr->extra_state = extra_state;
    keyval_ptr->copyfn.user_function = comm_copy_attr_fn;
    keyval_ptr->copyfn.proxy = MPII_Attr_copy_c_proxy;
    keyval_ptr->delfn.user_function = comm_delete_attr_fn;
    keyval_ptr->delfn.proxy = MPII_Attr_delete_c_proxy;

    MPIR_OBJ_PUBLISH_HANDLE(*comm_keyval, keyval_ptr->handle);

  fn_exit:
    return mpi_errno;
  fn_fail:

    goto fn_exit;
}

int MPIR_Type_create_keyval_impl(MPI_Type_copy_attr_function * type_copy_attr_fn,
                                 MPI_Type_delete_attr_function * type_delete_attr_fn,
                                 int *type_keyval, void *extra_state)
{
    int mpi_errno = MPI_SUCCESS;
    MPII_Keyval *keyval_ptr;

    keyval_ptr = (MPII_Keyval *) MPIR_Handle_obj_alloc(&MPII_Keyval_mem);
    MPIR_ERR_CHKANDJUMP(!keyval_ptr, mpi_errno, MPI_ERR_OTHER, "**nomem");

    /* Initialize the attribute dup function */
    if (!MPIR_Process.attr_dup) {
        MPIR_Process.attr_dup = MPIR_Attr_dup_list;
        MPIR_Process.attr_free = MPIR_Attr_delete_list;
    }

    /* The handle encodes the keyval kind.  Modify it to have the correct
     * field */
    keyval_ptr->handle = (keyval_ptr->handle & ~(0x03c00000)) | (MPIR_DATATYPE << 22);
    MPIR_Object_set_ref(keyval_ptr, 1);
    keyval_ptr->was_freed = 0;
    keyval_ptr->kind = MPIR_DATATYPE;
    keyval_ptr->extra_state = extra_state;
    keyval_ptr->copyfn.user_function = type_copy_attr_fn;
    keyval_ptr->copyfn.proxy = MPII_Attr_copy_c_proxy;
    keyval_ptr->delfn.user_function = type_delete_attr_fn;
    keyval_ptr->delfn.proxy = MPII_Attr_delete_c_proxy;

    /* Tell finalize to check for attributes on permanent types */
    MPII_Datatype_attr_finalize();

    MPIR_OBJ_PUBLISH_HANDLE(*type_keyval, keyval_ptr->handle);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Win_create_keyval_impl(MPI_Win_copy_attr_function * win_copy_attr_fn,
                                MPI_Win_delete_attr_function * win_delete_attr_fn,
                                int *win_keyval, void *extra_state)
{
    int mpi_errno = MPI_SUCCESS;
    MPII_Keyval *keyval_ptr;

    keyval_ptr = (MPII_Keyval *) MPIR_Handle_obj_alloc(&MPII_Keyval_mem);
    MPIR_ERR_CHKANDJUMP(!keyval_ptr, mpi_errno, MPI_ERR_OTHER, "**nomem");

    /* Initialize the attribute dup function */
    if (!MPIR_Process.attr_dup) {
        MPIR_Process.attr_dup = MPIR_Attr_dup_list;
        MPIR_Process.attr_free = MPIR_Attr_delete_list;
    }

    /* The handle encodes the keyval kind.  Modify it to have the correct
     * field */
    keyval_ptr->handle = (keyval_ptr->handle & ~(0x03c00000)) | (MPIR_WIN << 22);
    MPIR_Object_set_ref(keyval_ptr, 1);
    keyval_ptr->was_freed = 0;
    keyval_ptr->kind = MPIR_WIN;
    keyval_ptr->extra_state = extra_state;
    keyval_ptr->copyfn.user_function = win_copy_attr_fn;
    keyval_ptr->copyfn.proxy = MPII_Attr_copy_c_proxy;
    keyval_ptr->delfn.user_function = win_delete_attr_fn;
    keyval_ptr->delfn.proxy = MPII_Attr_delete_c_proxy;

    MPIR_OBJ_PUBLISH_HANDLE(*win_keyval, keyval_ptr->handle);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* Find the requested attribute.  If it exists, return either the attribute
   entry or the address of the entry, based on whether the request is for
   a pointer-valued attribute (C or C++) or an integer-valued attribute
   (Fortran, either 77 or 90).

   If the attribute has the same type as the request, it is returned as-is.
   Otherwise, the address of the attribute is returned.
*/
int MPIR_Comm_get_attr_impl(MPIR_Comm * comm_ptr, int comm_keyval, void *attribute_val,
                            int *flag, MPIR_Attr_type outAttrType)
{
    int mpi_errno = MPI_SUCCESS;
    static PreDefined_attrs attr_copy;  /* Used to provide a copy of the
                                         * predefined attributes */
    MPIR_FUNC_ENTER;

    /* Check for builtin attribute */
    /* This code is ok for correct programs, but it would be better
     * to copy the values from the per-process block and pass the user
     * a pointer to a copy */
    /* Note that if we are called from Fortran, we must return the values,
     * not the addresses, of these attributes */
    if (HANDLE_IS_BUILTIN(comm_keyval)) {
        int attr_idx = comm_keyval & 0x0000000f;
        void **attr_val_p = (void **) attribute_val;
#ifdef HAVE_FORTRAN_BINDING
        /* This is an address-sized int instead of a Fortran (MPI_Fint)
         * integer because, even for the Fortran keyvals, the C interface is
         * used which stores the result in a pointer (hence we need a
         * pointer-sized int).  Thus we use intptr_t instead of MPI_Fint.
         * On some 64-bit platforms, such as Solaris-SPARC, using an MPI_Fint
         * will cause the value to placed into the high, rather than low,
         * end of the output value. */
#endif
        *flag = 1;

        /* FIXME : We could initialize some of these here; only tag_ub is
         * used in the error checking. */
        /*
         * The C versions of the attributes return the address of a
         * *COPY* of the value (to prevent the user from changing it)
         * and the Fortran versions provide the actual value (as an Fint)
         */
        attr_copy = MPIR_Process.attrs;
        switch (attr_idx) {
            case 1:    /* TAG_UB */
            case 2:
                *attr_val_p = &attr_copy.tag_ub;
                break;
            case 3:    /* HOST */
            case 4:
                *attr_val_p = &attr_copy.host;
                break;
            case 5:    /* IO */
            case 6:
                *attr_val_p = &attr_copy.io;
                break;
            case 7:    /* WTIME */
            case 8:
                *attr_val_p = &attr_copy.wtime_is_global;
                break;
            case 9:    /* UNIVERSE_SIZE */
            case 10:
                /* This is a special case.  If universe is not set, then we
                 * attempt to get it from the device.  If the device is doesn't
                 * supply a value, then we set the flag accordingly.  Note that
                 * we must initialize the cached value, in MPIR_Process.attr,
                 * not the value in the copy, since that is vulnerable to being
                 * overwritten by the user, and because we always re-initialize
                 * the copy from the cache (yes, there was a time when the code
                 * did the wrong thing here). */
                if (attr_copy.universe >= 0) {
                    *attr_val_p = &attr_copy.universe;
                } else if (attr_copy.universe == MPIR_UNIVERSE_SIZE_NOT_AVAILABLE) {
                    *flag = 0;
                } else {
                    /* We call MPID_Get_universe_size only once (see 10.5.1).
                     * Thus, we must put the value into the "main" copy */
                    mpi_errno = MPID_Get_universe_size(&MPIR_Process.attrs.universe);
                    /* --BEGIN ERROR HANDLING-- */
                    if (mpi_errno != MPI_SUCCESS) {
                        MPIR_Process.attrs.universe = MPIR_UNIVERSE_SIZE_NOT_AVAILABLE;
                        goto fn_fail;
                    }
                    /* --END ERROR HANDLING-- */
                    attr_copy.universe = MPIR_Process.attrs.universe;
                    if (attr_copy.universe >= 0) {
                        *attr_val_p = &attr_copy.universe;
                    } else {
                        *flag = 0;
                    }
                }
                break;
            case 11:   /* LASTUSEDCODE */
            case 12:
                *attr_val_p = &attr_copy.lastusedcode;
                break;
            case 13:   /* APPNUM */
            case 14:
                /* This is another special case.  If appnum is negative,
                 * we take that as indicating no value of APPNUM, and set
                 * the flag accordingly */
                if (attr_copy.appnum < 0) {
                    *flag = 0;
                } else {
                    *attr_val_p = &attr_copy.appnum;
                }
                break;
        }
        /* All of the predefined attributes are stored internally as C ints;
         * since we've set the output value as the pointer to these, we need
         * to dereference it here. We must also be very careful of the 3
         * different output cases, since the two Fortran cases correspond
         * to MPI_Fint and MPIR_FAint (an internal MPICH typedef for C
         * version of INTEGER (KIND=MPI_ADDRESS_KIND)) */
        if (*flag) {
            /* Use the internal pointer-sized-int for systems (e.g., BG/P)
             * that define MPI_Aint as a different size than intptr_t.
             * The casts must be as they are:
             * On the right, the value is a pointer to an int, so to
             * get the correct value, we need to extract the int.
             * On the left, the output type is given by the argument
             * outAttrType - and the cast must match the intended results */
            /* FIXME: This code is broken.  The MPIR_ATTR_INT is for Fortran
             * MPI_Fint types, not int, and MPIR_ATTR_AINT is for Fortran
             * INTEGER(KIND=MPI_ADDRESS_KIND), which is probably an MPI_Aint,
             * and intptr_t is for exactly the case where MPI_Aint is not
             * the same as intptr_t.
             * This code needs to be fixed in every place that it occurs
             * (i.e., see the win and type get_attr routines). */
            if (outAttrType == MPIR_ATTR_AINT)
                *(intptr_t *) attr_val_p = *(int *) *(void **) attr_val_p;
            else if (outAttrType == MPIR_ATTR_INT) {
                /* *(int*)attr_val_p = *(int *)*(void **)attr_val_p; */
                /* This is correct, because the corresponding code
                 * in the Fortran interface expects to find a pointer-length
                 * integer value.  Thus, this works for both big and little
                 * endian systems. Any changes made here must have
                 * corresponding changes in src/binding/f77/attr_getf.c ,
                 * which is generated by src/binding/f77/buildiface . */
                *(intptr_t *) attr_val_p = *(int *) *(void **) attr_val_p;
            }
        }
    } else {
        MPIR_Attribute *p = comm_ptr->attributes;

        /*   */
        *flag = 0;
        while (p) {
            if (p->keyval->handle == comm_keyval) {
                *flag = 1;
                if (outAttrType == MPIR_ATTR_PTR) {
                    if (p->attrType == MPIR_ATTR_INT) {
                        /* This is the tricky case: if the system is
                         * bigendian, and we have to return a pointer to
                         * an int, then we may need to point to the
                         * correct location in the word. */
#if defined(WORDS_LITTLEENDIAN) || (SIZEOF_VOID_P == SIZEOF_INT)
                        *(void **) attribute_val = &(p->value);
#else
                        int *p_loc = (int *) &(p->value);
#if SIZEOF_VOID_P == 2 * SIZEOF_INT
                        p_loc++;
#else
#error Expected sizeof(void*) to be either sizeof(int) or 2*sizeof(int)
#endif
                        *(void **) attribute_val = p_loc;
#endif
                    } else if (p->attrType == MPIR_ATTR_AINT) {
                        *(void **) attribute_val = &(p->value);
                    } else {
                        *(void **) attribute_val = (void *) (intptr_t) (p->value);
                    }
                } else {
                    *(void **) attribute_val = (void *) (intptr_t) (p->value);
                }

                break;
            }
            p = p->next;
        }
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Comm_set_attr_impl(MPIR_Comm * comm_ptr, MPII_Keyval * keyval_ptr, void *attribute_val,
                            MPIR_Attr_type attrType)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Attribute *p;

    /* CHANGE FOR MPI 2.2:  Look for attribute.  They are ordered by when they
     * were added, with the most recent first. This uses
     * a simple linear list algorithm because few applications use more than a
     * handful of attributes */

    /* printf("Setting attr val to %x\n", attribute_val); */
    p = comm_ptr->attributes;
    while (p) {
        if (p->keyval->handle == keyval_ptr->handle) {
            /* If found, call the delete function before replacing the
             * attribute */
            mpi_errno = MPIR_Call_attr_delete(comm_ptr->handle, p);
            if (mpi_errno) {
                goto fn_fail;
            }
            p->attrType = attrType;
            /* FIXME: This code is incorrect in some cases, particularly
             * in the case where intptr_t is different from MPI_Aint,
             * since in that case, the Fortran 9x interface will provide
             * more bytes in the attribute_val than this allows. The
             * dual casts are a sign that this is faulty. This will
             * need to be fixed in the type/win set_attr routines as
             * well. */
            p->value = (MPII_Attr_val_t) (intptr_t) attribute_val;
            /* printf("Updating attr at %x\n", &p->value); */
            /* Does not change the reference count on the keyval */
            break;
        }
        p = p->next;
    }
    /* CHANGE FOR MPI 2.2: If not found, add at the beginning */
    if (!p) {
        MPIR_Attribute *new_p = MPID_Attr_alloc();
        MPIR_ERR_CHKANDJUMP(!new_p, mpi_errno, MPI_ERR_OTHER, "**nomem");
        /* Did not find in list.  Add at end */
        new_p->keyval = keyval_ptr;
        new_p->attrType = attrType;
        new_p->pre_sentinal = 0;
        /* FIXME: See the comment above on this dual cast. */
        new_p->value = (MPII_Attr_val_t) (intptr_t) attribute_val;
        new_p->post_sentinal = 0;
        new_p->next = comm_ptr->attributes;
        MPII_Keyval_add_ref(keyval_ptr);
        comm_ptr->attributes = new_p;
        /* printf("Creating attr at %x\n", &new_p->value); */
    }

    /* Here is where we could add a hook for the device to detect attribute
     * value changes, using something like
     * MPID_Comm_attr_hook(comm_ptr, keyval, attribute_val);
     */


  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static void delete_attr(MPIR_Attribute ** attributes_list, MPIR_Attribute * attr)
{
    MPIR_Attribute *p, **old_p;

    p = *attributes_list;
    old_p = attributes_list;

    while (p) {
        if (p == attr) {
            *old_p = p->next;

            int in_use;
            MPII_Keyval_release_ref(p->keyval, &in_use);
            if (!in_use) {
                MPIR_Handle_obj_free(&MPII_Keyval_mem, p->keyval);
            }
            MPID_Attr_free(p);
            break;
        }
        old_p = &p->next;
        p = p->next;
    }
}

int MPIR_Comm_delete_attr_impl(MPIR_Comm * comm_ptr, MPII_Keyval * keyval_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Attribute *p;

    /* Look for attribute.  They are ordered by keyval handle */

    p = comm_ptr->attributes;
    while (p) {
        if (p->keyval->handle == keyval_ptr->handle) {
            break;
        }
        p = p->next;
    }

    /* We can't unlock yet, because we must not free the attribute until
     * we know whether the delete function has returned with a 0 status
     * code */

    if (p) {
        /* Run the delete function, if any, and then free the
         * attribute storage.  Note that due to an ambiguity in the
         * standard, if the usr function returns something other than
         * MPI_SUCCESS, we should either return the user return code,
         * or an mpich error code.  The precedent set by the Intel
         * test suite says we should return the user return code.  So
         * we must not ERR_POP here. */
        mpi_errno = MPIR_Call_attr_delete(comm_ptr->handle, p);
        if (mpi_errno)
            goto fn_fail;

        /* NOTE: it's incorrect to remove p by its parent pointer because the delete function
         *       may have invalidated the parent pointer, e.g. by removing the parent attribute */
        delete_attr(&comm_ptr->attributes, p);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Type_get_attr_impl(MPIR_Datatype * type_ptr, int type_keyval, void *attribute_val,
                            int *flag, MPIR_Attr_type outAttrType)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Attribute *p;
    MPIR_FUNC_ENTER;

    /* ... body of routine ...  */

    *flag = 0;
    p = type_ptr->attributes;
    while (p) {
        if (p->keyval->handle == type_keyval) {
            *flag = 1;
            if (outAttrType == MPIR_ATTR_PTR) {
                if (p->attrType == MPIR_ATTR_INT) {
                    /* This is the tricky case: if the system is
                     * bigendian, and we have to return a pointer to
                     * an int, then we may need to point to the
                     * correct location in the word. */
#if defined(WORDS_LITTLEENDIAN) || (SIZEOF_VOID_P == SIZEOF_INT)
                    *(void **) attribute_val = &(p->value);
#else
                    int *p_loc = (int *) &(p->value);
#if SIZEOF_VOID_P == 2 * SIZEOF_INT
                    p_loc++;
#else
#error Expected sizeof(void*) to be either sizeof(int) or 2*sizeof(int)
#endif
                    *(void **) attribute_val = p_loc;
#endif
                } else if (p->attrType == MPIR_ATTR_AINT) {
                    *(void **) attribute_val = &(p->value);
                } else {
                    *(void **) attribute_val = (void *) (intptr_t) (p->value);
                }
            } else
                *(void **) attribute_val = (void *) (intptr_t) (p->value);

            break;
        }
        p = p->next;
    }

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

int MPIR_Type_set_attr_impl(MPIR_Datatype * type_ptr, MPII_Keyval * keyval_ptr, void *attribute_val,
                            MPIR_Attr_type attrType)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Attribute *p, **old_p;

    /* Look for attribute.  They are ordered by keyval handle.  This uses
     * a simple linear list algorithm because few applications use more than a
     * handful of attributes */

    old_p = &type_ptr->attributes;
    p = type_ptr->attributes;
    while (p) {
        if (p->keyval->handle == keyval_ptr->handle) {
            /* If found, call the delete function before replacing the
             * attribute */
            mpi_errno = MPIR_Call_attr_delete(type_ptr->handle, p);
            /* --BEGIN ERROR HANDLING-- */
            if (mpi_errno) {
                goto fn_fail;
            }
            /* --END ERROR HANDLING-- */
            p->value = (MPII_Attr_val_t) (intptr_t) attribute_val;
            p->attrType = attrType;
            break;
        } else if (p->keyval->handle > keyval_ptr->handle) {
            MPIR_Attribute *new_p = MPID_Attr_alloc();
            MPIR_ERR_CHKANDJUMP1(!new_p, mpi_errno, MPI_ERR_OTHER,
                                 "**nomem", "**nomem %s", "MPIR_Attribute");
            new_p->keyval = keyval_ptr;
            new_p->attrType = attrType;
            new_p->pre_sentinal = 0;
            new_p->value = (MPII_Attr_val_t) (intptr_t) attribute_val;
            new_p->post_sentinal = 0;
            new_p->next = p->next;
            MPII_Keyval_add_ref(keyval_ptr);
            p->next = new_p;
            break;
        }
        old_p = &p->next;
        p = p->next;
    }
    if (!p) {
        MPIR_Attribute *new_p = MPID_Attr_alloc();
        MPIR_ERR_CHKANDJUMP1(!new_p, mpi_errno, MPI_ERR_OTHER,
                             "**nomem", "**nomem %s", "MPIR_Attribute");
        /* Did not find in list.  Add at end */
        new_p->keyval = keyval_ptr;
        new_p->attrType = attrType;
        new_p->pre_sentinal = 0;
        new_p->value = (MPII_Attr_val_t) (intptr_t) attribute_val;
        new_p->post_sentinal = 0;
        new_p->next = 0;
        MPII_Keyval_add_ref(keyval_ptr);
        *old_p = new_p;
    }

    /* Here is where we could add a hook for the device to detect attribute
     * value changes, using something like
     * MPID_Type_attr_hook(type_ptr, keyval, attribute_val);
     */

    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Type_delete_attr_impl(MPIR_Datatype * type_ptr, MPII_Keyval * keyval_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Attribute *p;

    /* Look for attribute.  They are ordered by keyval handle */

    p = type_ptr->attributes;
    while (p) {
        if (p->keyval->handle == keyval_ptr->handle) {
            break;
        }
        p = p->next;
    }

    /* We can't unlock yet, because we must not free the attribute until
     * we know whether the delete function has returned with a 0 status
     * code */

    if (p) {
        /* Run the delete function, if any, and then free the
         * attribute storage.  Note that due to an ambiguity in the
         * standard, if the usr function returns something other than
         * MPI_SUCCESS, we should either return the user return code,
         * or an mpich error code.  The precedent set by the Intel
         * test suite says we should return the user return code.  So
         * we must not ERR_POP here. */
        mpi_errno = MPIR_Call_attr_delete(type_ptr->handle, p);
        if (mpi_errno)
            goto fn_fail;

        delete_attr(&type_ptr->attributes, p);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Win_get_attr_impl(MPIR_Win * win_ptr, int win_keyval, void *attribute_val,
                           int *flag, MPIR_Attr_type outAttrType)
{
    int mpi_errno = MPI_SUCCESS;

    /* Check for builtin attribute */
    /* This code is ok for correct programs, but it would be better
     * to copy the values from the per-process block and pass the user
     * a pointer to a copy */
    /* Note that if we are called from Fortran, we must return the values,
     * not the addresses, of these attributes */
    if (HANDLE_IS_BUILTIN(win_keyval)) {
        void **attr_val_p = (void **) attribute_val;
#ifdef HAVE_FORTRAN_BINDING
        /* Note that this routine only has a Fortran 90 binding,
         * so the attribute value is an address-sized int */
        intptr_t *attr_int = (intptr_t *) attribute_val;
#endif
        *flag = 1;

        /*
         * The C versions of the attributes return the address of a
         * *COPY* of the value (to prevent the user from changing it)
         * and the Fortran versions provide the actual value (as a Fint)
         */
        switch (win_keyval) {
            case MPI_WIN_BASE:
                *attr_val_p = win_ptr->base;
                break;
            case MPI_WIN_SIZE:
                win_ptr->copySize = win_ptr->size;
                *attr_val_p = &win_ptr->copySize;
                break;
            case MPI_WIN_DISP_UNIT:
                win_ptr->copyDispUnit = win_ptr->disp_unit;
                *attr_val_p = &win_ptr->copyDispUnit;
                break;
            case MPI_WIN_CREATE_FLAVOR:
                win_ptr->copyCreateFlavor = win_ptr->create_flavor;
                *attr_val_p = &win_ptr->copyCreateFlavor;
                break;
            case MPI_WIN_MODEL:
                win_ptr->copyModel = win_ptr->model;
                *attr_val_p = &win_ptr->copyModel;
                break;
#ifdef HAVE_FORTRAN_BINDING
            case MPII_ATTR_C_TO_FORTRAN(MPI_WIN_BASE):
                /* The Fortran routine that matches this routine should
                 * provide an address-sized integer, not an MPI_Fint */
                *attr_int = (MPI_Aint) win_ptr->base;
                break;
            case MPII_ATTR_C_TO_FORTRAN(MPI_WIN_SIZE):
                /* We do not need to copy because we return the value,
                 * not a pointer to the value */
                *attr_int = win_ptr->size;
                break;
            case MPII_ATTR_C_TO_FORTRAN(MPI_WIN_DISP_UNIT):
                /* We do not need to copy because we return the value,
                 * not a pointer to the value */
                *attr_int = win_ptr->disp_unit;
                break;
            case MPII_ATTR_C_TO_FORTRAN(MPI_WIN_CREATE_FLAVOR):
                *attr_int = win_ptr->create_flavor;
                break;
            case MPII_ATTR_C_TO_FORTRAN(MPI_WIN_MODEL):
                *attr_int = win_ptr->model;
                break;
#endif
            default:
                MPIR_Assert(FALSE);
                break;
        }
    } else {
        MPIR_Attribute *p = win_ptr->attributes;

        *flag = 0;
        while (p) {
            if (p->keyval->handle == win_keyval) {
                *flag = 1;
                if (outAttrType == MPIR_ATTR_PTR) {
                    if (p->attrType == MPIR_ATTR_INT) {
                        /* This is the tricky case: if the system is
                         * bigendian, and we have to return a pointer to
                         * an int, then we may need to point to the
                         * correct location in the word. */
#if defined(WORDS_LITTLEENDIAN) || (SIZEOF_VOID_P == SIZEOF_INT)
                        *(void **) attribute_val = &(p->value);
#else
                        int *p_loc = (int *) &(p->value);
#if SIZEOF_VOID_P == 2 * SIZEOF_INT
                        p_loc++;
#else
#error Expected sizeof(void*) to be either sizeof(int) or 2*sizeof(int)
#endif
                        *(void **) attribute_val = p_loc;
#endif
                    } else if (p->attrType == MPIR_ATTR_AINT) {
                        *(void **) attribute_val = &(p->value);
                    } else {
                        *(void **) attribute_val = (void *) (intptr_t) (p->value);
                    }
                } else
                    *(void **) attribute_val = (void *) (intptr_t) (p->value);

                break;
            }
            p = p->next;
        }
    }

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

int MPIR_Win_set_attr_impl(MPIR_Win * win_ptr, MPII_Keyval * keyval_ptr, void *attribute_val,
                           MPIR_Attr_type attrType)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Attribute *p, **old_p;

    /* Look for attribute.  They are ordered by keyval handle.  This uses
     * a simple linear list algorithm because few applications use more than a
     * handful of attributes */

    old_p = &win_ptr->attributes;
    p = win_ptr->attributes;
    while (p) {
        if (p->keyval->handle == keyval_ptr->handle) {
            /* If found, call the delete function before replacing the
             * attribute */
            mpi_errno = MPIR_Call_attr_delete(win_ptr->handle, p);
            /* --BEGIN ERROR HANDLING-- */
            if (mpi_errno) {
                /* FIXME : communicator of window? */
                goto fn_fail;
            }
            /* --END ERROR HANDLING-- */
            p->value = (MPII_Attr_val_t) (intptr_t) attribute_val;
            p->attrType = attrType;
            /* Does not change the reference count on the keyval */
            break;
        } else if (p->keyval->handle > keyval_ptr->handle) {
            MPIR_Attribute *new_p = MPID_Attr_alloc();
            MPIR_ERR_CHKANDJUMP1(!new_p, mpi_errno, MPI_ERR_OTHER,
                                 "**nomem", "**nomem %s", "MPIR_Attribute");
            new_p->keyval = keyval_ptr;
            new_p->attrType = attrType;
            new_p->pre_sentinal = 0;
            new_p->value = (MPII_Attr_val_t) (intptr_t) attribute_val;
            new_p->post_sentinal = 0;
            new_p->next = p->next;
            MPII_Keyval_add_ref(keyval_ptr);
            p->next = new_p;
            break;
        }
        old_p = &p->next;
        p = p->next;
    }
    if (!p) {
        MPIR_Attribute *new_p = MPID_Attr_alloc();
        MPIR_ERR_CHKANDJUMP1(!new_p, mpi_errno, MPI_ERR_OTHER,
                             "**nomem", "**nomem %s", "MPIR_Attribute");
        /* Did not find in list.  Add at end */
        new_p->attrType = attrType;
        new_p->keyval = keyval_ptr;
        new_p->pre_sentinal = 0;
        new_p->value = (MPII_Attr_val_t) (intptr_t) attribute_val;
        new_p->post_sentinal = 0;
        new_p->next = 0;
        MPII_Keyval_add_ref(keyval_ptr);
        *old_p = new_p;
    }

    /* Here is where we could add a hook for the device to detect attribute
     * value changes, using something like
     * MPID_Win_attr_hook(win_ptr, keyval, attribute_val);
     */

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

int MPIR_Win_delete_attr_impl(MPIR_Win * win_ptr, MPII_Keyval * keyval_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Attribute *p;

    /* Look for attribute.  They are ordered by keyval handle */

    p = win_ptr->attributes;
    while (p) {
        if (p->keyval->handle == keyval_ptr->handle) {
            break;
        }
        p = p->next;
    }

    /* We can't unlock yet, because we must not free the attribute until
     * we know whether the delete function has returned with a 0 status
     * code */

    if (p) {
        /* Run the delete function, if any, and then free the
         * attribute storage.  Note that due to an ambiguity in the
         * standard, if the usr function returns something other than
         * MPI_SUCCESS, we should either return the user return code,
         * or an mpich error code.  The precedent set by the Intel
         * test suite says we should return the user return code.  So
         * we must not ERR_POP here. */
        mpi_errno = MPIR_Call_attr_delete(win_ptr->handle, p);
        if (mpi_errno)
            goto fn_fail;

        delete_attr(&win_ptr->attributes, p);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
