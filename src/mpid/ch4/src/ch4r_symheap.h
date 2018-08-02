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

#include <mpichconf.h>

#include <opa_primitives.h>
#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif /* HAVE_SYS_MMAN_H */
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif /* HAVE_SYS_TIME_H */
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif /* HAVE_SYS_STAT_H */
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif /* HAVE_FCNTL_H */
#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif /* HAVE_STDINT_H */

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

    - name        : MPIR_CVAR_CH4_SYMHEAP_RETRY
      category    : CH4
      type        : int
      default     : 100
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_GROUP
      description : >-
        The default number of retries for allocating a symmetric heap in a process
        group. A retrying involves collective communication over the group.

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
int MPIDIU_get_symmetric_heap(MPI_Aint size, MPIR_Comm * comm, void **base, MPIR_Win * win);

#endif /* CH4R_SYMHEAP_H_INCLUDED */
