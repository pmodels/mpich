#include "adio.h"
#include "adio_extern.h"
#ifdef ROMIO_GPFS
/* right now this is GPFS only but TODO: extend this to all file systems */
#include "../ad_gpfs/ad_gpfs_tuning.h"
#else
int gpfsmpio_onesided_no_rmw = 0;
int gpfsmpio_aggmethod = 0;
#endif

#include <pthread.h>

//  #define onesidedtrace 1

/* Uncomment this to use fences for the one-sided communication.
 */
// #define ACTIVE_TARGET 1

/* This data structure holds the number of extents, the index into the flattened buffer and the remnant length
 * beyond the flattened buffer index corresponding to the base buffer offset for non-contiguous source data
 * for the range to be written coresponding to the round and target agg.
 */
typedef struct NonContigSourceBufOffset {
  int dataTypeExtent;
  int flatBufIndice;
  ADIO_Offset indiceOffset;
} NonContigSourceBufOffset;

static int ADIOI_OneSidedSetup(ADIO_File fd, int procs) {
    int ret = MPI_SUCCESS;

    ret = MPI_Win_create(fd->io_buf,fd->hints->cb_buffer_size,1,
	    MPI_INFO_NULL,fd->comm, &fd->io_buf_window);
    if (ret != MPI_SUCCESS) goto fn_exit;
    fd->io_buf_put_amounts = (int *) ADIOI_Malloc(procs*sizeof(int));
    ret =MPI_Win_create(fd->io_buf_put_amounts,procs*sizeof(int),sizeof(int),
	    MPI_INFO_NULL,fd->comm, &fd->io_buf_put_amounts_window);
fn_exit:
    return ret;
}

int ADIOI_OneSidedCleanup(ADIO_File fd)
{
    int ret = MPI_SUCCESS;
    if (fd->io_buf_window != MPI_WIN_NULL)
	ret = MPI_Win_free(&fd->io_buf_window);
    if (fd->io_buf_put_amounts_window != MPI_WIN_NULL)
	ret = MPI_Win_free(&fd->io_buf_put_amounts_window);
    if (fd->io_buf_put_amounts != NULL)
	ADIOI_Free(fd->io_buf_put_amounts);

    return ret;
}

void ADIOI_OneSidedWriteAggregation(ADIO_File fd,
    ADIO_Offset *offset_list,
    ADIO_Offset *len_list,
    int contig_access_count,
    const void *buf,
    MPI_Datatype datatype,
    int *error_code,
    ADIO_Offset *st_offsets,
    ADIO_Offset *end_offsets,
    ADIO_Offset *fd_start,
    ADIO_Offset* fd_end,
    int *hole_found)

