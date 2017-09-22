/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2014 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hcoll.h"
#include "hcoll/api/hcoll_dte.h"

static int recv_nb(dte_data_representation_t data,
                   uint32_t count,
                   void *buffer,
                   rte_ec_handle_t, rte_grp_handle_t, uint32_t tag, rte_request_handle_t * req);

static int send_nb(dte_data_representation_t data,
                   uint32_t count,
                   void *buffer,
                   rte_ec_handle_t ec_h,
                   rte_grp_handle_t grp_h, uint32_t tag, rte_request_handle_t * req);

static int test(rte_request_handle_t * request, int *completed);

static int ec_handle_compare(rte_ec_handle_t handle_1,
                             rte_grp_handle_t
                             group_handle_1,
                             rte_ec_handle_t handle_2, rte_grp_handle_t group_handle_2);

static int get_ec_handles(int num_ec,
                          int *ec_indexes, rte_grp_handle_t, rte_ec_handle_t * ec_handles);

static int group_size(rte_grp_handle_t group);
static int my_rank(rte_grp_handle_t grp_h);
static int ec_on_local_node(rte_ec_handle_t ec, rte_grp_handle_t group);
static rte_grp_handle_t get_world_group_handle(void);
static uint32_t jobid(void);

static void *get_coll_handle(void);
static int coll_handle_test(void *handle);
static void coll_handle_free(void *handle);
static void coll_handle_complete(void *handle);
static int group_id(rte_grp_handle_t group);

static int world_rank(rte_grp_handle_t grp_h, rte_ec_handle_t ec);

#undef FUNCNAME
#define FUNCNAME progress
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static void progress(void)
{
    int ret;
    int made_progress;

    if (0 == world_comm_destroying) {
        MPID_Progress_test();
    }
    else {
        /* FIXME: The hcoll library needs to be updated to return
         * error codes.  The progress function pointer right now
         * expects that the function returns void. */
        ret = hcoll_do_progress(&made_progress);
        MPIR_Assert(ret == MPI_SUCCESS);
    }
}

#undef FUNCNAME
#define FUNCNAME init_module_fns
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static void init_module_fns(void)
{
    hcoll_rte_functions.send_fn = send_nb;
    hcoll_rte_functions.recv_fn = recv_nb;
    hcoll_rte_functions.ec_cmp_fn = ec_handle_compare;
    hcoll_rte_functions.get_ec_handles_fn = get_ec_handles;
    hcoll_rte_functions.rte_group_size_fn = group_size;
    hcoll_rte_functions.test_fn = test;
    hcoll_rte_functions.rte_my_rank_fn = my_rank;
    hcoll_rte_functions.rte_ec_on_local_node_fn = ec_on_local_node;
    hcoll_rte_functions.rte_world_group_fn = get_world_group_handle;
    hcoll_rte_functions.rte_jobid_fn = jobid;
    hcoll_rte_functions.rte_progress_fn = progress;
    hcoll_rte_functions.rte_get_coll_handle_fn = get_coll_handle;
    hcoll_rte_functions.rte_coll_handle_test_fn = coll_handle_test;
    hcoll_rte_functions.rte_coll_handle_free_fn = coll_handle_free;
    hcoll_rte_functions.rte_coll_handle_complete_fn = coll_handle_complete;
    hcoll_rte_functions.rte_group_id_fn = group_id;
    hcoll_rte_functions.rte_world_rank_fn = world_rank;
}

#undef FUNCNAME
#define FUNCNAME hcoll_rte_fns_setup
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
void hcoll_rte_fns_setup(void)
{
    init_module_fns();
}

#undef FUNCNAME
#define FUNCNAME recv_nb
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int recv_nb(struct dte_data_representation_t data,
                   uint32_t count,
                   void *buffer,
                   rte_ec_handle_t ec_h,
                   rte_grp_handle_t grp_h, uint32_t tag, rte_request_handle_t * req)
{
    int mpi_errno;
    MPI_Datatype dtype;
    MPIR_Request *request;
    MPIR_Comm *comm;
    size_t size;
    mpi_errno = MPI_SUCCESS;
    comm = (MPIR_Comm *) grp_h;
    if (!ec_h.handle) {
        MPIR_ERR_SETANDJUMP2(mpi_errno, MPI_ERR_OTHER, "**hcoll_wrong_arg",
                             "**hcoll_wrong_arg %p %d", ec_h.handle, ec_h.rank);
    }

    MPIR_Assert(HCOL_DTE_IS_INLINE(data));
    if (!buffer && !HCOL_DTE_IS_ZERO(data)) {
        MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**null_buff_ptr");
    }
    size = (size_t) data.rep.in_line_rep.data_handle.in_line.packed_size * count / 8;
    dtype = MPI_CHAR;
    request = NULL;
    mpi_errno = MPIC_Irecv(buffer, size, dtype, ec_h.rank, tag, comm, &request);
    MPIR_Assert(request);
    req->data = (void *) request;
    req->status = HCOLRTE_REQUEST_ACTIVE;
  fn_exit:
    return mpi_errno;
  fn_fail:
    return HCOLL_ERROR;
}

