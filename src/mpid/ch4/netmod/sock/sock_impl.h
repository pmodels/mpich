/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef SOCK_IMPL_H_INCLUDED
#define SOCK_IMPL_H_INCLUDED

#include <mpidimpl.h>
#include "sock_types.h"
#include "mpidch4r.h"
#include "ch4_impl.h"

#define MPIDI_SOCK_AV(av)     ((av)->netmod.sock)

int MPIDI_SOCK_comm_addr_exchange(MPIR_Comm * comm);

#endif /* SOCK_IMPL_H_INCLUDED */
