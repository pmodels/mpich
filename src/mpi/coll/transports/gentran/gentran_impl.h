/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2017 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

#ifndef GENTRAN_IMPL_H_INCLUDED
#define GENTRAN_IMPL_H_INCLUDED

/* transport initialization */
int MPII_Gentran_init(void);

/* transport cleanup */
int MPII_Gentran_finalize(void);

/* communicator-specific initializtion */
int MPII_Gentran_comm_init(MPIR_Comm * comm_ptr);

/* communicator-specific cleanup */
int MPII_Gentran_comm_cleanup(MPIR_Comm * comm_ptr);

/* check if there are any pending schedules */
int MPII_Gentran_scheds_are_pending(void);

#endif /* GENTRAN_IMPL_H_INCLUDED */
