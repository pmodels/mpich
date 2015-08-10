/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#if !defined MUTEXIMPL_H_INCLUDED
#define MUTEXIMPL_H_INCLUDED

#include <mpi.h>
#include <stdint.h>
#define MPIX_MUTEX_TAG 100

#ifdef ENABLE_DEBUG
#define debug_print(...) do { printf(__VA_ARGS__); } while (0)
#else
#define debug_print(...)
#endif

struct mpixi_mutex_s {
    int my_count;
    int max_count;
    MPI_Comm comm;
    MPI_Win *windows;
    uint8_t **bases;
};

/* TODO: Make these mutex operations no-ops for sequential runs */

#endif /* MUTEXIMPL_H_INCLUDED */
