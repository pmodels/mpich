/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Cart_map */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Cart_map = PMPI_Cart_map
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Cart_map  MPI_Cart_map
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Cart_map as PMPI_Cart_map
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Cart_map(MPI_Comm comm, int ndims, const int dims[], const int periods[], int *newrank)
    __attribute__ ((weak, alias("PMPI_Cart_map")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Cart_map
#define MPI_Cart_map PMPI_Cart_map

/* create a communication matrix from the cartiesian properties */
double **get_cart_graph_comm_matrix(int ndims, const int dims[],
                                    const int periods[]ATTRIBUTE((unused)))
{
    double **matrix;
    int num_nodes = 1;
    int disp = 1;
    for (int i = 0; i < ndims; i++) {
        num_nodes *= dims[i];
    }

    matrix = MPL_calloc(num_nodes, sizeof(double *), MPL_MEM_OTHER);
    for (int i = 0; i < num_nodes; i++) {
        matrix[i] = MPL_calloc(num_nodes, sizeof(double), MPL_MEM_OTHER);
        for (int j = 0; j < num_nodes; j++) {
            matrix[i][j] = 1.0;
        }
    }

    /* Create the topololgy structure */
    MPIR_Topology *cart_ptr = MPL_malloc(sizeof(MPIR_Topology), MPL_MEM_OTHER);

    cart_ptr->kind = MPI_CART;
    cart_ptr->topo.cart.nnodes = num_nodes;
    cart_ptr->topo.cart.ndims = ndims;
    cart_ptr->topo.cart.dims = MPL_malloc(ndims * sizeof(int), MPL_MEM_OTHER);
    cart_ptr->topo.cart.periodic = MPL_malloc(ndims * sizeof(int), MPL_MEM_OTHER);
    cart_ptr->topo.cart.position = MPL_malloc(ndims * sizeof(int), MPL_MEM_OTHER);
    for (int i = 0; i < ndims; i++) {
        cart_ptr->topo.cart.dims[i] = dims[i];
        cart_ptr->topo.cart.periodic[i] = periods[i];
    }

    /* determine neighbors */
    for (int i = 0; i < num_nodes; i++) {

        /* set the position of rank i */
        int rank = i;
        int nranks = num_nodes;
        for (int j = 0; j < ndims; j++) {
            nranks = nranks / dims[j];
            /* FIXME: nranks could be zero (?) */
            cart_ptr->topo.cart.position[j] = rank / nranks;
            rank = rank % nranks;
        }

        /* do cart shift in each dimension */
        for (int j = 0; j < ndims; j++) {

            int pos[MAX_CART_DIM];
            int disp = 1;
            for (int k = 0; k < cart_ptr->topo.cart.ndims; k++) {
                pos[k] = cart_ptr->topo.cart.position[k];
            }
            /* We must return MPI_PROC_NULL if shifted over the edge of a
             * non-periodic mesh */
            pos[j] += disp;
            if (!cart_ptr->topo.cart.periodic[j] &&
                (pos[j] >= cart_ptr->topo.cart.dims[j] || pos[j] < 0)) {
                ;
            } else {
                /* get rank of neighbor and update comm_matrix */
                int neighbor;
                MPIR_Cart_rank_impl(cart_ptr, pos, &neighbor);
                matrix[i][neighbor] += 999.0;
            }

            pos[j] = cart_ptr->topo.cart.position[j] - disp;
            if (!cart_ptr->topo.cart.periodic[j] &&
                (pos[j] >= cart_ptr->topo.cart.dims[j] || pos[j] < 0)) {
                ;
            } else {
                /* get rank of neighbor and update comm_matrix */
                int neighbor;
                MPIR_Cart_rank_impl(cart_ptr, pos, &neighbor);
                matrix[i][neighbor] += 1.0;
            }
        }
    }

  fn_exit:
    /* free temporary memory */
    MPL_free(cart_ptr->topo.cart.dims);
    MPL_free(cart_ptr->topo.cart.periodic);
    MPL_free(cart_ptr->topo.cart.position);
    MPL_free(cart_ptr);

    return matrix;

  fn_fail:
    goto fn_exit;
}

#ifdef HAVE_SCOTCH
#include <scotch.h>

static SCOTCH_Num *verttab;     /* Vertex array [vertnbr+1] */
static SCOTCH_Num *edgetab;     /* Edge array [edgenbr]     */
static SCOTCH_Num *edlotab;     /* Edge load array          */

/* create scotch graph from communication matrix */
static int comm_matrix_to_scotch_graph(double **matrix, int n, SCOTCH_Graph * graph)
{
    int ret;

    SCOTCH_Num base;            /* Base value               */
    SCOTCH_Num vert;            /* Number of vertices       */
    SCOTCH_Num *vendtab;        /* Vertex array [vertnbr]   */
    SCOTCH_Num *velotab;        /* Vertex load array        */
    SCOTCH_Num *vlbltab;        /* Vertex label array       */
    SCOTCH_Num edge;            /* Number of edges (arcs)   */

    base = 0;
    vert = n;

    verttab = (SCOTCH_Num *) MPL_malloc((vert + 1) * sizeof(SCOTCH_Num), MPL_MEM_OTHER);
    for (int v = 0; v < vert + 1; v++) {
        verttab[v] = v * (n - 1);
    }

    vendtab = NULL;
    velotab = NULL;
    vlbltab = NULL;

    edge = n * (n - 1);

    /* Compute the lowest load to reduce of the values of the load to avoid overflow */
    double min_load = -1;
    for (int v1 = 0; v1 < vert; v1++) {
        for (int v2 = 0; v2 < vert; v2++) {
            double load = matrix[v1][v2];
            if (load >= 0.01 && (load < min_load || min_load < 0))      /* TODO set an epsilon */
                min_load = load;
        }
    }

    edgetab = (SCOTCH_Num *) MPL_malloc(n * (n - 1) * sizeof(SCOTCH_Num), MPL_MEM_OTHER);
    edlotab = (SCOTCH_Num *) MPL_malloc(n * (n - 1) * sizeof(SCOTCH_Num), MPL_MEM_OTHER);
    for (int v1 = 0; v1 < vert; v1++) {
        for (int v2 = 0; v2 < vert; v2++) {
            if (v2 == v1)
                continue;
            int idx = v1 * (n - 1) + ((v2 < v1) ? v2 : v2 - 1);
            edgetab[idx] = v2;
            edlotab[idx] = (int) (matrix[v1][v2] / min_load);
        }
    }

    ret = SCOTCH_graphBuild(graph, base, vert,
                            verttab, vendtab, velotab, vlbltab, edge, edgetab, edlotab);

    return ret;
}

int get_mapping_from_graph(SCOTCH_Graph * graph, int **ranks)
{
    int ret;

    SCOTCH_Arch *scotch_arch = MPIR_Process.arch;

    int graph_size;
    SCOTCH_graphSize(graph, &graph_size, NULL);

    SCOTCH_Strat strategy;
    SCOTCH_stratInit(&strategy);
    /* We force Scotch to use all the processes
     * barat is 0.01 as in SCOTCH_STRATDEFAULT */
    SCOTCH_stratGraphMapBuild(&strategy, SCOTCH_STRATQUALITY, graph_size, 0.01);

    /* The ranks are the indices of the nodes in the complete graph */
    int *mapped_ranks = (int *) MPL_malloc(graph_size * sizeof(int), MPL_MEM_OTHER);
    ret = SCOTCH_graphMap(graph, scotch_arch, &strategy, mapped_ranks);

    SCOTCH_stratExit(&strategy);

    if (ret != 0) {
        fprintf(stderr, "Error: SCOTCH_graphMap failed\n");
        goto ERROR;
    }

    *ranks = mapped_ranks;

  ERROR:
    if (ret == 0)
        return ret;
    MPL_free(ranks);
    return ret;
}
#endif

#undef FUNCNAME
#define FUNCNAME MPIR_Cart_map
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Cart_map(const MPIR_Comm * comm_ptr, int ndims, const int dims[],
                  const int periodic[], int *newrank)
{
    int rank, nranks, i, size, mpi_errno = MPI_SUCCESS;

    MPL_UNREFERENCED_ARG(periodic);

    /* Determine number of processes needed for topology */
    if (ndims == 0) {
        nranks = 1;
    } else {
        nranks = dims[0];
        for (i = 1; i < ndims; i++)
            nranks *= dims[i];
    }
    size = comm_ptr->remote_size;

    /* Test that the communicator is large enough */
    MPIR_ERR_CHKANDJUMP2(size < nranks, mpi_errno, MPI_ERR_DIMS, "**topotoolarge",
                         "**topotoolarge %d %d", size, nranks);

#ifdef HAVE_SCOTCH
    if (nranks > 1) {
        int *mapped_ranks;
        double **comm_matrix = NULL;
        SCOTCH_Graph *graph;

        /* create communication graph and map it to arch */
        graph = SCOTCH_graphAlloc();
        comm_matrix = get_cart_graph_comm_matrix(ndims, dims, periodic);
        comm_matrix_to_scotch_graph(comm_matrix, nranks, graph);
        get_mapping_from_graph(graph, &mapped_ranks);

        /* assign new ranks from mapping */
        *newrank = MPI_UNDEFINED;
        for (i = 0; i < nranks; i++) {
            if (MPIR_Process.arch_mapping[i] == MPIR_Comm_rank(comm_ptr)) {
                *newrank = mapped_ranks[i];
            }
        }
        MPL_free(mapped_ranks);

        SCOTCH_graphExit(graph);
        MPL_free(verttab);
        MPL_free(edgetab);
        MPL_free(edlotab);
        for (i = 0; i < nranks; i++)
            MPL_free(comm_matrix[i]);
        MPL_free(comm_matrix);

        goto fn_exit;
    }
#endif

    /* Am I in this range? */
    rank = comm_ptr->rank;
    if (rank < nranks)
        /* This relies on the ranks *not* being reordered by the current
         * Cartesian routines */
        *newrank = rank;
    else
        *newrank = MPI_UNDEFINED;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Cart_map_impl
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Cart_map_impl(const MPIR_Comm * comm_ptr, int ndims, const int dims[],
                       const int periods[], int *newrank)
{
    int mpi_errno = MPI_SUCCESS;

    if (comm_ptr->topo_fns != NULL && comm_ptr->topo_fns->cartMap != NULL) {
        /* --BEGIN USEREXTENSION-- */
        mpi_errno = comm_ptr->topo_fns->cartMap(comm_ptr, ndims,
                                                (const int *) dims, (const int *) periods, newrank);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
        /* --END USEREXTENSION-- */
    } else {
        mpi_errno = MPIR_Cart_map(comm_ptr, ndims,
                                  (const int *) dims, (const int *) periods, newrank);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Cart_map
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@
MPI_Cart_map - Maps process to Cartesian topology information

Input Parameters:
+ comm - input communicator (handle)
. ndims - number of dimensions of Cartesian structure (integer)
. dims - integer array of size 'ndims' specifying the number of processes in
  each coordinate direction
- periods - logical array of size 'ndims' specifying the periodicity
  specification in each coordinate direction

Output Parameters:
. newrank - reordered rank of the calling process; 'MPI_UNDEFINED' if
  calling process does not belong to grid (integer)

.N SignalSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_COMM
.N MPI_ERR_DIMS
.N MPI_ERR_ARG
@*/
int MPI_Cart_map(MPI_Comm comm, int ndims, const int dims[], const int periods[], int *newrank)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *comm_ptr = NULL;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_CART_MAP);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_CART_MAP);

    /* Validate parameters, especially handles needing to be converted */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_COMM(comm, mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif

    /* Convert MPI object handles to object pointers */
    MPIR_Comm_get_ptr(comm, comm_ptr);

    /* Validate parameters and objects (post conversion) */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            /* Validate comm_ptr */
            MPIR_Comm_valid_ptr(comm_ptr, mpi_errno, TRUE);
            if (mpi_errno)
                goto fn_fail;
            /* If comm_ptr is not valid, it will be reset to null */
            MPIR_ERRTEST_ARGNULL(newrank, "newrank", mpi_errno);
            MPIR_ERRTEST_ARGNULL(dims, "dims", mpi_errno);
            /* As of MPI 2.1, 0-dimensional cartesian topologies are valid */
            if (ndims < 0) {
                mpi_errno = MPIR_Err_create_code(MPI_SUCCESS,
                                                 MPIR_ERR_RECOVERABLE,
                                                 FCNAME, __LINE__, MPI_ERR_DIMS,
                                                 "**dims", "**dims %d", ndims);
                goto fn_fail;
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    mpi_errno = MPIR_Cart_map_impl(comm_ptr, ndims,
                                   (const int *) dims, (const int *) periods, newrank);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);
    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_CART_MAP);
    return mpi_errno;

    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
#ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno =
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
                                 "**mpi_cart_map", "**mpi_cart_map %C %d %p %p %p", comm, ndims,
                                 dims, periods, newrank);
    }
#endif
    mpi_errno = MPIR_Err_return_comm(comm_ptr, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
