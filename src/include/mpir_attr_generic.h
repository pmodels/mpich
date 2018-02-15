/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MPIR_ATTR_GENERIC_H_INCLUDED
#define MPIR_ATTR_GENERIC_H_INCLUDED

/* ------------------------------------------------------------------------- */
/* Keyvals and attributes */
/*TKyOverview.tex

  Keyvals are MPI objects that, unlike most MPI objects, are defined to be
  integers rather than a handle (e.g., 'MPI_Comm').  However, they really
  `are` MPI opaque objects and are handled by the MPICH implementation in
  the same way as all other MPI opaque objects.  The only difference is that
  there is no 'typedef int MPI_Keyval;' in 'mpi.h'.  In particular, keyvals
  are encoded (for direct and indirect references) in the same way that
  other MPI opaque objects are

  Each keyval has a copy and a delete function associated with it.
  Unfortunately, these have a slightly different calling sequence for
  each language, particularly when the size of a pointer is
  different from the size of a Fortran integer.  The unions
  'copy_function' and 'delete_function' capture the differences
  in a single union type.

  The above comment is out of date but has never been updated as it should
  have to match the introduction of a different interface.  Beware!

  Notes:

  In the original design, retrieving a attribute from a different
  language that set it was thought to be an error.  The MPI Forum
  decided that this should be allowed, and after much discussion, the
  behavior was defined.  Thus, we need to record what sort of
  attribute was provided, and be able to properly return the correct
  value in each case.  See MPI 2.2, Section 16.3.7 (Attributes) for
  specific requirements.  One consequence of this is that the value
  that is returned may have a different length that how it was set.
  On little-endian platforms (e.g., x86), this doesn't cause much of a
  problem, because the address is that of the least significant byte,
  and the lower bytes have the data that is needed in the case that
  the desired attribute type is shorter than the stored attribute.
  However, on a big-endian platform (e.g., IBM POWER), since the most
  significant bytes are stored first, depending on the length of the
  result type, the address of the result may not be the beginning of
  the memory area.  For example, assume that an MPI_Fint is 4 bytes
  and a void * (and a Fortran INTEGER of kind MPI_ADDRESS_KIND) is 8
  bytes, and let the attribute store the value in an 8 byte integer in
  a field named "value".  On a little-endian platform, the address of
  the value is always the beginning of the field "value".  On a
  big-endian platform, the address of the value is the beginning of
  the field if the return type is a pointer (e.g., from C) or Fortran
  (KIND=MPI_ADDRESS_KIND), and the address of the beginning of the
  field + 4 if the return type is a Fortran 77 integer (and, as
  specified above, an MPI_Fint is 4 bytes shorter than a void *).

  For the big-endian case, it is possible to manage these shifts (using
  WORDS_LITTLEENDIAN to detect the big-endian case).  Alternatively,
  at a small cost in space, copies in variables of the correct length
  can be maintained.  At this writing, the code in src/mpi/attr makes
  use of WORDS_LITTLEENDIAN to provide the appropriate code for the most
  common cases.

  T*/
