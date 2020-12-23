/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPIR_IMPL_TMP_H_INCLUDED
#define MPIR_IMPL_TMP_H_INCLUDED

/* This file declares prototypes for MPIR level implementation functions.
 * Generally, each MPI function will have a corresponding MPIR implementation
 * function listed here, except trivial ones.
 */

/* -- errhan -- */
int MPIR_Errhandler_free_impl(MPIR_Errhandler * errhan_ptr);
int MPIR_Comm_create_errhandler_impl(MPI_Comm_errhandler_function * function,
                                     MPIR_Errhandler ** errhandler);
void MPIR_Comm_get_errhandler_impl(MPIR_Comm * comm_ptr, MPI_Errhandler * errhandler);
void MPIR_Comm_set_errhandler_impl(MPIR_Comm * comm_ptr, MPIR_Errhandler * errhandler_ptr);
int MPIR_Comm_call_errhandler_impl(MPIR_Comm * comm_ptr, int errorcode);

int MPIR_Win_create_errhandler_impl(MPI_Win_errhandler_function * function,
                                    MPIR_Errhandler ** errhandler);
void MPIR_Win_get_errhandler_impl(MPIR_Win * win_ptr, MPI_Errhandler * errhandler);
void MPIR_Win_set_errhandler_impl(MPIR_Win * win_ptr, MPIR_Errhandler * errhandler_ptr);
int MPIR_Win_call_errhandler_impl(MPIR_Win * win_ptr, int errorcode);

int MPIR_File_create_errhandler_impl(MPI_File_errhandler_function * function,
                                     MPIR_Errhandler ** errhandler);
/* NOTE: File is handled differently from Comm/Win due to ROMIO abstraction */
void MPIR_File_get_errhandler_impl(MPI_File file, MPI_Errhandler * errhandler);
void MPIR_File_set_errhandler_impl(MPI_File file, MPIR_Errhandler * errhandler_ptr);
int MPIR_File_call_errhandler_impl(MPI_File file, int errorcode);

#endif /* MPIR_IMPL_TMP_H_INCLUDED */
