/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* MPI_Comm_free_keyval, MPI_Type_free_keyval, and MPI_Win_free_keyval share
 * the same routine. */
void MPIR_free_keyval(MPII_Keyval * keyval_ptr)
{
    if (!keyval_ptr->was_freed) {
        keyval_ptr->was_freed = 1;
        MPIR_Keyval_release(keyval_ptr);
    }
}

int MPIR_Comm_free_keyval_impl(MPII_Keyval * keyval_ptr)
{
    MPIR_FUNC_ENTER;

    MPIR_free_keyval(keyval_ptr);

    MPIR_FUNC_EXIT;
    return MPI_SUCCESS;
}

int MPIR_Type_free_keyval_impl(MPII_Keyval * keyval_ptr)
{
    MPIR_FUNC_ENTER;

    MPIR_free_keyval(keyval_ptr);

    MPIR_FUNC_EXIT;
    return MPI_SUCCESS;
}

int MPIR_Win_free_keyval_impl(MPII_Keyval * keyval_ptr)
{
    MPIR_FUNC_ENTER;

    MPIR_free_keyval(keyval_ptr);

    MPIR_FUNC_EXIT;
    return MPI_SUCCESS;
}

static int create_keyval_x(MPI_Comm_copy_attr_function * copy_fn,
                           MPI_Comm_delete_attr_function * delete_fn,
                           MPIX_Destructor_function * destructor_fn,
                           int *keyval, void *extra_state, MPII_Object_kind kind)
{
    int mpi_errno = MPI_SUCCESS;
    MPII_Keyval *keyval_ptr;

    MPIR_FUNC_ENTER;

    keyval_ptr = (MPII_Keyval *) MPIR_Handle_obj_alloc(&MPII_Keyval_mem);
    MPIR_ERR_CHKANDJUMP(!keyval_ptr, mpi_errno, MPI_ERR_OTHER, "**nomem");

    /* Initialize the attribute dup function */
    if (!MPIR_Process.attr_dup) {
        MPIR_Process.attr_dup = MPIR_Attr_dup_list;
        MPIR_Process.attr_free = MPIR_Attr_delete_list;
    }

    /* The handle encodes the keyval kind.  Modify it to have the correct
     * field */
    keyval_ptr->handle = (keyval_ptr->handle & ~(0x03c00000)) | (kind << 22);
    MPIR_Object_set_ref(keyval_ptr, 1);
    keyval_ptr->was_freed = 0;
    keyval_ptr->kind = kind;
    keyval_ptr->extra_state = extra_state;
    keyval_ptr->copyfn.user_function = copy_fn;
    keyval_ptr->copyfn.proxy = MPII_Attr_copy_c_proxy;
    keyval_ptr->delfn.user_function = delete_fn;
    keyval_ptr->delfn.proxy = MPII_Attr_delete_c_proxy;
    keyval_ptr->destructor_fn = destructor_fn;

    MPIR_OBJ_PUBLISH_HANDLE(*keyval, keyval_ptr->handle);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:

    goto fn_exit;
}

int MPIR_Comm_create_keyval_x_impl(MPI_Comm_copy_attr_function * comm_copy_attr_fn,
                                   MPI_Comm_delete_attr_function * comm_delete_attr_fn,
                                   MPIX_Destructor_function * destructor_fn,
                                   int *comm_keyval, void *extra_state)
{
#ifdef BUILD_MPI_ABI
    if (comm_copy_attr_fn == MPI_COMM_NULL_COPY_FN) {
        comm_copy_attr_fn = NULL;
    } else if (comm_copy_attr_fn == MPI_COMM_DUP_FN) {
        comm_copy_attr_fn = MPIR_Comm_dup_fn;
    }
    if (comm_delete_attr_fn == MPI_COMM_NULL_DELETE_FN) {
        comm_delete_attr_fn = NULL;
    }
#endif

    return create_keyval_x(comm_copy_attr_fn, comm_delete_attr_fn, destructor_fn,
                           comm_keyval, extra_state, MPIR_COMM);
}

