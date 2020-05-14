/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "fbox_impl.h"
#include "fbox_types.h"

/* Note: without the following initialiazation, the common symbol may get lost
   during linking. This is manifested by static linking on mac osx.
*/
MPIDI_POSIX_eager_fbox_control_t MPIDI_POSIX_eager_fbox_control_global = { 0 };
