/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#define _GNU_SOURCE
#include "mpidimpl.h"
#include "ib_device.h"
#include "ib_module_cm.h"

int MPID_nem_ib_module_init_dev_param();
int MPID_nem_ib_module_init_cm_param();

#undef FUNCNAME
#define FUNCNAME MPID_nem_ib_module_init_dev_param
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)

int MPID_nem_ib_module_init_dev_param()
{
    int mpi_errno = MPI_SUCCESS;

    MPID_nem_ib_dev_param_ptr =
        MPIU_Malloc(sizeof(MPID_nem_ib_dev_param_t));

    if(NULL == MPID_nem_ib_dev_param_ptr) {
        MPIU_CHKMEM_SETERR(mpi_errno, sizeof(MPID_nem_ib_dev_param_t), 
                "IB dev paramaters");
    }

    memset(MPID_nem_ib_dev_param_ptr, 0, 
            sizeof(MPID_nem_ib_dev_param_t));

    MPID_nem_ib_dev_param_ptr->max_num_cqe = 40000;
    MPID_nem_ib_dev_param_ptr->max_hcas = 0;
    MPID_nem_ib_dev_param_ptr->progress_type = IB_PROGRESS_POLL;
    MPID_nem_ib_dev_param_ptr->max_send_sge = 1;
    MPID_nem_ib_dev_param_ptr->max_srq_recv_sge = 1;
    MPID_nem_ib_dev_param_ptr->max_srq_wr = 512;
    MPID_nem_ib_dev_param_ptr->srq_limit = 30;
    MPID_nem_ib_dev_param_ptr->srq_n_preserve = 100;
    MPID_nem_ib_dev_param_ptr->max_send_wr = 32;
    MPID_nem_ib_dev_param_ptr->max_inline_size = 128;
    MPID_nem_ib_dev_param_ptr->path_mtu = IBV_MTU_2048;
    MPID_nem_ib_dev_param_ptr->max_dst_rd_atomic = 4;
    MPID_nem_ib_dev_param_ptr->min_rnr_timer = 12;
    MPID_nem_ib_dev_param_ptr->sl = 0;
    MPID_nem_ib_dev_param_ptr->src_path_bits = 0;
    MPID_nem_ib_dev_param_ptr->static_rate = 0;
    MPID_nem_ib_dev_param_ptr->timeout = 10;
    MPID_nem_ib_dev_param_ptr->retry_cnt = 7;
    MPID_nem_ib_dev_param_ptr->rnr_retry = 7;
    MPID_nem_ib_dev_param_ptr->sq_psn = 0;
    MPID_nem_ib_dev_param_ptr->max_rd_atomic = 8;
    MPID_nem_ib_dev_param_ptr->max_cell_elem = 1024;
    MPID_nem_ib_dev_param_ptr->sec_pool_size = 128;
    MPID_nem_ib_dev_param_ptr->max_vc_queue = 32;

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPID_nem_ib_module_init_cm_param
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)

int MPID_nem_ib_module_init_cm_param()
{
    int mpi_errno = MPI_SUCCESS;

    MPID_nem_ib_cm_param_ptr = 
        MPIU_Malloc(sizeof(MPID_nem_ib_cm_param_t));

    if(NULL == MPID_nem_ib_cm_param_ptr) {
        MPIU_CHKMEM_SETERR(mpi_errno, sizeof(MPID_nem_ib_cm_param_t), 
                "IB CM paramaters");
    }

    memset(MPID_nem_ib_cm_param_ptr, 0, 
            sizeof(MPID_nem_ib_cm_param_t));

    MPID_nem_ib_cm_param_ptr->cm_n_buf = 1024;
    MPID_nem_ib_cm_param_ptr->cm_max_send = 32;
    MPID_nem_ib_cm_param_ptr->cm_max_spin_count = 10000;
    MPID_nem_ib_cm_param_ptr->cm_hash_size = 1024;
    MPID_nem_ib_cm_param_ptr->ud_overhead = 40;
    MPID_nem_ib_cm_param_ptr->cm_timeout_ms = 1;

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}


#if 0
int MPID_nem_ib_module_init_param(MPID_nem_ib_device_ptr_t dev)
{
    int mpi_errno = MPI_SUCCESS;

    dev->param = MPIU_Malloc(sizeof(MPID_nem_ib_param_t));
    if(NULL == dev->param) {
        MPIU_CHKMEM_SETERR(mpi_errno, sizeof(MPID_nem_ib_param_t), 
                "IB Module paramaters");
    }

    memset(dev->param, 0, sizeof(MPID_nem_ib_param_t));

    dev->param->max_num_cqe = 40000;
    dev->param->max_hcas = 1;
    dev->param->progress_type = IB_PROGRESS_POLL;
    dev->param->max_send_sge = 20;
    dev->param->max_srq_recv_sge = 1;
    dev->param->max_srq_wr = 512;
    dev->param->srq_limit = 30;
    dev->param->srq_n_preserve = 50;
    dev->param->cm_n_buf = 1024;
    dev->param->cm_max_send = 32;
    dev->param->cm_max_spin_count = 10000;
    dev->param->max_send_wr = 128;
    dev->param->max_inline_size = 128;
    dev->param->path_mtu = IBV_MTU_2048;
    dev->param->max_dst_rd_atomic = 4;
    dev->param->min_rnr_timer = 12;
    dev->param->sl = 0;
    dev->param->src_path_bits = 0;
    dev->param->static_rate = 0;
    dev->param->timeout = 10;
    dev->param->retry_cnt = 7;
    dev->param->rnr_retry = 7;
    dev->param->sq_psn = 0;
    dev->param->max_rd_atomic = 8;
    dev->param->ud_overhead = 40;
    dev->param->cm_hash_size = 1024;
    dev->param->cm_timeout_ms = 1;

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}
#endif
