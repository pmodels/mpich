/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* Definitions private to the datatype code */
extern int MPIR_Datatype_init( void );
extern int MPIR_Datatype_builtin_fillin( void );
extern int MPIR_Datatype_init_names( void );
extern void MPIR_Datatype_iscontig( MPI_Datatype , int * );

/* LB/UB calculation helper macros */

/* MPII_DATATYPE_CONTIG_LB_UB()
 *
 * Determines the new LB and UB for a block of old types given the
 * old type's LB, UB, and extent, and a count of these types in the
 * block.
 *
 * Note: if the displacement is non-zero, the MPII_DATATYPE_BLOCK_LB_UB()
 * should be used instead (see below).
 */
#define MPII_DATATYPE_CONTIG_LB_UB(cnt_,		\
				   old_lb_,		\
				   old_ub_,		\
				   old_extent_,	\
				   lb_,		\
				   ub_)		\
do {							\
    if (cnt_ == 0) {					\
	lb_ = old_lb_;				\
	ub_ = old_ub_;				\
    }							\
    else if (old_ub_ >= old_lb_) {			\
        lb_ = old_lb_;				\
        ub_ = old_ub_ + (old_extent_) * (cnt_ - 1);	\
    }							\
    else /* negative extent */ {			\
	lb_ = old_lb_ + (old_extent_) * (cnt_ - 1);	\
	ub_ = old_ub_;				\
    }                                                   \
} while(0)

/* MPII_DATATYPE_VECTOR_LB_UB()
 *
 * Determines the new LB and UB for a vector of blocks of old types
 * given the old type's LB, UB, and extent, and a count, stride, and
 * blocklen describing the vectorization.
 */
#define MPII_DATATYPE_VECTOR_LB_UB(cnt_,			\
				   stride_,			\
				   blklen_,			\
				   old_lb_,			\
				   old_ub_,			\
				   old_extent_,		\
				   lb_,			\
				   ub_)			\
do {								\
    if (cnt_ == 0 || blklen_ == 0) {				\
	lb_ = old_lb_;					\
	ub_ = old_ub_;					\
    }								\
    else if (stride_ >= 0 && (old_extent_) >= 0) {		\
	lb_ = old_lb_;					\
	ub_ = old_ub_ + (old_extent_) * ((blklen_) - 1) +	\
	    (stride_) * ((cnt_) - 1);				\
    }								\
    else if (stride_ < 0 && (old_extent_) >= 0) {		\
	lb_ = old_lb_ + (stride_) * ((cnt_) - 1);		\
	ub_ = old_ub_ + (old_extent_) * ((blklen_) - 1);	\
    }								\
    else if (stride_ >= 0 && (old_extent_) < 0) {		\
	lb_ = old_lb_ + (old_extent_) * ((blklen_) - 1);	\
	ub_ = old_ub_ + (stride_) * ((cnt_) - 1);		\
    }								\
    else {							\
	lb_ = old_lb_ + (old_extent_) * ((blklen_) - 1) +	\
	    (stride_) * ((cnt_) - 1);				\
	ub_ = old_ub_;					\
    }								\
} while(0)

/* MPII_DATATYPE_BLOCK_LB_UB()
 *
 * Determines the new LB and UB for a block of old types given the LB,
 * UB, and extent of the old type as well as a new displacement and count
 * of types.
 *
 * Note: we need the extent here in addition to the lb and ub because the
 * extent might have some padding in it that we need to take into account.
 */
#define MPII_DATATYPE_BLOCK_LB_UB(cnt_,				\
				  disp_,				\
				  old_lb_,				\
				  old_ub_,				\
				  old_extent_,				\
				  lb_,					\
				  ub_)					\
do {									\
    if (cnt_ == 0) {							\
	lb_ = old_lb_ + (disp_);					\
	ub_ = old_ub_ + (disp_);					\
    }									\
    else if (old_ub_ >= old_lb_) {					\
        lb_ = old_lb_ + (disp_);					\
        ub_ = old_ub_ + (disp_) + (old_extent_) * ((cnt_) - 1);	\
    }									\
    else /* negative extent */ {					\
	lb_ = old_lb_ + (disp_) + (old_extent_) * ((cnt_) - 1);	\
	ub_ = old_ub_ + (disp_);					\
    }									\
} while(0)

/* internal debugging functions */
void MPII_Datatype_printf(MPI_Datatype type, int depth, MPI_Aint displacement, int blocklength, int header);
void MPII_Dataloop_dot_printf(MPIR_Dataloop *loop_p, int depth, int header);

void MPII_Datatype_get_contents_ints(MPIR_Datatype_contents *cp, int *user_ints);
void MPII_Datatype_get_contents_aints(MPIR_Datatype_contents *cp, MPI_Aint *user_aints);
void MPII_Datatype_get_contents_types(MPIR_Datatype_contents *cp, MPI_Datatype *user_types);
