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
#include "mpiimpl.h"

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
    char *ioAgentCount = getenv("MP_IOAGENT_CNT");
    int i,j;
    int inTERcommFlag = 0;

    int myRank;
    MPI_Comm_rank(fd->comm, &myRank);

    MPI_Comm_test_inter(fd->comm, &inTERcommFlag);
    if (inTERcommFlag) {
      FPRINTF(stderr,"inTERcomms are not supported in MPI-IO - aborting....\n");
      perror("ADIOI_PE_gen_agg_ranklist:");
      MPI_Abort(MPI_COMM_WORLD, 1);
    }

    if (ioTaskList) {
      char tmpBuf[8];   /* Big enough for 1M tasks (7 digits task ID). */
      tmpBuf[7] = '\0';
      for (i=0; i<7; i++) {
         tmpBuf[i] = *ioTaskList++;      /* Maximum is 7 digits for 1 million. */
         if (*ioTaskList == ':') {       /* If the next char is a ':' ends it. */
             tmpBuf[i+1] = '\0';
             break;
         }
      }
      numAggs = atoi(tmpBuf);
      fd->hints->ranklist = (int *) ADIOI_Malloc (numAggs * sizeof(int));

      for (j=0; j<numAggs; j++) {
         ioTaskList++;                /* Advance past the ':' */
         for (i=0; i<7; i++) {
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
    }
    else {
      MPID_Comm *mpidCommData;

      MPID_Comm_get_ptr(fd->comm,mpidCommData);
      int localSize = mpidCommData->local_size;

      // get my node rank
      int myNodeRank = mpidCommData->intranode_table[mpidCommData->rank];

      int *allNodeRanks = (int *) ADIOI_Malloc (localSize * sizeof(int));

      allNodeRanks[myRank] = myNodeRank;
      MPI_Allgather(MPI_IN_PLACE, 1, MPI_INT, allNodeRanks, 1, MPI_INT, fd->comm);

#ifdef AGG_DEBUG
      printf("MPID_Comm data: remote_size is %d local_size is %d\nintranode_table entries:\n",mpidCommData->remote_size,mpidCommData->local_size);
      for (i=0;i<localSize;i++) {
        printf("%d ",mpidCommData->intranode_table[i]);
      }
      printf("\ninternode_table entries:\n");
      for (i=0;i<localSize;i++) {
        printf("%d ",mpidCommData->internode_table[i]);
      }
      printf("\n");

      printf("\allNodeRanks entries:\n");
      for (i=0;i<localSize;i++) {
        printf("%d ",allNodeRanks[i]);
      }
      printf("\n");

#endif

      if (ioAgentCount) {
        int cntType = -1;

        if ( strcasecmp(ioAgentCount, "ALL") ) {
           if ( (cntType = atoi(ioAgentCount)) <= 0 ) {
              /* Input is other non-digit or less than 1 the  assume */
              /* 1 agent per node.  Note: atoi(-1) reutns -1.        */
              /* No warning message given here -- done earlier.      */
              cntType = -1;
           }
        }
        else {
           /* ALL is specified set agent count to localSize */
           cntType = -2;
        }
        switch(cntType) {
           case -1:
              /* 1 agent/node case */
             {
             int rankListIndex = 0;
             fd->hints->ranklist = (int *) ADIOI_Malloc (localSize * sizeof(int));
             for (i=0;i<localSize;i++) {
               if (allNodeRanks[i] == 0) {
                 fd->hints->ranklist[rankListIndex++] = i;
                 numAggs++;
               }
             }
             }
              break;
           case -2:
              /* ALL tasks case */
             fd->hints->ranklist = (int *) ADIOI_Malloc (localSize * sizeof(int));
             for (i=0;i<localSize;i++) {
               fd->hints->ranklist[i] = i;
               numAggs++;
             }
              break;
           default:
              /* Specific agent count case -- MUST be less than localSize. */
              if (cntType <= localSize) {
                 numAggs = cntType;
                 // Round-robin thru allNodeRanks - pick the 0's, then the 1's, etc
                 int currentNodeRank = 0;  // node rank currently being selected as aggregator
                 int rankListIndex = 0;
                 int currentAllNodeIndex = 0;

                 fd->hints->ranklist = (int *) ADIOI_Malloc (numAggs * sizeof(int));

                 while (rankListIndex < numAggs) {

                   int foundEntry = 0;
                   while (!foundEntry && (currentAllNodeIndex < localSize)) {
                     if (allNodeRanks[currentAllNodeIndex] == currentNodeRank) {
                       fd->hints->ranklist[rankListIndex++] = currentAllNodeIndex;
                       foundEntry = 1;
                     }
                     currentAllNodeIndex++;
                   }
                   if (!foundEntry) {
                     currentNodeRank++;
                     currentAllNodeIndex = 0;
                   }
                } // while
              } // (cntType <= localSize)
              else {
                ADIOI_Assert(1);
              }
              break;
            } // switch(cntType)
        } // if (ioAgentCount)

      else { // default is 1 aggregator per node
        // take the 0 entries from allNodeRanks
          int rankListIndex = 0;
          fd->hints->ranklist = (int *) ADIOI_Malloc (localSize * sizeof(int));
          for (i=0;i<localSize;i++) {
            if (allNodeRanks[i] == 0) {
              fd->hints->ranklist[rankListIndex++] = i;
              numAggs++;
            }
          }
      }
    }

    if ( getenv("MP_I_SHOW_AGENTS") ) {
      if (myRank == 0) {
      printf("Agg rank list of %d generated:\n", numAggs);
      for (i=0;i<numAggs;i++) {
        printf("%d ",fd->hints->ranklist[i]);
      }
      printf("\n");
      }
    }

    fd->hints->cb_nodes = numAggs;

    return 0;
}

