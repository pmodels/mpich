/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef GROUP_H_INCLUDED
#define GROUP_H_INCLUDED

/* MPIR_Group_create is needed by some of the routines that return groups
   from communicators, so it is in mpidimpl.h */
void MPII_Group_setup_lpid_list(MPIR_Group *);
int MPIR_Group_check_valid_ranks(MPIR_Group *, const int[], int);
int MPIR_Group_check_valid_ranges(MPIR_Group *, int[][3], int);
void MPIR_Group_setup_lpid_pairs(MPIR_Group *, MPIR_Group *);

#endif /* GROUP_H_INCLUDED */
