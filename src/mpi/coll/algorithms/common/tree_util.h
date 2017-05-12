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


static inline int COLL_ilog(int k, int number)
{
    int i = 1, p = k - 1;

    for (; p - 1 < number; i++)
        p *= k;

    return i;
}

static inline int COLL_ipow(int base, int exp)
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

static inline int COLL_getdigit(int k, int number, int digit)
{
    return (number / COLL_ipow(k, digit)) % k;
}

static inline int COLL_setdigit(int k, int number, int digit, int newdigit)
{
    int res = number;
    int lshift = COLL_ipow(k, digit);
    res -= COLL_getdigit(k, number, digit) * lshift;
    res += newdigit * lshift;
    return res;
}

static inline void COLL_tree_add_child(COLL_tree_t * t, int rank)
{
    if (t->numRanges > 0 && t->children[t->numRanges - 1].endRank == rank - 1)
        t->children[t->numRanges - 1].endRank = rank;
    else {
        t->numRanges++;
        COLL_Assert(t->numRanges < COLL_MAX_TREE_BREADTH);
        t->children[t->numRanges - 1].startRank = rank;
        t->children[t->numRanges - 1].endRank = rank;
    }
}

static inline void COLL_tree_kary_init(int rank, int nranks, int k, int root, COLL_tree_t * ct)
{
    ct->rank = rank;
    ct->nranks = nranks;
    ct->numRanges = 0;
    ct->parent = -1;

    if (nranks <= 0)
        return;

    int lrank = (rank + (nranks - root)) % nranks;
    int child;
    COLL_Assert(k >= 1);

    ct->parent = (lrank <= 0) ? -1 : (((lrank - 1) / k) + root) % nranks;

    for (child = 1; child <= k; child++) {
        int val = lrank * k + child;
        if (val >= nranks)
            break;
        val = (val + root) % nranks;
        COLL_tree_add_child(ct, val);
    }
}

static inline void COLL_tree_knomial_init(int rank, int nranks, int k, int root, COLL_tree_t * ct)
{
    ct->rank = rank;
    ct->nranks = nranks;
    ct->numRanges = 0;
    ct->parent = -1;

    if (nranks <= 0)
        return;

    int lrank = (rank + (nranks - root)) % nranks;
    int basek, i, j;
    COLL_Assert(k >= 2);

    int maxtime = 0;            /*maximum number of steps while generating the knomial tree */
    int tmp = nranks - 1;
    while (tmp) {
        maxtime++;
        tmp /= k;
    }

    int time = 0, _k;
    int parent = -1;            /*root has no parent */
    int current_rank = 0;       /*start at root of the tree */
    int running_rank;           /*used for calculation below */
    running_rank = current_rank + 1;    /*start with first child of the current_rank */
    while (true) {
        if (lrank == current_rank)      /*desired rank found */
            break;
        for (j = 1; j < k; j++) {
            if (lrank >= running_rank && lrank < running_rank + COLL_ipow(k, maxtime - time - 1)) {     /*check if rank lies in this range */
                /*move to the corresponding subtree */
                parent = current_rank;
                current_rank = running_rank;
                running_rank = current_rank + 1;
                break;
            }
            else
                running_rank += COLL_ipow(k, maxtime - time - 1);
        }
        time++;
    }
    if (parent == -1)
        ct->parent = -1;
    else
        ct->parent = (parent + root) % nranks;
    int crank = lrank + 1;      /*cranks stands for child rank */
    if (0)
        fprintf(stderr, "parent of rank %d is %d, total ranks = %d (root=%d)\n", rank, ct->parent,
                nranks, root);
    for (i = time; i < maxtime; i++) {
        for (j = 1; j < k; j++) {
            if (crank < nranks) {
                if (0)
                    fprintf(stderr, "adding child %d to rank %d\n", (crank + root) % nranks, rank);
                COLL_tree_add_child(ct, (crank + root) % nranks);
            }
            crank += COLL_ipow(k, maxtime - i - 1);
        }
    }
}
