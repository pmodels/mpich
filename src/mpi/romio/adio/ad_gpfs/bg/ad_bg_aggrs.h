/* ---------------------------------------------------------------- */
/* (C)Copyright IBM Corp.  2007, 2008                               */
/* ---------------------------------------------------------------- */
/**
 * \file ad_bg_aggrs.h
 * \brief ???
 */

/* 
 * File: ad_bg_aggrs.h
 * 
 * Declares functions specific for BG/L - GPFS parallel I/O solution. The implemented optimizations are:
 * 	. Aligned file-domain partitioning, integrated in 7/28/2005
 * 
 * In addition, following optimizations are planned:
 * 	. Integrating multiple file-domain partitioning schemes 
 *	  (corresponding to Alok Chouhdary's persistent file domain work).
 */

#ifndef AD_BG_AGGRS_H_
#define AD_BG_AGGRS_H_

#include "adio.h"
#include <sys/stat.h>

#if !defined(GPFS_SUPER_MAGIC)
  #define GPFS_SUPER_MAGIC (0x47504653)
#endif

    /* generate a list of I/O aggregators that utilizes BG-PSET orginization. */
    int ADIOI_BG_gen_agg_ranklist(ADIO_File fd, int n_aggrs_per_pset);

#endif  /* AD_BG_AGGRS_H_ */
