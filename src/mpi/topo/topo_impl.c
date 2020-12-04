/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

categories:
    - name        : DIMS
      description : Dims_create cvars

cvars:
    - name        : MPIR_CVAR_DIMS_VERBOSE
      category    : DIMS
      type        : boolean
      default     : false
      class       : none
      verbosity   : MPI_T_VERBOSITY_MPIDEV_DETAIL
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
           If true, enable verbose output about the actions of the
           implementation of MPI_Dims_create.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

int MPIR_Cart_coords_impl(MPIR_Comm * comm_ptr, int rank, int maxdims, int coords[])
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Topology *cart_ptr;

    cart_ptr = MPIR_Topology_get(comm_ptr);

    /* Calculate coords */
    int nnodes = cart_ptr->topo.cart.nnodes;
    for (int i = 0; i < cart_ptr->topo.cart.ndims; i++) {
        nnodes = nnodes / cart_ptr->topo.cart.dims[i];
        coords[i] = rank / nnodes;
        rank = rank % nnodes;
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int cart_create(MPIR_Comm * comm_ptr, int ndims, const int dims[],
                       const int periods[], int reorder, MPIR_Comm ** comm_cart_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    int i, newsize, rank, nranks;
    MPIR_Comm *newcomm_ptr = NULL;
    MPIR_Topology *cart_ptr = NULL;
    MPIR_CHKPMEM_DECL(4);

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
            MPIR_ERR_CHECK(mpi_errno);

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
            *comm_cart_ptr = NULL;
            goto fn_exit;
        }
    } else {

        /* Create a new communicator as a duplicate of the input communicator
         * (but do not duplicate the attributes) */
        if (reorder) {

            /* Allow the cart map routine to remap the assignment of ranks to
             * processes */
            mpi_errno = MPIR_Cart_map_impl(comm_ptr, ndims, (const int *) dims,
                                           (const int *) periods, &rank);
            MPIR_ERR_CHECK(mpi_errno);

            /* Create the new communicator with split, since we need to reorder
             * the ranks (including the related internals, such as the connection
             * tables */
            mpi_errno = MPIR_Comm_split_impl(comm_ptr,
                                             rank == MPI_UNDEFINED ? MPI_UNDEFINED : 1,
                                             rank, &newcomm_ptr);
            MPIR_ERR_CHECK(mpi_errno);

        } else {
            mpi_errno = MPII_Comm_copy((MPIR_Comm *) comm_ptr, newsize, NULL, &newcomm_ptr);
            MPIR_ERR_CHECK(mpi_errno);
            rank = comm_ptr->rank;
        }

        /* If this process is not in the resulting communicator, return a
         * null communicator and exit */
        if (rank >= newsize || rank == MPI_UNDEFINED) {
            *comm_cart_ptr = NULL;
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
    MPIR_ERR_CHECK(mpi_errno);

    *comm_cart_ptr = newcomm_ptr;

  fn_exit:
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    MPIR_CHKPMEM_REAP();
    /* --END ERROR HANDLING-- */
    goto fn_exit;
}

int MPIR_Cart_create_impl(MPIR_Comm * comm_ptr, int ndims, const int dims[],
                          const int periods[], int reorder, MPIR_Comm ** comm_cart_ptr)
{
    if (comm_ptr->topo_fns != NULL && comm_ptr->topo_fns->cartCreate != NULL) {
        /* --BEGIN USEREXTENSION-- */
        return comm_ptr->topo_fns->cartCreate(comm_ptr, ndims, dims, periods, reorder,
                                              comm_cart_ptr);
        /* --END USEREXTENSION-- */
    } else {
        return cart_create(comm_ptr, ndims, dims, periods, reorder, comm_cart_ptr);
    }
}

int MPIR_Cart_get_impl(MPIR_Comm * comm_ptr, int maxdims, int dims[], int periods[], int coords[])
{
    int mpi_errno = MPI_SUCCESS;
    int i, n, *vals;

    MPIR_Topology *cart_ptr = MPIR_Topology_get(comm_ptr);
    n = cart_ptr->topo.cart.ndims;

    vals = cart_ptr->topo.cart.dims;
    for (i = 0; i < n; i++)
        *dims++ = *vals++;

    /* Get periods */
    vals = cart_ptr->topo.cart.periodic;
    for (i = 0; i < n; i++)
        *periods++ = *vals++;

    /* Get coords */
    vals = cart_ptr->topo.cart.position;
    for (i = 0; i < n; i++)
        *coords++ = *vals++;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int cart_map(const MPIR_Comm * comm_ptr, int ndims, const int dims[],
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

int MPIR_Cart_map_impl(MPIR_Comm * comm_ptr, int ndims, const int dims[],
                       const int periods[], int *newrank)
{
    int mpi_errno = MPI_SUCCESS;

    if (comm_ptr->topo_fns != NULL && comm_ptr->topo_fns->cartMap != NULL) {
        /* --BEGIN USEREXTENSION-- */
        mpi_errno = comm_ptr->topo_fns->cartMap(comm_ptr, ndims,
                                                (const int *) dims, (const int *) periods, newrank);
        MPIR_ERR_CHECK(mpi_errno);
        /* --END USEREXTENSION-- */
    } else {
        mpi_errno = cart_map(comm_ptr, ndims, (const int *) dims, (const int *) periods, newrank);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Cart_rank_impl(MPIR_Comm * comm_ptr, const int coords[], int *rank)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Topology *cart_ptr;
    int i, ndims, coord, multiplier;

    cart_ptr = MPIR_Topology_get(comm_ptr);
    MPIR_ERR_CHKANDJUMP((!cart_ptr || cart_ptr->kind != MPI_CART),
                        mpi_errno, MPI_ERR_TOPOLOGY, "**notcarttopo");
    ndims = cart_ptr->topo.cart.ndims;
    *rank = 0;
    multiplier = 1;
    for (i = ndims - 1; i >= 0; i--) {
        coord = coords[i];
        if (cart_ptr->topo.cart.periodic[i]) {
            if (coord >= cart_ptr->topo.cart.dims[i])
                coord = coord % cart_ptr->topo.cart.dims[i];
            else if (coord < 0) {
                coord = coord % cart_ptr->topo.cart.dims[i];
                if (coord)
                    coord = cart_ptr->topo.cart.dims[i] + coord;
            }
        }
        *rank += multiplier * coord;
        multiplier *= cart_ptr->topo.cart.dims[i];
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Cart_shift_impl(MPIR_Comm * comm_ptr, int direction, int disp, int *rank_source,
                         int *rank_dest)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Topology *cart_ptr;
    int i;
    int pos[MAX_CART_DIM];

    cart_ptr = MPIR_Topology_get(comm_ptr);
    MPIR_ERR_CHKANDJUMP((!cart_ptr || cart_ptr->kind != MPI_CART),
                        mpi_errno, MPI_ERR_TOPOLOGY, "**notcarttopo");

    MPIR_ERR_CHKANDJUMP((cart_ptr->topo.cart.ndims == 0), mpi_errno, MPI_ERR_TOPOLOGY,
                        "**dimszero");
    MPIR_ERR_CHKANDJUMP2((direction >= cart_ptr->topo.cart.ndims), mpi_errno, MPI_ERR_ARG,
                         "**dimsmany", "**dimsmany %d %d", cart_ptr->topo.cart.ndims, direction);

    /* Check for the case of a 0 displacement */
    if (disp == 0) {
        *rank_source = *rank_dest = comm_ptr->rank;
    } else {
        /* To support advanced implementations that support MPI_Cart_create,
         * we compute the new position and call PMPI_Cart_rank to get the
         * source and destination.  We could bypass that step if we know that
         * the mapping is trivial.  Copy the current position. */
        for (i = 0; i < cart_ptr->topo.cart.ndims; i++) {
            pos[i] = cart_ptr->topo.cart.position[i];
        }
        /* We must return MPI_PROC_NULL if shifted over the edge of a
         * non-periodic mesh */
        pos[direction] += disp;
        if (!cart_ptr->topo.cart.periodic[direction] &&
            (pos[direction] >= cart_ptr->topo.cart.dims[direction] || pos[direction] < 0)) {
            *rank_dest = MPI_PROC_NULL;
        } else {
            MPIR_Cart_rank_impl(comm_ptr, pos, rank_dest);
        }

        pos[direction] = cart_ptr->topo.cart.position[direction] - disp;
        if (!cart_ptr->topo.cart.periodic[direction] &&
            (pos[direction] >= cart_ptr->topo.cart.dims[direction] || pos[direction] < 0)) {
            *rank_source = MPI_PROC_NULL;
        } else {
            MPIR_Cart_rank_impl(comm_ptr, pos, rank_source);
        }
    }


  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Cart_sub_impl(MPIR_Comm * comm_ptr, const int remain_dims[], MPIR_Comm ** p_newcomm_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    int all_false;
    MPIR_Comm *newcomm_ptr;
    MPIR_Topology *topo_ptr, *toponew_ptr;
    int ndims, key, color, ndims_in_subcomm, nnodes_in_subcomm, i, j, rank;
    MPIR_CHKPMEM_DECL(4);

    /* Check that the communicator already has a Cartesian topology */
    topo_ptr = MPIR_Topology_get(comm_ptr);

    MPIR_ERR_CHKANDJUMP(!topo_ptr, mpi_errno, MPI_ERR_TOPOLOGY, "**notopology");
    MPIR_ERR_CHKANDJUMP(topo_ptr->kind != MPI_CART, mpi_errno, MPI_ERR_TOPOLOGY, "**notcarttopo");

    ndims = topo_ptr->topo.cart.ndims;

    all_false = 1;      /* all entries in remain_dims are false */
    for (i = 0; i < ndims; i++) {
        if (remain_dims[i]) {
            /* any 1 is true, set flag to 0 and break */
            all_false = 0;
            break;
        }
    }

    if (all_false) {
        /* ndims=0, or all entries in remain_dims are false.
         * MPI 2.1 says return a 0D Cartesian topology. */
        mpi_errno = MPIR_Cart_create_impl(comm_ptr, 0, NULL, NULL, 0, p_newcomm_ptr);
        MPIR_ERR_CHECK(mpi_errno);
    } else {
        /* Determine the number of remaining dimensions */
        ndims_in_subcomm = 0;
        nnodes_in_subcomm = 1;
        for (i = 0; i < ndims; i++) {
            if (remain_dims[i]) {
                ndims_in_subcomm++;
                nnodes_in_subcomm *= topo_ptr->topo.cart.dims[i];
            }
        }

        /* Split this communicator.  Do this even if there are no remaining
         * dimensions so that the topology information is attached */
        key = 0;
        color = 0;
        for (i = 0; i < ndims; i++) {
            if (remain_dims[i]) {
                key = (key * topo_ptr->topo.cart.dims[i]) + topo_ptr->topo.cart.position[i];
            } else {
                color = (color * topo_ptr->topo.cart.dims[i]) + topo_ptr->topo.cart.position[i];
            }
        }
        mpi_errno = MPIR_Comm_split_impl(comm_ptr, color, key, &newcomm_ptr);
        MPIR_ERR_CHECK(mpi_errno);

        *p_newcomm_ptr = newcomm_ptr;

        /* Save the topology of this new communicator */
        MPIR_CHKPMEM_MALLOC(toponew_ptr, MPIR_Topology *, sizeof(MPIR_Topology),
                            mpi_errno, "toponew_ptr", MPL_MEM_COMM);

        toponew_ptr->kind = MPI_CART;
        toponew_ptr->topo.cart.ndims = ndims_in_subcomm;
        toponew_ptr->topo.cart.nnodes = nnodes_in_subcomm;
        if (ndims_in_subcomm) {
            MPIR_CHKPMEM_MALLOC(toponew_ptr->topo.cart.dims, int *,
                                ndims_in_subcomm * sizeof(int), mpi_errno, "cart.dims",
                                MPL_MEM_COMM);
            MPIR_CHKPMEM_MALLOC(toponew_ptr->topo.cart.periodic, int *,
                                ndims_in_subcomm * sizeof(int), mpi_errno, "cart.periodic",
                                MPL_MEM_COMM);
            MPIR_CHKPMEM_MALLOC(toponew_ptr->topo.cart.position, int *,
                                ndims_in_subcomm * sizeof(int), mpi_errno, "cart.position",
                                MPL_MEM_COMM);
        } else {
            toponew_ptr->topo.cart.dims = 0;
            toponew_ptr->topo.cart.periodic = 0;
            toponew_ptr->topo.cart.position = 0;
        }

        j = 0;
        for (i = 0; i < ndims; i++) {
            if (remain_dims[i]) {
                toponew_ptr->topo.cart.dims[j] = topo_ptr->topo.cart.dims[i];
                toponew_ptr->topo.cart.periodic[j] = topo_ptr->topo.cart.periodic[i];
                j++;
            }
        }

        /* Compute the position of this process in the new communicator */
        rank = newcomm_ptr->rank;
        for (i = 0; i < ndims_in_subcomm; i++) {
            nnodes_in_subcomm /= toponew_ptr->topo.cart.dims[i];
            toponew_ptr->topo.cart.position[i] = rank / nnodes_in_subcomm;
            rank = rank % nnodes_in_subcomm;
        }

        mpi_errno = MPIR_Topology_put(newcomm_ptr, toponew_ptr);
        if (mpi_errno)
            goto fn_fail;
    }

  fn_exit:
    MPIR_CHKPMEM_REAP();
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Cartdim_get_impl(MPIR_Comm * comm_ptr, int *ndims)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Topology *cart_ptr = MPIR_Topology_get(comm_ptr);
    MPIR_ERR_CHKANDJUMP((!cart_ptr || cart_ptr->kind != MPI_CART),
                        mpi_errno, MPI_ERR_TOPOLOGY, "**notcarttopo");
    *ndims = cart_ptr->topo.cart.ndims;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static MPIR_T_pvar_timer_t PVAR_TIMER_dims_getdivs;
static MPIR_T_pvar_timer_t PVAR_TIMER_dims_sort;
static MPIR_T_pvar_timer_t PVAR_TIMER_dims_fact;
static MPIR_T_pvar_timer_t PVAR_TIMER_dims_basefact;
static MPIR_T_pvar_timer_t PVAR_TIMER_dims_div;
static MPIR_T_pvar_timer_t PVAR_TIMER_dims_bal;

static unsigned long long PVAR_COUNTER_dims_npruned;
static unsigned long long PVAR_COUNTER_dims_ndivmade;
static unsigned long long PVAR_COUNTER_dims_optbalcalls;

/* MPI_Dims_create and PMPI_Dims_create must see the same variable for this
   one-time initialization flag.  If this file must be compiled twice,
   this variable is defined here and as external for the other build. */
static volatile int MPIR_DIMS_initPCVars = 1;

/* This routine is called once to define any PVARS and CVARS */
static int MPIR_Dims_create_init(void)
{

    MPIR_T_PVAR_TIMER_REGISTER_STATIC(DIMS,
                                      MPI_DOUBLE,
                                      dims_getdivs,
                                      MPI_T_VERBOSITY_MPIDEV_DETAIL,
                                      MPI_T_BIND_NO_OBJECT,
                                      MPIR_T_PVAR_FLAG_READONLY,
                                      "DIMS", "Time to find divisors (seconds)");
    MPIR_T_PVAR_TIMER_REGISTER_STATIC(DIMS,
                                      MPI_DOUBLE,
                                      dims_sort,
                                      MPI_T_VERBOSITY_MPIDEV_DETAIL,
                                      MPI_T_BIND_NO_OBJECT,
                                      MPIR_T_PVAR_FLAG_READONLY,
                                      "DIMS", "Time to sort divisors (seconds)");
    MPIR_T_PVAR_TIMER_REGISTER_STATIC(DIMS,
                                      MPI_DOUBLE,
                                      dims_fact,
                                      MPI_T_VERBOSITY_MPIDEV_DETAIL,
                                      MPI_T_BIND_NO_OBJECT,
                                      MPIR_T_PVAR_FLAG_READONLY,
                                      "DIMS", "Time to find factors (seconds)");
    MPIR_T_PVAR_TIMER_REGISTER_STATIC(DIMS,
                                      MPI_DOUBLE,
                                      dims_basefact,
                                      MPI_T_VERBOSITY_MPIDEV_DETAIL,
                                      MPI_T_BIND_NO_OBJECT,
                                      MPIR_T_PVAR_FLAG_READONLY, "DIMS", "Time to ? (seconds)");
    MPIR_T_PVAR_TIMER_REGISTER_STATIC(DIMS,
                                      MPI_DOUBLE,
                                      dims_div,
                                      MPI_T_VERBOSITY_MPIDEV_DETAIL,
                                      MPI_T_BIND_NO_OBJECT,
                                      MPIR_T_PVAR_FLAG_READONLY,
                                      "DIMS", "Time spent in integer division (seconds)");
    MPIR_T_PVAR_TIMER_REGISTER_STATIC(DIMS,
                                      MPI_DOUBLE,
                                      dims_bal,
                                      MPI_T_VERBOSITY_MPIDEV_DETAIL,
                                      MPI_T_BIND_NO_OBJECT,
                                      MPIR_T_PVAR_FLAG_READONLY,
                                      "DIMS", "Time finding balanced decomposition (seconds)");

    MPIR_T_PVAR_COUNTER_REGISTER_STATIC(DIMS, MPI_UNSIGNED_LONG_LONG, dims_npruned, MPI_T_VERBOSITY_MPIDEV_DETAIL, MPI_T_BIND_NO_OBJECT, (MPIR_T_PVAR_FLAG_READONLY | MPIR_T_PVAR_FLAG_CONTINUOUS), "DIMS",     /* category name */
                                        "Number of divisors pruned from the search for a balanced decomposition");
    MPIR_T_PVAR_COUNTER_REGISTER_STATIC(DIMS, MPI_UNSIGNED_LONG_LONG, dims_ndivmade, MPI_T_VERBOSITY_MPIDEV_DETAIL, MPI_T_BIND_NO_OBJECT, (MPIR_T_PVAR_FLAG_READONLY | MPIR_T_PVAR_FLAG_CONTINUOUS), "DIMS",    /* category name */
                                        "Number of integer divisions performed");
    MPIR_T_PVAR_COUNTER_REGISTER_STATIC(DIMS, MPI_UNSIGNED_LONG_LONG, dims_optbalcalls, MPI_T_VERBOSITY_MPIDEV_DETAIL, MPI_T_BIND_NO_OBJECT, (MPIR_T_PVAR_FLAG_READONLY | MPIR_T_PVAR_FLAG_CONTINUOUS), "DIMS", /* category name */
                                        "Number of call to optbalance");

    return 0;
}

/* This includes all primes up to 46337 (sufficient for 2 billion ranks).
   If storage is a concern (MPICH was originally organized so that only
   code and data needed by applications would be loaded in order to keep
   both the size of the executable and any shared library loads small),
   this include could be organized include only primes up to 1000 (for
   a maximum of a million ranks) or 3400 (for a maximum of 10 million ranks),
   which will significantly reduce the number of elements included here.
*/
#include "primes.h"

/* Because we store factors with their multiplicities, a small array can
   store all of the factors for a large number (grows *faster* than n
   factorial). */
#define MAX_FACTORS 10
/* 2^20 is a millon */
#define MAX_DIMS    20

typedef struct Factors {
    int val, cnt;
} Factors;

/* Local only routines.  These should *not* have standard prefix */
static int factor_num(int, Factors[], int *);
static int ndivisors_from_factor(int nf, const Factors * factors);
static int factor_to_divisors(int nf, Factors * factors, int ndivs, int divs[]);
static void factor_to_dims_by_rr(int nf, Factors f[], int nd, int dims[]);
static int optbalance(int n, int idx, int nd, int ndivs,
                      const int divs[], int trydims[], int *curbal_p, int optdims[]);

/*
 * Factor "n", returning the prime factors and their counts in factors, in
 * increasing order.  Return the sum of the number of primes, including their
 * powers (e.g., 2^3 * 7^5 * 19 gives 9 divisors, the maximum that can be created
 * from this factorization)
 */
static int factor_num(int nn, Factors factors[], int *nprimes)
{
    int n = nn, val;
    int i, nfactors = 0, nall = 0;
    int cnt;

    /* Find the prime number that less than that value and try dividing
     * out the primes.  */

    /* First, get factors of 2 without division */
    if ((n & 0x1) == 0) {
        cnt = 1;
        n >>= 1;
        while ((n & 0x1) == 0) {
            cnt++;
            n >>= 1;
        }
        factors[0].cnt = cnt;
        factors[0].val = 2;
        nfactors = 1;
        nall = cnt;
    }

    /* An earlier version computed an approximation to the sqrt(n) to serve
     * as a stopping criteria.  However, tests showed that checking that the
     * square of the current prime is less than the remaining value (after
     * dividing out the primes found so far) is faster.
     */
    i = 1;
    do {
        int n2;
        val = primes[i];
        /* Note that we keep removing factors from n, so this test becomes
         * easier and easier to satisfy as we remove factors from n
         */
        if (val * val > n)
            break;
        n2 = n / val;
        if (n2 * val == n) {
            cnt = 1;
            n = n2;
            n2 = n / val;
            while (n2 * val == n) {
                cnt++;
                n = n2;
                n2 = n / val;
            }
            /* Test on nfactors >= MAX_FACTORS?  */
            /* --BEGIN ERROR HANDLING-- */
            if (nfactors + 1 == MAX_FACTORS) {
                /* Time to panic.  This should not happen, since the
                 * smallest number that could exceed this would
                 * be the product of the first 10 primes that are
                 * greater than one, which is 6,469,693,230 */
                return nfactors;
            }
            /* --END ERROR HANDLING-- */
            factors[nfactors].val = val;
            factors[nfactors++].cnt = cnt;
            nall += cnt;
            if (n == 1)
                break;
        }
        i++;
    } while (1);
    if (n != 1) {
        /* There was one factor > sqrt(n).  This factor must be prime.
         * Note if nfactors was 0, n was prime */
        factors[nfactors].val = n;
        factors[nfactors++].cnt = 1;
        nall++;
    }
    *nprimes = nall;
    return nfactors;
}

static int ndivisors_from_factor(int nf, const Factors * factors)
{
    int i, ndiv;
    ndiv = 1;
    for (i = 0; i < nf; i++) {
        ndiv *= (factors[i].cnt + 1);
    }
    ndiv -= 2;

    return ndiv;
}

static int factor_to_divisors(int nf, Factors * factors, int ndiv, int divs[])
{
    int i, powers[MAX_FACTORS], curbase[MAX_FACTORS], nd, idx, val, mpi_errno;

    MPIR_T_PVAR_TIMER_START(DIMS, dims_getdivs);
    for (i = 0; i < nf; i++) {
        powers[i] = 0;
        curbase[i] = 1;
    }

    for (nd = 0; nd < ndiv; nd++) {
        /* Get the next power */
        for (idx = 0; idx < nf; idx++) {
            powers[idx]++;
            if (powers[idx] > factors[idx].cnt) {
                powers[idx] = 0;
                curbase[idx] = 1;
            } else {
                curbase[idx] *= factors[idx].val;
                break;
            }
        }
        /* Compute the divisor.  Note that we could keep the scan of values
         * from k to nf-1, then curscan[0] would always be the next value */
        val = 1;
        for (idx = 0; idx < nf; idx++)
            val *= curbase[idx];
        divs[nd] = val;
    }
    MPIR_T_PVAR_TIMER_END(DIMS, dims_getdivs);

    /* Values are not sorted - for example, 2(2),3 will give 2,4,3 as the
     * divisors */
    /* This code is used because it is significantly faster than using
     * the qsort routine.  In tests of several million dims_create
     * calls, this code took 1.02 seconds (with -O3) and 1.79 seconds
     * (without optimization) while qsort took 2.5 seconds.
     */
    if (nf > 1) {
        int gap, j, j1, j2, k, j1max, j2max;
        MPIR_CHKLMEM_DECL(1);
        int *divs2;
        int *restrict d1, *restrict d2;

        MPIR_CHKLMEM_MALLOC(divs2, int *, nd * sizeof(int), mpi_errno, "divs2", MPL_MEM_COMM);

        MPIR_T_PVAR_TIMER_START(DIMS, dims_sort);
        /* handling the first set of pairs separately saved about 20%;
         * done inplace */
        for (j = 0; j < nd - 1; j += 2) {
            if (divs[j] > divs[j + 1]) {
                int tmp = divs[j];
                divs[j] = divs[j + 1];
                divs[j + 1] = tmp;
            }
        }
        /* Using pointers d1 and d2 about 2x copying divs2 back into divs
         * at the end of each phase */
        d1 = divs;
        d2 = divs2;
        for (gap = 2; gap < nd; gap *= 2) {
            k = 0;
            for (j = 0; j + gap < nd; j += gap * 2) {
                j1 = j;
                j2 = j + gap;
                j1max = j2;
                j2max = j2 + gap;
                if (j2max > nd)
                    j2max = nd;
                while (j1 < j1max && j2 < j2max) {
                    if (d1[j1] < d1[j2]) {
                        d2[k++] = d1[j1++];
                    } else {
                        d2[k++] = d1[j2++];
                    }
                }
                /* Copy remainder */
                while (j1 < j1max)
                    d2[k++] = d1[j1++];
                while (j2 < j2max)
                    d2[k++] = d1[j2++];
            }
            /* May be some left over */
            while (j < nd)
                d2[k++] = d1[j++];
            /* swap pointers and start again */
            {
                int *dt = d1;
                d1 = d2;
                d2 = dt;
            }
        }
        MPIR_T_PVAR_TIMER_END(DIMS, dims_sort);
        /* Result must end up in divs */
        if (d1 != divs) {
            for (j1 = 0; j1 < nd; j1++)
                divs[j1] = d1[j1];
        }
        MPIR_CHKLMEM_FREEALL();
    }
    return nd;
  fn_fail:
    return mpi_errno;
}


/*
 * This is a modified round robin assignment.  The goal is to
 * get a good first guess at a good distribution.
 *
 * First, distribute factors to dims[0..nd-1].  The purpose is to get the
 * initial factors set and to ensure that the smallest dimension is > 1.
 * Second, distibute the remaining factors, starting with the largest, to
 * the elements of dims with the smallest index i such that
 *   dims[i-1] > dims[i]*val
 * or to dims[0] if no i satisfies.
 * For example, this will successfully distribute the factors of 100 in 3-d
 * as 5,5,4.  A pure round robin would give 10,5,2.
 */
static void factor_to_dims_by_rr(int nf, Factors f[], int nd, int dims[])
{
    int i, j, k, cnt, val;

    /* Initialize dims */
    for (i = 0; i < nd; i++)
        dims[i] = 1;

    k = 0;
    /* Start with the largest factors, move back to the smallest factor */
    for (i = nf - 1; i >= 0; i--) {
        val = f[i].val;
        cnt = f[i].cnt;
        for (j = 0; j < cnt; j++) {
            if (k < nd) {
                dims[k++] = val;
            } else {
                int kk;
                /* find first location where apply val is valid.
                 * Can always apply at dims[0] */
                for (kk = nd - 1; kk > 0; kk--) {
                    if (dims[kk] * val <= dims[kk - 1])
                        break;
                }
                dims[kk] *= val;
            }
        }
    }
}


/* need to set a minidx where it stops because the remaining
   values are known.  Then pass in the entire array.  This is needed
   to get the correct values for "ties" between the first and last values.
 */
static int optbalance(int n, int idx, int nd, int ndivs, const int divs[],
                      int trydims[], int *curbal_p, int optdims[])
{
    int min = trydims[nd - 1], curbal = *curbal_p, testbal;
    int k, f, q, ff, i, ii, kk, nndivs, sf, mpi_errno = MPI_SUCCESS;

    MPIR_T_PVAR_COUNTER_INC(DIMS, dims_optbalcalls, 1);
    if (MPIR_CVAR_DIMS_VERBOSE) {
        MPL_msg_printf("Noptb: idx=%d, nd=%d, ndivs=%d, balance=%d\n", idx, nd, ndivs, curbal);
        MPL_msg_printf("Noptb:optdims: ");
        for (i = 0; i < nd; i++)
            MPL_msg_printf("%d%c", optdims[i], (i + 1 < nd) ? 'x' : '\n');
        MPL_msg_printf("Noptb:trydims: ");
        for (i = idx + 1; i < nd; i++)
            MPL_msg_printf("%d%c", trydims[i], (i + 1 < nd) ? 'x' : '\n');
    }
    if (idx > 1) {
        MPIR_CHKLMEM_DECL(1);
        int *newdivs;
        MPIR_CHKLMEM_MALLOC(newdivs, int *, ndivs * sizeof(int), mpi_errno, "divs", MPL_MEM_COMM);

        /* At least 3 divisors to set (0...idx).  We try all choices
         * recursively, but stop looking when we can easily tell that
         * no additional cases can improve the current solution. */
        for (k = 0; k < ndivs; k++) {
            f = divs[k];
            if (MPIR_CVAR_DIMS_VERBOSE) {
                MPL_msg_printf("Noptb: try f=%d at dims[%d]\n", f, idx);
            }
            if (idx < nd - 1 && f - min > curbal) {
                if (MPIR_CVAR_DIMS_VERBOSE) {
                    MPL_msg_printf("f-min = %d, curbal = %d, skipping other divisors\n",
                                   f - min, curbal);
                }
                MPIR_T_PVAR_COUNTER_INC(DIMS, dims_npruned, 1);
                break;  /* best case is all divisors at least
                         * f, so the best balance is at least
                         * f - min */
            }
            q = n / f;
            /* check whether f is a divisor of what's left.  If so,
             * remember it as the smallest new divisor */
            /* sf is the smallest remaining factor; we use this in an
             * optimization for possible divisors */
            nndivs = 0;
            if (q % f == 0) {
                newdivs[nndivs++] = f;
                sf = f;
            } else if (k + 1 < ndivs) {
                sf = divs[k + 1];
            } else {
                /* run out of next factors, bail out */
                break;
            }
            if (idx < nd - 1 && sf - min > curbal) {
                MPIR_T_PVAR_COUNTER_INC(DIMS, dims_npruned, 1);
                break;
            }
            if (MPIR_CVAR_DIMS_VERBOSE) {
                MPL_msg_printf("Noptb: sf = %d\n", sf);
            }
            ff = sf * sf;       /* At least 2 more divisors */
            for (ii = idx - 1; ii > 0 && ff <= q; ii--)
                ff *= sf;
            if (ii > 0) {
                if (MPIR_CVAR_DIMS_VERBOSE) {
                    MPL_msg_printf("break for ii = %d, ff = %d and q = %d\n", ii, ff, q);
                }
                MPIR_T_PVAR_COUNTER_INC(DIMS, dims_npruned, 1);
                break;  /* remaining divisors >= sf, and
                         * product must be <= q */
            }

            /* Try f as the divisor at the idx location */
            trydims[idx] = f;
            MPIR_T_PVAR_TIMER_START(DIMS, dims_div);
            /* find the subset of divisors of n that are divisors of q
             * and larger than f but such that f*f <= q */
            for (kk = k + 1; kk < ndivs; kk++) {
                f = divs[kk];
                ff = f * f;
                if (ff > q) {
                    MPIR_T_PVAR_COUNTER_INC(DIMS, dims_npruned, 1);
                    break;
                }
                MPIR_T_PVAR_COUNTER_INC(DIMS, dims_ndivmade, 1);
                if ((q % f) == 0) {
                    newdivs[nndivs++] = f;
                }
                /* sf * f <= q, break otherwise */
            }
            MPIR_T_PVAR_TIMER_END(DIMS, dims_div);
            /* recursively try to find the best subset */
            if (nndivs > 0) {
                mpi_errno = optbalance(q, idx - 1, nd, nndivs, newdivs, trydims, curbal_p, optdims);
                MPIR_ERR_CHECK(mpi_errno);
            }
        }
        MPIR_CHKLMEM_FREEALL();
    } else if (idx == 1) {
        /* Only two.  Find the pair such that q,f has min value for q-min, i.e.,
         * the smallest q such that q > f.  Note that f*f < n if q > f and
         * q*f = n */
        /* Valid choices for f, q >= divs[0] */
        int qprev = -1;
        for (k = 1; k < ndivs; k++) {
            f = divs[k];
            q = n / f;
            if (q < f)
                break;
            qprev = q;
        }
        f = divs[k - 1];
        if (qprev > 0)
            q = qprev;
        else
            q = n / f;

        if (q < f) {
            if (MPIR_CVAR_DIMS_VERBOSE) {
                MPL_msg_printf("Skipping because %d < %d\n", q, f);
            }
            /* No valid solution.  Exit without changing current optdims */
            MPIR_T_PVAR_COUNTER_INC(DIMS, dims_npruned, 1);
            goto fn_exit;
        }
        if (MPIR_CVAR_DIMS_VERBOSE) {
            MPL_msg_printf("Found best factors %d,%d, from divs[%d]\n", q, f, k - 1);
        }
        /* If nd is 2 (and not greater), we will be replacing all values
         * in dims.  In that case, testbal is q-f;
         */
        if (nd == 2)
            testbal = q - f;
        else
            testbal = q - min;

        /* Take the new value if it is at least as good as the
         * current choice.  This is what Jesper Traeff's version does
         * (see the code he provided for his EuroMPI'15 paper) */
        if (testbal <= curbal) {
            for (i = 2; i < nd; i++)
                optdims[i] = trydims[i];
            optdims[0] = q;
            optdims[1] = f;
            /* Use the balance for the range of dims, not the total
             * balance */
            *curbal_p = q - min;
        }
    } else {
        /* idx == 0.  Only one choice for divisor */
        if (n - min <= curbal) {
            for (i = 1; i < nd; i++)
                optdims[i] = trydims[i];
            optdims[0] = n;
            *curbal_p = n - min;
        }
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


/* FIXME: The error checking should really be part of MPI_Dims_create,
   not part of MPIR_Dims_create_impl.  That slightly changes the
   semantics of Dims_create provided by the device, but only by
   removing the need to check for errors */


int MPIR_Dims_create_impl(int nnodes, int ndims, int dims[])
{
    Factors f[MAX_FACTORS];
    int nf, nprimes = 0, i, j, k, val, nextidx;
    int ndivs, curbal;
    int trydims[MAX_DIMS];
    int dims_needed, dims_product, mpi_errno;
    int chosen[MAX_DIMS], foundDecomp;
    int *divs;
    MPIR_CHKLMEM_DECL(1);

    /* Initialize pvars and cvars if this is the first call */
    if (MPIR_DIMS_initPCVars) {
        MPIR_Dims_create_init();
        MPIR_DIMS_initPCVars = 0;
    }

    if (MPIR_Process.dimsCreate != NULL) {
        mpi_errno = MPIR_Process.dimsCreate(nnodes, ndims, dims);
        goto fn_exit;
    }

    /* Find the number of unspecified dimensions in dims and the product
     * of the positive values in dims */
    dims_needed = 0;
    dims_product = 1;
    for (i = 0; i < ndims; i++) {
        if (dims[i] < 0) {
            mpi_errno = MPIR_Err_create_code(MPI_SUCCESS,
                                             MPIR_ERR_RECOVERABLE,
                                             "MPIR_Dims_create", __LINE__,
                                             MPI_ERR_DIMS,
                                             "**argarrayneg",
                                             "**argarrayneg %s %d %d", "dims", i, dims[i]);
            return mpi_errno;
        }
        if (dims[i] == 0)
            dims_needed++;
        else
            dims_product *= dims[i];
    }

    /* Can we factor nnodes by dims_product? */
    if ((nnodes / dims_product) * dims_product != nnodes) {
        mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                                         "MPIR_Dims_create", __LINE__,
                                         MPI_ERR_DIMS, "**dimspartition", 0);
        return mpi_errno;
    }

    if (!dims_needed) {
        /* Special case - all dimensions provided */
        return MPI_SUCCESS;
    }

    if (dims_needed > MAX_DIMS) {
/* --BEGIN ERROR HANDLING-- */
        mpi_errno = MPIR_Err_create_code(MPI_SUCCESS,
                                         MPIR_ERR_RECOVERABLE,
                                         "MPIR_Dims_create", __LINE__, MPI_ERR_DIMS,
                                         "**dimsmany", "**dimsmany %d %d", dims_needed, MAX_DIMS);
        return mpi_errno;
/* --END ERROR HANDLING-- */
    }

    nnodes /= dims_product;

    /* Factor the remaining nodes into dims_needed components.  The
     * MPI standard says that these should be as balanced as possible;
     * in particular, unfortunately, not necessarily arranged in a way
     * that makes sense for the underlying machine topology.  The approach
     * used here was inspired by "Specification Guideline Violations
     * by MPI_Dims_create," by Jesper Traeff and Felix Luebbe, EuroMPI'15.
     * Among the changes are the use of a table of primes to speed up the
     * process of finding factors, and some effort to reduce the number
     * of integer division operations.
     *
     * Values are put into chosen[0..dims_needed].  These values are then
     * copied back into dims into the "empty" slots (values == 0).
     */

    if (dims_needed == 1) {
        /* Simply put the correct value in place */
        for (j = 0; j < ndims; j++) {
            if (dims[j] == 0) {
                dims[j] = nnodes;
                break;
            }
        }
        return 0;
    }

    /* Factor nnodes */
    MPIR_T_PVAR_TIMER_START(DIMS, dims_fact);
    nf = factor_num(nnodes, f, &nprimes);
    MPIR_T_PVAR_TIMER_END(DIMS, dims_fact);

    if (MPIR_CVAR_DIMS_VERBOSE) {
        MPL_msg_printf("found %d factors\n", nf);
    }

    /* Remove primes > sqrt(n) */
    nextidx = 0;
    for (j = nf - 1; j > 0 && nextidx < dims_needed - 1; j--) {
        val = f[j].val;
        if (f[j].cnt == 1 && val * val > nnodes) {
            if (MPIR_CVAR_DIMS_VERBOSE) {
                MPL_msg_printf("prime %d required in idx %d\n", val, nextidx);
            }
            chosen[nextidx++] = val;
            nf--;
            nnodes /= val;
            nprimes--;
        } else
            break;
    }

    /* Now, handle some special cases.  If we find *any* of these,
     * we're done and can use the values in chosen to return values in dims */
    foundDecomp = 0;
    if (nprimes + nextidx <= dims_needed) {
        /* Fill with the primes */
        int m, cnt;
        if (MPIR_CVAR_DIMS_VERBOSE) {
            MPL_msg_printf("Nprimes = %d, number dims left = %d\n", nprimes, dims_needed - nextidx);
        }
        i = nextidx + nprimes;
        for (k = 0; k < nf; k++) {
            cnt = f[k].cnt;
            val = f[k].val;
            for (m = 0; m < cnt; m++)
                chosen[--i] = val;
        }
        i = nextidx + nprimes;
        while (i < ndims)
            chosen[i++] = 1;
        foundDecomp = 1;
    } else if (dims_needed - nextidx == 1) {
        chosen[nextidx] = nnodes;
        foundDecomp = 1;
    } else if (nf == 1) {
        /* What's left is a product of a single prime, so there is only one
         * possible factorization */
        int cnt = f[0].cnt;
        val = f[0].val;
        if (MPIR_CVAR_DIMS_VERBOSE) {
            MPL_msg_printf("only 1 prime = %d left\n", val);
        }
        for (k = nextidx; k < dims_needed; k++)
            chosen[k] = 1;
        k = nextidx;
        while (cnt-- > 0) {
            if (k >= dims_needed)
                k = nextidx;
            chosen[k++] *= val;
        }
        foundDecomp = 1;
    }
    /* There's another case we consider, particularly since we've
     * factored out the "large" primes.  If (cnt % (ndims-nextidx)) == 0
     * for every remaining factor, then
     * f = product of (val^ (cnt/(ndims-nextidx)))
     */
    if (!foundDecomp) {
        int ndleft = dims_needed - nextidx;
        for (j = 0; j < nf; j++)
            if (f[j].cnt % ndleft != 0)
                break;
        if (j == nf) {
            int fval;
            val = 1;
            for (j = 0; j < nf; j++) {
                int pow = f[j].cnt / ndleft;
                fval = f[j].val;
                while (pow--)
                    val *= fval;
            }
            for (j = nextidx; j < dims_needed; j++)
                chosen[j] = val;
            if (MPIR_CVAR_DIMS_VERBOSE) {
                MPL_msg_printf("Used power of factors for dims\n");
            }
            foundDecomp = 1;
        }
    }

    if (foundDecomp) {
        j = 0;
        for (i = 0; i < ndims; i++) {
            if (dims[i] == 0) {
                dims[i] = chosen[j++];
            }
        }
        return 0;
    }

    /* ndims >= 2 */

    /* No trivial solution.  Balance the remaining values (note that we may
     * have already trimmed off some large factors */
    /* First, find all of the divisors given by the remaining factors */
    ndivs = ndivisors_from_factor(nf, (const Factors *) f);
    MPIR_CHKLMEM_MALLOC(divs, int *, ((unsigned int) ndivs) * sizeof(int), mpi_errno, "divs",
                        MPL_MEM_COMM);
    ndivs = factor_to_divisors(nf, f, ndivs, divs);
    if (MPIR_CVAR_DIMS_VERBOSE) {
        for (i = 0; i < ndivs; i++) {
            if (divs[i] <= 0) {
                MPL_msg_printf("divs[%d]=%d!\n", i, divs[i]);
            }
        }
    }

    /* Create a simple first guess.  We do this by using a round robin
     * distribution of the primes in the remaining values (from nextidx
     * to ndims) */
    MPIR_T_PVAR_TIMER_START(DIMS, dims_basefact);
    factor_to_dims_by_rr(nf, f, dims_needed - nextidx, chosen + nextidx);
    MPIR_T_PVAR_TIMER_END(DIMS, dims_basefact);

    /* This isn't quite right, as the balance may depend on the other (larger)
     * divisors already chosen */
    /* Need to add nextidx */
    curbal = chosen[0] - chosen[dims_needed - 1];
    trydims[dims_needed - nextidx - 1] = divs[0];
    if (MPIR_CVAR_DIMS_VERBOSE) {
        MPL_msg_printf("N: initial decomp is: ");
        for (i = 0; i < dims_needed; i++)
            MPL_msg_printf("%d%c", chosen[i], (i + 1 < dims_needed) ? 'x' : '\n');
    }
    MPIR_T_PVAR_TIMER_START(DIMS, dims_bal);
    mpi_errno = optbalance(nnodes, dims_needed - nextidx - 1, dims_needed - nextidx,
                           ndivs, divs, trydims, &curbal, chosen + nextidx);
    MPIR_T_PVAR_TIMER_END(DIMS, dims_bal);
    MPIR_CHKLMEM_FREEALL();
    MPIR_ERR_CHECK(mpi_errno);

    if (MPIR_CVAR_DIMS_VERBOSE) {
        MPL_msg_printf("N: final decomp is: ");
        for (i = 0; i < dims_needed; i++)
            MPL_msg_printf("%d%c", chosen[i], (i + 1 < dims_needed) ? 'x' : '\n');
    }

    j = 0;
    for (i = 0; i < ndims; i++) {
        if (dims[i] == 0) {
            dims[i] = chosen[j++];
        }
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Dist_graph_create_impl(MPIR_Comm * comm_ptr,
                                int n, const int sources[], const int degrees[],
                                const int destinations[], const int weights[],
                                MPIR_Info * info_ptr, int reorder,
                                MPIR_Comm ** p_comm_dist_graph_ptr);
{
    int mpi_errno = MPI_SUCCESS;
    /* Implementation based on Torsten Hoefler's reference implementation
     * attached to MPI-2.2 ticket #33. */
    MPIR_Comm *comm_dist_graph_ptr = NULL;
    int comm_size = comm_ptr->local_size;
    MPIR_CHKLMEM_DECL(7);
    MPIR_CHKPMEM_DECL(1);

    /* following the spirit of the old topo interface, attributes do not
     * propagate to the new communicator (see MPI-2.1 pp. 243 line 11) */
    mpi_errno = MPII_Comm_copy(comm_ptr, comm_size, NULL, &comm_dist_graph_ptr);
    MPIR_ERR_CHECK(mpi_errno);
    MPIR_Assert(comm_dist_graph_ptr != NULL);

    /* rin is an array of size comm_size containing pointers to arrays of
     * rin_sizes[x].  rin[x] is locally known number of edges into this process
     * from rank x.
     *
     * rout is an array of comm_size containing pointers to arrays of
     * rout_sizes[x].  rout[x] is the locally known number of edges out of this
     * process to rank x. */
    int **rin, *rin_sizes, *rin_idx;
    int **rout, *rout_sizes, *rout_idx;
    MPIR_CHKLMEM_MALLOC(rout, int **, comm_size * sizeof(int *), mpi_errno, "rout", MPL_MEM_COMM);
    MPIR_CHKLMEM_MALLOC(rin, int **, comm_size * sizeof(int *), mpi_errno, "rin", MPL_MEM_COMM);
    MPIR_CHKLMEM_MALLOC(rin_sizes, int *, comm_size * sizeof(int), mpi_errno, "rin_sizes",
                        MPL_MEM_COMM);
    MPIR_CHKLMEM_MALLOC(rout_sizes, int *, comm_size * sizeof(int), mpi_errno, "rout_sizes",
                        MPL_MEM_COMM);
    MPIR_CHKLMEM_MALLOC(rin_idx, int *, comm_size * sizeof(int), mpi_errno, "rin_idx",
                        MPL_MEM_COMM);
    MPIR_CHKLMEM_MALLOC(rout_idx, int *, comm_size * sizeof(int), mpi_errno, "rout_idx",
                        MPL_MEM_COMM);

    memset(rout, 0, comm_size * sizeof(int *));
    memset(rin, 0, comm_size * sizeof(int *));
    memset(rin_sizes, 0, comm_size * sizeof(int));
    memset(rout_sizes, 0, comm_size * sizeof(int));
    memset(rin_idx, 0, comm_size * sizeof(int));
    memset(rout_idx, 0, comm_size * sizeof(int));

    /* compute array sizes */
    int idx = 0;
    for (int i = 0; i < n; ++i) {
        MPIR_Assert(sources[i] < comm_size);
        for (int j = 0; j < degrees[i]; ++j) {
            MPIR_Assert(destinations[idx] < comm_size);
            /* rout_sizes[i] is twice as long as the number of edges to be
             * sent to rank i by this process */
            rout_sizes[sources[i]] += 2;
            rin_sizes[destinations[idx]] += 2;
            ++idx;
        }
    }

    /* allocate arrays */
    for (int i = 0; i < comm_size; ++i) {
        /* can't use CHKLMEM macros b/c we are in a loop */
        if (rin_sizes[i]) {
            rin[i] = MPL_malloc(rin_sizes[i] * sizeof(int), MPL_MEM_COMM);
        }
        if (rout_sizes[i]) {
            rout[i] = MPL_malloc(rout_sizes[i] * sizeof(int), MPL_MEM_COMM);
        }
    }

    /* populate arrays */
    idx = 0;
    for (int i = 0; i < n; ++i) {
        /* TODO add this assert as proper error checking above */
        int s_rank = sources[i];
        MPIR_Assert(s_rank < comm_size);
        MPIR_Assert(s_rank >= 0);

        for (j = 0; j < degrees[i]; ++j) {
            int d_rank = destinations[idx];
            int weight = (weights == MPI_UNWEIGHTED ? 0 : weights[idx]);
            /* TODO add this assert as proper error checking above */
            MPIR_Assert(d_rank < comm_size);
            MPIR_Assert(d_rank >= 0);

            /* XXX DJG what about self-edges? do we need to drop one of these
             * cases when there is a self-edge to avoid double-counting? */

            /* rout[s][2*x] is the value of d for the j'th edge between (s,d)
             * with weight rout[s][2*x+1], where x is the current end of the
             * outgoing edge list for s.  x==(rout_idx[s]/2) */
            rout[s_rank][rout_idx[s_rank]++] = d_rank;
            rout[s_rank][rout_idx[s_rank]++] = weight;

            /* rin[d][2*x] is the value of s for the j'th edge between (s,d)
             * with weight rout[d][2*x+1], where x is the current end of the
             * incoming edge list for d.  x==(rin_idx[d]/2) */
            rin[d_rank][rin_idx[d_rank]++] = s_rank;
            rin[d_rank][rin_idx[d_rank]++] = weight;

            ++idx;
        }
    }

    for (int i = 0; i < comm_size; ++i) {
        /* sanity check that all arrays are fully populated */
        MPIR_Assert(rin_idx[i] == rin_sizes[i]);
        MPIR_Assert(rout_idx[i] == rout_sizes[i]);
    }

    int *rs;
    MPIR_CHKLMEM_MALLOC(rs, int *, 2 * comm_size * sizeof(int), mpi_errno, "red-scat source buffer",
                        MPL_MEM_COMM);
    for (i = 0; i < comm_size; ++i) {
        rs[2 * i] = (rin_sizes[i] ? 1 : 0);
        rs[2 * i + 1] = (rout_sizes[i] ? 1 : 0);
    }

    /* compute the number of peers I will recv from */
    int in_out_peers[2] = { -1, -1 };
    mpi_errno =
        MPIR_Reduce_scatter_block(rs, in_out_peers, 2, MPI_INT, MPI_SUM, comm_ptr, &errflag);
    MPIR_ERR_CHECK(mpi_errno);
    MPIR_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");

    MPIR_Assert(in_out_peers[0] <= comm_size && in_out_peers[0] >= 0);
    MPIR_Assert(in_out_peers[1] <= comm_size && in_out_peers[1] >= 0);

    idx = 0;
    /* must be 2*comm_size requests because we will possibly send inbound and
     * outbound edges to everyone in our communicator */
    MPIR_Request **reqs;
    MPIR_CHKLMEM_MALLOC(reqs, MPIR_Request **, 2 * comm_size * sizeof(MPIR_Request *), mpi_errno,
                        "temp request array", MPL_MEM_COMM);
    for (int i = 0; i < comm_size; ++i) {
        if (rin_sizes[i]) {
            /* send edges where i is a destination to process i */
            mpi_errno =
                MPIC_Isend(&rin[i][0], rin_sizes[i], MPI_INT, i, MPIR_TOPO_A_TAG, comm_ptr,
                           &reqs[idx++], &errflag);
            MPIR_ERR_CHECK(mpi_errno);
        }
        if (rout_sizes[i]) {
            /* send edges where i is a source to process i */
            mpi_errno =
                MPIC_Isend(&rout[i][0], rout_sizes[i], MPI_INT, i, MPIR_TOPO_B_TAG, comm_ptr,
                           &reqs[idx++], &errflag);
            MPIR_ERR_CHECK(mpi_errno);
        }
    }
    MPIR_Assert(idx <= (2 * comm_size));

    /* Create the topology structure */
    MPIR_CHKPMEM_MALLOC(topo_ptr, MPIR_Topology *, sizeof(MPIR_Topology), mpi_errno, "topo_ptr",
                        MPL_MEM_COMM);
    topo_ptr->kind = MPI_DIST_GRAPH;
    dist_graph_ptr = &topo_ptr->topo.dist_graph;
    dist_graph_ptr->indegree = 0;
    dist_graph_ptr->in = NULL;
    dist_graph_ptr->in_weights = NULL;
    dist_graph_ptr->outdegree = 0;
    dist_graph_ptr->out = NULL;
    dist_graph_ptr->out_weights = NULL;
    dist_graph_ptr->is_weighted = (weights != MPI_UNWEIGHTED);

    /* can't use CHKPMEM macros for this b/c we need to realloc */
    int in_capacity = 10;       /* arbitrary */
    dist_graph_ptr->in = MPL_malloc(in_capacity * sizeof(int), MPL_MEM_COMM);
    if (dist_graph_ptr->is_weighted) {
        dist_graph_ptr->in_weights = MPL_malloc(in_capacity * sizeof(int), MPL_MEM_COMM);
        MPIR_Assert(dist_graph_ptr->in_weights != NULL);
    }
    int out_capacity = 10;      /* arbitrary */
    dist_graph_ptr->out = MPL_malloc(out_capacity * sizeof(int), MPL_MEM_COMM);
    if (dist_graph_ptr->is_weighted) {
        dist_graph_ptr->out_weights = MPL_malloc(out_capacity * sizeof(int), MPL_MEM_COMM);
        MPIR_Assert(dist_graph_ptr->out_weights);
    }

    for (int i = 0; i < in_out_peers[0]; ++i) {
        MPI_Status status;
        MPI_Aint count;
        int *buf;
        /* receive inbound edges */
        mpi_errno = MPIC_Probe(MPI_ANY_SOURCE, MPIR_TOPO_A_TAG, comm_old, &status);
        MPIR_ERR_CHECK(mpi_errno);
        MPIR_Get_count_impl(&status, MPI_INT, &count);
        /* can't use CHKLMEM macros b/c we are in a loop */
        /* FIXME: Why not - there is only one allocated at a time. Is it only
         * that there is no defined macro to pop and free an item? */
        buf = MPL_malloc(count * sizeof(int), MPL_MEM_COMM);
        MPIR_ERR_CHKANDJUMP(!buf, mpi_errno, MPI_ERR_OTHER, "**nomem");

        mpi_errno =
            MPIC_Recv(buf, count, MPI_INT, MPI_ANY_SOURCE, MPIR_TOPO_A_TAG, comm_ptr,
                      MPI_STATUS_IGNORE, &errflag);
        /* FIXME: buf is never freed on error! */
        MPIR_ERR_CHECK(mpi_errno);

        for (int j = 0; j < count / 2; ++j) {
            int deg = dist_graph_ptr->indegree++;
            if (deg >= in_capacity) {
                in_capacity *= 2;
                /* FIXME: buf is never freed on error! */
                MPIR_REALLOC_ORJUMP(dist_graph_ptr->in, in_capacity * sizeof(int), MPL_MEM_COMM,
                                    mpi_errno);
                if (dist_graph_ptr->is_weighted)
                    /* FIXME: buf is never freed on error! */
                    MPIR_REALLOC_ORJUMP(dist_graph_ptr->in_weights, in_capacity * sizeof(int),
                                        MPL_MEM_COMM, mpi_errno);
            }
            dist_graph_ptr->in[deg] = buf[2 * j];
            if (dist_graph_ptr->is_weighted)
                dist_graph_ptr->in_weights[deg] = buf[2 * j + 1];
        }
        MPL_free(buf);
    }

    for (int i = 0; i < in_out_peers[1]; ++i) {
        MPI_Status status;
        MPI_Aint count;
        int *buf;
        /* receive outbound edges */
        mpi_errno = MPIC_Probe(MPI_ANY_SOURCE, MPIR_TOPO_B_TAG, comm_old, &status);
        MPIR_ERR_CHECK(mpi_errno);
        MPIR_Get_count_impl(&status, MPI_INT, &count);
        /* can't use CHKLMEM macros b/c we are in a loop */
        /* Why not? */
        buf = MPL_malloc(count * sizeof(int), MPL_MEM_COMM);
        MPIR_ERR_CHKANDJUMP(!buf, mpi_errno, MPI_ERR_OTHER, "**nomem");

        mpi_errno =
            MPIC_Recv(buf, count, MPI_INT, MPI_ANY_SOURCE, MPIR_TOPO_B_TAG, comm_ptr,
                      MPI_STATUS_IGNORE, &errflag);
        /* FIXME: buf is never freed on error! */
        MPIR_ERR_CHECK(mpi_errno);

        for (int j = 0; j < count / 2; ++j) {
            int deg = dist_graph_ptr->outdegree++;
            if (deg >= out_capacity) {
                out_capacity *= 2;
                /* FIXME: buf is never freed on error! */
                MPIR_REALLOC_ORJUMP(dist_graph_ptr->out, out_capacity * sizeof(int), MPL_MEM_COMM,
                                    mpi_errno);
                if (dist_graph_ptr->is_weighted)
                    /* FIXME: buf is never freed on error! */
                    MPIR_REALLOC_ORJUMP(dist_graph_ptr->out_weights, out_capacity * sizeof(int),
                                        MPL_MEM_COMM, mpi_errno);
            }
            dist_graph_ptr->out[deg] = buf[2 * j];
            if (dist_graph_ptr->is_weighted)
                dist_graph_ptr->out_weights[deg] = buf[2 * j + 1];
        }
        MPL_free(buf);
    }

    mpi_errno = MPIC_Waitall(idx, reqs, MPI_STATUSES_IGNORE, &errflag);
    MPIR_ERR_CHECK(mpi_errno);

    /* remove any excess memory allocation */
    MPIR_REALLOC_ORJUMP(dist_graph_ptr->in, dist_graph_ptr->indegree * sizeof(int), MPL_MEM_COMM,
                        mpi_errno);
    MPIR_REALLOC_ORJUMP(dist_graph_ptr->out, dist_graph_ptr->outdegree * sizeof(int), MPL_MEM_COMM,
                        mpi_errno);
    if (dist_graph_ptr->is_weighted) {
        MPIR_REALLOC_ORJUMP(dist_graph_ptr->in_weights, dist_graph_ptr->indegree * sizeof(int),
                            MPL_MEM_COMM, mpi_errno);
        MPIR_REALLOC_ORJUMP(dist_graph_ptr->out_weights, dist_graph_ptr->outdegree * sizeof(int),
                            MPL_MEM_COMM, mpi_errno);
    }

    mpi_errno = MPIR_Topology_put(comm_dist_graph_ptr, topo_ptr);
    MPIR_ERR_CHECK(mpi_errno);

    *p_comm_dist_graph_ptr = comm_dist_graph_ptr;

  fn_exit:
    for (i = 0; i < comm_size; ++i) {
        MPL_free(rin[i]);
        MPL_free(rout[i]);
    }
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno;

  fn_fail:
    if (dist_graph_ptr) {
        MPL_free(dist_graph_ptr->in);
        MPL_free(dist_graph_ptr->in_weights);
        MPL_free(dist_graph_ptr->out);
        MPL_free(dist_graph_ptr->out_weights);
    }
    MPIR_CHKPMEM_REAP();
    goto fn_exit;
}

int MPIR_Dist_graph_create_adjacent_impl(MPIR_Comm comm_ptr,
                                         int indegree, const int sources[],
                                         const int sourceweights[], int outdegree,
                                         const int destinations[], const int destweights[],
                                         MPIR_Info * info_ptr, int reorder,
                                         MPIR_Comm ** p_comm_dist_graph)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_CHKPMEM_DECL(5);

    /* Implementation based on Torsten Hoefler's reference implementation
     * attached to MPI-2.2 ticket #33. */

    /* following the spirit of the old topo interface, attributes do not
     * propagate to the new communicator (see MPI-2.1 pp. 243 line 11) */
    MPIR_Comm *comm_dist_graph_ptr;
    mpi_errno = MPII_Comm_copy(comm_ptr, comm_ptr->local_size, NULL, &comm_dist_graph_ptr);
    MPIR_ERR_CHECK(mpi_errno);

    /* Create the topology structure */
    MPIR_Topology *topo_ptr;
    MPIR_CHKPMEM_MALLOC(topo_ptr, MPIR_Topology *, sizeof(MPIR_Topology), mpi_errno, "topo_ptr",
                        MPL_MEM_COMM);
    topo_ptr->kind = MPI_DIST_GRAPH;
    dist_graph_ptr = &topo_ptr->topo.dist_graph;
    dist_graph_ptr->indegree = indegree;
    dist_graph_ptr->in = NULL;
    dist_graph_ptr->in_weights = NULL;
    dist_graph_ptr->outdegree = outdegree;
    dist_graph_ptr->out = NULL;
    dist_graph_ptr->out_weights = NULL;
    dist_graph_ptr->is_weighted = (sourceweights != MPI_UNWEIGHTED);

    if (indegree > 0) {
        MPIR_CHKPMEM_MALLOC(dist_graph_ptr->in, int *, indegree * sizeof(int), mpi_errno,
                            "dist_graph_ptr->in", MPL_MEM_COMM);
        MPIR_Memcpy(dist_graph_ptr->in, sources, indegree * sizeof(int));
        if (dist_graph_ptr->is_weighted) {
            MPIR_CHKPMEM_MALLOC(dist_graph_ptr->in_weights, int *, indegree * sizeof(int),
                                mpi_errno, "dist_graph_ptr->in_weights", MPL_MEM_COMM);
            MPIR_Memcpy(dist_graph_ptr->in_weights, sourceweights, indegree * sizeof(int));
        }
    }

    if (outdegree > 0) {
        MPIR_CHKPMEM_MALLOC(dist_graph_ptr->out, int *, outdegree * sizeof(int), mpi_errno,
                            "dist_graph_ptr->out", MPL_MEM_COMM);
        MPIR_Memcpy(dist_graph_ptr->out, destinations, outdegree * sizeof(int));
        if (dist_graph_ptr->is_weighted) {
            MPIR_CHKPMEM_MALLOC(dist_graph_ptr->out_weights, int *, outdegree * sizeof(int),
                                mpi_errno, "dist_graph_ptr->out_weights", MPL_MEM_COMM);
            MPIR_Memcpy(dist_graph_ptr->out_weights, destweights, outdegree * sizeof(int));
        }
    }

    mpi_errno = MPIR_Topology_put(comm_dist_graph_ptr, topo_ptr);
    MPIR_ERR_CHECK(mpi_errno);

    *p_comm_dist_graph = comm_dist_graph_ptr;

  fn_exit:
    return mpi_errno;
  fn_fail:
    MPIR_CHKPMEM_REAP();
    goto fn_exit;
}

int MPIR_Dist_graph_neighbors_impl(MPIR_Comm * comm_ptr,
                                   int maxindegree, int sources[], int sourceweights[],
                                   int maxoutdegree, int destinations[], int destweights[])
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Topology *topo_ptr = NULL;

    topo_ptr = MPIR_Topology_get(comm_ptr);
    MPIR_ERR_CHKANDJUMP(!topo_ptr ||
                        topo_ptr->kind != MPI_DIST_GRAPH, mpi_errno, MPI_ERR_TOPOLOGY,
                        "**notdistgraphtopo");

    if (maxindegree > 0) {
        MPIR_Memcpy(sources, topo_ptr->topo.dist_graph.in, maxindegree * sizeof(int));
        if (sourceweights != MPI_UNWEIGHTED && topo_ptr->topo.dist_graph.is_weighted) {
            MPIR_Memcpy(sourceweights, topo_ptr->topo.dist_graph.in_weights,
                        maxindegree * sizeof(int));
        }
    }

    if (maxoutdegree > 0) {
        MPIR_Memcpy(destinations, topo_ptr->topo.dist_graph.out, maxoutdegree * sizeof(int));

        if (destweights != MPI_UNWEIGHTED && topo_ptr->topo.dist_graph.is_weighted) {
            MPIR_Memcpy(destweights, topo_ptr->topo.dist_graph.out_weights,
                        maxoutdegree * sizeof(int));
        }
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* any utility functions should go here, usually prefixed with PMPI_LOCAL to
 * correctly handle weak symbols and the profiling interface */

int MPIR_Dist_graph_neighbors_count_impl(MPIR_Comm * comm_ptr, int *indegree, int *outdegree,
                                         int *weighted)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Topology *topo_ptr = NULL;

    topo_ptr = MPIR_Topology_get(comm_ptr);
    MPIR_ERR_CHKANDJUMP(!topo_ptr ||
                        topo_ptr->kind != MPI_DIST_GRAPH, mpi_errno, MPI_ERR_TOPOLOGY,
                        "**notdistgraphtopo");
    *indegree = topo_ptr->topo.dist_graph.indegree;
    *outdegree = topo_ptr->topo.dist_graph.outdegree;
    *weighted = topo_ptr->topo.dist_graph.is_weighted;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int graph_create(MPIR_Comm * comm_ptr, int nnodes,
                        const int indx[], const int edges[], int reorder,
                        MPIR_Comm ** comm_graph_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    int i, nedges;
    MPIR_Comm *newcomm_ptr = NULL;
    MPIR_Topology *graph_ptr = NULL;
    MPIR_CHKPMEM_DECL(3);

    /* Create a new communicator */
    if (reorder) {
        int nrank;

        /* Allow the cart map routine to remap the assignment of ranks to
         * processes */
        mpi_errno = MPIR_Graph_map_impl(comm_ptr, nnodes, indx, edges, &nrank);
        MPIR_ERR_CHECK(mpi_errno);
        /* Create the new communicator with split, since we need to reorder
         * the ranks (including the related internals, such as the connection
         * tables */
        mpi_errno = MPIR_Comm_split_impl(comm_ptr,
                                         nrank == MPI_UNDEFINED ? MPI_UNDEFINED : 1,
                                         nrank, &newcomm_ptr);
        MPIR_ERR_CHECK(mpi_errno);
    } else {
        /* Just use the first nnodes processes in the communicator */
        mpi_errno = MPII_Comm_copy((MPIR_Comm *) comm_ptr, nnodes, NULL, &newcomm_ptr);
        MPIR_ERR_CHECK(mpi_errno);
    }


    /* If this process is not in the resulting communicator, return a
     * null communicator and exit */
    if (!newcomm_ptr) {
        *comm_graph_ptr = NULL;
        goto fn_exit;
    }

    nedges = indx[nnodes - 1];
    MPIR_CHKPMEM_MALLOC(graph_ptr, MPIR_Topology *, sizeof(MPIR_Topology),
                        mpi_errno, "graph_ptr", MPL_MEM_COMM);

    graph_ptr->kind = MPI_GRAPH;
    graph_ptr->topo.graph.nnodes = nnodes;
    graph_ptr->topo.graph.nedges = nedges;
    MPIR_CHKPMEM_MALLOC(graph_ptr->topo.graph.index, int *,
                        nnodes * sizeof(int), mpi_errno, "graph.index", MPL_MEM_COMM);
    MPIR_CHKPMEM_MALLOC(graph_ptr->topo.graph.edges, int *,
                        nedges * sizeof(int), mpi_errno, "graph.edges", MPL_MEM_COMM);
    for (i = 0; i < nnodes; i++)
        graph_ptr->topo.graph.index[i] = indx[i];
    for (i = 0; i < nedges; i++)
        graph_ptr->topo.graph.edges[i] = edges[i];

    /* Finally, place the topology onto the new communicator and return the
     * handle */
    mpi_errno = MPIR_Topology_put(newcomm_ptr, graph_ptr);
    MPIR_ERR_CHECK(mpi_errno);

    *comm_graph_ptr = newcomm_ptr;

  fn_exit:
    return mpi_errno;
  fn_fail:
    MPIR_CHKPMEM_REAP();
    goto fn_exit;
}

int MPIR_Graph_create_impl(MPIR_Comm * comm_ptr, int nnodes,
                           const int indx[], const int edges[], int reorder,
                           MPIR_Comm ** comm_graph_ptr)
{
    if (comm_ptr->topo_fns != NULL && comm_ptr->topo_fns->graphCreate != NULL) {
        /* --BEGIN USEREXTENSION-- */
        return comm_ptr->topo_fns->graphCreate(comm_ptr, nnodes, indx, edges, reorder,
                                               comm_graph_ptr);
        /* --END USEREXTENSION-- */
    } else {
        return graph_create(comm_ptr, nnodes, indx, edges, reorder, comm_graph_ptr);
    }
}

int MPIR_Graph_get_impl(MPIR_Comm * comm_ptr, int maxindex, int maxedges, int indx[], int edges[])
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Topology *topo_ptr;
    int i, n, *vals;

    topo_ptr = MPIR_Topology_get(comm_ptr);

    MPIR_ERR_CHKANDJUMP((!topo_ptr ||
                         topo_ptr->kind != MPI_GRAPH), mpi_errno, MPI_ERR_TOPOLOGY,
                        "**notgraphtopo");
    MPIR_ERR_CHKANDJUMP3((topo_ptr->topo.graph.nnodes > maxindex), mpi_errno, MPI_ERR_ARG,
                         "**argtoosmall", "**argtoosmall %s %d %d", "maxindex", maxindex,
                         topo_ptr->topo.graph.nnodes);
    MPIR_ERR_CHKANDJUMP3((topo_ptr->topo.graph.nedges > maxedges), mpi_errno, MPI_ERR_ARG,
                         "**argtoosmall", "**argtoosmall %s %d %d", "maxedges", maxedges,
                         topo_ptr->topo.graph.nedges);

    /* Get index */
    n = topo_ptr->topo.graph.nnodes;
    vals = topo_ptr->topo.graph.index;
    for (i = 0; i < n; i++)
        *indx++ = *vals++;

    /* Get edges */
    n = topo_ptr->topo.graph.nedges;
    vals = topo_ptr->topo.graph.edges;
    for (i = 0; i < n; i++)
        *edges++ = *vals++;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int graph_map(const MPIR_Comm * comm_ptr, int nnodes,
                     const int indx[]ATTRIBUTE((unused)),
                     const int edges[]ATTRIBUTE((unused)), int *newrank)
{
    MPL_UNREFERENCED_ARG(indx);
    MPL_UNREFERENCED_ARG(edges);

    /* This is the trivial version that does not remap any processes. */
    if (comm_ptr->rank < nnodes) {
        *newrank = comm_ptr->rank;
    } else {
        *newrank = MPI_UNDEFINED;
    }
    return MPI_SUCCESS;
}

int MPIR_Graph_map_impl(MPIR_Comm * comm_ptr, int nnodes,
                        const int indx[], const int edges[], int *newrank)
{
    int mpi_errno = MPI_SUCCESS;

    if (comm_ptr->topo_fns != NULL && comm_ptr->topo_fns->graphMap != NULL) {
        /* --BEGIN USEREXTENSION-- */
        mpi_errno = comm_ptr->topo_fns->graphMap(comm_ptr, nnodes,
                                                 (const int *) indx, (const int *) edges, newrank);
        MPIR_ERR_CHECK(mpi_errno);
        /* --END USEREXTENSION-- */
    } else {
        mpi_errno = graph_map(comm_ptr, nnodes, (const int *) indx, (const int *) edges, newrank);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* any non-MPI functions go here, especially non-static ones */

int MPIR_Graph_neighbors_impl(MPIR_Comm * comm_ptr, int rank, int maxneighbors, int neighbors[])
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Topology *graph_ptr;
    int i, is, ie;

    graph_ptr = MPIR_Topology_get(comm_ptr);

    MPIR_ERR_CHKANDJUMP((!graph_ptr ||
                         graph_ptr->kind != MPI_GRAPH), mpi_errno, MPI_ERR_TOPOLOGY,
                        "**notgraphtopo");
    MPIR_ERR_CHKANDJUMP2((rank < 0 ||
                          rank >= graph_ptr->topo.graph.nnodes), mpi_errno, MPI_ERR_RANK, "**rank",
                         "**rank %d %d", rank, graph_ptr->topo.graph.nnodes);

    /* Get location in edges array of the neighbors of the specified rank */
    if (rank == 0)
        is = 0;
    else
        is = graph_ptr->topo.graph.index[rank - 1];
    ie = graph_ptr->topo.graph.index[rank];

    /* Get neighbors */
    for (i = is; i < ie; i++)
        *neighbors++ = graph_ptr->topo.graph.edges[i];
  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Graph_neighbors_count_impl(MPIR_Comm * comm_ptr, int rank, int *nneighbors)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Topology *graph_ptr;

    graph_ptr = MPIR_Topology_get(comm_ptr);

    MPIR_ERR_CHKANDJUMP((!graph_ptr ||
                         graph_ptr->kind != MPI_GRAPH), mpi_errno, MPI_ERR_TOPOLOGY,
                        "**notgraphtopo");
    MPIR_ERR_CHKANDJUMP2((rank < 0 ||
                          rank >= graph_ptr->topo.graph.nnodes), mpi_errno, MPI_ERR_RANK, "**rank",
                         "**rank %d %d", rank, graph_ptr->topo.graph.nnodes);

    if (rank == 0)
        *nneighbors = graph_ptr->topo.graph.index[rank];
    else
        *nneighbors = graph_ptr->topo.graph.index[rank] - graph_ptr->topo.graph.index[rank - 1];

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Graphdims_get_impl(MPIR_Comm * comm_ptr, int *nnodes, int *nedges)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Topology *topo_ptr = MPIR_Topology_get(comm_ptr);
    MPIR_ERR_CHKANDJUMP((!topo_ptr || topo_ptr->kind != MPI_GRAPH),
                        mpi_errno, MPI_ERR_TOPOLOGY, "**notgraphtopo");
    *nnodes = topo_ptr->topo.graph.nnodes;
    *nedges = topo_ptr->topo.graph.nedges;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Topo_test_impl(MPIR_Comm * comm_ptr, int *status)
{
    MPIR_Topology *topo_ptr = MPIR_Topology_get(comm_ptr);
    if (topo_ptr) {
        *status = (int) (topo_ptr->kind);
    } else {
        *status = MPI_UNDEFINED;
    }
    return MPI_SUCCESS;
}
