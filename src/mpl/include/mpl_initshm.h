/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPL_INITSHM_H_INCLUDED

/* This file defines an interface where a process opens or creates a shared
 * memory *without* synchronization with another process. This is useful to
 * implement the MPI session model where not all local processes may present
 * during a communicator bootstrapping stage.
 *
 * We rely on pre-agreement of a shared name and the shared memory size. The
 * former can be derived from e.g. PMI kvsname. The latter derives from local
 * size (also from PMI) or a compile time maximum size.
 *
 * It is limited to a single slab throughout due to the limit of pre-agreement.
 * That's why it is only suitable as initshm.
 *
 * Currently we only implement this interface via POSIX shared memory.
 */

void *MPL_initshm_open(const char *name, int size, bool * is_root);
int MPL_initshm_freeall(void);

#endif /* MPL_INITSHM_H_INCLUDED */
