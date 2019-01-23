/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */

/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MPIR_DATALOOP_H_INCLUDED
#define MPIR_DATALOOP_H_INCLUDED

#include <mpi.h>

/*
 * The following macro allows us to reference the regular
 * value for the 3 fields (NULL,_size,_depth) in the
 * MPIR_Datatype structure.  This is used in the many
 * macros that access fields of the datatype.  We need this macro
 * to simplify the definition of the other macros.
 */
#define MPIR_DATALOOP_GET_FIELD(value_,fieldname_) \
      value_ = ((MPIR_Datatype *)ptr)->dataloop##fieldname_

#define MPIR_DATALOOP_SET_FIELD(value_,fieldname_) \
    ((MPIR_Datatype *)ptr)->dataloop##fieldname_ = value_

/* The following accessor functions must also be defined:
 *
 * DLOOP_Handle_extent()
 * DLOOP_Handle_size()
 * DLOOP_Handle_loopptr()
 * DLOOP_Handle_hasloop()
 *
 */

/* USE THE NOTATION THAT BILL USED IN MPIIMPL.H AND MAKE THESE MACROS */

/* NOTE: put get size into mpiimpl.h; the others go here until such time
 * as we see that we need them elsewhere.
 */
#define DLOOP_Handle_hasloop_macro(handle_)                           \
    ((HANDLE_GET_KIND(handle_) == HANDLE_KIND_BUILTIN) ? 0 : 1)

/* NOTE: ASSUMING LAST TYPE IS SIGNED */
#define SEGMENT_IGNORE_LAST ((MPI_Aint) -1)
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

struct MPIR_Dataloop;

/*S
  MPIR_Dataloop_contig - Description of a contiguous dataloop

  Fields:
+ count - Number of elements
- dataloop - Dataloop of the elements

  Module:
  Datatype
  S*/
typedef struct MPIR_Dataloop_contig {
    MPI_Aint count;
    struct MPIR_Dataloop *dataloop;
} MPIR_Dataloop_contig;

/*S
  MPIR_Dataloop_vector - Description of a vector or strided dataloop

  Fields:
+ count - Number of elements
. blocksize - Number of dataloops in each element
. stride - Stride (in bytes) between each block
- dataloop - Dataloop of each element

  Module:
  Datatype
  S*/
typedef struct MPIR_Dataloop_vector {
    MPI_Aint count;
    struct MPIR_Dataloop *dataloop;
    MPI_Aint blocksize;
    MPI_Aint stride;
} MPIR_Dataloop_vector;

/*S
  MPIR_Dataloop_blockindexed - Description of a block-indexed dataloop

  Fields:
+ count - Number of blocks
. blocksize - Number of elements in each block
. offset_array - Array of offsets (in bytes) to each block
- dataloop - Dataloop of each element

  Module:
  Datatype

  S*/
typedef struct MPIR_Dataloop_blockindexed {
    MPI_Aint count;
    struct MPIR_Dataloop *dataloop;
    MPI_Aint blocksize;
    MPI_Aint *offset_array;
} MPIR_Dataloop_blockindexed;

/*S
  MPIR_Dataloop_indexed - Description of an indexed dataloop

  Fields:
+ count - Number of blocks
. blocksize_array - Array giving the number of elements in each block
. offset_array - Array of offsets (in bytes) to each block
. total_blocks - count of total blocks in the array (cached value)
- dataloop - Dataloop of each element

  Module:
  Datatype

  S*/
typedef struct MPIR_Dataloop_indexed {
    MPI_Aint count;
    struct MPIR_Dataloop *dataloop;
    MPI_Aint *blocksize_array;
    MPI_Aint *offset_array;
    MPI_Aint total_blocks;
} MPIR_Dataloop_indexed;

/*S
  MPIR_Dataloop_struct - Description of a structure dataloop

  Fields:
+ count - Number of blocks
. blocksize_array - Array giving the number of elements in each block
. offset_array - Array of offsets (in bytes) to each block
- dataloop_array - Array of dataloops describing the elements of each block

  Module:
  Datatype

  S*/
typedef struct MPIR_Dataloop_struct {
    MPI_Aint count;
    struct MPIR_Dataloop **dataloop_array;
    MPI_Aint *blocksize_array;
    MPI_Aint *offset_array;
    MPI_Aint *el_extent_array;  /* need more than one */
} MPIR_Dataloop_struct;

