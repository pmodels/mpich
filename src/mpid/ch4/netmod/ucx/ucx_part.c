/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "ucx_impl.h"
#include "ucx_part_utils.h"
#include "ucx_noinline.h"

struct precv_matched_hdr {
    MPI_Request sreq;
    MPI_Request rreq;
};

#if defined(HAVE_UCP_TAG_NBX) && !defined(MPIDI_ENABLE_AM_ONLY)
static void init_part_request(MPIR_Request * request)
{
    int is_contig;
    MPI_Aint true_lb;
    MPI_Datatype datatype = MPIDI_PART_REQUEST(request, datatype);

    MPIDI_UCX_PART_REQ(request).ep =
        MPIDI_UCX_COMM_TO_EP(request->comm, MPIDI_PART_REQUEST(request, rank), 0, 0);

    MPIDI_Datatype_check_contig_lb(MPIDI_PART_REQUEST(request, datatype), is_contig, true_lb);
    if (!is_contig) {
        MPIR_Datatype *dt_ptr;

        MPIR_Datatype_get_ptr(datatype, dt_ptr);
        MPIR_Assert(dt_ptr != NULL);
        MPIDI_UCX_PART_REQ(request).ucp_datatype = dt_ptr->dev.netmod.ucx.ucp_datatype;
    } else {
        MPI_Aint dt_size;
        MPIR_Datatype_get_size_macro(datatype, dt_size);

        MPIDI_UCX_PART_REQ(request).ucp_datatype = ucp_dt_make_contig(dt_size);
        MPIDI_PART_REQUEST(request, buffer) =
            (char *) MPIDI_PART_REQUEST(request, buffer) + true_lb;
    }
}
#endif

int MPIDI_UCX_mpi_psend_init_hook(void *buf, int partitions, MPI_Aint count,
                                  MPI_Datatype datatype, int dest, int tag,
                                  MPIR_Comm * comm, MPIR_Info * info,
                                  MPIDI_av_entry_t * av, MPIR_Request ** request)
{
#if defined(HAVE_UCP_TAG_NBX) && !defined(MPIDI_ENABLE_AM_ONLY)
    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(0).lock);
    init_part_request(*request);
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(0).lock);
#endif

    return MPI_SUCCESS;
}

int MPIDI_UCX_mpi_precv_init_hook(void *buf, int partitions, MPI_Aint count,
                                  MPI_Datatype datatype, int source, int tag,
                                  MPIR_Comm * comm, MPIR_Info * info,
                                  MPIDI_av_entry_t * av, MPIR_Request ** request)
{
#if defined(HAVE_UCP_TAG_NBX) && !defined(MPIDI_ENABLE_AM_ONLY)
    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(0).lock);
    init_part_request(*request);
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(0).lock);
#endif

    return MPI_SUCCESS;
}

int MPIDI_UCX_precv_matched_hook(MPIR_Request * part_req)
{
    int mpi_errno = MPI_SUCCESS;

#if defined(HAVE_UCP_TAG_NBX) && !defined(MPIDI_ENABLE_AM_ONLY)
    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(0).lock);

    struct precv_matched_hdr hdr = {
        .sreq = MPIDI_PART_REQUEST(part_req, peer_req),
        .rreq = part_req->handle,
    };

    mpi_errno =
        MPIDI_NM_am_send_hdr_reply(part_req->comm, MPIDI_PART_REQUEST(part_req, rank),
                                   MPIDI_UCX_PRECV_MATCHED, &hdr, sizeof(hdr));
    MPIR_ERR_CHECK(mpi_errno);

  fn_fail:
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(0).lock);
#endif
    return mpi_errno;
}

/* ucx specific partitioned active messages */

static int precv_matched_target_msg_cb(int handler_id, void *am_hdr,
                                       void *data, MPI_Aint data_sz,
                                       int is_local, int is_async, MPIR_Request ** req)
{
#ifdef HAVE_UCP_TAG_NBX
    struct precv_matched_hdr *hdr = am_hdr;
    MPIR_Request *sreq;
    MPIR_Request_get_ptr(hdr->sreq, sreq);
    MPIR_Assert(sreq);

    MPIDI_PART_REQUEST(sreq, peer_req) = hdr->rreq;

    if (MPIR_Part_request_is_active(sreq) && MPIR_cc_get(MPIDI_UCX_PART_REQ(sreq).parts_left) == 0) {
        MPIDI_UCX_part_send(sreq);
    }
#endif

    return MPI_SUCCESS;
}

void MPIDI_UCX_am_init(void)
{
    MPIDIG_am_reg_cb(MPIDI_UCX_PRECV_MATCHED, NULL, &precv_matched_target_msg_cb);
}
