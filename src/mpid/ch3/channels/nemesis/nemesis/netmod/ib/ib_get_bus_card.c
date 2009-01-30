/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include <stdio.h>

#include "mpidimpl.h"
#include "ib_impl.h"
#include "ib_utils.h"
#include "ib_cm.h"


#undef FUNCNAME
#define FUNCNAME MPID_nem_ib_get_business_card
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)

int MPID_nem_ib_get_business_card (int my_rank, char **bc_val_p, 
        int *val_max_sz_p)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIU_Str_add_int_arg(bc_val_p, val_max_sz_p,
            MPID_NEM_IB_UD_QPN_KEY, 
            MPID_nem_ib_cm_ctxt_ptr->ud_qp->qp_num);

    if(mpi_errno != MPIU_STR_SUCCESS) {

        if(mpi_errno == MPIU_STR_NOMEM) {
            MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**buscard_len");
        } else {  
            MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**buscard");
        }
        return mpi_errno;
    }

    mpi_errno = MPIU_Str_add_int_arg(bc_val_p, val_max_sz_p,
            MPID_NEM_IB_LID_KEY, 
            MPID_nem_ib_cm_ctxt_ptr->port_attr.lid);

    if(mpi_errno != MPIU_STR_SUCCESS) {

        if(mpi_errno == MPIU_STR_NOMEM) {
            MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**buscard_len");
        } else {  
            MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**buscard");
        }
        return mpi_errno;
    }

    mpi_errno = MPIU_Str_add_binary_arg(bc_val_p, val_max_sz_p,
            MPID_NEM_IB_GUID_KEY, 
            (char *) &MPID_nem_ib_cm_ctxt_ptr->guid, 
            sizeof(uint64_t));

    if(mpi_errno != MPIU_STR_SUCCESS) {

        if(mpi_errno == MPIU_STR_NOMEM) {
            MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**buscard_len");
        } else {  
            MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**buscard");
        }
        return mpi_errno;
    }

    NEM_IB_DBG("Get bus card: val_max_sz %d, "
            "ud_qpn %u, lid %u, guid %lu",
             *val_max_sz_p, MPID_nem_ib_cm_ctxt_ptr->ud_qp->qp_num,
             MPID_nem_ib_cm_ctxt_ptr->port_attr.lid,
             MPID_nem_ib_cm_ctxt_ptr->guid);

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}
