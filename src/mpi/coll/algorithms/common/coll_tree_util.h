/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2017 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

/* Routine to calculate log_k of an integer */
MPL_STATIC_INLINE_PREFIX int COLL_ilog(int k, int number)
{
    int i = 1, p = k - 1;

    for (; p - 1 < number; i++)
        p *= k;

    return i;
}

/* Routing to calculate base^exp for integers */
MPL_STATIC_INLINE_PREFIX int COLL_ipow(int base, int exp)
{
    int result = 1;

    while (exp) {
        if (exp & 1)
            result *= base;

        exp >>= 1;
        base *= base;
    }

    return result;
}

/* get the number at 'digit'th location in base k representation of 'number' */
MPL_STATIC_INLINE_PREFIX int COLL_getdigit(int k, int number, int digit)
{
    return (number / COLL_ipow(k, digit)) % k;
}

/* set the number at 'digit'the location in base k representation of 'number' to newdigit */
MPL_STATIC_INLINE_PREFIX int COLL_setdigit(int k, int number, int digit, int newdigit)
{
    int res = number;
    int lshift = COLL_ipow(k, digit);
    res -= COLL_getdigit(k, number, digit) * lshift;
    res += newdigit * lshift;
    return res;
}

/* utility function to add a child to COLL_tree_t data structure */
MPL_STATIC_INLINE_PREFIX void COLL_tree_add_child(COLL_tree_t * t, int rank)
{
    if(t->num_children + 1 > t->max_children){
       int* old_children = t->children;
       t->children = (int*) MPL_malloc(sizeof(int) * 2 * t->max_children);
       t->max_children = 2 * t->max_children;
       memcpy(t->children, old_children, sizeof(int)*t->max_children);
       MPL_free(old_children);
    }
    t->children[t->num_children++] = rank;
}

/* Function to generate kary tree information for rank 'rank' and store it in COLL_tree_t data structure */
/*     4 ranks                                                8 ranks
 *       0                                                      0
 *     /  \                                                 /       \
 *    1   2                                                1         2
 *    |                                                  /   \     /   \
 *    3                                                 3     4   5     6
 *                                                      |
 *                                                      7
 */
MPL_STATIC_INLINE_PREFIX void COLL_tree_kary_init(int rank, int nranks, int k, int root, COLL_tree_t * ct)
{
    int lrank, child;

    ct->rank = rank;
    ct->nranks = nranks;
    ct->num_children = 0;
    ct->parent = -1;
    ct->children = (int*) MPL_malloc(sizeof(int)*k);
    ct->max_children = k;

    if (nranks <= 0)
        return;

    lrank = (rank + (nranks - root)) % nranks;
    MPIC_Assert(k >= 1);

    ct->parent = (lrank <= 0) ? -1 : (((lrank - 1) / k) + root) % nranks;

    for (child = 1; child <= k; child++) {
        int val = lrank * k + child;
        if (val >= nranks)
            break;
        val = (val + root) % nranks;
        COLL_tree_add_child(ct, val);
    }
}

/* Function to generate knomial tree information for rank 'rank' and store it in COLL_tree_t data structure */
/*     4 ranks                                                8 ranks
 *       0                                                      0
 *     /  \                                                 /   |   \
 *    1   3                                               1     5    7
 *    |                                                 /   \   |
 *    2                                                2     4  6
 *                                                     |
 *                                                     3
 */
