/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "mpidch4r.h"
#include "mpidig_part_utils.h"
#include "mpidpre.h"


/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

categories :
    - name : PARTITION
      description : A category for partitioned communication related variables

cvars:
    - name        : MPIR_CVAR_PART_MAXREQ_PERCOMM
      category    : PARTITION
      type        : int
      default     : 1
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        The maximum number of simultaneous partitioned communications initiated from a given process
        on a communicator
    - name        : MPIR_CVAR_PART_AGGR_SIZE
      category    : PARTITION
      type        : int
      default     : 0
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        The sizes up to which the partitions should be gathered together to be sent and reduce
        latency for small messages. This size will be matched as best as possible but might not be
        matched exactly

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

/* Create a MPIR_Request for part comm, both send and recv ones */
static int MPIDI_part_req_create(void *buf, int partitions, MPI_Aint count,
                                 MPI_Datatype datatype, int rank, int tag,
                                 MPIR_Comm * comm, MPIR_Request_kind_t kind,
                                 MPIR_Request ** req_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *req = NULL;

    MPIR_FUNC_ENTER;

    /* Set refcnt=1 for user-defined partitioned pattern; decrease at request_free. */
    /* the request is not stored internally at this point */
    /* this will create all the MPIR_Request fields */
    MPIDI_CH4_REQUEST_CREATE(req, kind, 0, 1);
    MPIR_ERR_CHKANDSTMT((req) == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");

    MPIR_Comm_add_ref(comm);
    req->comm = comm;
    MPIR_Comm_save_inactive_request(comm, req);

    MPIR_Datatype_add_ref_if_not_builtin(datatype);

    MPIDI_PART_REQUEST(req, buffer) = buf;
    MPIDI_PART_REQUEST(req, count) = count;
    MPIDI_PART_REQUEST(req, datatype) = datatype;
    if (kind == MPIR_REQUEST_KIND__PART_SEND) {
        MPIDI_PART_REQUEST(req, u.send.dest) = rank;
    } else {
        MPIDI_PART_REQUEST(req, u.recv.source) = rank;
        MPIDI_PART_REQUEST(req, u.recv.tag) = tag;
        MPIDI_PART_REQUEST(req, u.recv.context_id) = comm->context_id;
    }
    // store the number of partitions at the higher level
    req->u.part.partitions = partitions;


#ifndef MPIDI_CH4_DIRECT_NETMOD
    MPIDI_av_entry_t *av = MPIDIU_comm_rank_to_av(comm, rank);
    MPIDI_REQUEST(req, is_local) = MPIDI_av_is_local(av);
#endif

    /* Inactive partitioned comm request can be freed by request_free */
    MPIR_Part_request_inactivate(req);
    /* Completion cntr increases when request becomes active at start. */
    MPIR_cc_set(req->cc_ptr, 0);

    *req_ptr = req;

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDIG_mpi_psend_init(const void *buf, int partitions, MPI_Aint count, MPI_Datatype datatype,
                          int dest, int tag, MPIR_Comm * comm, MPIR_Info * info,
                          MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    // lock the VCI[0] -> request creation relies on the VCI
    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(0).lock);

    /* Create and initialize device-layer partitioned request */
    mpi_errno =
        MPIDI_part_req_create((void *) buf, partitions, count, datatype, dest, tag, comm,
                              MPIR_REQUEST_KIND__PART_SEND, request);
    MPIR_ERR_CHECK(mpi_errno);

    /* initialize the MPIDIG part of the request and reset the cc_part counters
     * they are allocated and set to the number of partitions as we don't have anything else yet */
    MPIDIG_part_sreq_create(request);

    //--------------------------------------------------------------------------
    /* we decide if we use the AM or the regular send code path using tags
     * due to the tag structure we can only perform a given number of simultaneous
     * partitioned communications */
    const bool do_tag = MPIR_cc_get(comm->part_context_cc) < MPIR_CVAR_PART_MAXREQ_PERCOMM;
    MPIR_cc_inc(&comm->part_context_cc);
    if (do_tag) {
        /* Initialize tag-matching components for send */
        MPIDIG_PART_REQUEST(*request, mode) = 1;
    } else {
        /* Initialize am components for send */
        MPIDIG_PART_REQUEST(*request, mode) = 0;
    }

    /* we need to know do_tag to set the cc_part values */
    MPIDIG_part_sreq_set_cc_part(*request);

    //--------------------------------------------------------------------------
    /* Initiate handshake with receiver for message matching */
    MPIDIG_part_send_init_msg_t am_hdr;
    am_hdr.src_rank = comm->rank;
    am_hdr.tag = tag;
    am_hdr.context_id = comm->context_id;
    am_hdr.sreq_ptr = *request;
    am_hdr.do_tag = do_tag;

    MPI_Aint dtype_size = 0;
    MPIR_Datatype_get_size_macro(datatype, dtype_size);
    am_hdr.data_sz = dtype_size * count * partitions;   /* count is per partition */

    /* indicate how many partitions there are on the sender side and the total count  */
    am_hdr.send_npart = partitions;
    am_hdr.send_ttl_dcount = partitions * count;

    /* send the header */
    CH4_CALL(am_send_hdr(dest, comm, MPIDIG_PART_SEND_INIT, &am_hdr, sizeof(am_hdr), 0, 0),
             MPIDI_REQUEST(*request, is_local), mpi_errno);

  fn_exit:
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(0).lock);
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDIG_mpi_precv_init(void *buf, int partitions, MPI_Aint count,
                          MPI_Datatype datatype, int source, int tag,
                          MPIR_Comm * comm, MPIR_Info * info, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(0).lock);

    /* Create and initialize device-layer partitioned request */
    mpi_errno =
        MPIDI_part_req_create(buf, partitions, count, datatype, source, tag, comm,
                              MPIR_REQUEST_KIND__PART_RECV, request);
    MPIR_ERR_CHECK(mpi_errno);
    /*initialize the MPIDIG part of the request */
    MPIDIG_part_rreq_create(request);

    /*--------------------------------------------------------------------------*/
    /* Try matching a request or post a new one */
    MPIR_Request *unexp_req = NULL;
    unexp_req =
        MPIDIG_rreq_dequeue(source, tag, comm->context_id, &MPIDI_global.part_unexp_list,
                            MPIDIG_PART);
    if (unexp_req) {
        /* the send_init msg has been received and the request was stored upon reception in
         * MPIDIG_part_send_init_target_msg_cb()
         * copy sender info from unexp_req to local part_rreq as stored in MPIDIG_part_rreq_update_sinfo */
        MPIDIG_PART_REQUEST(*request, u.recv.send_dsize) =
            MPIDIG_PART_REQUEST(unexp_req, u.recv.send_dsize);
        MPIDIG_PART_REQUEST(*request, msg_part) = MPIDIG_PART_REQUEST(unexp_req, msg_part);
        MPIDIG_PART_REQUEST(*request, mode) = MPIDIG_PART_REQUEST(unexp_req, mode);
        MPIDIG_PART_REQUEST(*request, peer_req_ptr) = MPIDIG_PART_REQUEST(unexp_req, peer_req_ptr);
        MPIDI_CH4_REQUEST_FREE(unexp_req);

        /* we have matched, fill the missing part of the requests: fill the status and allocate cc_part */
        MPIDIG_part_rreq_matched(*request);
    } else {
        /* add a reference for handshake
         * the request will be filled and handled in MPIDIG_part_send_init_target_msg_cb */
        MPIR_Request_add_ref(*request);
        MPIDIG_enqueue_request(*request, &MPIDI_global.part_posted_list, MPIDIG_PART);
    }

  fn_exit:
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(0).lock);
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
