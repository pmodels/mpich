/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *   Copyright (C) 1997 University of Chicago.
 *   See COPYRIGHT notice in top-level directory.
 *
 *   Copyright (C) 2007 Oak Ridge National Laboratory
 *
 *   Copyright (C) 2008 Sun Microsystems, Lustre group
 *
 *   Copyright (C) 2018 DDN, Lustre group
 */

#include "ad_lustre.h"

#undef OPEN_DEBUG

int ADIOI_LUSTRE_clear_locks(ADIO_File fd);     /* in ad_lustre_lock.c */
int ADIOI_LUSTRE_request_only_lock_ioctl(ADIO_File fd); /* in ad_lustre_lock.c */

void ADIOI_LUSTRE_Open(ADIO_File fd, int *error_code)
{
    int perm, old_mask, amode, amode_direct;
    int myrank, flag, err = MPI_SUCCESS, set_layout = 0;
    struct llapi_layout *layout = NULL;
    char *value;
    ADIO_Offset str_factor = 0, str_unit = 0, start_iodev = -1;

#if defined(MPICH) || !defined(PRINT_ERR_MSG)
    static char myname[] = "ADIOI_LUSTRE_OPEN";
#endif

    MPI_Comm_rank(fd->comm, &myrank);

    if (fd->perm == ADIO_PERM_NULL) {
        old_mask = umask(022);
        umask(old_mask);
        perm = old_mask ^ 0666;
    } else
        perm = fd->perm;

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

    amode_direct = amode | O_DIRECT;

    value = (char *) ADIOI_Malloc((MPI_MAX_INFO_VAL + 1) * sizeof(char));
    /* we already validated in LUSTRE_SetInfo that these are going to be the same */
    /* striping information */
    if (fd->info != MPI_INFO_NULL) {
#ifdef HAVE_LUSTRE_COMP_LAYOUT_SUPPORT
        if (fd->hints->fs_hints.lustre.comp_layout != NULL) {
            set_layout = 1;
        } else
#endif
        {
            ADIOI_Info_get(fd->info, "striping_unit", MPI_MAX_INFO_VAL, value, &flag);
            if (flag)
                str_unit = atoll(value);

            ADIOI_Info_get(fd->info, "striping_factor", MPI_MAX_INFO_VAL, value, &flag);
            if (flag)
                str_factor = atoll(value);

            ADIOI_Info_get(fd->info, "romio_lustre_start_iodevice", MPI_MAX_INFO_VAL, value, &flag);
            if (flag)
                start_iodev = atoll(value);

            if (str_unit > 0 || (str_factor == -1 || str_factor > 0) || start_iodev >= 0)
                set_layout = 1;
#ifdef OPEN_DEBUG
            if (myrank == 0)
                LDEBUG("stripe_size=%ld, stripe_count=%d, stripe_offset=%d\n",
                       (long long) str_unit, (int) str_factor, (int) start_iodev);
#endif
        }
    }

    /* we set these hints on new files */
    if ((amode & O_CREAT) && (myrank == 0 || fd->comm == MPI_COMM_SELF) && set_layout) {
        /* if user has specified striping info, first aggregator tries to set
         * it */
#ifdef HAVE_LUSTRE_COMP_LAYOUT_SUPPORT
        if (fd->hints->fs_hints.lustre.comp_layout != NULL) {
            char *comp_layout = fd->hints->fs_hints.lustre.comp_layout;
#ifdef HAVE_YAML_SUPPORT
            /* YAML template file ? */
            if (ADIOI_LUSTRE_Parse_yaml_temp(comp_layout, &layout))
#endif /* HAVE_YAML_SUPPORT */
                /* option string ? */
                if (ADIOI_LUSTRE_Parse_comp_layout_opt(comp_layout, &layout))
                    /* lustre source file ? */
                    if (!(layout = llapi_layout_get_by_path(comp_layout, 0))) {
                        LDEBUG("'%s' is not a Lustre src file.\n", comp_layout);
                        GOTO(fn_exit, err = MPI_KEYVAL_INVALID);
                    }
        } else
#endif /* HAVE_LUSTRE_COMP_LAYOUT_SUPPORT */
        {
            layout = llapi_layout_alloc();
            if (layout == NULL) {
                LDEBUG("Failed to allocate Lustre file layout %s \n", strerror(errno));
                goto fn_exit;
            }

            if (str_unit > 0) {
                str_unit = str_unit > UINT_MAX ? UINT_MAX : str_unit;
                err = llapi_layout_stripe_size_set(layout, (uint64_t) str_unit);
                if (err) {
                    LDEBUG("Failed to set stripe_size (%d): %s \n", err, strerror(errno));
                    GOTO(fn_exit, err = MPI_KEYVAL_INVALID);
                }
            }
            if (str_factor == -1 || str_factor > 0) {
#ifdef HAVE_LUSTRE_COMP_LAYOUT_SUPPORT
                if (str_factor == -1)
                    str_factor = LLAPI_LAYOUT_WIDE;
                else
#endif
                if (str_factor > USHRT_MAX)
                    str_factor = USHRT_MAX;
                err = llapi_layout_stripe_count_set(layout, (uint64_t) str_factor);
                if (err) {
                    LDEBUG("Failed to set stripe_count (%d): %s\n", err, strerror(errno));
                    GOTO(fn_exit, err = MPI_KEYVAL_INVALID);
                }
            }
            if (start_iodev >= 0) {
                start_iodev = start_iodev > USHRT_MAX ? USHRT_MAX : start_iodev;
                err = llapi_layout_ost_index_set(layout, 0, (uint64_t) start_iodev);
                if (err) {
                    LDEBUG("Failed to set stripe_offset (%d): %s \n", err, strerror(errno));
                    GOTO(fn_exit, err = MPI_KEYVAL_INVALID);
                }
            }
        }

        err = llapi_layout_file_create(fd->filename, amode, perm, layout);
        if (err > 0)
            fd->fd_sys = err;
        else if (err == -1 && errno != EEXIST)
            /* not a fatal error, but user might care to know */
            LDEBUG("Failed to set stripe info %s \n", strerror(errno));
    }
    /* End of striping parameters validation */
    if (fd->fd_sys <= 0) {
        if (set_layout || myrank != 0)
            amode &= ~O_CREAT;
        fd->fd_sys = open(fd->filename, amode, perm);
    }
#ifdef OPEN_DEBUG
    LDEBUG("rank(%d): fd_sys=%d\n", myrank, fd->fd_sys);
#endif
    if (fd->fd_sys <= 0)
        goto fn_exit;

    /* Get/set common stripe size and LCM of stripe count */
    layout = llapi_layout_get_by_fd(fd->fd_sys, 0);
    if (layout != NULL) {
        fd->hints->striping_unit = ADIOI_LUSTRE_Get_last_stripe_size(layout);
        fd->hints->striping_factor = ADIOI_LUSTRE_Get_lcm_stripe_count(layout);
    }

    if (fd->access_mode & ADIO_APPEND)
        fd->fp_ind = fd->fp_sys_posn = lseek(fd->fd_sys, 0, SEEK_END);

    fd->fd_direct = -1;
    if (fd->direct_write || fd->direct_read) {
        fd->fd_direct = open(fd->filename, amode_direct, perm);
        if (fd->fd_direct != -1) {
            fd->d_mem = fd->d_miniosz = (1 << 12);
        } else {
            perror("cannot open file with O_Direct");
            fd->direct_write = fd->direct_read = 0;
        }
    }
#ifdef HAVE_LUSTRE_LOCKAHEAD
    if (fd->hints->fs_hints.lustre.lock_ahead_read || fd->hints->fs_hints.lustre.lock_ahead_write) {
        ADIOI_LUSTRE_clear_locks(fd);
        ADIOI_LUSTRE_request_only_lock_ioctl(fd);
    }
#endif


  fn_exit:
    llapi_layout_free(layout);
    ADIOI_Free(value);
    /* --BEGIN ERROR HANDLING-- */
    if (fd->fd_sys == -1 || ((fd->fd_direct == -1) && (fd->direct_write || fd->direct_read)))
        *error_code = ADIOI_Err_create_code(myname, fd->filename, errno);
    /* --END ERROR HANDLING-- */
    else
        *error_code = MPI_SUCCESS;

    if (err == MPI_KEYVAL_INVALID)
        *error_code = MPI_KEYVAL_INVALID;
}
