/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "viaimpl.h"

#ifdef WITH_METHOD_VIA

int via_get_business_card(char *value, int length)
{
    MPIDI_STATE_DECL(MPID_STATE_VIA_GET_BUSINESS_CARD);
    MPIDI_FUNC_ENTER(MPID_STATE_VIA_GET_BUSINESS_CARD);

    if (length < 1)
    {
	MPIDI_FUNC_EXIT(MPID_STATE_VIA_GET_BUSINESS_CARD);
	return -1;
    }
    snprintf(value, length, "none");

    MPIDI_FUNC_EXIT(MPID_STATE_VIA_GET_BUSINESS_CARD);
    return MPI_SUCCESS;
}

#endif
