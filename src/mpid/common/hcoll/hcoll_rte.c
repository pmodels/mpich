#include "hcoll.h"
#include "hcoll/api/hcoll_dte.h"
#include <assert.h>

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

static int get_my_ec(rte_grp_handle_t, rte_ec_handle_t * ec_handle);

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

    if (0 == world_comm_destroying) {
        MPID_Progress_test();
    }
    else {
        /* FIXME: The hcoll library needs to be updated to return
         * error codes.  The progress function pointer right now
         * expects that the function returns void. */
        ret = hcoll_do_progress();
        assert(ret == MPI_SUCCESS);
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

/* This function converts dte_general_representation data into regular iovec array which is
  used in rml
  */
static inline int count_total_dte_repeat_entries(struct dte_data_representation_t *data)
{
    unsigned int i;

    struct dte_generalized_iovec_t *dte_iovec = data->rep.general_rep->data_representation.data;
    int total_entries_number = 0;
    for (i = 0; i < dte_iovec->repeat_count; i++) {
        total_entries_number += dte_iovec->repeat[i].n_elements;
    }
    return total_entries_number;
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
    MPID_Request *request;
    MPID_Comm *comm;
    int context_offset;
    size_t size;
    mpi_errno = MPI_SUCCESS;
    context_offset = MPID_CONTEXT_INTRA_COLL;
    comm = (MPID_Comm *) grp_h;
    if (!ec_h.handle) {
        MPIR_ERR_SETANDJUMP2(mpi_errno, MPI_ERR_OTHER, "**hcoll_wrong_arg",
                             "**hcoll_wrong_arg %p %d", ec_h.handle, ec_h.rank);
    }

    if (HCOL_DTE_IS_INLINE(data)) {
        if (!buffer && !HCOL_DTE_IS_ZERO(data)) {
            MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**null_buff_ptr");
        }
        size = (size_t) data.rep.in_line_rep.data_handle.in_line.packed_size * count / 8;
        dtype = MPI_CHAR;
        mpi_errno = MPID_Irecv(buffer, size, dtype, ec_h.rank, tag, comm, context_offset, &request);
        req->data = (void *) request;
        req->status = HCOLRTE_REQUEST_ACTIVE;
    }
    else {
        int total_entries_number;
        int i;
        unsigned int j;
        void *buf;
        uint64_t len;
        int repeat_count;
        struct dte_struct_t *repeat;
        if (NULL != buffer) {
            /* We have a full data description & buffer pointer simultaneously.
             * It is ambiguous. Throw a warning since the user might have made a
             * mistake with data reps */
            MPIU_DBG_MSG(CH3_OTHER, VERBOSE, "Warning: buffer_pointer != NULL for NON-inline data "
                         "representation: buffer_pointer is ignored");
        }
        total_entries_number = count_total_dte_repeat_entries(&data);
        repeat = data.rep.general_rep->data_representation.data->repeat;
        repeat_count = data.rep.general_rep->data_representation.data->repeat_count;
        for (i = 0; i < repeat_count; i++) {
            for (j = 0; j < repeat[i].n_elements; j++) {
                char *repeat_unit = (char *) &repeat[i];
                buf = (void *) (repeat_unit + repeat[i].elements[j].base_offset);
                len = repeat[i].elements[j].packed_size;
                recv_nb(DTE_BYTE, len, buf, ec_h, grp_h, tag, req);
            }
        }
    }
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
    MPID_Request *request;
    MPID_Comm *comm;
    int context_offset;
    size_t size;
    mpi_errno = MPI_SUCCESS;
    context_offset = MPID_CONTEXT_INTRA_COLL;
    comm = (MPID_Comm *) grp_h;
    if (!ec_h.handle) {
        MPIR_ERR_SETANDJUMP2(mpi_errno, MPI_ERR_OTHER, "**hcoll_wrong_arg",
                             "**hcoll_wrong_arg %p %d", ec_h.handle, ec_h.rank);
    }

    if (HCOL_DTE_IS_INLINE(data)) {
        if (!buffer && !HCOL_DTE_IS_ZERO(data)) {
            MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**null_buff_ptr");
        }
        size = (size_t) data.rep.in_line_rep.data_handle.in_line.packed_size * count / 8;
        dtype = MPI_CHAR;
        mpi_errno = MPID_Isend(buffer, size, dtype, ec_h.rank, tag, comm, context_offset, &request);
        req->data = (void *) request;
        req->status = HCOLRTE_REQUEST_ACTIVE;
    }
    else {
        int total_entries_number;
        int i;
        unsigned int j;
        void *buf;
        uint64_t len;
        int repeat_count;
        struct dte_struct_t *repeat;
        if (NULL != buffer) {
            /* We have a full data description & buffer pointer simultaneously.
             * It is ambiguous. Throw a warning since the user might have made a
             * mistake with data reps */
            MPIU_DBG_MSG(CH3_OTHER, VERBOSE, "Warning: buffer_pointer != NULL for NON-inline data "
                         "representation: buffer_pointer is ignored");
        }
        total_entries_number = count_total_dte_repeat_entries(&data);
        repeat = data.rep.general_rep->data_representation.data->repeat;
        repeat_count = data.rep.general_rep->data_representation.data->repeat_count;
        for (i = 0; i < repeat_count; i++) {
            for (j = 0; j < repeat[i].n_elements; j++) {
                char *repeat_unit = (char *) &repeat[i];
                buf = (void *) (repeat_unit + repeat[i].elements[j].base_offset);
                len = repeat[i].elements[j].packed_size;
                send_nb(DTE_BYTE, len, buf, ec_h, grp_h, tag, req);
            }
        }
    }
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
    MPID_Request *req;
    req = (MPID_Request *) request->data;
    if (HCOLRTE_REQUEST_ACTIVE != request->status) {
        *completed = true;
        return HCOLL_SUCCESS;
    }

    *completed = (int) MPID_Request_is_complete(req);
    if (*completed) {
        MPID_Request_release(req);
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
    MPID_Comm *comm;
    comm = (MPID_Comm *) grp_h;
    for (i = 0; i < num_ec; i++) {
        ec_handles[i].rank = ec_indexes[i];
        ec_handles[i].handle = (void *) (comm->vcrt->vcr_table[ec_indexes[i]]);
    }
    return HCOLL_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME get_my_ec
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int get_my_ec(rte_grp_handle_t grp_h, rte_ec_handle_t * ec_handle)
{
    MPID_Comm *comm;
    comm = (MPID_Comm *) grp_h;
    int my_rank = MPIR_Comm_rank(comm);
    ec_handle->handle = (void *) (comm->vcrt->vcr_table[my_rank]);
    ec_handle->rank = my_rank;
    return HCOLL_SUCCESS;
}


#undef FUNCNAME
#define FUNCNAME group_size
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int group_size(rte_grp_handle_t grp_h)
{
    return MPIR_Comm_size((MPID_Comm *) grp_h);
}

#undef FUNCNAME
#define FUNCNAME my_rank
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int my_rank(rte_grp_handle_t grp_h)
{
    return MPIR_Comm_rank((MPID_Comm *) grp_h);
}

#undef FUNCNAME
#define FUNCNAME ec_on_local_node
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int ec_on_local_node(rte_ec_handle_t ec, rte_grp_handle_t group)
{
    MPID_Comm *comm;
    MPID_Node_id_t nodeid, my_nodeid;
    int my_rank;
    comm = (MPID_Comm *) group;
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
    MPID_Comm *comm;
    comm = (MPID_Comm *) group;
    return comm->context_id;
}

#undef FUNCNAME
#define FUNCNAME get_coll_handle
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static void *get_coll_handle(void)
{
    MPID_Request *req;
    req = MPID_Request_create();
    req->kind = MPID_COLL_REQUEST;
    return (void *) req;
}

#undef FUNCNAME
#define FUNCNAME coll_handle_test
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int coll_handle_test(void *handle)
{
    int completed;
    MPID_Request *req;
    req = (MPID_Request *) handle;
    completed = (int) MPID_Request_is_complete(req);
    return completed;
}

#undef FUNCNAME
#define FUNCNAME coll_handle_free
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static void coll_handle_free(void *handle)
{
    MPID_Request *req;
    if (NULL != handle) {
        req = (MPID_Request *) handle;
        MPID_Request_release(req);
    }
}

#undef FUNCNAME
#define FUNCNAME coll_handle_complete
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static void coll_handle_complete(void *handle)
{
    MPID_Request *req;
    if (NULL != handle) {
        req = (MPID_Request *) handle;
        MPID_Request_complete(req);
    }
}

#undef FUNCNAME
#define FUNCNAME world_rank
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int world_rank(rte_grp_handle_t grp_h, rte_ec_handle_t ec)
{
    return (MPIR_Process.comm_world->rank);
}
