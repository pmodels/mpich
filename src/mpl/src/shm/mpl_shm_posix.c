/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* NOTE: currently this only implements MPL_initshm_ interface */

#include "mpl.h"

MPL_SUPPRESS_OSX_HAS_NO_SYMBOLS_WARNING;

#ifndef MPL_HAS_POSIX_SHM

void *MPL_initshm_open(const char *name, int size)
{
    return NULL;
}

int MPL_initshm_close(void)
{
    return MPL_SUCCESS;
}

#else /* MPL_HAS_POSIX_SHM */

#include <fcntl.h>
#ifdef MPL_HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif
#ifdef MPL_HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

static const char *initshm_name;
static bool is_root = false;
static void *initshm;
static int initshm_size;

void *MPL_initshm_open(const char *name, int size, bool * is_root_p)
{
    initshm_name = name;
    initshm_size = size;

    int fd = shm_open(name, O_CREAT | O_RDWR | O_EXCL, S_IRUSR | S_IWUSR);
    if (fd > 0) {
        /* first process creates the shared memory and it is responsible for ftruncate the size */
        is_root = true;
        int rc = ftruncate(fd, size);
        if (rc) {
            goto fn_fail;
        }
    } else {
        /* shared memory exist, just open it */
        fd = shm_open(name, O_RDWR, S_IRUSR | S_IWUSR);
        if (!fd) {
            goto fn_fail;
        }
    }

    initshm = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (initshm == MAP_FAILED) {
        goto fn_fail;
    }

  fn_exit:
    if (is_root_p) {
        *is_root_p = is_root;
    }
    return initshm;

  fn_fail:
    if (is_root) {
        shm_unlink(name);
    }
    return NULL;
}

int MPL_initshm_close(void)
{
    munmap(initshm, initshm_size);
    if (is_root) {
        shm_unlink(initshm_name);
    }
    return MPL_SUCCESS;
}

#endif /* MPL_HAS_POSIX_SHM */
