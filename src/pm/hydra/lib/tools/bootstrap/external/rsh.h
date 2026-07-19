/*
 * Copyright 2026 Argonne National Laboratory
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef RSH_H_INCLUDED
#define RSH_H_INCLUDED

#include "hydra.h"

HYD_status HYDT_bscd_rsh_query_env_inherit(const char *env_name, int *should_inherit);

#endif /* RSH_H_INCLUDED */
