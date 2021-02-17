/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPICOMM_H_INCLUDED
#define MPICOMM_H_INCLUDED

/* Function prototypes for communicator helper functions */
int MPIR_Get_intercomm_contextid(MPIR_Comm *, MPIR_Context_id_t *, MPIR_Context_id_t *);

int MPII_compare_info_hint(const char *hint_str, MPIR_Comm * comm_ptr, int *info_args_are_equal);

#endif /* MPICOMM_H_INCLUDED */
