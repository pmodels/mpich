/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#define _GNU_SOURCE
#include "mpidimpl.h"
#include "mpid_nem_impl.h"
#include "ib_device.h"
#include "ib_utils.h"
#include "ib_module_cm.h"
#include "ib_module_priv.h"
#include "ib_module_impl.h"

MPID_nem_netmod_funcs_t MPIDI_nem_ib_module_funcs = {
    MPID_nem_ib_module_init,
    MPID_nem_ib_module_finalize,
    MPID_nem_ib_module_ckpt_shutdown,
    MPID_nem_ib_module_poll,
    MPID_nem_ib_module_send,
    MPID_nem_ib_module_get_business_card,
    MPID_nem_ib_module_connect_to_root,
    MPID_nem_ib_module_vc_init,
    MPID_nem_ib_module_vc_destroy,
    MPID_nem_ib_module_vc_terminate
};


MPID_nem_ib_ctxt_ptr_t  MPID_nem_ib_ctxt_ptr = NULL;
MPID_nem_ib_cm_ctxt_ptr_t MPID_nem_ib_cm_ctxt_ptr = NULL;

MPID_nem_queue_ptr_t MPID_nem_process_recv_queue = 0;
MPID_nem_queue_ptr_t MPID_nem_process_free_queue = 0;

static MPID_nem_queue_t _recv_queue;
static MPID_nem_queue_t _free_queue;

MPID_nem_queue_ptr_t MPID_nem_module_ib_recv_queue = 0;
MPID_nem_queue_ptr_t MPID_nem_module_ib_free_queue = 0;

struct ibv_mr *proc_elements_mr = NULL;
struct ibv_mr *module_elements_mr = NULL;

MPID_nem_ib_module_queue_ptr_t MPID_nem_ib_module_vc_queue = 0;
MPID_nem_ib_module_cell_pool_t MPID_nem_ib_module_cell_pool;

MPID_nem_ib_dev_param_t *MPID_nem_ib_dev_param_ptr = 0;
MPID_nem_ib_cm_param_t *MPID_nem_ib_cm_param_ptr = 0;

/**
 * init_ib_ctxt - Initializes the InfiniBand context
 *  of a process. An InfiniBand context consists of all
 *  possible underlying IB devices and their associated
 *  properties such as protection domains, completion
 *  queues, shared receive queues, etc.
 */

#undef FUNCNAME
#define FUNCNAME init_ib_ctxt
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)

