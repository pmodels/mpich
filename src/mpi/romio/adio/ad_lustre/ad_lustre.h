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

#ifndef AD_LUSTRE_H_INCLUDED
#define AD_LUSTRE_H_INCLUDED

#define HAVE_ASM_TYPES_H 1

#include <unistd.h>
#include <linux/types.h>

#ifdef __linux__
#include <sys/ioctl.h>  /* necessary for: */
#include <time.h>
#ifndef __USE_GNU
#define __USE_GNU 1     /* O_DIRECT and */
#include <fcntl.h>      /* IO operations */
#endif
#undef __USE_GNU
#endif /* __linux__ */

#include <sys/ioctl.h>

#include "adio.h"
#include "ad_tuning.h"

#ifdef HAVE_LUSTRE_LUSTRE_USER_H
#include <lustre/lustre_user.h>
#endif
#ifdef HAVE_LINUX_LUSTRE_LUSTRE_USER_H
#include <linux/lustre/lustre_user.h>
#endif

#ifdef HAVE_LUSTRE_LUSTREAPI_H
#include <lustre/lustreapi.h>
#endif

#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif

#ifdef HAVE_AIO_LITE_H
#include <aio-lite.h>
#else
#ifdef  HAVE_AIO_H
#include <aio.h>
#endif
#ifdef HAVE_SYS_AIO_H
#include <sys/aio.h>
#endif
#endif /* End of HAVE_AIO_LITE_H */

