/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef DATALOOP_INTERNAL_H_INCLUDED
#define DATALOOP_INTERNAL_H_INCLUDED

#include <mpiimpl.h>
#include "dataloop.h"

#define MPII_DATALOOP_HANDLE_HASLOOP(handle_)                           \
    ((HANDLE_IS_BUILTIN(handle_)) ? 0 : 1)

#define MPII_DATALOOP_FINAL_MASK  0x00000008
#define MPII_DATALOOP_KIND_MASK   0x00000007
#define MPII_DATALOOP_KIND_CONTIG 0x1
#define MPII_DATALOOP_KIND_VECTOR 0x2
#define MPII_DATALOOP_KIND_BLOCKINDEXED 0x3
#define MPII_DATALOOP_KIND_INDEXED 0x4
#define MPII_DATALOOP_KIND_STRUCT 0x5

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
        MPI_Aint count;
        struct {
            MPI_Aint count;
            struct MPII_Dataloop *dataloop;
        } c_t;
        struct {
            MPI_Aint count;
            struct MPII_Dataloop *dataloop;
            MPI_Aint blocksize;
            MPI_Aint stride;
        } v_t;
        struct {
            MPI_Aint count;
            struct MPII_Dataloop *dataloop;
            MPI_Aint blocksize;
            MPI_Aint *offset_array;
        } bi_t;
        struct {
            MPI_Aint count;
            struct MPII_Dataloop *dataloop;
            MPI_Aint *blocksize_array;
            MPI_Aint *offset_array;
            MPI_Aint total_blocks;
        } i_t;
        struct {
            MPI_Aint count;
            struct MPII_Dataloop **dataloop_array;
            MPI_Aint *blocksize_array;
            MPI_Aint *offset_array;
            MPI_Aint *el_extent_array;  /* need more than one */
        } s_t;
        struct {
            MPI_Aint count;
            struct MPII_Dataloop *dataloop;
        } cm_t;
    } loop_params;
    MPI_Aint el_size;
    MPI_Aint el_extent;
    MPI_Datatype el_type;
    int is_contig;
    MPI_Aint num_contig;

    MPI_Aint dloop_sz;
} MPII_Dataloop;

static int MPII_Type_dloop_size(MPI_Datatype type) ATTRIBUTE((unused));
static int MPII_Type_dloop_size(MPI_Datatype type)
{
    MPII_Dataloop *dloop;

    MPIR_DATALOOP_GET_LOOPPTR(type, dloop);
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

MPI_Aint MPII_Dataloop_stackelm_blocksize(struct MPII_Dataloop_stackelm *elmp);
MPI_Aint MPII_Dataloop_stackelm_offset(struct MPII_Dataloop_stackelm *elmp);
void MPII_Dataloop_stackelm_load(struct MPII_Dataloop_stackelm *elmp,
                                 MPII_Dataloop * dlp, int branch_flag);

MPI_Aint MPII_Dataloop_stream_size(MPII_Dataloop * dl_p, MPI_Aint(*sizefn) (MPI_Datatype el_type));

void MPII_Dataloop_alloc(int kind, MPI_Aint count, MPII_Dataloop ** new_loop_p);
void MPII_Dataloop_alloc_and_copy(int kind,
                                  MPI_Aint count,
                                  MPII_Dataloop * old_loop, MPII_Dataloop ** new_loop_p);

void MPII_Dataloop_segment_flatten(MPIR_Segment * segp,
                                   MPI_Aint first,
                                   MPI_Aint * lastp,
                                   MPI_Aint * blklens, MPI_Aint * disps, MPI_Aint * lengthp);

void MPII_Dataloop_update(MPII_Dataloop * dataloop, MPI_Aint ptrdiff);

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

#endif /* DATALOOP_INTERNAL_H_INCLUDED */
