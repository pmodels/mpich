/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */

/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MPIR_DATALOOP_H
#define MPIR_DATALOOP_H

#include <mpi.h>

/*
 * The following macro allows us to reference either the regular or 
 * hetero value for the 3 fields (NULL,_size,_depth) in the
 * MPIR_Datatype structure.  This is used in the many
 * macros that access fields of the datatype.  We need this macro
 * to simplify the definition of the other macros in the case where
 * MPID_HAS_HETERO is *not* defined.
 */
#if defined(MPID_HAS_HETERO) || 1
#define MPIR_DATALOOP_GET_FIELD(hetero_,value_,fieldname_) do {                          \
        if (hetero_ != MPIR_DATALOOP_HETEROGENEOUS)                             \
            value_ = ((MPIR_Datatype *)ptr)->dataloop##fieldname_;              \
        else value_ = ((MPIR_Datatype *) ptr)->hetero_dloop##fieldname_;        \
    } while(0)
#else
#define MPIR_DATALOOP_GET_FIELD(hetero_,value_,fieldname_) \
      value_ = ((MPIR_Datatype *)ptr)->dataloop##fieldname_
#endif

#if defined(MPID_HAS_HETERO) || 1
#define MPIR_DATALOOP_SET_FIELD(hetero_,value_,fieldname_) do {                          \
        if (hetero_ != MPIR_DATALOOP_HETEROGENEOUS)                             \
            ((MPIR_Datatype *)ptr)->dataloop##fieldname_ = value_;              \
        else ((MPIR_Datatype *) ptr)->hetero_dloop##fieldname_ = value_;        \
    } while(0)
#else
#define MPIR_DATALOOP_SET_FIELD(hetero_,value_,fieldname_) \
    ((MPIR_Datatype *)ptr)->dataloop##fieldname_ = value_
#endif

/* These following dataloop-specific types will be used throughout the DLOOP
 * instance:
 */
#define DLOOP_Offset     MPI_Aint
#define DLOOP_Count      MPI_Aint
#define DLOOP_Handle     MPI_Datatype
#define DLOOP_Type       MPI_Datatype
#define DLOOP_Buffer     void *
#define DLOOP_VECTOR     MPL_IOV
#define DLOOP_VECTOR_LEN MPL_IOV_LEN
#define DLOOP_VECTOR_BUF MPL_IOV_BUF
#define DLOOP_Size       MPI_Aint

/* The following accessor functions must also be defined:
 *
 * DLOOP_Handle_extent()
 * DLOOP_Handle_size()
 * DLOOP_Handle_loopptr()
 * DLOOP_Handle_loopdepth()
 * DLOOP_Handle_hasloop()
 *
 */

/* USE THE NOTATION THAT BILL USED IN MPIIMPL.H AND MAKE THESE MACROS */

/* NOTE: put get size into mpiimpl.h; the others go here until such time
 * as we see that we need them elsewhere.
 */
#define DLOOP_Handle_get_loopdepth_macro(handle_,depth_,flag_) \
    MPIR_Datatype_get_loopdepth_macro(handle_,depth_,flag_)

#define DLOOP_Handle_get_loopsize_macro(handle_,size_,flag_) \
    MPIR_Datatype_get_loopsize_macro(handle_,size_,flag_)

#define DLOOP_Handle_get_loopptr_macro(handle_,lptr_,flag_) \
    MPIR_Datatype_get_loopptr_macro(handle_,lptr_,flag_)

#define DLOOP_Handle_set_loopptr_macro(handle_,lptr_,flag_) \
    MPIR_Datatype_set_loopptr_macro(handle_,lptr_,flag_)

#define DLOOP_Handle_set_loopdepth_macro(handle_,depth_,flag_) \
    MPIR_Datatype_set_loopdepth_macro(handle_,depth_,flag_)

#define DLOOP_Handle_set_loopsize_macro(handle_,size_,flag_) \
    MPIR_Datatype_set_loopsize_macro(handle_,size_,flag_)

#define DLOOP_Handle_get_size_macro(handle_,size_) \
    MPIR_Datatype_get_size_macro(handle_,size_)

