/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2020 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2018-2020 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

#include "adio.h"
#include "ad_daos.h"

int ADIOI_DAOS_Feature(ADIO_File fd, int flag)
{
    switch (flag) {
        case ADIO_SCALABLE_OPEN:
        case ADIO_SCALABLE_RESIZE:
            return 1;
        case ADIO_TWO_PHASE:
        case ADIO_SHARED_FP:
        case ADIO_LOCKS:
        case ADIO_SEQUENTIAL:
        case ADIO_DATA_SIEVING_WRITES:
        case ADIO_ATOMIC_MODE:
        case ADIO_UNLINK_AFTER_CLOSE:
        default:
            return 0;
    }
}