#undef FUNCNAME
#define FUNCNAME send_nb
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int send_nb(dte_data_representation_t data,
                   uint32_t count,
                   void *buffer,
                   rte_ec_handle_t ec_h,
                   rte_grp_handle_t grp_h, uint32_t tag, rte_request_handle_t * req)
{
    int mpi_errno;
    MPI_Datatype dtype;
    MPIR_Request *request;
    MPIR_Comm *comm;
    size_t size;
    mpi_errno = MPI_SUCCESS;
    comm = (MPIR_Comm *) grp_h;
    if (!ec_h.handle) {
        MPIR_ERR_SETANDJUMP2(mpi_errno, MPI_ERR_OTHER, "**hcoll_wrong_arg",
                             "**hcoll_wrong_arg %p %d", ec_h.handle, ec_h.rank);
    }

    MPIR_Assert(HCOL_DTE_IS_INLINE(data));
    if (!buffer && !HCOL_DTE_IS_ZERO(data)) {
        MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**null_buff_ptr");
    }
    size = (size_t) data.rep.in_line_rep.data_handle.in_line.packed_size * count / 8;
    dtype = MPI_CHAR;
    request = NULL;
    MPIR_Errflag_t err = MPIR_ERR_NONE;
    mpi_errno = MPIC_Isend(buffer, size, dtype, ec_h.rank, tag, comm, &request, &err);
    MPIR_Assert(request);
    req->data = (void *) request;
    req->status = HCOLRTE_REQUEST_ACTIVE;
  fn_exit:
    return mpi_errno;
  fn_fail:
    return HCOLL_ERROR;
}

#undef FUNCNAME
#define FUNCNAME test
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int test(rte_request_handle_t * request, int *completed)
{
    MPIR_Request *req;
    req = (MPIR_Request *) request->data;
    if (HCOLRTE_REQUEST_ACTIVE != request->status) {
        *completed = true;
        return HCOLL_SUCCESS;
    }

    *completed = (int) MPIR_Request_is_complete(req);
    if (*completed) {
        MPIR_Request_free(req);
        request->status = HCOLRTE_REQUEST_DONE;
    }

    return HCOLL_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME ec_handle_compare
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int ec_handle_compare(rte_ec_handle_t handle_1,
                             rte_grp_handle_t
                             group_handle_1,
                             rte_ec_handle_t handle_2, rte_grp_handle_t group_handle_2)
{
    return handle_1.handle == handle_2.handle;
}

#undef FUNCNAME
#define FUNCNAME get_ec_handles
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int get_ec_handles(int num_ec,
                          int *ec_indexes, rte_grp_handle_t grp_h, rte_ec_handle_t * ec_handles)
{
    int i;
    MPIR_Comm *comm;
    comm = (MPIR_Comm *) grp_h;
    for (i = 0; i < num_ec; i++) {
        ec_handles[i].rank = ec_indexes[i];
#ifdef MPIDCH4_H_INCLUDED
        ec_handles[i].handle = (void *) (MPIDIU_comm_rank_to_av(comm, ec_indexes[i]));
#else
        ec_handles[i].handle = (void *) (comm->dev.vcrt->vcr_table[ec_indexes[i]]);
#endif
    }
    return HCOLL_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME group_size
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int group_size(rte_grp_handle_t grp_h)
{
    return MPIR_Comm_size((MPIR_Comm *) grp_h);
}

#undef FUNCNAME
#define FUNCNAME my_rank
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int my_rank(rte_grp_handle_t grp_h)
{
    return MPIR_Comm_rank((MPIR_Comm *) grp_h);
}

#undef FUNCNAME
#define FUNCNAME ec_on_local_node
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int ec_on_local_node(rte_ec_handle_t ec, rte_grp_handle_t group)
{
    MPIR_Comm *comm;
    MPID_Node_id_t nodeid, my_nodeid;
    int my_rank;
    comm = (MPIR_Comm *) group;
    MPID_Get_node_id(comm, ec.rank, &nodeid);
    my_rank = MPIR_Comm_rank(comm);
    MPID_Get_node_id(comm, my_rank, &my_nodeid);
    return (nodeid == my_nodeid);
}


#undef FUNCNAME
#define FUNCNAME get_world_group_handle
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static rte_grp_handle_t get_world_group_handle(void)
{
    return (rte_grp_handle_t) (MPIR_Process.comm_world);
}

#undef FUNCNAME
#define FUNCNAME jobid
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static uint32_t jobid(void)
{
    /* not used currently */
    return 0;
}

#undef FUNCNAME
#define FUNCNAME group_id
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int group_id(rte_grp_handle_t group)
{
    MPIR_Comm *comm;
    comm = (MPIR_Comm *) group;
    return comm->context_id;
}

#undef FUNCNAME
#define FUNCNAME get_coll_handle
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static void *get_coll_handle(void)
{
    MPIR_Request *req;
    req = MPIR_Request_create(MPIR_REQUEST_KIND__COLL);
    MPIR_Request_add_ref(req);
    return (void *) req;
}

#undef FUNCNAME
#define FUNCNAME coll_handle_test
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int coll_handle_test(void *handle)
{
    int completed;
    MPIR_Request *req;
    req = (MPIR_Request *) handle;
    completed = (int) MPIR_Request_is_complete(req);
    return completed;
}

#undef FUNCNAME
#define FUNCNAME coll_handle_free
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static void coll_handle_free(void *handle)
{
    MPIR_Request *req;
    if (NULL != handle) {
        req = (MPIR_Request *) handle;
        MPIR_Request_free(req);
    }
}

#undef FUNCNAME
#define FUNCNAME coll_handle_complete
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static void coll_handle_complete(void *handle)
{
    MPIR_Request *req;
    if (NULL != handle) {
        req = (MPIR_Request *) handle;
        MPID_Request_complete(req);
    }
}

#undef FUNCNAME
#define FUNCNAME world_rank
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int world_rank(rte_grp_handle_t grp_h, rte_ec_handle_t ec)
{
#ifdef MPIDCH4_H_INCLUDED
    return MPIDI_CH4U_rank_to_lpid(ec.rank,
                                   (MPIR_Comm *) grp_h);
#else
    return ((struct MPIDI_VC *)ec.handle)->pg_rank;
#endif
}
