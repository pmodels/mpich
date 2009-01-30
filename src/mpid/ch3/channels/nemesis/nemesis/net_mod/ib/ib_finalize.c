/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#define _GNU_SOURCE
#include "mpidimpl.h"
#include "ib_impl.h"
#include "ib_device.h"
#include "ib_cm.h"

#undef FUNCNAME
#define FUNCNAME MPID_nem_ib_finalize
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)

int MPID_nem_ib_finalize (void)
{
    int mpi_errno = MPI_SUCCESS;
    int i;


    if(MPID_nem_ib_cm_ctxt_ptr) {

        mpi_errno = MPID_nem_ib_cm_finalize_rc_qps();

        if(mpi_errno) {
            MPIU_ERR_POP(mpi_errno);
        }

        mpi_errno = MPID_nem_ib_cm_finalize_cm();

        if(mpi_errno) {
            MPIU_ERR_POP(mpi_errno);
        }

        MPIU_Free(MPID_nem_ib_cm_ctxt_ptr);
    }

    /* Release all InfiniBand context resources */

    if(MPID_nem_ib_ctxt_ptr) {

        if(MPID_nem_ib_ctxt_ptr->ib_dev) {

            for(i = 0; i < MPID_nem_ib_ctxt_ptr->n_ib_dev; i++) {

                if(IB_PROGRESS_BLOCK == 
                        MPID_nem_ib_dev_param_ptr->progress_type) {
                    mpi_errno = ibv_destroy_comp_channel
                        (MPID_nem_ib_ctxt_ptr->ib_dev[i].comp_channel);
                    if(mpi_errno) {
                        MPIU_ERR_POP(mpi_errno);
                    }
                }

                pthread_cancel(MPID_nem_ib_ctxt_ptr->ib_dev[i].async_thread);

                mpi_errno = ibv_destroy_srq
                    (MPID_nem_ib_ctxt_ptr->ib_dev[i].srq);
                if(mpi_errno) {
                    MPIU_ERR_POP(mpi_errno);
                }

                mpi_errno = ibv_destroy_cq
                    (MPID_nem_ib_ctxt_ptr->ib_dev[i].cq);
                if(mpi_errno) {
                    MPIU_ERR_POP(mpi_errno);
                }

                mpi_errno = ibv_close_device
                    (MPID_nem_ib_ctxt_ptr->ib_dev[i].context);
                if(mpi_errno) {
                    MPIU_ERR_POP(mpi_errno);
                }

                MPIU_Free(MPID_nem_ib_ctxt_ptr->ib_dev[i].ib_port);

            }
            
            MPIU_Free(MPID_nem_ib_ctxt_ptr->ib_dev);
        }
        MPIU_Free(MPID_nem_ib_ctxt_ptr);
    }

    if(NULL != proc_elements_mr) {
        ibv_dereg_mr(proc_elements_mr);
    }

    if(NULL != module_elements_mr) {
        ibv_dereg_mr(module_elements_mr);
    }

    MPID_nem_ib_finalize_cell_pool();

    MPID_nem_ib_queue_finalize(MPID_nem_ib_vc_queue);

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}