{
    *error_code = MPI_SUCCESS; /* initialize to success */

#ifdef ROMIO_GPFS
    double startTimeBase,endTimeBase;
    startTimeBase = MPI_Wtime();
#endif

    int i,j; /* generic iterators */
    MPI_Status status;
    pthread_t io_thread;
    void *thread_ret;
    ADIOI_IO_ThreadFuncData io_thread_args;

    int nprocs,myrank;
    MPI_Comm_size(fd->comm, &nprocs);
    MPI_Comm_rank(fd->comm, &myrank);

    if (fd->io_buf_window == MPI_WIN_NULL ||
	    fd->io_buf_put_amounts_window == MPI_WIN_NULL)
    {
	ADIOI_OneSidedSetup(fd, nprocs);
    }

    /* This flag denotes whether the source datatype is contiguous, which is referenced throughout the algorithm
     * and defines how the source buffer offsets and data chunks are determined.  If the value is 1 (true - contiguous data)
     * things are profoundly simpler in that the source buffer offset for a given target offset simply linearly increases
     * by the chunk sizes being written.  If the value is 0 (non-contiguous) then these values are based on calculations
     * from the flattened source datatype.
     */
    int bufTypeIsContig;

    MPI_Aint bufTypeExtent;
    ADIOI_Flatlist_node *flatBuf=NULL;
    ADIOI_Datatype_iscontig(datatype, &bufTypeIsContig);

    /* maxNumContigOperations keeps track of how many different chunks we will need to send
     * for the purpose of pre-allocating the data structures to hold them.
     */
    int maxNumContigOperations = contig_access_count;

    if (!bufTypeIsContig) {
    /* Flatten the non-contiguous source datatype.
     */
      flatBuf = ADIOI_Flatten_and_find(datatype);
      MPI_Type_extent(datatype, &bufTypeExtent);
#ifdef onesidedtrace
      printf("flatBuf->count is %d bufTypeExtent is %d\n", flatBuf->count,bufTypeExtent);
      for (i=0;i<flatBuf->count;i++)
        printf("flatBuf->blocklens[%d] is %d flatBuf->indices[%d] is %ld\n",i,flatBuf->blocklens[i],i,flatBuf->indices[i]);
#endif
    }

#ifdef onesidedtrace
    printf(" ADIOI_OneSidedWriteAggregation bufTypeIsContig is %d contig_access_count is %d\n",bufTypeIsContig,contig_access_count);
#endif

    ADIO_Offset lastFileOffset = 0, firstFileOffset = -1;
    /* Get the total range being written.
     */
    for (j=0;j<nprocs;j++) {
      if (end_offsets[j] > st_offsets[j]) {
        /* Guard against ranks with empty data.
         */
        lastFileOffset = ADIOI_MAX(lastFileOffset,end_offsets[j]);
        if (firstFileOffset == -1)
          firstFileOffset = st_offsets[j];
        else
          firstFileOffset = ADIOI_MIN(firstFileOffset,st_offsets[j]);
      }
    }

    int myAggRank = -1; /* if I am an aggregor this is my index into fd->hints->ranklist */
    int iAmUsedAgg = 0; /* whether or not this rank is used as an aggregator. */

    int naggs = fd->hints->cb_nodes;
    int coll_bufsize = fd->hints->cb_buffer_size;
#ifdef ROMIO_GPFS
    if (gpfsmpio_pthreadio == 1) {
      /* split buffer in half for a kind of double buffering with the threads*/
      coll_bufsize = fd->hints->cb_buffer_size/2;
    }
#endif

    /* This logic defines values that are used later to determine what offsets define the portion
     * of the file domain the agg is writing this round.
     */
    int greatestFileDomainAggRank = -1,smallestFileDomainAggRank = -1;
    ADIO_Offset greatestFileDomainOffset = 0;
    ADIO_Offset smallestFileDomainOffset = lastFileOffset;
    for (j=0;j<naggs;j++) {
      if (fd_end[j] > greatestFileDomainOffset) {
        greatestFileDomainOffset = fd_end[j];
        greatestFileDomainAggRank = j;
      }
      if (fd_start[j] < smallestFileDomainOffset) {
        smallestFileDomainOffset = fd_start[j];
        smallestFileDomainAggRank = j;
      }
      if (fd->hints->ranklist[j] == myrank) {
        myAggRank = j;
        if (fd_end[j] > fd_start[j]) {
          iAmUsedAgg = 1;
        }
      }
    }

#ifdef onesidedtrace
    printf("contig_access_count is %d lastFileOffset is %ld firstFileOffset is %ld\n",contig_access_count,lastFileOffset,firstFileOffset);
    for (j=0;j<contig_access_count;j++) {
      printf("offset_list[%d]: %ld , len_list[%d]: %ld\n",j,offset_list[j],j,len_list[j]);
    }
#endif

    /* Determine how much data and to whom I need to send.  For source proc
     * targets, also determine the target file domain offsets locally to
     * reduce communication overhead.
     */
    int *targetAggsForMyData = (int *)ADIOI_Malloc(naggs * sizeof(int));
    ADIO_Offset *targetAggsForMyDataFDStart = (ADIO_Offset *)ADIOI_Malloc(naggs * sizeof(ADIO_Offset));
    ADIO_Offset *targetAggsForMyDataFDEnd = (ADIO_Offset *)ADIOI_Malloc(naggs * sizeof(ADIO_Offset));
    int numTargetAggs = 0;

    /* Compute number of rounds.
     */
    ADIO_Offset numberOfRounds = (ADIO_Offset)((((ADIO_Offset)(end_offsets[nprocs-1]-st_offsets[0]))/((ADIO_Offset)((ADIO_Offset)coll_bufsize*(ADIO_Offset)naggs)))) + 1;

    /* This data structure holds the beginning offset and len list index for the range to be written
     * coresponding to the round and target agg.  Initialize to -1 to denote being unset.
     */
    int **targetAggsForMyDataFirstOffLenIndex = (int **)ADIOI_Malloc(numberOfRounds * sizeof(int *));
    for (i=0;i<numberOfRounds;i++) {
      targetAggsForMyDataFirstOffLenIndex[i] = (int *)ADIOI_Malloc(naggs * sizeof(int));
      for (j=0;j<naggs;j++)
        targetAggsForMyDataFirstOffLenIndex[i][j] = -1;
    }

    /* This data structure holds the ending offset and len list index for the range to be written
     * coresponding to the round and target agg.
     */
    int **targetAggsForMyDataLastOffLenIndex = (int **)ADIOI_Malloc(numberOfRounds * sizeof(int *));
    for (i=0;i<numberOfRounds;i++)
      targetAggsForMyDataLastOffLenIndex[i] = (int *)ADIOI_Malloc(naggs * sizeof(int));

    /* This data structure holds the base buffer offset for contiguous source data for the range to be written
     * coresponding to the round and target agg.  Initialize to -1 to denote being unset.yeah
     */
    ADIO_Offset **baseSourceBufferOffset;

    if (bufTypeIsContig) {
      baseSourceBufferOffset= (ADIO_Offset **)ADIOI_Malloc(numberOfRounds * sizeof(ADIO_Offset *));
      for (i=0;i<numberOfRounds;i++) {
        baseSourceBufferOffset[i] = (ADIO_Offset *)ADIOI_Malloc(naggs * sizeof(ADIO_Offset));
        for (j=0;j<naggs;j++)
          baseSourceBufferOffset[i][j] = -1;
      }
    }
    ADIO_Offset currentSourceBufferOffset = 0;

    /* This data structure holds the number of extents, the index into the flattened buffer and the remnant length
     * beyond the flattened buffer indice corresponding to the base buffer offset for non-contiguous source data
     * for the range to be written coresponding to the round and target agg.
     */
    NonContigSourceBufOffset **baseNonContigSourceBufferOffset;
    if (!bufTypeIsContig) {
      baseNonContigSourceBufferOffset = (NonContigSourceBufOffset **) ADIOI_Malloc(numberOfRounds * sizeof(NonContigSourceBufOffset *));
      for (i=0;i<numberOfRounds;i++) {
        baseNonContigSourceBufferOffset[i] = (NonContigSourceBufOffset *)ADIOI_Malloc(naggs * sizeof(NonContigSourceBufOffset));
        /* initialize flatBufIndice to -1 to indicate that it is unset.
         */
        for (j=0;j<naggs;j++)
          baseNonContigSourceBufferOffset[i][j].flatBufIndice = -1;
      }
    }

    int currentDataTypeExtent = 0;
    int currentFlatBufIndice=0;
    ADIO_Offset currentIndiceOffset = 0;

#ifdef onesidedtrace
   printf("NumberOfRounds is %d\n",numberOfRounds);
   for (i=0;i<naggs;i++)
     printf("fd->hints->ranklist[%d]is %d fd_start is %ld fd_end is %ld\n",i,fd->hints->ranklist[i],fd_start[i],fd_end[i]);
   for (j=0;j<contig_access_count;j++)
     printf("offset_list[%d] is %ld len_list is %ld\n",j,offset_list[j],len_list[j]);
#endif

    int currentAggRankListIndex = 0;
    int maxNumNonContigSourceChunks = 0;

    /* This denotes the coll_bufsize boundaries within the source buffer for writing for the same round.
     */
    ADIO_Offset intraRoundCollBufsizeOffset = 0;

    /* This data structure tracks what target aggs need to be written to on what rounds.
     */
    int *targetAggsForMyDataCurrentRoundIter = (int *)ADIOI_Malloc(naggs * sizeof(int));
    for (i=0;i<naggs;i++)
      targetAggsForMyDataCurrentRoundIter[i] = 0;

    /* This is the first of the two main loops in this algorithm.  The purpose of this loop is essentially to populate
     * the data structures defined above for what source data blocks needs to go where (target agg) and when (round iter).
     */
    int blockIter;
    for (blockIter=0;blockIter<contig_access_count;blockIter++) {

      /* Determine the starting source buffer offset for this block - for iter 0 skip it since that value is 0.
       */
      if (blockIter>0) {
        if (bufTypeIsContig)
          currentSourceBufferOffset += len_list[blockIter-1];
        else {
          /* Non-contiguous source datatype, count up the extents and indices to this point
           * in the blocks for use in computing the source buffer offset.
           */
          ADIO_Offset sourceBlockTotal = 0;
          int lastIndiceUsed = currentFlatBufIndice;
          int numNonContigSourceChunks = 0;
#ifdef onesidedtrace
          printf("blockIter %d len_list[blockIter-1] is %d currentIndiceOffset is %ld currentFlatBufIndice is %d\n",blockIter,len_list[blockIter-1],currentIndiceOffset,currentFlatBufIndice);
#endif
          while (sourceBlockTotal < len_list[blockIter-1]) {
            numNonContigSourceChunks++;
            sourceBlockTotal += (flatBuf->blocklens[currentFlatBufIndice] - currentIndiceOffset);
            lastIndiceUsed = currentFlatBufIndice;
            currentFlatBufIndice++;
            if (currentFlatBufIndice == flatBuf->count) {
              currentFlatBufIndice = 0;
              currentDataTypeExtent++;
            }
            currentIndiceOffset = 0;
          }
          if (sourceBlockTotal > len_list[blockIter-1]) {
            currentFlatBufIndice--;
            if (currentFlatBufIndice < 0 ) {
              currentDataTypeExtent--;
              currentFlatBufIndice = flatBuf->count-1;
            }
            currentIndiceOffset =  len_list[blockIter-1] - (sourceBlockTotal - flatBuf->blocklens[lastIndiceUsed]);
            ADIOI_Assert((currentIndiceOffset >= 0) && (currentIndiceOffset < flatBuf->blocklens[currentFlatBufIndice]));
          }
          else
            currentIndiceOffset = 0;
          maxNumContigOperations += numNonContigSourceChunks;
          if (numNonContigSourceChunks > maxNumNonContigSourceChunks)
            maxNumNonContigSourceChunks = numNonContigSourceChunks;
#ifdef onesidedtrace
          printf("blockiter %d currentFlatBufIndice is now %d currentDataTypeExtent is now %d currentIndiceOffset is now %ld maxNumContigOperations is now %d\n",blockIter,currentFlatBufIndice,currentDataTypeExtent,currentIndiceOffset,maxNumContigOperations);
#endif
        } // !bufTypeIsContig
      } // blockIter > 0

      /* For the first iteration we need to include these maxNumContigOperations and maxNumNonContigSourceChunks
       * for non-contig case even though we did not need to compute the starting offset.
       */
      if ((blockIter == 0) && (!bufTypeIsContig)) {
        ADIO_Offset sourceBlockTotal = 0;
        int tmpCurrentFlatBufIndice = currentFlatBufIndice;
        while (sourceBlockTotal < len_list[0]) {
          maxNumContigOperations++;
          maxNumNonContigSourceChunks++;
          sourceBlockTotal += flatBuf->blocklens[tmpCurrentFlatBufIndice];
          tmpCurrentFlatBufIndice++;
          if (tmpCurrentFlatBufIndice == flatBuf->count) {
            tmpCurrentFlatBufIndice = 0;
          }
        }
      }

      ADIO_Offset blockStart = offset_list[blockIter], blockEnd = offset_list[blockIter]+len_list[blockIter]-(ADIO_Offset)1;

      /* Find the starting target agg for this block - normally it will be the current agg so guard the expensive
       * while loop with a cheap if-check which for large numbers of small blocks will usually be false.
       */
      if (!((blockStart >= fd_start[currentAggRankListIndex]) && (blockStart <= fd_end[currentAggRankListIndex]))) {
        while (!((blockStart >= fd_start[currentAggRankListIndex]) && (blockStart <= fd_end[currentAggRankListIndex])))
          currentAggRankListIndex++;
      };

#ifdef onesidedtrace
      printf("currentAggRankListIndex is %d blockStart %ld blockEnd %ld fd_start[currentAggRankListIndex] %ld fd_end[currentAggRankListIndex] %ld\n",currentAggRankListIndex,blockStart,blockEnd,fd_start[currentAggRankListIndex],fd_end[currentAggRankListIndex]);
#endif

      /* Determine if this is a new target agg.
       */
      if (blockIter>0) {
        if ((offset_list[blockIter-1]+len_list[blockIter-1]-(ADIO_Offset)1) < fd_start[currentAggRankListIndex])
          numTargetAggs++;
      }

       /* Determine which round to start writing.
        */
      if ((blockStart - fd_start[currentAggRankListIndex]) >= coll_bufsize) {
        ADIO_Offset currentRoundBlockStart = fd_start[currentAggRankListIndex];
        int startingRound = 0;
        while (blockStart > (currentRoundBlockStart + coll_bufsize - (ADIO_Offset)1)) {
          currentRoundBlockStart+=coll_bufsize;
          startingRound++;
        }
        targetAggsForMyDataCurrentRoundIter[numTargetAggs] = startingRound;
      }

      /* Initialize the data structures if this is the first offset in the round/target agg.
       */
      if (targetAggsForMyDataFirstOffLenIndex[targetAggsForMyDataCurrentRoundIter[numTargetAggs]][numTargetAggs] == -1) {
        targetAggsForMyData[numTargetAggs] = fd->hints->ranklist[currentAggRankListIndex];
        targetAggsForMyDataFDStart[numTargetAggs] = fd_start[currentAggRankListIndex];
        /* Round up file domain to the first actual offset used if this is the first file domain.
         */
        if (currentAggRankListIndex == smallestFileDomainAggRank) {
          if (targetAggsForMyDataFDStart[numTargetAggs] < firstFileOffset)
            targetAggsForMyDataFDStart[numTargetAggs] = firstFileOffset;
        }
        targetAggsForMyDataFDEnd[numTargetAggs] = fd_end[currentAggRankListIndex];
        /* Round down file domain to the last actual offset used if this is the last file domain.
         */
        if (currentAggRankListIndex == greatestFileDomainAggRank) {
          if (targetAggsForMyDataFDEnd[numTargetAggs] > lastFileOffset)
            targetAggsForMyDataFDEnd[numTargetAggs] = lastFileOffset;
        }
        targetAggsForMyDataFirstOffLenIndex[targetAggsForMyDataCurrentRoundIter[numTargetAggs]][numTargetAggs] = blockIter;
        if (bufTypeIsContig)
          baseSourceBufferOffset[targetAggsForMyDataCurrentRoundIter[numTargetAggs]][numTargetAggs] = currentSourceBufferOffset;
        else {
          baseNonContigSourceBufferOffset[targetAggsForMyDataCurrentRoundIter[numTargetAggs]][numTargetAggs].flatBufIndice = currentFlatBufIndice;
          baseNonContigSourceBufferOffset[targetAggsForMyDataCurrentRoundIter[numTargetAggs]][numTargetAggs].dataTypeExtent = currentDataTypeExtent;
          baseNonContigSourceBufferOffset[targetAggsForMyDataCurrentRoundIter[numTargetAggs]][numTargetAggs].indiceOffset = currentIndiceOffset;
        }

        intraRoundCollBufsizeOffset = fd_start[currentAggRankListIndex] + ((targetAggsForMyDataCurrentRoundIter[numTargetAggs]+1) * coll_bufsize);

#ifdef onesidedtrace
        printf("Initial settings numTargetAggs %d offset_list[%d] with value %ld past fd border %ld with len %ld currentSourceBufferOffset set to %ld intraRoundCollBufsizeOffset set to %ld\n",numTargetAggs,blockIter,offset_list[blockIter],fd_start[currentAggRankListIndex],len_list[blockIter],currentSourceBufferOffset,intraRoundCollBufsizeOffset);
#endif
      }

      /* Replace the last offset block iter with this one.
       */
      targetAggsForMyDataLastOffLenIndex[targetAggsForMyDataCurrentRoundIter[numTargetAggs]][numTargetAggs] = blockIter;

      /* If this blocks extends into the next file domain handle this situation.
       */
      if (blockEnd > fd_end[currentAggRankListIndex]) {
#ifdef onesidedtrace
      printf("large block, blockEnd %ld >= fd_end[currentAggRankListIndex] %ld\n",blockEnd,fd_end[currentAggRankListIndex]);
#endif
        while (blockEnd >= fd_end[currentAggRankListIndex]) {
          ADIO_Offset thisAggBlockEnd = fd_end[currentAggRankListIndex];
          if (thisAggBlockEnd >= intraRoundCollBufsizeOffset) {
            while (thisAggBlockEnd >= intraRoundCollBufsizeOffset) {
              targetAggsForMyDataCurrentRoundIter[numTargetAggs]++;
              intraRoundCollBufsizeOffset += coll_bufsize;
              targetAggsForMyDataFirstOffLenIndex[targetAggsForMyDataCurrentRoundIter[numTargetAggs]][numTargetAggs] = blockIter;
              if (bufTypeIsContig)
                baseSourceBufferOffset[targetAggsForMyDataCurrentRoundIter[numTargetAggs]][numTargetAggs] = currentSourceBufferOffset;
              else {
                baseNonContigSourceBufferOffset[targetAggsForMyDataCurrentRoundIter[numTargetAggs]][numTargetAggs].flatBufIndice = currentFlatBufIndice;
                baseNonContigSourceBufferOffset[targetAggsForMyDataCurrentRoundIter[numTargetAggs]][numTargetAggs].dataTypeExtent = currentDataTypeExtent;
                baseNonContigSourceBufferOffset[targetAggsForMyDataCurrentRoundIter[numTargetAggs]][numTargetAggs].indiceOffset = currentIndiceOffset;
              }

              targetAggsForMyDataLastOffLenIndex[targetAggsForMyDataCurrentRoundIter[numTargetAggs]][numTargetAggs] = blockIter;
#ifdef onesidedtrace
              printf("targetAggsForMyDataCurrentRoundI%d] is now %d intraRoundCollBufsizeOffset is now %ld\n",numTargetAggs,targetAggsForMyDataCurrentRoundIter[numTargetAggs],intraRoundCollBufsizeOffset);
#endif
            } // while (thisAggBlockEnd >= intraRoundCollBufsizeOffset)
          } // if (thisAggBlockEnd >= intraRoundCollBufsizeOffset)

          currentAggRankListIndex++;

          /* Skip over unused aggs.
           */
          if (fd_start[currentAggRankListIndex] > fd_end[currentAggRankListIndex]) {
            while (fd_start[currentAggRankListIndex] > fd_end[currentAggRankListIndex])
              currentAggRankListIndex++;

          } // (fd_start[currentAggRankListIndex] > fd_end[currentAggRankListIndex])

          /* Start new target agg.
           */
          if (blockEnd >= fd_start[currentAggRankListIndex]) {
            numTargetAggs++;
            targetAggsForMyData[numTargetAggs] = fd->hints->ranklist[currentAggRankListIndex];
            targetAggsForMyDataFDStart[numTargetAggs] = fd_start[currentAggRankListIndex];
            /* Round up file domain to the first actual offset used if this is the first file domain.
             */
            if (currentAggRankListIndex == smallestFileDomainAggRank) {
              if (targetAggsForMyDataFDStart[numTargetAggs] < firstFileOffset)
                targetAggsForMyDataFDStart[numTargetAggs] = firstFileOffset;
            }
            targetAggsForMyDataFDEnd[numTargetAggs] = fd_end[currentAggRankListIndex];
            /* Round down file domain to the last actual offset used if this is the last file domain.
             */
            if (currentAggRankListIndex == greatestFileDomainAggRank) {
              if (targetAggsForMyDataFDEnd[numTargetAggs] > lastFileOffset)
                targetAggsForMyDataFDEnd[numTargetAggs] = lastFileOffset;
            }
            targetAggsForMyDataFirstOffLenIndex[targetAggsForMyDataCurrentRoundIter[numTargetAggs]][numTargetAggs] = blockIter;
            if (bufTypeIsContig)
              baseSourceBufferOffset[targetAggsForMyDataCurrentRoundIter[numTargetAggs]][numTargetAggs] = currentSourceBufferOffset;
            else {
              baseNonContigSourceBufferOffset[targetAggsForMyDataCurrentRoundIter[numTargetAggs]][numTargetAggs].flatBufIndice = currentFlatBufIndice;
              baseNonContigSourceBufferOffset[targetAggsForMyDataCurrentRoundIter[numTargetAggs]][numTargetAggs].dataTypeExtent = currentDataTypeExtent;
              baseNonContigSourceBufferOffset[targetAggsForMyDataCurrentRoundIter[numTargetAggs]][numTargetAggs].indiceOffset = currentIndiceOffset;
            }
#ifdef onesidedtrace
            printf("large block init settings numTargetAggs %d offset_list[%d] with value %ld past fd border %ld with len %ld\n",numTargetAggs,i,offset_list[blockIter],fd_start[currentAggRankListIndex],len_list[blockIter]);
#endif
            intraRoundCollBufsizeOffset = fd_start[currentAggRankListIndex] + coll_bufsize;
            targetAggsForMyDataLastOffLenIndex[targetAggsForMyDataCurrentRoundIter[numTargetAggs]][numTargetAggs] = blockIter;
          } // if (blockEnd >= fd_start[currentAggRankListIndex])
        } // while (blockEnd >= fd_end[currentAggRankListIndex])
      } // if (blockEnd > fd_end[currentAggRankListIndex])

      /* Else if we are still in the same file domain / target agg but have gone past the coll_bufsize and need
       * to advance to the next round handle this situation.
       */
      else if (blockEnd >= intraRoundCollBufsizeOffset) {
        ADIO_Offset currentBlockEnd = blockEnd;
        while (currentBlockEnd >= intraRoundCollBufsizeOffset) {
          targetAggsForMyDataCurrentRoundIter[numTargetAggs]++;
          intraRoundCollBufsizeOffset += coll_bufsize;
          targetAggsForMyDataFirstOffLenIndex[targetAggsForMyDataCurrentRoundIter[numTargetAggs]][numTargetAggs] = blockIter;
          if (bufTypeIsContig)
            baseSourceBufferOffset[targetAggsForMyDataCurrentRoundIter[numTargetAggs]][numTargetAggs] = currentSourceBufferOffset;
          else {
            baseNonContigSourceBufferOffset[targetAggsForMyDataCurrentRoundIter[numTargetAggs]][numTargetAggs].flatBufIndice = currentFlatBufIndice;
            baseNonContigSourceBufferOffset[targetAggsForMyDataCurrentRoundIter[numTargetAggs]][numTargetAggs].dataTypeExtent = currentDataTypeExtent;
            baseNonContigSourceBufferOffset[targetAggsForMyDataCurrentRoundIter[numTargetAggs]][numTargetAggs].indiceOffset = currentIndiceOffset;
          }
          targetAggsForMyDataLastOffLenIndex[targetAggsForMyDataCurrentRoundIter[numTargetAggs]][numTargetAggs] = blockIter;
#ifdef onesidedtrace
        printf("smaller than fd currentBlockEnd is now %ld intraRoundCollBufsizeOffset is now %ld targetAggsForMyDataCurrentRoundIter[%d] is now %d\n",currentBlockEnd, intraRoundCollBufsizeOffset, numTargetAggs,targetAggsForMyDataCurrentRoundIter[numTargetAggs]);
#endif
        } // while (currentBlockEnd >= intraRoundCollBufsizeOffset)
      } // else if (blockEnd >= intraRoundCollBufsizeOffset)

      /* Need to advance numTargetAggs if this is the last target offset to
       * include this one.
       */
      if (blockIter == (contig_access_count-1))
        numTargetAggs++;
    }

#ifdef onesidedtrace
    printf("numTargetAggs is %d\n",numTargetAggs);
    for (i=0;i<numTargetAggs;i++) {
      for (j=0;j<=targetAggsForMyDataCurrentRoundIter[i];j++)
        printf("targetAggsForMyData[%d] is %d targetAggsForMyDataFDStart[%d] is %ld targetAggsForMyDataFDEnd is %ld targetAggsForMyDataFirstOffLenIndex is %d with value %ld targetAggsForMyDataLastOffLenIndex is %d with value %ld\n",i,targetAggsForMyData[i],i,targetAggsForMyDataFDStart[i],targetAggsForMyDataFDEnd[i],targetAggsForMyDataFirstOffLenIndex[j][i],offset_list[targetAggsForMyDataFirstOffLenIndex[j][i]],targetAggsForMyDataLastOffLenIndex[j][i],offset_list[targetAggsForMyDataLastOffLenIndex[j][i]]);

    }
#endif

    ADIOI_Free(targetAggsForMyDataCurrentRoundIter);

    int currentWriteBuf = 0;
    int useIOBuffer = 0;
#ifdef ROMIO_GPFS
    if (gpfsmpio_pthreadio && (numberOfRounds>1)) {
    useIOBuffer = 1;
    io_thread = pthread_self();
    }
#endif

    /* use the write buffer allocated in the file_open */
    char *write_buf0 = fd->io_buf;
    char *write_buf1 = fd->io_buf + coll_bufsize;

    /* start off pointing to the first buffer. If we use the 2nd buffer (threaded
     * case) we'll swap later */
    char *write_buf = write_buf0;
    MPI_Win write_buf_window = fd->io_buf_window;

    int *write_buf_put_amounts = fd->io_buf_put_amounts;
    if(!gpfsmpio_onesided_no_rmw) {
      *hole_found = 0;
      for (i=0;i<nprocs;i++)
        write_buf_put_amounts[i] = 0;
    }
#ifdef ACTIVE_TARGET
    MPI_Win_fence(0, write_buf_window);
    if (!gpfsmpio_onesided_no_rmw)
      MPI_Win_fence(0, fd->io_buf_put_amounts_window);
#endif

    ADIO_Offset currentRoundFDStart = 0;
    ADIO_Offset currentRoundFDEnd = 0;

    if (iAmUsedAgg) {
      currentRoundFDStart = fd_start[myAggRank];
      if (myAggRank == smallestFileDomainAggRank) {
        if (currentRoundFDStart < firstFileOffset)
          currentRoundFDStart = firstFileOffset;
      }
      else if (myAggRank == greatestFileDomainAggRank) {
        if (currentRoundFDEnd > lastFileOffset)
          currentRoundFDEnd = lastFileOffset;
      }
    }

#ifdef ROMIO_GPFS
    endTimeBase = MPI_Wtime();
    gpfsmpio_prof_cw[GPFSMPIO_CIO_T_DEXCH_SETUP] += (endTimeBase-startTimeBase);
    startTimeBase = MPI_Wtime();
#endif

    ADIO_Offset currentBaseSourceBufferOffset = 0;

    /* These data structures are used to track the offset/len pairs for non-contiguous source buffers that are
     * to be used for each block of data in the offset list.  Allocate them once with the maximum size and then 
     * reuse the space throughout the algoritm below.
     */
    ADIO_Offset *nonContigSourceOffsets;
    int *nonContigSourceLens;
    if (!bufTypeIsContig) {
      nonContigSourceOffsets = (ADIO_Offset *)ADIOI_Malloc((maxNumNonContigSourceChunks+2) * sizeof(ADIO_Offset));
      nonContigSourceLens = (int *)ADIOI_Malloc((maxNumNonContigSourceChunks+2) * sizeof(int));
    }
    /* This is the second main loop of the algorithm, actually nested loop of target aggs within rounds.  There are 2 flavors of this.
     * For gpfsmpio_aggmethod of 1 each nested iteration for the target agg does an mpi_put on a contiguous chunk using a primative datatype
     * determined using the data structures from the first main loop.  For gpfsmpio_aggmethod of 2 each nested iteration for the target agg
     * builds up data to use in created a derived data type for 1 mpi_put that is done for the target agg for each round.
     */
    int roundIter;
    for (roundIter=0;roundIter<numberOfRounds;roundIter++) {

    int aggIter;
    for (aggIter=0;aggIter<numTargetAggs;aggIter++) {

    int numBytesPutThisAggRound = 0;
    /* If we have data for the round/agg process it.
     */
    if ((bufTypeIsContig && (baseSourceBufferOffset[roundIter][aggIter] != -1)) || (!bufTypeIsContig && (baseNonContigSourceBufferOffset[roundIter][aggIter].flatBufIndice != -1))) {

      ADIO_Offset currentRoundFDStartForMyTargetAgg = (ADIO_Offset)((ADIO_Offset)targetAggsForMyDataFDStart[aggIter] + (ADIO_Offset)((ADIO_Offset)roundIter*(ADIO_Offset)coll_bufsize));
      ADIO_Offset currentRoundFDEndForMyTargetAgg = (ADIO_Offset)((ADIO_Offset)targetAggsForMyDataFDStart[aggIter] + (ADIO_Offset)((ADIO_Offset)(roundIter+1)*(ADIO_Offset)coll_bufsize) - (ADIO_Offset)1);

      int targetAggContigAccessCount = 0;

      /* These data structures are used for the derived datatype mpi_put
       * in the gpfsmpio_aggmethod of 2 case.
       */
      int *targetAggBlockLengths;
      MPI_Aint *targetAggDisplacements, *sourceBufferDisplacements;
      MPI_Datatype *targetAggDataTypes;

      int allocatedDerivedTypeArrays = 0;
#ifdef onesidedtrace
      printf("roundIter %d processing targetAggsForMyData %d \n",roundIter,targetAggsForMyData[aggIter]);
#endif

      ADIO_Offset sourceBufferOffset = 0;

      /* Process the range of offsets for this target agg.
       */
      int offsetIter;
      int startingOffLenIndex = targetAggsForMyDataFirstOffLenIndex[roundIter][aggIter], endingOffLenIndex = targetAggsForMyDataLastOffLenIndex[roundIter][aggIter];
      for (offsetIter=startingOffLenIndex;offsetIter<=endingOffLenIndex;offsetIter++) {
        if (currentRoundFDEndForMyTargetAgg > targetAggsForMyDataFDEnd[aggIter])
            currentRoundFDEndForMyTargetAgg = targetAggsForMyDataFDEnd[aggIter];

        ADIO_Offset offsetStart = offset_list[offsetIter], offsetEnd = (offset_list[offsetIter]+len_list[offsetIter]-(ADIO_Offset)1);

        /* Get the base source buffer offset for this iterataion of the target agg in the round.
         * For the first one just get the predetermined base value, for subsequent ones keep track
         * of the value for each iteration and increment it.
         */
        if (offsetIter == startingOffLenIndex) {
          if (bufTypeIsContig)
            currentBaseSourceBufferOffset = baseSourceBufferOffset[roundIter][aggIter];
          else {
            currentIndiceOffset = baseNonContigSourceBufferOffset[roundIter][aggIter].indiceOffset;
            currentDataTypeExtent = baseNonContigSourceBufferOffset[roundIter][aggIter].dataTypeExtent;
            currentFlatBufIndice = baseNonContigSourceBufferOffset[roundIter][aggIter].flatBufIndice;
          }
        }
        else {
          if (bufTypeIsContig)
            currentBaseSourceBufferOffset += len_list[offsetIter-1];
          else {

          /* For non-contiguous source datatype advance the flattened buffer machinery to this offset.
           * Note that currentDataTypeExtent, currentFlatBufIndice and currentIndiceOffset are used and
           * advanced across the offsetIters.
           */
          ADIO_Offset sourceBlockTotal = 0;
          int lastIndiceUsed = currentFlatBufIndice;
          while (sourceBlockTotal < len_list[offsetIter-1]) {
            sourceBlockTotal += (flatBuf->blocklens[currentFlatBufIndice] - currentIndiceOffset);
            lastIndiceUsed = currentFlatBufIndice;
            currentFlatBufIndice++;
            if (currentFlatBufIndice == flatBuf->count) {
              currentFlatBufIndice = 0;
              currentDataTypeExtent++;
            }
            currentIndiceOffset = 0;
          } // while
          if (sourceBlockTotal > len_list[offsetIter-1]) {
            currentFlatBufIndice--;
            if (currentFlatBufIndice < 0 ) {
              currentDataTypeExtent--;
              currentFlatBufIndice = flatBuf->count-1;
            }
            currentIndiceOffset =  len_list[offsetIter-1] - (sourceBlockTotal - flatBuf->blocklens[lastIndiceUsed]);
            ADIOI_Assert((currentIndiceOffset >= 0) && (currentIndiceOffset < flatBuf->blocklens[currentFlatBufIndice]));
          }
          else
            currentIndiceOffset = 0;
#ifdef onesidedtrace
          printf("offsetIter %d contig_access_count target agg %d currentFlatBufIndice is now %d currentDataTypeExtent is now %d currentIndiceOffset is now %ld\n",offsetIter,aggIter,currentFlatBufIndice,currentDataTypeExtent,currentIndiceOffset);
#endif
          }
        }

        /* For source buffer contiguous here is the offset we will use for this iteration.
         */
        if (bufTypeIsContig)
          sourceBufferOffset = currentBaseSourceBufferOffset;

#ifdef onesidedtrace
        printf("roundIter %d target iter %d targetAggsForMyData is %d offset_list[%d] is %ld len_list[%d] is %ld targetAggsForMyDataFDStart is %ld targetAggsForMyDataFDEnd is %ld currentRoundFDStartForMyTargetAgg is %ld currentRoundFDEndForMyTargetAgg is %ld targetAggsForMyDataFirstOffLenIndex is %ld baseSourceBufferOffset is %ld\n",
            roundIter,aggIter,targetAggsForMyData[aggIter],offsetIter,offset_list[offsetIter],offsetIter,len_list[offsetIter],
            targetAggsForMyDataFDStart[aggIter],targetAggsForMyDataFDEnd[aggIter],
            currentRoundFDStartForMyTargetAgg,currentRoundFDEndForMyTargetAgg, targetAggsForMyDataFirstOffLenIndex[roundIter][aggIter], baseSourceBufferOffset[roundIter][aggIter]);
#endif

        /* Determine the amount of data and exact source buffer offsets to use.
         */
        int bufferAmountToSend = 0;

        if ((offsetStart >= currentRoundFDStartForMyTargetAgg) && (offsetStart <= currentRoundFDEndForMyTargetAgg)) {
            if (offsetEnd > currentRoundFDEndForMyTargetAgg)
            bufferAmountToSend = (currentRoundFDEndForMyTargetAgg - offsetStart) +1;
            else
            bufferAmountToSend = (offsetEnd - offsetStart) +1;
        }
        else if ((offsetEnd >= currentRoundFDStartForMyTargetAgg) && (offsetEnd <= currentRoundFDEndForMyTargetAgg)) {
            if (offsetEnd > currentRoundFDEndForMyTargetAgg)
            bufferAmountToSend = (currentRoundFDEndForMyTargetAgg - currentRoundFDStartForMyTargetAgg) +1;
            else
            bufferAmountToSend = (offsetEnd - currentRoundFDStartForMyTargetAgg) +1;
            if (offsetStart < currentRoundFDStartForMyTargetAgg) {
              if (bufTypeIsContig)
                sourceBufferOffset += (currentRoundFDStartForMyTargetAgg-offsetStart);
              offsetStart = currentRoundFDStartForMyTargetAgg;
            }
        }
        else if ((offsetStart <= currentRoundFDStartForMyTargetAgg) && (offsetEnd >= currentRoundFDEndForMyTargetAgg)) {
            bufferAmountToSend = (currentRoundFDEndForMyTargetAgg - currentRoundFDStartForMyTargetAgg) +1;
            if (bufTypeIsContig)
              sourceBufferOffset += (currentRoundFDStartForMyTargetAgg-offsetStart);
            offsetStart = currentRoundFDStartForMyTargetAgg;
        }

        numBytesPutThisAggRound += bufferAmountToSend;
#ifdef onesidedtrace
        printf("bufferAmountToSend is %d\n",bufferAmountToSend);
#endif
        if (bufferAmountToSend > 0) { /* we have data to send this round */
          if (gpfsmpio_aggmethod == 2) {
            /* Only allocate these arrays if we are using method 2 and only do it once for this round/target agg.
             */
            if (!allocatedDerivedTypeArrays) {
              targetAggBlockLengths = (int *)ADIOI_Malloc(maxNumContigOperations * sizeof(int));
              targetAggDisplacements = (MPI_Aint *)ADIOI_Malloc(maxNumContigOperations * sizeof(MPI_Aint));
              sourceBufferDisplacements = (MPI_Aint *)ADIOI_Malloc(maxNumContigOperations * sizeof(MPI_Aint));
              targetAggDataTypes = (MPI_Datatype *)ADIOI_Malloc(maxNumContigOperations * sizeof(MPI_Datatype));
              allocatedDerivedTypeArrays = 1;
            }
          }
          /* For the non-contiguous source datatype we need to determine the number,size and offsets of the chunks
           * from the source buffer to be sent to this contiguous chunk defined by the round/agg/iter in the target.
           */
          int numNonContigSourceChunks = 0;
          ADIO_Offset baseDatatypeInstanceOffset = 0;

          if (!bufTypeIsContig) {

            currentSourceBufferOffset = (ADIO_Offset)((ADIO_Offset)currentDataTypeExtent * (ADIO_Offset)bufTypeExtent) + flatBuf->indices[currentFlatBufIndice] + currentIndiceOffset;
#ifdef onesidedtrace
            printf("!bufTypeIsContig currentSourceBufferOffset set to %ld for roundIter %d target %d currentDataTypeExtent %d flatBuf->indices[currentFlatBufIndice] %ld currentIndiceOffset %ld currentFlatBufIndice %d\n",currentSourceBufferOffset,roundIter,aggIter,currentDataTypeExtent,flatBuf->indices[currentFlatBufIndice],currentIndiceOffset,currentFlatBufIndice);
#endif

            /* Use a tmp variable for the currentFlatBufIndice and currentIndiceOffset as they are used across the offsetIters
             * to compute the starting point for this iteration and will be modified now to compute the data chunks for
             * this iteration.
             */
            int tmpFlatBufIndice = currentFlatBufIndice;
            ADIO_Offset tmpIndiceOffset = currentIndiceOffset;

            /* now populate the nonContigSourceOffsets and nonContigSourceLens arrays for use in the one-sided operations.
             */
            int ncArrayIndex = 0;
            int remainingBytesToLoadedIntoNCArrays = bufferAmountToSend;
            int datatypeInstances = currentDataTypeExtent;
            while (remainingBytesToLoadedIntoNCArrays > 0) {
              ADIOI_Assert(ncArrayIndex < (maxNumNonContigSourceChunks+2));
              nonContigSourceOffsets[ncArrayIndex] = currentSourceBufferOffset;

              if ((flatBuf->blocklens[tmpFlatBufIndice] - tmpIndiceOffset) >= remainingBytesToLoadedIntoNCArrays) {
                nonContigSourceLens[ncArrayIndex] = remainingBytesToLoadedIntoNCArrays;
                remainingBytesToLoadedIntoNCArrays = 0;
              }
              else {
                nonContigSourceLens[ncArrayIndex] = (int)(flatBuf->blocklens[tmpFlatBufIndice] - tmpIndiceOffset);
                remainingBytesToLoadedIntoNCArrays -= (flatBuf->blocklens[tmpFlatBufIndice] - tmpIndiceOffset);
                tmpIndiceOffset = 0;
                tmpFlatBufIndice++;
                if (tmpFlatBufIndice == flatBuf->count) {
                  tmpFlatBufIndice = 0;
                  datatypeInstances++;
                  baseDatatypeInstanceOffset = datatypeInstances * bufTypeExtent;
                }
                currentSourceBufferOffset = baseDatatypeInstanceOffset + flatBuf->indices[tmpFlatBufIndice];
              }
#ifdef onesidedtrace
              printf("currentSourceBufferOffset set to %ld off of baseDatatypeInstanceOffset of %ld + tmpFlatBufIndice %d with value %ld\n ncArrayIndex is %d nonContigSourceOffsets[ncArrayIndex] is %ld nonContigSourceLens[ncArrayIndex] is %ld\n",currentSourceBufferOffset,baseDatatypeInstanceOffset, tmpFlatBufIndice, flatBuf->indices[tmpFlatBufIndice],ncArrayIndex,nonContigSourceOffsets[ncArrayIndex],nonContigSourceLens[ncArrayIndex]);
#endif
              ncArrayIndex++;
              numNonContigSourceChunks++;
            } // while

          } // !bufTypeIsContig

          /* Determine the offset into the target window.
           */
          MPI_Aint targetDisplacementToUseThisRound = (MPI_Aint) ((ADIO_Offset)offsetStart - currentRoundFDStartForMyTargetAgg);

          /* If using the thread writer select the appropriate side of the split window.
           */
          if (useIOBuffer && (write_buf == write_buf1)) {
            targetDisplacementToUseThisRound += coll_bufsize;
          }

          /* For gpfsmpio_aggmethod of 1 do the mpi_put using the primitive MPI_BYTE type on each contiguous chunk of source data.
           */
          if (gpfsmpio_aggmethod == 1) {
#ifndef ACTIVE_TARGET
            MPI_Win_lock(MPI_LOCK_SHARED, targetAggsForMyData[aggIter], 0, write_buf_window);
#endif
            if (bufTypeIsContig) {
              MPI_Put(((char*)buf) + sourceBufferOffset,bufferAmountToSend, MPI_BYTE,targetAggsForMyData[aggIter],targetDisplacementToUseThisRound, bufferAmountToSend,MPI_BYTE,write_buf_window);
            }
            else {
              for (i=0;i<numNonContigSourceChunks;i++) {
                MPI_Put(((char*)buf) + nonContigSourceOffsets[i],nonContigSourceLens[i], MPI_BYTE,targetAggsForMyData[aggIter],targetDisplacementToUseThisRound, nonContigSourceLens[i],MPI_BYTE,write_buf_window);

#ifdef onesidedtrace
                printf("mpi_put[%d] nonContigSourceOffsets is %d of nonContigSourceLens %d to target disp %d first int of data: %d\n ",i,nonContigSourceOffsets[i],nonContigSourceLens[i],targetDisplacementToUseThisRound, ((int*)(((char*)buf) + nonContigSourceOffsets[i]))[0]);
#endif
                targetDisplacementToUseThisRound += nonContigSourceLens[i];
              }
            }
#ifndef ACTIVE_TARGET
            MPI_Win_unlock(targetAggsForMyData[aggIter], write_buf_window);
#endif
          }

          /* For gpfsmpio_aggmethod of 2 populate the data structures for this round/agg for this offset iter
           * to be used subsequently when building the derived type for 1 mpi_put for all the data for this
           * round/agg.
           */
          else if (gpfsmpio_aggmethod == 2) {

            if (bufTypeIsContig) {
              targetAggBlockLengths[targetAggContigAccessCount]= bufferAmountToSend;
              targetAggDataTypes[targetAggContigAccessCount] = MPI_BYTE;
              targetAggDisplacements[targetAggContigAccessCount] = targetDisplacementToUseThisRound;
              sourceBufferDisplacements[targetAggContigAccessCount] = (MPI_Aint)sourceBufferOffset;
              targetAggContigAccessCount++;
            }
            else {
              for (i=0;i<numNonContigSourceChunks;i++) {
              if (nonContigSourceLens[i] > 0) {
                targetAggBlockLengths[targetAggContigAccessCount]= nonContigSourceLens[i];
                targetAggDataTypes[targetAggContigAccessCount] = MPI_BYTE;
                targetAggDisplacements[targetAggContigAccessCount] = targetDisplacementToUseThisRound;
                sourceBufferDisplacements[targetAggContigAccessCount] = (MPI_Aint)nonContigSourceOffsets[i];
#ifdef onesidedtrace
                printf("mpi_put building arrays for iter %d - sourceBufferDisplacements is %d of nonContigSourceLens %d to target disp %d targetAggContigAccessCount is %d targetAggBlockLengths[targetAggContigAccessCount] is %d\n",i,sourceBufferDisplacements[targetAggContigAccessCount],nonContigSourceLens[i],targetAggDisplacements[targetAggContigAccessCount],targetAggContigAccessCount, targetAggBlockLengths[targetAggContigAccessCount]);
#endif
                targetAggContigAccessCount++;
                targetDisplacementToUseThisRound += nonContigSourceLens[i];
              }
              }
            }
          }
#ifdef onesidedtrace
        printf("roundIter %d bufferAmountToSend is %d sourceBufferOffset is %d offsetStart is %ld currentRoundFDStartForMyTargetAgg is %ld targetDisplacementToUseThisRound is %ld targetAggsForMyDataFDStart[aggIter] is %ld\n",roundIter, bufferAmountToSend,sourceBufferOffset, offsetStart,currentRoundFDStartForMyTargetAgg,targetDisplacementToUseThisRound,targetAggsForMyDataFDStart[aggIter]);
#endif

        } // bufferAmountToSend > 0
      } // contig list

      /* For gpfsmpio_aggmethod of 2 now build the derived type using the data from this round/agg and do 1 single mpi_put.
       */
      if (gpfsmpio_aggmethod == 2) {
        MPI_Datatype sourceBufferDerivedDataType, targetBufferDerivedDataType;
        if (targetAggContigAccessCount > 0) {
        /* Rebase source buffer offsets to 0 if there are any negative offsets for safety
         * when iteracting with PAMI.
         */
        MPI_Aint lowestDisplacement = 0;
        for (i=0;i<targetAggContigAccessCount;i++) {
          if (sourceBufferDisplacements[i] < lowestDisplacement)
            lowestDisplacement = sourceBufferDisplacements[i];
        }
        if (lowestDisplacement  < 0) {
          lowestDisplacement *= -1;
          for (i=0;i<targetAggContigAccessCount;i++)
            sourceBufferDisplacements[i] += lowestDisplacement;
        }
        MPI_Type_create_struct(targetAggContigAccessCount, targetAggBlockLengths, sourceBufferDisplacements, targetAggDataTypes, &sourceBufferDerivedDataType);
        MPI_Type_commit(&sourceBufferDerivedDataType);
        MPI_Type_create_struct(targetAggContigAccessCount, targetAggBlockLengths, targetAggDisplacements, targetAggDataTypes, &targetBufferDerivedDataType);
        MPI_Type_commit(&targetBufferDerivedDataType);

#ifdef onesidedtrace
        printf("mpi_put of derived type to agg %d targetAggContigAccessCount is %d\n",targetAggsForMyData[aggIter],targetAggContigAccessCount);
#endif
#ifndef ACTIVE_TARGET
        MPI_Win_lock(MPI_LOCK_SHARED, targetAggsForMyData[aggIter], 0, write_buf_window);
#endif

        MPI_Put((((char*)buf) - lowestDisplacement),1, sourceBufferDerivedDataType,targetAggsForMyData[aggIter],0, 1,targetBufferDerivedDataType,write_buf_window);

#ifndef ACTIVE_TARGET
        MPI_Win_unlock(targetAggsForMyData[aggIter], write_buf_window);
#endif

        }
        if (allocatedDerivedTypeArrays) {
          ADIOI_Free(targetAggBlockLengths);
          ADIOI_Free(targetAggDisplacements);
          ADIOI_Free(targetAggDataTypes);
          ADIOI_Free(sourceBufferDisplacements);
        }
        if (targetAggContigAccessCount > 0) {
        MPI_Type_free(&sourceBufferDerivedDataType);
        MPI_Type_free(&targetBufferDerivedDataType);
        }
      }
      if (!gpfsmpio_onesided_no_rmw) {
#ifndef ACTIVE_TARGET
        MPI_Win_lock(MPI_LOCK_SHARED, targetAggsForMyData[aggIter], 0, fd->io_buf_put_amounts_window);
#endif
        MPI_Put(&numBytesPutThisAggRound,1, MPI_INT,targetAggsForMyData[aggIter],myrank, 1,MPI_INT,fd->io_buf_put_amounts_window);

#ifndef ACTIVE_TARGET
        MPI_Win_unlock(targetAggsForMyData[aggIter], fd->io_buf_put_amounts_window);
#endif
      }
      } // baseoffset != -1
    } // target aggs

    /* the source procs send the requested data to the aggs */

#ifdef ACTIVE_TARGET
    MPI_Win_fence(0, write_buf_window);
    if (!gpfsmpio_onesided_no_rmw)
      MPI_Win_fence(0, fd->io_buf_put_amounts_window);
#else
    MPI_Barrier(fd->comm);
#endif

    if (iAmUsedAgg) {
    /* Determine what offsets define the portion of the file domain the agg is writing this round.
     */
        if ((fd_end[myAggRank] - currentRoundFDStart) < coll_bufsize) {
          if (myAggRank == greatestFileDomainAggRank) {
            if (fd_end[myAggRank] > lastFileOffset)
              currentRoundFDEnd = lastFileOffset;
            else
              currentRoundFDEnd = fd_end[myAggRank];
          }
          else
            currentRoundFDEnd = fd_end[myAggRank];
        }
        else
        currentRoundFDEnd = currentRoundFDStart + coll_bufsize - 1;

#ifdef onesidedtrace
        printf("currentRoundFDStart is %ld currentRoundFDEnd is %ld within file domeain %ld to %ld\n",currentRoundFDStart,currentRoundFDEnd,fd_start[myAggRank],fd_end[myAggRank]);
#endif

        int doWriteContig = 1;
        if (!gpfsmpio_onesided_no_rmw) {
          int numBytesPutIntoBuf = 0;
          for (i=0;i<nprocs;i++) {
            numBytesPutIntoBuf += write_buf_put_amounts[i];
            write_buf_put_amounts[i] = 0;
          }
          if (numBytesPutIntoBuf != ((int)(currentRoundFDEnd - currentRoundFDStart)+1)) {
            doWriteContig = 0;
            *hole_found = 1;
          }
        }

        if (!useIOBuffer) {
          if (doWriteContig)
            ADIO_WriteContig(fd, write_buf, (int)(currentRoundFDEnd - currentRoundFDStart)+1,
              MPI_BYTE, ADIO_EXPLICIT_OFFSET,currentRoundFDStart, &status, error_code);

        } else { /* use the thread writer */

        if(!pthread_equal(io_thread, pthread_self())) {
            pthread_join(io_thread, &thread_ret);
            *error_code = *(int *)thread_ret;
            if (*error_code != MPI_SUCCESS) return;
            io_thread = pthread_self();

        }
        io_thread_args.fd = fd;
        /* do a little pointer shuffling: background I/O works from one
         * buffer while two-phase machinery fills up another */

        if (currentWriteBuf == 0) {
            io_thread_args.buf = write_buf0;
            currentWriteBuf = 1;
            write_buf = write_buf1;
        }
        else {
            io_thread_args.buf = write_buf1;
            currentWriteBuf = 0;
            write_buf = write_buf0;
        }
        if (doWriteContig) {
        io_thread_args.io_kind = ADIOI_WRITE;
        io_thread_args.size = (currentRoundFDEnd-currentRoundFDStart) + 1;
        io_thread_args.offset = currentRoundFDStart;
        io_thread_args.status = status;
        io_thread_args.error_code = *error_code;

        if ( (pthread_create(&io_thread, NULL,
                ADIOI_IO_Thread_Func, &(io_thread_args))) != 0)
            io_thread = pthread_self();
        }
        } // useIOBuffer

    } // iAmUsedAgg

    if (!iAmUsedAgg && useIOBuffer) {
        if (currentWriteBuf == 0) {
            currentWriteBuf = 1;
            write_buf = write_buf1;
        }
        else {
            currentWriteBuf = 0;
            write_buf = write_buf0;
        }
    }

    if (iAmUsedAgg)
        currentRoundFDStart += coll_bufsize;

    if (roundIter<(numberOfRounds-1))
      MPI_Barrier(fd->comm);

    } /* for-loop roundIter */

    if (!bufTypeIsContig) {
      ADIOI_Free(nonContigSourceOffsets);
      ADIOI_Free(nonContigSourceLens);
    }

#ifdef ROMIO_GPFS
    endTimeBase = MPI_Wtime();
    gpfsmpio_prof_cw[GPFSMPIO_CIO_T_DEXCH] += (endTimeBase-startTimeBase);
#endif

    if (useIOBuffer) { /* thread writer cleanup */

    if ( !pthread_equal(io_thread, pthread_self()) ) {
        pthread_join(io_thread, &thread_ret);
        *error_code = *(int *)thread_ret;
    }

    }

    ADIOI_Free(targetAggsForMyData);
    ADIOI_Free(targetAggsForMyDataFDStart);
    ADIOI_Free(targetAggsForMyDataFDEnd);

    for (i=0;i<numberOfRounds;i++) {
      ADIOI_Free(targetAggsForMyDataFirstOffLenIndex[i]);
      ADIOI_Free(targetAggsForMyDataLastOffLenIndex[i]);
      if (bufTypeIsContig)
        ADIOI_Free(baseSourceBufferOffset[i]);
      else
        ADIOI_Free(baseNonContigSourceBufferOffset[i]);
    }
    ADIOI_Free(targetAggsForMyDataFirstOffLenIndex);
    ADIOI_Free(targetAggsForMyDataLastOffLenIndex);
    if (bufTypeIsContig)
      ADIOI_Free(baseSourceBufferOffset);
    else
      ADIOI_Free(baseNonContigSourceBufferOffset);

    if (!bufTypeIsContig)
      ADIOI_Delete_flattened(datatype);
    return;
}


