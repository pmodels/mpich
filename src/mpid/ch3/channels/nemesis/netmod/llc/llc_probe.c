/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/* vim: set ts=8 sts=4 sw=4 noexpandtab : */
/*
 *
 */



#include "mpid_nem_impl.h"
#include "llc_impl.h"

//#define MPID_NEM_LLC_DEBUG_PROBE
#ifdef MPID_NEM_LLC_DEBUG_PROBE
#define dprintf printf
#else
#define dprintf(...)
#endif


#undef FUNCNAME
#define FUNCNAME MPID_nem_llc_probe
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_nem_llc_probe(MPIDI_VC_t * vc, int source, int tag, MPID_Comm * comm, int context_offset,
                       MPI_Status * status)
{
    int mpi_errno = MPI_SUCCESS;

    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_LLC_PROBE);
    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_LLC_PROBE);
    dprintf("llc_probe,source=%d,tag=%d\n", source, tag);

    /* NOTE : This function is not used. Because 'vc->comm_ops->probe()' is not used */
  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_LLC_PROBE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_llc_iprobe
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_nem_llc_iprobe(MPIDI_VC_t * vc, int source, int tag, MPID_Comm * comm, int context_offset,
                        int *flag, MPI_Status * status)
{
    int mpi_errno = MPI_SUCCESS, llc_errno;
    int rank;
    LLC_tag_t _tag;
    LLC_probe_t probe;
    LLC_match_mask_t mask;

    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_LLC_IPROBE);
    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_LLC_IPROBE);
    dprintf("llc_iprobe,source=%d,tag=%d\n", source, tag);

    mask.rank = ~0;
    mask.tag = ~0;

    if (tag == MPI_ANY_TAG) {
        *(int32_t *) ((uint8_t *) & _tag) = LLC_ANY_TAG;
        *(int32_t *) ((uint8_t *) & mask.tag) = 0;
    }
    else {
        *(int32_t *) ((uint8_t *) & _tag) = tag;
    }

    *(MPIU_Context_id_t *) ((uint8_t *) & _tag + sizeof(int32_t)) =
        comm->recvcontext_id + context_offset;
    memset((uint8_t *) & _tag + sizeof(int32_t) + sizeof(MPIU_Context_id_t),
           0, sizeof(LLC_tag_t) - sizeof(int32_t) - sizeof(MPIU_Context_id_t));

    if (source == MPI_ANY_SOURCE) {
        rank = LLC_ANY_SOURCE;
        mask.rank = 0;
    }
    else {
        MPIU_Assert(vc);
        rank = VC_FIELD(vc, remote_endpoint_addr);
    }

    llc_errno = LLC_probe(LLC_COMM_MPICH, rank, _tag, &mask, &probe);
    if (llc_errno == LLC_SUCCESS) {
        *flag = 1;
        status->MPI_ERROR = MPI_SUCCESS;
        if (source != MPI_ANY_SOURCE) {
            status->MPI_SOURCE = source;
        }
        else {
            int found = 0;
            found = convert_rank_llc2mpi(comm, probe.rank, &status->MPI_SOURCE);
            MPIU_Assert(found);
        }
        status->MPI_TAG = probe.tag & 0xffffffff;
        MPIR_STATUS_SET_COUNT(*status, probe.len);
    }
    else {
        *flag = 0;

        MPID_Progress_poke();   /* do LLC_poll */
    }

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_LLC_IPROBE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_llc_improbe
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_nem_llc_improbe(MPIDI_VC_t * vc, int source, int tag, MPID_Comm * comm, int context_offset,
                         int *flag, MPID_Request ** message, MPI_Status * status)
{
    int mpi_errno = MPI_SUCCESS;
    int rank;
    LLC_tag_t _tag;
    LLC_probe_t probe;
    LLC_match_mask_t mask;
    LLC_cmd_t *msg = NULL;

    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_LLC_IMPROBE);
    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_LLC_IMPROBE);
    dprintf("llc_improbe,source=%d,tag=%d\n", source, tag);

    mask.rank = ~0;
    mask.tag = ~0;

    if (tag == MPI_ANY_TAG) {
        *(int32_t *) ((uint8_t *) & _tag) = LLC_ANY_TAG;
        *(int32_t *) ((uint8_t *) & mask.tag) = 0;
    }
    else {
        *(int32_t *) ((uint8_t *) & _tag) = tag;
    }

    *(MPIU_Context_id_t *) ((uint8_t *) & _tag + sizeof(int32_t)) =
        comm->recvcontext_id + context_offset;
    memset((uint8_t *) & _tag + sizeof(int32_t) + sizeof(MPIU_Context_id_t),
           0, sizeof(LLC_tag_t) - sizeof(int32_t) - sizeof(MPIU_Context_id_t));

    if (source == MPI_ANY_SOURCE) {
        rank = LLC_ANY_SOURCE;
        mask.rank = 0;
    }
    else {
        MPIU_Assert(vc);
        rank = VC_FIELD(vc, remote_endpoint_addr);
    }

    msg = LLC_mprobe(LLC_COMM_MPICH, rank, _tag, &mask, &probe);
    if (msg) {
        MPID_Request *req;

        *flag = 1;

        req = MPID_Request_create();
        MPIU_Object_set_ref(req, 2);
        req->kind = MPID_REQUEST_MPROBE;
        req->comm = comm;
        MPIR_Comm_add_ref(comm);
        req->ch.vc = vc;

        MPIDI_Request_set_msg_type(req, MPIDI_REQUEST_EAGER_MSG);
        req->dev.recv_pending_count = 1;
        req->status.MPI_ERROR = MPI_SUCCESS;
        if (source != MPI_ANY_SOURCE) {
            req->status.MPI_SOURCE = source;
        }
        else {
            int found = 0;
            found = convert_rank_llc2mpi(comm, probe.rank, &req->status.MPI_SOURCE);
            MPIU_Assert(found);
        }
        req->status.MPI_TAG = probe.tag & 0xffffffff;
        req->dev.recv_data_sz = probe.len;
        MPIR_STATUS_SET_COUNT(req->status, req->dev.recv_data_sz);
        req->dev.tmpbuf = MPIU_Malloc(req->dev.recv_data_sz);
        MPIU_Assert(req->dev.tmpbuf);

        /* receive message in req->dev.tmpbuf */
        LLC_cmd_t *cmd = LLC_cmd_alloc2(1, 1, 1);

        cmd[0].opcode = 0;      // not use
        cmd[0].comm = LLC_COMM_MPICH;
        cmd[0].req_id = (uint64_t) cmd;
        cmd[0].rank = msg->rank;
        // cmd[0].tag = 0; // not use

        cmd[0].iov_local[0].addr = (uint64_t) req->dev.tmpbuf;
        cmd[0].iov_local[0].length = req->dev.recv_data_sz;
        cmd[0].niov_local = 1;

        cmd[0].iov_remote[0].addr = 0;
        cmd[0].iov_remote[0].length = req->dev.recv_data_sz;
        cmd[0].niov_remote = 1;

        ((struct llc_cmd_area *) cmd[0].usr_area)->cbarg = req;
        if (source == MPI_ANY_SOURCE) {
            ((struct llc_cmd_area *) cmd[0].usr_area)->raddr = MPI_ANY_SOURCE;
        }
        else {
            ((struct llc_cmd_area *) cmd[0].usr_area)->raddr = VC_FIELD(vc, remote_endpoint_addr);
        }

        LLC_recv_msg(cmd, msg);

        /* Wait until the reception of data is completed */
        do {
            mpi_errno = MPID_nem_llc_poll(0);
        } while (!MPID_Request_is_complete(req));

//        MPID_Request_complete(req); // This operation is done in llc_poll.

        *message = req;

        /* TODO : Should we change status ? */
        //status->MPI_ERROR = MPI_SUCCESS;
        status->MPI_SOURCE = req->status.MPI_SOURCE;
        status->MPI_TAG = req->status.MPI_TAG;
        MPIR_STATUS_SET_COUNT(*status, req->dev.recv_data_sz);
    }
    else {
        *flag = 0;
        *message = NULL;

        MPID_Progress_poke();   /* do LLC_poll */
    }

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_LLC_IMPROBE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
