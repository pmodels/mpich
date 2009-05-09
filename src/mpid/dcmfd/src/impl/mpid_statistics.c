/*  (C)Copyright IBM Corp.  2007, 2008  */
/**
 * \file src/impl/mpid_statistics.c
 * \brief The statistics implementation
 */

#include "mpidimpl.h"

#ifdef USE_STATISTICS
/* #warning Statistics are ON */

MPIDI_Statistics_t MPIDI_Statistics;

#define MPIDI_Statistics_print(r, s)                            \
({                                                              \
  double s0 = s.s0;                                             \
  double s1 = s.s1;                                             \
  double s2 = s.s2;                                             \
  printf("%3d: Statistics for \"" #s "\":\n", r);               \
  printf("     Count    : %zu\n", s.s0);                         \
  printf("     Max      : %zu\n", s.max);                        \
  printf("     Mean     : %g\n", s1/s0);                        \
  printf("     Variance : %g\n", (s0*s2 - s1*s1) / (s0*s0));    \
})

void MPIDI_Statistics_init()
{
  memset(&MPIDI_Statistics, 0, sizeof(MPIDI_Statistics));
}

void MPIDI_Statistics_finalize()
{
  if (MPIDI_Process.statistics)
    {
      MPIDI_Statistics_print(0, MPIDI_Statistics.recvq.posted_search);
      MPIDI_Statistics_print(0, MPIDI_Statistics.recvq.unexpected_search);
    }
}

#else
/* #warning Statistic are OFF */
void MPIDI_Statistics_init()     {}
void MPIDI_Statistics_finalize() {}
#endif
