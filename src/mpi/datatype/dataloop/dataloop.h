/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */

/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef DATALOOP_H_INCLUDED
#define DATALOOP_H_INCLUDED

#include <mpiimpl.h>

#define MPII_DATALOOP_HANDLE_HASLOOP(handle_)                           \
    ((HANDLE_GET_KIND(handle_) == HANDLE_KIND_BUILTIN) ? 0 : 1)

#define MPII_DATALOOP_FINAL_MASK  0x00000008
#define MPII_DATALOOP_KIND_MASK   0x00000007
#define MPII_DATALOOP_KIND_CONTIG 0x1
#define MPII_DATALOOP_KIND_VECTOR 0x2
#define MPII_DATALOOP_KIND_BLOCKINDEXED 0x3
#define MPII_DATALOOP_KIND_INDEXED 0x4
#define MPII_DATALOOP_KIND_STRUCT 0x5

#define GET_FIELD(value_,fieldname_) \
      value_ = ((MPIR_Datatype *)ptr)->dataloop##fieldname_

#define SET_FIELD(value_,fieldname_) \
    ((MPIR_Datatype *)ptr)->dataloop##fieldname_ = value_

#define MPII_DATALOOP_GET_LOOPPTR(a,lptr_) do {         \
    void *ptr;                                    \
    switch (HANDLE_GET_KIND(a)) {                      \
        case HANDLE_KIND_DIRECT:                       \
            MPIR_Assert(HANDLE_INDEX(a) < MPIR_DATATYPE_PREALLOC);  \
            ptr = MPIR_Datatype_direct+HANDLE_INDEX(a);               \
            GET_FIELD(lptr_,);                             \
            break;                                \
        case HANDLE_KIND_INDIRECT:                     \
            ptr = ((MPIR_Datatype *)                        \
             MPIR_Handle_get_ptr_indirect(a,&MPIR_Datatype_mem));     \
            GET_FIELD(lptr_,);                             \
            break;                                \
        case HANDLE_KIND_INVALID:                      \
        case HANDLE_KIND_BUILTIN:                      \
        default:                                  \
            lptr_ = 0;                                 \
            break;                                \
    }                                             \
} while (0)

#define MPII_DATALOOP_GET_LOOPSIZE(a,depth_) do {             \
    void *ptr;                                                      \
    switch (HANDLE_GET_KIND(a)) {                                   \
        case HANDLE_KIND_DIRECT:                                    \
            MPIR_Assert(HANDLE_INDEX(a) < MPIR_DATATYPE_PREALLOC) ; \
            ptr = MPIR_Datatype_direct+HANDLE_INDEX(a);             \
            GET_FIELD(depth_,_size);                  \
            break;                                                  \
        case HANDLE_KIND_INDIRECT:                                  \
            ptr = ((MPIR_Datatype *)                                \
             MPIR_Handle_get_ptr_indirect(a,&MPIR_Datatype_mem));   \
            GET_FIELD(depth_,_size);                                \
            break;                                                  \
        case HANDLE_KIND_INVALID:                                   \
        case HANDLE_KIND_BUILTIN:                                   \
        default:                                                    \
            depth_ = 0;                                             \
            break;                                                  \
    }                                                               \
} while (0)

#define MPII_DATALOOP_SET_LOOPPTR(a,lptr_) do {               \
    void *ptr;                                                      \
    switch (HANDLE_GET_KIND(a)) {                                   \
        case HANDLE_KIND_DIRECT:                                    \
            MPIR_Assert(HANDLE_INDEX(a) < MPIR_DATATYPE_PREALLOC);  \
            ptr = MPIR_Datatype_direct+HANDLE_INDEX(a);             \
            SET_FIELD(lptr_,);                        \
            break;                                                  \
        case HANDLE_KIND_INDIRECT:                                  \
            ptr = ((MPIR_Datatype *)                                \
             MPIR_Handle_get_ptr_indirect(a,&MPIR_Datatype_mem));   \
            SET_FIELD(lptr_,);                        \
            break;                                                  \
        case HANDLE_KIND_INVALID:                                   \
        case HANDLE_KIND_BUILTIN:                                   \
        default:                                                    \
            lptr_ = 0;                                              \
            break;                                                  \
    }                                                               \
} while (0)

#define MPII_DATALOOP_SET_LOOPSIZE(a,depth_) do {             \
    void *ptr;                                                      \
    switch (HANDLE_GET_KIND(a)) {                                   \
        case HANDLE_KIND_DIRECT:                                    \
            MPIR_Assert(HANDLE_INDEX(a) < MPIR_DATATYPE_PREALLOC);  \
            ptr = MPIR_Datatype_direct+HANDLE_INDEX(a);             \
            SET_FIELD(depth_,_size);                  \
            break;                                                  \
        case HANDLE_KIND_INDIRECT:                                  \
            ptr = ((MPIR_Datatype *)                                \
             MPIR_Handle_get_ptr_indirect(a,&MPIR_Datatype_mem));   \
            SET_FIELD(depth_,_size);                  \
            break;                                                  \
        case HANDLE_KIND_INVALID:                                   \
        case HANDLE_KIND_BUILTIN:                                   \
        default:                                                    \
            depth_ = 0;                                             \
            break;                                                  \
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

    MPI_Aint curcount;
    MPI_Aint curoffset;
    MPI_Aint curblock;

    MPI_Aint orig_count;
    MPI_Aint orig_offset;
    MPI_Aint orig_block;

    struct MPIR_Dataloop *loop_p;
} MPII_Dataloop_stackelm;

