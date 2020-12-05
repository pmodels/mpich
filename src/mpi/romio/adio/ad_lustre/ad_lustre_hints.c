/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *   Copyright (C) 1997 University of Chicago.
 *   See COPYRIGHT notice in top-level directory.
 *
 *   Copyright (C) 2007 Oak Ridge National Laboratory
 *
 *   Copyright (C) 2008 Sun Microsystems, Lustre group
 */

#include "ad_lustre.h"
#include "adio_extern.h"
#include "hint_fns.h"
#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif

void ADIOI_LUSTRE_SetInfo(ADIO_File fd, MPI_Info users_info, int *error_code)
{
    char *value;
    int flag;
    ADIO_Offset stripe_val[3], str_factor = -1, str_unit = 0, start_iodev = -1;
    int myrank;
    static char myname[] = "ADIOI_LUSTRE_SETINFO";
    /* These variables are for getting Lustre stripe count information */
    int lumlen, err, number_of_nodes, stripe_count = 0;
    FDTYPE temp_sys;
    int perm, old_mask, amode;
    struct lov_user_md *lum = NULL;
    MPI_Comm_rank(fd->comm, &myrank);

#ifdef HAVE_LUSTRE_LOCKAHEAD
    /* Set lock ahead default hints */
    fd->hints->fs_hints.lustre.lock_ahead_read = 0;
    fd->hints->fs_hints.lustre.lock_ahead_write = 0;
    fd->hints->fs_hints.lustre.lock_ahead_num_extents = 500;
    fd->hints->fs_hints.lustre.lock_ahead_flags = 0;
#endif

    value = (char *) ADIOI_Malloc((MPI_MAX_INFO_VAL + 1) * sizeof(char));
    if ((fd->info) == MPI_INFO_NULL) {
        /* This must be part of the open call. can set striping parameters
         * if necessary. */
        MPI_Info_create(&(fd->info));

        ADIOI_Info_set(fd->info, "direct_read", "false");
        ADIOI_Info_set(fd->info, "direct_write", "false");
        fd->direct_read = fd->direct_write = 0;
        /* initialize lustre hints */
        ADIOI_Info_set(fd->info, "romio_lustre_co_ratio", "1");
        fd->hints->fs_hints.lustre.co_ratio = 1;
        ADIOI_Info_set(fd->info, "romio_lustre_coll_threshold", "0");
        fd->hints->fs_hints.lustre.coll_threshold = 0;

        /* has user specified striping or server buffering parameters
         * and do they have the same value on all processes? */
        if (users_info != MPI_INFO_NULL) {
            /* striping information */
            ADIOI_Info_get(users_info, "striping_unit", MPI_MAX_INFO_VAL, value, &flag);
            if (flag) {
                ADIOI_Info_set(fd->info, "striping_unit", value);
                str_unit = atoll(value);
            }

            ADIOI_Info_get(users_info, "striping_factor", MPI_MAX_INFO_VAL, value, &flag);
            if (flag) {
                ADIOI_Info_set(fd->info, "striping_factor", value);
                str_factor = atoll(value);
            }

            ADIOI_Info_get(users_info, "romio_lustre_start_iodevice",
                           MPI_MAX_INFO_VAL, value, &flag);
            if (flag) {
                ADIOI_Info_set(fd->info, "romio_lustre_start_iodevice", value);
                start_iodev = atoll(value);
            }

            /* direct read and write */
            ADIOI_Info_get(users_info, "direct_read", MPI_MAX_INFO_VAL, value, &flag);
            if (flag && (!strcmp(value, "true") || !strcmp(value, "TRUE"))) {
                ADIOI_Info_set(fd->info, "direct_read", "true");
                fd->direct_read = 1;
            }
            ADIOI_Info_get(users_info, "direct_write", MPI_MAX_INFO_VAL, value, &flag);
            if (flag && (!strcmp(value, "true") || !strcmp(value, "TRUE"))) {
                ADIOI_Info_set(fd->info, "direct_write", "true");
                fd->direct_write = 1;
            }
#ifdef HAVE_LUSTRE_LOCKAHEAD
            /* Get lock ahead hints */

            ADIOI_Info_check_and_install_int(fd, users_info,
                                             "romio_lustre_cb_lock_ahead_write",
                                             &(fd->hints->fs_hints.lustre.lock_ahead_write),
                                             myname, error_code);
            ADIOI_Info_check_and_install_int(fd, users_info,
                                             "romio_lustre_cb_lock_ahead_read",
                                             &(fd->hints->fs_hints.lustre.lock_ahead_read),
                                             myname, error_code);

            /* If, and only if, we're using lock ahead,
             * process/set the number of extents to pre-lock and the flags */
            if (fd->hints->fs_hints.lustre.lock_ahead_read ||
                fd->hints->fs_hints.lustre.lock_ahead_write) {
                /* Get user's number of extents */
                ADIOI_Info_check_and_install_int(fd, users_info,
                                                 "romio_lustre_cb_lock_ahead_num_extents",
                                                 &(fd->hints->fs_hints.
                                                   lustre.lock_ahead_num_extents), myname,
                                                 error_code);

                /* ADIOI_Info_check_and_install_int doesn't set the
                 * value in fd unless it was in user_info, but knowing
                 * the value - default or explicit - is useful.
                 * Set the final number of extents in the fd->info */
                MPL_snprintf(value, MPI_MAX_INFO_VAL + 1, "%d",
                             fd->hints->fs_hints.lustre.lock_ahead_num_extents);
                ADIOI_Info_set(fd->info, "romio_lustre_cb_lock_ahead_num_extents", value);

                /* Get user's flags */
                ADIOI_Info_check_and_install_int(fd, users_info,
                                                 "romio_lustre_cb_lock_ahead_flags",
                                                 &(fd->hints->fs_hints.lustre.lock_ahead_flags),
                                                 myname, error_code);
            }
#endif
        }
        flag = 0;
        if (users_info != MPI_INFO_NULL) {
            ADIOI_Info_get(users_info, "number_of_nodes", MPI_MAX_INFO_VAL, value, &flag);
        }
        /* Must be true for all processes (done in ad_open.c) */  
        if (flag) {
            number_of_nodes = atoi(value);
            /* We set global aggregators automatically for user if hints are not specified.*/
            /* number_of_nodes is a system info set by ad_open.c, we need to perform nullity check.*/
            /*Get Lustre file striping factor in advance. This information is not availabe until the actual open time.
            However, we need it for determining cb_nodes and cb_config_list. Hence we open and close the file at here.
            rank 0 does the job and broadcast to other processes.*/
            if (myrank == 0) {
                if (fd->perm == ADIO_PERM_NULL) {
                    old_mask = umask(022);
                    umask(old_mask);
                    perm = old_mask ^ 0666;
                } else{
                    perm = fd->perm;
                }
                amode = 0;
                if (fd->access_mode & ADIO_CREATE)
                    amode = amode | O_CREAT;
                if (fd->access_mode & ADIO_RDONLY)
                    amode = amode | O_RDONLY;
                if (fd->access_mode & ADIO_WRONLY)
                    amode = amode | O_WRONLY;
                if (fd->access_mode & ADIO_RDWR)
                    amode = amode | O_RDWR;
                if (fd->access_mode & ADIO_EXCL)
                    amode = amode | O_EXCL;
                temp_sys = open(fd->filename, amode, perm);
                if (temp_sys == -1) {
                    /* Nothing we can do if we cannot even open the file, just abort processing Lustre hints in advance. */
                    stripe_count = 0;
                } else {
                    lumlen = sizeof(struct lov_user_md) + 1000 * sizeof(struct lov_user_ost_data);
                    lum = (struct lov_user_md *) ADIOI_Calloc(1, lumlen);
                    memset(lum, 0, lumlen);
                    lum->lmm_magic = LOV_USER_MAGIC;
                    err = ioctl(temp_sys, LL_IOC_LOV_GETSTRIPE, (void *) lum);
                    if (err){
                        /* If getting stripe count failed, we set it to zero. Hence setting hints based on stripe_count will be skipped. */
                        stripe_count = 0;
                    } else {
                        stripe_count = (int) lum->lmm_stripe_count;
                    }
                    close(temp_sys);
                    ADIOI_Free(lum);
                }
            }
        }
        /* Broadcast stripe count */
        MPI_Bcast( &stripe_count, 1, MPI_INT, 0, fd->comm );
        /* If cb_nodes has not been set by user or system, we set it to lustre striping factor
         * For some reasons, getting stripe count can give 0 for various reasons.
         * In that case, we do not want to cause trouble, simply jump this settings. */

        if (stripe_count && users_info != MPI_INFO_NULL) {
            ADIOI_Info_get(users_info, "cb_nodes", MPI_MAX_INFO_VAL, value, &flag);
            if (!flag) {
                MPL_snprintf(value, MPI_MAX_INFO_VAL + 1, "%d",stripe_count);
                //sprintf(value,"%d",stripe_count);
                ADIOI_Info_set(users_info, "cb_nodes", value);
            }
            /* If cb_config_list has not been set by user or system, we set it to dividing cb_nodes across all nodes */
            ADIOI_Info_get(users_info, "cb_config_list", MPI_MAX_INFO_VAL, value, &flag);
            if (!flag) {
                /* number_of_nodes is a system info set by ad_open.c, we need to perform nullity check. */
                MPL_snprintf(value, MPI_MAX_INFO_VAL + 1, "*:%d",(stripe_count + number_of_nodes - 1)/number_of_nodes);
                //sprintf(value,"*:%d",(stripe_count + number_of_nodes - 1)/number_of_nodes);
                ADIOI_Info_set(users_info, "cb_config_list", value);
            }
        }
        /* set striping information with ioctl */
        if (myrank == 0) {
            stripe_val[0] = str_factor;
            stripe_val[1] = str_unit;
            stripe_val[2] = start_iodev;
        }
        MPI_Bcast(stripe_val, 3, MPI_OFFSET, 0, fd->comm);

        /* do not open file in hint processing.   Open file in open routines,
         * where we can better deal with EXCL flag .  Continue to check the
         * "all processors set a value" condition holds.  */
        if (stripe_val[0] != str_factor
            || stripe_val[1] != str_unit || stripe_val[2] != start_iodev) {
            MPIO_ERR_CREATE_CODE_INFO_NOT_SAME("ADIOI_LUSTRE_SetInfo",
                                               "str_factor or str_unit or start_iodev", error_code);
            ADIOI_Free(value);
            return;
        }
    }

    /* get other hint */
    if (users_info != MPI_INFO_NULL) {
        /* CO: IO Clients/OST,
         * to keep the load balancing between clients and OSTs */
        ADIOI_Info_check_and_install_int(fd, users_info, "romio_lustre_co_ratio",
                                         &(fd->hints->fs_hints.lustre.co_ratio), myname,
                                         error_code);

        /* coll_threshold:
         * if the req size is bigger than this, collective IO may not be performed.
         */
        ADIOI_Info_check_and_install_int(fd, users_info, "romio_lustre_coll_threshold",
                                         &(fd->hints->fs_hints.lustre.coll_threshold), myname,
                                         error_code);
    }
    /* set the values for collective I/O and data sieving parameters */
    ADIOI_GEN_SetInfo(fd, users_info, error_code);

    /* generic hints might step on striping_unit */
    if (users_info != MPI_INFO_NULL) {
        ADIOI_Info_check_and_install_int(fd, users_info, "striping_unit", NULL, myname, error_code);
    }

    if (ADIOI_Direct_read)
        fd->direct_read = 1;
    if (ADIOI_Direct_write)
        fd->direct_write = 1;

    ADIOI_Free(value);

    *error_code = MPI_SUCCESS;
}
