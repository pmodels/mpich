/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef OFI_AM_H_INCLUDED
#define OFI_AM_H_INCLUDED
#include "ofi_impl.h"
#include "ofi_am_impl.h"
#include "ofi_am_events.h"
#include "mpidu_genq.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_CH4_OFI_AM_LONG_FORCE_PIPELINE
      category    : DEVELOPER
      type        : boolean
      default     : false
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        For long message to be sent using pipeline rather than default
        RDMA read.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_progress_do_queue(int vni_idx);

MPL_STATIC_INLINE_PREFIX void MPIDI_NM_am_request_init(MPIR_Request * req)
{
    MPIDI_OFI_AMREQUEST(req, sreq_hdr) = NULL;
    MPIDI_OFI_AMREQUEST(req, rreq_hdr) = NULL;
    MPIDI_OFI_AMREQUEST(req, am_type_choice) = MPIDI_AMTYPE_NONE;
}

MPL_STATIC_INLINE_PREFIX void MPIDI_NM_am_request_finalize(MPIR_Request * req)
{
    MPIDI_OFI_am_clear_request(req);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_am_isend(int rank,
                                               MPIR_Comm * comm,
                                               int handler_id,
                                               const void *am_hdr,
                                               MPI_Aint am_hdr_sz,
                                               const void *data,
                                               MPI_Aint count, MPI_Datatype datatype,
                                               int src_vci, int dst_vci, MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPI_Aint data_sz = 0;
    MPIR_FUNC_ENTER;

    int vni_src = MPIDI_OFI_vci_to_vni(src_vci);
    int vni_dst = MPIDI_OFI_vci_to_vni(dst_vci);
    switch (MPIDI_OFI_AMREQUEST(sreq, am_type_choice)) {
        case MPIDI_AMTYPE_NONE:
            /* if no preselected amtype, do check here */
            MPIDI_Datatype_check_size(datatype, count, data_sz);
            if (data_sz + am_hdr_sz <= MPIDI_NM_am_eager_limit()) {
                /* EAGER */
                mpi_errno = MPIDI_OFI_do_am_isend_eager(rank, comm, handler_id, am_hdr, am_hdr_sz,
                                                        data, count, datatype, sreq, false, vni_src,
                                                        vni_dst);
            } else {
                if (MPIDI_OFI_ENABLE_RMA && !MPIR_CVAR_CH4_OFI_AM_LONG_FORCE_PIPELINE) {
                    /* RDMA READ */
                    mpi_errno = MPIDI_OFI_do_am_isend_rdma_read(rank, comm, handler_id, am_hdr,
                                                                am_hdr_sz, data, count, datatype,
                                                                sreq, false, vni_src, vni_dst);
                } else {
                    /* PIPELINE */
                    mpi_errno = MPIDI_OFI_do_am_isend_pipeline(rank, comm, handler_id, am_hdr,
                                                               am_hdr_sz, data, count, datatype,
                                                               sreq, data_sz, false, vni_src,
                                                               vni_dst);
                }
            }
            break;
        case MPIDI_AMTYPE_SHORT_HDR:
            MPIR_Assert(0);     /* header only should go to the send hdr interface */
            break;
        case MPIDI_AMTYPE_SHORT:
            mpi_errno = MPIDI_OFI_do_am_isend_eager(rank, comm, handler_id, am_hdr, am_hdr_sz, data,
                                                    count, datatype, sreq, false, vni_src, vni_dst);
            /* cleanup preselected amtype to avoid problem with reused request */
            MPIDI_OFI_AMREQUEST(sreq, am_type_choice) = MPIDI_AMTYPE_NONE;
            break;
        case MPIDI_AMTYPE_PIPELINE:
            mpi_errno = MPIDI_OFI_do_am_isend_pipeline(rank, comm, handler_id, am_hdr, am_hdr_sz,
                                                       data, count, datatype, sreq,
                                                       MPIDI_OFI_AMREQUEST(sreq, data_sz), false,
                                                       vni_src, vni_dst);
            /* cleanup preselected amtype to avoid problem with reused request */
            MPIDI_OFI_AMREQUEST(sreq, am_type_choice) = MPIDI_AMTYPE_NONE;
            break;
        case MPIDI_AMTYPE_RDMA_READ:
            mpi_errno = MPIDI_OFI_do_am_isend_rdma_read(rank, comm, handler_id, am_hdr, am_hdr_sz,
                                                        data, count, datatype, sreq, false, vni_src,
                                                        vni_dst);
            /* cleanup preselected amtype to avoid problem with reused request */
            MPIDI_OFI_AMREQUEST(sreq, am_type_choice) = MPIDI_AMTYPE_NONE;
            break;
        default:
            MPIR_Assert(0);     /* header only should go to the send hdr interface */
    }

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_am_isend_reply(MPIR_Comm * comm,
                                                     int src_rank,
                                                     int handler_id,
                                                     const void *am_hdr,
                                                     MPI_Aint am_hdr_sz,
                                                     const void *data,
                                                     MPI_Aint count,
                                                     MPI_Datatype datatype,
                                                     int src_vci, int dst_vci, MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    mpi_errno = MPIDI_NM_am_isend(src_rank, comm, handler_id,
                                  am_hdr, am_hdr_sz, data, count, datatype, src_vci, dst_vci, sreq);

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX MPI_Aint MPIDI_NM_am_hdr_max_sz(void)
{
    /* Maximum size that fits in short send */
    MPI_Aint max_shortsend = MPIDI_OFI_DEFAULT_SHORT_SEND_SIZE -
        (sizeof(MPIDI_OFI_am_header_t) + sizeof(MPIDI_OFI_lmt_msg_payload_t));
    /* Maximum payload size representable by MPIDI_OFI_am_header_t::am_hdr_sz field */
    return MPL_MIN(max_shortsend, MPIDI_OFI_MAX_AM_HDR_SIZE);
}

MPL_STATIC_INLINE_PREFIX MPI_Aint MPIDI_NM_am_eager_limit(void)
{
    return MPIDI_OFI_DEFAULT_SHORT_SEND_SIZE - sizeof(MPIDI_OFI_am_header_t);
}

MPL_STATIC_INLINE_PREFIX MPI_Aint MPIDI_NM_am_eager_buf_limit(void)
{
    return MPIDI_OFI_DEFAULT_SHORT_SEND_SIZE;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_am_send_hdr(int rank,
                                                  MPIR_Comm * comm,
                                                  int handler_id, const void *am_hdr,
                                                  MPI_Aint am_hdr_sz, int src_vci, int dst_vci)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    int vni_src = MPIDI_OFI_vci_to_vni(src_vci);
    int vni_dst = MPIDI_OFI_vci_to_vni(dst_vci);
    mpi_errno = MPIDI_OFI_do_inject(rank, comm, handler_id, am_hdr, am_hdr_sz, vni_src, vni_dst);

    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_am_send_hdr_reply(MPIR_Comm * comm,
                                                        int src_rank,
                                                        int handler_id, const void *am_hdr,
                                                        MPI_Aint am_hdr_sz,
                                                        int src_vci, int dst_vci)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    mpi_errno =
        MPIDI_OFI_do_inject(src_rank, comm, handler_id, am_hdr, am_hdr_sz, src_vci, dst_vci);

    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX bool MPIDI_NM_am_check_eager(MPI_Aint am_hdr_sz, MPI_Aint data_sz,
                                                      const void *data, MPI_Aint count,
                                                      MPI_Datatype datatype, MPIR_Request * sreq)
{
    MPIDI_OFI_AMREQUEST(sreq, data_sz) = data_sz;
    if ((am_hdr_sz + data_sz)
        <= (MPIDI_OFI_DEFAULT_SHORT_SEND_SIZE - sizeof(MPIDI_OFI_am_header_t))) {
        MPIDI_OFI_AMREQUEST(sreq, am_type_choice) = MPIDI_AMTYPE_SHORT;
        return true;
    } else {
        if (MPIDI_OFI_ENABLE_RMA && !MPIR_CVAR_CH4_OFI_AM_LONG_FORCE_PIPELINE) {
            MPIDI_OFI_AMREQUEST(sreq, am_type_choice) = MPIDI_AMTYPE_RDMA_READ;
            return true;
        } else {
            /* Forced PIPELINE */
            MPIDI_OFI_AMREQUEST(sreq, am_type_choice) = MPIDI_AMTYPE_PIPELINE;
            return false;
        }
    }
}

MPL_STATIC_INLINE_PREFIX MPIDIG_recv_data_copy_cb MPIDI_NM_am_get_data_copy_cb(uint32_t attr)
{
    MPIR_Assert(attr & MPIDI_OFI_AM_ATTR__RDMA);
    return MPIDI_OFI_am_rdma_read_recv_cb;
}

#endif /* OFI_AM_H_INCLUDED */
