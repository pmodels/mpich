/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2012 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#include "ofi_impl.h"

#define NORMAL_PEEK    0
#define CLAIM_PEEK     1

/* ------------------------------------------------------------------------ */
/* This routine looks up the request that contains a context object         */
/* ------------------------------------------------------------------------ */
static inline MPID_Request *context_to_req(void *ofi_context)
{
    return (MPID_Request *) container_of(ofi_context, MPID_Request, ch.netmod_area.padding);
}

#define ADD_SUFFIX(name) name
#undef API_SET
#define API_SET API_SET_1
#include "ofi_probe_template.c"
#undef ADD_SUFFIX
#define ADD_SUFFIX(name) name##_2
#undef API_SET
#define API_SET API_SET_2
#include "ofi_probe_template.c"


#undef FCNAME
#define FCNAME DECL_FUNC(MPID_nem_ofi_poll)
int MPID_nem_ofi_poll(int in_blocking_poll)
{
    int complete = 0, mpi_errno = MPI_SUCCESS;
    ssize_t ret;
    cq_tagged_entry_t wc;
    cq_err_entry_t error;
    MPIDI_VC_t *vc;
    MPID_Request *req;
    req_fn reqFn;
    BEGIN_FUNC(FCNAME);
    do {
        /* ----------------------------------------------------- */
        /* Poll the completion queue                             */
        /* The strategy here is                                  */
        /* ret>0 successfull poll, events returned               */
        /* ret==0 empty poll, no events/no error                 */
        /* ret<0, error, but some error instances should not     */
        /* cause MPI to terminate                                */
        /* ----------------------------------------------------- */
        ret = fi_cq_read(gl_data.cq,    /* Tagged completion queue       */
                         (void *) &wc,  /* OUT:  Tagged completion entry */
                         1);    /* Number of entries to poll     */
        if (ret > 0) {
            if (NULL != wc.op_context) {
                req = context_to_req(wc.op_context);
                if (REQ_OFI(req)->event_callback) {
                    MPIDI_CH3I_NM_OFI_RC(REQ_OFI(req)->event_callback(&wc, req));
                    continue;
                }
                reqFn = req->dev.OnDataAvail;
                if (reqFn) {
                    if (REQ_OFI(req)->pack_buffer) {
                        MPIU_Free(REQ_OFI(req)->pack_buffer);
                    }
                    vc = REQ_OFI(req)->vc;

                    complete = 0;
                    MPIDI_CH3I_NM_OFI_RC(reqFn(vc, req, &complete));
                    continue;
                }
                else {
                    MPIU_Assert(0);
                }
            }
            else {
                MPIU_Assert(0);
            }
        }
        else if (ret == -FI_EAGAIN)
          ;
        else if (ret < 0) {
            if (ret == -FI_EAVAIL) {
                ret = fi_cq_readerr(gl_data.cq, (void *) &error, 0);
                if (error.err == FI_ETRUNC) {
                    /* ----------------------------------------------------- */
                    /* This error message should only be delivered on send   */
                    /* events.  We want to ignore truncation errors          */
                    /* on the sender side, but complete the request anyway   */
                    /* Other kinds of requests, this is fatal.               */
                    /* ----------------------------------------------------- */
                    req = context_to_req(error.op_context);
                    if (req->kind == MPID_REQUEST_SEND) {
                        mpi_errno = REQ_OFI(req)->event_callback(NULL, req);
                    }
                    else if (req->kind == MPID_REQUEST_RECV) {
                        mpi_errno = REQ_OFI(req)->event_callback(&wc, req);
                        req->status.MPI_ERROR = MPI_ERR_TRUNCATE;
                        req->status.MPI_TAG = error.tag;
                    }
                    else {
                        mpi_errno = MPI_ERR_OTHER;
                    }
                }
		else if (error.err == FI_ECANCELED) {
			req = context_to_req(error.op_context);
			MPIR_STATUS_SET_CANCEL_BIT(req->status, TRUE);
		}
		else {
                        mpi_errno = MPI_ERR_OTHER;
		}
            }
            else {
                MPIR_ERR_CHKANDJUMP4(1, mpi_errno, MPI_ERR_OTHER, "**ofi_poll",
                                     "**ofi_poll %s %d %s %s", __SHORT_FILE__,
                                     __LINE__, FCNAME, fi_strerror(-ret));
            }
        }
    } while (in_blocking_poll && (ret > 0));
    END_FUNC_RC(FCNAME);
}
