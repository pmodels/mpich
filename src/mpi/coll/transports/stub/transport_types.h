/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2017 Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

/* refer to ../mpich/transport_types.h for documentation */
typedef int MPIR_COLL_STUB_dt_t;

typedef int MPIR_COLL_STUB_op_t;

typedef struct MPIR_COLL_STUB_comm_t {
} MPIR_COLL_STUB_comm_t;

typedef struct MPIR_COLL_STUB_sched_t {
} MPIR_COLL_STUB_sched_t;

typedef struct MPIR_COLL_STUB_req_t {
} MPIR_COLL_STUB_req_t;

typedef struct MPIR_COLL_STUB_global_t {
    MPIR_COLL_STUB_dt_t control_dt;
} MPIR_COLL_STUB_global_t;

typedef uint64_t MPIR_COLL_STUB_aint_t;