static int init_ib_ctxt(void)
{
    int mpi_errno = MPI_SUCCESS;
    struct ibv_device **dev_list = NULL;
    struct ibv_device *ib_dev;
    int count = 0, i, j, total_ports, curr_port;

    dev_list = ibv_get_device_list(NULL);

    MPIU_ERR_CHKANDJUMP1(dev_list == NULL, mpi_errno, MPI_ERR_OTHER,
            "**ibv_get_device_list", "**ibv_get_device_list %p", dev_list);

    /* Count the number of HCAs */
    while(dev_list[count]) {
        count++;
    }

    if(count <= 0) {
        NEM_IB_ERR("No IB Device Found");
        return MPI_ERR_OTHER;
    }
    
    /* TODO: This is a hack to limit the IB module
     * to use maximum one device for now */
    count = 1;

    MPID_nem_ib_ctxt_ptr = MPIU_Malloc(sizeof(MPID_nem_ib_ctxt_t));

    if(NULL == MPID_nem_ib_ctxt_ptr) {
        MPIU_CHKMEM_SETERR(mpi_errno, sizeof(MPID_nem_ib_ctxt_t), 
                "IB Module Context");
    }

    NEM_IB_DBG("Got %d IB devices", count);

    memset(MPID_nem_ib_ctxt_ptr, 0, sizeof(MPID_nem_ib_ctxt_t));
    

    MPID_nem_ib_ctxt_ptr->ib_dev = 
        MPIU_Malloc( sizeof(MPID_nem_ib_device_t) * count);

    memset(MPID_nem_ib_ctxt_ptr->ib_dev, 0,
            sizeof(MPID_nem_ib_device_t) * count);

    for(i = 0; i < count; i++) {

        MPID_nem_ib_ctxt_ptr->ib_dev[i].nic = dev_list[i];

        mpi_errno = MPID_nem_ib_module_open_hca(dev_list[i], 
                &MPID_nem_ib_ctxt_ptr->ib_dev[i].context);

        if(mpi_errno) {
            MPIU_ERR_POP(mpi_errno);
        }

        mpi_errno = MPID_nem_ib_module_alloc_pd
            (MPID_nem_ib_ctxt_ptr->ib_dev[i].context,
                &MPID_nem_ib_ctxt_ptr->ib_dev[i].prot_domain, dev_list[i]);

        if(mpi_errno) {
            MPIU_ERR_POP(mpi_errno);
        }

        mpi_errno = MPID_nem_ib_module_get_dev_attr
            (MPID_nem_ib_ctxt_ptr->ib_dev[i].context,
                &MPID_nem_ib_ctxt_ptr->ib_dev[i].dev_attr, dev_list[i]);

        if(mpi_errno) {
            MPIU_ERR_POP(mpi_errno);
        }

        MPID_nem_ib_ctxt_ptr->ib_dev[i].dev_attr.node_guid = 
            ibv_get_device_guid(dev_list[i]);

        total_ports = MPID_nem_ib_ctxt_ptr->ib_dev[i].dev_attr.phys_port_cnt;

        /* Find out number of active ports */
        MPID_nem_ib_ctxt_ptr->ib_dev[i].n_active_ports = 0;

        for(j = 1; j <= total_ports; j++) {

            if(MPID_nem_ib_module_is_port_active(
                        MPID_nem_ib_ctxt_ptr->ib_dev[i].context, j)) {
                MPID_nem_ib_ctxt_ptr->ib_dev[i].n_active_ports++;
            }
        }

        NEM_IB_DBG("Found %d ACTIVE out of %d ports",
                MPID_nem_ib_ctxt_ptr->ib_dev[i].n_active_ports,
                total_ports);

        /* Allocate space for port structures */
        MPID_nem_ib_ctxt_ptr->ib_dev[i].ib_port = MPIU_Malloc
            (sizeof(MPID_nem_ib_port_t) * 
             MPID_nem_ib_ctxt_ptr->ib_dev[i].n_active_ports);

        if(NULL == MPID_nem_ib_ctxt_ptr->ib_dev[i].ib_port) {
            MPIU_CHKMEM_SETERR(mpi_errno, (sizeof(MPID_nem_ib_port_t) *
                    MPID_nem_ib_ctxt_ptr->ib_dev[i].n_active_ports), 
                    "IB Port Structures");
        }

        memset(MPID_nem_ib_ctxt_ptr->ib_dev[i].ib_port, 0,
                sizeof(MPID_nem_ib_port_t) *
                MPID_nem_ib_ctxt_ptr->ib_dev[i].n_active_ports);

        /* Open the "right" ports */

        curr_port = 0;

        for(j = 1; j <= total_ports; j++) {

            if(MPID_nem_ib_module_is_port_active(
                        MPID_nem_ib_ctxt_ptr->ib_dev[i].context, j)) {

                NEM_IB_DBG("Opening port %u", j);

                MPID_nem_ib_module_get_port_prop(
                        MPID_nem_ib_ctxt_ptr->ib_dev[i].context, 
                        (uint8_t) j,
                        &(MPID_nem_ib_ctxt_ptr->
                            ib_dev[i].ib_port[curr_port].port_attr));
                MPID_nem_ib_ctxt_ptr->
                    ib_dev[i].ib_port[curr_port].port_num = j;

                curr_port++;

                if(curr_port >= 
                        MPID_nem_ib_ctxt_ptr->
                        ib_dev[i].n_active_ports) {
                    break;
                }
            }
        }

        mpi_errno = MPID_nem_ib_module_create_cq
            (MPID_nem_ib_ctxt_ptr->ib_dev[i].context,
                &MPID_nem_ib_ctxt_ptr->ib_dev[i].cq, 
                &MPID_nem_ib_ctxt_ptr->ib_dev[i].comp_channel,
                MPID_nem_ib_dev_param_ptr->max_num_cqe,
                MPID_nem_ib_dev_param_ptr->progress_type,
                dev_list[i]);

        if(mpi_errno) {
            MPIU_ERR_POP(mpi_errno);
        }

        mpi_errno = MPID_nem_ib_module_create_srq
            (MPID_nem_ib_ctxt_ptr->ib_dev[i].context,
                MPID_nem_ib_ctxt_ptr->ib_dev[i].prot_domain,
                &MPID_nem_ib_ctxt_ptr->ib_dev[i].srq, 
                MPID_nem_ib_dev_param_ptr->max_srq_wr,
                MPID_nem_ib_dev_param_ptr->max_srq_recv_sge,
                MPID_nem_ib_dev_param_ptr->srq_limit,
                dev_list[i]);

        if(mpi_errno) {
            MPIU_ERR_POP(mpi_errno);
        }

        MPID_nem_ib_ctxt_ptr->ib_dev[i].srq_n_posted = 0;

        pthread_spin_init(
                &MPID_nem_ib_ctxt_ptr->ib_dev[i].srq_post_lock, 0);

        MPID_nem_ib_ctxt_ptr->n_ib_dev++;

        if(MPID_nem_ib_dev_param_ptr->max_hcas) {
            if(MPID_nem_ib_ctxt_ptr->n_ib_dev >=
                    MPID_nem_ib_dev_param_ptr->max_hcas) {
                break;
            }
        }
    }


fn_exit:
    ibv_free_device_list(dev_list);
    return mpi_errno;
fn_fail:
    if(MPID_nem_ib_ctxt_ptr) {
        if(MPID_nem_ib_ctxt_ptr->ib_dev) {
            MPIU_Free(MPID_nem_ib_ctxt_ptr->ib_dev);
        }
        MPIU_Free(MPID_nem_ib_ctxt_ptr);
    }
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME init_ib_cm_ctxt
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)

