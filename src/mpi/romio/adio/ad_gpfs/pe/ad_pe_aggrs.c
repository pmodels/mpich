/* ---------------------------------------------------------------- */
/* (C)Copyright IBM Corp.  2007, 2008                               */
/* ---------------------------------------------------------------- */
/**
 * \file ad_pe_aggrs.c
 * \brief The externally used function from this file is is declared in ad_pe_aggrs.h
 */

/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *   Copyright (C) 1997-2001 University of Chicago.
 *   See COPYRIGHT notice in top-level directory.
 */

/*#define TRACE_ON */

#include "adio.h"
#include "adio_cb_config_list.h"
#include "../ad_gpfs.h"
#include "ad_pe_aggrs.h"
#ifdef AGGREGATION_PROFILE
#include "mpe.h"
#endif

#ifdef USE_DBG_LOGGING
  #define AGG_DEBUG 1
#endif

#ifndef TRACE_ERR
#  define TRACE_ERR(format...)
#endif

/*
 * Compute the aggregator-related parameters that are required in 2-phase collective IO of ADIO.
 * The parameters are
 * 	. the number of aggregators (proxies) : fd->hints->cb_nodes
 *	. the ranks of the aggregators :        fd->hints->ranklist
 * For now set these based on MP_IOTASKLIST
 */
int
ADIOI_PE_gen_agg_ranklist(ADIO_File fd)
{

    int numAggs = 0;
    char *ioTaskList = getenv( "MP_IOTASKLIST" );

    ADIOI_Assert(ioTaskList);

      char tmpBuf[8];   /* Big enough for 1M tasks (7 digits task ID). */
      tmpBuf[7] = '\0';
      for (int i=0; i<7; i++) {
         tmpBuf[i] = *ioTaskList++;      /* Maximum is 7 digits for 1 million. */
         if (*ioTaskList == ':') {       /* If the next char is a ':' ends it. */
             tmpBuf[i+1] = '\0';
             break;
         }
      }
      numAggs = atoi(tmpBuf);
      fd->hints->ranklist = (int *) ADIOI_Malloc (numAggs * sizeof(int));

      for (int j=0; j<numAggs; j++) {
         ioTaskList++;                /* Advance past the ':' */
         for (int i=0; i<7; i++) {
            tmpBuf[i] = *ioTaskList++;
            if ( (*ioTaskList == ':') || (*ioTaskList == '\0') ) {
                tmpBuf[i+1] = '\0';
                break;
            }
         }
         fd->hints->ranklist[j] = atoi(tmpBuf);

         /* At the end check whether the list is shorter than specified. */
         if (*ioTaskList == '\0') {
            if (j < (numAggs-1)) {
               numAggs = j;
            }
            break;
         }
      }

      fd->hints->cb_nodes = numAggs;

#ifdef AGG_DEBUG
  printf("Agg rank list of %d generated for PE:\n", numAggs);
  for (int i=0;i<numAggs;i++) {
    printf("%d ",fd->hints->ranklist[i]);
  }
  printf("\n");

#endif

    return 0;
}

