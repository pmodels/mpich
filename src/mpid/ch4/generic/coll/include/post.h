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

#if !defined(CH4_COLL_POST_H_INCLUDED)
#define  CH4_COLL_POST_H_INCLUDED
#include "./coll_def.h"
#include "./progress.h"

extern int comm_counter, op_counter, dt_counter;
extern sched_entry_t *sched_table;
MPL_STATIC_INLINE_PREFIX int MPIDI_COLL_Comm_init(struct MPIR_Comm *comm)
{
    int rank, size;
    int *tag = &(MPIDI_COLL_COMM(comm)->use_tag);
    *tag = 0;
    MPIDI_COLL_COMM(comm)->issued_collectives = 0;
    rank = comm->rank;
    size = comm->local_size;
    /*initialize communicators for ch4 collectives
    * each communiactor is assigned a unique id 
    */
    MPIDI_COLL_STUB_STUB_comm_init(&(MPIDI_COLL_COMM(comm)->stub_stub), comm_counter++, tag,  rank, size);
    MPIDI_COLL_STUB_KARY_comm_init(&(MPIDI_COLL_COMM(comm)->stub_kary), comm_counter++, tag, rank, size);
    MPIDI_COLL_STUB_KNOMIAL_comm_init(&(MPIDI_COLL_COMM(comm)->stub_knomial), comm_counter++, tag,  rank, size);
    MPIDI_COLL_STUB_RECEXCH_comm_init(&(MPIDI_COLL_COMM(comm)->stub_recexch), comm_counter++, tag,  rank, size);
    MPIDI_COLL_STUB_DISSEM_comm_init(&(MPIDI_COLL_COMM(comm)->stub_dissem), comm_counter++, tag,  rank, size);
    MPIDI_COLL_MPICH_STUB_comm_init(&(MPIDI_COLL_COMM(comm)->mpich_stub), comm_counter++, tag, rank, size);
    MPIDI_COLL_MPICH_KARY_comm_init(&(MPIDI_COLL_COMM(comm)->mpich_kary), comm_counter++, tag, rank, size);
    MPIDI_COLL_MPICH_KNOMIAL_comm_init(&(MPIDI_COLL_COMM(comm)->mpich_knomial), comm_counter++, tag, rank, size);
    MPIDI_COLL_MPICH_RECEXCH_comm_init(&(MPIDI_COLL_COMM(comm)->mpich_recexch), comm_counter++, tag,  rank, size);
    MPIDI_COLL_MPICH_DISSEM_comm_init(&(MPIDI_COLL_COMM(comm)->mpich_dissem), comm_counter++, tag,  rank, size);
    MPIDI_COLL_X_TREEBASIC_comm_init(&(MPIDI_COLL_COMM(comm)->x_treebasic), comm_counter++, tag, rank, size);

#ifdef MPIDI_NM_COLL_COMM_INIT_HOOK
    MPIDI_NM_COLL_COMM_INIT_HOOK;
#endif
    return 0;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_COLL_Comm_cleanup(struct MPIR_Comm *comm)
{
    /*cleanup netmod collective communicators*/
#ifdef MPIDI_NM_COLL_COMM_CLEANUP_HOOK 
    MPIDI_NM_COLL_COMM_CLEANUP_HOOK;
#endif
    /*cleanup all ch4 collective communicators*/
    MPIDI_COLL_STUB_STUB_comm_cleanup(&(MPIDI_COLL_COMM(comm)->stub_stub));
    MPIDI_COLL_STUB_KARY_comm_cleanup(&(MPIDI_COLL_COMM(comm)->stub_kary));
    MPIDI_COLL_STUB_KNOMIAL_comm_cleanup(&(MPIDI_COLL_COMM(comm)->stub_knomial));
    MPIDI_COLL_STUB_RECEXCH_comm_cleanup(&(MPIDI_COLL_COMM(comm)->stub_recexch));
    MPIDI_COLL_STUB_DISSEM_comm_cleanup(&(MPIDI_COLL_COMM(comm)->stub_dissem));

    MPIDI_COLL_MPICH_STUB_comm_cleanup(&(MPIDI_COLL_COMM(comm)->mpich_stub));
    MPIDI_COLL_MPICH_KARY_comm_cleanup(&(MPIDI_COLL_COMM(comm)->mpich_kary));
    MPIDI_COLL_MPICH_KNOMIAL_comm_cleanup(&(MPIDI_COLL_COMM(comm)->mpich_knomial));
    MPIDI_COLL_MPICH_RECEXCH_comm_cleanup(&(MPIDI_COLL_COMM(comm)->mpich_recexch));
    MPIDI_COLL_MPICH_DISSEM_comm_cleanup(&(MPIDI_COLL_COMM(comm)->mpich_dissem));

    MPIDI_COLL_X_TREEBASIC_comm_cleanup(&(MPIDI_COLL_COMM(comm)->x_treebasic));

    return 0;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_COLL_dt_init(MPIR_Datatype * dt)
{
    /*initialize ch4 collective datatypes*/
    MPIDI_COLL_STUB_STUB_dt_init(&(MPIDI_COLL_DT(dt)->stub_stub), dt_counter++);
    MPIDI_COLL_STUB_KARY_dt_init(&(MPIDI_COLL_DT(dt)->stub_kary), dt_counter++);
    MPIDI_COLL_STUB_KNOMIAL_dt_init(&(MPIDI_COLL_DT(dt)->stub_knomial), dt_counter++);
    MPIDI_COLL_STUB_RECEXCH_dt_init(&(MPIDI_COLL_DT(dt)->stub_recexch), dt_counter++);
    MPIDI_COLL_STUB_DISSEM_dt_init(&(MPIDI_COLL_DT(dt)->stub_dissem), dt_counter++);
    MPIDI_COLL_MPICH_STUB_dt_init(&(MPIDI_COLL_DT(dt)->mpich_stub), dt_counter++);
    MPIDI_COLL_MPICH_KARY_dt_init(&(MPIDI_COLL_DT(dt)->mpich_kary), dt_counter++);
    MPIDI_COLL_MPICH_KNOMIAL_dt_init(&(MPIDI_COLL_DT(dt)->mpich_knomial), dt_counter++);
    MPIDI_COLL_MPICH_RECEXCH_dt_init(&(MPIDI_COLL_DT(dt)->mpich_recexch), dt_counter++);
    MPIDI_COLL_MPICH_DISSEM_dt_init(&(MPIDI_COLL_DT(dt)->mpich_dissem), dt_counter++);
#ifdef MPIDI_NM_COLL_DT_INIT_HOOK
    MPIDI_NM_COLL_DT_INIT_HOOK;
#endif

    return 0;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_COLL_op_init(struct MPIR_Op *op)
{
    /*initialize ch4 collective operations*/
    MPIDI_COLL_STUB_STUB_op_init(&(MPIDI_COLL_OP(op)->stub_stub), op_counter++);
    MPIDI_COLL_STUB_KARY_op_init(&(MPIDI_COLL_OP(op)->stub_kary), op_counter++);
    MPIDI_COLL_STUB_KNOMIAL_op_init(&(MPIDI_COLL_OP(op)->stub_knomial), op_counter++);
    MPIDI_COLL_STUB_RECEXCH_op_init(&(MPIDI_COLL_OP(op)->stub_recexch), op_counter++);
    MPIDI_COLL_STUB_DISSEM_op_init(&(MPIDI_COLL_OP(op)->stub_dissem), op_counter++);
    MPIDI_COLL_MPICH_STUB_op_init(&(MPIDI_COLL_OP(op)->mpich_stub), op_counter++);
    MPIDI_COLL_MPICH_KARY_op_init(&(MPIDI_COLL_OP(op)->mpich_kary), op_counter++);
    MPIDI_COLL_MPICH_KNOMIAL_op_init(&(MPIDI_COLL_OP(op)->mpich_knomial), op_counter++);
    MPIDI_COLL_MPICH_RECEXCH_op_init(&(MPIDI_COLL_OP(op)->mpich_recexch), op_counter++);
    MPIDI_COLL_MPICH_DISSEM_op_init(&(MPIDI_COLL_OP(op)->mpich_dissem), op_counter++);
#ifdef MPIDI_NM_COLL_OP_INIT_HOOK
    MPIDI_NM_COLL_OP_INIT_HOOK;
#endif

    return 0;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_COLL_init_builtin_dt()
{
    MPIR_Datatype *dt_ptr;

#define INIT_DT_BUILTIN(X)              \
    MPID_Datatype_get_ptr(X, dt_ptr);   \
    MPIDI_COLL_dt_init(dt_ptr);

    INIT_DT_BUILTIN(MPI_CHAR);
    INIT_DT_BUILTIN(MPI_UNSIGNED_CHAR);
    INIT_DT_BUILTIN(MPI_SIGNED_CHAR);
    INIT_DT_BUILTIN(MPI_BYTE);
    INIT_DT_BUILTIN(MPI_WCHAR);
    INIT_DT_BUILTIN(MPI_SHORT);
    INIT_DT_BUILTIN(MPI_UNSIGNED_SHORT);
    INIT_DT_BUILTIN(MPI_INT);
    INIT_DT_BUILTIN(MPI_UNSIGNED);
    INIT_DT_BUILTIN(MPI_LONG);
    INIT_DT_BUILTIN(MPI_UNSIGNED_LONG);       /* 10 */

    INIT_DT_BUILTIN(MPI_FLOAT);
    INIT_DT_BUILTIN(MPI_DOUBLE);
    INIT_DT_BUILTIN(MPI_LONG_DOUBLE);
    INIT_DT_BUILTIN(MPI_LONG_LONG);
    INIT_DT_BUILTIN(MPI_UNSIGNED_LONG_LONG);
    INIT_DT_BUILTIN(MPI_PACKED);
    INIT_DT_BUILTIN(MPI_LB);
    INIT_DT_BUILTIN(MPI_UB);
    INIT_DT_BUILTIN(MPI_2INT);

    /* C99 types */
    INIT_DT_BUILTIN(MPI_INT8_T);      /* 20 */
    INIT_DT_BUILTIN(MPI_INT16_T);
    INIT_DT_BUILTIN(MPI_INT32_T);
    INIT_DT_BUILTIN(MPI_INT64_T);
    INIT_DT_BUILTIN(MPI_UINT8_T);
    INIT_DT_BUILTIN(MPI_UINT16_T);
    INIT_DT_BUILTIN(MPI_UINT32_T);
    INIT_DT_BUILTIN(MPI_UINT64_T);
    INIT_DT_BUILTIN(MPI_C_BOOL);
    INIT_DT_BUILTIN(MPI_C_FLOAT_COMPLEX);
    INIT_DT_BUILTIN(MPI_C_DOUBLE_COMPLEX);    /* 30 */
    INIT_DT_BUILTIN(MPI_C_LONG_DOUBLE_COMPLEX);
    /* address/offset/count types */
    INIT_DT_BUILTIN(MPI_AINT);
    INIT_DT_BUILTIN(MPI_OFFSET);
    INIT_DT_BUILTIN(MPI_COUNT);
#ifdef HAVE_FORTRAN_BINDING
    INIT_DT_BUILTIN(MPI_COMPLEX);
    INIT_DT_BUILTIN(MPI_DOUBLE_COMPLEX);
    INIT_DT_BUILTIN(MPI_LOGICAL);
    INIT_DT_BUILTIN(MPI_REAL);
    INIT_DT_BUILTIN(MPI_DOUBLE_PRECISION);
    INIT_DT_BUILTIN(MPI_INTEGER);     /* 40 */
    INIT_DT_BUILTIN(MPI_2INTEGER);
#ifdef MPICH_DEFINE_2COMPLEX
    INIT_DT_BUILTIN(MPI_2COMPLEX);
    INIT_DT_BUILTIN(MPI_2DOUBLE_COMPLEX);
#endif
    INIT_DT_BUILTIN(MPI_2REAL);
    INIT_DT_BUILTIN(MPI_2DOUBLE_PRECISION);
    INIT_DT_BUILTIN(MPI_CHARACTER);
    INIT_DT_BUILTIN(MPI_REAL4);
    INIT_DT_BUILTIN(MPI_REAL8);
    INIT_DT_BUILTIN(MPI_REAL16);
    INIT_DT_BUILTIN(MPI_COMPLEX8);    /* 50 */
    INIT_DT_BUILTIN(MPI_COMPLEX16);
    INIT_DT_BUILTIN(MPI_COMPLEX32);
    INIT_DT_BUILTIN(MPI_INTEGER1);
    INIT_DT_BUILTIN(MPI_INTEGER2);
    INIT_DT_BUILTIN(MPI_INTEGER4);
    INIT_DT_BUILTIN(MPI_INTEGER8);

    if (MPI_INTEGER16 != MPI_DATATYPE_NULL) {
        INIT_DT_BUILTIN(MPI_INTEGER16);
    }

#endif
    INIT_DT_BUILTIN(MPI_FLOAT_INT);
    INIT_DT_BUILTIN(MPI_DOUBLE_INT);
    INIT_DT_BUILTIN(MPI_LONG_INT);
    INIT_DT_BUILTIN(MPI_SHORT_INT);   /* 60 */
    INIT_DT_BUILTIN(MPI_LONG_DOUBLE_INT);

#undef INIT_DT_BUILTIN

#ifdef HAVE_CXX_BINDING

#define INIT_DT_BUILTIN(X)              \
    MPID_Datatype_get_ptr(X, dt_ptr);   \
    dt_ptr->handle=X;                   \
    MPIDI_COLL_dt_init(dt_ptr);

    INIT_DT_BUILTIN(MPI_CXX_BOOL);
    INIT_DT_BUILTIN(MPI_CXX_FLOAT_COMPLEX);
    INIT_DT_BUILTIN(MPI_CXX_DOUBLE_COMPLEX);
    INIT_DT_BUILTIN(MPI_CXX_LONG_DOUBLE_COMPLEX);
#undef INIT_DT_BUILTIN
#endif


}

MPL_STATIC_INLINE_PREFIX int MPIDI_COLL_init_builtin_ops()
{
    int i;
    for( i = 0; i < MPIR_OP_N_BUILTIN; ++i) {
        MPIR_Op_builtin[i].handle = MPI_MAX+i;
        MPIDI_COLL_op_init(&MPIR_Op_builtin[i]);
    }
    return 0;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_COLL_Op_get_ptr(MPI_Op op, MPIR_Op **ptr)
{
    if(HANDLE_GET_KIND(op) == HANDLE_KIND_BUILTIN)
    {
        *ptr = &MPIR_Op_builtin[(op & 0xFF) -1];
    } else {
        MPIR_Op *tmp;
        MPIR_Op_get_ptr(op,tmp);
        *ptr=tmp;
    }
    return 0;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_COLL_Init()
{
    MPIDI_COLL_progress_global.progress_fn = MPID_Progress_test;
    comm_counter=0;
    op_counter=0;
    dt_counter=0;
    sched_table = NULL;
    MPIDI_COLL_TRANSPORT_STUB_init();
    MPIDI_COLL_TRANSPORT_MPICH_init();
    MPIDI_COLL_STUB_STUB_init();
    MPIDI_COLL_STUB_KARY_init();
    MPIDI_COLL_STUB_KNOMIAL_init();
    MPIDI_COLL_STUB_RECEXCH_init();
    MPIDI_COLL_STUB_DISSEM_init();
    MPIDI_COLL_MPICH_STUB_init();
    MPIDI_COLL_MPICH_KARY_init();
    MPIDI_COLL_MPICH_KNOMIAL_init();
    MPIDI_COLL_MPICH_RECEXCH_init();
    MPIDI_COLL_MPICH_DISSEM_init();
    MPIDI_COLL_X_TREEBASIC_init();

#ifdef MPIDI_NM_COLL_INIT_HOOK
    MPIDI_NM_COLL_INIT_HOOK;
#endif
    MPIDI_COLL_init_builtin_dt();
    MPIDI_COLL_init_builtin_ops();
    return 0;
}

#endif /* CH4_COLL_POST_H_INCLUDED */
