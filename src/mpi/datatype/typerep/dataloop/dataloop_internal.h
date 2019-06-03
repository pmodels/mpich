/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */

/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef DATALOOP_INTERNAL_H_INCLUDED
#define DATALOOP_INTERNAL_H_INCLUDED

#include <mpiimpl.h>
#include "dataloop.h"

#define MPII_DATALOOP_HANDLE_HASLOOP(handle_)                           \
    ((HANDLE_GET_KIND(handle_) == HANDLE_KIND_BUILTIN) ? 0 : 1)

#define MPII_DATALOOP_FINAL_MASK  0x00000008
#define MPII_DATALOOP_KIND_MASK   0x00000007
#define MPII_DATALOOP_KIND_CONTIG 0x1
#define MPII_DATALOOP_KIND_VECTOR 0x2
#define MPII_DATALOOP_KIND_BLOCKINDEXED 0x3
#define MPII_DATALOOP_KIND_INDEXED 0x4
#define MPII_DATALOOP_KIND_STRUCT 0x5

#define MPII_DATALOOP_GET_LOOPPTR(a,lptr_) do {                         \
        void *ptr;                                                      \
        switch (HANDLE_GET_KIND(a)) {                                   \
        case HANDLE_KIND_DIRECT:                                        \
            MPIR_Assert(HANDLE_INDEX(a) < MPIR_DATATYPE_PREALLOC);      \
            ptr = MPIR_Datatype_direct+HANDLE_INDEX(a);                 \
            lptr_ = ((MPIR_Datatype *)ptr)->typerep;                    \
            break;                                                      \
        case HANDLE_KIND_INDIRECT:                                      \
            ptr = ((MPIR_Datatype *)                                    \
                   MPIR_Handle_get_ptr_indirect(a,&MPIR_Datatype_mem)); \
            lptr_ = ((MPIR_Datatype *)ptr)->typerep;                    \
            break;                                                      \
        case HANDLE_KIND_INVALID:                                       \
        case HANDLE_KIND_BUILTIN:                                       \
        default:                                                        \
            lptr_ = 0;                                                  \
            break;                                                      \
        }                                                               \
    } while (0)

#define MPII_DATALOOP_SET_LOOPPTR(a,lptr_) do {                         \
        void *ptr;                                                      \
        switch (HANDLE_GET_KIND(a)) {                                   \
        case HANDLE_KIND_DIRECT:                                        \
            MPIR_Assert(HANDLE_INDEX(a) < MPIR_DATATYPE_PREALLOC);      \
            ptr = MPIR_Datatype_direct+HANDLE_INDEX(a);                 \
            ((MPIR_Datatype *)ptr)->typerep = lptr_;                    \
            break;                                                      \
        case HANDLE_KIND_INDIRECT:                                      \
            ptr = ((MPIR_Datatype *)                                    \
                   MPIR_Handle_get_ptr_indirect(a,&MPIR_Datatype_mem)); \
            ((MPIR_Datatype *)ptr)->typerep = lptr_;                    \
            break;                                                      \
        case HANDLE_KIND_INVALID:                                       \
        case HANDLE_KIND_BUILTIN:                                       \
        default:                                                        \
            lptr_ = 0;                                                  \
            break;                                                      \
        }                                                               \
    } while (0)

/* The max datatype depth is the maximum depth of the stack used to
   evaluate datatypes.  It represents the length of the chain of
   datatype dependencies.  Defining this and testing when a datatype
   is created removes a test in the datatype evaluation loop. */
#define MPII_DATALOOP_MAX_DATATYPE_DEPTH 16

/*S
  MPII_Dataloop_stackelm - Structure for an element of the stack used
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
typedef struct MPII_Dataloop_stackelm {
    int may_require_reloading;  /* indicates that items below might
                                 * need reloading (e.g. this is a struct)
                                 */

    size_t curcount;
    size_t curoffset;
    size_t curblock;

    size_t orig_count;
    size_t orig_offset;
    size_t orig_block;

    struct MPII_Dataloop *loop_p;
} MPII_Dataloop_stackelm;

