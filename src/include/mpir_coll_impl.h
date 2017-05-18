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

#if !defined(MPIR_COLL_IMPL_H_INCLUDED)
#define  MPIR_COLL_IMPL_H_INCLUDED

#include <sys/queue.h>

extern MPIC_progress_global_t MPIC_progress_global;
extern MPIC_global_t MPIC_global_instance;
#include "../mpi/coll/include/coll_impl.h"

#define MPIC_NUM_ENTRIES (8)
struct MPIR_Request;

MPL_STATIC_INLINE_PREFIX int MPIR_Coll_cycle_algorithm(MPIR_Comm * comm_ptr, int pick[], int num)
{
    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTERCOMM)
        return 0;
    int idx;
    MPIC_COMM(comm_ptr)->issued_collectives++;
    idx = MPIC_COMM(comm_ptr)->issued_collectives % num;
    return pick[idx];
}

static inline int MPIC_Progress(int n, void *cq[])
{
    int i = 0;
    COLL_queue_elem_t *s = MPIC_progress_global.head.tqh_first;
    for (; ((s != NULL) && (i < n)); s = s->list_data.tqe_next) {
        if (s->kick_fn(s))
            cq[i++] = (void *) s;
    }
    return i;
}

static inline int MPIC_progress_hook()
{
    void *coll_entries[MPIC_NUM_ENTRIES];
    int i, coll_count, mpi_errno;
    mpi_errno = MPI_SUCCESS;
    coll_count = MPIC_Progress(MPIC_NUM_ENTRIES, coll_entries);
    for (i = 0; i < coll_count; i++) {
        MPIC_req_t *base = (MPIC_req_t *) coll_entries[i];
        MPIR_Request *req = container_of(base, MPIR_Request, coll);
        MPID_Request_complete(req);
    }
    return mpi_errno;
}

extern int MPIC_comm_counter;
extern MPIC_sched_entry_t *MPIC_sched_table;
MPL_STATIC_INLINE_PREFIX int MPIC_comm_init(struct MPIR_Comm *comm)
{
    int rank, size;
    int *tag = &(MPIC_COMM(comm)->use_tag);
    *tag = 0;
    MPIC_COMM(comm)->issued_collectives = 0;
    rank = comm->rank;
    size = comm->local_size;
    /* initialize communicators for collectives
     * each communiactor is assigned a unique id
     */
    MPIC_STUB_KARY_comm_init(&(MPIC_COMM(comm)->stub_kary), MPIC_comm_counter++, tag, rank, size);
    MPIC_STUB_KNOMIAL_comm_init(&(MPIC_COMM(comm)->stub_knomial), MPIC_comm_counter++, tag, rank,
                                size);
    MPIC_MPICH_KARY_comm_init(&(MPIC_COMM(comm)->mpich_kary), MPIC_comm_counter++, tag, rank, size);
    MPIC_MPICH_KNOMIAL_comm_init(&(MPIC_COMM(comm)->mpich_knomial), MPIC_comm_counter++, tag, rank,
                                 size);

#ifdef MPID_COLL_COMM_INIT_HOOK
    MPID_COLL_COMM_INIT_HOOK;
#endif
    return 0;
}

MPL_STATIC_INLINE_PREFIX int MPIC_comm_cleanup(struct MPIR_Comm *comm)
{
    /*cleanup netmod collective communicators */
#ifdef MPID_COLL_COMM_CLEANUP_HOOK
    MPID_COLL_COMM_CLEANUP_HOOK;
#endif
    /*cleanup all collective communicators */
    MPIC_STUB_KARY_comm_cleanup(&(MPIC_COMM(comm)->stub_kary));
    MPIC_STUB_KNOMIAL_comm_cleanup(&(MPIC_COMM(comm)->stub_knomial));

    MPIC_MPICH_KARY_comm_cleanup(&(MPIC_COMM(comm)->mpich_kary));
    MPIC_MPICH_KNOMIAL_comm_cleanup(&(MPIC_COMM(comm)->mpich_knomial));

    return 0;
}


/* dt_t and Op_t are not requres extra initialiation MPICH or STUB.
 * But we should be able to call this hooks for any type or operation,
 * because of different transports might to use transport-specific sections
 * for datatypes or operations (e.g. UCX-netmod).*/
MPL_STATIC_INLINE_PREFIX int MPIC_dt_init(MPIR_Datatype * dt)
{
    /*initialize collective datatypes */
#ifdef MPID_COLL_DT_INIT_HOOK
    MPID_COLL_DT_INIT_HOOK;
#endif

    return 0;
}

