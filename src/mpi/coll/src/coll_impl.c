/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "coll_impl.h"
#include "coll_csel.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

categories :
   - name : COLLECTIVE
     description : A category for collective communication variables.

cvars:
    - name        : MPIR_CVAR_COLLECTIVE_FALLBACK
      category    : COLLECTIVE
      type        : enum
      default     : silent
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : |-
        Variable to control what the MPI library should do if the
        user-specified collective algorithm does not work for the
        arguments passed in by the user.
        error   - throw an error
        print   - print an error message and fallback to the internally selected algorithm
        silent  - silently fallback to the internally selected algorithm

    - name        : MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE
      category    : COLLECTIVE
      type        : string
      default     : ""
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Defines the location of tuning file.

    - name        : MPIR_CVAR_COLL_SELECTION_JSON_FILE
      category    : COLLECTIVE
      type        : string
      default     : ""
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Defines the location of tuning file that selects basic collective algorithms.

    - name        : MPIR_CVAR_COLL_COMPOSITION_JSON_FILE
      category    : COLLECTIVE
      type        : string
      default     : ""
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Defines the location of tuning file that selects composition collective algorithms.

    - name        : MPIR_CVAR_HIERARCHY_DUMP
      category    : COLLECTIVE
      type        : boolean
      default     : false
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        If set to true, each rank will dump the hierarchy data structure to a file named "hierarchy[rank]" in the current folder.
        If set to false, the hierarchy data structure will not be dumped.

    - name        : MPIR_CVAR_COORDINATES_FILE
      category    : COLLECTIVE
      type        : string
      default     : ""
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Defines the location of the input coordinates file.

    - name        : MPIR_CVAR_COLL_TREE_DUMP
      category    : COLLECTIVE
      type        : boolean
      default     : false
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        If set to true, each rank will dump the tree to a file named "colltree[rank].json" in the current folder.
        If set to false, the tree will not be dumped.

    - name        : MPIR_CVAR_COORDINATES_DUMP
      category    : COLLECTIVE
      type        : boolean
      default     : false
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        If set to true, rank 0 will dump the network coordinates to a file named "coords" in the current folder.
        If set to false, the network coordinates will not be dumped.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

int MPIR_Nbc_progress_hook_id = 0;

MPIR_Tree_type_t MPIR_Iallreduce_tree_type = MPIR_TREE_TYPE_KARY;
MPIR_Tree_type_t MPIR_Allreduce_tree_type = MPIR_TREE_TYPE_KARY;
MPIR_Tree_type_t MPIR_Ibcast_tree_type = MPIR_TREE_TYPE_KARY;
MPIR_Tree_type_t MPIR_Bcast_tree_type = MPIR_TREE_TYPE_KARY;
MPIR_Tree_type_t MPIR_Ireduce_tree_type = MPIR_TREE_TYPE_KARY;
void *MPIR_Csel_root = NULL;
const char *MPIR_Csel_source;

/* TODO: remove the old MPIR_Csel_root etc. */
void *MPIR_Csel_composition = NULL;
void *MPIR_Csel_selection = NULL;

/* table of all collective algorithms */
MPIR_Coll_algo_fn *MPIR_Coll_algo_table;

MPIR_Tree_type_t get_tree_type_from_string(const char *tree_str)
{
    MPIR_Tree_type_t tree_type = MPIR_TREE_TYPE_KARY;
    if (0 == strcmp(tree_str, "kary"))
        tree_type = MPIR_TREE_TYPE_KARY;
    else if (0 == strcmp(tree_str, "knomial_1"))
        tree_type = MPIR_TREE_TYPE_KNOMIAL_1;
    else if (0 == strcmp(tree_str, "knomial_2"))
        tree_type = MPIR_TREE_TYPE_KNOMIAL_2;
    else
        tree_type = MPIR_TREE_TYPE_KARY;
    return tree_type;
}