#define DLOOP_Handle_get_basic_type_macro(handle_,basic_type_) \
    MPIR_Datatype_get_basic_type(handle_, basic_type_)

#define DLOOP_Handle_get_extent_macro(handle_,extent_) \
    MPIR_Datatype_get_extent_macro(handle_,extent_)

#define DLOOP_Handle_hasloop_macro(handle_)                           \
    ((HANDLE_GET_KIND(handle_) == HANDLE_KIND_BUILTIN) ? 0 : 1)

#define DLOOP_Ensure_Offset_fits_in_pointer(value_) \
    MPIR_Ensure_Aint_fits_in_pointer(value_)

/* allocate and free functions must also be defined. */
#define DLOOP_Malloc MPL_malloc
#define DLOOP_Free   MPL_free

/* assert function */
#define DLOOP_Assert MPIR_Assert

/* memory copy function */
#define DLOOP_Memcpy MPIR_Memcpy

/* casting macros */
#define DLOOP_OFFSET_CAST_TO_VOID_PTR MPIR_AINT_CAST_TO_VOID_PTR
#define DLOOP_VOID_PTR_CAST_TO_OFFSET MPIR_VOID_PTR_CAST_TO_MPI_AINT
#define DLOOP_PTR_DISP_CAST_TO_OFFSET MPIR_PTR_DISP_CAST_TO_MPI_AINT

/* printing macros */
#define DLOOP_OFFSET_FMT_DEC_SPEC MPI_AINT_FMT_DEC_SPEC
#define DLOOP_OFFSET_FMT_HEX_SPEC MPI_AINT_FMT_HEX_SPEC

/* Redefine all of the internal structures in terms of the prefix */
#define DLOOP_Dataloop              MPIR_Dataloop
#define DLOOP_Dataloop_contig       MPIR_Dataloop_contig
#define DLOOP_Dataloop_vector       MPIR_Dataloop_vector
#define DLOOP_Dataloop_blockindexed MPIR_Dataloop_blockindexed
#define DLOOP_Dataloop_indexed      MPIR_Dataloop_indexed
#define DLOOP_Dataloop_struct       MPIR_Dataloop_struct
#define DLOOP_Dataloop_common       MPIR_Dataloop_common
#define DLOOP_Segment               MPIR_Segment
#define DLOOP_Dataloop_stackelm     MPIR_Dataloop_stackelm

/* These flags are used at creation time to specify what types of
 * optimizations may be applied. They are also passed in at Segment_init
 * time to specify which dataloop to use.
 *
 * Note: The flag to MPIR_Segment_init() was originally simply "hetero"
 * and was a boolean value (0 meaning homogeneous). Some MPICH code
 * may still rely on HOMOGENEOUS being "0" and HETEROGENEOUS being "1".
 */
#define DLOOP_DATALOOP_HOMOGENEOUS   0
#define DLOOP_DATALOOP_HETEROGENEOUS 1
#define DLOOP_DATALOOP_ALL_BYTES     2

/* NOTE: ASSUMING LAST TYPE IS SIGNED */
#define SEGMENT_IGNORE_LAST ((DLOOP_Offset) -1)
/*
 * Each of the MPI datatypes can be mapped into one of 5 very simple
 * loops.  This loop has the following parameters:
 * - count
 * - blocksize[]
 * - offset[]
 * - stride
 * - datatype[]
 *
 * where each [] indicates that a field may be *either* an array or a scalar.
 * For each such type, we define a struct that describes these parameters
 */

/*S
  DLOOP_Dataloop_contig - Description of a contiguous dataloop

  Fields:
+ count - Number of elements
- dataloop - Dataloop of the elements

  Module:
  Datatype
  S*/
typedef struct DLOOP_Dataloop_contig {
    DLOOP_Count count;
    struct DLOOP_Dataloop *dataloop;
} DLOOP_Dataloop_contig;

/*S
  DLOOP_Dataloop_vector - Description of a vector or strided dataloop

  Fields:
+ count - Number of elements
. blocksize - Number of dataloops in each element
. stride - Stride (in bytes) between each block
- dataloop - Dataloop of each element

  Module:
  Datatype
  S*/
