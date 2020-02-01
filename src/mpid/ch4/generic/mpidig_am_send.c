#include "mpidimpl.h"

struct am_send_hdr {
    int src_rank;
    int tag;
    MPIR_Context_id_t context_id;
    int error_bits;
};

static int MPIDIG_send_origin_cb(MPIR_Request * sreq);
/* TODO: remove handler_id, target_cmpl_cb
 *       why **data, *p_data_sz
 *       do we need is_local, *is_contig
 *       remove req
 */
static int MPIDIG_send_target_msg_cb(int handler_id, void *am_hdr, void **data, size_t * p_data_sz,
                                     int is_local, int *is_contig,
                                     MPIDIG_am_target_cmpl_cb * target_cmpl_cb,
                                     MPIR_Request ** req);

int MPIDIG_am_send_init(void)
{
    /* MPIDIGI_AM_SEND_HDR_SIZE is used for testing EAGER/RENDEZVOUS limit.
     * Assertion is used to keep the am_send_hdr opaque to outside.
     */
    MPIR_Assert(sizeof(struct am_send_hdr) <= MPIDIGI_AM_SEND_HDR_SIZE);
    return MPIDIG_am_reg_cb(MPIDIG_SEND, &MPIDIG_send_origin_cb, &MPIDIG_send_target_msg_cb);
}

int MPIDIG_do_isend(const void *buf, MPI_Aint count, MPI_Datatype datatype,
                    int rank, int tag, MPIR_Comm * comm, int context_offset,
                    MPIDI_av_entry_t * addr, MPIR_Request * sreq, MPIR_Errflag_t errflag)
{
    int mpi_errno = MPI_SUCCESS;

    struct am_send_hdr am_hdr;
    am_hdr.src_rank = comm->rank;
    am_hdr.tag = tag;
    am_hdr.context_id = comm->context_id + context_offset;
    am_hdr.error_bits = errflag;

#ifdef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_NM_am_isend(rank, comm, MPIDIG_SEND,
                                  &am_hdr, sizeof(am_hdr), buf, count, datatype, sreq);
#else
    if (MPIDI_av_is_local(addr)) {
        mpi_errno = MPIDI_SHM_am_isend(rank, comm, MPIDIG_SEND,
                                       &am_hdr, sizeof(am_hdr), buf, count, datatype, sreq);
    } else {
        mpi_errno = MPIDI_NM_am_isend(rank, comm, MPIDIG_SEND,
                                      &am_hdr, sizeof(am_hdr), buf, count, datatype, sreq);
    }
#endif
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* internal static functions */
static int MPIDIG_send_origin_cb(MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Request_complete(sreq);
    return mpi_errno;
}

static int MPIDIG_send_target_msg_cb(int handler_id, void *am_hdr, void **data, size_t * p_data_sz,
                                     int is_local, int *is_contig,
                                     MPIDIG_am_target_cmpl_cb * target_cmpl_cb, MPIR_Request ** req)
{
    struct am_send_hdr *hdr = (struct am_send_hdr *) am_hdr;
    return MPIDIG_handle_send(hdr->src_rank, hdr->tag, hdr->context_id, hdr->error_bits,
                              data, p_data_sz, is_local, is_contig, target_cmpl_cb, req);
}
