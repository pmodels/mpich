/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPIR_MISC_H_INCLUDED
#define MPIR_MISC_H_INCLUDED

#define MPIR_UNIVERSE_SIZE_NOT_SET -1
#define MPIR_UNIVERSE_SIZE_NOT_AVAILABLE -2

#define MPIR_FINALIZE_CALLBACK_PRIO 5
#define MPIR_FINALIZE_CALLBACK_HANDLE_CHECK_PRIO 1
#define MPIR_FINALIZE_CALLBACK_DEFAULT_PRIO 0
#define MPIR_FINALIZE_CALLBACK_MAX_PRIO 10

/* Define values for collective attribute. Collective attributes pass
 * down contexts including error flags.
 */
#define MPIR_COLL_ATTR_SYNC  0x1        /* It's an internal collective that focuses
                                         * on synchronization rather than batch latency.
                                         * In particular, advise netmod to avoid using
                                         * injection send. */
#define MPIR_ERR_PROC_FAILED 0x2
#define MPIR_ERR_OTHER       0x4
#define MPIR_COLL_ATTR_ERR_MASK 0x6

#define MPIR_COLL_ATTR_HAS_ERR(coll_attr) ((coll_attr) & MPIR_COLL_ATTR_ERR_MASK)

#define MPIR_ERR_NONE 0 /* FIXME: transition hack */
typedef int MPIR_Errflag_t;     /* FIXME: transition hack */
/*E
  MPIR_Lang_t - Known language bindings for MPI

  A few operations in MPI need to know what language they were called from
  or created by.  This type enumerates the possible languages so that
  the MPI implementation can choose the correct behavior.  An example of this
  are the keyval attribute copy and delete functions.

  MPIR_LANG__X is for new extensions e.g. MPIX_Comm_create_errhandler_x,
  which calls with scalar input as in C and an extra_state for context.
  It is meant language bindings to use a proxy for user callbacks.

  Module:
  Attribute-DS
  E*/
typedef enum MPIR_Lang_t {
    MPIR_LANG__C,
    MPIR_LANG__X
#ifdef HAVE_FORTRAN_BINDING
        , MPIR_LANG__FORTRAN, MPIR_LANG__FORTRAN90
#endif
#ifdef HAVE_CXX_BINDING
        , MPIR_LANG__CXX
#endif
} MPIR_Lang_t;

extern MPL_initlock_t MPIR_init_lock;

#include "typerep_pre.h"        /* needed for MPIR_Typerep_req */

/* FIXME: bad names. Not gpu-specific, confusing with MPIR_Request.
 *        It's a general async handle.
 */
typedef enum {
    MPIR_NULL_REQUEST = 0,
    MPIR_TYPEREP_REQUEST,
    MPIR_GPU_REQUEST,
} MPIR_request_type_t;

typedef struct {
    union {
        MPIR_Typerep_req y_req;
        MPL_gpu_request gpu_req;
    } u;
    MPIR_request_type_t type;
} MPIR_gpu_req;

int MPIR_Localcopy(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                   void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype);
int MPIR_Ilocalcopy(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                    void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype,
                    MPIR_Typerep_req * typerep_req);
int MPIR_Localcopy_stream(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                          void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype, void *stream);
int MPIR_Localcopy_gpu(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                       MPI_Aint sendoffset, MPL_pointer_attr_t * sendattr, void *recvbuf,
                       MPI_Aint recvcount, MPI_Datatype recvtype, MPI_Aint recvoffset,
                       MPL_pointer_attr_t * recvattr, MPL_gpu_copy_direction_t dir,
                       MPL_gpu_engine_type_t enginetype, bool commit);
int MPIR_Ilocalcopy_gpu(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                        MPI_Aint sendoffset, MPL_pointer_attr_t * sendattr, void *recvbuf,
                        MPI_Aint recvcount, MPI_Datatype recvtype, MPI_Aint recvoffset,
                        MPL_pointer_attr_t * recvattr, MPL_gpu_copy_direction_t dir,
                        MPL_gpu_engine_type_t enginetype, bool commit, MPIR_gpu_req * req);

/* Contiguous datatype calculates buffer address with `(char *) buf + dt_true_lb`.
 * However, dt_true_lb is treated as ptrdiff_t (signed), and when buf is MPI_BOTTOM
 * and on 32-bit systems, ubsan will warn the latter overflow. Cast to uintptr_t
 * to work around.
 */
#define MPIR_get_contig_ptr(buf, true_lb) \
    (void *) ((uintptr_t) buf + (uintptr_t) (true_lb))

/*@ MPIR_Add_finalize - Add a routine to be called when MPI_Finalize is invoked

+ routine - Routine to call
. extra   - Void pointer to data to pass to the routine
- priority - Indicates the priority of this callback and controls the order
  in which callbacks are executed.  Use a priority of zero for most handlers;
  higher priorities will be executed first.

Notes:
  The routine 'MPID_Finalize' is executed with priority
  'MPIR_FINALIZE_CALLBACK_PRIO' (currently defined as 5).  Handlers with
  a higher priority execute before 'MPID_Finalize' is called; those with
  a lower priority after 'MPID_Finalize' is called.
@*/
void MPIR_Add_finalize(int (*routine) (void *), void *extra, int priority);


#define MPIR_CAST(T, val) CAST_##T((val))
#ifdef NDEBUG
#define MPIR_CAST_int(val) ((int) (val))
#define MPIR_CAST_Aint(val) ((MPI_Aint) (val))
#else
#define MPIR_CAST_int(val) \
    (((val) > INT_MAX || ((val) < 0 && (val) < INT_MIN)) ? (assert(0), 0) : (int) (val))
#define MPIR_CAST_Aint(val) \
    (((val) > MPIR_AINT_MAX || ((val) < 0 && (val) < MPIR_AINT_MIN)) ? (assert(0), 0) \
     : (MPI_Aint) (val))
#endif

int MPIR_get_supported_memory_kinds(char *requested_kinds, char **out_kinds);
void MPIR_get_memory_kinds_from_comm(MPIR_Comm * comm_ptr, char **out_kinds);

#endif /* MPIR_MISC_H_INCLUDED */
