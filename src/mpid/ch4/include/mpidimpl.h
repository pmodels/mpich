/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPIDIMPL_H_INCLUDED
#define MPIDIMPL_H_INCLUDED

#include "mpichconf.h"
#include <stdio.h>

#ifdef HAVE_ASSERT_H
#include <assert.h>
#endif

#define MPICH_SKIP_MPICXX
#include "mpiimpl.h"
#include "mpidpre.h"
#include "mpidch4.h"

int MPIDI_Comm_set_vcis(MPIR_Comm * comm, int num_implicit, int num_reserved);

#endif /* MPIDIMPL_H_INCLUDED */