typedef struct DLOOP_Dataloop_vector {
    DLOOP_Count count;
    struct DLOOP_Dataloop *dataloop;
    DLOOP_Count blocksize;
    DLOOP_Offset stride;
} DLOOP_Dataloop_vector;

/*S
  DLOOP_Dataloop_blockindexed - Description of a block-indexed dataloop

  Fields:
+ count - Number of blocks
. blocksize - Number of elements in each block
. offset_array - Array of offsets (in bytes) to each block
- dataloop - Dataloop of each element

  Module:
  Datatype

  S*/
typedef struct DLOOP_Dataloop_blockindexed {
    DLOOP_Count count;
    struct DLOOP_Dataloop *dataloop;
    DLOOP_Count blocksize;
    DLOOP_Offset *offset_array;
} DLOOP_Dataloop_blockindexed;

/*S
  DLOOP_Dataloop_indexed - Description of an indexed dataloop

  Fields:
+ count - Number of blocks
. blocksize_array - Array giving the number of elements in each block
. offset_array - Array of offsets (in bytes) to each block
. total_blocks - count of total blocks in the array (cached value)
- dataloop - Dataloop of each element

  Module:
  Datatype

  S*/
typedef struct DLOOP_Dataloop_indexed {
    DLOOP_Count count;
    struct DLOOP_Dataloop *dataloop;
    DLOOP_Count *blocksize_array;
    DLOOP_Offset *offset_array;
    DLOOP_Count total_blocks;
} DLOOP_Dataloop_indexed;

/*S
  DLOOP_Dataloop_struct - Description of a structure dataloop

  Fields:
+ count - Number of blocks
. blocksize_array - Array giving the number of elements in each block
. offset_array - Array of offsets (in bytes) to each block
- dataloop_array - Array of dataloops describing the elements of each block

  Module:
  Datatype

  S*/
typedef struct DLOOP_Dataloop_struct {
    DLOOP_Count count;
    struct DLOOP_Dataloop **dataloop_array;
    DLOOP_Count            *blocksize_array;
    DLOOP_Offset           *offset_array;
    DLOOP_Offset           *el_extent_array; /* need more than one */
} DLOOP_Dataloop_struct;

/* In many cases, we need the count and the next dataloop item. This
   common structure gives a quick access to both.  Note that all other
   structures must use the same ordering of elements.
   Question: should we put the pointer first in case
   sizeof(pointer)>sizeof(int) ?
*/
typedef struct DLOOP_Dataloop_common {
    DLOOP_Count count;
    struct DLOOP_Dataloop *dataloop;
} DLOOP_Dataloop_common;

/*S
  DLOOP_Dataloop - Description of the structure used to hold a dataloop
  description

  Fields:
+  kind - Describes the type of the dataloop.  This is divided into three
   separate bit fields\:
.vb
     Dataloop type (e.g., DLOOP_CONTIG etc.).  3 bits
     IsFinal (a "leaf" dataloop; see text) 1 bit
     Element Size (units for fields.) 2 bits
        Element size has 4 values
        0   - Elements are in units of bytes
        1   - Elements are in units of 2 bytes
        2   - Elements are in units of 4 bytes
        3   - Elements are in units of 8 bytes
.ve
  The dataloop type is one of 'DLOOP_CONTIG', 'DLOOP_VECTOR',
  'DLOOP_BLOCKINDEXED', 'DLOOP_INDEXED', or 'DLOOP_STRUCT'.
. loop_parms - A union containing the 5 dataloop structures, e.g.,
  'DLOOP_Dataloop_contig', 'DLOOP_Dataloop_vector', etc.  A sixth element in
  this union, 'count', allows quick access to the shared 'count' field in the
  five dataloop structure.
. extent - The extent of the dataloop

  Module:
  Datatype

  S*/
