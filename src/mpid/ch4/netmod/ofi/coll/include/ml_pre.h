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
#ifndef NETMOD_OFI_ML_PRE_H_INCLUDED
#define NETMOD_OFI_ML_PRE_H_INCLUDED

#include "./coll_impl.h"

#include "coll_pre.h"

#define NAMESPACE_ MPIDI_OFI_COLL_
#define LOCAL_     MPICH_2ARY
#define REMOTE_    MPICH_2ARY
#define CONC(A, B)  CONC_(A, B)
#define CONC_(A, B) A##B
#define C0 CONC(NAMESPACE_,LOCAL_)
#define C1 CONC(C0, _)
#define C2 CONC(C1, REMOTE_)
#define C3 CONC(C2, _)
#define COLL_NAMESPACE(fn)   CONC(C3,fn)

#include "coll_namespace_pre.h"

#define STRUCT_DEF_(OBJTYPE, OBJNAME)              \
    CONC(CONC(CONC(CONC(MPIDI_OFI_COLL_,OBJTYPE),_),OBJNAME),_t)

typedef struct COLL_hybrid_dt_t {
    STRUCT_DEF_(LOCAL_,dt)  *local;
    STRUCT_DEF_(REMOTE_,dt) *remote;
} COLL_hybrid_dt_t;

typedef struct COLL_hybrid_op_t {
    STRUCT_DEF_(LOCAL_,op)  *local;
    STRUCT_DEF_(REMOTE_,op) *remote;
} COLL_hybrid_op_t;

typedef struct COLL_hybrid_comm_t {
    STRUCT_DEF_(LOCAL_,comm)  *local;
    STRUCT_DEF_(REMOTE_,comm) *remote;
} COLL_hybrid_comm_t;

typedef struct COLL_hybrid_sched_t {
    STRUCT_DEF_(LOCAL_,sched)  *local;
    STRUCT_DEF_(REMOTE_,sched) *remote;
} COLL_hybrid_sched_t;

typedef struct COLL_hybrid_aint_t {
    STRUCT_DEF_(LOCAL_,aint)  *local;
    STRUCT_DEF_(REMOTE_,aint) *remote;
} COLL_hybrid_aint_t;


#include "../algo/ml/ml_types.h"
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

#undef STRUCT_DEF_


#endif
