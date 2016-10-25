/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2012 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#ifndef NETMOD_OFI_ML_TEMPLATE_H_INCLUDED
#define NETMOD_OFI_ML_TEMPLATE_H_INCLUDED

#include "coll_post.h"

#define NAMESPACE_ MPIDI_OFI_COLL_
#define LOCAL_     MPICH_KARY
#define REMOTE_    MPICH_KARY

#define CONC(A, B) CONC_(A, B)
#define CONC_(A, B) A##B
#define C0 CONC(NAMESPACE_,LOCAL_)
#define C1 CONC(C0,_)
#define C2 CONC(C1, REMOTE_)
#define C3 CONC(C2, _)
#define COLL_NAMESPACE(fn)    CONC(C3,fn)
#define COLL_NAMESPACE_LOCAL(fn)  CONC(CONC(CONC(NAMESPACE_,LOCAL_),_),fn)
#define COLL_NAMESPACE_REMOTE(fn) CONC(CONC(CONC(NAMESPACE_,REMOTE_),_),fn)

#include "coll_namespace_pre.h"
#include "../algo/ml/ml_coll.h"
#include "coll_namespace_post.h"

#undef NAMESPACE_
#undef LOCAL_
#undef REMOTE_
#undef CONC
#undef CONC_
#undef C0
#undef C1
#undef C2
#undef C3

#endif