/* In many cases, we need the count and the next dataloop item. This
   common structure gives a quick access to both.  Note that all other
   structures must use the same ordering of elements.
   Question: should we put the pointer first in case
   sizeof(pointer)>sizeof(int) ?
*/
typedef struct MPIR_Dataloop_common {
    MPI_Aint count;
    struct MPIR_Dataloop *dataloop;
} MPIR_Dataloop_common;

/*S
  MPIR_Dataloop - Description of the structure used to hold a dataloop
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
  The dataloop type is one of 'DLOOP_CONTIG', 'MPL_IOV',
  'DLOOP_BLOCKINDEXED', 'DLOOP_INDEXED', or 'DLOOP_STRUCT'.
. loop_parms - A union containing the 5 dataloop structures, e.g.,
  'MPIR_Dataloop_contig', 'MPIR_Dataloop_vector', etc.  A sixth element in
  this union, 'count', allows quick access to the shared 'count' field in the
  five dataloop structure.
. extent - The extent of the dataloop

  Module:
  Datatype

  S*/
typedef struct MPIR_Dataloop {
    int kind;                   /* Contains both the loop type
                                 * (contig, vector, blockindexed, indexed,
                                 * or struct) and a bit that indicates
                                 * whether the dataloop is a leaf type. */
    union {
        MPI_Aint count;
        MPIR_Dataloop_contig c_t;
        MPIR_Dataloop_vector v_t;
        MPIR_Dataloop_blockindexed bi_t;
        MPIR_Dataloop_indexed i_t;
        MPIR_Dataloop_struct s_t;
        MPIR_Dataloop_common cm_t;
    } loop_params;
    MPI_Aint el_size;
    MPI_Aint el_extent;
    MPI_Datatype el_type;
} MPIR_Dataloop;

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
  MPIR_Dataloop_stackelm - Structure for an element of the stack used
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
typedef struct MPIR_Dataloop_stackelm {
    int may_require_reloading;  /* indicates that items below might
                                 * need reloading (e.g. this is a struct)
                                 */

    MPI_Aint curcount;
    MPI_Aint curoffset;
    MPI_Aint curblock;

    MPI_Aint orig_count;
    MPI_Aint orig_offset;
    MPI_Aint orig_block;

    struct MPIR_Dataloop *loop_p;
} MPIR_Dataloop_stackelm;

/*S
  MPIR_Segment - Description of the Segment datatype

  Notes:
  This has no corresponding MPI datatype.

  Module:
  Segment

  Questions:
  Should this have an id for allocation and similarity purposes?
  S*/
typedef struct MPIR_Segment {
    void *ptr;                  /* pointer to datatype buffer */
    MPI_Datatype handle;
    MPI_Aint stream_off;        /* next offset into data stream resulting from datatype
                                 * processing.  in other words, how many bytes have
                                 * we created/used by parsing so far?  that amount + 1.
                                 */
    MPIR_Dataloop_stackelm stackelm[DLOOP_MAX_DATATYPE_DEPTH];
    int cur_sp;                 /* Current stack pointer when using dataloop */
    int valid_sp;               /* maximum valid stack pointer.  This is used to
                                 * maintain information on the stack after it has
                                 * been placed there by following the datatype field
                                 * in a DLOOP_Dataloop_st for any type except struct */

    struct MPIR_Dataloop builtin_loop;  /* used for both predefined types (which
                                         * won't have a loop already) and for
                                         * situations where a count is passed in
                                         * and we need to create a contig loop
                                         * to handle it
                                         */
    /* other, device-specific information */
} MPIR_Segment;

/* Dataloop functions (dataloop.c) */
void MPIR_Dataloop_copy(void *dest, void *src, MPI_Aint size);
void MPIR_Dataloop_update(MPIR_Dataloop * dataloop, MPI_Aint ptrdiff);
MPI_Aint MPIR_Dataloop_stream_size(MPIR_Dataloop * dl_p, MPI_Aint(*sizefn) (MPI_Datatype el_type));
void MPIR_Dataloop_print(MPIR_Dataloop * dataloop, int depth);

void MPIR_Dataloop_alloc(int kind,
                         MPI_Aint count, MPIR_Dataloop ** new_loop_p, MPI_Aint * new_loop_sz_p);
void MPIR_Dataloop_alloc_and_copy(int kind,
                                  MPI_Aint count,
                                  MPIR_Dataloop * old_loop,
                                  MPI_Aint old_loop_sz,
                                  MPIR_Dataloop ** new_loop_p, MPI_Aint * new_loop_sz_p);
