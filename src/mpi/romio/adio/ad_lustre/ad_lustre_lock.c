/*
 *  Copyright 2016 Cray Inc. All Rights Reserved.
 */
#include "adio.h"

#include "ad_lustre.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <stdint.h>

#ifndef LL_IOC_LOCK_AHEAD
/* Declarations for lustre lock ahead */
#define LL_IOC_LOCK_AHEAD _IOWR('f', 251, struct llapi_lock_ahead)

typedef enum {
    READ_USER = 1,
    WRITE_USER,
    MAX_USER,
} lock_mode_user;

/* Extent for lock ahead requests */
struct lu_extent {
    __u64 start;
    __u64 end;
    /* 0 on success, -ERRNO on error, except -EWOULDBLOCK, which
     * indicates the lock was not granted because a
     * conflicting lock was found */
    __u32 result;
};

/* lock ahead ioctl arguments */
struct llapi_lock_ahead {
    __u32 lla_version;
    __u32 lla_lock_mode;
    __u32 lla_flags;
    __u32 lla_extent_count;
    struct lu_extent lla_extents[0];
};
#endif

int ADIOI_LUSTRE_clear_locks(ADIO_File fd)
{
    int err;
    int id;

    if (!fd->my_cb_nodes_index) {
        srand(time(NULL));
        id = rand();
        err = ioctl(fd->fd_sys, LL_IOC_GROUP_LOCK, id);
        err = ioctl(fd->fd_sys, LL_IOC_GROUP_UNLOCK, id);
    }
    return err;
}

