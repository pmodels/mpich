/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 * (C) 2001 by Argonne National Laboratory.
 *     See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"
#include "blockallocator.h"

BlockAllocator MM_Car_allocator;

void mm_car_init()
{
    MPIDI_STATE_DECL(MPID_STATE_MM_CAR_INIT);
    MPIDI_FUNC_ENTER(MPID_STATE_MM_CAR_INIT);
    MM_Car_allocator = BlockAllocInit(sizeof(MM_Car), 100, 100, malloc, free);
    MPIDI_FUNC_EXIT(MPID_STATE_MM_CAR_INIT);
}

void mm_car_finalize()
{
    MPIDI_STATE_DECL(MPID_STATE_MM_CAR_FINALIZE);
    MPIDI_FUNC_ENTER(MPID_STATE_MM_CAR_FINALIZE);
    BlockAllocFinalize(&MM_Car_allocator);
    MPIDI_FUNC_EXIT(MPID_STATE_MM_CAR_FINALIZE);
}

MM_Car* mm_car_alloc()
{
    MM_Car *pCar;
    MPIDI_STATE_DECL(MPID_STATE_MM_CAR_ALLOC);

    MPIDI_FUNC_ENTER(MPID_STATE_MM_CAR_ALLOC);

    pCar = BlockAlloc(MM_Car_allocator);
    pCar->freeme = TRUE;

    MPIDI_FUNC_EXIT(MPID_STATE_MM_CAR_ALLOC);

    return pCar;
}

void mm_car_free(MM_Car *car_ptr)
{
    MPIDI_STATE_DECL(MPID_STATE_MM_CAR_FREE);
    MPIDI_FUNC_ENTER(MPID_STATE_MM_CAR_FREE);
    if (car_ptr->freeme)
	BlockFree(MM_Car_allocator, car_ptr);
    MPIDI_FUNC_EXIT(MPID_STATE_MM_CAR_FREE);
}
