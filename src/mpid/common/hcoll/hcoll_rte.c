/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "mpidimpl.h"
#include "hcoll.h"
#include "hcoll/api/hcoll_dte.h"
#include "hcoll_dtypes.h"

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

static void progress(void)
{
    int ret;
    int made_progress;

    if (0 == world_comm_destroying) {
        MPID_Progress_test();
    } else {
        /* FIXME: The hcoll library needs to be updated to return
         * error codes.  The progress function pointer right now
         * expects that the function returns void. */
        ret = hcoll_do_progress(&made_progress);
        MPIR_Assert(ret == MPI_SUCCESS);
    }
}

#if HCOLL_API >= HCOLL_VERSION(3,6)
static int get_mpi_type_envelope(void *mpi_type, int *num_integers,
                                 int *num_addresses, int *num_datatypes,
                                 hcoll_mpi_type_combiner_t * combiner);
static int get_mpi_type_contents(void *mpi_type, int max_integers, int max_addresses,
                                 int max_datatypes, int *array_of_integers,
                                 void *array_of_addresses, void *array_of_datatypes);
static int get_hcoll_type(void *mpi_type, dte_data_representation_t * hcoll_type);
static int set_hcoll_type(void *mpi_type, dte_data_representation_t hcoll_type);
static int get_mpi_constants(size_t * mpi_datatype_size,
                             int *mpi_order_c, int *mpi_order_fortran,
                             int *mpi_distribute_block,
                             int *mpi_distribute_cyclic,
                             int *mpi_distribute_none, int *mpi_distribute_dflt_darg);
#endif

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
#if HCOLL_API >= HCOLL_VERSION(3,6)
    hcoll_rte_functions.rte_get_mpi_type_envelope_fn = get_mpi_type_envelope;
    hcoll_rte_functions.rte_get_mpi_type_contents_fn = get_mpi_type_contents;
    hcoll_rte_functions.rte_get_hcoll_type_fn = get_hcoll_type;
    hcoll_rte_functions.rte_set_hcoll_type_fn = set_hcoll_type;
    hcoll_rte_functions.rte_get_mpi_constants_fn = get_mpi_constants;
#endif
}

void hcoll_rte_fns_setup(void)
{
    init_module_fns();
}

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

static int ec_handle_compare(rte_ec_handle_t handle_1,
                             rte_grp_handle_t
                             group_handle_1,
                             rte_ec_handle_t handle_2, rte_grp_handle_t group_handle_2)
{
    return handle_1.handle == handle_2.handle;
}

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

static int group_size(rte_grp_handle_t grp_h)
{
    return MPIR_Comm_size((MPIR_Comm *) grp_h);
}

static int my_rank(rte_grp_handle_t grp_h)
{
    return MPIR_Comm_rank((MPIR_Comm *) grp_h);
}

static int ec_on_local_node(rte_ec_handle_t ec, rte_grp_handle_t group)
{
    MPIR_Comm *comm;
    int nodeid, my_nodeid;
    int my_rank;
    comm = (MPIR_Comm *) group;
    MPID_Get_node_id(comm, ec.rank, &nodeid);
    my_rank = MPIR_Comm_rank(comm);
    MPID_Get_node_id(comm, my_rank, &my_nodeid);
    return (nodeid == my_nodeid);
}


static rte_grp_handle_t get_world_group_handle(void)
{
    return (rte_grp_handle_t) (MPIR_Process.comm_world);
}

static uint32_t jobid(void)
{
    /* not used currently */
    return 0;
}

static int group_id(rte_grp_handle_t group)
{
    MPIR_Comm *comm;
    comm = (MPIR_Comm *) group;
    return comm->context_id;
}

static void *get_coll_handle(void)
{
    MPIR_Request *req;
    req = MPIR_Request_create(MPIR_REQUEST_KIND__COLL, 0);
    MPIR_Request_add_ref(req);
    return (void *) req;
}

static int coll_handle_test(void *handle)
{
    int completed;
    MPIR_Request *req;
    req = (MPIR_Request *) handle;
    completed = (int) MPIR_Request_is_complete(req);
    return completed;
}

static void coll_handle_free(void *handle)
{
    MPIR_Request *req;
    if (NULL != handle) {
        req = (MPIR_Request *) handle;
        MPIR_Request_free(req);
    }
}

static void coll_handle_complete(void *handle)
{
    MPIR_Request *req;
    if (NULL != handle) {
        req = (MPIR_Request *) handle;
        MPIR_Request_complete(req);
    }
}

static int world_rank(rte_grp_handle_t grp_h, rte_ec_handle_t ec)
{
#ifdef MPIDCH4_H_INCLUDED
    return MPIDIU_rank_to_lpid(ec.rank, (MPIR_Comm *) grp_h);
#else
    return ((struct MPIDI_VC *) ec.handle)->pg_rank;
#endif
}

