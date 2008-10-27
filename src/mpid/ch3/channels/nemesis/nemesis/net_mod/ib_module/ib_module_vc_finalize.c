/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#define _GNU_SOURCE
#include "mpidimpl.h"
#include "ib_module_impl.h"
#include "ib_device.h"
#include "ib_module_cm.h"
#include "ib_utils.h"

#undef FUNCNAME
#define FUNCNAME MPID_nem_ib_module_vc_finalize
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)

int MPID_nem_ib_module_vc_terminate (MPIDI_VC_t *vc)
{
    int mpi_errno = MPI_SUCCESS;

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}
