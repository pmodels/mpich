/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#define _GNU_SOURCE
#include "mpidimpl.h"
#include "ib_module_impl.h"
#include "ib_device.h"
#include "ib_module_cm.h"
#include "ib_utils.h"

#undef FUNCNAME
#define FUNCNAME MPID_nem_ib_module_vc_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)

int MPID_nem_ib_module_vc_init (MPIDI_VC_t *vc, 
        const char *business_card)
{
    int mpi_errno = MPI_SUCCESS;
    int ud_qpn, dlid, out_len, i;
    struct ibv_ah_attr attr;
    uint64_t guid;
    MPID_nem_ib_cm_remote_id_ptr_t remote_info;

    mpi_errno = MPIU_Str_get_int_arg(business_card, 
            MPID_NEM_IB_UD_QPN_KEY, &ud_qpn);
    if (mpi_errno != MPIU_STR_SUCCESS) {
        MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER, "**argstr_hostd");
    }

    VC_FIELD(vc, ud_qpn) = ud_qpn;

    mpi_errno = MPIU_Str_get_int_arg(business_card, 
            MPID_NEM_IB_LID_KEY, &dlid);
    if (mpi_errno != MPIU_STR_SUCCESS) {
        MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER, "**argstr_hostd");
    }

    VC_FIELD(vc, ud_dlid) = dlid;

    mpi_errno = MPIU_Str_get_binary_arg(business_card, 
            MPID_NEM_IB_GUID_KEY, (char *) &guid, sizeof(uint64_t), &out_len);
    if (mpi_errno != MPIU_STR_SUCCESS) {
        MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER, "**argstr_hostd");
    }

    VC_FIELD(vc, node_guid) = guid;
    VC_FIELD(vc, qp) = NULL;
    VC_FIELD(vc, avail_send_wqes) = MPID_nem_ib_dev_param_ptr->max_send_wr;
    VC_FIELD(vc, in_queue) = 0;

    memset(&attr, 0, sizeof(struct ibv_ah_attr));

    attr.is_global = 0;
    attr.dlid = VC_FIELD(vc, ud_dlid);
    attr.sl = 0;
    attr.src_path_bits = 0;
    attr.port_num = MPID_nem_ib_cm_ctxt_ptr->hca_port;
    
    VC_FIELD(vc, ud_ah) = ibv_create_ah 
        (MPID_nem_ib_cm_ctxt_ptr->prot_domain, &attr);

    MPIU_ERR_CHKANDJUMP1(VC_FIELD(vc, ud_ah) == NULL, mpi_errno, MPI_ERR_OTHER, 
            "**ibv_create_ah", "**ibv_create_ah %p", VC_FIELD(vc, ud_ah));


    VC_FIELD(vc, conn_status) = MPID_NEM_IB_CONN_NONE;

    /* Add this information to the hash table */

    remote_info = MPIU_Malloc(sizeof(MPID_nem_ib_cm_remote_id_t));

    if (NULL == remote_info) {
        MPIU_CHKMEM_SETERR (mpi_errno, 
                sizeof(MPID_nem_ib_cm_remote_id_t), 
                "IB Module remote info");
    }

    memset(remote_info, 0, sizeof(MPID_nem_ib_cm_remote_id_t));

    remote_info->guid = guid;
    remote_info->qp_num = ud_qpn;
    remote_info->vc = (void *) vc;

    mpi_errno = MPID_nem_ib_module_insert_hash_elem(
        &MPID_nem_ib_cm_ctxt_ptr->hash_table,
        guid, (void *) remote_info, ud_qpn);

    if(mpi_errno) {
        MPIU_ERR_POP(mpi_errno);
    }

    MPID_nem_ib_module_queue_init(
            (MPID_nem_ib_module_queue_t **)(&VC_FIELD(vc, ib_send_queue)));

    NEM_IB_DBG("VC Init called, vc->pg_rank %d, ud_qpn %u, dlid %u, GUID %lu",
            vc->pg_rank, VC_FIELD(vc, ud_qpn), VC_FIELD(vc, ud_dlid), VC_FIELD(vc, node_guid));

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_ib_module_vc_destroy
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_ib_module_vc_destroy(MPIDI_VC_t *vc)
{
    int mpi_errno = MPI_SUCCESS;

    /* free any resources associated with this VC here */
    
   fn_exit:   
       return mpi_errno;
   fn_fail:
       goto fn_exit;
}
