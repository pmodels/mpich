/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef IBIMPL_H
#define IBIMPL_H

#include "mm_ib.h"

#include "iba.h"

#define IB_SUCCESS 0
#define IB_FAILURE 1

#define IB_EAGER_LIMIT            (1024 * 1024 * 6)
#define IB_ERROR_MSG_LENGTH       256
#define IB_LISTENER_POINTER       &IB_Process
//#define IB_PINNED_MEMORY_SIZE  2*1024*1024
#define IB_PACKET_SIZE            (1024 * 64)
#define IB_PACKET_COUNT           64
#define IB_NUM_PREPOSTED_RECEIVES 32
#define IB_MAX_CQ_ENTRIES         255

typedef struct IB_PerProcess {
    MPID_Thread_lock_t lock;
       ib_hca_handle_t hca_handle;
        ib_pd_handle_t pd_handle;
       ib_cqd_handle_t cqd_handle;
        ib_cq_handle_t cq_handle;
                   int lid;
       ib_hca_attr_t * attr_p;
		   int error;
		  char err_msg[IB_ERROR_MSG_LENGTH];
} IB_PerProcess;

extern IB_PerProcess IB_Process;

int ib_handle_read(MPIDI_VC *vc_ptr, void *mem_ptr, int num_bytes);
int ib_handle_read_data(MPIDI_VC *vc_ptr, void * mem_ptr, int num_read);
int ib_handle_written(MPIDI_VC *vc_ptr, void *mem_ptr, int num_bytes);
int ib_setup_connections();
int ib_read_data(MPIDI_VC *vc_ptr);
int ib_write_aggressive(MPIDI_VC *vc_ptr);
int ibr_post_receive(MPIDI_VC *vc_ptr);
int ibr_post_write(MPIDI_VC *vc_ptr, void *buf, int len, int (*write_progress_update)(int, void*));
int ibr_post_writev(MPIDI_VC *vc_ptr, MPID_IOV *iov, int n, int (*write_progress_update)(int, void*));
int ibr_next_num_written();

#endif
