/*  (C)Copyright IBM Corp.  2007, 2008  */
/**
 * \file src/misc/mpid_time.c
 * \brief Devince interface between MPI_Wtime() and DCMF_Timer()
 */
#include "mpidimpl.h"

#if MPICH_TIMER_KIND != USE_DEVICE
#error "Not using USE_BG_TIMEBASE"
#endif


void MPID_Wtime( MPID_Time_t *tval )
{
  *tval = DCMF_Timer();
}
double MPID_Wtick()
{
  return DCMF_Tick();
}
void MPID_Wtime_diff( MPID_Time_t *t1, MPID_Time_t *t2, double *diff )
{
  *diff = *t2 - *t1;
}
void MPID_Wtime_todouble( MPID_Time_t *t, double *val )
{
  *val = *t;
}
void MPID_Wtime_acc( MPID_Time_t *t1, MPID_Time_t *t2, MPID_Time_t *t3 )
{
  *t3 += *t1 - *t2;
}
void MPID_Wtime_init()
{
  /* We used to call DCMF_Timer() here, but the messager wasn't created yet */
}
