/* ---------------------------------------------------------------- */
/* (C)Copyright IBM Corp.  2007, 2008                               */
/* ---------------------------------------------------------------- */
/**
 * \file ad_tuning.h
 * \brief Defines common performance tuning env var options
 */

/*---------------------------------------------------------------------
 * ad_tuning.h
 *
 * declares common global variables and functions for performance tuning
 * and functional debugging.
 *---------------------------------------------------------------------*/

#ifndef AD_TUNING_H_
#define AD_TUNING_H_

#include "adio.h"


/*-----------------------------------------
 *  Global variables for the control of performance tuning.
 *-----------------------------------------*/

/* corresponds to environment variables to select optimizations and timing level */
extern int      romio_pthreadio;
extern int      romio_p2pcontig;
extern int      romio_write_aggmethod;
extern int      romio_read_aggmethod;
extern int      romio_onesided_no_rmw;
extern int      romio_onesided_always_rmw;
extern int      romio_onesided_inform_rmw;

/* set internal variables for tuning environment variables */
void ad_get_env_vars(void);

#endif  /* AD_TUNING_H_ */
