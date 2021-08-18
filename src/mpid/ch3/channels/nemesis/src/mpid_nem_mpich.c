/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
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

int
MPID_nem_mpich_init(void)
{
    int mpi_errno = MPI_SUCCESS;
    int i;
    MPIR_CHKPMEM_DECL (2);

    MPIR_FUNC_ENTER;

    MPID_nem_prefetched_cell = NULL;

    MPIR_CHKPMEM_MALLOC (MPID_nem_recv_seqno, unsigned short *, sizeof(*MPID_nem_recv_seqno) * MPID_nem_mem_region.num_procs, mpi_errno, "recv seqno", MPL_MEM_SHM);

    for (i = 0; i < MPID_nem_mem_region.num_procs; ++i)
    {
        MPID_nem_recv_seqno[i] = 0;
    }

    /* set up fbox queue */
    MPIR_CHKPMEM_MALLOC (MPID_nem_fboxq_elem_list, MPID_nem_fboxq_elem_t *, MPID_nem_mem_region.num_local * sizeof(MPID_nem_fboxq_elem_t), mpi_errno, "fastbox element list", MPL_MEM_SHM);

    for (i = 0; i < MPID_nem_mem_region.num_local; ++i)
    {
        MPID_nem_fboxq_elem_list[i].usage = 0;
        MPID_nem_fboxq_elem_list[i].prev = NULL;
        MPID_nem_fboxq_elem_list[i].next = NULL;
        MPID_nem_fboxq_elem_list[i].grank = MPID_nem_mem_region.local_procs[i];
        MPID_nem_fboxq_elem_list[i].fbox = MPID_nem_mem_region.mailboxes.in[i];
    }
	
    MPID_nem_fboxq_head = NULL;
    MPID_nem_fboxq_tail = NULL;
    MPID_nem_curr_fboxq_elem = NULL;
    MPID_nem_curr_fbox_all_poll = &MPID_nem_fboxq_elem_list[0];
    MPID_nem_fboxq_elem_list_last = &MPID_nem_fboxq_elem_list[MPID_nem_mem_region.num_local - 1];

    MPIR_CHKPMEM_COMMIT();
fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    MPIR_CHKPMEM_REAP();
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

int MPID_nem_send_iov(MPIDI_VC_t *vc, MPIR_Request **sreq_ptr, struct iovec *iov, int n_iov)
{
    int mpi_errno = MPI_SUCCESS;
    intptr_t data_sz;
    int i;
    int iov_data_copied;
    MPIR_Request *sreq = *sreq_ptr;
    struct iovec *data_iov = &iov[1]; /* iov of just the data, not the header */
    int data_n_iov = n_iov - 1;


    MPIR_FUNC_ENTER;

    if (*sreq_ptr == NULL)
    {
	/* create a request */
	sreq = MPIR_Request_create(MPIR_REQUEST_KIND__UNDEFINED);
	MPIR_Assert(sreq != NULL);
	MPIR_Object_set_ref(sreq, 2);
	sreq->kind = MPIR_REQUEST_KIND__SEND;
        sreq->dev.OnDataAvail = 0;
    }

    /* directly use the iov-based send API if it is provided */
    if (vc->ch.iSendIov) {
        MPIR_Assert(n_iov >= 1 && n_iov <= MPL_IOV_LIMIT);

        /* header and remaining iovs */
        mpi_errno = vc->ch.iSendIov(vc, sreq, iov[0].iov_base, iov[0].iov_len, &iov[1], n_iov - 1);
        MPIR_ERR_CHECK(mpi_errno);

        *sreq_ptr = sreq;
        goto fn_exit;
    }

    data_sz = 0;
    for (i = 0; i < data_n_iov; ++i)
        data_sz += data_iov[i].iov_len;


    if (!MPIDI_Request_get_srbuf_flag(sreq))
    {
        MPIDI_CH3U_SRBuf_alloc(sreq, data_sz);
        /* --BEGIN ERROR HANDLING-- */
        if (sreq->dev.tmpbuf_sz == 0)
        {
            MPL_DBG_MSG(MPIDI_CH3_DBG_CHANNEL,TYPICAL,"SRBuf allocation failure");
            mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL,
                                             __func__, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
            sreq->status.MPI_ERROR = mpi_errno;
            goto fn_exit;
        }
        /* --END ERROR HANDLING-- */
    }

    MPIR_Assert(sreq->dev.tmpbuf_sz >= data_sz);

    iov_data_copied = 0;
    for (i = 0; i < data_n_iov; ++i) {
        MPIR_Memcpy((char*) sreq->dev.tmpbuf + iov_data_copied, data_iov[i].iov_base, data_iov[i].iov_len);
        iov_data_copied += data_iov[i].iov_len;
    }

    mpi_errno = vc->ch.iSendContig(vc, sreq, iov[0].iov_base, iov[0].iov_len, sreq->dev.tmpbuf, data_sz);
    MPIR_ERR_CHECK(mpi_errno);

    *sreq_ptr = sreq;

 fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}






