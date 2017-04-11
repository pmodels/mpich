/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2017 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef HYDRA_HASH_H_INCLUDED
#define HYDRA_HASH_H_INCLUDED

#include "hydra_base.h"

struct HYD_int_hash {
    int key;
    int val;
    MPL_UT_hash_handle hh;
};

#endif /* HYDRA_HASH_H_INCLUDED */
