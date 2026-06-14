/*
 * Copyright 2026 Argonne National Laboratory
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef STUBALGO_H_INCLUDED
#define STUBALGO_H_INCLUDED

int MPII_Stubalgo_init(void);
int MPII_Stubalgo_comm_init(MPIR_Comm * comm);
int MPII_Stubalgo_comm_cleanup(MPIR_Comm * comm);

#endif /* STUBALGO_H_INCLUDED */