int MPIR_Type_create_keyval_x_impl(MPI_Type_copy_attr_function * type_copy_attr_fn,
                                   MPI_Type_delete_attr_function * type_delete_attr_fn,
                                   MPIX_Destructor_function * destructor_fn,
                                   int *type_keyval, void *extra_state)
{
#ifdef BUILD_MPI_ABI
    if (type_copy_attr_fn == MPI_TYPE_NULL_COPY_FN) {
        type_copy_attr_fn = NULL;
    } else if (type_copy_attr_fn == MPI_TYPE_DUP_FN) {
        type_copy_attr_fn = MPIR_Type_dup_fn;
    }
    if (type_delete_attr_fn == MPI_TYPE_NULL_DELETE_FN) {
        type_delete_attr_fn = NULL;
    }
#endif

    /* Tell finalize to check for attributes on permanent types */
    MPII_Datatype_attr_finalize();

    return create_keyval_x(type_copy_attr_fn, type_delete_attr_fn, destructor_fn,
                           type_keyval, extra_state, MPIR_DATATYPE);
}

int MPIR_Win_create_keyval_x_impl(MPI_Win_copy_attr_function * win_copy_attr_fn,
                                  MPI_Win_delete_attr_function * win_delete_attr_fn,
                                  MPIX_Destructor_function * destructor_fn,
                                  int *win_keyval, void *extra_state)
{
#ifdef BUILD_MPI_ABI
    if (win_copy_attr_fn == MPI_WIN_NULL_COPY_FN) {
        win_copy_attr_fn = NULL;
    } else if (win_copy_attr_fn == MPI_WIN_DUP_FN) {
        win_copy_attr_fn = MPIR_Win_dup_fn;
    }
    if (win_delete_attr_fn == MPI_WIN_NULL_DELETE_FN) {
        win_delete_attr_fn = NULL;
    }
#endif

    return create_keyval_x(win_copy_attr_fn, win_delete_attr_fn, destructor_fn,
                           win_keyval, extra_state, MPIR_WIN);
}

int MPIR_Comm_create_keyval_impl(MPI_Comm_copy_attr_function * comm_copy_attr_fn,
                                 MPI_Comm_delete_attr_function * comm_delete_attr_fn,
                                 int *comm_keyval, void *extra_state)
{
    return MPIR_Comm_create_keyval_x_impl(comm_copy_attr_fn, comm_delete_attr_fn, NULL,
                                          comm_keyval, extra_state);
}

int MPIR_Type_create_keyval_impl(MPI_Type_copy_attr_function * type_copy_attr_fn,
                                 MPI_Type_delete_attr_function * type_delete_attr_fn,
                                 int *type_keyval, void *extra_state)
{
    return MPIR_Type_create_keyval_x_impl(type_copy_attr_fn, type_delete_attr_fn, NULL,
                                          type_keyval, extra_state);
}

int MPIR_Win_create_keyval_impl(MPI_Win_copy_attr_function * win_copy_attr_fn,
                                MPI_Win_delete_attr_function * win_delete_attr_fn,
                                int *win_keyval, void *extra_state)
{
    return MPIR_Win_create_keyval_x_impl(win_copy_attr_fn, win_delete_attr_fn, NULL,
                                         win_keyval, extra_state);
}

