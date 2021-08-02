/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "ofi_impl.h"

#define MPID_NORMAL_SEND 0

#define MPID_CREATE_REQ      0
#define MPID_DONT_CREATE_REQ 1

static inline int
MPID_nem_ofi_sync_recv_callback(cq_tagged_entry_t * wc ATTRIBUTE((unused)), MPIR_Request * rreq);

static inline int
MPID_nem_ofi_send_callback(cq_tagged_entry_t * wc ATTRIBUTE((unused)), MPIR_Request * sreq);

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
/* Receive callback called after sending a synchronous send acknowledgement. */
/* ------------------------------------------------------------------------ */
static inline int MPID_nem_ofi_sync_recv_callback(cq_tagged_entry_t * wc ATTRIBUTE((unused)),
                                                  MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    MPIDI_CH3U_Recvq_DP(REQ_OFI(rreq)->parent);
    MPIDI_CH3I_NM_OFI_RC(MPID_Request_complete(REQ_OFI(rreq)->parent));
    MPIDI_CH3I_NM_OFI_RC(MPID_Request_complete(rreq));

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* ------------------------------------------------------------------------ */
/* Send done callback                                                       */
/* Free any temporary/pack buffers and complete the send request            */
/* ------------------------------------------------------------------------ */
static inline int MPID_nem_ofi_send_callback(cq_tagged_entry_t * wc ATTRIBUTE((unused)),
                                             MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;
    MPL_free(REQ_OFI(sreq)->pack_buffer);
    MPIDI_CH3I_NM_OFI_RC(MPID_Request_complete(sreq));
  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* Use macro as the two functions share common body */
#define DO_CANCEL(req) \
  int mpi_errno = MPI_SUCCESS;                          \
  int ret;                                              \
  MPIR_FUNC_ENTER;                                      \
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
  MPIR_FUNC_EXIT;                                       \
  return mpi_errno;

int MPID_nem_ofi_cancel_send(struct MPIDI_VC *vc ATTRIBUTE((unused)), struct MPIR_Request *sreq)
{
    DO_CANCEL(sreq);
}

int MPID_nem_ofi_cancel_recv(struct MPIDI_VC *vc ATTRIBUTE((unused)), struct MPIR_Request *rreq)
{
    DO_CANCEL(rreq);
}


int MPID_nem_ofi_anysource_matched(MPIR_Request * rreq)
{
    int matched = FALSE;
    int ret;
    MPIR_FUNC_ENTER;
    /* ----------------------------------------------------- */
    /* Netmod has notified us that it has matched an any     */
    /* source request on another device.  We have the chance */
    /* to cancel this shared request if it has been posted   */
    /* ----------------------------------------------------- */
    ret = fi_cancel((fid_t) gl_data.endpoint, &(REQ_OFI(rreq)->ofi_context));
    if (ret == 0) {
        /* Cancel succeeded. This means that the actual message has been
         *  received via nemesis shared memory. We need to return
         *  matched=False. The request will be completed at the nemesis level.
         *
         * If anysource was posted for non-contig dtype then don't forget
         * to clean up tmp space.
         */
        MPL_free(REQ_OFI(rreq)->pack_buffer);
        matched = FALSE;
    } else {
        /* Cancel failed. We can only fail in the case of the message
         *  being already actually received via ofi fabric. return TRUE.
         */
        matched = TRUE;
    }
    MPIR_FUNC_EXIT;
    return matched;
}
