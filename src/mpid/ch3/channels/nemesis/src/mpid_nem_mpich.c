/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpid_nem_impl.h"
#include "mpid_nem_fbox.h"
#include "mpid_nem_nets.h"

#define DO_PAPI(x)  /*x */
#define DO_PAPI2(x) /*x */
#define DO_PAPI3(x) /*x */

MPID_nem_fboxq_elem_t *MPID_nem_fboxq_head = 0;
MPID_nem_fboxq_elem_t *MPID_nem_fboxq_tail = 0;
MPID_nem_fboxq_elem_t *MPID_nem_fboxq_elem_list = 0;
MPID_nem_fboxq_elem_t *MPID_nem_fboxq_elem_list_last = 0;
MPID_nem_fboxq_elem_t *MPID_nem_curr_fboxq_elem = 0;
MPID_nem_fboxq_elem_t *MPID_nem_curr_fbox_all_poll = 0;

MPID_nem_cell_ptr_t MPID_nem_prefetched_cell = 0;

unsigned short *MPID_nem_recv_seqno = 0;

#undef FUNCNAME
#define FUNCNAME MPID_nem_mpich_init
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int
MPID_nem_mpich_init(void)
{
    int mpi_errno = MPI_SUCCESS;
    int i;
    MPIU_CHKPMEM_DECL (2);
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_MPICH_INIT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_MPICH_INIT);

    MPID_nem_prefetched_cell = NULL;

    MPIU_CHKPMEM_MALLOC (MPID_nem_recv_seqno, unsigned short *, sizeof(*MPID_nem_recv_seqno) * MPID_nem_mem_region.num_procs, mpi_errno, "recv seqno");

    for (i = 0; i < MPID_nem_mem_region.num_procs; ++i)
    {
        MPID_nem_recv_seqno[i] = 0;
    }

    /* set up fbox queue */
    MPIU_CHKPMEM_MALLOC (MPID_nem_fboxq_elem_list, MPID_nem_fboxq_elem_t *, MPID_nem_mem_region.num_local * sizeof(MPID_nem_fboxq_elem_t), mpi_errno, "fastbox element list");

    for (i = 0; i < MPID_nem_mem_region.num_local; ++i)
    {
        MPID_nem_fboxq_elem_list[i].usage = 0;
        MPID_nem_fboxq_elem_list[i].prev = NULL;
        MPID_nem_fboxq_elem_list[i].next = NULL;
        MPID_nem_fboxq_elem_list[i].grank = MPID_nem_mem_region.local_procs[i];
        MPID_nem_fboxq_elem_list[i].fbox = &MPID_nem_mem_region.mailboxes.in[i]->mpich;
    }
	
    MPID_nem_fboxq_head = NULL;
    MPID_nem_fboxq_tail = NULL;
    MPID_nem_curr_fboxq_elem = NULL;
    MPID_nem_curr_fbox_all_poll = &MPID_nem_fboxq_elem_list[0];
    MPID_nem_fboxq_elem_list_last = &MPID_nem_fboxq_elem_list[MPID_nem_mem_region.num_local - 1];

    MPIU_CHKPMEM_COMMIT();
fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_MPICH_INIT);
    return mpi_errno;
fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    MPIU_CHKPMEM_REAP();
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_send_iov
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_nem_send_iov(MPIDI_VC_t *vc, MPID_Request **sreq_ptr, MPL_IOV *iov, int n_iov)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_msg_sz_t data_sz;
    int i;
    int iov_data_copied;
    MPID_Request *sreq = *sreq_ptr;
    MPL_IOV *data_iov = &iov[1]; /* iov of just the data, not the header */
    int data_n_iov = n_iov - 1;

    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_SEND_IOV);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_SEND_IOV);

    if (*sreq_ptr == NULL)
    {
	/* create a request */
	sreq = MPID_Request_create();
	MPIU_Assert(sreq != NULL);
	MPIU_Object_set_ref(sreq, 2);
	sreq->kind = MPID_REQUEST_SEND;
        sreq->dev.OnDataAvail = 0;
    }

    data_sz = 0;
    for (i = 0; i < data_n_iov; ++i)
        data_sz += data_iov[i].MPL_IOV_LEN;


    if (!MPIDI_Request_get_srbuf_flag(sreq))
    {
        MPIDI_CH3U_SRBuf_alloc(sreq, data_sz);
        /* --BEGIN ERROR HANDLING-- */
        if (sreq->dev.tmpbuf_sz == 0)
        {
            MPIU_DBG_MSG(CH3_CHANNEL,TYPICAL,"SRBuf allocation failure");
            mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL,
                                             FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
            sreq->status.MPI_ERROR = mpi_errno;
            goto fn_exit;
        }
        /* --END ERROR HANDLING-- */
    }

    MPIU_Assert(sreq->dev.tmpbuf_sz >= data_sz);

    iov_data_copied = 0;
    for (i = 0; i < data_n_iov; ++i) {
        MPIU_Memcpy((char*) sreq->dev.tmpbuf + iov_data_copied, data_iov[i].MPL_IOV_BUF, data_iov[i].MPL_IOV_LEN);
        iov_data_copied += data_iov[i].MPL_IOV_LEN;
    }

    mpi_errno = vc->ch.iSendContig(vc, sreq, iov[0].MPL_IOV_BUF, iov[0].MPL_IOV_LEN, sreq->dev.tmpbuf, data_sz);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

    *sreq_ptr = sreq;

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_SEND_IOV);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}