/* Find the requested attribute.  If it exists, return either the attribute
   entry or the address of the entry, based on whether the request is for
   a pointer-valued attribute (C or C++) or an integer-valued attribute
   (Fortran, either 77 or 90).

   If the attribute has the same type as the request, it is returned as-is.
   Otherwise, the address of the attribute is returned.
*/
static int comm_get_attr(MPIR_Comm * comm_ptr, int comm_keyval, void *attribute_val, int *flag,
                         bool as_fortran)
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
        void **attr_val_p = (void **) attribute_val;
        *flag = 1;

        /* FIXME : We could initialize some of these here; only tag_ub is
         * used in the error checking. */
        /*
         * The C versions of the attributes return the address of a
         * *COPY* of the value (to prevent the user from changing it)
         * and the Fortran versions provide the actual value (as an Fint)
         */
        attr_copy = MPIR_Process.attrs;
        int *val_p = NULL;
        switch (comm_keyval) {
            case MPI_TAG_UB:
                val_p = &attr_copy.tag_ub;
                break;
            case MPI_HOST:
                val_p = &attr_copy.host;
                break;
            case MPI_IO:
                val_p = &attr_copy.io;
                break;
            case MPI_WTIME_IS_GLOBAL:
                val_p = &attr_copy.wtime_is_global;
                break;
            case MPI_UNIVERSE_SIZE:
                /* This is a special case.  If universe is not set, then we
                 * attempt to get it from the device.  If the device is doesn't
                 * supply a value, then we set the flag accordingly.  Note that
                 * we must initialize the cached value, in MPIR_Process.attr,
                 * not the value in the copy, since that is vulnerable to being
                 * overwritten by the user, and because we always re-initialize
                 * the copy from the cache (yes, there was a time when the code
                 * did the wrong thing here). */
                if (attr_copy.universe >= 0) {
                    val_p = &attr_copy.universe;
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
                        val_p = &attr_copy.universe;
                    } else {
                        *flag = 0;
                    }
                }
                break;
            case MPI_LASTUSEDCODE:
                val_p = &attr_copy.lastusedcode;
                break;
            case MPI_APPNUM:
                /* This is another special case.  If appnum is negative,
                 * we take that as indicating no value of APPNUM, and set
                 * the flag accordingly */
                if (attr_copy.appnum < 0) {
                    *flag = 0;
                } else {
                    val_p = &attr_copy.appnum;
                }
                break;
            default:
                MPIR_Assert(0);
                break;
        }
        if (*flag) {
            if (as_fortran) {
                /* Fortran retrieves scalar integer cast as void * */
                *attr_val_p = (void *) (intptr_t) (*val_p);
            } else {
                *attr_val_p = val_p;
            }
        }
    } else {
        MPIR_Attribute *p;
        p = comm_ptr->attributes;
#ifdef ENABLE_THREADCOMM
        if (comm_ptr->threadcomm) {
            MPIR_threadcomm_tls_t *tls = MPIR_threadcomm_get_tls(comm_ptr->threadcomm);
            MPIR_Assert(tls);
            p = tls->attributes;
        }
#endif

        /*   */
        *flag = 0;
        while (p) {
            if (p->keyval->handle == comm_keyval) {
                *flag = 1;
                if (p->as_fortran && !as_fortran) {
                    /* Fortran set as integer, C get a pointer to its internal storage */
                    *(void **) attribute_val = &(p->value);
                } else {
                    *(void **) attribute_val = p->value;
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

static int comm_set_attr(MPIR_Comm * comm_ptr, MPII_Keyval * keyval_ptr, void *attribute_val,
                         bool as_fortran)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Attribute *p, **old_p; /* old_p is needed if we are adding new attribute */

    MPIR_FUNC_ENTER;

    /* CHANGE FOR MPI 2.2:  Look for attribute.  They are ordered by when they
     * were added, with the most recent first. This uses
     * a simple linear list algorithm because few applications use more than a
     * handful of attributes */

    /* printf("Setting attr val to %x\n", attribute_val); */
    p = comm_ptr->attributes;
    old_p = &(comm_ptr->attributes);
#ifdef ENABLE_THREADCOMM
    if (comm_ptr->threadcomm) {
        MPIR_threadcomm_tls_t *tls = MPIR_threadcomm_get_tls(comm_ptr->threadcomm);
        MPIR_Assert(tls);
        p = tls->attributes;
        old_p = &(tls->attributes);
    }
#endif
    while (p) {
        if (p->keyval->handle == keyval_ptr->handle) {
            /* If found, call the delete function before replacing the
             * attribute. If the delete function fails on the old
             * attribute value, we keep the old value. */
            mpi_errno = MPIR_Call_attr_delete(comm_ptr->handle, p);
            if (mpi_errno != MPI_SUCCESS) {
                goto fn_fail;
            }
            p->value = attribute_val;
            p->as_fortran = as_fortran;
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
        new_p->pre_sentinal = 0;
        new_p->value = attribute_val;
        new_p->as_fortran = as_fortran;
        new_p->post_sentinal = 0;
        new_p->next = *old_p;
        MPII_Keyval_add_ref(keyval_ptr);
        *old_p = new_p;
        /* printf("Creating attr at %x\n", &new_p->value); */
    }

    /* Here is where we could add a hook for the device to detect attribute
     * value changes, using something like
     * MPID_Comm_attr_hook(comm_ptr, keyval, attribute_val);
     */


  fn_exit:
    MPIR_FUNC_EXIT;
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

            MPIR_Keyval_release(p->keyval);
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
    MPIR_Attribute *p, **attributes_list;

    MPIR_FUNC_ENTER;

    /* Look for attribute.  They are ordered by keyval handle */

    attributes_list = &comm_ptr->attributes;
#ifdef ENABLE_THREADCOMM
    MPIR_threadcomm_tls_t *tls = NULL;
    if (comm_ptr->threadcomm) {
        tls = MPIR_threadcomm_get_tls(comm_ptr->threadcomm);
        MPIR_Assert(tls);
        attributes_list = &tls->attributes;
    }
#endif
    p = *attributes_list;
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
         * test suite says we should return the user return code. So
         * we must not ERR_POP here. */
        mpi_errno = MPIR_Call_attr_delete(comm_ptr->handle, p);
        /* If the user delete function fails, we still remove the attribute from the list. */
        if (mpi_errno != MPI_SUCCESS)
            goto fn_fail;

        /* NOTE: it's incorrect to remove p by its parent pointer because the delete function
         *       may have invalidated the parent pointer, e.g. by removing the parent attribute */
        delete_attr(attributes_list, p);
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int type_get_attr(MPIR_Datatype * type_ptr, int type_keyval, void *attribute_val, int *flag,
                         bool as_fortran)
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
            if (p->as_fortran && !as_fortran) {
                /* Fortran set as integer, C get a pointer to its internal storage */
                *(void **) attribute_val = &(p->value);
            } else {
                *(void **) attribute_val = p->value;
            }

            break;
        }
        p = p->next;
    }

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

static int type_set_attr(MPIR_Datatype * type_ptr, MPII_Keyval * keyval_ptr, void *attribute_val,
                         bool as_fortran)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Attribute *p, **old_p;

    MPIR_FUNC_ENTER;

    /* Look for attribute.  They are ordered by keyval handle.  This uses
     * a simple linear list algorithm because few applications use more than a
     * handful of attributes */

    old_p = &type_ptr->attributes;
    p = type_ptr->attributes;
    while (p) {
        if (p->keyval->handle == keyval_ptr->handle) {
            /* If found, call the delete function before replacing the
             * attribute. If the delete function fails on the old
             * attribute value, we keep the old value. */
            mpi_errno = MPIR_Call_attr_delete(type_ptr->handle, p);
            if (mpi_errno != MPI_SUCCESS) {
                goto fn_fail;
            }
            p->value = attribute_val;
            p->as_fortran = as_fortran;
            /* Does not change the reference count on the keyval */
            break;
        } else if (p->keyval->handle > keyval_ptr->handle) {
            MPIR_Attribute *new_p = MPID_Attr_alloc();
            MPIR_ERR_CHKANDJUMP1(!new_p, mpi_errno, MPI_ERR_OTHER,
                                 "**nomem", "**nomem %s", "MPIR_Attribute");
            new_p->keyval = keyval_ptr;
            new_p->pre_sentinal = 0;
            new_p->value = attribute_val;
            new_p->as_fortran = as_fortran;
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
        new_p->pre_sentinal = 0;
        new_p->value = attribute_val;
        new_p->as_fortran = as_fortran;
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

    MPIR_FUNC_ENTER;

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
        /* If the user delete function fails, we still remove the attribute from the list. */
        if (mpi_errno != MPI_SUCCESS)
            goto fn_fail;

        /* NOTE: it's incorrect to remove p by its parent pointer because the delete function
         *       may have invalidated the parent pointer, e.g. by removing the parent attribute */
        delete_attr(&type_ptr->attributes, p);
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int win_get_attr(MPIR_Win * win_ptr, int win_keyval, void *attribute_val, int *flag,
                        bool as_fortran)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    /* Check for builtin attribute */
    /* This code is ok for correct programs, but it would be better
     * to copy the values from the per-process block and pass the user
     * a pointer to a copy */
    /* Note that if we are called from Fortran, we must return the values,
     * not the addresses, of these attributes */
    if (HANDLE_IS_BUILTIN(win_keyval)) {
        void **attr_val_p = (void **) attribute_val;
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
                if (as_fortran) {
                    *attr_val_p = (void *) (intptr_t) win_ptr->copySize;
                } else {
                    *attr_val_p = &win_ptr->copySize;
                }
                break;
            case MPI_WIN_DISP_UNIT:
                win_ptr->copyDispUnit = win_ptr->disp_unit;
                if (as_fortran) {
                    *attr_val_p = (void *) (intptr_t) win_ptr->copyDispUnit;
                } else {
                    *attr_val_p = &win_ptr->copyDispUnit;
                }
                break;
            case MPI_WIN_CREATE_FLAVOR:
                win_ptr->copyCreateFlavor = win_ptr->create_flavor;
                if (as_fortran) {
                    *attr_val_p = (void *) (intptr_t) win_ptr->copyCreateFlavor;
                } else {
                    *attr_val_p = &win_ptr->copyCreateFlavor;
                }
                break;
            case MPI_WIN_MODEL:
                win_ptr->copyModel = win_ptr->model;
                if (as_fortran) {
                    *attr_val_p = (void *) (intptr_t) win_ptr->copyModel;
                } else {
                    *attr_val_p = &win_ptr->copyModel;
                }
                break;
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
                if (p->as_fortran && !as_fortran) {
                    /* Fortran set as integer, C get a pointer to its internal storage */
                    *(void **) attribute_val = &(p->value);
                } else {
                    *(void **) attribute_val = p->value;
                }

                break;
            }
            p = p->next;
        }
    }

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

static int win_set_attr(MPIR_Win * win_ptr, MPII_Keyval * keyval_ptr, void *attribute_val,
                        bool as_fortran)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Attribute *p, **old_p;

    MPIR_FUNC_ENTER;

    /* Look for attribute.  They are ordered by keyval handle.  This uses
     * a simple linear list algorithm because few applications use more than a
     * handful of attributes */

    old_p = &win_ptr->attributes;
    p = win_ptr->attributes;
    while (p) {
        if (p->keyval->handle == keyval_ptr->handle) {
            /* If found, call the delete function before replacing the
             * attribute. If the delete function fails on the old
             * attribute value, we keep the old value. */
            mpi_errno = MPIR_Call_attr_delete(win_ptr->handle, p);
            if (mpi_errno != MPI_SUCCESS) {
                goto fn_fail;
            }
            p->value = attribute_val;
            p->as_fortran = as_fortran;
            /* Does not change the reference count on the keyval */
            break;
        } else if (p->keyval->handle > keyval_ptr->handle) {
            MPIR_Attribute *new_p = MPID_Attr_alloc();
            MPIR_ERR_CHKANDJUMP1(!new_p, mpi_errno, MPI_ERR_OTHER,
                                 "**nomem", "**nomem %s", "MPIR_Attribute");
            new_p->keyval = keyval_ptr;
            new_p->pre_sentinal = 0;
            new_p->value = attribute_val;
            new_p->as_fortran = as_fortran;
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
        new_p->pre_sentinal = 0;
        new_p->value = attribute_val;
        new_p->as_fortran = as_fortran;
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
}

int MPIR_Win_delete_attr_impl(MPIR_Win * win_ptr, MPII_Keyval * keyval_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Attribute *p;

    MPIR_FUNC_ENTER;

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
        /* If the user delete function fails, we still remove the attribute from the list. */
        if (mpi_errno != MPI_SUCCESS)
            goto fn_fail;

        /* NOTE: it's incorrect to remove p by its parent pointer because the delete function
         *       may have invalidated the parent pointer, e.g. by removing the parent attribute */
        delete_attr(&win_ptr->attributes, p);
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* C API */

int MPIR_Comm_get_attr_impl(MPIR_Comm * comm_ptr, int comm_keyval, void *attribute_val, int *flag)
{
    return comm_get_attr(comm_ptr, comm_keyval, attribute_val, flag, false);
}

int MPIR_Comm_set_attr_impl(MPIR_Comm * comm_ptr, MPII_Keyval * keyval_ptr, void *attribute_val)
{
    return comm_set_attr(comm_ptr, keyval_ptr, attribute_val, false);
}

int MPIR_Type_get_attr_impl(MPIR_Datatype * type_ptr, int type_keyval,
                            void *attribute_val, int *flag)
{
    return type_get_attr(type_ptr, type_keyval, attribute_val, flag, false);
}

int MPIR_Type_set_attr_impl(MPIR_Datatype * type_ptr, MPII_Keyval * keyval_ptr, void *attribute_val)
{
    return type_set_attr(type_ptr, keyval_ptr, attribute_val, false);
}

int MPIR_Win_get_attr_impl(MPIR_Win * win_ptr, int win_keyval, void *attribute_val, int *flag)
{
    return win_get_attr(win_ptr, win_keyval, attribute_val, flag, false);
}

int MPIR_Win_set_attr_impl(MPIR_Win * win_ptr, MPII_Keyval * keyval_ptr, void *attribute_val)
{
    return win_set_attr(win_ptr, keyval_ptr, attribute_val, false);
}

/* Fortran binding call MPIX_{Comm,Type,Win}_{get,set}_attr */

int MPIR_Comm_get_attr_as_fortran_impl(MPIR_Comm * comm_ptr, int comm_keyval,
                                       void *attribute_val, int *flag)
{
    return comm_get_attr(comm_ptr, comm_keyval, attribute_val, flag, true);
}

int MPIR_Comm_set_attr_as_fortran_impl(MPIR_Comm * comm_ptr, MPII_Keyval * keyval_ptr,
                                       void *attribute_val)
{
    return comm_set_attr(comm_ptr, keyval_ptr, attribute_val, true);
}

int MPIR_Type_get_attr_as_fortran_impl(MPIR_Datatype * type_ptr, int type_keyval,
                                       void *attribute_val, int *flag)
{
    return type_get_attr(type_ptr, type_keyval, attribute_val, flag, true);
}

int MPIR_Type_set_attr_as_fortran_impl(MPIR_Datatype * type_ptr, MPII_Keyval * keyval_ptr,
                                       void *attribute_val)
{
    return type_set_attr(type_ptr, keyval_ptr, attribute_val, true);
}

int MPIR_Win_get_attr_as_fortran_impl(MPIR_Win * win_ptr, int win_keyval,
                                      void *attribute_val, int *flag)
{
    return win_get_attr(win_ptr, win_keyval, attribute_val, flag, true);
}

int MPIR_Win_set_attr_as_fortran_impl(MPIR_Win * win_ptr, MPII_Keyval * keyval_ptr,
                                      void *attribute_val)
{
    return win_set_attr(win_ptr, keyval_ptr, attribute_val, true);
}