static MPIR_Tree_type_t get_tree_type_from_string_with_topo(const char *tree_str)
{
    MPIR_Tree_type_t tree_type = MPIR_TREE_TYPE_KARY;
    if (0 == strcmp(tree_str, "kary"))
        tree_type = MPIR_TREE_TYPE_KARY;
    else if (0 == strcmp(tree_str, "knomial_1"))
        tree_type = MPIR_TREE_TYPE_KNOMIAL_1;
    else if (0 == strcmp(tree_str, "knomial_2"))
        tree_type = MPIR_TREE_TYPE_KNOMIAL_2;
    else if (0 == strcmp(tree_str, "topology_aware"))
        tree_type = MPIR_TREE_TYPE_TOPOLOGY_AWARE;
    else if (0 == strcmp(tree_str, "topology_aware_k"))
        tree_type = MPIR_TREE_TYPE_TOPOLOGY_AWARE_K;
    else if (0 == strcmp(tree_str, "topology_wave"))
        tree_type = MPIR_TREE_TYPE_TOPOLOGY_WAVE;
    else
        tree_type = MPIR_TREE_TYPE_KARY;
    return tree_type;
}

int get_ccl_from_string(const char *ccl_str)
{
    int ccl = -1;
    if (0 == strcmp(ccl_str, "auto"))
        ccl = MPIR_CVAR_ALLREDUCE_CCL_auto;
    else if (0 == strcmp(ccl_str, "nccl"))
        ccl = MPIR_CVAR_ALLREDUCE_CCL_nccl;
    else if (0 == strcmp(ccl_str, "rccl"))
        ccl = MPIR_CVAR_ALLREDUCE_CCL_rccl;
    return ccl;
}

#define LOAD_CSEL_JSON(csel_var, cvar_name, builtin_str) \
   do { \
        if (!strcmp(cvar_name, "")) { \
            mpi_errno = MPIR_Csel_create_from_buf(builtin_str, MPII_Create_container, &csel_var); \
        } else { \
            mpi_errno = MPIR_Csel_create_from_file(cvar_name, MPII_Create_container, &csel_var); \
        } \
        MPIR_ERR_CHECK(mpi_errno); \
   } while (0)

