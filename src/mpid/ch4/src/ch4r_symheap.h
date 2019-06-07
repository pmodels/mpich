/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2016 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#ifndef CH4R_SYMHEAP_H_INCLUDED
#define CH4R_SYMHEAP_H_INCLUDED

#include <stddef.h>

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_CH4_RANDOM_ADDR_RETRY
      category    : CH4
      type        : int
      default     : 100
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_GROUP
      description : >-
        The default number of retries for generating a random address. A retrying
        involves only local operations.

    - name        : MPIR_CVAR_CH4_SHM_SYMHEAP_RETRY
      category    : CH4
      type        : int
      default     : 100
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_GROUP
      description : >-
        The default number of retries for allocating a symmetric heap in shared
        memory. A retrying involves collective communication over the group in
        the shared memory.
=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

size_t MPIDIU_get_mapsize(size_t size, size_t * psz);
int MPIDIU_get_shm_symheap(MPI_Aint shm_size, MPI_Aint * shm_offsets, MPIR_Comm * comm,
                           MPIR_Win * win, bool * fail_flag);

#endif /* CH4R_SYMHEAP_H_INCLUDED */
