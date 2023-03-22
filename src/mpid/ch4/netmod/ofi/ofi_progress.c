/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "ofi_impl.h"
#include "ofi_events.h"

int MPIDI_OFI_progress_uninlined(int vci)
{
    return MPIDI_NM_progress(vci, 0);
}