static int init_ib_cm_ctxt(MPID_nem_ib_device_ptr_t dev_ptr,
        MPIDI_PG_t *pg_p, 
        int pg_rank)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_nem_ib_cm_ctxt_ptr_t ctxt_ptr;
    struct ibv_qp_init_attr init_attr;
    struct ibv_qp_attr attr;

    MPID_nem_ib_cm_ctxt_ptr = 
        MPIU_Malloc(sizeof(MPID_nem_ib_cm_ctxt_t));

    if (NULL == MPID_nem_ib_cm_ctxt_ptr) {
        MPIU_CHKMEM_SETERR (mpi_errno, 
                sizeof (MPID_nem_ib_cm_ctxt_t), 
                "IB Module CM structure");

    }

    ctxt_ptr = MPID_nem_ib_cm_ctxt_ptr;

    memset(ctxt_ptr, 0, sizeof(MPID_nem_ib_cm_ctxt_t));

    dev_ptr = &MPID_nem_ib_ctxt_ptr->ib_dev[0];
    
    ctxt_ptr->rank = pg_rank;

    ctxt_ptr->context = dev_ptr->context;
    ctxt_ptr->prot_domain = dev_ptr->prot_domain;
    ctxt_ptr->nic = dev_ptr->nic;
    ctxt_ptr->hca_port = dev_ptr->ib_port[0].port_num;
    ctxt_ptr->guid = dev_ptr->dev_attr.node_guid;

    /* TODO: Which HCA device should new QPs be
     * connected to? For now, connect all QPs
     * to the first port of the first HCA
     */

    /* QP Create */
    memset(&init_attr, 0, sizeof(struct ibv_qp_init_attr));
    init_attr.send_cq = dev_ptr->cq;
    init_attr.recv_cq = dev_ptr->cq;
    init_attr.srq     = dev_ptr->srq;
    init_attr.cap.max_recv_wr = 0;
    init_attr.cap.max_send_wr = 
        MPID_nem_ib_dev_param_ptr->max_send_wr;
    init_attr.cap.max_send_sge = 
        MPID_nem_ib_dev_param_ptr->max_send_sge;
    init_attr.cap.max_recv_sge = 
        MPID_nem_ib_dev_param_ptr->max_srq_recv_sge;
    init_attr.cap.max_inline_data = 
        MPID_nem_ib_dev_param_ptr->max_inline_size;
    init_attr.qp_type = IBV_QPT_RC;

    ctxt_ptr->rc_qp_init_attr = init_attr;

    /* QP -> Init */

    memset(&attr, 0, sizeof(struct ibv_qp_attr));
    attr.qp_state = IBV_QPS_INIT;
    attr.pkey_index = MPID_nem_ib_dev_param_ptr->pkey_ix;
    attr.port_num = 1;
    attr.qp_access_flags = IBV_ACCESS_REMOTE_WRITE |
        IBV_ACCESS_REMOTE_READ;

    ctxt_ptr->rc_qp_attr_to_init = attr;

    ctxt_ptr->rc_qp_mask_to_init = IBV_QP_STATE |
        IBV_QP_PKEY_INDEX | IBV_QP_PORT | IBV_QP_ACCESS_FLAGS;


    /* Init -> RTR */

    memset(&attr, 0, sizeof(struct ibv_qp_attr));
    attr.qp_state = IBV_QPS_RTR;
    attr.path_mtu = MPID_nem_ib_dev_param_ptr->path_mtu;
    attr.max_dest_rd_atomic = 
        MPID_nem_ib_dev_param_ptr->max_dst_rd_atomic;
    attr.min_rnr_timer = MPID_nem_ib_dev_param_ptr->min_rnr_timer;
    attr.ah_attr.sl = MPID_nem_ib_dev_param_ptr->sl;
    attr.ah_attr.src_path_bits = 
        MPID_nem_ib_dev_param_ptr->src_path_bits;
    attr.ah_attr.port_num = 1;
    attr.ah_attr.static_rate = MPID_nem_ib_dev_param_ptr->static_rate;

    ctxt_ptr->rc_qp_attr_to_rtr = attr;

    ctxt_ptr->rc_qp_mask_to_rtr = IBV_QP_STATE |
        IBV_QP_AV |
        IBV_QP_PATH_MTU |
        IBV_QP_DEST_QPN |
        IBV_QP_RQ_PSN |
        IBV_QP_MAX_DEST_RD_ATOMIC | IBV_QP_MIN_RNR_TIMER;

    /* RTR -> RTS */

    memset(&attr, 0, sizeof(struct ibv_qp_attr));
    attr.qp_state = IBV_QPS_RTS;
    attr.timeout = MPID_nem_ib_dev_param_ptr->timeout;
    attr.retry_cnt = MPID_nem_ib_dev_param_ptr->retry_cnt;
    attr.rnr_retry = MPID_nem_ib_dev_param_ptr->rnr_retry;
    attr.sq_psn = MPID_nem_ib_dev_param_ptr->sq_psn;
    attr.max_rd_atomic = MPID_nem_ib_dev_param_ptr->max_rd_atomic;

    ctxt_ptr->rc_qp_attr_to_rts = attr;
    ctxt_ptr->rc_qp_mask_to_rts = IBV_QP_STATE |
        IBV_QP_TIMEOUT |
        IBV_QP_RETRY_CNT |
        IBV_QP_RNR_RETRY | IBV_QP_SQ_PSN | IBV_QP_MAX_QP_RD_ATOMIC;

    mpi_errno = MPID_nem_ib_cm_init_cm();

    if(mpi_errno) {
        MPIU_ERR_POP(mpi_errno);
    }

