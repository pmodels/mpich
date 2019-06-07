/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2016 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
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

#ifndef MPICH_MPIDPRE_H_INCLUDED
#include "mpidpre.h"
#endif

#include "mpidch4.h"

#endif /* MPIDIMPL_H_INCLUDED */
