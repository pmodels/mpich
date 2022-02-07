/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef CDESC_H_INCLUDED
#define CDESC_H_INCLUDED

#include "mpichconf.h"
#include <stdio.h>
#include <stdlib.h>
#include <ISO_Fortran_binding.h>
#include <mpi.h>
#include "cdesc_proto.h"

#ifndef HAVE_ROMIO
#define MPIO_Request MPI_Request
#endif

extern int MPIR_F08_MPI_IN_PLACE;
extern int MPIR_F08_MPI_BOTTOM;

extern int cdesc_create_datatype(CFI_cdesc_t * cdesc, MPI_Aint oldcount, MPI_Datatype oldtype,
                                 MPI_Datatype * newtype);
extern int MPIR_Fortran_array_of_string_f2c(const char *strs_f, char ***strs_c, int str_len,
                                            int know_size, int size);
extern int MPIR_Comm_spawn_c(const char *command, char *argv_f, int maxprocs, MPI_Info info,
                             int root, MPI_Comm comm, MPI_Comm * intercomm, int *array_of_errcodes,
                             int argv_elem_len);
extern int MPIR_Comm_spawn_multiple_c(int count, char *array_of_commands_f, char *array_of_argv_f,
                                      const int *array_of_maxprocs, const MPI_Info * array_of_info,
                                      int root, MPI_Comm comm, MPI_Comm * intercomm,
                                      int *array_of_errcodes, int commands_elem_len,
                                      int argv_elem_len);

void *MPIR_F08_get_MPI_STATUS_IGNORE(void);
void *MPIR_F08_get_MPI_STATUSES_IGNORE(void);
void *MPIR_F08_get_MPI_ARGV_NULL(void);
void *MPIR_F08_get_MPI_ARGVS_NULL(void);
void *MPIR_F08_get_MPI_ERRCODES_IGNORE(void);
void *MPIR_F08_get_MPI_UNWEIGHTED(void);
void *MPIR_F08_get_MPI_WEIGHTS_EMPTY(void);

#endif /* CDESC_H_INCLUDED */