#define LDEBUG(format, ...)				\
do {							\
    fprintf(stderr, "%s(%d): "format,			\
	    __func__, __LINE__, ## __VA_ARGS__);	\
} while (0)

#define DEFAULT_STRIPE_SIZE (1 << 20)

#define GOTO(label, rc)				\
do {							\
    ((void)(rc));					\
    goto label;						\
} while (0)

void ADIOI_LUSTRE_Open(ADIO_File fd, int *error_code);
void ADIOI_LUSTRE_Close(ADIO_File fd, int *error_code);
void ADIOI_LUSTRE_ReadContig(ADIO_File fd, void *buf, int count,
                             MPI_Datatype datatype, int file_ptr_type,
                             ADIO_Offset offset, ADIO_Status * status, int *error_code);
void ADIOI_LUSTRE_WriteContig(ADIO_File fd, const void *buf, int count,
                              MPI_Datatype datatype, int file_ptr_type,
                              ADIO_Offset offset, ADIO_Status * status, int *error_code);
void ADIOI_LUSTRE_WriteStrided(ADIO_File fd, const void *buf, int count,
                               MPI_Datatype datatype, int file_ptr_type,
                               ADIO_Offset offset, ADIO_Status * status, int *error_code);
void ADIOI_LUSTRE_WriteStridedColl(ADIO_File fd, const void *buf, int count,
                                   MPI_Datatype datatype, int file_ptr_type,
                                   ADIO_Offset offset, ADIO_Status * status, int *error_code);
void ADIOI_LUSTRE_ReadStridedColl(ADIO_File fd, void *buf, int count,
                                  MPI_Datatype datatype, int file_ptr_type,
                                  ADIO_Offset offset, ADIO_Status * status, int *error_code);
void ADIOI_LUSTRE_ReadStrided(ADIO_File fd, void *buf, int count,
                              MPI_Datatype datatype, int file_ptr_type,
                              ADIO_Offset offset, ADIO_Status * status, int *error_code);
void ADIOI_LUSTRE_Fcntl(ADIO_File fd, int flag, ADIO_Fcntl_t * fcntl_struct, int *error_code);
void ADIOI_LUSTRE_SetInfo(ADIO_File fd, MPI_Info users_info, int *error_code);

/* the lustre utilities: */
int ADIOI_LUSTRE_Docollect(ADIO_File fd, int contig_access_count,
                           ADIO_Offset * len_list, int nprocs);

void ADIOI_LUSTRE_Get_striping_info(ADIO_File fd, int mode, int **striping_info_ptr);
void ADIOI_LUSTRE_Calc_my_req(ADIO_File fd, ADIO_Offset * offset_list,
                              ADIO_Offset * len_list, int contig_access_count,
                              int *striping_info, int nprocs,
                              int *count_my_req_procs_ptr,
                              int **count_my_req_per_proc_ptr,
                              ADIOI_Access ** my_req_ptr, ADIO_Offset *** buf_idx_ptr);

int ADIOI_LUSTRE_Calc_aggregator(ADIO_File fd, ADIO_Offset off,
                                 int *striping_info, ADIO_Offset * len);

#ifdef HAVE_LUSTRE_COMP_LAYOUT_SUPPORT
int ADIOI_LUSTRE_Parse_comp_layout_opt(char *opt, struct llapi_layout **layout_ptr);

#ifdef HAVE_YAML_SUPPORT
int ADIOI_LUSTRE_Parse_yaml_temp(char *tempfile, struct llapi_layout **layout_ptr);
#endif /* HAVE_YAML_SUPPORT */

#else /* Backward compatibility support */

#define llapi_layout lov_user_md

static inline struct llapi_layout *llapi_layout_alloc()
{
    struct llapi_layout *layout;

    layout = ADIOI_Calloc(1, XATTR_SIZE_MAX);
    layout->lmm_magic = LOV_USER_MAGIC;

    return layout;
}

static inline void llapi_layout_free(struct llapi_layout *layout)
{
    if (layout != NULL)
        ADIOI_Free(layout);
}

static inline int
llapi_layout_file_open(const char *path, int open_flags, mode_t mode,
                       const struct llapi_layout *layout)
{
    int fd;

    if (path == NULL || layout == NULL || (layout != NULL && layout->lmm_magic != LOV_USER_MAGIC)) {
        errno = EINVAL;
        return -1;
    }

    if (layout != NULL && (open_flags & O_CREAT))
        open_flags |= O_LOV_DELAY_CREATE;

    fd = open(path, open_flags, mode);
    if (layout == NULL || fd < 0)
        return fd;

    return ioctl(fd, LL_IOC_LOV_SETSTRIPE, layout);
}

static inline int
llapi_layout_file_create(const char *path, int open_flags, int mode,
                         const struct llapi_layout *layout)
{
    return llapi_layout_file_open(path, open_flags | O_CREAT | O_EXCL, mode, layout);
}

static inline struct llapi_layout *llapi_layout_get_by_fd(int fd, uint32_t flags)
{
    struct llapi_layout *layout;
    int err;

    layout = llapi_layout_alloc();
    if (layout == NULL)
        goto out;

    err = ioctl(fd, LL_IOC_LOV_GETSTRIPE, (void *) layout);
    if (err)
        goto out;

    if (layout->lmm_magic == LOV_USER_MAGIC_V1 || layout->lmm_magic == LOV_USER_MAGIC_V3)
        return layout;
  out:
    if (layout)
        ADIOI_Free(layout);
    return NULL;
}

static inline struct llapi_layout *llapi_layout_get_by_path(const char *path, uint32_t flags)
{
    struct llapi_layout *layout = NULL;
    int fd;
    int tmp;

    fd = open(path, O_RDONLY);
    if (fd < 0)
        return layout;

    layout = llapi_layout_get_by_fd(fd, flags);
    tmp = errno;
    close(fd);
    errno = tmp;

    return layout;
}

static inline int llapi_layout_stripe_count_get(struct llapi_layout *layout, uint64_t * count)
{
    if (layout == NULL)
        return -1;

    *count = layout->lmm_stripe_count;
    return 0;
}

static inline int llapi_layout_stripe_size_get(const struct llapi_layout *layout, uint64_t * size)
{
    if (layout == NULL)
        return -1;

    *size = layout->lmm_stripe_size;
    return 0;
}

static inline int llapi_layout_stripe_size_set(struct llapi_layout *layout, uint64_t size)
{
    if (layout == NULL)
        return -1;

    layout->lmm_stripe_size = size;

    return 0;
}

static inline int llapi_layout_stripe_count_set(struct llapi_layout *layout, uint64_t count)
{
    if (layout == NULL)
        return -1;

    layout->lmm_stripe_count = count;

    return 0;
}

static inline int
llapi_layout_ost_index_set(struct llapi_layout *layout, int stripe_number, uint64_t ost_index)
{
    if (layout == NULL)
        return -1;

    layout->lmm_stripe_offset = ost_index;

    return 0;
}

#endif /* HAVE_LUSTRE_COMP_LAYOUT_SUPPORT */

ADIO_Offset ADIOI_LUSTRE_Get_last_stripe_size(struct llapi_layout * layout);
int ADIOI_LUSTRE_Get_lcm_stripe_count(struct llapi_layout *layout);

#endif /* AD_LUSTRE_H_INCLUDED */