typedef struct DLOOP_Dataloop {
    int kind;                  /* Contains both the loop type
                                  (contig, vector, blockindexed, indexed,
                                  or struct) and a bit that indicates
                                  whether the dataloop is a leaf type. */
    union {
        DLOOP_Count                 count;
        DLOOP_Dataloop_contig       c_t;
        DLOOP_Dataloop_vector       v_t;
        DLOOP_Dataloop_blockindexed bi_t;
        DLOOP_Dataloop_indexed      i_t;
        DLOOP_Dataloop_struct       s_t;
        DLOOP_Dataloop_common       cm_t;
    } loop_params;
    DLOOP_Offset el_size;
    DLOOP_Offset el_extent;
    DLOOP_Type   el_type;
} DLOOP_Dataloop;

#define DLOOP_FINAL_MASK  0x00000008
#define DLOOP_KIND_MASK   0x00000007
#define DLOOP_KIND_CONTIG 0x1
#define DLOOP_KIND_VECTOR 0x2
#define DLOOP_KIND_BLOCKINDEXED 0x3
#define DLOOP_KIND_INDEXED 0x4
#define DLOOP_KIND_STRUCT 0x5

/* The max datatype depth is the maximum depth of the stack used to
   evaluate datatypes.  It represents the length of the chain of
   datatype dependencies.  Defining this and testing when a datatype
   is created removes a test in the datatype evaluation loop. */
#define DLOOP_MAX_DATATYPE_DEPTH 16

/*S
  DLOOP_Dataloop_stackelm - Structure for an element of the stack used
  to process dataloops

  Fields:
+ curcount - Current loop count value (between 0 and
             loop.loop_params.count-1)
. orig_count - original count value (cached so we don't have to look it up)
. curoffset - Offset into memory relative to the pointer to the buffer
              passed in by the user.  Used to maintain our position as we
              move up and down the stack.  NEED MORE NOTES ON THIS!!!
. orig_offset - original offset, set before the stackelm is processed, so that
                we know where the offset was.  this is used in processing indexed
                types and possibly others.  it is set for all types, but not
                referenced in some cases.
. curblock - Current block value...NEED MORE NOTES ON THIS!!!
. orig_block - original block value (caches so we don't have to look it up);
               INVALID FOR INDEX AND STRUCT TYPES.
- loop_p  - pointer to Loop-based description of the dataloop

S*/
typedef struct DLOOP_Dataloop_stackelm {
    int may_require_reloading; /* indicates that items below might
                                * need reloading (e.g. this is a struct)
                                */

    DLOOP_Count  curcount;
    DLOOP_Offset curoffset;
    DLOOP_Count  curblock;

    DLOOP_Count  orig_count;
    DLOOP_Offset orig_offset;
    DLOOP_Count  orig_block;

    struct DLOOP_Dataloop *loop_p;
} DLOOP_Dataloop_stackelm;

/*S
  DLOOP_Segment - Description of the Segment datatype

  Notes:
  This has no corresponding MPI datatype.

  Module:
  Segment

  Questions:
  Should this have an id for allocation and similarity purposes?
  S*/
typedef struct DLOOP_Segment {
    void *ptr; /* pointer to datatype buffer */
    DLOOP_Handle handle;
    DLOOP_Offset stream_off; /* next offset into data stream resulting from datatype
                              * processing.  in other words, how many bytes have
                              * we created/used by parsing so far?  that amount + 1.
                              */
    DLOOP_Dataloop_stackelm stackelm[DLOOP_MAX_DATATYPE_DEPTH];
    int  cur_sp;   /* Current stack pointer when using dataloop */
    int  valid_sp; /* maximum valid stack pointer.  This is used to
                      maintain information on the stack after it has
                      been placed there by following the datatype field
                      in a DLOOP_Dataloop_st for any type except struct */

    struct DLOOP_Dataloop builtin_loop; /* used for both predefined types (which
                                          * won't have a loop already) and for
                                  * situations where a count is passed in
                                  * and we need to create a contig loop
                                  * to handle it
                                  */
    /* other, device-specific information */
} DLOOP_Segment;

/* Dataloop functions (dataloop.c) */
void MPIR_Dataloop_copy(void *dest,
                                   void *src,
                                   DLOOP_Size size);