void MPIR_Dataloop_struct_alloc(MPI_Aint count,
                                MPI_Aint old_loop_sz,
                                int basic_ct,
                                MPIR_Dataloop ** old_loop_p,
                                MPIR_Dataloop ** new_loop_p, MPI_Aint * new_loop_sz_p);
void MPIR_Dataloop_dup(MPIR_Dataloop * old_loop, MPI_Aint old_loop_sz, MPIR_Dataloop ** new_loop_p);

void MPIR_Dataloop_free(MPIR_Dataloop ** dataloop);

/* Segment functions (segment.c) */
MPIR_Segment *MPIR_Segment_alloc(void);

void MPIR_Segment_free(MPIR_Segment * segp);

int MPIR_Segment_init(const void *buf, MPI_Aint count, MPI_Datatype handle, MPIR_Segment * segp);

void
MPIR_Segment_manipulate(MPIR_Segment * segp,
                        MPI_Aint first,
                        MPI_Aint * lastp,
                        int (*piecefn) (MPI_Aint * blocks_p,
                                        MPI_Datatype el_type,
                                        MPI_Aint rel_off,
                                        void *bufp,
                                        void *v_paramp),
                        int (*vectorfn) (MPI_Aint * blocks_p,
                                         MPI_Aint count,
                                         MPI_Aint blklen,
                                         MPI_Aint stride,
                                         MPI_Datatype el_type,
                                         MPI_Aint rel_off,
                                         void *bufp,
                                         void *v_paramp),
                        int (*blkidxfn) (MPI_Aint * blocks_p,
                                         MPI_Aint count,
                                         MPI_Aint blklen,
                                         MPI_Aint * offsetarray,
                                         MPI_Datatype el_type,
                                         MPI_Aint rel_off,
                                         void *bufp,
                                         void *v_paramp),
                        int (*indexfn) (MPI_Aint * blocks_p,
                                        MPI_Aint count,
                                        MPI_Aint * blockarray,
                                        MPI_Aint * offsetarray,
                                        MPI_Datatype el_type,
                                        MPI_Aint rel_off,
                                        void *bufp,
                                        void *v_paramp),
                        MPI_Aint(*sizefn) (MPI_Datatype el_type), void *pieceparams);

/* Common segment operations (segment_ops.c) */
void MPIR_Segment_count_contig_blocks(MPIR_Segment * segp,
                                      MPI_Aint first, MPI_Aint * lastp, MPI_Aint * countp);
void MPIR_Segment_mpi_flatten(MPIR_Segment * segp,
                              MPI_Aint first,
                              MPI_Aint * lastp,
                              MPI_Aint * blklens, MPI_Aint * disps, MPI_Aint * lengthp);

#define DLOOP_M2M_TO_USERBUF   0
#define DLOOP_M2M_FROM_USERBUF 1

struct MPIR_m2m_params {
    int direction;              /* M2M_TO_USERBUF or M2M_FROM_USERBUF */
    char *streambuf;
    char *userbuf;
};

void MPIR_Segment_pack(struct MPIR_Segment *segp,
                       MPI_Aint first, MPI_Aint * lastp, void *streambuf);
void MPIR_Segment_unpack(struct MPIR_Segment *segp,
                         MPI_Aint first, MPI_Aint * lastp, void *streambuf);

/* Segment piece functions that are used in specific cases elsewhere */
int MPIR_Segment_contig_m2m(MPI_Aint * blocks_p, MPI_Datatype el_type, MPI_Aint rel_off, void *bufp,    /* unused */
                            void *v_paramp);
int MPIR_Segment_vector_m2m(MPI_Aint * blocks_p, MPI_Aint count,        /* unused */
                            MPI_Aint blksz, MPI_Aint stride, MPI_Datatype el_type, MPI_Aint rel_off, void *bufp,        /* unused */
                            void *v_paramp);
int MPIR_Segment_blkidx_m2m(MPI_Aint * blocks_p, MPI_Aint count, MPI_Aint blocklen, MPI_Aint * offsetarray, MPI_Datatype el_type, MPI_Aint rel_off, void *bufp, /*unused */
                            void *v_paramp);
int MPIR_Segment_index_m2m(MPI_Aint * blocks_p, MPI_Aint count, MPI_Aint * blockarray, MPI_Aint * offsetarray, MPI_Datatype el_type, MPI_Aint rel_off, void *bufp,      /*unused */
                           void *v_paramp);

