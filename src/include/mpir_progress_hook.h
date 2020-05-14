/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPIR_PROGRESS_HOOK_H_INCLUDED
#define MPIR_PROGRESS_HOOK_H_INCLUDED

int MPIR_Progress_hook_exec_all(int *made_progress);
int MPIR_Progress_hook_register(int (*progress_fn) (int *), int *id);
int MPIR_Progress_hook_deregister(int id);
int MPIR_Progress_hook_activate(int id);
int MPIR_Progress_hook_deactivate(int id);

#endif /* MPIR_PROGRESS_HOOK_H_INCLUDED */
