/*
 * Copyright 2026 Argonne National Laboratory
 * SPDX-License-Identifier: Apache-2.0
 */

#include "adio.h"
#include "ad_pvfs2.h"

int ADIOI_PVFS2_Feature(ADIO_File fd, int flag)
{
    switch (flag) {
        case ADIO_SCALABLE_OPEN:
        case ADIO_SCALABLE_RESIZE:
            return 1;
        case ADIO_SHARED_FP:
        case ADIO_LOCKS:
        case ADIO_SEQUENTIAL:
        case ADIO_DATA_SIEVING_WRITES:
        default:
            return 0;
    }
}
