/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 */

#ifndef MPIR_CONTEXTID_H_INCLUDED
#define MPIR_CONTEXTID_H_INCLUDED

#define MPIR_CONTEXT_ID_T_DATATYPE MPI_UINT16_T
typedef uint16_t MPIR_Context_id_t;
#define MPIR_INVALID_CONTEXT_ID ((MPIR_Context_id_t)0xffff)

/* The following preprocessor macros provide bitfield access information for
 * context ID values.  They follow a uniform naming pattern:
 *
 * MPIR_CONTEXT_foo_WIDTH - the width in bits of the field
 * MPIR_CONTEXT_foo_MASK  - A valid bit mask for bit-wise AND and OR operations
 *                          with exactly all of the bits in the field set.
 * MPIR_CONTEXT_foo_SHIFT - The number of bits that the field should be shifted
 *                          rightwards to place it in the least significant bits
 *                          of the ID.  There may still be higher order bits
 *                          from other fields, so the _MASK should be used first
 *                          if you want to reliably retrieve the exact value of
 *                          the field.
 */

/* yields an rvalue that is the value of the field_name_ in the least significant bits */
#define MPIR_CONTEXT_READ_FIELD(field_name_,id_) \
    (((id_) & MPIR_CONTEXT_##field_name_##_MASK) >> MPIR_CONTEXT_##field_name_##_SHIFT)
/* yields an rvalue that is the old_id_ with field_name_ set to field_val_ */
#define MPIR_CONTEXT_SET_FIELD(field_name_,old_id_,field_val_) \
    ((old_id_ & ~MPIR_CONTEXT_##field_name_##_MASK) | ((field_val_) << MPIR_CONTEXT_##field_name_##_SHIFT))

/* Context suffixes for separating pt2pt and collective communication */
#define MPIR_CONTEXT_SUFFIX_WIDTH (1)
#define MPIR_CONTEXT_SUFFIX_SHIFT (0)
#define MPIR_CONTEXT_SUFFIX_MASK ((1 << MPIR_CONTEXT_SUFFIX_WIDTH) - 1)
#define MPIR_CONTEXT_INTRA_PT2PT (0)
#define MPIR_CONTEXT_INTRA_COLL  (1)
#define MPIR_CONTEXT_INTER_PT2PT (0)
#define MPIR_CONTEXT_INTER_COLL  (1)

/* Used to derive context IDs for sub-communicators from a parent communicator's
   context ID value.  This field comes after the one bit suffix.
   values are shifted left by 1. */
#define MPIR_CONTEXT_SUBCOMM_WIDTH (2)
#define MPIR_CONTEXT_SUBCOMM_SHIFT (MPIR_CONTEXT_SUFFIX_WIDTH + MPIR_CONTEXT_SUFFIX_SHIFT)
#define MPIR_CONTEXT_SUBCOMM_MASK      (((1 << MPIR_CONTEXT_SUBCOMM_WIDTH) - 1) << MPIR_CONTEXT_SUBCOMM_SHIFT)

/* these values may be added/subtracted directly to/from an existing context ID
 * in order to determine the context ID of the child/parent */
#define MPIR_CONTEXT_PARENT_OFFSET    (0 << MPIR_CONTEXT_SUBCOMM_SHIFT)
#define MPIR_CONTEXT_INTRANODE_OFFSET (1 << MPIR_CONTEXT_SUBCOMM_SHIFT)
#define MPIR_CONTEXT_INTERNODE_OFFSET (2 << MPIR_CONTEXT_SUBCOMM_SHIFT)

/* this field (IS_LOCALCOM) is used to derive a context ID for local
 * communicators of intercommunicators without communication */
#define MPIR_CONTEXT_IS_LOCALCOMM_WIDTH (1)
#define MPIR_CONTEXT_IS_LOCALCOMM_SHIFT (MPIR_CONTEXT_SUBCOMM_SHIFT + MPIR_CONTEXT_SUBCOMM_WIDTH)
#define MPIR_CONTEXT_IS_LOCALCOMM_MASK (((1 << MPIR_CONTEXT_IS_LOCALCOMM_WIDTH) - 1) << MPIR_CONTEXT_IS_LOCALCOMM_SHIFT)

/* MPIR_MAX_CONTEXT_MASK is the number of ints that make up the bit vector that
 * describes the context ID prefix space.
 *
 * The following must hold:
 * (num_bits_in_vector) <= (maximum_context_id_prefix)
 *   which is the following in concrete terms:
 * MPIR_MAX_CONTEXT_MASK*MPIR_CONTEXT_INT_BITS <= 2**(MPIR_CONTEXT_ID_BITS - (MPIR_CONTEXT_PREFIX_SHIFT + MPIR_CONTEXT_DYNAMIC_PROC_WIDTH))
 *
 * We currently always assume MPIR_CONTEXT_INT_BITS is 32, regardless of the
 * value of sizeof(int)*CHAR_BITS.  We also make the assumption that CHAR_BITS==8.
 *
 * For a 16-bit context id field and CHAR_BITS==8, this implies MPIR_MAX_CONTEXT_MASK <= 256
 */

/* number of bits to shift right by in order to obtain the context ID prefix */
#define MPIR_CONTEXT_PREFIX_SHIFT (MPIR_CONTEXT_IS_LOCALCOMM_SHIFT + MPIR_CONTEXT_IS_LOCALCOMM_WIDTH)
#define MPIR_CONTEXT_PREFIX_WIDTH (MPIR_CONTEXT_ID_BITS - (MPIR_CONTEXT_PREFIX_SHIFT + MPIR_CONTEXT_DYNAMIC_PROC_WIDTH))
#define MPIR_CONTEXT_PREFIX_MASK (((1 << MPIR_CONTEXT_PREFIX_WIDTH) - 1) << MPIR_CONTEXT_PREFIX_SHIFT)

#define MPIR_CONTEXT_DYNAMIC_PROC_WIDTH (1)     /* the upper half is reserved for dynamic procs */
#define MPIR_CONTEXT_DYNAMIC_PROC_SHIFT (MPIR_CONTEXT_ID_BITS - MPIR_CONTEXT_DYNAMIC_PROC_WIDTH)        /* the upper half is reserved for dynamic procs */
#define MPIR_CONTEXT_DYNAMIC_PROC_MASK (((1 << MPIR_CONTEXT_DYNAMIC_PROC_WIDTH) - 1) << MPIR_CONTEXT_DYNAMIC_PROC_SHIFT)

/* should probably be (sizeof(int)*CHAR_BITS) once we make the code CHAR_BITS-clean */
#define MPIR_CONTEXT_INT_BITS (32)
#define MPIR_CONTEXT_ID_BITS (sizeof(MPIR_Context_id_t)*8)      /* 8 --> CHAR_BITS eventually */
#define MPIR_MAX_CONTEXT_MASK \
    ((1 << (MPIR_CONTEXT_ID_BITS - (MPIR_CONTEXT_PREFIX_SHIFT + MPIR_CONTEXT_DYNAMIC_PROC_WIDTH))) / MPIR_CONTEXT_INT_BITS)

/* Utility routines.  Where possible, these are kept in the source directory
   with the other comm routines (src/mpi/comm, in mpicomm.h).  However,
   to create a new communicator after a spawn or connect-accept operation,
   the device may need to create a new contextid */
int MPIR_Get_contextid_sparse(MPIR_Comm * comm_ptr, MPIR_Context_id_t * context_id, int ignore_id);
int MPIR_Get_contextid_sparse_group(MPIR_Comm * comm_ptr, MPIR_Group * group_ptr, int tag,
                                    MPIR_Context_id_t * context_id, int ignore_id);

int MPIR_Get_contextid_nonblock(MPIR_Comm * comm_ptr, MPIR_Comm * newcommp, MPIR_Request ** req);
int MPIR_Get_intercomm_contextid_nonblock(MPIR_Comm * comm_ptr, MPIR_Comm * newcommp,
                                          MPIR_Request ** req);

void MPIR_Free_contextid(MPIR_Context_id_t);

#endif /* MPIR_CONTEXTID_H_INCLUDED */
