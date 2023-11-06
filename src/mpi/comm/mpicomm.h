/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPICOMM_H_INCLUDED
#define MPICOMM_H_INCLUDED

/* Function prototypes for communicator helper functions */
int MPIR_Get_intercomm_contextid(MPIR_Comm *, MPIR_Context_id_t *, MPIR_Context_id_t *);

/* Utitlity function that retrieves an info key and ensures it is collectively equal */
int MPII_collect_info_key(MPIR_Comm * comm_ptr, MPIR_Info * info_ptr, const char *key,
                          const char **value_ptr);

#endif /* MPICOMM_H_INCLUDED */
