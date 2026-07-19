/*
 * Copyright 2026 Argonne National Laboratory
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MPL_ARG_SERIAL_H_INCLUDED
#define MPL_ARG_SERIAL_H_INCLUDED

#include "mplconfig.h"

int MPL_args_serialize(int argc, char **argv, int *len, void **serialized_buf);
int MPL_args_deserialize(int len, const void *serialized_buf, int *argc, char ***argv);

#endif /* MPL_ARG_SERIAL_H_INCLUDED */