/*S
  MPII_Dataloop - Description of the structure used to hold a dataloop
  description

  Fields:
+  kind - Describes the type of the dataloop.  This is divided into three
   separate bit fields\:
.vb
     Dataloop type.  3 bits
     IsFinal (a "leaf" dataloop; see text) 1 bit
     Element Size (units for fields.) 2 bits
        Element size has 4 values
        0   - Elements are in units of bytes
        1   - Elements are in units of 2 bytes
        2   - Elements are in units of 4 bytes
        3   - Elements are in units of 8 bytes
.ve
. loop_parms - A union containing the 5 dataloop structures.  A sixth element in
  this union, 'count', allows quick access to the shared 'count' field in the
  five dataloop structure.
. extent - The extent of the dataloop

  Module:
  Datatype

  S*/
typedef struct MPII_Dataloop {
    int kind;                   /* Contains both the loop type
                                 * (contig, vector, blockindexed, indexed,
                                 * or struct) and a bit that indicates
                                 * whether the dataloop is a leaf type. */
    union {
        size_t count;
        struct {
            size_t count;
            struct MPII_Dataloop *dataloop;
        } c_t;
        struct {
            size_t count;
            struct MPII_Dataloop *dataloop;
            size_t blocksize;
            size_t stride;
        } v_t;
        struct {
            size_t count;
            struct MPII_Dataloop *dataloop;
            size_t blocksize;
            size_t *offset_array;
        } bi_t;
        struct {
            size_t count;
            struct MPII_Dataloop *dataloop;
            size_t *blocksize_array;
            size_t *offset_array;
            size_t total_blocks;
        } i_t;
        struct {
            size_t count;
            struct MPII_Dataloop **dataloop_array;
            size_t *blocksize_array;
            size_t *offset_array;
            MPI_Aint *el_extent_array;  /* need more than one */
        } s_t;
        struct {
            size_t count;
            struct MPII_Dataloop *dataloop;
        } cm_t;
    } loop_params;
    size_t el_size;
    size_t el_extent;
    MPI_Datatype el_type;

    size_t dloop_sz;
} MPII_Dataloop;

static int MPII_Type_dloop_size(MPI_Datatype type) ATTRIBUTE((unused));
static int MPII_Type_dloop_size(MPI_Datatype type)
{
    MPII_Dataloop *dloop;

    MPII_DATALOOP_GET_LOOPPTR(type, dloop);
    return dloop->dloop_sz;
}


/*S
  MPIR_Segment - Description of the Segment datatype

  Notes:
  This has no corresponding MPI datatype.

  Module:
  Segment

  Questions:
  Should this have an id for allocation and similarity purposes?
  S*/
struct MPIR_Segment {
    void *ptr;                  /* pointer to datatype buffer */
    MPI_Datatype handle;
    size_t stream_off;        /* next offset into data stream resulting from datatype
                                 * processing.  in other words, how many bytes have
                                 * we created/used by parsing so far?  that amount + 1.
                                 */
    MPII_Dataloop_stackelm stackelm[MPII_DATALOOP_MAX_DATATYPE_DEPTH];
    int cur_sp;                 /* Current stack pointer when using dataloop */
    int valid_sp;               /* maximum valid stack pointer.  This is used to
                                 * maintain information on the stack after it has
                                 * been placed there by following the datatype field
                                 * for any type except struct */
    MPII_Dataloop builtin_loop; /* used for both predefined types
                                 * (which won't have a loop already)
                                 * and for situations where a count
                                 * is passed in and we need to create
                                 * a contig loop to handle it
                                 */
};

struct MPII_Dataloop_m2m_params {
    int direction;              /* M2M_TO_USERBUF or M2M_FROM_USERBUF */
    char *streambuf;
    char *userbuf;
};

