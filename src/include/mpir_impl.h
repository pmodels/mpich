/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPIR_IMPL_H_INCLUDED
#define MPIR_IMPL_H_INCLUDED

/* This file declares prototypes for MPIR level implementation functions.
 * Generally, each MPI function will have a corresponding MPIR implementation
 * function listed here, except trivial ones.
 */

/* attr */
/* MPI_Comm_free_keyval, MPI_Type_free_keyval, and MPI_Win_free_keyval can share
 * the same routine. */
void MPIR_free_keyval(MPII_Keyval * keyval_ptr);

int MPIR_Comm_create_keyval_impl(MPI_Comm_copy_attr_function * comm_copy_attr_fn,
                                 MPI_Comm_delete_attr_function * comm_delete_attr_fn,
                                 int *comm_keyval, void *extra_state);
int MPIR_Comm_get_attr_impl(MPIR_Comm * comm_ptr, int comm_keyval, void *attribute_val,
                            int *flag, MPIR_Attr_type outAttrType);
int MPIR_Comm_set_attr_impl(MPIR_Comm * comm_ptr, MPII_Keyval * keyval_ptr, void *attribute_val,
                            MPIR_Attr_type attrType);
int MPIR_Comm_delete_attr_impl(MPIR_Comm * comm_ptr, MPII_Keyval * keyval_ptr);

int MPIR_Type_create_keyval_impl(MPI_Type_copy_attr_function * type_copy_attr_fn,
                                 MPI_Type_delete_attr_function * type_delete_attr_fn,
                                 int *type_keyval, void *extra_state);
int MPIR_Type_get_attr_impl(MPIR_Datatype * type_ptr, int type_keyval, void *attribute_val,
                            int *flag, MPIR_Attr_type outAttrType);
int MPIR_Type_set_attr_impl(MPIR_Datatype * type_ptr, MPII_Keyval * keyval_ptr, void *attribute_val,
                            MPIR_Attr_type attrType);
int MPIR_Type_delete_attr_impl(MPIR_Datatype * type_ptr, MPII_Keyval * keyval_ptr);

int MPIR_Win_create_keyval_impl(MPI_Win_copy_attr_function * win_copy_attr_fn,
                                MPI_Win_delete_attr_function * win_delete_attr_fn,
                                int *win_keyval, void *extra_state);
int MPIR_Win_get_attr_impl(MPIR_Win * win_ptr, int win_keyval, void *attribute_val,
                           int *flag, MPIR_Attr_type outAttrType);
int MPIR_Win_set_attr_impl(MPIR_Win * win_ptr, MPII_Keyval * keyval_ptr, void *attribute_val,
                           MPIR_Attr_type attrType);
int MPIR_Win_delete_attr_impl(MPIR_Win * win_ptr, MPII_Keyval * keyval_ptr);

#endif /* MPIR_IMPL_H_INCLUDED */
