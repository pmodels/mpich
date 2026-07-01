/*
 * Copyright 2026 Argonne National Laboratory
 * SPDX-License-Identifier: Apache-2.0
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