void MPIR_Dataloop_update(DLOOP_Dataloop *dataloop,
                                     DLOOP_Offset ptrdiff);
DLOOP_Offset
MPIR_Dataloop_stream_size(DLOOP_Dataloop *dl_p,
                                     DLOOP_Offset (*sizefn)(DLOOP_Type el_type));
void MPIR_Dataloop_print(DLOOP_Dataloop *dataloop,
                                    int depth);

void MPIR_Dataloop_alloc(int kind,
                                    DLOOP_Count count,
                                    DLOOP_Dataloop **new_loop_p,
                                    DLOOP_Size *new_loop_sz_p);
void MPIR_Dataloop_alloc_and_copy(int kind,
                                             DLOOP_Count count,
                                             DLOOP_Dataloop *old_loop,
                                             DLOOP_Size old_loop_sz,
                                             DLOOP_Dataloop **new_loop_p,
                                             DLOOP_Size *new_loop_sz_p);
void MPIR_Dataloop_struct_alloc(DLOOP_Count count,
                                           DLOOP_Size old_loop_sz,
                                           int basic_ct,
                                           DLOOP_Dataloop **old_loop_p,
                                           DLOOP_Dataloop **new_loop_p,
                                           DLOOP_Size *new_loop_sz_p);
void MPIR_Dataloop_dup(DLOOP_Dataloop *old_loop,
                                  DLOOP_Size old_loop_sz,
                                  DLOOP_Dataloop **new_loop_p);

void MPIR_Dataloop_free(DLOOP_Dataloop **dataloop);

/* Segment functions (segment.c) */
DLOOP_Segment * MPIR_Segment_alloc(void);

void MPIR_Segment_free(DLOOP_Segment *segp);

int MPIR_Segment_init(const DLOOP_Buffer buf,
                                 DLOOP_Count count,
                                 DLOOP_Handle handle,
                                 DLOOP_Segment *segp,
                                 int hetero);

void
MPIR_Segment_manipulate(DLOOP_Segment *segp,
                                   DLOOP_Offset first,
                                   DLOOP_Offset *lastp,
                                   int (*piecefn) (DLOOP_Offset *blocks_p,
                                                   DLOOP_Type el_type,
                                                   DLOOP_Offset rel_off,
                                                   DLOOP_Buffer bufp,
                                                   void *v_paramp),
                                   int (*vectorfn) (DLOOP_Offset *blocks_p,
                                                    DLOOP_Count count,
                                                    DLOOP_Count blklen,
                                                    DLOOP_Offset stride,
                                                    DLOOP_Type el_type,
                                                    DLOOP_Offset rel_off,
                                                    DLOOP_Buffer bufp,
                                                    void *v_paramp),
                                   int (*blkidxfn) (DLOOP_Offset *blocks_p,
                                                    DLOOP_Count count,
                                                    DLOOP_Count blklen,
                                                    DLOOP_Offset *offsetarray,
                                                    DLOOP_Type el_type,
                                                    DLOOP_Offset rel_off,
                                                    DLOOP_Buffer bufp,
                                                    void *v_paramp),
                                   int (*indexfn) (DLOOP_Offset *blocks_p,
                                                   DLOOP_Count count,
                                                   DLOOP_Count *blockarray,
                                                   DLOOP_Offset *offsetarray,
                                                   DLOOP_Type el_type,
                                                   DLOOP_Offset rel_off,
                                                   DLOOP_Buffer bufp,
                                                   void *v_paramp),
                                   DLOOP_Offset (*sizefn) (DLOOP_Type el_type),
                                   void *pieceparams);

/* Common segment operations (segment_ops.c) */
void MPIR_Segment_count_contig_blocks(DLOOP_Segment *segp,
                                                 DLOOP_Offset first,
                                                 DLOOP_Offset *lastp,
                                                 DLOOP_Count *countp);
void MPIR_Segment_mpi_flatten(DLOOP_Segment *segp,
                                         DLOOP_Offset first,
                                         DLOOP_Offset *lastp,
                                         DLOOP_Size *blklens,
                                         MPI_Aint *disps,
                                         DLOOP_Size *lengthp);

