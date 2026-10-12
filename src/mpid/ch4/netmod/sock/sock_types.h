/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef SOCK_TYPES_H_INCLUDED
#define SOCK_TYPES_H_INCLUDED

#include "mpidimpl.h"

typedef struct {
    int listen_fd;
    MPIDI_SOCK_name_t name;     /* address of this process */
    MPI_Aint eager_size;        /* max size of am header plus data in an eager message */
} MPIDI_SOCK_global_t;

extern MPIDI_SOCK_global_t MPIDI_SOCK_global;

#endif /* SOCK_TYPES_H_INCLUDED */
