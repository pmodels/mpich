/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "ofi_impl.h"
#include "ofi_noinline.h"

int MPIDI_OFI_mpi_type_free_hook(MPIR_Datatype * datatype_p)
{
    return 0;
}

int MPIDI_OFI_mpi_type_commit_hook(MPIR_Datatype * datatype_p)
{
    return 0;
}
