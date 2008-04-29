/*  (C)Copyright IBM Corp.  2007, 2008  */
/**
 * \file include/mpid_statistics.h
 * \brief Functions useful for instrumenting code and collecting performance statistics
 */

#ifndef MPID_STATISTICS_H
#define MPID_STATISTICS_H

/**
 * \defgroup STATISTICS Performance statistics
 * \brief These items handle the processing of performance counters
 */

/**
 * \ingroup STATISTICS
 * \brief Turn on the statistics code (or not)
 */
#define USE_STATISTICS
/* #undef  USE_STATISTICS */



#ifdef USE_STATISTICS

/**
 * \ingroup STATISTICS
 * \brief Storage for statistical collection
 */
typedef struct {
  size_t s0;  /**< The running sum of the input^0 (AKA add 1 every time) */
  size_t s1;  /**< The running sum of the input^1 */
  size_t s2;  /**< The running sum of the input^2 */
  size_t max; /**< The largest input seen so far */
} stat_time;

/**
 * \ingroup STATISTICS
 * \brief local storage for the individual counters
 */
typedef struct {
  struct {
    stat_time  posted_search;     /**< The counter for the posted queue */
    stat_time  unexpected_search; /**< The counter for the unexpected queue */
  } recvq; /**< Counters related to the recvq system */
} MPIDI_Statistics_t;

/**
 * \ingroup STATISTICS
 * \brief Add another event with the stats system
 * \param[in,out] s The current counter structure
 * \param[in]     m The time it took the event to complete
 * \returns m
 */
#define MPIDI_Statistics_time(s, m)             \
({                                              \
  s.s0 += 1;                                    \
  s.s1 += m;                                    \
  s.s2 += m*m;                                  \
  if (s.max < m)                                \
    s.max = m;                                  \
  m;                                            \
})
#else
#define MPIDI_Statistics_time(s, m) m
#endif

#ifdef USE_STATISTICS
/**
 * \ingroup STATISTICS
 * \brief External storage for the individual counters
 */
extern MPIDI_Statistics_t MPIDI_Statistics;
#endif


/**
 * \ingroup STATISTICS
 * \brief Initialize the individual counters
 *
 * Basically, this simply zeroes out the entire statistics structure.
 */
void MPIDI_Statistics_init();

/**
 * \ingroup STATISTICS
 * \brief Complete and optionally print the results from the stats collection
 *
 * The results are only printed when the MPIDI_Process.statistics flag is set.
 */
void MPIDI_Statistics_finalize();

#endif
