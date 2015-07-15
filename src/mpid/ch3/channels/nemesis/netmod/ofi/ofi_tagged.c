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

#define MPID_NORMAL_SEND 0
static inline int
MPID_nem_ofi_sync_recv_callback(cq_tagged_entry_t * wc ATTRIBUTE((unused)),
                                MPID_Request * rreq);

static inline int
MPID_nem_ofi_send_callback(cq_tagged_entry_t * wc ATTRIBUTE((unused)),
                           MPID_Request * sreq);

#define ADD_SUFFIX(name) name
#undef API_SET
#define API_SET API_SET_1
#include "ofi_tagged_template.c"
#undef ADD_SUFFIX
#define ADD_SUFFIX(name) name##_2
#undef API_SET
#define API_SET API_SET_2
#include "ofi_tagged_template.c"

/* ------------------------------------------------------------------------ */
/* Receive callback called after sending a syncronous send acknowledgement. */
/* ------------------------------------------------------------------------ */
#undef FCNAME
#define FCNAME DECL_FUNC(MPID_nem_ofi_sync_recv_callback)
static inline int MPID_nem_ofi_sync_recv_callback(cq_tagged_entry_t * wc ATTRIBUTE((unused)),
                                                  MPID_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;

    BEGIN_FUNC(FCNAME);

    MPIDI_CH3U_Recvq_DP(REQ_OFI(rreq)->parent);
    MPIDI_CH3I_NM_OFI_RC(MPID_Request_complete(REQ_OFI(rreq)->parent));
    MPIDI_CH3I_NM_OFI_RC(MPID_Request_complete(rreq));

    END_FUNC_RC(FCNAME);
}

/* ------------------------------------------------------------------------ */
/* Send done callback                                                       */
/* Free any temporary/pack buffers and complete the send request            */
/* ------------------------------------------------------------------------ */
#undef FCNAME
#define FCNAME DECL_FUNC(MPID_nem_ofi_send_callback)
static inline int MPID_nem_ofi_send_callback(cq_tagged_entry_t * wc ATTRIBUTE((unused)),
                                             MPID_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    BEGIN_FUNC(FCNAME);
    if (REQ_OFI(sreq)->pack_buffer)
        MPIU_Free(REQ_OFI(sreq)->pack_buffer);
    MPIDI_CH3I_NM_OFI_RC(MPID_Request_complete(sreq));
    END_FUNC_RC(FCNAME);
}

#define DO_CANCEL(req)                                  \
({                                                      \
  int mpi_errno = MPI_SUCCESS;                          \
  int ret;                                              \
  BEGIN_FUNC(FCNAME);                                   \
  MPID_nem_ofi_poll(MPID_NONBLOCKING_POLL);             \
  ret = fi_cancel((fid_t)gl_data.endpoint,              \
                  &(REQ_OFI(req)->ofi_context));        \
  if (ret == 0) {                                        \
      while (!MPIR_STATUS_GET_CANCEL_BIT(req->status)) {		\
	  if ((mpi_errno = MPID_nem_ofi_poll(MPID_NONBLOCKING_POLL)) != \
	      MPI_SUCCESS) {						\
	      MPIR_STATUS_SET_CANCEL_BIT(req->status, FALSE);	        \
	      return mpi_errno;						\
	  }								\
      }									\
    MPIR_STATUS_SET_CANCEL_BIT(req->status, TRUE);      \
  } else {                                              \
    MPIR_STATUS_SET_CANCEL_BIT(req->status, FALSE);     \
  }                                                     \
  END_FUNC(FCNAME);                                     \
  return mpi_errno;                                     \
})

#undef FCNAME
#define FCNAME DECL_FUNC(MPID_nem_ofi_cancel_send)
int MPID_nem_ofi_cancel_send(struct MPIDI_VC *vc ATTRIBUTE((unused)), struct MPID_Request *sreq)
{
    DO_CANCEL(sreq);
}

#undef FCNAME
#define FCNAME DECL_FUNC(MPID_nem_ofi_cancel_recv)
int MPID_nem_ofi_cancel_recv(struct MPIDI_VC *vc ATTRIBUTE((unused)), struct MPID_Request *rreq)
{
    DO_CANCEL(rreq);
}


#undef FCNAME
#define FCNAME DECL_FUNC(MPID_nem_ofi_anysource_matched)
int MPID_nem_ofi_anysource_matched(MPID_Request * rreq)
{
    int matched = FALSE;
    int ret;
    BEGIN_FUNC(FCNAME);
    /* ----------------------------------------------------- */
    /* Netmod has notified us that it has matched an any     */
    /* source request on another device.  We have the chance */
    /* to cancel this shared request if it has been posted   */
    /* ----------------------------------------------------- */
    ret = fi_cancel((fid_t) gl_data.endpoint, &(REQ_OFI(rreq)->ofi_context));
    if (ret == 0) {
        /* Cancel succeded. This means that the actual message has been
         *  received via nemesis shared memory. We need to return
         *  matched=False. The request will be completed at the nemesis level.
         *
         * If anysource was posted for non-contig dtype then don't forget
         * to clean up tmp space.
         */
        if (REQ_OFI(rreq)->pack_buffer) {
            MPIU_Free(REQ_OFI(rreq)->pack_buffer);
        }
        matched = FALSE;
    }else{
        /* Cancel failed. We can only fail in the case of the message
         *  being already actually received via ofi fabric. return TRUE.
         */
        matched = TRUE;
    }
    END_FUNC(FCNAME);
    return matched;
}
