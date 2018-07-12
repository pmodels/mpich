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
#ifndef CH4_SPAWN_H_INCLUDED
#define CH4_SPAWN_H_INCLUDED

#include "ch4_impl.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_CH4_COMM_CONNECT_TIMEOUT
      category    : CH4
      type        : int
      default     : 180
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_GROUP_EQ
      description : >-
        The default time out period in seconds for a connection attempt to the
        server communicator where the named port exists but no pending accept.
        User can change the value for a specified connection through its info
        argument.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

#endif /* CH4_SPAWN_H_INCLUDED */