void ADIOI_LUSTRE_lock_ahead_ioctl(ADIO_File fd, int avail_cb_nodes, ADIO_Offset next_offset,
                                   int *error_code)
{
    struct llapi_lock_ahead *arg;
    int err = 0, i;
    int num_extents = fd->hints->fs_hints.lustre.lock_ahead_num_extents;
    int flags = fd->hints->fs_hints.lustre.lock_ahead_flags;
    ADIO_Offset offset = 0, step_size = 0;
    int stripe_size = fd->hints->striping_unit;

    int agg_idx = fd->my_cb_nodes_index;

    /* Not a collective aggregator? Do nothing and return
     * since current code is based on aggregator/stripes */
    if (agg_idx < 0) {
        /* Disable further lock ahead ...
         * fd->hints->fs_hints.lustre.lock_ahead_read  = 0;
         * fd->hints->fs_hints.lustre.lock_ahead_write = 0;
         * fd->hints->fs_hints.lustre.lock_ahead_start_extent = 0;
         * fd->hints->fs_hints.lustre.lock_ahead_end_extent   = INT64_MAX;
         */
        return;
    }
#ifdef LOCK_AHEAD_DEBUG
    {
        /* Debug check.  Calculate the expected rank for this stripe */
        int rank_index;
        rank_index = (int) ((next_offset / stripe_size) % avail_cb_nodes);
        /* Not sure why, but this happens in the generic read coll?
         * It doesn't do the aggregation striped quite as expected.
         * We'll probably lock the wrong stripes ...
         */
        if (agg_idx != rank_index) {
            fprintf(stderr, "%s(%d) rank[%d] file system %d "
                    "lock ahead debug R(%d)/W(%d), "
                    "aggregator %d(%d)/%d(%d), "
                    "offset %lld, start offset %lld, stripe %lld "
                    "num_extents %d\n",
                    __func__, __LINE__,
                    fd->hints->ranklist[agg_idx],
                    fd->file_system,
                    fd->hints->fs_hints.lustre.lock_ahead_read,
                    fd->hints->fs_hints.lustre.lock_ahead_write,
                    agg_idx, rank_index,
                    avail_cb_nodes, fd->hints->cb_nodes,
                    (long long) next_offset, (long long) (next_offset / stripe_size * stripe_size),
                    (long long) next_offset / stripe_size, num_extents);
        }
        /* Just checking the config vs what was passed in */
        if (agg_idx >= avail_cb_nodes) {
            fprintf(stderr, "%s(%d) file system %d "
                    "lock ahead debug R(%d)/W(%d), "
                    "aggregator %d(%d)/%d(%d), "
                    "num_extents %d\n",
                    __func__, __LINE__, fd->file_system,
                    fd->hints->fs_hints.lustre.lock_ahead_read,
                    fd->hints->fs_hints.lustre.lock_ahead_write,
                    agg_idx, rank_index, avail_cb_nodes, fd->hints->cb_nodes, num_extents);
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
    }
#endif

    /* Check file access vs requested lock ahead */
    if (fd->access_mode & ADIO_RDONLY) {
        /* Don't need write lock ahead */
        fd->hints->fs_hints.lustre.lock_ahead_write = 0;

        /* Do need read lock ahead or give up. */
        if (!(fd->hints->fs_hints.lustre.lock_ahead_read)) {
            fd->hints->fs_hints.lustre.lock_ahead_start_extent = 0;
            fd->hints->fs_hints.lustre.lock_ahead_end_extent = INT64_MAX;
            return;
        }
    }
    if (fd->access_mode & ADIO_WRONLY) {
        /* Don't need read lock ahead */
        fd->hints->fs_hints.lustre.lock_ahead_read = 0;

        /* Do need write lock ahead or give up. */
        if (!(fd->hints->fs_hints.lustre.lock_ahead_write)) {
            fd->hints->fs_hints.lustre.lock_ahead_start_extent = 0;
            fd->hints->fs_hints.lustre.lock_ahead_end_extent = INT64_MAX;
            return;
        }
    }


    step_size = (ADIO_Offset) avail_cb_nodes *stripe_size;

    if (next_offset == 0) {     /* 1st call, calculate our starting offset */
        offset = (ADIO_Offset) agg_idx *stripe_size;
    } else      /* Have to assume we're writing to one of our stripes */
        offset = next_offset / stripe_size * stripe_size;       /* start of stripe */

    arg = ADIOI_Malloc(sizeof(struct llapi_lock_ahead) + sizeof(struct lu_extent) * num_extents);
    for (i = 0; i < num_extents; ++i) {
        arg->lla_extents[i].start = offset;
        arg->lla_extents[i].end = offset + stripe_size - 1;
        arg->lla_extents[i].result = 0;
        offset += step_size;
    }

    /* Simply save the new start/end extents, forget what we aleady had locked
     * since lustre may reclaim it at any time. */
    fd->hints->fs_hints.lustre.lock_ahead_start_extent = arg->lla_extents[0].start;
    fd->hints->fs_hints.lustre.lock_ahead_end_extent = arg->lla_extents[num_extents - 1].end;

    arg->lla_version = 1;
    arg->lla_extent_count = num_extents;
    arg->lla_flags = flags;

    if (fd->hints->fs_hints.lustre.lock_ahead_write)    /* or read/write */
        arg->lla_lock_mode = WRITE_USER;
    else if (fd->hints->fs_hints.lustre.lock_ahead_read)        /* read only */
        arg->lla_lock_mode = READ_USER;
    else
        MPI_Abort(MPI_COMM_WORLD, 1);

    err = ioctl(fd->fd_sys, LL_IOC_LOCK_AHEAD, arg);

#ifdef LOCK_AHEAD_DEBUG
    /* Print any per extent errors */
    for (i = 0; i < num_extents; ++i) {
        if (arg->lla_extents[i].result) {
            fprintf(stderr, "%s(%d) "
                    "lock ahead extent[%4.4d] {%ld,%ld} stripe {%lld,%lld} errno %d\n",
                    __func__, __LINE__,
                    i,
                    (long int) arg->lla_extents[i].start,
                    (long int) arg->lla_extents[i].end,
                    (long int) arg->lla_extents[i].start / stripe_size,
                    (long int) arg->lla_extents[i].end / stripe_size, arg->lla_extents[i].result);
        }
    }
#endif


    if (err == -1) {    /* turn off lock ahead after a failure */
#ifdef LOCK_AHEAD_DEBUG
        fprintf(stderr, "%s(%d) file system %d "
                "lock ahead failure R(%d)/W(%d), "
                "aggregator %d/%d, "
                "next offset %lld, stripe %lld, "
                "last offset %lld, stripe %lld, "
                "step %lld, stripe size %lld "
                "num_extents %d\n",
                __func__, __LINE__, fd->file_system,
                fd->hints->fs_hints.lustre.lock_ahead_read,
                fd->hints->fs_hints.lustre.lock_ahead_write,
                agg_idx,
                avail_cb_nodes,
                (long long) next_offset, (long long) next_offset / stripe_size,
                (long long) offset, (long long) offset / stripe_size,
                (long long) step_size, (long long) stripe_size, num_extents);
#endif
        fd->hints->fs_hints.lustre.lock_ahead_read = 0;
        fd->hints->fs_hints.lustre.lock_ahead_write = 0;
        fd->hints->fs_hints.lustre.lock_ahead_start_extent = 0;
        fd->hints->fs_hints.lustre.lock_ahead_end_extent = INT64_MAX;

        *error_code = ADIOI_Err_create_code("ADIOI_LUSTRE_lock_ahead_ioctl", fd->filename, errno);
        if (agg_idx == 0) {
            fprintf(stderr, "%s: ioctl(LL_IOC_LOCK_AHEAD) \'%s\'\n", __func__, strerror(errno));
        }

    }

    ADIOI_Free(arg);
    return;
}