#if HCOLL_API >= HCOLL_VERSION(3,6)
hcoll_mpi_type_combiner_t mpi_combiner_2_hcoll_combiner(int combiner)
{
    switch (combiner) {
        case MPI_COMBINER_CONTIGUOUS:
            return HCOLL_MPI_COMBINER_CONTIGUOUS;
        case MPI_COMBINER_VECTOR:
            return HCOLL_MPI_COMBINER_VECTOR;
        case MPI_COMBINER_HVECTOR:
            return HCOLL_MPI_COMBINER_HVECTOR;
        case MPI_COMBINER_INDEXED:
            return HCOLL_MPI_COMBINER_INDEXED;
        case MPI_COMBINER_HINDEXED_INTEGER:
        case MPI_COMBINER_HINDEXED:
            return HCOLL_MPI_COMBINER_HINDEXED;
        case MPI_COMBINER_DUP:
            return HCOLL_MPI_COMBINER_DUP;
        case MPI_COMBINER_INDEXED_BLOCK:
            return HCOLL_MPI_COMBINER_INDEXED_BLOCK;
        case MPI_COMBINER_HINDEXED_BLOCK:
            return HCOLL_MPI_COMBINER_HINDEXED_BLOCK;
        case MPI_COMBINER_SUBARRAY:
            return HCOLL_MPI_COMBINER_SUBARRAY;
        case MPI_COMBINER_DARRAY:
            return HCOLL_MPI_COMBINER_DARRAY;
        case MPI_COMBINER_F90_REAL:
            return HCOLL_MPI_COMBINER_F90_REAL;
        case MPI_COMBINER_F90_COMPLEX:
            return HCOLL_MPI_COMBINER_F90_COMPLEX;
        case MPI_COMBINER_F90_INTEGER:
            return HCOLL_MPI_COMBINER_F90_INTEGER;
        case MPI_COMBINER_RESIZED:
            return HCOLL_MPI_COMBINER_RESIZED;
        case MPI_COMBINER_STRUCT:
        case MPI_COMBINER_STRUCT_INTEGER:
            return HCOLL_MPI_COMBINER_STRUCT;
        default:
            break;
    }
    return HCOLL_MPI_COMBINER_LAST;
}

static int get_mpi_type_envelope(void *mpi_type, int *num_integers,
                                 int *num_addresses, int *num_datatypes,
                                 hcoll_mpi_type_combiner_t * combiner)
{
    int mpi_combiner;
    MPI_Datatype dt_handle = (MPI_Datatype) (intptr_t) mpi_type;

    MPIR_Type_get_envelope(dt_handle, num_integers, num_addresses, num_datatypes, &mpi_combiner);

    *combiner = mpi_combiner_2_hcoll_combiner(mpi_combiner);

    return HCOLL_SUCCESS;
}

static int get_mpi_type_contents(void *mpi_type, int max_integers, int max_addresses,
                                 int max_datatypes, int *array_of_integers,
                                 void *array_of_addresses, void *array_of_datatypes)
{
    int ret;
    MPI_Datatype dt_handle = (MPI_Datatype) (intptr_t) mpi_type;

    ret = MPIR_Type_get_contents(dt_handle,
                                 max_integers, max_addresses, max_datatypes,
                                 array_of_integers,
                                 (MPI_Aint *) array_of_addresses,
                                 (MPI_Datatype *) array_of_datatypes);

    return ret == MPI_SUCCESS ? HCOLL_SUCCESS : HCOLL_ERROR;
}

static int get_hcoll_type(void *mpi_type, dte_data_representation_t * hcoll_type)
{
    MPI_Datatype dt_handle = (MPI_Datatype) (intptr_t) mpi_type;
    MPIR_Datatype *dt_ptr;

    *hcoll_type = mpi_dtype_2_hcoll_dtype(dt_handle, -1, TRY_FIND_DERIVED);

    return HCOL_DTE_IS_ZERO((*hcoll_type)) ? HCOLL_ERR_NOT_FOUND : HCOLL_SUCCESS;
}

static int set_hcoll_type(void *mpi_type, dte_data_representation_t hcoll_type)
{
    return HCOLL_SUCCESS;
}

static int get_mpi_constants(size_t * mpi_datatype_size,
                             int *mpi_order_c, int *mpi_order_fortran,
                             int *mpi_distribute_block,
                             int *mpi_distribute_cyclic,
                             int *mpi_distribute_none, int *mpi_distribute_dflt_darg)
{
    *mpi_datatype_size = sizeof(MPI_Datatype);
    *mpi_order_c = MPI_ORDER_C;
    *mpi_order_fortran = MPI_ORDER_FORTRAN;
    *mpi_distribute_block = MPI_DISTRIBUTE_BLOCK;
    *mpi_distribute_cyclic = MPI_DISTRIBUTE_CYCLIC;
    *mpi_distribute_none = MPI_DISTRIBUTE_NONE;
    *mpi_distribute_dflt_darg = MPI_DISTRIBUTE_DFLT_DARG;

    return HCOLL_SUCCESS;
}

#endif
