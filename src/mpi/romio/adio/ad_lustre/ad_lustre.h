/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *   Copyright (C) 1997 University of Chicago.
 *   See COPYRIGHT notice in top-level directory.
 *
 *   Copyright (C) 2007 Oak Ridge National Laboratory
 *
 *   Copyright (C) 2008 Sun Microsystems, Lustre group
 */

#ifndef AD_LUSTRE_H_INCLUDED
#define AD_LUSTRE_H_INCLUDED

/* temp*/
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

typedef struct {
    ADIO_Offset fd_size;        /* size of coalesced file domains */
    ADIO_Offset AAR_st_offset;  /* min offset of aggregate access region */
    ADIO_Offset AAR_end_offset; /* max offset of aggregate access region */
    int AAR_st_rank;            /* start process rank of AAR */
    int AAR_end_rank;           /*end process rank of AAR */
    int MAR_num_stripes;        /* number of stripes in my access region */
    int num_stripes_per_group;  /* AAR stripes / striping_factor */
    int pivot;
    int rem_groups;             /*number of groups in last (AAR_num_stripes % cb_nodes) */
    ADIO_Offset *starts;        /* [cb_nodes] start offsets of all domains */
    ADIO_Offset *ends;          /* [cb_nodes] end offsets of all domains */
    int *num_stripes;           /* [cb_nodes] number of stripes in each domains */
    int *ntimes;                /* [cb_nodes] number of two-phase I/O */
    ADIO_Offset **offsets;      /* [cb_nodes][ntimes] offset of each phase */
    ADIO_Offset **sizes;        /* [cb_nodes][ntimes] size of each phase, non-coalesced */
    int my_agg_rank;
    int pipelining;             /* pipelining flag */
} ADIOI_LUSTRE_file_domain;

/* structure for storing access info of this process's request
   to the file domains on all other processes, and vice-versa. used
   as array of structures indexed by process number. */
typedef struct {
    ADIO_Offset start_offset;   /* my start offset */
    ADIO_Offset end_offset;     /* my end offset */
    int contig_access_count;
    ADIO_Offset *offset_list;   /* [contig_access_count] */
    ADIO_Offset *len_list;      /* [contig_access_count] */
    int buftype_is_contig;
    int *buf_idx;               /* [nprocs] 4-byte integer assuming the buffer size < 2GB */
    ADIO_Offset buf_size;
} ADIOI_LUSTRE_Access;

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

void ADIOI_LUSTRE_Get_striping_info(ADIO_File fd, int **striping_info_ptr, int mode);
void ADIOI_LUSTRE_Calc_my_req(ADIO_File fd, ADIO_Offset * offset_list,
                              ADIO_Offset * len_list, int contig_access_count,
                              int *striping_info, int nprocs,
                              int *count_my_req_procs_ptr,
                              int **count_my_req_per_proc_ptr,
                              ADIOI_LUSTRE_file_domain * file_domain,
                              ADIOI_Access ** my_req_ptr, ADIO_Offset *** buf_idx_ptr);

void ADIOI_LUSTRE_Calc_my_off_len(ADIO_File fd,
                                  int bufcount,
                                  MPI_Datatype datatype,
                                  int file_ptr_type,
                                  ADIO_Offset offset, ADIOI_LUSTRE_Access * my_access);

void ADIOI_LUSTRE_Calc_file_domains(ADIO_File fd,
                                    int nprocs,
                                    int myrank,
                                    ADIOI_LUSTRE_Access * my_access,
                                    ADIOI_LUSTRE_file_domain * file_domain);

void ADIOI_LUSTRE_Calc_others_req(ADIO_File fd,
                                  int nprocs,
                                  int myrank,
                                  int *count_my_req_per_proc,
                                  ADIOI_Access * my_req, ADIOI_Access ** others_req_ptr);

int ADIOI_LUSTRE_Calc_aggregator(ADIO_File fd, ADIO_Offset off,
                                 ADIOI_LUSTRE_file_domain * file_domain,
                                 ADIO_Offset * len, int *striping_info);
#endif /* AD_LUSTRE_H_INCLUDED */
