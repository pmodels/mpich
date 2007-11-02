/*
 * Globus device code:          Copyright 2005 Northern Illinois University
 * Borrowed MPICH-G2 code:      Copyright 2000 Argonne National Laboratory and Northern Illinois University
 * Borrowed MPICH2 device code: Copyright 2001 Argonne National Laboratory
 *
 * XXX: INSERT POINTER TO OFFICIAL COPYRIGHT TEXT
 */

#if !defined(MPICH2_MPIG_CM_OTHER_H_INCLUDED)
#define MPICH2_MPIG_CM_OTHER_H_INCLUDED

/*
 * expose the communication module's vtable so that it is accessible to other modules in the device
 */
extern struct mpig_cm mpig_cm_other;


/*
 * special case VC for handling things like MPI_ANY_SOURCE and MPI_PROC_NULL
 */
extern struct mpig_vc * mpig_cm_other_vc;

#endif /* !defined(MPICH2_MPIG_CM_OTHER_H_INCLUDED) */
