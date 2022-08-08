/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "pmi_config.h"
#include "mpl.h"

#include "pmi_util.h"
#include "pmi.h"
#include "pmi_wire.h"
#include "pmi_common.h"

PMIState PMI_initialized = PMI_UNINITIALIZED;

int PMI_fd = -1;
int PMI_size = 1;
int PMI_rank = 0;
