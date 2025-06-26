/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* NOTE: currently this only implements MPL_initshm_ interface */

#include "mpl.h"

MPL_SUPPRESS_OSX_HAS_NO_SYMBOLS_WARNING;

#ifndef MPL_HAVE_POSIX_SHM

void *MPL_initshm_open(const char *name, int size, bool * is_root_p)
{
    return NULL;
}

int MPL_initshm_free(const char *name, void *slab, int size, bool need_unlink);
{
    return MPL_SUCCESS;
}

#else /* MPL_HAVE_POSIX_SHM */

#include <fcntl.h>
#ifdef MPL_HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif
#ifdef MPL_HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

void *MPL_initshm_open(const char *name, int size, bool * is_root_p)
{
    void *slab = NULL;
    bool is_root = false;

    int fd = shm_open(name, O_CREAT | O_RDWR | O_EXCL, S_IRUSR | S_IWUSR);
    if (fd != -1) {
        /* first process creates the shared memory and it is responsible for ftruncate the size */
        is_root = true;
        int rc = ftruncate(fd, size);
        if (rc) {
            goto fn_fail;
        }
    } else {
        /* shared memory exist, just open it */
        fd = shm_open(name, O_RDWR, S_IRUSR | S_IWUSR);
        if (fd == -1) {
            goto fn_fail;
        }

        /* wait until the region size is set by the root */
        struct stat statbuf;
        do {
            int rc = fstat(fd, &statbuf);
            if (rc) {
                goto fn_fail;
            }
        } while (statbuf.st_size == 0);

        if (statbuf.st_size < size) {
            goto fn_fail;
        }
    }

    slab = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (slab == MAP_FAILED) {
        goto fn_fail;
    }

    if (is_root_p) {
        *is_root_p = is_root;
    }
    return slab;

  fn_fail:
    if (is_root) {
        shm_unlink(name);
    }
    return NULL;
}

int MPL_initshm_free(const char *name, void *slab, int size, bool need_unlink)
{
    munmap(slab, size);
    if (need_unlink) {
        shm_unlink(name);
    }
    return MPL_SUCCESS;
}

#endif /* MPL_HAVE_POSIX_SHM */
