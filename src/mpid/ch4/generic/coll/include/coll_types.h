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


#ifndef CH4_GENERIC_COLL_TYPES_H_INCLUDED
#define CH4_GENERIC_COLL_TYPES_H_INCLUDED

typedef struct {
        int use_tag;
        int issued_collectives;
        MPIDI_COLL_STUB_STUB_comm_t stub_stub;
        MPIDI_COLL_STUB_KARY_comm_t stub_kary;
        MPIDI_COLL_STUB_KNOMIAL_comm_t stub_knomial;
        MPIDI_COLL_STUB_RECEXCH_comm_t stub_recexch;
        MPIDI_COLL_STUB_DISSEM_comm_t stub_dissem;
        MPIDI_COLL_MPICH_STUB_comm_t mpich_stub;
        MPIDI_COLL_MPICH_KARY_comm_t mpich_kary;
        MPIDI_COLL_MPICH_KNOMIAL_comm_t mpich_knomial;
        MPIDI_COLL_MPICH_RECEXCH_comm_t mpich_recexch;
        MPIDI_COLL_MPICH_DISSEM_comm_t mpich_dissem;
        MPIDI_COLL_X_TREEBASIC_comm_t x_treebasic;
} MPIDI_COLL_comm_t;

typedef struct {
        MPIDI_COLL_STUB_STUB_dt_t stub_stub;
        MPIDI_COLL_STUB_KARY_dt_t stub_kary;
        MPIDI_COLL_STUB_KNOMIAL_dt_t stub_knomial;
        MPIDI_COLL_STUB_RECEXCH_dt_t stub_recexch;
        MPIDI_COLL_STUB_DISSEM_dt_t stub_dissem;
        MPIDI_COLL_MPICH_STUB_dt_t mpich_stub;
        MPIDI_COLL_MPICH_KARY_dt_t mpich_kary;
        MPIDI_COLL_MPICH_KNOMIAL_dt_t mpich_knomial;
        MPIDI_COLL_MPICH_RECEXCH_dt_t mpich_recexch;
        MPIDI_COLL_MPICH_DISSEM_dt_t mpich_dissem;
} MPIDI_COLL_dt_t;

typedef struct {
        MPIDI_COLL_STUB_STUB_op_t stub_stub;
        MPIDI_COLL_STUB_KARY_op_t stub_kary;
        MPIDI_COLL_STUB_KNOMIAL_op_t stub_knomial;
        MPIDI_COLL_STUB_RECEXCH_op_t stub_recexch;
        MPIDI_COLL_STUB_DISSEM_op_t stub_dissem;
        MPIDI_COLL_MPICH_STUB_op_t mpich_stub;
        MPIDI_COLL_MPICH_KARY_op_t mpich_kary;
        MPIDI_COLL_MPICH_KNOMIAL_op_t mpich_knomial;
        MPIDI_COLL_MPICH_RECEXCH_op_t mpich_recexch;
        MPIDI_COLL_MPICH_DISSEM_op_t mpich_dissem;
} MPIDI_COLL_op_t;


typedef struct {
        MPIDI_COLL_TRANSPORT_STUB_global_t tsp_stub;
        MPIDI_COLL_TRANSPORT_MPICH_global_t tsp_mpich;

        MPIDI_COLL_STUB_STUB_global_t stub_stub;
        MPIDI_COLL_STUB_KARY_global_t stub_kary;
        MPIDI_COLL_STUB_KNOMIAL_global_t stub_knomial;
        MPIDI_COLL_STUB_RECEXCH_global_t stub_recexch;
        MPIDI_COLL_STUB_DISSEM_global_t stub_dissem;
        MPIDI_COLL_MPICH_STUB_global_t mpich_stub;
        MPIDI_COLL_MPICH_KARY_global_t mpich_kary;
        MPIDI_COLL_MPICH_KNOMIAL_global_t mpich_knomial;
        MPIDI_COLL_MPICH_RECEXCH_global_t mpich_recexch;
        MPIDI_COLL_MPICH_DISSEM_global_t mpich_dissem;
        MPIDI_COLL_X_TREEBASIC_global_t x_treebasic;
} MPIDI_COLL_global_t;



static MPIDI_COLL_global_t MPIDI_COLL_global_instance;

typedef union {
        MPIDI_COLL_STUB_STUB_req_t stub_stub;
        MPIDI_COLL_STUB_KARY_req_t stub_kary;
        MPIDI_COLL_STUB_KNOMIAL_req_t stub_knomial;
        MPIDI_COLL_STUB_RECEXCH_req_t stub_recexch;
        MPIDI_COLL_STUB_DISSEM_req_t stub_dissem;
        MPIDI_COLL_MPICH_STUB_req_t mpich_stub;
        MPIDI_COLL_MPICH_KARY_req_t mpich_kary;
        MPIDI_COLL_MPICH_KNOMIAL_req_t mpich_knomial;
        MPIDI_COLL_MPICH_RECEXCH_req_t mpich_recexch;
        MPIDI_COLL_MPICH_DISSEM_req_t mpich_dissem;
} MPIDI_COLL_req_t;

#endif /* CH4_GENERIC_COLL_TYPES_H_INCLUDED */
