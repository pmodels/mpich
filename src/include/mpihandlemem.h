/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id: mpihandlemem.h,v 1.10 2007/07/11 16:06:37 robl Exp $
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef MPIHANDLE_H_INCLUDED
#define MPIHANDLE_H_INCLUDED

/*TOpaqOverview.tex
  MPI Opaque Objects:

  MPI Opaque objects such as 'MPI_Comm' or 'MPI_Datatype' are specified by 
  integers (in the MPICH2 implementation); the MPI standard calls these
  handles.  
  Out of range values are invalid; the value 0 is reserved.
  For most (with the possible exception of 
  'MPI_Request' for performance reasons) MPI Opaque objects, the integer
  encodes both the kind of object (allowing runtime tests to detect a datatype
  passed where a communicator is expected) and important properties of the 
  object.  Even the 'MPI_xxx_NULL' values should be encoded so that 
  different null handles can be distinguished.  The details of the encoding
  of the handles is covered in more detail in the MPICH2 Design Document.
  For the most part, the ADI uses pointers to the underlying structures
  rather than the handles themselves.  However, each structure contains an 
  'handle' field that is the corresponding integer handle for the MPI object.

  MPID objects (objects used within the implementation of MPI) are not opaque.

  T*/

/* Known MPI object types.  These are used for both the error handlers 
   and for the handles.  This is a 4 bit value.  0 is reserved for so 
   that all-zero handles can be flagged as an error. */
/*E
  MPID_Object_kind - Object kind (communicator, window, or file)

  Notes:
  This enum is used by keyvals and errhandlers to indicate the type of
  object for which MPI opaque types the data is valid.  These are defined
  as bits to allow future expansion to the case where an object is value for
  multiple types (for example, we may want a universal error handler for 
  errors return).  This is also used to indicate the type of MPI object a 
  MPI handle represents.  It is an enum because only this applies only the
  the MPI and internal MPICH2 objects.

  The 'MPID_PROCGROUP' kind is used to manage process groups (different
  from MPI Groups) that are used to keep track of collections of
  processes (each 'MPID_PROCGROUP' corresponds to a group of processes
  that define an 'MPI_COMM_WORLD'.  This becomes important only 
  when MPI-2 dynamic process features are supported.  'MPID_VCONN' is
  a virtual connection; while this is not part of the overall ADI3
  design, an object that manages connections to other processes is
  a common need, and 'MPID_VCONN' may be used for that.

  Module:
  Attribute-DS
  E*/
typedef enum MPID_Object_kind { 
  MPID_COMM       = 0x1, 
  MPID_GROUP      = 0x2,
  MPID_DATATYPE   = 0x3,
  MPID_FILE       = 0x4,               /* This is not used */
  MPID_ERRHANDLER = 0x5,
  MPID_OP         = 0x6,
  MPID_INFO       = 0x7,
  MPID_WIN        = 0x8,
  MPID_KEYVAL     = 0x9,
  MPID_ATTR       = 0xa,
  MPID_REQUEST    = 0xb,
  MPID_PROCGROUP  = 0xc,               /* These are internal device objects */
  MPID_VCONN      = 0xd,
  MPID_GREQ_CLASS = 0xf
  } MPID_Object_kind;

#define HANDLE_MPI_KIND_SHIFT 26
#define HANDLE_GET_MPI_KIND(a) ( ((a)&0x3c000000) >> HANDLE_MPI_KIND_SHIFT )

/* Handle types.  These are really 2 bits */
#define HANDLE_KIND_INVALID  0x0
#define HANDLE_KIND_BUILTIN  0x1
#define HANDLE_KIND_DIRECT   0x2
#define HANDLE_KIND_INDIRECT 0x3
/* Mask assumes that ints are at least 4 bytes */
#define HANDLE_KIND_MASK 0xc0000000
#define HANDLE_KIND_SHIFT 30
#define HANDLE_GET_KIND(a) (((unsigned)(a)&HANDLE_KIND_MASK)>>HANDLE_KIND_SHIFT)
#define HANDLE_SET_KIND(a,kind) ((a)|((kind)<<HANDLE_KIND_SHIFT))

/* For indirect, the remainder of the handle has a block and index */
#define HANDLE_INDIRECT_SHIFT 16
#define HANDLE_BLOCK(a) (((a)& 0x03FF0000) >> HANDLE_INDIRECT_SHIFT)
#define HANDLE_BLOCK_INDEX(a) ((a) & 0x0000FFFF)

/* Handle block is between 1 and 1024 *elements* */
#define HANDLE_BLOCK_SIZE 256
/* Index size is bewtween 1 and 65536 *elements* */
#define HANDLE_BLOCK_INDEX_SIZE 1024

/* For direct, the remainder of the handle is the index into a predefined 
   block */
#define HANDLE_MASK 0x03FFFFFF
#define HANDLE_INDEX(a) ((a)& HANDLE_MASK)

/* ALL objects have the handle as the first value. */
/* Inactive (unused and stored on the appropriate avail list) objects 
   have MPIU_Handle_common as the head */
typedef struct MPIU_Handle_common {
    int  handle;
    volatile int ref_count; /* This field is used to indicate that the
			       object is not in use (see, e.g., 
			       MPID_Comm_valid_ptr) */
    void *next;   /* Free handles use this field to point to the next
                     free object */
} MPIU_Handle_common;

/* All *active* (in use) objects have the handle as the first value; objects
   with referene counts have the reference count as the second value.
   See MPIU_Object_add_ref and MPIU_Object_release_ref. */
typedef struct MPIU_Handle_head {
    int handle;
    volatile int ref_count;
} MPIU_Handle_head;

/* This type contains all of the data, except for the direct array,
   used by the object allocators. */
typedef struct MPIU_Object_alloc_t {
    MPIU_Handle_common *avail;          /* Next available object */
    int                initialized;     /* */
    void              *(*indirect)[];   /* Pointer to indirect object blocks */
    int                indirect_size;   /* Number of allocated indirect blocks */
    MPID_Object_kind   kind;            /* Kind of object this is for */
    int                size;            /* Size of an individual object */
    void               *direct;         /* Pointer to direct block, used 
                                           for allocation */
    int                direct_size;     /* Size of direct block */
} MPIU_Object_alloc_t;
extern void *MPIU_Handle_obj_alloc(MPIU_Object_alloc_t *);
extern void *MPIU_Handle_obj_alloc_unsafe(MPIU_Object_alloc_t *);
extern void MPIU_Handle_obj_alloc_complete(MPIU_Object_alloc_t *, int);
extern void MPIU_Handle_obj_free( MPIU_Object_alloc_t *, void * );
void *MPIU_Handle_get_ptr_indirect( int, MPIU_Object_alloc_t * );
extern void *MPIU_Handle_direct_init(void *direct, int direct_size, 
				     int obj_size, int handle_type);
#endif
