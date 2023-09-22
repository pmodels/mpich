/*
* Copyright (C) by Argonne National Laboratory
*     See COPYRIGHT in top-level directory.
*/

#include "ad_oceanfs.h"
#include <sys/ioctl.h>

typedef struct oceanfs_group_lock {
    int fd;
    u_int64_t lock_id;
} oceanfs_group_lock_t;

#define OCEANFS_IOCTL_GET_GROUPLOCK    _IOWR('S', 103, oceanfs_group_lock_t)
#define OCEANFS_IOCTL_SET_GROUPLOCK    _IOWR('S', 111, oceanfs_group_lock_t)

int ADIOI_OCEANFS_GetGroupLock(int fd, u_int64_t * lock_id)
{
    oceanfs_group_lock_t group_lock;
    group_lock.fd = fd;
    int ret = ioctl(fd, OCEANFS_IOCTL_GET_GROUPLOCK, &group_lock);
    if (ret) {
        *lock_id = 0;
        return ret;
    }

    *lock_id = group_lock.lock_id;
    return ret;
}

int ADIOI_OCEANFS_SetGroupLock(int fd, u_int64_t lock_id)
{
    oceanfs_group_lock_t group_lock;
    group_lock.fd = fd;
    group_lock.lock_id = lock_id;
    return ioctl(fd, OCEANFS_IOCTL_SET_GROUPLOCK, &group_lock);
}

int ADIOI_OCEANFS_GetMode(ADIO_File fd)
{
    int amode = 0;
    /* setup the file access mode */
    if (fd->access_mode & ADIO_CREATE) {
        amode = amode | O_CREAT;
    }
    if (fd->access_mode & ADIO_RDONLY) {
        amode = amode | O_RDONLY;
    }
    if (fd->access_mode & ADIO_WRONLY) {
        amode = amode | O_WRONLY;
    }
    if (fd->access_mode & ADIO_RDWR) {
        amode = amode | O_RDWR;
    }
    if (fd->access_mode & ADIO_EXCL) {
        amode = amode | O_EXCL;
    }
    if (fd->access_mode & ADIO_APPEND) {
        amode = amode | O_APPEND;
    }

    return amode;
}

// using group_lock in default.
static int ADIOI_OCEANFS_GroupLockEnable()
{
    int group_lock = 1;
    char *env = NULL;
    env = getenv("OCEANFS_MPIO_GROUP_LOCK_ENABLE");
    if (env) {
        group_lock = atoi(env);
    }

    return group_lock;
}

static void ADIOI_OCEANFS_SyncGroupLock(ADIO_File fd, int rank)
{
    int ret;
    u_int64_t lock_id = 0;

    if (!ADIOI_OCEANFS_GroupLockEnable()) {
        return;
    }
    if (rank == 0) {
        ret = ADIOI_OCEANFS_GetGroupLock(fd->fd_sys, &lock_id);
        ADIOI_Assert(ret == 0);
        MPI_Bcast(&lock_id, 1, MPI_UNSIGNED_LONG_LONG, 0, fd->comm);
    } else {
        MPI_Bcast(&lock_id, 1, MPI_UNSIGNED_LONG_LONG, 0, fd->comm);
        ret = ADIOI_OCEANFS_SetGroupLock(fd->fd_sys, lock_id);
        ADIOI_Assert(ret == 0);
    }
}

static int ADIOI_OCEANFS_SetupFilePerm(ADIO_File fd)
{
    static const int umask_param = 022;
    static const int mask_param = 0666;
    mode_t old_mask;
    int perm;
    if (fd->perm == ADIO_PERM_NULL) {
        old_mask = umask(umask_param);
        umask(old_mask);
        perm = old_mask ^ mask_param;
    } else {
        perm = fd->perm;
    }
    perm &= ~S_IFMT;
    perm |= S_IFREG;

    return perm;
}

void ADIOI_OCEANFS_Open(ADIO_File fd, int *error_code)
{
    static char myname[] = "ADIOI_OCEANFS_OPEN";
    int perm, amode, ret, rank;
    *error_code = MPI_SUCCESS;

    /* setup file permissions */
    perm = ADIOI_OCEANFS_SetupFilePerm(fd);
    amode = ADIOI_OCEANFS_GetMode(fd);
    /* init OCEANFS */
    fd->fs_ptr = NULL;
    MPI_Comm_rank(fd->comm, &rank);

    /* all processes open the file */
    ret = open(fd->filename, amode, perm);
    if (ret < 0) {
        *error_code = ADIOI_Err_create_code(myname, fd->filename, errno);
        return;
    }

    fd->fd_sys = ret;
    fd->fd_direct = -1;

    ADIOI_OCEANFS_SyncGroupLock(fd, rank);

    if ((fd->fd_sys != -1) && ((u_int32_t) fd->access_mode & ADIO_APPEND)) {
        ret = lseek(fd->fd_sys, 0, SEEK_END);
        if (ret < 0) {
            *error_code = ADIOI_Err_create_code(myname, fd->filename, errno);
            return;
        }
        fd->fp_ind = ret;
        fd->fp_sys_posn = ret;
    }
}

