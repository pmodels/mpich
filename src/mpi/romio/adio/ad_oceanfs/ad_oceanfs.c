/*
* Copyright (C) by Argonne National Laboratory
*     See COPYRIGHT in top-level directory.
*/

#include "ad_oceanfs.h"
#include "adioi.h"

struct ADIOI_Fns_struct ADIO_OCEANFS_operations = {
    ADIOI_OCEANFS_Open, /* Open */
    ADIOI_GEN_OpenColl, /* OpenColl */
    ADIOI_GEN_ReadContig,       /* ReadContig */
    ADIOI_GEN_WriteContig,      /* WriteContig */
    ADIOI_GEN_ReadStridedColl,  /* ReadStridedColl */
    ADIOI_GEN_WriteStridedColl, /* WriteStridedColl */
    ADIOI_GEN_SeekIndividual,   /* SeekIndividual */
    ADIOI_GEN_Fcntl,    /* Fcntl */
    ADIOI_GEN_SetInfo,  /* SetInfo */
    ADIOI_GEN_ReadStrided,      /* ReadStrided */
    ADIOI_GEN_WriteStrided,     /* WriteStrided */
    ADIOI_GEN_Close,    /* Close */
    ADIOI_FAKE_IreadContig,     /* IreadContig */
    ADIOI_FAKE_IwriteContig,    /* IwriteContig */
    ADIOI_FAKE_IODone,  /* ReadDone */
    ADIOI_FAKE_IODone,  /* WriteDone */
    ADIOI_FAKE_IOComplete,      /* ReadComplete */
    ADIOI_FAKE_IOComplete,      /* WriteComplete */
    ADIOI_FAKE_IreadStrided,    /* IreadStrided */
    ADIOI_FAKE_IwriteStrided,   /* IwriteStrided */
    ADIOI_GEN_Flush,    /* Flush */
    ADIOI_GEN_Resize,   /* Resize */
    ADIOI_GEN_Delete,   /* Delete */
    ADIOI_GEN_Feature,  /* Feature */
    "OCEANFS: ROMIO driver for OCEANFS",
    ADIOI_GEN_IreadStridedColl, /* IreadStridedColl */
    ADIOI_GEN_IwriteStridedColl,        /* IwriteStridedColl */
#if defined(F_SETLKW64)
    ADIOI_GEN_SetLock   /* SetLock */
#else
    ADIOI_GEN_SetLock64 /* SetLock */
#endif
};