fn_exit:
    return mpi_errno;
fn_fail:
    if(ctxt_ptr) {
        MPIU_Free(ctxt_ptr);
    }
    goto fn_exit;
}

/**
 * MPID_nem_ib_module_init - Initialize the Nemesis IB module
 *
 * @proc_recv_queue: main recv queue for the process
 * @proc_free_queue: main free queue for the process
 * @proc_elements: pointer to process' queue elements
 * @num_proc_elements: number of process' queue elements
 * @module_elements: pointer to the queue elements used
 *  by this module
 * @num_module_elements: number of queue elements for use
 *  by this module
 * @module_recv_queue: pointer to recv queue for this
 *  module. The process will add elements to this
 *  queue for the module to send
 * @module_free_queue: pointer to the free queue for 
 *  this module. The process will return elements to
 *  this queue
 * @ckpt_restart: true if this is a restart from a 
 *  checkpoint. In a restart, the network needs to
 *  be brought up again, but we want to keep things 
 *  like sequence numbers.
 * @pg_p: MPICH2 process group pointer
 * @pg_rank: Rank in the process group
 * @bc_val_p: Pointer to business card pointer
 * @val_max_sz_p: Pointer to max. size
 */

#undef FUNCNAME
#define FUNCNAME MPID_nem_ib_module_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)

