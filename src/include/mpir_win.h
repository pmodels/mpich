/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 */

#ifndef MPIR_WIN_H_INCLUDED
#define MPIR_WIN_H_INCLUDED

/*S
  MPIR_Win - Description of the Window Object data structure.

  Module:
  Win-DS

  Notes:
  The following 3 keyvals are defined for attributes on all MPI
  Window objects\:
.vb
 MPI_WIN_SIZE
 MPI_WIN_BASE
 MPI_WIN_DISP_UNIT
.ve
  These correspond to the values in 'length', 'start_address', and
  'disp_unit'.

  The communicator in the window is the same communicator that the user
  provided to 'MPI_Win_create' (not a dup).  However, each intracommunicator
  has a special context id that may be used if MPI communication is used
  by the implementation to implement the RMA operations.

  There is no separate window group; the group of the communicator should be
  used.

  Question:
  Should a 'MPID_Win' be defined after 'MPIR_Segment' in case the device
  wants to
  store a queue of pending put/get operations, described with 'MPIR_Segment'
  (or 'MPIR_Request')s?

  S*/
struct MPIR_Win {
    MPIR_OBJECT_HEADER; /* adds handle and ref_count fields */
    MPID_Thread_mutex_t mutex;
    MPIR_Errhandler *errhandler;  /* Pointer to the error handler structure */
    void *base;
    MPI_Aint    size;
    int          disp_unit;      /* Displacement unit of *local* window */
    MPIR_Attribute *attributes;
    MPIR_Comm *comm_ptr;         /* Pointer to comm of window (dup) */
#ifdef USE_THREADED_WINDOW_CODE
    /* These were causing compilation errors.  We need to figure out how to
       integrate threads into MPICH before including these fields. */
    /* FIXME: The test here should be within a test for threaded support */
#ifdef HAVE_PTHREAD_H
    pthread_t wait_thread_id; /* id of thread handling MPI_Win_wait */
    pthread_t passive_target_thread_id; /* thread for passive target RMA */
#elif defined(HAVE_WINTHREADS)
    HANDLE wait_thread_id;
    HANDLE passive_target_thread_id;
#endif
#endif
    /* These are COPIES of the values so that addresses to them
       can be returned as attributes.  They are initialized by the
       MPI_Win_get_attr function.

       These values are constant for the lifetime of the window, so
       this is thread-safe.
     */
    int  copyDispUnit;
    MPI_Aint copySize;

    char          name[MPI_MAX_OBJECT_NAME];

    MPIR_Win_flavor_t create_flavor;
    MPIR_Win_model_t  model;
    MPIR_Win_flavor_t copyCreateFlavor;
    MPIR_Win_model_t  copyModel;

  /* Other, device-specific information */
#ifdef MPID_DEV_WIN_DECL
    MPID_DEV_WIN_DECL
#endif
};
extern MPIR_Object_alloc_t MPIR_Win_mem;
/* Preallocated win objects */
extern MPIR_Win MPIR_Win_direct[];

int MPIR_Type_is_rma_atomic(MPI_Datatype type);
int MPIR_Compare_equal(const void *a, const void *b, MPI_Datatype type);

#endif /* MPIR_WIN_H_INCLUDED */
