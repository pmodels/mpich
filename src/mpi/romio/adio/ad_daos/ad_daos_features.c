/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *   Copyright (C) 1997 University of Chicago.
 *   See COPYRIGHT notice in top-level directory.
 *
 *   Copyright (C) 2018 Intel Corporation
 */

#include "adio.h"
#include "ad_daos.h"

int ADIOI_DAOS_Feature(ADIO_File fd, int flag)
{
	switch(flag) {
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