int MPID_nem_ib_module_init (MPID_nem_queue_ptr_t proc_recv_queue, 
        MPID_nem_queue_ptr_t proc_free_queue, 
        MPID_nem_cell_ptr_t proc_elements,
        int num_proc_elements, 
        MPID_nem_cell_ptr_t module_elements, 
        int num_module_elements,
        MPID_nem_queue_ptr_t *module_free_queue, 
        int ckpt_restart,
        MPIDI_PG_t *pg_p, 
        int pg_rank, 
        char **bc_val_p, 
        int *val_max_sz_p)
{
    int mpi_errno = MPI_SUCCESS;
    int ret, i;

    /* first make sure that our private fields in the vc fit into the area provided  */
    MPIU_Assert(sizeof(MPID_nem_ib_module_vc_area) <= MPID_NEM_VC_NETMOD_AREA_LEN);
    
    INIT_NEM_IB_PROC_DESC(pg_rank);

    mpi_errno = MPID_nem_ib_module_init_cm_param();
    if(mpi_errno) {
        MPIU_ERR_POP(mpi_errno);
    }

    mpi_errno = MPID_nem_ib_module_init_dev_param();
    if(mpi_errno) {
        MPIU_ERR_POP(mpi_errno);
    }

    mpi_errno = init_ib_ctxt();
    if(mpi_errno) {
        MPIU_ERR_POP(mpi_errno);
    }

    /* Use the first available IB device to do
     * connection management
     * TODO: Define a user variable to control
     * which HCA to use for conn. management
     */

    mpi_errno = init_ib_cm_ctxt(&MPID_nem_ib_ctxt_ptr->ib_dev[0],
            pg_p, pg_rank);
    if(mpi_errno) {
        MPIU_ERR_POP(mpi_errno);
    }

    mpi_errno = MPID_nem_ib_module_get_business_card
        (pg_rank, bc_val_p, val_max_sz_p);
    if(mpi_errno) {
        MPIU_ERR_POP(mpi_errno);
    }

    /* Record the process receive and free queues */
    MPID_nem_process_recv_queue = proc_recv_queue;
    MPID_nem_process_free_queue = proc_free_queue;

    /* Register the process and module elements */

    ret = MPID_nem_ib_module_reg_mem
        (MPID_nem_ib_ctxt_ptr->ib_dev[0].prot_domain,
         (void *)proc_elements,
         sizeof (MPID_nem_cell_t) * num_proc_elements,
         MPID_NEM_IB_BUF_READ_WRITE,
         &proc_elements_mr);

    MPIU_ERR_CHKANDJUMP1(ret != 0, mpi_errno, MPI_ERR_OTHER, 
            "**ibv_reg_mr", "**ibv_reg_mr %d", ret);

    ret = MPID_nem_ib_module_reg_mem
        (MPID_nem_ib_ctxt_ptr->ib_dev[0].prot_domain,
         (void *)module_elements,
         sizeof (MPID_nem_cell_t) * num_module_elements,
         MPID_NEM_IB_BUF_READ_WRITE,
         &module_elements_mr);

    MPIU_ERR_CHKANDJUMP1(ret != 0, mpi_errno, MPI_ERR_OTHER, 
            "**ibv_reg_mr", "**ibv_reg_mr %d", ret);

    /* Initialize the network module Queues */
    MPID_nem_module_ib_recv_queue = &_recv_queue;
    MPID_nem_module_ib_free_queue = &_free_queue;

    MPID_nem_queue_init(MPID_nem_module_ib_recv_queue);
    MPID_nem_queue_init(MPID_nem_module_ib_free_queue);

    mpi_errno = MPID_nem_ib_module_init_cell_pool(
            MPID_nem_ib_dev_param_ptr->max_cell_elem);

    if(mpi_errno) {
        MPIU_ERR_POP(mpi_errno);
    }

    /* Enqueue all network module elements into
     * the free queue */
    for(i = 0; i < num_module_elements; i++) {
        MPID_nem_queue_enqueue(MPID_nem_module_ib_free_queue, 
                &module_elements[i]);
    }

    /* Dequeue all elements and post them to the network */

    while(!MPID_nem_queue_empty(MPID_nem_module_ib_free_queue)
            &&  (MPID_nem_ib_ctxt_ptr->ib_dev[0].srq_n_posted <
            MPID_nem_ib_dev_param_ptr->max_srq_wr)) {

        MPID_nem_cell_ptr_t c;
        MPID_nem_ib_module_cell_elem_t *ce;

        MPID_nem_queue_dequeue(MPID_nem_module_ib_free_queue, &c);

        mpi_errno = MPID_nem_ib_module_get_cell(&ce);
        if(mpi_errno) {
            MPIU_ERR_POP(mpi_errno);
        }

        ce->nem_cell = c;

        MPID_nem_ib_module_prep_cell_recv(ce, (void *) MPID_NEM_CELL_TO_PACKET(c));

        ret = MPID_nem_ib_module_post_srq(
                MPID_nem_ib_ctxt_ptr->ib_dev[0].srq,
                &ce->desc.u.r_wr);

        MPIU_ERR_CHKANDJUMP1(ret != 0, mpi_errno, MPI_ERR_OTHER, 
                "**ibv_post_srq_recv", "**ibv_post_srq_recv %d", ret);

        MPID_nem_ib_ctxt_ptr->ib_dev[0].srq_n_posted++;
    }

    /* Enable watch for SRQ events */

    ret = MPID_nem_ib_module_modify_srq(
            MPID_nem_ib_ctxt_ptr->ib_dev[0].srq,
            MPID_nem_ib_dev_param_ptr->max_srq_wr,
            MPID_nem_ib_dev_param_ptr->srq_limit);

    MPIU_ERR_CHKANDJUMP1(ret != 0, mpi_errno, MPI_ERR_OTHER, 
            "**ibv_modify_srq", "**ibv_modify_srq %d", ret);

    pthread_create(&MPID_nem_ib_ctxt_ptr->ib_dev[0].async_thread,
            NULL, (void *) async_thread, 
            MPID_nem_ib_ctxt_ptr->ib_dev[0].context);

    *module_free_queue = MPID_nem_module_ib_free_queue;

    MPID_nem_ib_module_queue_init(&MPID_nem_ib_module_vc_queue);

    for(i = 0; i < MPID_nem_ib_dev_param_ptr->max_vc_queue; i++) {

        MPID_nem_ib_module_queue_elem_t *e;

        mpi_errno = MPID_nem_ib_module_queue_new_elem(&e, NULL);
        if(mpi_errno) {
            MPIU_ERR_POP(mpi_errno);
        }
            
        MPID_nem_ib_module_queue_free(MPID_nem_ib_module_vc_queue, e);
    }

    NEM_IB_DBG("End of ib_module_init module elem %d, proc elem %d, "
            "posted SRQ %d\n",
            num_module_elements, 
            num_proc_elements,
            MPID_nem_ib_ctxt_ptr->ib_dev[0].srq_n_posted);
fn_exit:
    return mpi_errno;
fn_fail:
    if(MPID_nem_ib_ctxt_ptr) {
        MPIU_Free(MPID_nem_ib_ctxt_ptr);
    }
    goto fn_exit;
}