/*S
  MPIR_Dataloop - Description of the structure used to hold a dataloop
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
struct MPIR_Dataloop {
    int kind;                   /* Contains both the loop type
                                 * (contig, vector, blockindexed, indexed,
                                 * or struct) and a bit that indicates
                                 * whether the dataloop is a leaf type. */
    union {
        MPI_Aint count;
        struct {
            MPI_Aint count;
            struct MPIR_Dataloop *dataloop;
        } c_t;
        struct {
            MPI_Aint count;
            struct MPIR_Dataloop *dataloop;
            MPI_Aint blocksize;
            MPI_Aint stride;
        } v_t;
        struct {
            MPI_Aint count;
            struct MPIR_Dataloop *dataloop;
            MPI_Aint blocksize;
            MPI_Aint *offset_array;
        } bi_t;
        struct {
            MPI_Aint count;
            struct MPIR_Dataloop *dataloop;
            MPI_Aint *blocksize_array;
            MPI_Aint *offset_array;
            MPI_Aint total_blocks;
        } i_t;
        struct {
            MPI_Aint count;
            struct MPIR_Dataloop **dataloop_array;
            MPI_Aint *blocksize_array;
            MPI_Aint *offset_array;
            MPI_Aint *el_extent_array;  /* need more than one */
        } s_t;
        struct {
            MPI_Aint count;
            struct MPIR_Dataloop *dataloop;
        } cm_t;
    } loop_params;
    MPI_Aint el_size;
    MPI_Aint el_extent;
    MPI_Datatype el_type;
};

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
    MPI_Aint stream_off;        /* next offset into data stream resulting from datatype
                                 * processing.  in other words, how many bytes have
                                 * we created/used by parsing so far?  that amount + 1.
                                 */
    MPII_Dataloop_stackelm stackelm[MPII_DATALOOP_MAX_DATATYPE_DEPTH];
    int cur_sp;                 /* Current stack pointer when using dataloop */
    int valid_sp;               /* maximum valid stack pointer.  This is used to
                                 * maintain information on the stack after it has
                                 * been placed there by following the datatype field
                                 * for any type except struct */
    struct MPIR_Dataloop builtin_loop;  /* used for both predefined types (which
                                         * won't have a loop already) and for
                                         * situations where a count is passed in
                                         * and we need to create a contig loop
                                         * to handle it
                                         */
};

struct MPII_Dataloop_m2m_params {
    int direction;              /* M2M_TO_USERBUF or M2M_FROM_USERBUF */
    char *streambuf;
    char *userbuf;
};

MPI_Aint MPII_Dataloop_stackelm_blocksize(struct MPII_Dataloop_stackelm *elmp);
MPI_Aint MPII_Dataloop_stackelm_offset(struct MPII_Dataloop_stackelm *elmp);
void MPII_Dataloop_stackelm_load(struct MPII_Dataloop_stackelm *elmp,
                                 struct MPIR_Dataloop *dlp, int branch_flag);

int MPII_Dataloop_create_contiguous(MPI_Aint count,
                                    MPI_Datatype oldtype,
                                    MPIR_Dataloop ** dlp_p, MPI_Aint * dlsz_p);
int MPII_Dataloop_create_vector(MPI_Aint count,
                                MPI_Aint blocklength,
                                MPI_Aint stride,
                                int strideinbytes,
                                MPI_Datatype oldtype, MPIR_Dataloop ** dlp_p, MPI_Aint * dlsz_p);
int MPII_Dataloop_create_blockindexed(MPI_Aint count,
                                      MPI_Aint blklen,
                                      const void *disp_array,
                                      int dispinbytes,
                                      MPI_Datatype oldtype,
                                      MPIR_Dataloop ** dlp_p, MPI_Aint * dlsz_p);
/* we bump up the size of the blocklength array because create_struct might use
 * create_indexed in an optimization, and in course of doing so, generate a
 * request of a large blocklength. */
int MPII_Dataloop_create_indexed(MPI_Aint count,
                                 const MPI_Aint * blocklength_array,
                                 const void *displacement_array,
                                 int dispinbytes,
                                 MPI_Datatype oldtype, MPIR_Dataloop ** dlp_p, MPI_Aint * dlsz_p);
int MPII_Dataloop_create_struct(MPI_Aint count,
                                const int *blklen_array,
                                const MPI_Aint * disp_array,
                                const MPI_Datatype * oldtype_array,
                                MPIR_Dataloop ** dlp_p, MPI_Aint * dlsz_p);

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

MPI_Aint MPII_Dataloop_stream_size(MPIR_Dataloop * dl_p, MPI_Aint(*sizefn) (MPI_Datatype el_type));

void MPII_Dataloop_alloc(int kind,
                         MPI_Aint count, MPIR_Dataloop ** new_loop_p, MPI_Aint * new_loop_sz_p);
void MPII_Dataloop_alloc_and_copy(int kind,
                                  MPI_Aint count,
                                  MPIR_Dataloop * old_loop,
                                  MPI_Aint old_loop_sz,
                                  MPIR_Dataloop ** new_loop_p, MPI_Aint * new_loop_sz_p);

void MPII_Dataloop_segment_flatten(MPIR_Segment * segp,
                                   MPI_Aint first,
                                   MPI_Aint * lastp,
                                   MPI_Aint * blklens, MPI_Aint * disps, MPI_Aint * lengthp);

void MPII_Dataloop_update(MPIR_Dataloop * dataloop, MPI_Aint ptrdiff);

void MPII_Segment_manipulate(MPIR_Segment * segp,
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

#endif /* DATALOOP_H_INCLUDED */
