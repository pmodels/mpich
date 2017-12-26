/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2017 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

#ifndef STUBALGO_H_INCLUDED
#define STUBALGO_H_INCLUDED

int MPII_Stubalgo_init(void);
int MPII_Stubalgo_comm_init(MPIR_Comm * comm);
int MPII_Stubalgo_comm_cleanup(MPIR_Comm * comm);

#endif /* STUBALGO_H_INCLUDED */
