/*
 * Copyright 2026 Argonne National Laboratory
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MPIR_PROGRESS_HOOK_H_INCLUDED
#define MPIR_PROGRESS_HOOK_H_INCLUDED

int MPIR_Progress_hook_exec_all(int vci, int *made_progress);
int MPIR_Progress_hook_register(int vci, int (*progress_fn) (int, int *), int *id);
int MPIR_Progress_hook_deregister(int id);
int MPIR_Progress_hook_activate(int id);
int MPIR_Progress_hook_deactivate(int id);

#endif /* MPIR_PROGRESS_HOOK_H_INCLUDED */
