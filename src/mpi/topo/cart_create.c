/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Cart_create */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Cart_create = PMPI_Cart_create
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Cart_create  MPI_Cart_create
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Cart_create as PMPI_Cart_create
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Cart_create(MPI_Comm comm_old, int ndims, const int dims[], const int periods[],
                    int reorder, MPI_Comm * comm_cart)
    __attribute__ ((weak, alias("PMPI_Cart_create")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Cart_create
#define MPI_Cart_create PMPI_Cart_create

#endif

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef FUNCNAME
#define FUNCNAME MPIR_Cart_create
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Cart_create(MPIR_Comm * comm_ptr, int ndims, const int dims[],
                     const int periods[], int reorder, MPI_Comm * comm_cart)
{
    int i, newsize, rank, nranks, mpi_errno = MPI_SUCCESS;
    MPIR_Comm *newcomm_ptr = NULL;
    MPIR_Topology *cart_ptr = NULL;
    MPIR_CHKPMEM_DECL(4);

    /* Set this as null incase we exit with an error */
    *comm_cart = MPI_COMM_NULL;

    /* Check for invalid arguments */
    newsize = 1;
    for (i = 0; i < ndims; i++)
        newsize *= dims[i];

    /* Use ERR_ARG instead of ERR_TOPOLOGY because there is no topology yet */
    MPIR_ERR_CHKANDJUMP2((newsize > comm_ptr->remote_size), mpi_errno,
                         MPI_ERR_ARG, "**cartdim",
                         "**cartdim %d %d", comm_ptr->remote_size, newsize);

    if (ndims == 0) {
        /* specified as a 0D Cartesian topology in MPI 2.1.
         * Rank 0 returns a dup of COMM_SELF with the topology info attached.
         * Others return MPI_COMM_NULL. */

        rank = comm_ptr->rank;

        if (rank == 0) {
            MPIR_Comm *comm_self_ptr;
            MPIR_Comm_get_ptr(MPI_COMM_SELF, comm_self_ptr);
            mpi_errno = MPIR_Comm_dup_impl(comm_self_ptr, &newcomm_ptr);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);

            /* Create the topology structure */
            MPIR_CHKPMEM_MALLOC(cart_ptr, MPIR_Topology *, sizeof(MPIR_Topology),
                                mpi_errno, "cart_ptr", MPL_MEM_COMM);

            cart_ptr->kind = MPI_CART;
            cart_ptr->topo.cart.nnodes = 1;
            cart_ptr->topo.cart.ndims = 0;

            /* make mallocs of size 1 int so that they get freed as part of the
             * normal free mechanism */

            MPIR_CHKPMEM_MALLOC(cart_ptr->topo.cart.dims, int *, sizeof(int),
                                mpi_errno, "cart.dims", MPL_MEM_COMM);
            MPIR_CHKPMEM_MALLOC(cart_ptr->topo.cart.periodic, int *, sizeof(int),
                                mpi_errno, "cart.periodic", MPL_MEM_COMM);
            MPIR_CHKPMEM_MALLOC(cart_ptr->topo.cart.position, int *, sizeof(int),
                                mpi_errno, "cart.position", MPL_MEM_COMM);
        } else {
            *comm_cart = MPI_COMM_NULL;
            goto fn_exit;
        }
    } else {

        /* Create a new communicator as a duplicate of the input communicator
         * (but do not duplicate the attributes) */
        if (reorder) {

            double **comm_matrix;
            int newrank = (rank == MPI_UNDEFINED ? MPI_UNDEFINED : 1);
            int comm_size;
            /* Allow the cart map routine to remap the assignment of ranks to
             * processes */
            mpi_errno = MPIR_Cart_map_impl(comm_ptr, ndims, (const int *) dims,
                                           (const int *) periods, &rank);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);

#ifdef HAVE_NETLOC
            comm_size = MPIR_Comm_size(comm_ptr);
            MPIR_Netloc_get_cart_graph_comm_matrix(ndims, dims, periods, &comm_matrix);
            MPIR_Netloc_get_reordered_rank(rank, &newrank, comm_size, comm_matrix);
#endif
            /* Create the new communicator with split, since we need to reorder
             * the ranks (including the related internals, such as the connection
             * tables */
            mpi_errno = MPIR_Comm_split_impl(comm_ptr, newrank, rank, &newcomm_ptr);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);
            if (comm_matrix != NULL) {
                MPL_free(comm_matrix);
            }

        } else {
            mpi_errno = MPII_Comm_copy((MPIR_Comm *) comm_ptr, newsize, &newcomm_ptr);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);
            rank = comm_ptr->rank;
        }

        /* If this process is not in the resulting communicator, return a
         * null communicator and exit */
        if (rank >= newsize || rank == MPI_UNDEFINED) {
            *comm_cart = MPI_COMM_NULL;
            goto fn_exit;
        }

        /* Create the topololgy structure */
        MPIR_CHKPMEM_MALLOC(cart_ptr, MPIR_Topology *, sizeof(MPIR_Topology),
                            mpi_errno, "cart_ptr", MPL_MEM_COMM);

        cart_ptr->kind = MPI_CART;
        cart_ptr->topo.cart.nnodes = newsize;
        cart_ptr->topo.cart.ndims = ndims;
        MPIR_CHKPMEM_MALLOC(cart_ptr->topo.cart.dims, int *, ndims * sizeof(int),
                            mpi_errno, "cart.dims", MPL_MEM_COMM);
        MPIR_CHKPMEM_MALLOC(cart_ptr->topo.cart.periodic, int *, ndims * sizeof(int),
                            mpi_errno, "cart.periodic", MPL_MEM_COMM);
        MPIR_CHKPMEM_MALLOC(cart_ptr->topo.cart.position, int *, ndims * sizeof(int),
                            mpi_errno, "cart.position", MPL_MEM_COMM);
        nranks = newsize;
        for (i = 0; i < ndims; i++) {
            cart_ptr->topo.cart.dims[i] = dims[i];
            cart_ptr->topo.cart.periodic[i] = periods[i];
            nranks = nranks / dims[i];
            /* FIXME: nranks could be zero (?) */
            cart_ptr->topo.cart.position[i] = rank / nranks;
            rank = rank % nranks;
        }
    }


    /* Place this topology onto the communicator */
    mpi_errno = MPIR_Topology_put(newcomm_ptr, cart_ptr);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    MPIR_OBJ_PUBLISH_HANDLE(*comm_cart, newcomm_ptr->handle);

  fn_exit:
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    MPIR_CHKPMEM_REAP();
    /* --END ERROR HANDLING-- */
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Cart_create_impl
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Cart_create_impl(MPIR_Comm * comm_ptr, int ndims, const int dims[],
                          const int periods[], int reorder, MPI_Comm * comm_cart)
{
    int mpi_errno = MPI_SUCCESS;

    if (comm_ptr->topo_fns != NULL && comm_ptr->topo_fns->cartCreate != NULL) {
        /* --BEGIN USEREXTENSION-- */
        mpi_errno = comm_ptr->topo_fns->cartCreate(comm_ptr, ndims,
                                                   (const int *) dims,
                                                   (const int *) periods, reorder, comm_cart);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
        /* --END USEREXTENSION-- */
    } else {
        mpi_errno = MPIR_Cart_create(comm_ptr, ndims,
                                     (const int *) dims, (const int *) periods, reorder, comm_cart);
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
#define FUNCNAME MPI_Cart_create
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@

MPI_Cart_create - Makes a new communicator to which topology information
                  has been attached

Input Parameters:
+ comm_old - input communicator (handle)
. ndims - number of dimensions of cartesian grid (integer)
. dims - integer array of size ndims specifying the number of processes in
  each dimension
. periods - logical array of size ndims specifying whether the grid is
  periodic (true) or not (false) in each dimension
- reorder - ranking may be reordered (true) or not (false) (logical)

Output Parameters:
. comm_cart - communicator with new cartesian topology (handle)

Algorithm:
We ignore 'reorder' info currently.

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_TOPOLOGY
.N MPI_ERR_DIMS
.N MPI_ERR_ARG
@*/
int MPI_Cart_create(MPI_Comm comm_old, int ndims, const int dims[],
                    const int periods[], int reorder, MPI_Comm * comm_cart)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *comm_ptr = NULL;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_CART_CREATE);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_CART_CREATE);

    /* Validate parameters, especially handles needing to be converted */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_COMM(comm_old, mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif

    /* Convert MPI object handles to object pointers */
    MPIR_Comm_get_ptr(comm_old, comm_ptr);

    /* Validate parameters and objects (post conversion) */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            /* Validate comm_ptr */
            MPIR_Comm_valid_ptr(comm_ptr, mpi_errno, FALSE);
            if (mpi_errno != MPI_SUCCESS)
                goto fn_fail;
            /* If comm_ptr is not valid, it will be reset to null */
            if (comm_ptr) {
                MPIR_ERRTEST_COMM_INTRA(comm_ptr, mpi_errno);
            }

            if (ndims > 0) {
                MPIR_ERRTEST_ARGNULL(dims, "dims", mpi_errno);
                MPIR_ERRTEST_ARGNULL(periods, "periods", mpi_errno);
            }
            MPIR_ERRTEST_ARGNULL(comm_cart, "comm_cart", mpi_errno);
            if (ndims < 0) {
                /* Must have a non-negative number of dimensions */
                mpi_errno = MPIR_Err_create_code(MPI_SUCCESS,
                                                 MPIR_ERR_RECOVERABLE, FCNAME, __LINE__,
                                                 MPI_ERR_DIMS, "**dims", "**dims %d", 0);
                goto fn_fail;
            }
            MPIR_ERRTEST_ARGNEG(ndims, "ndims", mpi_errno);
            if (comm_ptr) {
                MPIR_ERRTEST_COMM_INTRA(comm_ptr, mpi_errno);
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    mpi_errno = MPIR_Cart_create_impl(comm_ptr, ndims,
                                      (const int *) dims,
                                      (const int *) periods, reorder, comm_cart);
    if (mpi_errno)
        goto fn_fail;
    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_CART_CREATE);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno =
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
                                 "**mpi_cart_create", "**mpi_cart_create %C %d %p %p %d %p",
                                 comm_old, ndims, dims, periods, reorder, comm_cart);
    }
#endif
    mpi_errno = MPIR_Err_return_comm(comm_ptr, FCNAME, mpi_errno);
    /* --END ERROR HANDLING-- */
    goto fn_exit;
}
