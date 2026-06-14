/*
 * Copyright 2026 Argonne National Laboratory
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mpidimpl.h"
#include "ofi_impl.h"
#include "ofi_events.h"

int MPIDI_OFI_progress_uninlined(int vci)
{
    int made_progress;
    return MPIDI_NM_progress(vci, &made_progress);
}