/*TAttrOverview.tex
 *
 * The MPI standard allows `attributes`, essentially an '(integer,pointer)'
 * pair, to be attached to communicators, windows, and datatypes.
 * The integer is a `keyval`, which is allocated by a call (at the MPI level)
 * to 'MPI_Comm/Type/Win_create_keyval'.  The pointer is the value of
 * the attribute.
 * Attributes are primarily intended for use by the user, for example, to save
 * information on a communicator, but can also be used to pass data to the
 * MPI implementation.  For example, an attribute may be used to pass
 * Quality of Service information to an implementation to be used with
 * communication on a particular communicator.
 * To provide the most general access by the ADI to all attributes, the
 * ADI defines a collection of routines that are used by the implementation
 * of the MPI attribute routines (such as 'MPI_Comm_get_attr').
 * In addition, the MPI routines involving attributes will invoke the
 * corresponding 'hook' functions (e.g., 'MPID_Comm_attr_hook')
 * should the device define them.
 *
 * Attributes on windows and datatypes are defined by MPI but not of
 * interest (as yet) to the device.
 *
 * In addition, there are seven predefined attributes that the device must
 * supply to the implementation.  This is accomplished through
 * data values that are part of the 'MPIR_Process' data block.
 *  The predefined keyvals on 'MPI_COMM_WORLD' are\:
 *.vb
 * Keyval                     Related Module
 * MPI_APPNUM                 Dynamic
 * MPI_HOST                   Core
 * MPI_IO                     Core
 * MPI_LASTUSEDCODE           Error
 * MPI_TAG_UB                 Communication
 * MPI_UNIVERSE_SIZE          Dynamic
 * MPI_WTIME_IS_GLOBAL        Timer
 *.ve
 * The values stored in the 'MPIR_Process' block are the actual values.  For
 * example, the value of 'MPI_TAG_UB' is the integer value of the largest tag.
 * The
 * value of 'MPI_WTIME_IS_GLOBAL' is a '1' for true and '0' for false.  Likely
 * values for 'MPI_IO' and 'MPI_HOST' are 'MPI_ANY_SOURCE' and 'MPI_PROC_NULL'
 * respectively.
 *
 T*/

/* bit 0 distinguishes between pointers (0) and integers (1) */
typedef enum {
    MPIR_ATTR_PTR = 0,
    MPIR_ATTR_AINT = 1,
    MPIR_ATTR_INT = 3
} MPIR_Attr_type;

#define MPII_ATTR_KIND(_a) (_a & 0x1)

MPICH_API_PUBLIC int MPII_Comm_set_attr(MPI_Comm, int, void *, MPIR_Attr_type);
MPICH_API_PUBLIC int MPII_Type_set_attr(MPI_Datatype, int, void *, MPIR_Attr_type);
MPICH_API_PUBLIC int MPII_Win_set_attr(MPI_Win, int, void *, MPIR_Attr_type);
MPICH_API_PUBLIC int MPII_Comm_get_attr(MPI_Comm, int, void *, int *, MPIR_Attr_type);
MPICH_API_PUBLIC int MPII_Type_get_attr(MPI_Datatype, int, void *, int *, MPIR_Attr_type);
MPICH_API_PUBLIC int MPII_Win_get_attr(MPI_Win, int, void *, int *, MPIR_Attr_type);

MPICH_API_PUBLIC int MPII_Comm_get_attr_fort(MPI_Comm, int, void *, int *, MPIR_Attr_type);


#if defined(__cplusplus)
extern "C" {
#endif

/*E
  Language bindings for MPI

  A few operations in MPI need to know how to marshal the callback into the calling
  lanuage calling convention. The marshaling code is provided by a thunk layer which
  implements the correct behavior.  Examples of these callback functions are the
  keyval attribute copy and delete functions.

  Module:
  Attribute-DS
  E*/

/*
 * Support bindings for Attribute copy/del callbacks
 * Consolidate Comm/Type/Win attribute functions together as the handle type is the same
 * use MPI_Comm for the prototypes
 */
    typedef
        int
     (MPII_Attr_copy_proxy) (MPI_Comm_copy_attr_function * user_function,
                             int handle,
                             int keyval,
                             void *extra_state,
                             MPIR_Attr_type attrib_type,
                             void *attrib, void **attrib_copy, int *flag);

    typedef
        int
     (MPII_Attr_delete_proxy) (MPI_Comm_delete_attr_function * user_function,
                               int handle,
                               int keyval,
                               MPIR_Attr_type attrib_type, void *attrib, void *extra_state);

    MPICH_API_PUBLIC void
        MPII_Keyval_set_proxy(int keyval,
                              MPII_Attr_copy_proxy copy_proxy, MPII_Attr_delete_proxy delete_proxy);

#if defined(__cplusplus)
}
#endif
#endif                          /* MPIR_ATTR_GENERIC_H_INCLUDED */