#define DLOOP_M2M_TO_USERBUF   0
#define DLOOP_M2M_FROM_USERBUF 1

struct MPIR_m2m_params {
    int direction; /* M2M_TO_USERBUF or M2M_FROM_USERBUF */
    char *streambuf;
    char *userbuf;
};

void MPIR_Segment_pack(struct DLOOP_Segment *segp,
                                  DLOOP_Offset   first,
                                  DLOOP_Offset  *lastp,
                                  void *streambuf);
void MPIR_Segment_unpack(struct DLOOP_Segment *segp,
                                    DLOOP_Offset   first,
                                    DLOOP_Offset  *lastp,
                                    void *streambuf);

/* Segment piece functions that are used in specific cases elsewhere */
int MPIR_Segment_contig_m2m(DLOOP_Offset *blocks_p,
                                       DLOOP_Type el_type,
                                       DLOOP_Offset rel_off,
                                       void *bufp, /* unused */
                                       void *v_paramp);
int MPIR_Segment_vector_m2m(DLOOP_Offset *blocks_p,
                                       DLOOP_Count count, /* unused */
                                       DLOOP_Count blksz,
                                       DLOOP_Offset stride,
                                       DLOOP_Type el_type,
                                       DLOOP_Offset rel_off,
                                       void *bufp, /* unused */
                                       void *v_paramp);
int MPIR_Segment_blkidx_m2m(DLOOP_Offset *blocks_p,
                                       DLOOP_Count count,
                                       DLOOP_Count blocklen,
                                       DLOOP_Offset *offsetarray,
                                       DLOOP_Type el_type,
                                       DLOOP_Offset rel_off,
                                       void *bufp, /*unused */
                                       void *v_paramp);
int MPIR_Segment_index_m2m(DLOOP_Offset *blocks_p,
                                      DLOOP_Count count,
                                      DLOOP_Count *blockarray,
                                      DLOOP_Offset *offsetarray,
                                      DLOOP_Type el_type,
                                      DLOOP_Offset rel_off,
                                      void *bufp, /*unused */
                                      void *v_paramp);

/* Dataloop construction functions */
void MPIR_Dataloop_create(MPI_Datatype type,
                                     DLOOP_Dataloop **dlp_p,
                                     DLOOP_Size *dlsz_p,
                                     int *dldepth_p,
                                     int flag);
int MPIR_Dataloop_create_contiguous(DLOOP_Count count,
                                               MPI_Datatype oldtype,
                                               DLOOP_Dataloop **dlp_p,
                                               DLOOP_Size *dlsz_p,
                                               int *dldepth_p,
                                               int flag);
int MPIR_Dataloop_create_vector(DLOOP_Count count,
                                           DLOOP_Size blocklength,
                                           MPI_Aint stride,
                                           int strideinbytes,
                                           MPI_Datatype oldtype,
                                           DLOOP_Dataloop **dlp_p,
                                           DLOOP_Size *dlsz_p,
                                           int *dldepth_p,
                                           int flag);
int MPIR_Dataloop_create_blockindexed(DLOOP_Count count,
                                                 DLOOP_Size blklen,
                                                 const void *disp_array,
                                                 int dispinbytes,
                                                 MPI_Datatype oldtype,
                                                 DLOOP_Dataloop **dlp_p,
                                                 DLOOP_Size *dlsz_p,
                                                 int *dldepth_p,
                                                 int flag);
/* we bump up the size of the blocklength array because create_struct might use
 * create_indexed in an optimization, and in course of doing so, generate a
 * request of a large blocklength. */
int MPIR_Dataloop_create_indexed(DLOOP_Count count,
                                            const DLOOP_Size *blocklength_array,
                                            const void *displacement_array,
                                            int dispinbytes,
                                            MPI_Datatype oldtype,
                                            DLOOP_Dataloop **dlp_p,
                                            DLOOP_Size *dlsz_p,
                                            int *dldepth_p,
                                            int flag);
