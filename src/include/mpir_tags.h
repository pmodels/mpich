/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 */

#ifndef MPIR_TAGS_H_INCLUDED
#define MPIR_TAGS_H_INCLUDED

/* Tags for point to point operations which implement collective and other
   internal operations */
#define MPIR_BARRIER_TAG               1
#define MPIR_BCAST_TAG                 2
#define MPIR_GATHER_TAG                3
#define MPIR_GATHERV_TAG               4
#define MPIR_SCATTER_TAG               5
#define MPIR_SCATTERV_TAG              6
#define MPIR_ALLGATHER_TAG             7
#define MPIR_ALLGATHERV_TAG            8
#define MPIR_ALLTOALL_TAG              9
#define MPIR_ALLTOALLV_TAG            10
#define MPIR_REDUCE_TAG               11
#define MPIR_USER_REDUCE_TAG          12
#define MPIR_USER_REDUCEA_TAG         13
#define MPIR_ALLREDUCE_TAG            14
#define MPIR_USER_ALLREDUCE_TAG       15
#define MPIR_USER_ALLREDUCEA_TAG      16
#define MPIR_REDUCE_SCATTER_TAG       17
#define MPIR_USER_REDUCE_SCATTER_TAG  18
#define MPIR_USER_REDUCE_SCATTERA_TAG 19
#define MPIR_SCAN_TAG                 20
#define MPIR_USER_SCAN_TAG            21
#define MPIR_USER_SCANA_TAG           22
#define MPIR_LOCALCOPY_TAG            23
#define MPIR_EXSCAN_TAG               24
#define MPIR_ALLTOALLW_TAG            25
#define MPIR_TOPO_A_TAG               26
#define MPIR_TOPO_B_TAG               27
#define MPIR_REDUCE_SCATTER_BLOCK_TAG 28
#define MPIR_SHRINK_TAG               29
#define MPIR_AGREE_TAG                30
#define MPIR_FIRST_HCOLL_TAG          31
#define MPIR_LAST_HCOLL_TAG           (MPIR_FIRST_HCOLL_TAG + 255)
#define MPIR_FIRST_NBC_TAG            (MPIR_LAST_HCOLL_TAG + 1)

/* These macros must be used carefully. These macros will not work with
 * negative tags. By definition, users are not to use negative tags and the
 * only negative tag in MPICH is MPI_ANY_TAG which is checked seperately, but
 * if there is a time where negative tags become more common, this setup won't
 * work anymore. */

/* This bitmask can be used to manually mask the tag space wherever it might
 * be necessary to do so (for instance in the receive queue */
#ifdef HAVE_TAG_ERROR_BITS
#define MPIR_TAG_ERROR_BIT (1 << 30)
#else
#define MPIR_TAG_ERROR_BIT
#endif

/* This bitmask is used to differentiate between a process failure
 * (MPIX_ERR_PROC_FAILED) and any other kind of failure (MPI_ERR_OTHER). */
#ifdef HAVE_TAG_ERROR_BITS
#define MPIR_TAG_PROC_FAILURE_BIT (1 << 29)
#else
#define MPIR_TAG_PROC_FAILURE_BIT
#endif

/* This macro checks the value of the error bit in the MPI tag and returns 1
 * if the tag is set and 0 if it is not. */
#ifdef HAVE_TAG_ERROR_BITS
#define MPIR_TAG_CHECK_ERROR_BIT(tag) ((MPIR_TAG_ERROR_BIT & (tag)) == MPIR_TAG_ERROR_BIT ? 1 : 0)
#else
#define MPIR_TAG_CHECK_ERROR_BIT(tag) 0
#endif

/* This macro checks the value of the process failure bit in the MPI tag and
 * returns 1 if the tag is set and 0 if it is not. */
#ifdef HAVE_TAG_ERROR_BITS
#define MPIR_TAG_CHECK_PROC_FAILURE_BIT(tag) ((MPIR_TAG_PROC_FAILURE_BIT & (tag)) == MPIR_TAG_PROC_FAILURE_BIT ? 1 : 0)
#else
#define MPIR_TAG_CHECK_PROC_FAILURE_BIT(tag) 0
#endif

/* This macro sets the value of the error bit in the MPI tag to 1 */
#ifdef HAVE_TAG_ERROR_BITS
#define MPIR_TAG_SET_ERROR_BIT(tag) ((tag) |= MPIR_TAG_ERROR_BIT)
#else
#define MPIR_TAG_SET_ERROR_BIT(tag) (tag)
#endif

/* This macro sets the value of the process failure bit in the MPI tag to 1 */
#ifdef HAVE_TAG_ERROR_BITS
#define MPIR_TAG_SET_PROC_FAILURE_BIT(tag) ((tag) |= (MPIR_TAG_ERROR_BIT | MPIR_TAG_PROC_FAILURE_BIT))
#else
#define MPIR_TAG_SET_PROC_FAILURE_BIT(tag) (tag)
#endif

/* This macro clears the value of the error bits in the MPI tag */
#ifdef HAVE_TAG_ERROR_BITS
#define MPIR_TAG_CLEAR_ERROR_BITS(tag) ((tag) &= ~(MPIR_TAG_ERROR_BIT ^ MPIR_TAG_PROC_FAILURE_BIT))
#else
#define MPIR_TAG_CLEAR_ERROR_BITS(tag) (tag)
#endif

/* This macro masks the value of the error bits in the MPI tag */
#ifdef HAVE_TAG_ERROR_BITS
#define MPIR_TAG_MASK_ERROR_BITS(tag) ((tag) & ~(MPIR_TAG_ERROR_BIT ^ MPIR_TAG_PROC_FAILURE_BIT))
#else
#define MPIR_TAG_MASK_ERROR_BITS(tag) (tag)
#endif

#endif /* MPIR_TAGS_H_INCLUDED */
