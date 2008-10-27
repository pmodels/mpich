/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef _IB_MODULE_IMPL_H
#define _IB_MODULE_IMPL_H

#define _GNU_SOURCE
#include "mpid_nem_impl.h"
#include "ib_utils.h"
#include <infiniband/verbs.h>

#define FREE_SEND_QUEUE_ELEMENTS  MPID_NEM_NUM_CELLS

extern MPID_nem_queue_ptr_t MPID_nem_module_ib_recv_queue;
extern MPID_nem_queue_ptr_t MPID_nem_module_ib_free_queue;

extern MPID_nem_queue_ptr_t MPID_nem_process_recv_queue;
extern MPID_nem_queue_ptr_t MPID_nem_process_free_queue;

extern struct ibv_mr *proc_elements_mr;
extern struct ibv_mr *module_elements_mr;

/* The vc provides a generic buffer in which network modules can store
   private fields This removes all dependencies from the VC struction
   on the network module, facilitating dynamic module loading. */
typedef struct 
{
    uint32_t  ud_qpn;
    uint16_t  ud_dlid;
    uint64_t  node_guid;
    struct ibv_ah *ud_ah;
    int conn_status;
    struct ibv_qp *qp;
    struct MPID_nem_ib_module_queue_t *ib_send_queue;
    struct MPID_nem_ib_module_queue_t *ib_recv_queue;
    uint32_t  avail_send_wqes;
    char   in_queue;
} MPID_nem_ib_module_vc_area;

/* accessor macro to private fields in VC */
#define VC_FIELD(vc, field) (((MPID_nem_ib_module_vc_area *)((MPIDI_CH3I_VC *)(vc)->channel_private)->netmod_area.padding)->field)

typedef struct {
    union {
        struct ibv_send_wr s_wr;
        struct ibv_recv_wr r_wr;
    } u;
    struct ibv_sge sg_list;
} MPID_nem_ib_module_descriptor_t;

typedef struct {
    MPID_nem_cell_ptr_t     nem_cell;
    int                     datalen;
    MPID_nem_ib_module_queue_elem_t *qe;
    MPIDI_VC_t              *vc;
    MPID_nem_ib_module_descriptor_t desc;
} MPID_nem_ib_module_cell_elem_t;

typedef struct _ib_cell_pool {
    MPID_nem_ib_module_queue_t          *queue;
    int                                 ncells;
    pthread_spinlock_t                 lock;
} MPID_nem_ib_module_cell_pool_t;

extern MPID_nem_ib_module_cell_pool_t MPID_nem_ib_module_cell_pool;

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
                             int *val_max_sz_p);
int MPID_nem_ib_module_finalize (void);
int MPID_nem_ib_module_poll (MPID_nem_poll_dir_t in_or_out);
int MPID_nem_ib_module_send (MPIDI_VC_t *vc, MPID_nem_cell_ptr_t cell, int datalen);
int MPID_nem_ib_module_get_business_card (int my_rank, char **bc_val_p, int *val_max_sz_p);
int MPID_nem_ib_module_ckpt_shutdown (void);
int MPID_nem_ib_module_connect_to_root (const char *business_card, MPIDI_VC_t *new_vc);
int MPID_nem_ib_module_vc_init (MPIDI_VC_t *vc);
int MPID_nem_ib_module_vc_destroy(MPIDI_VC_t *vc);
int MPID_nem_ib_module_vc_terminate (MPIDI_VC_t *vc);


int MPID_nem_ib_module_init_cell_pool(int n);
int MPID_nem_ib_module_get_cell(
        MPID_nem_ib_module_cell_elem_t **e);
void MPID_nem_ib_module_return_cell(
        MPID_nem_ib_module_cell_elem_t *ce);
void MPID_nem_ib_module_prep_cell_recv(
        MPID_nem_ib_module_cell_elem_t *ce,
        void* buf);
void MPID_nem_ib_module_prep_cell_send(
        MPID_nem_ib_module_cell_elem_t *ce,
        void* buf, uint32_t len);
int MPID_nem_ib_module_add_cells(int n);

#endif /* _IB_MODULE_IMPL_H */
