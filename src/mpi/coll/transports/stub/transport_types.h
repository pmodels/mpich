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
typedef int MPIC_STUB_dt_t;

typedef int MPIC_STUB_op_t;

typedef struct MPIC_STUB_comm_t {
} MPIC_STUB_comm_t;

typedef struct MPIC_STUB_sched_t {
} MPIC_STUB_sched_t;

typedef struct MPIC_STUB_req_t {
} MPIC_STUB_req_t;

typedef struct MPIC_STUB_global_t {
    MPIC_STUB_dt_t control_dt;
} MPIC_STUB_global_t;

typedef uint64_t MPIC_STUB_aint_t;
