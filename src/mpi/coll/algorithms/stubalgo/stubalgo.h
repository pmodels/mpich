/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef STUBALGO_H_INCLUDED
#define STUBALGO_H_INCLUDED

int MPII_Stubalgo_init(void);
int MPII_Stubalgo_comm_init(MPIR_Comm * comm);
int MPII_Stubalgo_comm_cleanup(MPIR_Comm * comm);

#endif /* STUBALGO_H_INCLUDED */
