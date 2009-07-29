/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef CKPOINT_BLCR_H_INCLUDED
#define CKPOINT_BLCR_H_INCLUDED

#if defined HAVE_BLCR
#include "libcr.h"
#endif /* HAVE_BLCR */

HYD_Status HYDU_ckpoint_blcr_suspend(void);
HYD_Status HYDU_ckpoint_blcr_restart(void);

#endif /* CKPOINT_BLCR_H_INCLUDED */
