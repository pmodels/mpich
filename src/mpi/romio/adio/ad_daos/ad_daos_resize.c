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

#include "ad_daos.h"

void ADIOI_DAOS_Resize(ADIO_File fd, ADIO_Offset size, int *error_code)
{
    int ret, rank;
    struct ADIO_DAOS_cont *cont = fd->fs_ptr;
    static char myname[] = "ADIOI_DAOS_RESIZE";

    *error_code = MPI_SUCCESS;
    MPI_Comm_rank(fd->comm, &rank);
    MPI_Barrier(fd->comm);

    if (rank == fd->hints->ranklist[0])
	ret = daos_array_set_size(cont->oh, DAOS_TX_NONE, size, NULL);

    MPI_Bcast(&ret, 1, MPI_INT, fd->hints->ranklist[0], fd->comm);
    if (ret != 0)
        *error_code = ADIOI_DAOS_err(myname, cont->obj_name, __LINE__, ret);
}
