/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *   Copyright (C) 1997 University of Chicago.
 *   See COPYRIGHT notice in top-level directory.
 *
 *   Copyright (C) 2018 Intel Corporation
 */

#include "ad_daos.h"

/* adioi.h has the ADIOI_Fns_struct define */
#include "adioi.h"

struct ADIOI_Fns_struct ADIO_DAOS_operations = {
    ADIOI_DAOS_Open, /* Open */
    ADIOI_DAOS_OpenColl, /* OpenColl */
    ADIOI_DAOS_ReadContig, /* ReadContig */
    ADIOI_DAOS_WriteContig, /* WriteContig */
    ADIOI_GEN_ReadStridedColl, /* ReadStridedColl */
    ADIOI_GEN_WriteStridedColl, /* WriteStridedColl */
    ADIOI_GEN_SeekIndividual, /* SeekIndividual */
    ADIOI_DAOS_Fcntl, /* Fcntl */
    ADIOI_DAOS_SetInfo, /* SetInfo */
    ADIOI_DAOS_ReadStrided, /* ReadStrided */
    ADIOI_DAOS_WriteStrided, /* WriteStrided */
    ADIOI_DAOS_Close, /* Close */
    ADIOI_DAOS_IReadContig, /* IreadContig */
    ADIOI_DAOS_IWriteContig, /* IwriteContig */
    ADIOI_FAKE_IODone, /* ReadDone */
    ADIOI_FAKE_IODone, /* WriteDone */
    ADIOI_FAKE_IOComplete, /* ReadComplete */
    ADIOI_FAKE_IOComplete, /* WriteComplete */
    ADIOI_DAOS_IreadStrided, /* IreadStrided */
    ADIOI_DAOS_IwriteStrided, /* IwriteStrided */
    ADIOI_DAOS_Flush, /* Flush */
    ADIOI_DAOS_Resize, /* Resize */
    ADIOI_DAOS_Delete, /* Delete */
    ADIOI_DAOS_Feature, /* Features */
    "DAOS: ROMIO driver for DAOS",
};
