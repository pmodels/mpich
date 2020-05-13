/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef STUBTRAN_IMPL_H_INCLUDED
#define STUBTRAN_IMPL_H_INCLUDED

int MPII_Stubtran_init(void);
int MPII_Stubtran_finalize(void);
int MPII_Stubtran_comm_init(MPIR_Comm * comm);
int MPII_Stubtran_comm_cleanup(MPIR_Comm * comm);

#endif /* STUBTRAN_IMPL_H_INCLUDED */
