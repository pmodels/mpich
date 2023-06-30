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

/* Define a typedef for the errflag value used by many internal
 * functions.  If an error needs to be returned, these values can be
 * used to signal such.  More details can be found further down in the
 * code with the bitmasking logic */
typedef enum {
    MPIR_ERR_NONE = MPI_SUCCESS,
    MPIR_ERR_PROC_FAILED = MPIX_ERR_PROC_FAILED,
    MPIR_ERR_OTHER = MPI_ERR_OTHER
} MPIR_Errflag_t;

/*E
  MPIR_Lang_t - Known language bindings for MPI

  A few operations in MPI need to know what language they were called from
  or created by.  This type enumerates the possible languages so that
  the MPI implementation can choose the correct behavior.  An example of this
  are the keyval attribute copy and delete functions.

  Module:
  Attribute-DS
  E*/
typedef enum MPIR_Lang_t {
    MPIR_LANG__C
#ifdef HAVE_FORTRAN_BINDING
        , MPIR_LANG__FORTRAN, MPIR_LANG__FORTRAN90
#endif
#ifdef HAVE_CXX_BINDING
        , MPIR_LANG__CXX
#endif
} MPIR_Lang_t;

extern MPL_initlock_t MPIR_init_lock;

#include "typerep_pre.h"        /* needed for MPIR_Typerep_req */

int MPIR_Localcopy(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                   void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype);
int MPIR_Ilocalcopy(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                    void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype,
                    MPIR_Typerep_req * typerep_req);
int MPIR_Localcopy_stream(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                          void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype, void *stream);
int MPIR_Localcopy_gpu(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                       MPL_pointer_attr_t * sendattr, void *recvbuf, MPI_Aint recvcount,
                       MPI_Datatype recvtype, MPL_pointer_attr_t * recvattr,
                       MPL_gpu_engine_type_t enginetype, bool commit);

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

/* Routines for determining local and remote processes */
int MPIR_Find_local(struct MPIR_Comm *comm, int *local_size_p, int *local_rank_p,
                    int **local_ranks_p, int **intranode_table);
int MPIR_Find_external(struct MPIR_Comm *comm, int *external_size_p, int *external_rank_p,
                       int **external_ranks_p, int **internode_table_p);
int MPIR_Get_internode_rank(MPIR_Comm * comm_ptr, int r);
int MPIR_Get_intranode_rank(MPIR_Comm * comm_ptr, int r);

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

#endif /* MPIR_MISC_H_INCLUDED */