size_t MPII_Dataloop_stackelm_blocksize(struct MPII_Dataloop_stackelm *elmp);
size_t MPII_Dataloop_stackelm_offset(struct MPII_Dataloop_stackelm *elmp);
void MPII_Dataloop_stackelm_load(struct MPII_Dataloop_stackelm *elmp,
                                 MPII_Dataloop * dlp, int branch_flag);

int MPII_Dataloop_create_contiguous(size_t count, MPI_Datatype oldtype, MPII_Dataloop ** dlp_p);
int MPII_Dataloop_create_vector(size_t count,
                                size_t blocklength,
                                size_t stride,
                                int strideinbytes, MPI_Datatype oldtype, MPII_Dataloop ** dlp_p);
int MPII_Dataloop_create_blockindexed(size_t count,
                                      size_t blklen,
                                      const void *disp_array,
                                      int dispinbytes,
                                      MPI_Datatype oldtype, MPII_Dataloop ** dlp_p);
/* we bump up the size of the blocklength array because create_struct might use
 * create_indexed in an optimization, and in course of doing so, generate a
 * request of a large blocklength. */
int MPII_Dataloop_create_indexed(size_t count,
                                 const size_t * blocklength_array,
                                 const void *displacement_array,
                                 int dispinbytes, MPI_Datatype oldtype, MPII_Dataloop ** dlp_p);
int MPII_Dataloop_create_struct(size_t count,
                                const int *blklen_array,
                                const size_t * disp_array,
                                const MPI_Datatype * oldtype_array, MPII_Dataloop ** dlp_p);

/* Helper functions for dataloop construction */
int MPII_Dataloop_convert_subarray(int ndims,
                                   int *array_of_sizes,
                                   int *array_of_subsizes,
                                   int *array_of_starts,
                                   int order, MPI_Datatype oldtype, MPI_Datatype * newtype);
int MPII_Dataloop_convert_darray(int size,
                                 int rank,
                                 int ndims,
                                 int *array_of_gsizes,
                                 int *array_of_distribs,
                                 int *array_of_dargs,
                                 int *array_of_psizes,
                                 int order, MPI_Datatype oldtype, MPI_Datatype * newtype);

size_t MPII_Dataloop_stream_size(MPII_Dataloop * dl_p, size_t(*sizefn) (MPI_Datatype el_type));

void MPII_Dataloop_alloc(int kind, size_t count, MPII_Dataloop ** new_loop_p);
void MPII_Dataloop_alloc_and_copy(int kind,
                                  size_t count,
                                  MPII_Dataloop * old_loop, MPII_Dataloop ** new_loop_p);

void MPII_Dataloop_segment_flatten(MPIR_Segment * segp,
                                   size_t first,
                                   size_t * lastp,
                                   size_t * blklens, size_t * disps, size_t * lengthp);

void MPII_Dataloop_update(MPII_Dataloop * dataloop, size_t ptrdiff);

void MPII_Segment_manipulate(MPIR_Segment * segp,
                             size_t first,
                             size_t * lastp,
                             int (*piecefn) (size_t * blocks_p,
                                             MPI_Datatype el_type,
                                             size_t rel_off,
                                             void *bufp,
                                             void *v_paramp),
                             int (*vectorfn) (size_t * blocks_p,
                                              size_t count,
                                              size_t blklen,
                                              size_t stride,
                                              MPI_Datatype el_type,
                                              size_t rel_off,
                                              void *bufp,
                                              void *v_paramp),
                             int (*blkidxfn) (size_t * blocks_p,
                                              size_t count,
                                              size_t blklen,
                                              size_t * offsetarray,
                                              MPI_Datatype el_type,
                                              size_t rel_off,
                                              void *bufp,
                                              void *v_paramp),
                             int (*indexfn) (size_t * blocks_p,
                                             size_t count,
                                             size_t * blockarray,
                                             size_t * offsetarray,
                                             MPI_Datatype el_type,
                                             size_t rel_off,
                                             void *bufp,
                                             void *v_paramp),
                             size_t(*sizefn) (MPI_Datatype el_type), void *pieceparams);

#endif /* DATALOOP_INTERNAL_H_INCLUDED */
