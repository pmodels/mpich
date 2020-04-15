/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
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
