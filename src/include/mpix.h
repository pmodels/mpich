/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MPIX_H_INCLUDED
#define MPIX_H_INCLUDED

#include <stdint.h>
#include <inttypes.h>

#include "mpi.h"

/* Prototypes in this file must be interpreted as C routines when this header
 * file is compiled by a C++ compiler.  "mpi.h" already does this for the main
 * MPI_ routines. */
#if defined(__cplusplus)
extern "C" {
#endif

int MPIX_Group_comm_create(MPI_Comm old_comm, MPI_Group group, int tag, MPI_Comm * new_comm);

/* RMA Mutexes extension declarations: */

struct mpix_mutex_s {
  int        my_count;
  int        max_count;
  MPI_Comm   comm;
  MPI_Win   *windows;
  uint8_t  **bases;
};

typedef struct mpix_mutex_s * MPIX_Mutex;

MPIX_Mutex MPIX_Mutex_create (int count, MPI_Comm comm);
int        MPIX_Mutex_destroy(MPIX_Mutex hdl);
void       MPIX_Mutex_lock   (MPIX_Mutex hdl, int mutex, int proc);
void       MPIX_Mutex_unlock (MPIX_Mutex hdl, int mutex, int proc);

#if defined(__cplusplus)
}
#endif

#endif /* MPIX_H_INCLUDED */
