/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef POSIX_COLL_H_INCLUDED
#define POSIX_COLL_H_INCLUDED

#include "posix_coll_release_gather.h"


/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_POSIX_POLL_FREQUENCY
      category    : COLLECTIVE
      type        : int
      default     : 1000
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        This cvar sets the number of loops before the yield function is called.  A
        value of 0 disables yielding.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

#endif /* POSIX_COLL_H_INCLUDED */
