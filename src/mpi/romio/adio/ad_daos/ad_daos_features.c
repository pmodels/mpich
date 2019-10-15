/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 * Copyright (C) 2018-2019 Intel Corporation
 *
 * GOVERNMENT LICENSE RIGHTS-OPEN SOURCE SOFTWARE
 * The Government's rights to use, modify, reproduce, release, perform, display,
 * or disclose this software are subject to the terms of the Apache License as
 * provided in Contract No. 8F-30005.
 * Any reproduction of computer software, computer software documentation, or
 * portions thereof marked with this legend must also reproduce the markings.
 */

#include "adio.h"
#include "ad_daos.h"

int ADIOI_DAOS_Feature(ADIO_File fd, int flag)
{
    switch (flag) {
        case ADIO_TWO_PHASE:
        case ADIO_SCALABLE_OPEN:
        case ADIO_SCALABLE_RESIZE:
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
