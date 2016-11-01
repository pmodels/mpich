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
#ifndef SHM_SHMAM_IMPL_H_INCLUDED
#define SHM_SHMAM_IMPL_H_INCLUDED

#include <mpidimpl.h>
#include "mpidch4r.h"

#include "shmam_types.h"
#include "shmam_datatypes.h"
#include "shmam_defs.h"


#define DECL_FUNC(FUNCNAME)  MPL_QUOTE(FUNCNAME)

#undef FUNCNAME
#define FUNCNAME nothing
#define BEGIN_FUNC(FUNCNAME)                    \
  MPIR_FUNC_VERBOSE_STATE_DECL(FUNCNAME);                   \
  MPIR_FUNC_VERBOSE_ENTER(FUNCNAME);
#define END_FUNC(FUNCNAME)                      \
  MPIR_FUNC_VERBOSE_EXIT(FUNCNAME);
#define END_FUNC_RC(FUNCNAME) \
  fn_exit:                    \
  MPIR_FUNC_VERBOSE_EXIT(FUNCNAME);  \
  return mpi_errno;           \
fn_fail:                      \
  goto fn_exit;

#define __SHORT_FILE__                          \
  (strrchr(__FILE__,'/')                        \
   ? strrchr(__FILE__,'/')+1                    \
   : __FILE__                                   \
)

#endif /* SHM_SHMAM_IMPL_H_INCLUDED */
