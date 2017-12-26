/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2017 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

#ifndef STUBTRAN_IMPL_H_INCLUDED
#define STUBTRAN_IMPL_H_INCLUDED

int MPII_Stubtran_init(void);
int MPII_Stubtran_finalize(void);
int MPII_Stubtran_comm_init(MPIR_Comm * comm);
int MPII_Stubtran_comm_cleanup(MPIR_Comm * comm);

#endif /* STUBTRAN_IMPL_H_INCLUDED */