int MPII_Coll_init(void)
{
    int mpi_errno = MPI_SUCCESS;

    /* Iallreduce */
    MPIR_Iallreduce_tree_type = get_tree_type_from_string(MPIR_CVAR_IALLREDUCE_TREE_TYPE);

    /* Allreduce */
    MPIR_Allreduce_tree_type = get_tree_type_from_string_with_topo(MPIR_CVAR_ALLREDUCE_TREE_TYPE);

    /* Ibcast */
    MPIR_Ibcast_tree_type = get_tree_type_from_string(MPIR_CVAR_IBCAST_TREE_TYPE);

    /* Bcast */
    MPIR_Bcast_tree_type = get_tree_type_from_string_with_topo(MPIR_CVAR_BCAST_TREE_TYPE);

    /* Ireduce */
    MPIR_Ireduce_tree_type = get_tree_type_from_string_with_topo(MPIR_CVAR_IREDUCE_TREE_TYPE);

    /* register non blocking collectives progress hook */
    mpi_errno = MPIR_Progress_hook_register(-1, MPIDU_Sched_progress, &MPIR_Nbc_progress_hook_id);
    MPIR_ERR_CHECK(mpi_errno);

    /* initialize transports */
    mpi_errno = MPII_TSP_init();
    MPIR_ERR_CHECK(mpi_errno);

    /* initialize algorithms */
    mpi_errno = MPII_Stubalgo_init();
    MPIR_ERR_CHECK(mpi_errno);
    mpi_errno = MPII_Treealgo_init();
    MPIR_ERR_CHECK(mpi_errno);
    mpi_errno = MPII_Recexchalgo_init();
    MPIR_ERR_CHECK(mpi_errno);

    /* initialize selection tree */
    if (!strcmp(MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE, "")) {
        mpi_errno = MPIR_Csel_create_from_buf(MPII_coll_generic_json,
                                              MPII_Create_container, &MPIR_Csel_root);
        MPIR_Csel_source = "MPII_coll_generic_json";
    } else {
        mpi_errno = MPIR_Csel_create_from_file(MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE,
                                               MPII_Create_container, &MPIR_Csel_root);
        MPIR_Csel_source = MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE;
    }
    MPIR_ERR_CHECK(mpi_errno);
    /* TODO: remove the old MPIR_Csel_root etc. */

    LOAD_CSEL_JSON(MPIR_Csel_composition,
                   MPIR_CVAR_COLL_COMPOSITION_JSON_FILE, MPII_coll_composition_json);
    LOAD_CSEL_JSON(MPIR_Csel_selection,
                   MPIR_CVAR_COLL_SELECTION_JSON_FILE, MPII_coll_selection_json);

    MPIR_Coll_algo_table = MPL_malloc(MPII_CSEL_CONTAINER_TYPE__ALGORITHM__Algorithm_count *
                                      sizeof(MPIR_Coll_algo_fn), MPL_MEM_COLL);
    MPIR_Coll_algo_init();

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPII_Coll_finalize(void)
{
    int mpi_errno = MPI_SUCCESS;

    /* deregister non blocking collectives progress hook */
    MPIR_Progress_hook_deregister(MPIR_Nbc_progress_hook_id);

    mpi_errno = MPII_TSP_finalize();
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIR_Csel_free(MPIR_Csel_root);
    MPIR_ERR_CHECK(mpi_errno);

    MPL_free(MPIR_Coll_algo_table);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* Function used by CH3 progress engine to decide whether to
 * block for a recv operation */
int MPIR_Coll_safe_to_block(void)
{
    return MPII_TSP_scheds_are_pending() == false;
}

/* Function to initialize communicators for collectives */
int MPIR_Coll_comm_init(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;

    /* initialize any stub algo related data structures */
    mpi_errno = MPII_Stubalgo_comm_init(comm);
    MPIR_ERR_CHECK(mpi_errno);
    /* initialize any tree algo related data structures */
    mpi_errno = MPII_Treealgo_comm_init(comm);
    MPIR_ERR_CHECK(mpi_errno);

    /* initialize any transport data structures */
    mpi_errno = MPII_TSP_comm_init(comm);
    MPIR_ERR_CHECK(mpi_errno);

    /* initialize algorithms */
    mpi_errno = MPII_Recexchalgo_comm_init(comm);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    mpi_errno = MPIR_Csel_prune(MPIR_Csel_root, comm, &comm->csel_comm);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* Function to cleanup any communicators for collectives */
int MPII_Coll_comm_cleanup(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Csel_free(comm->csel_comm);
    MPIR_ERR_CHECK(mpi_errno);

    /* cleanup all collective communicators */
    mpi_errno = MPII_Stubalgo_comm_cleanup(comm);
    MPIR_ERR_CHECK(mpi_errno);
    mpi_errno = MPII_Treealgo_comm_cleanup(comm);
    MPIR_ERR_CHECK(mpi_errno);

    /* cleanup transport data */
    mpi_errno = MPII_TSP_comm_cleanup(comm);
    MPIR_ERR_CHECK(mpi_errno);

    /* initialize algorithms */
    mpi_errno = MPII_Recexchalgo_comm_cleanup(comm);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* For reduction-type collective, this routine swaps the (potential) device buffers
 * with allocated host-buffer */
void MPIR_Coll_host_buffer_alloc(const void *sendbuf, const void *recvbuf, MPI_Aint count,
                                 MPI_Datatype datatype, void **host_sendbuf, void **host_recvbuf)
{
    void *tmp;
    if (sendbuf != MPI_IN_PLACE) {
        tmp = MPIR_gpu_host_swap(sendbuf, count, datatype);
        *host_sendbuf = tmp;
    } else {
        *host_sendbuf = NULL;
    }

    if (recvbuf == NULL) {
        *host_recvbuf = NULL;
    } else if (sendbuf == MPI_IN_PLACE) {
        tmp = MPIR_gpu_host_swap(recvbuf, count, datatype);
        *host_recvbuf = tmp;
    } else {
        tmp = MPIR_gpu_host_alloc(recvbuf, count, datatype);
        *host_recvbuf = tmp;
    }
}

void MPIR_Coll_host_buffer_free(void *host_sendbuf, void *host_recvbuf)
{
    MPL_free(host_sendbuf);
    MPL_free(host_recvbuf);
}

void MPIR_Coll_host_buffer_swap_back(void *host_sendbuf, void *host_recvbuf, void *in_recvbuf,
                                     MPI_Aint count, MPI_Datatype datatype, MPIR_Request * request)
{
    if (!host_sendbuf && !host_recvbuf) {
        /* no copy (or free) at completion necessary, just return */
        return;
    }

    if (request == NULL || MPIR_Request_is_complete(request)) {
        /* operation is complete, copy the data and return */
        if (host_sendbuf) {
            MPIR_gpu_host_free(host_sendbuf, count, datatype);
        }
        if (host_recvbuf) {
            MPIR_gpu_swap_back(host_recvbuf, in_recvbuf, count, datatype);
        }
        return;
    }

    /* data will be copied later during request completion */
    request->u.nbc.coll.host_sendbuf = host_sendbuf;
    request->u.nbc.coll.host_recvbuf = host_recvbuf;
    if (host_recvbuf) {
        request->u.nbc.coll.user_recvbuf = in_recvbuf;
    }
    request->u.nbc.coll.count = count;
    request->u.nbc.coll.datatype = datatype;
    MPIR_Datatype_add_ref_if_not_builtin(datatype);
}

void MPIR_Coll_host_buffer_persist_set(void *host_sendbuf, void *host_recvbuf, void *in_recvbuf,
                                       MPI_Aint count, MPI_Datatype datatype,
                                       MPIR_Request * request)
{
    if (!host_sendbuf && !host_recvbuf) {
        /* no copy (or free) at completion necessary, just return */
        return;
    }

    /* data will be copied later during request completion */
    request->u.persist_coll.coll.host_sendbuf = host_sendbuf;
    request->u.persist_coll.coll.host_recvbuf = host_recvbuf;
    if (host_recvbuf) {
        request->u.persist_coll.coll.user_recvbuf = in_recvbuf;
        request->u.persist_coll.coll.count = count;
        request->u.persist_coll.coll.datatype = datatype;
        MPIR_Datatype_add_ref_if_not_builtin(datatype);
    }
}

void MPIR_Coll_algo_init(void)
{
    /* manual entries now, but we will replace it with autogen later */
    MPIR_Coll_algo_table[MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Coll_auto] = MPIR_Coll_auto;
}

int MPIR_Coll_composition_auto(MPIR_Csel_coll_sig_s * coll_sig)
{
    int mpi_errno = MPI_SUCCESS;

    /* TODO: need a mechanism in coll_sig so we can assert and prevent a dead recursion loop */

    MPII_Csel_container_s *cnt = MPIR_Csel_search(MPIR_Csel_composition, coll_sig);
    MPIR_ERR_CHKANDJUMP(!cnt, mpi_errno, MPI_ERR_OTHER, "**csel_noresult");

    mpi_errno = MPIR_Coll_algo_table[cnt->id] (coll_sig, cnt);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Coll_auto(MPIR_Csel_coll_sig_s * coll_sig, MPII_Csel_container_s * me)
{
    int mpi_errno = MPI_SUCCESS;

    MPII_Csel_container_s *cnt = MPIR_Csel_search(MPIR_Csel_selection, coll_sig);
    MPIR_ERR_CHKANDJUMP(!cnt, mpi_errno, MPI_ERR_OTHER, "**csel_noresult");

    /* TODO: assert the selected algorithm is not a composition algorithm */

    mpi_errno = MPIR_Coll_algo_table[cnt->id] (coll_sig, cnt);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* blocking collectives by calling its nonblocking forms */
int MPIR_Coll_nb(MPIR_Csel_coll_sig_s * coll_sig, MPII_Csel_container_s * me)
{
    int mpi_errno = MPI_SUCCESS;

    /* Trick: blocking coll_type is even. Its nonblocking type is +1 */
    MPIR_Assert(coll_sig->coll_type % 2 == 0);
    coll_sig->coll_type += 1;

    mpi_errno = MPIR_Coll_auto(coll_sig, NULL);
    MPIR_ERR_CHECK(mpi_errno);

    MPIR_Request *req;
    MPII_SCHED_START(coll_sig->sched_type, coll_sig->sched, coll_sig->comm_ptr, &req);

    mpi_errno = MPIC_Wait(req);
    MPIR_ERR_CHECK(mpi_errno);
    MPIR_Request_free(req);

    /* clean up coll_sig just in case */
    coll_sig->coll_type -= 1;
    coll_sig->sched = NULL;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