int MPIR_Dataloop_create_struct(DLOOP_Count count,
                                           const int *blklen_array,
                                           const MPI_Aint *disp_array,
                                           const MPI_Datatype *oldtype_array,
                                           DLOOP_Dataloop **dlp_p,
                                           DLOOP_Size *dlsz_p,
                                           int *dldepth_p,
                                           int flag);
int MPIR_Dataloop_create_pairtype(MPI_Datatype type,
                                             DLOOP_Dataloop **dlp_p,
                                             DLOOP_Size *dlsz_p,
                                             int *dldepth_p,
                                             int flag);

/* Helper functions for dataloop construction */
int MPIR_Type_convert_subarray(int ndims,
                                          int *array_of_sizes,
                                          int *array_of_subsizes,
                                          int *array_of_starts,
                                          int order,
                                          MPI_Datatype oldtype,
                                          MPI_Datatype *newtype);
int MPIR_Type_convert_darray(int size,
                                        int rank,
                                        int ndims,
                                        int *array_of_gsizes,
                                        int *array_of_distribs,
                                        int *array_of_dargs,
                                        int *array_of_psizes,
                                        int order,
                                        MPI_Datatype oldtype,
                                        MPI_Datatype *newtype);

DLOOP_Count MPIR_Type_indexed_count_contig(DLOOP_Count count,
                                                      const DLOOP_Count *blocklength_array,
                                                      const void *displacement_array,
                                                      int dispinbytes,
                                                      DLOOP_Offset old_extent);

DLOOP_Count MPIR_Type_blockindexed_count_contig(DLOOP_Count count,
                                                           DLOOP_Count blklen,
                                                           const void *disp_array,
                                                           int dispinbytes,
                                                           DLOOP_Offset old_extent);

int MPIR_Type_blockindexed(int count,
                           int blocklength,
                           const void *displacement_array,
                           int dispinbytes,
                           MPI_Datatype oldtype,
                           MPI_Datatype *newtype);

int MPIR_Type_commit(MPI_Datatype *type);

/* Segment functions specific to MPICH */
void MPIR_Segment_pack_vector(struct DLOOP_Segment *segp,
                              DLOOP_Offset first,
                              DLOOP_Offset *lastp,
                              DLOOP_VECTOR *vector,
                              int *lengthp);

void MPIR_Segment_unpack_vector(struct DLOOP_Segment *segp,
                                DLOOP_Offset first,
                                DLOOP_Offset *lastp,
                                DLOOP_VECTOR *vector,
                                int *lengthp);

void MPIR_Segment_flatten(struct DLOOP_Segment *segp,
                          DLOOP_Offset first,
                          DLOOP_Offset *lastp,
                          DLOOP_Offset *offp,
                          DLOOP_Size *sizep,
                          DLOOP_Offset *lengthp);

void MPIR_Segment_pack_external32(struct DLOOP_Segment *segp,
                                  DLOOP_Offset first,
                                  DLOOP_Offset *lastp,
                                  void *pack_buffer);

void MPIR_Segment_unpack_external32(struct DLOOP_Segment *segp,
                                    DLOOP_Offset first,
                                    DLOOP_Offset *lastp,
                                    DLOOP_Buffer unpack_buffer);

/* These values are defined by DLOOP code.
 *
 * Note: DLOOP_DATALOOP_ALL_BYTES is used only when the device
 * defines MPID_NEEDS_DLOOP_ALL_BYTES.
 */
#define MPIR_DATALOOP_HETEROGENEOUS DLOOP_DATALOOP_HETEROGENEOUS
#define MPIR_DATALOOP_HOMOGENEOUS   DLOOP_DATALOOP_HOMOGENEOUS
#define MPIR_DATALOOP_ALL_BYTES     DLOOP_DATALOOP_ALL_BYTES

DLOOP_Count DLOOP_Stackelm_blocksize(struct DLOOP_Dataloop_stackelm *elmp);
DLOOP_Offset DLOOP_Stackelm_offset(struct DLOOP_Dataloop_stackelm *elmp);
void DLOOP_Stackelm_load(struct DLOOP_Dataloop_stackelm *elmp,
                         struct DLOOP_Dataloop *dlp,
                         int branch_flag);

#endif
