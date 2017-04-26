/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2017 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

#ifndef MPIDI_CH4_COLL_POST_H_INCLUDED
#define MPIDI_CH4_COLL_POST_H_INCLUDED

#define GLOBAL_NAME    MPIDI_COLL_

#define TRANSPORT_NAME STUB_
#define TRANSPORT_NAME_LC stub
#include "../transports/stub/transport.h"
#include "../algorithms/stub/post.h"
#include "../algorithms/tree/kary_post.h"
#include "../algorithms/tree/knomial_post.h"
#include "../algorithms/recexch/post.h"
#include "../algorithms/dissem/post.h"
#include "../src/tsp_namespace_undef.h"
#undef TRANSPORT_NAME
#undef TRANSPORT_NAME_LC

#define TRANSPORT_NAME MPICH_
#define TRANSPORT_NAME_LC mpich
#include "../transports/mpich/transport.h"
#include "../algorithms/stub/post.h"
#include "../algorithms/tree/kary_post.h"
#include "../algorithms/tree/knomial_post.h"
#include "../algorithms/recexch/post.h"
#include "../algorithms/dissem/post.h"
#include "../src/tsp_namespace_undef.h"

#undef TRANSPORT_NAME
#undef TRANSPORT_NAME_LC

#define TRANSPORT_NAME X_
#define TRANSPORT_NAME_LC x
#include "../algorithms/treebasic/post.h"
#undef TRANSPORT_NAME
#undef TRANSPORT_NAME_LC

#define TRANSPORT_NAME BMPICH_
#define TRANSPORT_NAME_LC bmpich
#include "../transports/bmpich/transport.h"
#include "../algorithms/tree/kary_post.h"
#include "../algorithms/tree/knomial_post.h"
#include "../src/tsp_namespace_undef.h"
#undef TRANSPORT_NAME
#undef TRANSPORT_NAME_LC

#undef GLOBAL_NAME
#endif /* MPIDI_CH4_COLL_POST_H_INCLUDED */
