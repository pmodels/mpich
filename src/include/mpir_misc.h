/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
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

extern const char MPII_Version_string[] MPICH_API_PUBLIC;
extern const char MPII_Version_date[] MPICH_API_PUBLIC;
extern const char MPII_Version_ABI[] MPICH_API_PUBLIC;
extern const char MPII_Version_configure[] MPICH_API_PUBLIC;
extern const char MPII_Version_device[] MPICH_API_PUBLIC;
extern const char MPII_Version_CC[] MPICH_API_PUBLIC;
extern const char MPII_Version_CXX[] MPICH_API_PUBLIC;
extern const char MPII_Version_F77[] MPICH_API_PUBLIC;
extern const char MPII_Version_FC[] MPICH_API_PUBLIC;
extern const char MPII_Version_custom[] MPICH_API_PUBLIC;

int MPIR_Localcopy(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                   void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype);

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
int MPIR_Find_local_and_external(struct MPIR_Comm *comm, int *local_size_p, int *local_rank_p,
                                 int **local_ranks_p, int *external_size_p, int *external_rank_p,
                                 int **external_ranks_p, int **intranode_table,
                                 int **internode_table_p);
int MPIR_Get_internode_rank(MPIR_Comm * comm_ptr, int r);
int MPIR_Get_intranode_rank(MPIR_Comm * comm_ptr, int r);

int MPIR_Close_port_impl(const char *port_name);
int MPIR_Open_port_impl(MPIR_Info * info_ptr, char *port_name);
int MPIR_Cancel(MPIR_Request * request_ptr);

/* Default routines for asynchronous progress thread */
int MPIR_Init_async_thread(void);
int MPIR_Finalize_async_thread(void);

#endif /* MPIR_MISC_H_INCLUDED */