void ADIOI_OneSidedReadAggregation(ADIO_File fd,
    ADIO_Offset *offset_list,
    ADIO_Offset *len_list,
    int contig_access_count,
    const void *buf,
    MPI_Datatype datatype,
    int *error_code,
    ADIO_Offset *st_offsets,
    ADIO_Offset *end_offsets,
    ADIO_Offset *fd_start,
    ADIO_Offset* fd_end)
{

    *error_code = MPI_SUCCESS; /* initialize to success */

#ifdef ROMIO_GPFS
    double startTimeBase,endTimeBase;
    startTimeBase = MPI_Wtime();
#endif

    MPI_Status status;
    pthread_t io_thread;
    void *thread_ret;
    ADIOI_IO_ThreadFuncData io_thread_args;
    int i,j; /* generic iterators */

    int nprocs,myrank;
    MPI_Comm_size(fd->comm, &nprocs);
    MPI_Comm_rank(fd->comm, &myrank);

    if (fd->io_buf_window == MPI_WIN_NULL ||
	    fd->io_buf_put_amounts_window == MPI_WIN_NULL)
    {
	ADIOI_OneSidedSetup(fd, nprocs);
    }
    /* This flag denotes whether the source datatype is contiguus, which is referenced throughout the algorithm
     * and defines how the source buffer offsets and data chunks are determined.  If the value is 1 (true - contiguous data)
     * things are profoundly simpler in that the source buffer offset for a given source offset simply linearly increases
     * by the chunk sizes being read.  If the value is 0 (non-contiguous) then these values are based on calculations
     * from the flattened source datatype.
     */
    int bufTypeIsContig;

    MPI_Aint bufTypeExtent;
    ADIOI_Flatlist_node *flatBuf=NULL;
    ADIOI_Datatype_iscontig(datatype, &bufTypeIsContig);

    /* maxNumContigOperations keeps track of how many different chunks we will need to recv
     * for the purpose of pre-allocating the data structures to hold them.
     */
    int maxNumContigOperations = contig_access_count;

    if (!bufTypeIsContig) {
    /* Flatten the non-contiguous source datatype.
     */
      flatBuf = ADIOI_Flatten_and_find(datatype);
      MPI_Type_extent(datatype, &bufTypeExtent);
    }
#ifdef onesidedtrace
      printf("ADIOI_OneSidedReadAggregation bufTypeIsContig is %d contig_access_count is %d\n",bufTypeIsContig,contig_access_count);
#endif

    ADIO_Offset lastFileOffset = 0, firstFileOffset = -1;

    /* Get the total range being read.
     */
    for (j=0;j<nprocs;j++) {
      if (end_offsets[j] > st_offsets[j]) {
        /* Guard against ranks with empty data.
         */
        lastFileOffset = ADIOI_MAX(lastFileOffset,end_offsets[j]);
        if (firstFileOffset == -1)
          firstFileOffset = st_offsets[j];
        else
          firstFileOffset = ADIOI_MIN(firstFileOffset,st_offsets[j]);
      }
    }

    int myAggRank = -1; /* if I am an aggregor this is my index into fd->hints->ranklist */
    int iAmUsedAgg = 0; /* whether or not this rank is used as an aggregator. */

    int naggs = fd->hints->cb_nodes;
    int coll_bufsize = fd->hints->cb_buffer_size;
#ifdef ROMIO_GPFS
    if (gpfsmpio_pthreadio == 1) {
    /* split buffer in half for a kind of double buffering with the threads*/
    coll_bufsize = fd->hints->cb_buffer_size/2;
    }
#endif

    /* This logic defines values that are used later to determine what offsets define the portion
     * of the file domain the agg is reading this round.
     */
    int greatestFileDomainAggRank = -1,smallestFileDomainAggRank = -1;
    ADIO_Offset greatestFileDomainOffset = 0;
    ADIO_Offset smallestFileDomainOffset = lastFileOffset;
    for (j=0;j<naggs;j++) {
      if (fd_end[j] > greatestFileDomainOffset) {
        greatestFileDomainOffset = fd_end[j];
        greatestFileDomainAggRank = j;
      }
      if (fd_start[j] < smallestFileDomainOffset) {
        smallestFileDomainOffset = fd_start[j];
        smallestFileDomainAggRank = j;
      }
      if (fd->hints->ranklist[j] == myrank) {
        myAggRank = j;
        if (fd_end[j] > fd_start[j]) {
          iAmUsedAgg = 1;
        }
      }
    }

#ifdef onesidedtrace
    printf("contig_access_count is %d lastFileOffset is %ld firstFileOffset is %ld\n",contig_access_count,lastFileOffset,firstFileOffset);
    for (j=0;j<contig_access_count;j++) {
      printf("offset_list[%d]: %ld , len_list[%d]: %ld\n",j,offset_list[j],j,len_list[j]);
    }
#endif


    /* for my offset range determine how much data and from whom I need to get
     * it.  For source ag sources, also determine the source file domain
     * offsets locally to reduce communication overhead */
    int *sourceAggsForMyData = (int *)ADIOI_Malloc(naggs * sizeof(int));
    ADIO_Offset *sourceAggsForMyDataFDStart = (ADIO_Offset *)ADIOI_Malloc(naggs * sizeof(ADIO_Offset));
    ADIO_Offset *sourceAggsForMyDataFDEnd = (ADIO_Offset *)ADIOI_Malloc(naggs * sizeof(ADIO_Offset));
    int numSourceAggs = 0;

    /* compute number of rounds */
    ADIO_Offset numberOfRounds = (ADIO_Offset)((((ADIO_Offset)(end_offsets[nprocs-1]-st_offsets[0]))/((ADIO_Offset)((ADIO_Offset)coll_bufsize*(ADIO_Offset)naggs)))) + 1;

    /* This data structure holds the beginning offset and len list index for the range to be read
     * coresponding to the round and source agg.
    */
    int **sourceAggsForMyDataFirstOffLenIndex = (int **)ADIOI_Malloc(numberOfRounds * sizeof(int *));
    for (i=0;i<numberOfRounds;i++) {
      sourceAggsForMyDataFirstOffLenIndex[i] = (int *)ADIOI_Malloc(naggs * sizeof(int));
      for (j=0;j<naggs;j++)
        sourceAggsForMyDataFirstOffLenIndex[i][j] = -1;
    }

    /* This data structure holds the ending offset and len list index for the range to be read
     * coresponding to the round and source agg.
    */
    int **sourceAggsForMyDataLastOffLenIndex = (int **)ADIOI_Malloc(numberOfRounds * sizeof(int *));
    for (i=0;i<numberOfRounds;i++)
      sourceAggsForMyDataLastOffLenIndex[i] = (int *)ADIOI_Malloc(naggs * sizeof(int));

    /* This data structure holds the base buffer offset for contiguous source data for the range to be read
     * coresponding to the round and source agg.
    */
    ADIO_Offset **baseRecvBufferOffset;
    if (bufTypeIsContig) {
      baseRecvBufferOffset = (ADIO_Offset **)ADIOI_Malloc(numberOfRounds * sizeof(ADIO_Offset *));
      for (i=0;i<numberOfRounds;i++) {
        baseRecvBufferOffset[i] = (ADIO_Offset *)ADIOI_Malloc(naggs * sizeof(ADIO_Offset));
        for (j=0;j<naggs;j++)
          baseRecvBufferOffset[i][j] = -1;
      }
    }
    ADIO_Offset currentRecvBufferOffset = 0;

    /* This data structure holds the number of extents, the index into the flattened buffer and the remnant length
     * beyond the flattened buffer indice corresponding to the base buffer offset for non-contiguous source data
     * for the range to be written coresponding to the round and target agg.
     */
    NonContigSourceBufOffset **baseNonContigSourceBufferOffset;
    if (!bufTypeIsContig) {
      baseNonContigSourceBufferOffset = (NonContigSourceBufOffset **) ADIOI_Malloc(numberOfRounds * sizeof(NonContigSourceBufOffset *));
      for (i=0;i<numberOfRounds;i++) {
        baseNonContigSourceBufferOffset[i] = (NonContigSourceBufOffset *)ADIOI_Malloc(naggs * sizeof(NonContigSourceBufOffset));
        /* initialize flatBufIndice to -1 to indicate that it is unset.
         */
        for (j=0;j<naggs;j++)
          baseNonContigSourceBufferOffset[i][j].flatBufIndice = -1;
      }
    }

    int currentDataTypeExtent = 0;
    int currentFlatBufIndice=0;
    ADIO_Offset currentIndiceOffset = 0;

#ifdef onesidedtrace
    printf("NumberOfRounds is %d\n",numberOfRounds);
    for (i=0;i<naggs;i++)
      printf("fd->hints->ranklist[%d]is %d fd_start is %ld fd_end is %ld\n",i,fd->hints->ranklist[i],fd_start[i],fd_end[i]);
    for (j=0;j<contig_access_count;j++)
      printf("offset_list[%d] is %ld len_list is %ld\n",j,offset_list[j],len_list[j]);
#endif

    int currentAggRankListIndex = 0;
    int maxNumNonContigSourceChunks = 0;

    /* This denotes the coll_bufsize boundaries within the source buffer for reading for 1 round.
     */
    ADIO_Offset intraRoundCollBufsizeOffset = 0;

    /* This data structure tracks what source aggs need to be read to on what rounds.
     */
    int *sourceAggsForMyDataCurrentRoundIter = (int *)ADIOI_Malloc(naggs * sizeof(int));
    for (i=0;i<naggs;i++)
      sourceAggsForMyDataCurrentRoundIter[i] = 0;

    /* This is the first of the two main loops in this algorithm.  The purpose of this loop is essentially to populate
     * the data structures defined above for what source data blocks needs to go where (source agg) and when (round iter).
     */
    int blockIter;
    for (blockIter=0;blockIter<contig_access_count;blockIter++) {

      /* Determine the starting source buffer offset for this block - for iter 0 skip it since that value is 0.
       */
      if (blockIter>0) {
        if (bufTypeIsContig) {
          currentRecvBufferOffset += len_list[blockIter-1];
        }
        else {
          /* Non-contiguous source datatype, count up the extents and indices to this point
           * in the blocks.
           */
          ADIO_Offset sourceBlockTotal = 0;
          int lastIndiceUsed;
          int numNonContigSourceChunks = 0;
          while (sourceBlockTotal < len_list[blockIter-1]) {
            numNonContigSourceChunks++;
            sourceBlockTotal += (flatBuf->blocklens[currentFlatBufIndice] - currentIndiceOffset);
            lastIndiceUsed = currentFlatBufIndice;
            currentFlatBufIndice++;
            if (currentFlatBufIndice == flatBuf->count) {
              currentFlatBufIndice = 0;
              currentDataTypeExtent++;
            }
            currentIndiceOffset = 0;
          }
          if (sourceBlockTotal > len_list[blockIter-1]) {
            currentFlatBufIndice--;
            if (currentFlatBufIndice < 0 ) {
              currentDataTypeExtent--;
              currentFlatBufIndice = flatBuf->count-1;
            }
            currentIndiceOffset =  len_list[blockIter-1] - (sourceBlockTotal - flatBuf->blocklens[lastIndiceUsed]);
            ADIOI_Assert((currentIndiceOffset >= 0) && (currentIndiceOffset < flatBuf->blocklens[currentFlatBufIndice]));
          }
          else
            currentIndiceOffset = 0;
          maxNumContigOperations += numNonContigSourceChunks;
          if (numNonContigSourceChunks > maxNumNonContigSourceChunks)
            maxNumNonContigSourceChunks = numNonContigSourceChunks;
#ifdef onesidedtrace
          printf("block iter %d currentFlatBufIndice is now %d currentDataTypeExtent is now %d currentIndiceOffset is now %ld maxNumContigOperations is now %d\n",blockIter,currentFlatBufIndice,currentDataTypeExtent,currentIndiceOffset,maxNumContigOperations);
#endif
        } // !bufTypeIsContig
      } // blockIter > 0

      /* For the first iteration we need to include these maxNumContigOperations and maxNumNonContigSourceChunks
       * for non-contig case even though we did not need to compute the starting offset.
       */
      if ((blockIter == 0) && (!bufTypeIsContig)) {
        ADIO_Offset sourceBlockTotal = 0;
        int tmpCurrentFlatBufIndice = currentFlatBufIndice;
        while (sourceBlockTotal < len_list[0]) {
          maxNumContigOperations++;
          maxNumNonContigSourceChunks++;
          sourceBlockTotal += flatBuf->blocklens[tmpCurrentFlatBufIndice];
          tmpCurrentFlatBufIndice++;
          if (tmpCurrentFlatBufIndice == flatBuf->count) {
            tmpCurrentFlatBufIndice = 0;
          }
        }
      }

      ADIO_Offset blockStart = offset_list[blockIter], blockEnd = offset_list[blockIter]+len_list[blockIter]-(ADIO_Offset)1;

      /* Find the starting source agg for this block - normally it will be the current agg so guard the expensive
       * while loop with a cheap if-check which for large numbers of small blocks will usually be false.
       */
      if (!((blockStart >= fd_start[currentAggRankListIndex]) && (blockStart <= fd_end[currentAggRankListIndex]))) {
        while (!((blockStart >= fd_start[currentAggRankListIndex]) && (blockStart <= fd_end[currentAggRankListIndex])))
          currentAggRankListIndex++;
      };

      /* Determine if this is a new source agg.
       */
      if (blockIter>0) {
        if ((offset_list[blockIter-1]+len_list[blockIter-1]-(ADIO_Offset)1) < fd_start[currentAggRankListIndex])
          numSourceAggs++;
      }

       /* Determine which round to start reading.
        */
      if ((blockStart - fd_start[currentAggRankListIndex]) >= coll_bufsize) {
        ADIO_Offset currentRoundBlockStart = fd_start[currentAggRankListIndex];
        int startingRound = 0;
        while (blockStart > (currentRoundBlockStart + coll_bufsize - (ADIO_Offset)1)) {
          currentRoundBlockStart+=coll_bufsize;
          startingRound++;
        }
        sourceAggsForMyDataCurrentRoundIter[numSourceAggs] = startingRound;
      }

      /* Initialize the data structures if this is the first offset in the round/source agg.
       */
      if (sourceAggsForMyDataFirstOffLenIndex[sourceAggsForMyDataCurrentRoundIter[numSourceAggs]][numSourceAggs] == -1) {
        sourceAggsForMyData[numSourceAggs] = fd->hints->ranklist[currentAggRankListIndex];
        sourceAggsForMyDataFDStart[numSourceAggs] = fd_start[currentAggRankListIndex];
        /* Round up file domain to the first actual offset used if this is the first file domain.
         */
        if (currentAggRankListIndex == smallestFileDomainAggRank) {
          if (sourceAggsForMyDataFDStart[numSourceAggs] < firstFileOffset)
            sourceAggsForMyDataFDStart[numSourceAggs] = firstFileOffset;
        }
        sourceAggsForMyDataFDEnd[numSourceAggs] = fd_end[currentAggRankListIndex];
        /* Round down file domain to the last actual offset used if this is the last file domain.
         */
        if (currentAggRankListIndex == greatestFileDomainAggRank) {
          if (sourceAggsForMyDataFDEnd[numSourceAggs] > lastFileOffset)
            sourceAggsForMyDataFDEnd[numSourceAggs] = lastFileOffset;
        }
        sourceAggsForMyDataFirstOffLenIndex[sourceAggsForMyDataCurrentRoundIter[numSourceAggs]][numSourceAggs] = blockIter;
        if (bufTypeIsContig)
          baseRecvBufferOffset[sourceAggsForMyDataCurrentRoundIter[numSourceAggs]][numSourceAggs] = currentRecvBufferOffset;
        else {
          baseNonContigSourceBufferOffset[sourceAggsForMyDataCurrentRoundIter[numSourceAggs]][numSourceAggs].flatBufIndice = currentFlatBufIndice;
          baseNonContigSourceBufferOffset[sourceAggsForMyDataCurrentRoundIter[numSourceAggs]][numSourceAggs].dataTypeExtent = currentDataTypeExtent;
          baseNonContigSourceBufferOffset[sourceAggsForMyDataCurrentRoundIter[numSourceAggs]][numSourceAggs].indiceOffset = currentIndiceOffset;
        }

        intraRoundCollBufsizeOffset = fd_start[currentAggRankListIndex] + ((sourceAggsForMyDataCurrentRoundIter[numSourceAggs]+1) * coll_bufsize);
#ifdef onesidedtrace
        printf("init settings numSourceAggs %d offset_list[%d] with value %ld past fd border %ld with len %ld currentRecvBufferOffset set to %ld intraRoundCollBufsizeOffset set to %ld\n",numSourceAggs,blockIter,offset_list[blockIter],fd_start[currentAggRankListIndex],len_list[blockIter],currentRecvBufferOffset,intraRoundCollBufsizeOffset);
#endif

      }

      /* Replace the last offset block iter with this one.
       */
      sourceAggsForMyDataLastOffLenIndex[sourceAggsForMyDataCurrentRoundIter[numSourceAggs]][numSourceAggs] = blockIter;

      /* If this blocks extends into the next file domain handle this situation.
       */
      if (blockEnd > fd_end[currentAggRankListIndex]) {
#ifdef onesidedtrace
        printf("large block, blockEnd %ld >= fd_end[currentAggRankListIndex] %ld\n",blockEnd,fd_end[currentAggRankListIndex]);
#endif
        while (blockEnd >= fd_end[currentAggRankListIndex]) {
          ADIO_Offset thisAggBlockEnd = fd_end[currentAggRankListIndex];
          if (thisAggBlockEnd >= intraRoundCollBufsizeOffset) {
            while (thisAggBlockEnd >= intraRoundCollBufsizeOffset) {
              sourceAggsForMyDataCurrentRoundIter[numSourceAggs]++;
              intraRoundCollBufsizeOffset += coll_bufsize;
              sourceAggsForMyDataFirstOffLenIndex[sourceAggsForMyDataCurrentRoundIter[numSourceAggs]][numSourceAggs] = blockIter;
              if (bufTypeIsContig)
                baseRecvBufferOffset[sourceAggsForMyDataCurrentRoundIter[numSourceAggs]][numSourceAggs] = currentRecvBufferOffset;
              else {
                baseNonContigSourceBufferOffset[sourceAggsForMyDataCurrentRoundIter[numSourceAggs]][numSourceAggs].flatBufIndice = currentFlatBufIndice;
                baseNonContigSourceBufferOffset[sourceAggsForMyDataCurrentRoundIter[numSourceAggs]][numSourceAggs].dataTypeExtent = currentDataTypeExtent;
                baseNonContigSourceBufferOffset[sourceAggsForMyDataCurrentRoundIter[numSourceAggs]][numSourceAggs].indiceOffset = currentIndiceOffset;
              }

              sourceAggsForMyDataLastOffLenIndex[sourceAggsForMyDataCurrentRoundIter[numSourceAggs]][numSourceAggs] = blockIter;
#ifdef onesidedtrace
              printf("sourceAggsForMyDataCurrentRoundI%d] is now %d intraRoundCollBufsizeOffset is now %ld\n",numSourceAggs,sourceAggsForMyDataCurrentRoundIter[numSourceAggs],intraRoundCollBufsizeOffset);
#endif
            } // while (thisAggBlockEnd >= intraRoundCollBufsizeOffset)
          } // if (thisAggBlockEnd >= intraRoundCollBufsizeOffset)

          currentAggRankListIndex++;

          /* Skip over unused aggs.
           */
          if (fd_start[currentAggRankListIndex] > fd_end[currentAggRankListIndex]) {
            while (fd_start[currentAggRankListIndex] > fd_end[currentAggRankListIndex])
              currentAggRankListIndex++;

          } // (fd_start[currentAggRankListIndex] > fd_end[currentAggRankListIndex])

          /* Start new source agg.
           */
          if (blockEnd >= fd_start[currentAggRankListIndex]) {
            numSourceAggs++;
            sourceAggsForMyData[numSourceAggs] = fd->hints->ranklist[currentAggRankListIndex];
            sourceAggsForMyDataFDStart[numSourceAggs] = fd_start[currentAggRankListIndex];
            /* Round up file domain to the first actual offset used if this is the first file domain.
             */
            if (currentAggRankListIndex == smallestFileDomainAggRank) {
              if (sourceAggsForMyDataFDStart[numSourceAggs] < firstFileOffset)
                sourceAggsForMyDataFDStart[numSourceAggs] = firstFileOffset;
            }
            sourceAggsForMyDataFDEnd[numSourceAggs] = fd_end[currentAggRankListIndex];
            /* Round down file domain to the last actual offset used if this is the last file domain.
             */
            if (currentAggRankListIndex == greatestFileDomainAggRank) {
              if (sourceAggsForMyDataFDEnd[numSourceAggs] > lastFileOffset)
                sourceAggsForMyDataFDEnd[numSourceAggs] = lastFileOffset;
            }
            sourceAggsForMyDataFirstOffLenIndex[sourceAggsForMyDataCurrentRoundIter[numSourceAggs]][numSourceAggs] = blockIter;
            if (bufTypeIsContig)
              baseRecvBufferOffset[sourceAggsForMyDataCurrentRoundIter[numSourceAggs]][numSourceAggs] = currentRecvBufferOffset;
            else {
              baseNonContigSourceBufferOffset[sourceAggsForMyDataCurrentRoundIter[numSourceAggs]][numSourceAggs].flatBufIndice = currentFlatBufIndice;
              baseNonContigSourceBufferOffset[sourceAggsForMyDataCurrentRoundIter[numSourceAggs]][numSourceAggs].dataTypeExtent = currentDataTypeExtent;
              baseNonContigSourceBufferOffset[sourceAggsForMyDataCurrentRoundIter[numSourceAggs]][numSourceAggs].indiceOffset = currentIndiceOffset;
            }

#ifdef onesidedtrace
            printf("large block init settings numSourceAggs %d offset_list[%d] with value %ld past fd border %ld with len %ld\n",numSourceAggs,i,offset_list[blockIter],fd_start[currentAggRankListIndex],len_list[blockIter]);
#endif
            intraRoundCollBufsizeOffset = fd_start[currentAggRankListIndex] + coll_bufsize;
            sourceAggsForMyDataLastOffLenIndex[sourceAggsForMyDataCurrentRoundIter[numSourceAggs]][numSourceAggs] = blockIter;
          } // if (blockEnd >= fd_start[currentAggRankListIndex])
        } // while (blockEnd >= fd_end[currentAggRankListIndex])
      } // if (blockEnd > fd_end[currentAggRankListIndex])

      /* Else if we are still in the same file domain / source agg but have gone past the coll_bufsize and need
       * to advance to the next round handle this situation.
       */
      else if (blockEnd >= intraRoundCollBufsizeOffset) {
        ADIO_Offset currentBlockEnd = blockEnd;
        while (currentBlockEnd >= intraRoundCollBufsizeOffset) {
          sourceAggsForMyDataCurrentRoundIter[numSourceAggs]++;
          intraRoundCollBufsizeOffset += coll_bufsize;
          sourceAggsForMyDataFirstOffLenIndex[sourceAggsForMyDataCurrentRoundIter[numSourceAggs]][numSourceAggs] = blockIter;
          if (bufTypeIsContig)
            baseRecvBufferOffset[sourceAggsForMyDataCurrentRoundIter[numSourceAggs]][numSourceAggs] = currentRecvBufferOffset;
          else {
            baseNonContigSourceBufferOffset[sourceAggsForMyDataCurrentRoundIter[numSourceAggs]][numSourceAggs].flatBufIndice = currentFlatBufIndice;
            baseNonContigSourceBufferOffset[sourceAggsForMyDataCurrentRoundIter[numSourceAggs]][numSourceAggs].dataTypeExtent = currentDataTypeExtent;
            baseNonContigSourceBufferOffset[sourceAggsForMyDataCurrentRoundIter[numSourceAggs]][numSourceAggs].indiceOffset = currentIndiceOffset;
          }
          sourceAggsForMyDataLastOffLenIndex[sourceAggsForMyDataCurrentRoundIter[numSourceAggs]][numSourceAggs] = blockIter;
#ifdef onesidedtrace
          printf("block less than fd currentBlockEnd is now %ld intraRoundCollBufsizeOffset is now %ld sourceAggsForMyDataCurrentRoundIter[%d] is now %d\n",currentBlockEnd, intraRoundCollBufsizeOffset, numSourceAggs,sourceAggsForMyDataCurrentRoundIter[numSourceAggs]);
#endif
        } // while (currentBlockEnd >= intraRoundCollBufsizeOffset)
      } // else if (blockEnd >= intraRoundCollBufsizeOffset)

      /* Need to advance numSourceAggs if this is the last source offset to
       * include this one.
       */
      if (blockIter == (contig_access_count-1))
        numSourceAggs++;
    }

#ifdef onesidedtrace
    printf("numSourceAggs is %d\n",numSourceAggs);
    for (i=0;i<numSourceAggs;i++) {
      for (j=0;j<=sourceAggsForMyDataCurrentRoundIter[i];j++)
        printf("sourceAggsForMyData[%d] is %d sourceAggsForMyDataFDStart[%d] is %ld sourceAggsForMyDataFDEnd is %ld sourceAggsForMyDataFirstOffLenIndex is %d with value %ld sourceAggsForMyDataLastOffLenIndex is %d with value %ld\n",i,sourceAggsForMyData[i],i,sourceAggsForMyDataFDStart[i],sourceAggsForMyDataFDEnd[i],sourceAggsForMyDataFirstOffLenIndex[j][i],offset_list[sourceAggsForMyDataFirstOffLenIndex[j][i]],sourceAggsForMyDataLastOffLenIndex[j][i],offset_list[sourceAggsForMyDataLastOffLenIndex[j][i]]);

    }
#endif

    ADIOI_Free(sourceAggsForMyDataCurrentRoundIter);

    /* use the two-phase buffer allocated in the file_open - no app should ever
     * be both reading and reading at the same time */
    char *read_buf0 = fd->io_buf;
    char *read_buf1 = fd->io_buf + coll_bufsize;
    /* if threaded i/o selected, we'll do a kind of double buffering */
    char *read_buf = read_buf0;

    int currentReadBuf = 0;
    int useIOBuffer = 0;
#ifdef ROMIO_GPFS
    if (gpfsmpio_pthreadio && (numberOfRounds>1)) {
    useIOBuffer = 1;
    io_thread = pthread_self();
    }
#endif

    MPI_Win read_buf_window = fd->io_buf_window;

#ifdef ACTIVE_TARGET
    MPI_Win_fence(0, read_buf_window);
#endif

    ADIO_Offset currentRoundFDStart = 0, nextRoundFDStart = 0;
    ADIO_Offset currentRoundFDEnd = 0, nextRoundFDEnd = 0;

    if (iAmUsedAgg) {
      currentRoundFDStart = fd_start[myAggRank];
      nextRoundFDStart = fd_start[myAggRank];
      if (myAggRank == smallestFileDomainAggRank) {
        if (currentRoundFDStart < firstFileOffset)
          currentRoundFDStart = firstFileOffset;
        if (nextRoundFDStart < firstFileOffset)
          nextRoundFDStart = firstFileOffset;
      }
      else if (myAggRank == greatestFileDomainAggRank) {
        if (currentRoundFDEnd > lastFileOffset)
          currentRoundFDEnd = lastFileOffset;
        if (nextRoundFDEnd > lastFileOffset)
          nextRoundFDEnd = lastFileOffset;
      }
    }

#ifdef ROMIO_GPFS
    endTimeBase = MPI_Wtime();
    gpfsmpio_prof_cw[GPFSMPIO_CIO_T_DEXCH_SETUP] += (endTimeBase-startTimeBase);
    startTimeBase = MPI_Wtime();
#endif


    ADIO_Offset currentBaseRecvBufferOffset = 0;

    /* These data structures are used to track the offset/len pairs for non-contiguous source buffers that are
     * to be used for each block of data in the offset list.  Allocate them once with the maximum size and then 
     * reuse the space throughout the algoritm below.
     */
    ADIO_Offset *nonContigSourceOffsets;
    int *nonContigSourceLens;
    if (!bufTypeIsContig) {
      nonContigSourceOffsets = (ADIO_Offset *)ADIOI_Malloc((maxNumNonContigSourceChunks+2) * sizeof(ADIO_Offset));
      nonContigSourceLens = (int *)ADIOI_Malloc((maxNumNonContigSourceChunks+2) * sizeof(int));
    }
    /* This is the second main loop of the algorithm, actually nested loop of source aggs within rounds.  There are 2 flavors of this.
     * For gpfsmpio_aggmethod of 1 each nested iteration for the source agg does an mpi_put on a contiguous chunk using a primative datatype
     * determined using the data structures from the first main loop.  For gpfsmpio_aggmethod of 2 each nested iteration for the source agg
     * builds up data to use in created a derived data type for 1 mpi_put that is done for the source agg for each round.
     */
    int roundIter;
    for (roundIter=0;roundIter<numberOfRounds;roundIter++) {

    /* determine what offsets define the portion of the file domain the agg is reading this round */
    if (iAmUsedAgg) {

        currentRoundFDStart = nextRoundFDStart;

        if (!useIOBuffer || (roundIter == 0)) {
        int amountDataToReadThisRound;
        if ((fd_end[myAggRank] - currentRoundFDStart) < coll_bufsize) {
            currentRoundFDEnd = fd_end[myAggRank];
            amountDataToReadThisRound = ((currentRoundFDEnd-currentRoundFDStart)+1);
        }
        else {
            currentRoundFDEnd = currentRoundFDStart + coll_bufsize - 1;
            amountDataToReadThisRound = coll_bufsize;
        }

        /* read currentRoundFDEnd bytes */
        ADIO_ReadContig(fd, read_buf,amountDataToReadThisRound,
            MPI_BYTE, ADIO_EXPLICIT_OFFSET, currentRoundFDStart,
            &status, error_code);
        currentReadBuf = 1;

        }
        if (useIOBuffer) { /* use the thread reader for the next round */
        /* switch back and forth between the read buffers so that the data aggregation code is diseminating 1 buffer while the thread is reading into the other */

        if (roundIter > 0)
            currentRoundFDEnd = nextRoundFDEnd;

        if (roundIter < (numberOfRounds-1)) {
            nextRoundFDStart += coll_bufsize;
            int amountDataToReadNextRound;
            if ((fd_end[myAggRank] - nextRoundFDStart) < coll_bufsize) {
            nextRoundFDEnd = fd_end[myAggRank];
            amountDataToReadNextRound = ((nextRoundFDEnd-nextRoundFDStart)+1);
            }
            else {
            nextRoundFDEnd = nextRoundFDStart + coll_bufsize - 1;
            amountDataToReadNextRound = coll_bufsize;
            }

            if(!pthread_equal(io_thread, pthread_self())) {
            pthread_join(io_thread, &thread_ret);
            *error_code = *(int *)thread_ret;
            if (*error_code != MPI_SUCCESS) return;
            io_thread = pthread_self();

            }
            io_thread_args.fd = fd;
            /* do a little pointer shuffling: background I/O works from one
             * buffer while two-phase machinery fills up another */

            if (currentReadBuf == 0) {
            io_thread_args.buf = read_buf0;
            currentReadBuf = 1;
            read_buf = read_buf1;
            }
            else {
            io_thread_args.buf = read_buf1;
            currentReadBuf = 0;
            read_buf = read_buf0;
            }
            io_thread_args.io_kind = ADIOI_READ;
            io_thread_args.size = amountDataToReadNextRound;
            io_thread_args.offset = nextRoundFDStart;
            io_thread_args.status = status;
            io_thread_args.error_code = *error_code;
            if ( (pthread_create(&io_thread, NULL,
                    ADIOI_IO_Thread_Func, &(io_thread_args))) != 0)
            io_thread = pthread_self();

        }
        else { /* last round */

            if(!pthread_equal(io_thread, pthread_self())) {
            pthread_join(io_thread, &thread_ret);
            *error_code = *(int *)thread_ret;
            if (*error_code != MPI_SUCCESS) return;
            io_thread = pthread_self();

            }
            if (currentReadBuf == 0) {
            read_buf = read_buf0;
            }
            else {
            read_buf = read_buf1;
            }

        }
        } /* useIOBuffer */
    } /* IAmUsedAgg */
    else if (useIOBuffer) {
      if (roundIter < (numberOfRounds-1)) {
            if (currentReadBuf == 0) {
            currentReadBuf = 1;
            read_buf = read_buf1;
            }
            else {
            currentReadBuf = 0;
            read_buf = read_buf0;
            }
      }
      else {
            if (currentReadBuf == 0) {
            read_buf = read_buf0;
            }
            else {
            read_buf = read_buf1;
            }
      }

    }
    // wait until the read buffers are full before we start pulling from the source procs
    MPI_Barrier(fd->comm);

    int aggIter;
    for (aggIter=0;aggIter<numSourceAggs;aggIter++) {

    /* If we have data for the round/agg process it.
     */
    if ((bufTypeIsContig && (baseRecvBufferOffset[roundIter][aggIter] != -1)) || (!bufTypeIsContig && (baseNonContigSourceBufferOffset[roundIter][aggIter].flatBufIndice != -1))) {

      ADIO_Offset currentRoundFDStartForMySourceAgg = (ADIO_Offset)((ADIO_Offset)sourceAggsForMyDataFDStart[aggIter] + (ADIO_Offset)((ADIO_Offset)roundIter*(ADIO_Offset)coll_bufsize));
      ADIO_Offset currentRoundFDEndForMySourceAgg = (ADIO_Offset)((ADIO_Offset)sourceAggsForMyDataFDStart[aggIter] + (ADIO_Offset)((ADIO_Offset)(roundIter+1)*(ADIO_Offset)coll_bufsize) - (ADIO_Offset)1);

      int sourceAggContigAccessCount = 0;

      /* These data structures are used for the derived datatype mpi_put
       * in the gpfsmpio_aggmethod of 2 case.
       */
      int *sourceAggBlockLengths;
      MPI_Aint *sourceAggDisplacements, *recvBufferDisplacements;
      MPI_Datatype *sourceAggDataTypes;

      int allocatedDerivedTypeArrays = 0;

      ADIO_Offset recvBufferOffset = 0;

      /* Process the range of offsets for this source agg.
       */
      int offsetIter;
         for (offsetIter=sourceAggsForMyDataFirstOffLenIndex[roundIter][aggIter];offsetIter<=sourceAggsForMyDataLastOffLenIndex[roundIter][aggIter];offsetIter++) {
        if (currentRoundFDEndForMySourceAgg > sourceAggsForMyDataFDEnd[aggIter])
            currentRoundFDEndForMySourceAgg = sourceAggsForMyDataFDEnd[aggIter];

        ADIO_Offset offsetStart = offset_list[offsetIter], offsetEnd = (offset_list[offsetIter]+len_list[offsetIter]-(ADIO_Offset)1);

        /* Get the base source buffer offset for this iterataion of the source agg in the round.
         * For the first one just get the predetermined base value, for subsequent ones keep track
         * of the value for each iteration and increment it.
         */
        if (offsetIter == sourceAggsForMyDataFirstOffLenIndex[roundIter][aggIter]) {
          if (bufTypeIsContig)
            currentBaseRecvBufferOffset = baseRecvBufferOffset[roundIter][aggIter];
          else {
            currentFlatBufIndice = baseNonContigSourceBufferOffset[roundIter][aggIter].flatBufIndice;
            currentDataTypeExtent = baseNonContigSourceBufferOffset[roundIter][aggIter].dataTypeExtent;
            currentIndiceOffset = baseNonContigSourceBufferOffset[roundIter][aggIter].indiceOffset;
#ifdef onesidedtrace
            printf("currentFlatBufIndice initially set to %d starting this round/agg %d/%d\n",currentFlatBufIndice,roundIter,aggIter);
#endif
          }
        }
        else {
          if (bufTypeIsContig)
            currentBaseRecvBufferOffset += len_list[offsetIter-1];
          else {

          /* For non-contiguous source datatype advance the flattened buffer machinery to this offset.
           * Note that currentDataTypeExtent, currentFlatBufIndice and currentIndiceOffset are used and
           * advanced across the offsetIters.
           */
          ADIO_Offset sourceBlockTotal = 0;
          int lastIndiceUsed;
          while (sourceBlockTotal < len_list[offsetIter-1]) {
            sourceBlockTotal += (flatBuf->blocklens[currentFlatBufIndice] - currentIndiceOffset);
            lastIndiceUsed = currentFlatBufIndice;
            currentFlatBufIndice++;
            if (currentFlatBufIndice == flatBuf->count) {
              currentFlatBufIndice = 0;
              currentDataTypeExtent++;
            }
            currentIndiceOffset = 0;
          } // while
          if (sourceBlockTotal > len_list[offsetIter-1]) {
            currentFlatBufIndice--;
            if (currentFlatBufIndice < 0 ) {
              currentDataTypeExtent--;
              currentFlatBufIndice = flatBuf->count-1;
            }
            currentIndiceOffset =  len_list[offsetIter-1] - (sourceBlockTotal - flatBuf->blocklens[lastIndiceUsed]);
            ADIOI_Assert((currentIndiceOffset >= 0) && (currentIndiceOffset < flatBuf->blocklens[currentFlatBufIndice]));
          }
          else
            currentIndiceOffset = 0;
#ifdef onesidedtrace
          printf("contig_access_count source agg %d currentFlatBufIndice is now %d currentDataTypeExtent is now %d currentIndiceOffset is now %ld\n",aggIter,currentFlatBufIndice,currentDataTypeExtent,currentIndiceOffset);
#endif
          }

        }

        /* For source buffer contiguous here is the offset we will use for this iteration.
         */
        if (bufTypeIsContig)
          recvBufferOffset = currentBaseRecvBufferOffset;

#ifdef onesidedtrace
        printf("roundIter %d source iter %d sourceAggsForMyData is %d offset_list[%d] is %ld len_list[%d] is %ld sourceAggsForMyDataFDStart is %ld sourceAggsForMyDataFDEnd is %ld currentRoundFDStartForMySourceAgg is %ld currentRoundFDEndForMySourceAgg is %ld sourceAggsForMyDataFirstOffLenIndex is %d baseRecvBufferOffset is %ld\n",
            roundIter,aggIter,sourceAggsForMyData[aggIter],offsetIter,offset_list[offsetIter],offsetIter,len_list[offsetIter],
            sourceAggsForMyDataFDStart[aggIter],sourceAggsForMyDataFDEnd[aggIter],
            currentRoundFDStartForMySourceAgg,currentRoundFDEndForMySourceAgg,sourceAggsForMyDataFirstOffLenIndex[roundIter][aggIter],baseRecvBufferOffset[roundIter][aggIter]);
#endif


        /* Determine the amount of data and exact source buffer offsets to use.
         */
        int bufferAmountToRecv = 0;

        if ((offsetStart >= currentRoundFDStartForMySourceAgg) && (offsetStart <= currentRoundFDEndForMySourceAgg)) {
            if (offsetEnd > currentRoundFDEndForMySourceAgg)
            bufferAmountToRecv = (currentRoundFDEndForMySourceAgg - offsetStart) +1;
            else
            bufferAmountToRecv = (offsetEnd - offsetStart) +1;
        }
        else if ((offsetEnd >= currentRoundFDStartForMySourceAgg) && (offsetEnd <= currentRoundFDEndForMySourceAgg)) {
            if (offsetEnd > currentRoundFDEndForMySourceAgg)
            bufferAmountToRecv = (currentRoundFDEndForMySourceAgg - currentRoundFDStartForMySourceAgg) +1;
            else
            bufferAmountToRecv = (offsetEnd - currentRoundFDStartForMySourceAgg) +1;
            if (offsetStart < currentRoundFDStartForMySourceAgg) {
              if (bufTypeIsContig)
                recvBufferOffset += (currentRoundFDStartForMySourceAgg-offsetStart);
              offsetStart = currentRoundFDStartForMySourceAgg;
            }
        }
        else if ((offsetStart <= currentRoundFDStartForMySourceAgg) && (offsetEnd >= currentRoundFDEndForMySourceAgg)) {
            bufferAmountToRecv = (currentRoundFDEndForMySourceAgg - currentRoundFDStartForMySourceAgg) +1;
            if (bufTypeIsContig)
              recvBufferOffset += (currentRoundFDStartForMySourceAgg-offsetStart);
            offsetStart = currentRoundFDStartForMySourceAgg;
        }

        if (bufferAmountToRecv > 0) { /* we have data to recv this round */
          if (gpfsmpio_aggmethod == 2) {
            /* Only allocate these arrays if we are using method 2 and only do it once for this round/source agg.
             */
            if (!allocatedDerivedTypeArrays) {
              sourceAggBlockLengths = (int *)ADIOI_Malloc(maxNumContigOperations * sizeof(int));
              sourceAggDisplacements = (MPI_Aint *)ADIOI_Malloc(maxNumContigOperations * sizeof(MPI_Aint));
              recvBufferDisplacements = (MPI_Aint *)ADIOI_Malloc(maxNumContigOperations * sizeof(MPI_Aint));
              sourceAggDataTypes = (MPI_Datatype *)ADIOI_Malloc(maxNumContigOperations * sizeof(MPI_Datatype));
              allocatedDerivedTypeArrays = 1;
            }
          }

          /* For the non-contiguous source datatype we need to determine the number,size and offsets of the chunks
           * from the source buffer to be sent to this contiguous chunk defined by the round/agg/iter in the source.
           */
          int numNonContigSourceChunks = 0;
          ADIO_Offset baseDatatypeInstanceOffset = 0;

          if (!bufTypeIsContig) {

            currentRecvBufferOffset = (ADIO_Offset)((ADIO_Offset)currentDataTypeExtent * (ADIO_Offset)bufTypeExtent) + flatBuf->indices[currentFlatBufIndice] + currentIndiceOffset;
#ifdef onesidedtrace
            printf("!bufTypeIsContig currentRecvBufferOffset set to %ld for roundIter %d source %d currentDataTypeExtent %d flatBuf->indices[currentFlatBufIndice] %ld currentIndiceOffset %ld currentFlatBufIndice %d\n",currentRecvBufferOffset,roundIter,aggIter,currentDataTypeExtent,flatBuf->indices[currentFlatBufIndice],currentIndiceOffset,currentFlatBufIndice);
#endif

            /* Use a tmp variable for the currentFlatBufIndice and currentIndiceOffset as they are used across the offsetIters
             * to compute the starting point for this iteration and will be modified now to compute the data chunks for
             * this iteration.
             */
            int tmpFlatBufIndice = currentFlatBufIndice;
            ADIO_Offset tmpIndiceOffset = currentIndiceOffset;

            /* now populate the nonContigSourceOffsets and nonContigSourceLens arrays for use in the one-sided operations.
             */
            int ncArrayIndex = 0;
            int remainingBytesToLoadedIntoNCArrays = bufferAmountToRecv;

            int datatypeInstances = currentDataTypeExtent;
            while (remainingBytesToLoadedIntoNCArrays > 0) {
              ADIOI_Assert(ncArrayIndex < (maxNumNonContigSourceChunks+2));
              nonContigSourceOffsets[ncArrayIndex] = currentRecvBufferOffset;

              if ((flatBuf->blocklens[tmpFlatBufIndice] - tmpIndiceOffset) >= remainingBytesToLoadedIntoNCArrays) {
                nonContigSourceLens[ncArrayIndex] = remainingBytesToLoadedIntoNCArrays;
                remainingBytesToLoadedIntoNCArrays = 0;
              }
              else {
                nonContigSourceLens[ncArrayIndex] = (int)(flatBuf->blocklens[tmpFlatBufIndice] - tmpIndiceOffset);
                remainingBytesToLoadedIntoNCArrays -= (flatBuf->blocklens[tmpFlatBufIndice] - tmpIndiceOffset);
                tmpIndiceOffset = 0;
                tmpFlatBufIndice++;
                if (tmpFlatBufIndice == flatBuf->count) {
                  tmpFlatBufIndice = 0;
                  datatypeInstances++;
                  baseDatatypeInstanceOffset = datatypeInstances * bufTypeExtent;
                }
                currentRecvBufferOffset = baseDatatypeInstanceOffset + flatBuf->indices[tmpFlatBufIndice];
              }
              ncArrayIndex++;
              numNonContigSourceChunks++;
            } // while
#ifdef onesidedtrace
            printf("currentRecvBufferOffset finally set to %ld\n",currentRecvBufferOffset);
#endif
          } // !bufTypeIsContig

          /* Determine the offset into the source window.
           */
          MPI_Aint sourceDisplacementToUseThisRound = (MPI_Aint) ((ADIO_Offset)offsetStart - currentRoundFDStartForMySourceAgg);

          /* If using the thread reader select the appropriate side of the split window.
           */
          if (useIOBuffer && (read_buf == read_buf1)) {
            sourceDisplacementToUseThisRound += coll_bufsize;
          }

          /* For gpfsmpio_aggmethod of 1 do the mpi_put using the primitive MPI_BYTE type on each contiguous chunk of source data.
           */
          if (gpfsmpio_aggmethod == 1) {
#ifndef ACTIVE_TARGET
            MPI_Win_lock(MPI_LOCK_SHARED, sourceAggsForMyData[aggIter], 0, read_buf_window);
#endif
            if (bufTypeIsContig) {
              MPI_Get(((char*)buf) + recvBufferOffset,bufferAmountToRecv, MPI_BYTE,sourceAggsForMyData[aggIter],sourceDisplacementToUseThisRound, bufferAmountToRecv,MPI_BYTE,read_buf_window);
            }
            else {
              for (i=0;i<numNonContigSourceChunks;i++) {
                MPI_Get(((char*)buf) + nonContigSourceOffsets[i],nonContigSourceLens[i], MPI_BYTE,sourceAggsForMyData[aggIter],sourceDisplacementToUseThisRound, nonContigSourceLens[i],MPI_BYTE,read_buf_window);
#ifdef onesidedtrace
                printf("mpi_put[%d] nonContigSourceOffsets is %d of nonContigSourceLens %d to source disp %d\n",i,nonContigSourceOffsets[i],nonContigSourceLens[i],sourceDisplacementToUseThisRound);
#endif
                sourceDisplacementToUseThisRound += nonContigSourceLens[i];
              }
            }
#ifndef ACTIVE_TARGET
            MPI_Win_unlock(sourceAggsForMyData[aggIter], read_buf_window);
#endif

          }

          /* For gpfsmpio_aggmethod of 2 populate the data structures for this round/agg for this offset iter
           * to be used subsequently when building the derived type for 1 mpi_put for all the data for this
           * round/agg.
           */
          else if (gpfsmpio_aggmethod == 2) {

            if (bufTypeIsContig) {
              sourceAggBlockLengths[sourceAggContigAccessCount]= bufferAmountToRecv;
              sourceAggDataTypes[sourceAggContigAccessCount] = MPI_BYTE;
              sourceAggDisplacements[sourceAggContigAccessCount] = sourceDisplacementToUseThisRound;
              recvBufferDisplacements[sourceAggContigAccessCount] = (MPI_Aint)recvBufferOffset;
              sourceAggContigAccessCount++;
            }
            else {
              for (i=0;i<numNonContigSourceChunks;i++) {
                sourceAggBlockLengths[sourceAggContigAccessCount]= nonContigSourceLens[i];
                sourceAggDataTypes[sourceAggContigAccessCount] = MPI_BYTE;
                sourceAggDisplacements[sourceAggContigAccessCount] = sourceDisplacementToUseThisRound;
                recvBufferDisplacements[sourceAggContigAccessCount] = (MPI_Aint)nonContigSourceOffsets[i];
#ifdef onesidedtrace
                printf("mpi_put building arrays for iter %d - recvBufferDisplacements is %d of nonContigSourceLens %d to source disp %d sourceAggContigAccessCount is %d sourceAggBlockLengths[sourceAggContigAccessCount] is %d\n",i,recvBufferDisplacements[sourceAggContigAccessCount],nonContigSourceLens[i],sourceAggDisplacements[sourceAggContigAccessCount],sourceAggContigAccessCount, sourceAggBlockLengths[sourceAggContigAccessCount]);
#endif
                sourceAggContigAccessCount++;
                sourceDisplacementToUseThisRound += nonContigSourceLens[i];
              }
            }
          }
#ifdef onesidedtrace
        printf("roundIter %d bufferAmountToRecv is %d recvBufferOffset is %d offsetStart is %ld currentRoundFDStartForMySourceAgg is %ld sourceDisplacementToUseThisRound is %ld sourceAggsForMyDataFDStart[aggIter] is %ld\n",roundIter, bufferAmountToRecv,recvBufferOffset, offsetStart,currentRoundFDStartForMySourceAgg,sourceDisplacementToUseThisRound,sourceAggsForMyDataFDStart[aggIter]);
#endif

          } // bufferAmountToRecv > 0
      } // contig list

      /* For gpfsmpio_aggmethod of 2 now build the derived type using the data from this round/agg and do 1 single mpi_put.
       */
      if (gpfsmpio_aggmethod == 2) {
        MPI_Datatype recvBufferDerivedDataType, sourceBufferDerivedDataType;
        if (sourceAggContigAccessCount > 0) {
        /* Rebase source buffer offsets to 0 if there are any negative offsets for safety
         * when iteracting with PAMI.
         */
        MPI_Aint lowestDisplacement = 0;
        for (i=0;i<sourceAggContigAccessCount;i++) {
          if (recvBufferDisplacements[i] < lowestDisplacement)
            lowestDisplacement = recvBufferDisplacements[i];
        }
        if (lowestDisplacement  < 0) {
          lowestDisplacement *= -1;
          for (i=0;i<sourceAggContigAccessCount;i++)
           recvBufferDisplacements[i] += lowestDisplacement;
        }
        MPI_Type_create_struct(sourceAggContigAccessCount, sourceAggBlockLengths, recvBufferDisplacements, sourceAggDataTypes, &recvBufferDerivedDataType);
        MPI_Type_commit(&recvBufferDerivedDataType);
        MPI_Type_create_struct(sourceAggContigAccessCount, sourceAggBlockLengths, sourceAggDisplacements, sourceAggDataTypes, &sourceBufferDerivedDataType);
        MPI_Type_commit(&sourceBufferDerivedDataType);
#ifndef ACTIVE_TARGET
        MPI_Win_lock(MPI_LOCK_SHARED, sourceAggsForMyData[aggIter], 0, read_buf_window);
#endif
        MPI_Get((((char*)buf) - lowestDisplacement),1, recvBufferDerivedDataType,sourceAggsForMyData[aggIter],0, 1,sourceBufferDerivedDataType,read_buf_window);

#ifndef ACTIVE_TARGET
        MPI_Win_unlock(sourceAggsForMyData[aggIter], read_buf_window);
#endif
        }

        if (allocatedDerivedTypeArrays) {
          ADIOI_Free(sourceAggBlockLengths);
          ADIOI_Free(sourceAggDisplacements);
          ADIOI_Free(sourceAggDataTypes);
          ADIOI_Free(recvBufferDisplacements);
        }
        if (sourceAggContigAccessCount > 0) {
        MPI_Type_free(&recvBufferDerivedDataType);
        MPI_Type_free(&sourceBufferDerivedDataType);
        }
      }
      } // baseoffset != -1
    } // source aggs

    /* the source procs recv the requested data to the aggs */

#ifdef ACTIVE_TARGET
    MPI_Win_fence(0, read_buf_window);
#else
    MPI_Barrier(fd->comm);
#endif

    nextRoundFDStart = currentRoundFDStart + coll_bufsize;

    } /* for-loop roundIter */

    if (!bufTypeIsContig) {
      ADIOI_Free(nonContigSourceOffsets);
      ADIOI_Free(nonContigSourceLens);
    }
#ifdef ROMIO_GPFS
    endTimeBase = MPI_Wtime();
    gpfsmpio_prof_cw[GPFSMPIO_CIO_T_DEXCH] += (endTimeBase-startTimeBase);
#endif

    if (useIOBuffer) { /* thread readr cleanup */

    if ( !pthread_equal(io_thread, pthread_self()) ) {
        pthread_join(io_thread, &thread_ret);
        *error_code = *(int *)thread_ret;
    }

    }

    ADIOI_Free(sourceAggsForMyData);
    ADIOI_Free(sourceAggsForMyDataFDStart);
    ADIOI_Free(sourceAggsForMyDataFDEnd);

    for (i=0;i<numberOfRounds;i++) {
      ADIOI_Free(sourceAggsForMyDataFirstOffLenIndex[i]);
      ADIOI_Free(sourceAggsForMyDataLastOffLenIndex[i]);
      if (bufTypeIsContig)
        ADIOI_Free(baseRecvBufferOffset[i]);
      else
        ADIOI_Free(baseNonContigSourceBufferOffset[i]);
    }
    ADIOI_Free(sourceAggsForMyDataFirstOffLenIndex);
    ADIOI_Free(sourceAggsForMyDataLastOffLenIndex);
    if (bufTypeIsContig)
      ADIOI_Free(baseRecvBufferOffset);
    else
      ADIOI_Free(baseNonContigSourceBufferOffset);

    if (!bufTypeIsContig)
      ADIOI_Delete_flattened(datatype);
    return;
}
