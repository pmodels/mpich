/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpichconf.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include "pmi.h"

int PMI_Spawn_multiple(int count,
                       const char *cmds[],
                       const char **argvs[],
                       const int maxprocs[],
                       const int info_keyval_sizes[],
                       const PMI_keyval_t * info_keyval_vectors[],
                       int preput_keyval_size,
                       const PMI_keyval_t preput_keyval_vector[], int errors[])
{
    PMI_Abort(-1, NULL);
    return PMI_SUCCESS;
}
