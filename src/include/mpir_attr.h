/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 */

#ifndef MPIR_ATTR_H_INCLUDED
#define MPIR_ATTR_H_INCLUDED

/* Because Comm, Datatype, and File handles are all ints, and because
   attributes are otherwise identical between the three types, we
   only store generic copy and delete functions.  This allows us to use
   common code for the attribute set, delete, and dup functions */
/*E
  copy_function - MPID Structure to hold an attribute copy function

  Notes:
  The appropriate element of this union is selected by using the language
  field of the 'keyval'.

  Because 'MPI_Comm', 'MPI_Win', and 'MPI_Datatype' are all 'int's in
  MPICH, we use a single C copy function rather than have separate
  ones for the Communicator, Window, and Datatype attributes.

  There are no corresponding typedefs for the Fortran functions.  The
  F77 function corresponds to the Fortran 77 binding used in MPI-1 and the
  F90 function corresponds to the Fortran 90 binding used in MPI-2.

  Module:
  Attribute-DS

  E*/
int
MPII_Attr_copy_c_proxy(MPI_Comm_copy_attr_function * user_function,
                       int handle,
                       int keyval,
                       void *extra_state,
                       MPIR_Attr_type attrib_type, void *attrib, void **attrib_copy, int *flag);

typedef struct copy_function {
    int (*C_CopyFunction) (int, int, void *, void *, void *, int *);
    void (*F77_CopyFunction) (MPI_Fint *, MPI_Fint *, MPI_Fint *, MPI_Fint *,
                              MPI_Fint *, MPI_Fint *, MPI_Fint *);
    void (*F90_CopyFunction) (MPI_Fint *, MPI_Fint *, MPI_Aint *, MPI_Aint *,
                              MPI_Aint *, MPI_Fint *, MPI_Fint *);
    /* The generic lang-independent user_function and proxy will
     * replace the lang dependent copy funcs above
     * Currently the lang-indpendent funcs are used only for keyvals
     */
    MPI_Comm_copy_attr_function *user_function;
    MPII_Attr_copy_proxy *proxy;
    /* The C++ function is the same as the C function */
} copy_function;

/*E
  delete_function - MPID Structure to hold an attribute delete function

  Notes:
  The appropriate element of this union is selected by using the language
  field of the 'keyval'.

  Because 'MPI_Comm', 'MPI_Win', and 'MPI_Datatype' are all 'int's in
  MPICH, we use a single C delete function rather than have separate
  ones for the Communicator, Window, and Datatype attributes.

  There are no corresponding typedefs for the Fortran functions.  The
  F77 function corresponds to the Fortran 77 binding used in MPI-1 and the
  F90 function corresponds to the Fortran 90 binding used in MPI-2.

  Module:
  Attribute-DS

  E*/
int
MPII_Attr_delete_c_proxy(MPI_Comm_delete_attr_function * user_function,
                         int handle,
                         int keyval, MPIR_Attr_type attrib_type, void *attrib, void *extra_state);

typedef struct delete_function {
    int (*C_DeleteFunction) (int, int, void *, void *);
    void (*F77_DeleteFunction) (MPI_Fint *, MPI_Fint *, MPI_Fint *, MPI_Fint *, MPI_Fint *);
    void (*F90_DeleteFunction) (MPI_Fint *, MPI_Fint *, MPI_Aint *, MPI_Aint *, MPI_Fint *);
    /* The generic lang-independent user_function and proxy will
     * replace the lang dependent copy funcs above
     * Currently the lang-indpendent funcs are used only for keyvals
     */
    MPI_Comm_delete_attr_function *user_function;
    MPII_Attr_delete_proxy *proxy;
} delete_function;

/*S
  MPII_Keyval - Structure of an MPID keyval

  Module:
  Attribute-DS

  S*/
typedef struct MPII_Keyval {
    MPIR_OBJECT_HEADER;         /* adds handle and ref_count fields */
    MPII_Object_kind kind;
    int was_freed;
    void *extra_state;
    copy_function copyfn;
    delete_function delfn;
    /* other, device-specific information */
#ifdef MPID_DEV_KEYVAL_DECL
     MPID_DEV_KEYVAL_DECL
#endif
} MPII_Keyval;

#define MPII_Keyval_add_ref(_keyval)                                  \
    do {                                                                \
        MPIR_Object_add_ref(_keyval);                                 \
    } while (0)

#define MPII_Keyval_release_ref(_keyval, _inuse)                      \
    do {                                                                \
        MPIR_Object_release_ref(_keyval, _inuse);                     \
    } while (0)


/* Attribute values in C/C++ are void * and in Fortran are ADDRESS_SIZED
   integers.  Normally, these are the same size, but in at least one
   case, the address-sized integers was selected as longer than void *
   to work with the datatype code used in the I/O library.  While this
   is really a limitation in the current Datatype implementation. */
#ifdef USE_AINT_FOR_ATTRVAL
typedef MPI_Aint MPII_Attr_val_t;
#else
typedef void *MPII_Attr_val_t;
#endif

/* Attributes need no ref count or handle, but since we want to use the
   common block allocator for them, we must provide those elements
*/
/*S
  MPIR_Attribute - Structure of an MPID attribute

  Notes:
  Attributes don''t have 'ref_count's because they don''t have reference
  count semantics.  That is, there are no shallow copies or duplicates
  of an attibute.  An attribute is copied when the communicator that
  it is attached to is duplicated.  Subsequent operations, such as
  'MPI_Comm_attr_free', can change the attribute list for one of the
  communicators but not the other, making it impractical to keep the
  same list.  (We could defer making the copy until the list is changed,
  but even then, there would be no reference count on the individual
  attributes.)

  A pointer to the keyval, rather than the (integer) keyval itself is
  used since there is no need within the attribute structure to make
  it any harder to find the keyval structure.

  The attribute value is a 'void *'.  If 'sizeof(MPI_Fint)' > 'sizeof(void*)',
  then this must be changed (no such system has been encountered yet).
  For the Fortran 77 routines in the case where 'sizeof(MPI_Fint)' <
  'sizeof(void*)', the high end of the 'void *' value is used.  That is,
  we cast it to 'MPI_Fint *' and use that value.

  MPI defines three kinds of attributes (see MPI 2.1, Section 16.3, pages
  487-488 (the standard says two, but there are really three, as discussed
  below).  These are pointer-valued attributes and two types of integer-valued
  attributes.
  Pointer-valued attributes are used in C.
  Integer-valued attributes are used in Fortran.  These are of type either
  INTEGER or INTEGER(KIND=MPI_ADDRESS_KIND).

  The predefined attributes are a combination of INTEGER and pointers.

  Module:
  Attribute-DS

 S*/
typedef struct MPIR_Attribute {
    MPIR_OBJECT_HEADER;         /* adds handle and ref_count fields */
    MPII_Keyval *keyval;        /* Keyval structure for this attribute */

    struct MPIR_Attribute *next;        /* Pointer to next in the list */
    MPIR_Attr_type attrType;    /* Type of the attribute */
    long pre_sentinal;          /* Used to detect user errors in accessing
                                 * the value */
    MPII_Attr_val_t value;      /* Stored value. An Aint must be at least
                                 * as large as an address - some builds
                                 * may make an Aint larger than a void * */
    long post_sentinal;         /* Like pre_sentinal */
    /* other, device-specific information */
#ifdef MPID_DEV_ATTR_DECL
     MPID_DEV_ATTR_DECL
#endif
} MPIR_Attribute;

#endif /* MPIR_ATTR_H_INCLUDED */
