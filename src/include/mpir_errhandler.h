/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 */

#ifndef MPIR_ERRHANDLER_H_INCLUDED
#define MPIR_ERRHANDLER_H_INCLUDED

/*E
  errhandler_fn - MPIR Structure to hold an error handler function

  Notes:
  The MPI-1 Standard declared only the C version of this, implicitly
  assuming that 'int' and 'MPI_Fint' were the same.

  Since Fortran does not have a C-style variable number of arguments
  interface, the Fortran interface simply accepts two arguments.  Some
  calling conventions for Fortran (particularly under Windows) require
  this.

  Module:
  ErrHand-DS

  Questions:
  What do we want to do about C++?  Do we want a hook for a routine that can
  be called to throw an exception in C++, particularly if we give C++ access
  to this structure?  Does the C++ handler need to be different (not part
  of the union)?

  E*/
typedef union errhandler_fn {
    void (*C_Comm_Handler_function) (MPI_Comm *, int *, ...);
    void (*F77_Handler_function) (MPI_Fint *, MPI_Fint *);
    void (*C_Win_Handler_function) (MPI_Win *, int *, ...);
    void (*C_File_Handler_function) (MPI_File *, int *, ...);
} errhandler_fn;

/*S
  MPIR_Errhandler - Description of the error handler structure

  Notes:
  Device-specific information may indicate whether the error handler is active;
  this can help prevent infinite recursion in error handlers caused by
  user-error without requiring the user to be as careful.  We might want to
  make this part of the interface so that the 'MPI_xxx_call_errhandler'
  routines would check.

  It is useful to have a way to indicate that the errhandler is no longer
  valid, to help catch the case where the user has freed the errhandler but
  is still using a copy of the 'MPI_Errhandler' value.  We may want to
  define the 'id' value for deleted errhandlers.

  Module:
  ErrHand-DS
  S*/
typedef struct MPIR_Errhandler {
    MPIR_OBJECT_HEADER;         /* adds handle and ref_count fields */
    MPIR_Lang_t language;
    MPII_Object_kind kind;
    errhandler_fn errfn;
    /* Other, device-specific information */
#ifdef MPID_DEV_ERRHANDLER_DECL
     MPID_DEV_ERRHANDLER_DECL
#endif
} MPIR_Errhandler;
extern MPIR_Object_alloc_t MPIR_Errhandler_mem;
/* Preallocated errhandler objects */
#define MPIR_ERRHANDLER_N_BUILTIN 3
extern MPIR_Errhandler MPIR_Errhandler_builtin[];
extern MPIR_Errhandler MPIR_Errhandler_direct[];

/* We never reference count the builtin error handler objects, regardless of how
 * we decide to reference count the other predefined objects.  If we get to the
 * point where we never reference count *any* of the builtin objects then we
 * should probably remove these checks and let them fall through to the checks
 * for BUILTIN down in the MPIR_Object_* routines. */
#define MPIR_Errhandler_add_ref(_errhand)                               \
    do {                                                                  \
        if (HANDLE_GET_KIND((_errhand)->handle) != HANDLE_KIND_BUILTIN) { \
            MPIR_Object_add_ref(_errhand);                              \
        }                                                                 \
    } while (0)
#define MPIR_Errhandler_release_ref(_errhand, _inuse)                   \
    do {                                                                  \
        if (HANDLE_GET_KIND((_errhand)->handle) != HANDLE_KIND_BUILTIN) { \
            MPIR_Object_release_ref((_errhand), (_inuse));              \
        }                                                                 \
        else {                                                            \
            *(_inuse) = 1;                                                \
        }                                                                 \
    } while (0)

void MPIR_Errhandler_free(MPIR_Errhandler * errhan_ptr);

#endif /* MPIR_ERRHANDLER_H_INCLUDED */
