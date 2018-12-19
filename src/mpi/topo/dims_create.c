/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/*
 * This CVAR is used for debugging support.  An alternative would be
 * to use the MPIU_DBG interface, which predates the CVAR interface,
 * and also provides different levels of debugging support.  In the
 * long run, the MPIU_DBG interface should be updated to make use of
 * CVARs.
 */
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

/* -- Begin Profiling Symbol Block for routine MPI_Dims_create */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Dims_create = PMPI_Dims_create
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Dims_create  MPI_Dims_create
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Dims_create as PMPI_Dims_create
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Dims_create(int nnodes, int ndims, int dims[])
    __attribute__ ((weak, alias("PMPI_Dims_create")));
#endif
/* -- End Profiling Symbol Block */



/* Because we store factors with their multiplicities, a small array can
   store all of the factors for a large number (grows *faster* than n
   factorial). */
#define MAX_FACTORS 10
/* 2^20 is a millon */
#define MAX_DIMS    20

typedef struct Factors {
    int val, cnt;
} Factors;

/* These routines may be global if we are not using weak symbols */
PMPI_LOCAL int MPIR_Dims_create_init(void);
PMPI_LOCAL int MPIR_Dims_create_impl(int nnodes, int ndims, int dims[]);

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines.  You can use USE_WEAK_SYMBOLS to see if MPICH is
   using weak symbols to implement the MPI routines. */

#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Dims_create
#define MPI_Dims_create PMPI_Dims_create

PMPI_LOCAL MPIR_T_pvar_timer_t PVAR_TIMER_dims_getdivs;
PMPI_LOCAL MPIR_T_pvar_timer_t PVAR_TIMER_dims_sort;
PMPI_LOCAL MPIR_T_pvar_timer_t PVAR_TIMER_dims_fact;
PMPI_LOCAL MPIR_T_pvar_timer_t PVAR_TIMER_dims_basefact;
PMPI_LOCAL MPIR_T_pvar_timer_t PVAR_TIMER_dims_div;
PMPI_LOCAL MPIR_T_pvar_timer_t PVAR_TIMER_dims_bal;

PMPI_LOCAL unsigned long long PVAR_COUNTER_dims_npruned;
PMPI_LOCAL unsigned long long PVAR_COUNTER_dims_ndivmade;
PMPI_LOCAL unsigned long long PVAR_COUNTER_dims_optbalcalls;

/* MPI_Dims_create and PMPI_Dims_create must see the same variable for this
   one-time initialization flag.  If this file must be compiled twice,
   this variable is defined here and as external for the other build. */
volatile int MPIR_DIMS_initPCVars = 1;

/* This routine is called once to define any PVARS and CVARS */
PMPI_LOCAL int MPIR_Dims_create_init(void)
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

#undef FCNAME
#define FCNAME "factor_to_divisors"
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
#undef FC_NAME
#define FC_NAME "optbalance"

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
        if (mpi_errno)
            return mpi_errno;

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
            } else {
                sf = divs[k + 1];
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
            if (nndivs > 0)
                optbalance(q, idx - 1, nd, nndivs, newdivs, trydims, curbal_p, optdims);
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
            return 0;
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
    return 0;
  fn_fail:
    return mpi_errno;
}


/* FIXME: The error checking should really be part of MPI_Dims_create,
   not part of MPIR_Dims_create_impl.  That slightly changes the
   semantics of Dims_create provided by the device, but only by
   removing the need to check for errors */


PMPI_LOCAL int MPIR_Dims_create_impl(int nnodes, int ndims, int dims[])
{
    Factors f[MAX_FACTORS];
    int nf, nprimes = 0, i, j, k, val, nextidx;
    int ndivs, curbal;
    int trydims[MAX_DIMS];
    int dims_needed, dims_product, mpi_errno;
    int chosen[MAX_DIMS], foundDecomp;
    int *divs;
    MPIR_CHKLMEM_DECL(1);

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
    optbalance(nnodes, dims_needed - nextidx - 1, dims_needed - nextidx,
               ndivs, divs, trydims, &curbal, chosen + nextidx);
    MPIR_T_PVAR_TIMER_END(DIMS, dims_bal);
    MPIR_CHKLMEM_FREEALL();

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

    return MPI_SUCCESS;
  fn_fail:
    return mpi_errno;
}

#else
/* MPI_Dims_create and PMPI_Dims_create must see the same variable for this
   one-time initialization flag */
extern volatile int MPIR_DIMS_initPCVars;

#endif /* PMPI Local */

#undef FUNCNAME
#define FUNCNAME MPI_Dims_create
#undef FCNAME
#define FCNAME "MPI_Dims_create"

/*@
    MPI_Dims_create - Creates a division of processors in a cartesian grid

Input Parameters:
+ nnodes - number of nodes in a grid (integer)
- ndims - number of cartesian dimensions (integer)

Input/Output Parameters:
. dims - integer array of size  'ndims' specifying the number of nodes in each
 dimension.  A value of 0 indicates that 'MPI_Dims_create' should fill in a
 suitable value.

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Dims_create(int nnodes, int ndims, int dims[])
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_DIMS_CREATE);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_DIMS_CREATE);

    if (ndims == 0)
        goto fn_exit;

    /* Validate parameters and objects (post conversion) */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_ARGNEG(nnodes, "nnodes", mpi_errno);
            MPIR_ERRTEST_ARGNEG(ndims, "ndims", mpi_errno);
            MPIR_ERRTEST_ARGNULL(dims, "dims", mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* Initialize pvars and cvars if this is the first call */
    if (MPIR_DIMS_initPCVars) {
        MPIR_Dims_create_init();
        MPIR_DIMS_initPCVars = 0;
    }

    /* ... body of routine ...  */
    if (MPIR_Process.dimsCreate != NULL) {
        mpi_errno = MPIR_Process.dimsCreate(nnodes, ndims, dims);
    } else {
        mpi_errno = MPIR_Dims_create_impl(nnodes, ndims, dims);
    }
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);
    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_DIMS_CREATE);
    return mpi_errno;

    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
#ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno =
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
                                 "**mpi_dims_create", "**mpi_dims_create %d %d %p", nnodes, ndims,
                                 dims);
    }
#endif
    mpi_errno = MPIR_Err_return_comm(NULL, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
