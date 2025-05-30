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

int MPL_initshm_close(void)
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

struct initshm {
    char *initshm_name;
    bool is_root;
    void *initshm;
    int initshm_size;
};

#define MAX_INISHM 10
static struct initshm initshm_list[MAX_INISHM];
static int initshm_count = 0;

void *MPL_initshm_open(const char *name, int size, bool * is_root_p)
{
    if (initshm_count >= MAX_INISHM) {
        return NULL;
    }
    struct initshm *p = &initshm_list[initshm_count];
    p->initshm_name = MPL_strdup(name);
    p->initshm_size = size;
    p->is_root = false;

    int fd = shm_open(name, O_CREAT | O_RDWR | O_EXCL, S_IRUSR | S_IWUSR);
    if (fd != -1) {
        /* first process creates the shared memory and it is responsible for ftruncate the size */
        p->is_root = true;
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

    p->initshm = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (p->initshm == MAP_FAILED) {
        goto fn_fail;
    }

    if (is_root_p) {
        *is_root_p = p->is_root;
    }
    initshm_count++;
    return p->initshm;

  fn_fail:
    if (p->is_root) {
        shm_unlink(name);
    }
    return NULL;
}

int MPL_initshm_freeall(void)
{
    for (int i = 0; i < initshm_count; i++) {
        struct initshm *p = &initshm_list[i];
        munmap(p->initshm, p->initshm_size);
        if (p->is_root) {
            shm_unlink(p->initshm_name);
        }
        MPL_free(p->initshm_name);
    }
    initshm_count = 0;
    return MPL_SUCCESS;
}

#endif /* MPL_HAVE_POSIX_SHM */