/* Dataloop construction functions */
void MPIR_Dataloop_create(MPI_Datatype type, MPIR_Dataloop ** dlp_p, MPI_Aint * dlsz_p);
int MPIR_Dataloop_create_contiguous(MPI_Aint count,
                                    MPI_Datatype oldtype,
                                    MPIR_Dataloop ** dlp_p, MPI_Aint * dlsz_p);
int MPIR_Dataloop_create_vector(MPI_Aint count,
                                MPI_Aint blocklength,
                                MPI_Aint stride,
                                int strideinbytes,
                                MPI_Datatype oldtype, MPIR_Dataloop ** dlp_p, MPI_Aint * dlsz_p);
int MPIR_Dataloop_create_blockindexed(MPI_Aint count,
                                      MPI_Aint blklen,
                                      const void *disp_array,
                                      int dispinbytes,
                                      MPI_Datatype oldtype,
                                      MPIR_Dataloop ** dlp_p, MPI_Aint * dlsz_p);
/* we bump up the size of the blocklength array because create_struct might use
 * create_indexed in an optimization, and in course of doing so, generate a
 * request of a large blocklength. */
int MPIR_Dataloop_create_indexed(MPI_Aint count,
                                 const MPI_Aint * blocklength_array,
                                 const void *displacement_array,
                                 int dispinbytes,
                                 MPI_Datatype oldtype, MPIR_Dataloop ** dlp_p, MPI_Aint * dlsz_p);
int MPIR_Dataloop_create_struct(MPI_Aint count,
                                const int *blklen_array,
                                const MPI_Aint * disp_array,
                                const MPI_Datatype * oldtype_array,
                                MPIR_Dataloop ** dlp_p, MPI_Aint * dlsz_p);
int MPIR_Dataloop_create_pairtype(MPI_Datatype type, MPIR_Dataloop ** dlp_p, MPI_Aint * dlsz_p);

/* Helper functions for dataloop construction */
int MPIR_Type_convert_subarray(int ndims,
                               int *array_of_sizes,
                               int *array_of_subsizes,
                               int *array_of_starts,
                               int order, MPI_Datatype oldtype, MPI_Datatype * newtype);
int MPIR_Type_convert_darray(int size,
                             int rank,
                             int ndims,
                             int *array_of_gsizes,
                             int *array_of_distribs,
                             int *array_of_dargs,
                             int *array_of_psizes,
                             int order, MPI_Datatype oldtype, MPI_Datatype * newtype);

MPI_Aint MPIR_Type_indexed_count_contig(MPI_Aint count,
                                        const MPI_Aint * blocklength_array,
                                        const void *displacement_array,
                                        int dispinbytes, MPI_Aint old_extent);

MPI_Aint MPIR_Type_blockindexed_count_contig(MPI_Aint count,
                                             MPI_Aint blklen,
                                             const void *disp_array,
                                             int dispinbytes, MPI_Aint old_extent);

int MPIR_Type_blockindexed(int count,
                           int blocklength,
                           const void *displacement_array,
                           int dispinbytes, MPI_Datatype oldtype, MPI_Datatype * newtype);

int MPIR_Type_commit(MPI_Datatype * type);

/* Segment functions specific to MPICH */
void MPIR_Segment_pack_vector(struct MPIR_Segment *segp,
                              MPI_Aint first, MPI_Aint * lastp, MPL_IOV * vector, int *lengthp);

void MPIR_Segment_unpack_vector(struct MPIR_Segment *segp,
                                MPI_Aint first, MPI_Aint * lastp, MPL_IOV * vector, int *lengthp);

void MPIR_Segment_flatten(struct MPIR_Segment *segp,
                          MPI_Aint first,
                          MPI_Aint * lastp, MPI_Aint * offp, MPI_Aint * sizep, MPI_Aint * lengthp);

void MPIR_Segment_pack_external32(struct MPIR_Segment *segp,
                                  MPI_Aint first, MPI_Aint * lastp, void *pack_buffer);

void MPIR_Segment_unpack_external32(struct MPIR_Segment *segp,
                                    MPI_Aint first, MPI_Aint * lastp, void *unpack_buffer);

MPI_Aint DLOOP_Stackelm_blocksize(struct MPIR_Dataloop_stackelm *elmp);
MPI_Aint DLOOP_Stackelm_offset(struct MPIR_Dataloop_stackelm *elmp);
void DLOOP_Stackelm_load(struct MPIR_Dataloop_stackelm *elmp,
                         struct MPIR_Dataloop *dlp, int branch_flag);

#endif /* MPIR_DATALOOP_H_INCLUDED */
