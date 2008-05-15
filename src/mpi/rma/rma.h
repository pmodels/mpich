/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef RMA_H_INCLUDED
#define RMA_H_INCLUDED

/*
 * To provide more flexibility in the handling of RMA operations, we provide 
 * these options:
 *
 *  Statically defined ADI routines
 *      MPID_Put etc, provided by the ADI
 *  Dynamically defined routines
 *      A function table is used, initialized by ???
 *
 * Which of these is used is selected by the device.  If
 *    USE_MPID_RMA_TABLE 
 * is defined, then the function table is used.  Otherwise, the calls 
 * turn into MPID_<Rma operation>, e.g., MPID_Put or MPID_Win_create.
 */

/* We need to export this header file (at least the struct) to the
   device, so that it can implement the init routine. */
#ifdef USE_MPID_RMA_TABLE
#define MPIU_RMA_CALL(winptr,funccall) (winptr)->RMAFns.funccall

#else
/* Just use the MPID_<fcn> version of the function */
#define MPIU_RMA_CALL(winptr,funccall) MPID_##funccall

#endif

#endif
