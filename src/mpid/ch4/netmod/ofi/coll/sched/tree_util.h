/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2012 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

#ifndef COLLTREE_UTIL_H_INCLUDED
#define COLLTREE_UTIL_H_INCLUDED

static inline
int COLL_ilog(int k, int number)
{
    int i=1, p=k-1;

    for(; p-1<number; i++)
        p *= k;

    return i;
}

static inline
int COLL_ipow(int base, int exp)
{
    int result = 1;

    while(exp) {
        if(exp & 1)
            result *= base;

        exp >>= 1;
        base *= base;
    }

    return result;
}

static inline
int COLL_getdigit(int k,
                  int number,
                  int digit)
{
    return (number/COLL_ipow(k, digit))%k;
}

static inline
int COLL_setdigit(int k,int number,
                  int digit,int newdigit)
{
    int res     = number;
    int lshift  = COLL_ipow(k, digit);
    res        -= COLL_getdigit(k, number, digit)*lshift;
    res        += newdigit*lshift;
    return res;
}

#endif /* COLLTREE_UTIL_H_INCLUDED */