MPL_STATIC_INLINE_PREFIX int MPIC_op_init(struct MPIR_Op *op)
{
    /*initialize collective operations */
#ifdef MPID_COLL_OP_INIT_HOOK
    MPID_COLL_OP_INIT_HOOK;
#endif

    return 0;
}

MPL_STATIC_INLINE_PREFIX int MPIC_init_builtin_dt()
{
    MPIR_Datatype *dt_ptr;

#define INIT_DT_BUILTIN(X)              \
    MPID_Datatype_get_ptr(X, dt_ptr);   \
    MPIC_dt_init(dt_ptr);

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
    INIT_DT_BUILTIN(MPI_UNSIGNED_LONG); /* 10 */

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
    INIT_DT_BUILTIN(MPI_INT8_T);        /* 20 */
    INIT_DT_BUILTIN(MPI_INT16_T);
    INIT_DT_BUILTIN(MPI_INT32_T);
    INIT_DT_BUILTIN(MPI_INT64_T);
    INIT_DT_BUILTIN(MPI_UINT8_T);
    INIT_DT_BUILTIN(MPI_UINT16_T);
    INIT_DT_BUILTIN(MPI_UINT32_T);
    INIT_DT_BUILTIN(MPI_UINT64_T);
    INIT_DT_BUILTIN(MPI_C_BOOL);
    INIT_DT_BUILTIN(MPI_C_FLOAT_COMPLEX);
    INIT_DT_BUILTIN(MPI_C_DOUBLE_COMPLEX);      /* 30 */
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
    INIT_DT_BUILTIN(MPI_INTEGER);       /* 40 */
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
    INIT_DT_BUILTIN(MPI_COMPLEX8);      /* 50 */
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
    INIT_DT_BUILTIN(MPI_SHORT_INT);     /* 60 */
    INIT_DT_BUILTIN(MPI_LONG_DOUBLE_INT);

#undef INIT_DT_BUILTIN

#ifdef HAVE_CXX_BINDING

#define INIT_DT_BUILTIN(X)              \
    MPID_Datatype_get_ptr(X, dt_ptr);   \
    dt_ptr->handle=X;                   \
    MPIC_dt_init(dt_ptr);

    INIT_DT_BUILTIN(MPI_CXX_BOOL);
    INIT_DT_BUILTIN(MPI_CXX_FLOAT_COMPLEX);
    INIT_DT_BUILTIN(MPI_CXX_DOUBLE_COMPLEX);
    INIT_DT_BUILTIN(MPI_CXX_LONG_DOUBLE_COMPLEX);
#undef INIT_DT_BUILTIN
#endif

    return 0;
}

MPL_STATIC_INLINE_PREFIX int MPIC_init_builtin_ops()
{
    int i;
    for (i = 0; i < MPIR_OP_N_BUILTIN; ++i) {
        MPIR_Op_builtin[i].handle = MPI_MAX + i;
        MPIC_op_init(&MPIR_Op_builtin[i]);
    }
    return 0;
}

MPL_STATIC_INLINE_PREFIX int MPIC_Op_get_ptr(MPI_Op op, MPIR_Op ** ptr)
{
    if (HANDLE_GET_KIND(op) == HANDLE_KIND_BUILTIN) {
        *ptr = &MPIR_Op_builtin[(op & 0xFF) - 1];
    }
    else {
        MPIR_Op *tmp;
        MPIR_Op_get_ptr(op, tmp);
        *ptr = tmp;
    }
    return 0;
}

MPL_STATIC_INLINE_PREFIX int MPIC_init()
{
    MPIC_progress_global.progress_fn = MPID_Progress_test;
    MPIC_STUB_init();
    MPIC_MPICH_init();

    MPIC_STUB_KARY_init();
    MPIC_STUB_KNOMIAL_init();
    MPIC_MPICH_KARY_init();
    MPIC_MPICH_KNOMIAL_init();

#ifdef MPIDI_NM_COLL_INIT_HOOK
    MPIDI_NM_COLL_INIT_HOOK;
#endif
    MPIC_init_builtin_dt();
    MPIC_init_builtin_ops();
    return 0;
}

MPL_STATIC_INLINE_PREFIX int MPIC_finalize()
{
    return MPI_SUCCESS;
}
#endif /* MPIR_COLL_IMPL_H_INCLUDED */