MPL_STATIC_INLINE_PREFIX void COLL_tree_knomial_1_init(int rank, int nranks, int k, int root, COLL_tree_t * ct)
{
    int lrank, i, j, maxtime, tmp, time, parent, current_rank, running_rank, crank;

    ct->rank = rank;
    ct->nranks = nranks;
    ct->num_children = 0;
    ct->parent = -1;

    if (nranks <= 0)
        return;

    lrank = (rank + (nranks - root)) % nranks;
    MPIC_Assert(k >= 2);

    maxtime = 0;            /* maximum number of steps while generating the knomial tree */
    tmp = nranks - 1;
    while (tmp) {
        maxtime++;
        tmp /= k;
    }

    ct->children = (int*) MPL_malloc(sizeof(int)*(maxtime)*(k-1));
    ct->max_children = maxtime*(k-1);
    time = 0;
    parent = -1;                     /* root has no parent */
    current_rank = 0;                /* start at root of the tree */
    running_rank = current_rank + 1; /* used for calculation below
                                      * start with first child of the current_rank */

    while (true) {
        if (lrank == current_rank) /* desired rank found */
            break;
        for (j = 1; j < k; j++) {
            if (lrank >= running_rank && lrank < running_rank + COLL_ipow(k, maxtime - time - 1)) {     /* check if rank lies in this range */
                /* move to the corresponding subtree */
                parent = current_rank;
                current_rank = running_rank;
                running_rank = current_rank + 1;
                break;
            } else
                running_rank += COLL_ipow(k, maxtime - time - 1);
        }
        time++;
        MPIR_Assert(time <= nranks); /* actually, should not need more steps than log_k(nranks)*/
    }

    if (parent == -1)
        ct->parent = -1;
    else
        ct->parent = (parent + root) % nranks;

    crank = lrank + 1; /* cranks stands for child rank */
    MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"parent of rank %d is %d, total ranks = %d (root=%d)\n", rank, ct->parent,
             nranks, root));
    for (i = time; i < maxtime; i++) {
        for (j = 1; j < k; j++) {
            if (crank < nranks) {
                MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"adding child %d to rank %d\n", (crank + root) % nranks, rank);
                COLL_tree_add_child(ct, (crank + root) % nranks));
            }
            crank += COLL_ipow(k, maxtime - i - 1);
        }
    }
}

/* Function to generate knomial tree information optimized for bcast for rank 'rank' and store it in COLL_tree_t data structure */
/*     4 ranks                                               8 ranks
 *       0                                                      0
 *     /  \                                                 /   |   \
 *    2    1                                              4     2    1
 *    |                                                 /   \   |
 *    3                                                6     5  3
 *                                                     |
 *                                                     7
 */
MPL_STATIC_INLINE_PREFIX void COLL_tree_knomial_2_init(int rank, int nranks, int k, int root, COLL_tree_t * ct)
{
    int lrank, i, j, basek;

    ct->rank = rank;
    ct->nranks = nranks;
    ct->num_children = 0;
    ct->parent = -1;

    if (nranks <= 0)
        return;

    lrank = (rank + (nranks - root)) % nranks;
    MPIC_Assert(k >= 2);
    /* Parent Calculation */
    if (lrank <= 0)
        ct->parent = -1;
    else {
        basek = COLL_ilog(k, nranks - 1);

        for (i = 0; i < basek; i++) {
            if (COLL_getdigit(k, lrank, i)) {
                ct->parent = (COLL_setdigit(k, lrank, i, 0) + root) % nranks;
                break;
            }
        }
    }

    if (lrank >= nranks)
        return;

    /* Children Calculation */
    basek = COLL_ilog(k, nranks - 1);
    int * flip_bit = (int*) MPL_calloc(basek,sizeof(int));
    ct->children = (int*) MPL_malloc(sizeof(int)*basek*(k-1));
    ct->max_children = basek*(k-1);

    for (j = 0 ; j < basek ; j++) {
        if (COLL_getdigit(k, lrank, j)){
            break;
        }
        flip_bit[j] = 1;
    }
    for (j = basek-1 ; j >= 0  ; j--) {
        if(flip_bit[j] == 1) {
            for (i = k-1 ; i >= 1 ; i--) {
                int child = COLL_setdigit(k, lrank, j, i);
                MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"child %d i %d basek %d j %d \n", child, i,basek,j));
                if (child < nranks)
                    COLL_tree_add_child(ct, (child + root) % nranks);
            }
        }
    }
    MPL_free(flip_bit);
    MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"parent of rank %d is %d, total ranks = %d (root=%d)\n", rank, ct->parent,
             nranks, root));
}
