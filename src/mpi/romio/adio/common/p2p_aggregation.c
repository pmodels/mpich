#include "adio.h"
#include "adio_extern.h"
#include "../ad_gpfs/ad_gpfs_tuning.h"

#include <pthread.h>

void ADIOI_P2PContigWriteAggregation(ADIO_File fd,
	const void *buf,
	int *error_code,
	ADIO_Offset *st_offsets,
	ADIO_Offset *end_offsets,
	ADIO_Offset *fd_start,
	ADIO_Offset* fd_end)
{

    *error_code = MPI_SUCCESS; // initialize to success

#ifdef ROMIO_GPFS
    double startTimeBase,endTimeBase;
#endif

    MPI_Status status;
    pthread_t io_thread;
    void *thread_ret;
    ADIOI_IO_ThreadFuncData io_thread_args;

    int nprocs,myrank;
    MPI_Comm_size(fd->comm, &nprocs);
    MPI_Comm_rank(fd->comm, &myrank);

    int myAggRank = -1; // if I am an aggregor this is my index into fd->hints->ranklist
    int iAmUsedAgg = 0;

#ifdef ROMIO_GPFS
    startTimeBase = MPI_Wtime();
#endif

    int naggs = fd->hints->cb_nodes;
    int coll_bufsize = fd->hints->cb_buffer_size;
#ifdef ROMIO_GPFS
    if (gpfsmpio_pthreadio == 1) {
	/* split buffer in half for a kind of double buffering with the threads*/
	coll_bufsize = fd->hints->cb_buffer_size/2;
    }
#endif

    int j;
    for (j=0;j<naggs;j++) {
	if (fd->hints->ranklist[j] == myrank) {
	    myAggRank = j;
	    if (fd_end[j] > fd_start[j]) {
		iAmUsedAgg = 1;
	    }
	}
    }

    /* determine how much data and to whom I need to send (also record the
     * offset in the buffer for procs that span file domains) */
    int *targetAggsForMyData = (int *)ADIOI_Malloc(naggs * sizeof(int));
    int *dataSizeToSendPerTargetAgg = (int *)ADIOI_Malloc(naggs * sizeof(int));
    int *bufferOffsetToSendPerTargetAgg = (int *)ADIOI_Malloc(naggs * sizeof(int));
    int numTargetAggs = 0;
    int i;
    for (i=0;i<naggs;i++) {
	if ( ((st_offsets[myrank] >= fd_start[i]) &&  (st_offsets[myrank] <= fd_end[i])) || ((end_offsets[myrank] >= fd_start[i]) &&  (end_offsets[myrank] <= fd_end[i]))) {
	    targetAggsForMyData[numTargetAggs] = fd->hints->ranklist[i];
	    if ( ((st_offsets[myrank] >= fd_start[i]) &&  (st_offsets[myrank] <= fd_end[i])) && ((end_offsets[myrank] >= fd_start[i]) &&  (end_offsets[myrank] <= fd_end[i]))) {
		dataSizeToSendPerTargetAgg[numTargetAggs] = (end_offsets[myrank] - st_offsets[myrank])+1;
		bufferOffsetToSendPerTargetAgg[numTargetAggs] = 0;
	    }
	    else if ((st_offsets[myrank] >= fd_start[i]) &&  (st_offsets[myrank] <= fd_end[i])) { // starts in this fd and goes past it
		dataSizeToSendPerTargetAgg[numTargetAggs] = (fd_end[i] - st_offsets[myrank]) +1;
		bufferOffsetToSendPerTargetAgg[numTargetAggs] = 0;
	    }
	    else { // starts in fd before this and ends in it
		dataSizeToSendPerTargetAgg[numTargetAggs] = (end_offsets[myrank] - fd_start[i]) +1;
		bufferOffsetToSendPerTargetAgg[numTargetAggs] = fd_start[i]- st_offsets[myrank];
	    }
	    numTargetAggs++;
	}
    }

    /* these 3 arrays track info on the procs that feed an aggregtor */
    int *sourceProcsForMyData=NULL;
    int *remainingDataAmountToGetPerProc=NULL;
    ADIO_Offset *remainingDataOffsetToGetPerProc=NULL;

    int numSourceProcs = 0;
    int totalDataSizeToGet = 0;

    if (iAmUsedAgg) { /* for the used aggregators figure out how much data I
			 need from what procs */

	// count numSourceProcs so we know how large to make the arrays
	for (i=0;i<nprocs;i++)
	    if ( ((st_offsets[i] >= fd_start[myAggRank]) &&  (st_offsets[i] <= fd_end[myAggRank])) || ((end_offsets[i] >= fd_start[myAggRank]) &&  (end_offsets[i] <= fd_end[myAggRank])))
		numSourceProcs++;

	sourceProcsForMyData = (int *)ADIOI_Malloc(numSourceProcs * sizeof(int));
	remainingDataAmountToGetPerProc = (int *)ADIOI_Malloc(numSourceProcs * sizeof(int));
	remainingDataOffsetToGetPerProc = (ADIO_Offset *)ADIOI_Malloc(numSourceProcs * sizeof(ADIO_Offset));

	/* TODO: here was a spot where the balancecontig code figured out bridge ranks */

	/* everybody has the st_offsets and end_offsets for all ranks so if I am a
	 * used aggregator go thru them and figure out which ranks have data that
	 * falls into my file domain assigned to me */
	numSourceProcs = 0;
	for (i=0;i<nprocs;i++) {
	    if ( ((st_offsets[i] >= fd_start[myAggRank]) &&  (st_offsets[i] <= fd_end[myAggRank])) || ((end_offsets[i] >= fd_start[myAggRank]) &&  (end_offsets[i] <= fd_end[myAggRank]))) {
		sourceProcsForMyData[numSourceProcs] = i;
		if ( ((st_offsets[i] >= fd_start[myAggRank]) &&  (st_offsets[i] <= fd_end[myAggRank])) && ((end_offsets[i] >= fd_start[myAggRank]) &&  (end_offsets[i] <= fd_end[myAggRank]))) {
		    remainingDataAmountToGetPerProc[numSourceProcs] = (end_offsets[i] - st_offsets[i])+1;
		    remainingDataOffsetToGetPerProc[numSourceProcs] = st_offsets[i];
		}
		else if ((st_offsets[i] >= fd_start[myAggRank]) &&  (st_offsets[i] <= fd_end[myAggRank])) {// starts in this fd and goes past it
		    remainingDataAmountToGetPerProc[numSourceProcs] = (fd_end[myAggRank] - st_offsets[i]) +1;
		    remainingDataOffsetToGetPerProc[numSourceProcs] = st_offsets[i];
		}
		else { // starts in fd before this and ends in it
		    remainingDataAmountToGetPerProc[numSourceProcs] = (end_offsets[i] - fd_start[myAggRank]) +1;
		    remainingDataOffsetToGetPerProc[numSourceProcs] = fd_start[myAggRank];
		}
		totalDataSizeToGet += remainingDataAmountToGetPerProc[numSourceProcs];
#ifdef p2pcontigtrace
		printf("getting %ld bytes from source proc %d in fd %d with borders %ld to %ld\n",remainingDataAmountToGetPerProc[numSourceProcs],fd->hints->ranklist[myAggRank],i,fd_start[myAggRank],fd_end[myAggRank]);
#endif
		numSourceProcs++;
	    }
	}
    }

    int *amountOfDataReqestedByTargetAgg = (int *)ADIOI_Malloc(naggs * sizeof(int));
    for (i=0;i<numTargetAggs;i++) {
	amountOfDataReqestedByTargetAgg[i] = 0;
#ifdef p2pcontigtrace
	printf("Need to send %ld bytes at buffer offset %d to agg %d\n", dataSizeToSendPerTargetAgg[i],bufferOffsetToSendPerTargetAgg[i],targetAggsForMyData[i]);
#endif
    }

    int totalAmountDataSent = 0;
    int totalAmountDataReceived = 0;
    MPI_Request *mpiSizeToSendRequest = (MPI_Request *) ADIOI_Malloc(numTargetAggs * sizeof(MPI_Request));
    MPI_Request *mpiRecvDataRequest = (MPI_Request *) ADIOI_Malloc(numSourceProcs * sizeof(MPI_Request));
    MPI_Request *mpiSendDataSizeRequest = (MPI_Request *) ADIOI_Malloc(numSourceProcs * sizeof(MPI_Request));

    MPI_Request *mpiSendDataToTargetAggRequest = (MPI_Request *) ADIOI_Malloc(numTargetAggs * sizeof(MPI_Request));
    MPI_Status mpiWaitAnyStatusFromTargetAggs,mpiWaitAnyStatusFromSourceProcs;
    MPI_Status mpiIsendStatusForSize,  mpiIsendStatusForData;

    // use the write buffer allocated in the file_open
    char *write_buf0 = fd->io_buf;
    char *write_buf1 = fd->io_buf + coll_bufsize;

    /* start off pointing to the first buffer. If we use the 2nd buffer (threaded
     * case) we'll swap later */
    char *write_buf = write_buf0;

    // compute number of rounds
    ADIO_Offset numberOfRounds = (ADIO_Offset)((((ADIO_Offset)(end_offsets[nprocs-1]-st_offsets[0]))/((ADIO_Offset)((ADIO_Offset)coll_bufsize*(ADIO_Offset)naggs)))) + 1;

    int currentWriteBuf = 0;
    int useIOBuffer = 0;
#ifdef ROMIO_GPFS
    if (gpfsmpio_pthreadio && (numberOfRounds>1)) {
	useIOBuffer = 1;
	io_thread = pthread_self();
    }
#endif

    ADIO_Offset currentRoundFDStart = 0;
    ADIO_Offset currentRoundFDEnd = 0;

    if (iAmUsedAgg) {
	currentRoundFDStart = fd_start[myAggRank];
    }

    int *dataSizeGottenThisRoundPerProc = (int *)ADIOI_Malloc(numSourceProcs * sizeof(int));
    int *mpiRequestMapPerProc = (int *)ADIOI_Malloc(numSourceProcs * sizeof(int));
    int *mpiSendRequestMapPerProc = (int *)ADIOI_Malloc(numTargetAggs * sizeof(int));

#ifdef ROMIO_GPFS
    endTimeBase = MPI_Wtime();
    gpfsmpio_prof_cw[GPFSMPIO_CIO_T_MYREQ] += (endTimeBase-startTimeBase);
    startTimeBase = MPI_Wtime();
#endif

    /* each iteration of this loop writes a coll_bufsize portion of the file
     * domain */
    int roundIter;
    for (roundIter=0;roundIter<numberOfRounds;roundIter++) {

	// determine what offsets define the portion of the file domain the agg is writing this round
	if (iAmUsedAgg) {
	    if ((fd_end[myAggRank] - currentRoundFDStart) < coll_bufsize) {
		currentRoundFDEnd = fd_end[myAggRank];
	    }
	    else
		currentRoundFDEnd = currentRoundFDStart + coll_bufsize - 1;
#ifdef p2pcontigtrace
	    printf("currentRoundFDStart is %ld currentRoundFDEnd is %ld within file domeain %ld to %ld\n",currentRoundFDStart,currentRoundFDEnd,fd_start[myAggRank],fd_end[myAggRank]);
#endif
	}

	int numRecvToWaitFor = 0;
	int irecv,isend;

	/* the source procs receive the amount of data the aggs want them to send */
#ifdef ROMIO_GPFS
	startTimeBase = MPI_Wtime();
#endif
	for (i=0;i<numTargetAggs;i++) {
	    MPI_Irecv(&amountOfDataReqestedByTargetAgg[i],1,
		    MPI_INT,targetAggsForMyData[i],0,
		    fd->comm,&mpiSizeToSendRequest[i]);
	    numRecvToWaitFor++;
#ifdef p2pcontigtrace
	    printf("MPI_Irecv from rank %d\n",targetAggsForMyData[i]);
#endif
	}

	// the aggs send the amount of data they need to their source procs
	for (i=0;i<numSourceProcs;i++) {
	    if ((remainingDataOffsetToGetPerProc[i] >= currentRoundFDStart) && (remainingDataOffsetToGetPerProc[i] <= currentRoundFDEnd)) {
		if ((remainingDataOffsetToGetPerProc[i] + remainingDataAmountToGetPerProc[i]) <= currentRoundFDEnd)
		    dataSizeGottenThisRoundPerProc[i] = remainingDataAmountToGetPerProc[i];
		else
		    dataSizeGottenThisRoundPerProc[i] = (currentRoundFDEnd - remainingDataOffsetToGetPerProc[i]) +1;
	    }
	    else if (((remainingDataOffsetToGetPerProc[i]+remainingDataAmountToGetPerProc[i]) >= currentRoundFDStart) && ((remainingDataOffsetToGetPerProc[i]+remainingDataAmountToGetPerProc[i]) <= currentRoundFDEnd)) {
		if ((remainingDataOffsetToGetPerProc[i]) >= currentRoundFDStart)
		    dataSizeGottenThisRoundPerProc[i] = remainingDataAmountToGetPerProc[i];
		else
		    dataSizeGottenThisRoundPerProc[i] = (remainingDataOffsetToGetPerProc[i]-currentRoundFDStart) +1;
	    }
	    else
		dataSizeGottenThisRoundPerProc[i] = 0;

#ifdef p2pcontigtrace
	    printf("dataSizeGottenThisRoundPerProc[%d] set to %d - remainingDataOffsetToGetPerProc is %d remainingDataAmountToGetPerProc is %d currentRoundFDStart is %d currentRoundFDEnd is %d\n",i,dataSizeGottenThisRoundPerProc[i],remainingDataOffsetToGetPerProc[i],remainingDataAmountToGetPerProc[i],currentRoundFDStart,currentRoundFDEnd);
#endif
	    MPI_Isend(&dataSizeGottenThisRoundPerProc[i],1,MPI_INT,
		    sourceProcsForMyData[i],0,fd->comm,&mpiSendDataSizeRequest[i]);

	}

        int numDataSendToWaitFor = 0;
	/* the source procs send the requested data to the aggs - only send if
	 * requested more than 0 bytes */
	for (i = 0; i < numRecvToWaitFor; i++) {
	    MPI_Waitany(numRecvToWaitFor,mpiSizeToSendRequest,&irecv,&mpiWaitAnyStatusFromTargetAggs);

#ifdef p2pcontigtrace
	    printf("was sent request for %d bytes from rank %d irecv index %d\n",amountOfDataReqestedByTargetAgg[irecv],targetAggsForMyData[irecv],irecv);
#endif

	    if (amountOfDataReqestedByTargetAgg[irecv] > 0) {
		MPI_Isend(&((char*)buf)[bufferOffsetToSendPerTargetAgg[irecv]],amountOfDataReqestedByTargetAgg[irecv],MPI_BYTE,
			targetAggsForMyData[irecv],0,fd->comm,&mpiSendDataToTargetAggRequest[irecv]);
		totalAmountDataSent += amountOfDataReqestedByTargetAgg[irecv];
		bufferOffsetToSendPerTargetAgg[irecv] += amountOfDataReqestedByTargetAgg[irecv];
                mpiSendRequestMapPerProc[numDataSendToWaitFor] = irecv;
                numDataSendToWaitFor++;
	    }

	}

#ifdef ROMIO_GPFS
	gpfsmpio_prof_cw[GPFSMPIO_CIO_T_DEXCH_SETUP] += (endTimeBase-startTimeBase);
	startTimeBase = MPI_Wtime();
#endif

	// the aggs receive the data from the source procs
	int numDataRecvToWaitFor = 0;
	for (i=0;i<numSourceProcs;i++) {

	    int currentWBOffset = 0;
	    for (j=0;j<i;j++)
		currentWBOffset += dataSizeGottenThisRoundPerProc[j];

	    // only receive from source procs that will send > 0 count data
	    if (dataSizeGottenThisRoundPerProc[i] > 0) {
#ifdef p2pcontigtrace
		printf("receiving data from rank %d dataSizeGottenThisRoundPerProc is %d currentWBOffset is %d\n",sourceProcsForMyData[i],dataSizeGottenThisRoundPerProc[i],currentWBOffset);
#endif
		MPI_Irecv(&((char*)write_buf)[currentWBOffset],dataSizeGottenThisRoundPerProc[i],
			MPI_BYTE,sourceProcsForMyData[i],0,
			fd->comm,&mpiRecvDataRequest[numDataRecvToWaitFor]);
		mpiRequestMapPerProc[numDataRecvToWaitFor] = i;
		numDataRecvToWaitFor++;
	    }

#ifdef p2pcontigtrace
	    printf("MPI_Irecv from rank %d\n",targetAggsForMyData[i]);
#endif
	}

	int totalDataReceivedThisRound = 0;
	for (i = 0; i < numDataRecvToWaitFor; i++) {
	    MPI_Waitany(numDataRecvToWaitFor,mpiRecvDataRequest,
		    &irecv,&mpiWaitAnyStatusFromSourceProcs);
	    totalDataReceivedThisRound +=
		dataSizeGottenThisRoundPerProc[mpiRequestMapPerProc[irecv]];
	    totalAmountDataReceived +=
		dataSizeGottenThisRoundPerProc[mpiRequestMapPerProc[irecv]];

#ifdef p2pcontigtrace
	    printf("numDataRecvToWaitFor is %d was sent %d bytes data for %d remaining bytes from rank %d irecv index %d\n",numDataRecvToWaitFor,dataSizeGottenThisRoundPerProc[mpiRequestMapPerProc[irecv]],remainingDataAmountToGetPerProc[mpiRequestMapPerProc[irecv]],sourceProcsForMyData[mpiRequestMapPerProc[irecv]],irecv);
#endif
	    remainingDataAmountToGetPerProc[mpiRequestMapPerProc[irecv]] -=
		dataSizeGottenThisRoundPerProc[mpiRequestMapPerProc[irecv]];
	    remainingDataOffsetToGetPerProc[mpiRequestMapPerProc[irecv]] +=
		dataSizeGottenThisRoundPerProc[mpiRequestMapPerProc[irecv]];

	}

	/* clean up the MPI_Request object for the MPI_Isend which told the
	 * source procs how much data to send */
        for (i=0;i<numSourceProcs;i++) {
           MPI_Waitany(numSourceProcs,mpiSendDataSizeRequest,
		   &isend,&mpiIsendStatusForSize);
        }


#ifdef ROMIO_GPFS
        endTimeBase = MPI_Wtime();
	gpfsmpio_prof_cw[GPFSMPIO_CIO_T_DEXCH_NET] += (endTimeBase-startTimeBase);
#endif
	// the aggs now write the data
	if (numDataRecvToWaitFor > 0) {

#ifdef p2pcontigtrace
	    printf("totalDataReceivedThisRound is %d\n",totalDataReceivedThisRound);
#endif
	    if (!useIOBuffer) {

		ADIO_WriteContig(fd, write_buf, (int)totalDataReceivedThisRound,
			MPI_BYTE, ADIO_EXPLICIT_OFFSET,
			currentRoundFDStart, &status, error_code);
	    } else { // use the thread writer

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
		io_thread_args.io_kind = ADIOI_WRITE;
		io_thread_args.size = totalDataReceivedThisRound;
		io_thread_args.offset = currentRoundFDStart;
		io_thread_args.status = status;
		io_thread_args.error_code = *error_code;
		if ( (pthread_create(&io_thread, NULL,
				ADIOI_IO_Thread_Func, &(io_thread_args))) != 0)
		    io_thread = pthread_self();

	    }

	} // numDataRecvToWaitFor > 0

	if (iAmUsedAgg)
	    currentRoundFDStart += coll_bufsize;
        for (i = 0; i < numDataSendToWaitFor; i++) {
          MPI_Wait(&mpiSendDataToTargetAggRequest[mpiSendRequestMapPerProc[i]],
		  &mpiIsendStatusForData);
        }

    } // for-loop roundIter

#ifdef ROMIO_GPFS
    endTimeBase = MPI_Wtime();
    gpfsmpio_prof_cw[GPFSMPIO_CIO_T_DEXCH] += (endTimeBase-startTimeBase);
#endif

    if (useIOBuffer) { // thread writer cleanup

	if ( !pthread_equal(io_thread, pthread_self()) ) {
	    pthread_join(io_thread, &thread_ret);
	    *error_code = *(int *)thread_ret;
	}

    }



    if (iAmUsedAgg) {
	ADIOI_Free(sourceProcsForMyData);
	ADIOI_Free(remainingDataAmountToGetPerProc);
	ADIOI_Free(remainingDataOffsetToGetPerProc);
    }

    ADIOI_Free(targetAggsForMyData);
    ADIOI_Free(dataSizeToSendPerTargetAgg);
    ADIOI_Free(bufferOffsetToSendPerTargetAgg);
    ADIOI_Free(amountOfDataReqestedByTargetAgg);
    ADIOI_Free(mpiSizeToSendRequest);
    ADIOI_Free(mpiRecvDataRequest);
    ADIOI_Free(mpiSendDataSizeRequest);
    ADIOI_Free(mpiSendDataToTargetAggRequest);
    ADIOI_Free(dataSizeGottenThisRoundPerProc);
    ADIOI_Free(mpiRequestMapPerProc);
    ADIOI_Free(mpiSendRequestMapPerProc);

    /* TODO: still need a barrier here? */
    MPI_Barrier(fd->comm);
    return;
}

void ADIOI_P2PContigReadAggregation(ADIO_File fd,
	const void *buf,
	int *error_code,
	ADIO_Offset *st_offsets,
	ADIO_Offset *end_offsets,
	ADIO_Offset *fd_start,
	ADIO_Offset* fd_end)
{

    *error_code = MPI_SUCCESS; // initialize to success

#ifdef ROMIO_GPFS
    double startTimeBase,endTimeBase;
#endif

    MPI_Status status;
    pthread_t io_thread;
    void *thread_ret;
    ADIOI_IO_ThreadFuncData io_thread_args;

#ifdef ROMIO_GPFS
    startTimeBase = MPI_Wtime();
#endif

    int nprocs,myrank;
    MPI_Comm_size(fd->comm, &nprocs);
    MPI_Comm_rank(fd->comm, &myrank);

    int myAggRank = -1; // if I am an aggregor this is my index into fd->hints->ranklist
    int iAmUsedAgg = 0;

    int naggs = fd->hints->cb_nodes;
    int coll_bufsize = fd->hints->cb_buffer_size;
#ifdef ROMIO_GPFS
    if (gpfsmpio_pthreadio == 1)
	/* share buffer between working threads */
	coll_bufsize = coll_bufsize/2;
#endif

    int j;
    for (j=0;j<naggs;j++) {
	if (fd->hints->ranklist[j] == myrank) {
	    myAggRank = j;
	    if (fd_end[j] > fd_start[j]) {
		iAmUsedAgg = 1;
	    }
	}
    }

    // for my offset range determine how much data and from whom I need to get (also record the offset in the buffer for procs that span file domains)
    int *sourceAggsForMyData = (int *)ADIOI_Malloc(naggs * sizeof(int));
    int *dataSizeToGetPerSourceAgg = (int *)ADIOI_Malloc(naggs * sizeof(int));
    int *bufferOffsetToGetPerSourceAgg = (int *)ADIOI_Malloc(naggs * sizeof(int));
    int numSourceAggs = 0;
    int i;
    for (i=0;i<naggs;i++) {
	if ( ((st_offsets[myrank] >= fd_start[i]) &&  (st_offsets[myrank] <= fd_end[i])) || ((end_offsets[myrank] >= fd_start[i]) &&  (end_offsets[myrank] <= fd_end[i]))) {
	    sourceAggsForMyData[numSourceAggs] = fd->hints->ranklist[i];
	    if ( ((st_offsets[myrank] >= fd_start[i]) &&  (st_offsets[myrank] <= fd_end[i])) && ((end_offsets[myrank] >= fd_start[i]) &&  (end_offsets[myrank] <= fd_end[i]))) {
		dataSizeToGetPerSourceAgg[numSourceAggs] = (end_offsets[myrank] - st_offsets[myrank])+1;
		bufferOffsetToGetPerSourceAgg[numSourceAggs] = 0;
	    }
	    else if ((st_offsets[myrank] >= fd_start[i]) &&  (st_offsets[myrank] <= fd_end[i])) { // starts in this fd and goes past it
		dataSizeToGetPerSourceAgg[numSourceAggs] = (fd_end[i] - st_offsets[myrank]) +1;
		bufferOffsetToGetPerSourceAgg[numSourceAggs] = 0;
	    }
	    else { // starts in fd before this and ends in it
		dataSizeToGetPerSourceAgg[numSourceAggs] = (end_offsets[myrank] - fd_start[i]) +1;
		bufferOffsetToGetPerSourceAgg[numSourceAggs] = fd_start[i]- st_offsets[myrank];
	    }
	    numSourceAggs++;
	}
    }

    /* these 3 arrays track info on the procs that are fed from an aggregtor -
     * to sacrifice some performance at setup to save on memory instead of
     * using max size of nprocs for the arrays could determine exact size first
     * and then allocate that size */
    int *targetProcsForMyData=NULL;
    int *remainingDataAmountToSendPerProc=NULL;
    ADIO_Offset *remainingDataOffsetToSendPerProc=NULL;

    int numTargetProcs = 0;
    int totalDataSizeToSend = 0;

    if (iAmUsedAgg) {
	/* for the used aggregators figure out how much data I need from what procs */

	/* count numTargetProcs so we know how large to make the arrays */
	for (i=0;i<nprocs;i++)
	    if ( ((st_offsets[i] >= fd_start[myAggRank]) &&
			(st_offsets[i] <= fd_end[myAggRank])) ||
		    ((end_offsets[i] >= fd_start[myAggRank]) &&
		     (end_offsets[i] <= fd_end[myAggRank]))  )
		numTargetProcs++;

	targetProcsForMyData =
	    (int *)ADIOI_Malloc(numTargetProcs * sizeof(int));
	remainingDataAmountToSendPerProc =
	    (int *)ADIOI_Malloc(numTargetProcs * sizeof(int));
	remainingDataOffsetToSendPerProc =
	    (ADIO_Offset *)ADIOI_Malloc(numTargetProcs * sizeof(ADIO_Offset));

	/* TODO: some balancecontig logic might need to go here */

	/* everybody has the st_offsets and end_offsets for all ranks so if I am a
	 * used aggregator go thru them and figure out which ranks have data that
	 * falls into my file domain assigned to me */
	numTargetProcs = 0;
	for (i=0;i<nprocs;i++) {
	    if ( ((st_offsets[i] >= fd_start[myAggRank]) &&  (st_offsets[i] <= fd_end[myAggRank])) || ((end_offsets[i] >= fd_start[myAggRank]) &&  (end_offsets[i] <= fd_end[myAggRank]))) {
		targetProcsForMyData[numTargetProcs] = i;
		if ( ((st_offsets[i] >= fd_start[myAggRank]) &&  (st_offsets[i] <= fd_end[myAggRank])) && ((end_offsets[i] >= fd_start[myAggRank]) &&  (end_offsets[i] <= fd_end[myAggRank]))) {
		    remainingDataAmountToSendPerProc[numTargetProcs] = (end_offsets[i] - st_offsets[i])+1;
		    remainingDataOffsetToSendPerProc[numTargetProcs] = st_offsets[i];
		}
		else if ((st_offsets[i] >= fd_start[myAggRank]) &&  (st_offsets[i] <= fd_end[myAggRank])) {// starts in this fd and goes past it
		    remainingDataAmountToSendPerProc[numTargetProcs] = (fd_end[myAggRank] - st_offsets[i]) +1;
		    remainingDataOffsetToSendPerProc[numTargetProcs] = st_offsets[i];
		}
		else { // starts in fd before this and ends in it
		    remainingDataAmountToSendPerProc[numTargetProcs] = (end_offsets[i] - fd_start[myAggRank]) +1;
		    remainingDataOffsetToSendPerProc[numTargetProcs] = fd_start[myAggRank];
		}
		totalDataSizeToSend += remainingDataAmountToSendPerProc[numTargetProcs];
		numTargetProcs++;
	    }
	}
    }

    int *amountOfDataReqestedFromSourceAgg = (int *)ADIOI_Malloc(naggs * sizeof(int));
    for (i=0;i<numSourceAggs;i++) {
	amountOfDataReqestedFromSourceAgg[i] = 0;
    }


    int totalAmountDataSent = 0;
    MPI_Request *mpiSizeToSendRequest = (MPI_Request *) ADIOI_Malloc(numSourceAggs * sizeof(MPI_Request));
    MPI_Request *mpiRecvDataFromSourceAggsRequest = (MPI_Request *) ADIOI_Malloc(numSourceAggs * sizeof(MPI_Request));
    MPI_Request *mpiSendDataSizeRequest = (MPI_Request *) ADIOI_Malloc(numTargetProcs * sizeof(MPI_Request));

    MPI_Request *mpiSendDataToTargetProcRequest = (MPI_Request *) ADIOI_Malloc(numTargetProcs * sizeof(MPI_Request));
    MPI_Status mpiWaitAnyStatusFromTargetAggs,mpiWaitAnyStatusFromSourceProcs,mpiIsendStatusForSize,mpiIsendStatusForData;

    /* use the two-phase buffer allocated in the file_open - no app should ever
     * be both reading and writing at the same time */
    char *read_buf0 = fd->io_buf;
    char *read_buf1 = fd->io_buf + coll_bufsize;
    /* if threaded i/o selected, we'll do a kind of double buffering */
    char *read_buf = read_buf0;

    // compute number of rounds
    ADIO_Offset numberOfRounds = (ADIO_Offset)((((ADIO_Offset)(end_offsets[nprocs-1]-st_offsets[0]))/((ADIO_Offset)((ADIO_Offset)coll_bufsize*(ADIO_Offset)naggs)))) + 1;

#ifdef p2pcontigtrace
    printf("need to send %d bytes - coll_bufsize is %d\n",totalDataSizeToSend,coll_bufsize);
#endif

    ADIO_Offset currentRoundFDStart = 0, nextRoundFDStart = 0;
    ADIO_Offset currentRoundFDEnd = 0, nextRoundFDEnd = 0;

    if (iAmUsedAgg) {
	currentRoundFDStart = fd_start[myAggRank];
	nextRoundFDStart = fd_start[myAggRank];
    }

    int *dataSizeSentThisRoundPerProc = (int *)ADIOI_Malloc(numTargetProcs * sizeof(int));
    *error_code = MPI_SUCCESS;

    int currentReadBuf = 0;
    int useIOBuffer = 0;
#ifdef ROMIO_GPFS
    if (gpfsmpio_pthreadio && (numberOfRounds>1)) {
	useIOBuffer = 1;
	io_thread = pthread_self();
    }
#endif

#ifdef ROMIO_GPFS
    endTimeBase = MPI_Wtime();
    gpfsmpio_prof_cw[GPFSMPIO_CIO_T_MYREQ] += (endTimeBase-startTimeBase);
#endif


    // each iteration of this loop reads a coll_bufsize portion of the file domain
    int roundIter;
    for (roundIter=0;roundIter<numberOfRounds;roundIter++) {

	int irecv,isend;
	// determine what offsets define the portion of the file domain the agg is reading this round
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

		// read currentRoundFDEnd bytes
		ADIO_ReadContig(fd, read_buf,amountDataToReadThisRound,
			MPI_BYTE, ADIO_EXPLICIT_OFFSET, currentRoundFDStart,
			&status, error_code);

#ifdef ROMIO_GPFS
		endTimeBase = MPI_Wtime();
#endif
	    }

	    if (useIOBuffer) { // use the thread reader for the next round
		// switch back and forth between the read buffers so that the data aggregation code is diseminating 1 buffer while the thread is reading into the other

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
		else { // last round

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
	    } // useIOBuffer
	} // IAmUsedAgg

	/* the source procs receive the amount of data the aggs will be sending them */
	for (i=0;i<numSourceAggs;i++) {
	    MPI_Irecv(&amountOfDataReqestedFromSourceAgg[i],1,
		    MPI_INT,sourceAggsForMyData[i],0,
		    fd->comm,&mpiSizeToSendRequest[i]);
	}

	// the aggs send the amount of data they will be sending to their source procs
	for (i=0;i<numTargetProcs;i++) {
	    if ((remainingDataOffsetToSendPerProc[i] >= currentRoundFDStart) &&
		    (remainingDataOffsetToSendPerProc[i] <= currentRoundFDEnd)) {
		if ((remainingDataOffsetToSendPerProc[i] +
			    remainingDataAmountToSendPerProc[i]) <= currentRoundFDEnd)
		    dataSizeSentThisRoundPerProc[i] = remainingDataAmountToSendPerProc[i];
		else
		    dataSizeSentThisRoundPerProc[i] =
			(currentRoundFDEnd - remainingDataOffsetToSendPerProc[i]) +1;
	    }
	    else if (((remainingDataOffsetToSendPerProc[i]+
			    remainingDataAmountToSendPerProc[i]) >=
			currentRoundFDStart) &&
		    ((remainingDataOffsetToSendPerProc[i]+
		      remainingDataAmountToSendPerProc[i]) <= currentRoundFDEnd)) {
		if ((remainingDataOffsetToSendPerProc[i]) >= currentRoundFDStart)
		    dataSizeSentThisRoundPerProc[i] = remainingDataAmountToSendPerProc[i];
		else
		    dataSizeSentThisRoundPerProc[i] =
			(remainingDataOffsetToSendPerProc[i]-currentRoundFDStart) +1;
	    }
	    else
		dataSizeSentThisRoundPerProc[i] = 0;

	    MPI_Isend(&dataSizeSentThisRoundPerProc[i],1,MPI_INT,
		    targetProcsForMyData[i],0,fd->comm,&mpiSendDataSizeRequest[i]);

	}

	/* the source procs get the requested data amount from the aggs and then
	 * receive that amount of data - only recv if requested more than 0 bytes */
	int numDataRecvToWaitFor = 0;
	for (i = 0; i < numSourceAggs; i++) {
	    MPI_Waitany(numSourceAggs,mpiSizeToSendRequest,
		    &irecv,&mpiWaitAnyStatusFromTargetAggs);
	    if (amountOfDataReqestedFromSourceAgg[irecv] > 0) {

		MPI_Irecv(&((char*)buf)[bufferOffsetToGetPerSourceAgg[irecv]],
			amountOfDataReqestedFromSourceAgg[irecv],MPI_BYTE,
			sourceAggsForMyData[irecv],0,fd->comm,
			&mpiRecvDataFromSourceAggsRequest[numDataRecvToWaitFor]);
		totalAmountDataSent += amountOfDataReqestedFromSourceAgg[irecv];
		bufferOffsetToGetPerSourceAgg[irecv] += amountOfDataReqestedFromSourceAgg[irecv];

		numDataRecvToWaitFor++;
	    }
	}

	// the aggs send the data to the source procs
	for (i=0;i<numTargetProcs;i++) {

	    int currentWBOffset = 0;
	    for (j=0;j<i;j++)
		currentWBOffset += dataSizeSentThisRoundPerProc[j];

	    // only send to target procs that will recv > 0 count data
	    if (dataSizeSentThisRoundPerProc[i] > 0) {
		MPI_Isend(&((char*)read_buf)[currentWBOffset],
			dataSizeSentThisRoundPerProc[i],
			MPI_BYTE,targetProcsForMyData[i],0,
			fd->comm,&mpiSendDataToTargetProcRequest[i]);
		remainingDataAmountToSendPerProc[i] -= dataSizeSentThisRoundPerProc[i];
		remainingDataOffsetToSendPerProc[i] += dataSizeSentThisRoundPerProc[i];
	    }
	}

	// wait for the target procs to get their data
	for (i = 0; i < numDataRecvToWaitFor; i++) {
	    MPI_Waitany(numDataRecvToWaitFor,mpiRecvDataFromSourceAggsRequest,
		    &irecv,&mpiWaitAnyStatusFromSourceProcs);
	}

	nextRoundFDStart = currentRoundFDStart + coll_bufsize;


        // clean up the MPI_Isend MPI_Requests
        for (i=0;i<numTargetProcs;i++) {
          MPI_Waitany(numTargetProcs,mpiSendDataSizeRequest,
		  &isend,&mpiIsendStatusForSize);
          if (dataSizeSentThisRoundPerProc[isend] > 0) {
            MPI_Wait(&mpiSendDataToTargetProcRequest[isend],&mpiIsendStatusForData);
          }
        }

	MPI_Barrier(fd->comm); // need to sync up the source aggs which did the isend with the target procs which did the irecvs to give the target procs time to get the data before overwriting with next round readcontig

    } // for-loop roundIter

    if (useIOBuffer) { // thread reader cleanup

	if ( !pthread_equal(io_thread, pthread_self()) ) {
	    pthread_join(io_thread, &thread_ret);
	    *error_code = *(int *)thread_ret;
	}
    }

    if (iAmUsedAgg) {
	ADIOI_Free(targetProcsForMyData);
	ADIOI_Free(remainingDataAmountToSendPerProc);
	ADIOI_Free(remainingDataOffsetToSendPerProc);
    }

    ADIOI_Free(sourceAggsForMyData);
    ADIOI_Free(dataSizeToGetPerSourceAgg);
    ADIOI_Free(bufferOffsetToGetPerSourceAgg);
    ADIOI_Free(amountOfDataReqestedFromSourceAgg);
    ADIOI_Free(mpiSizeToSendRequest);
    ADIOI_Free(mpiRecvDataFromSourceAggsRequest);
    ADIOI_Free(mpiSendDataSizeRequest);
    ADIOI_Free(mpiSendDataToTargetProcRequest);
    ADIOI_Free(dataSizeSentThisRoundPerProc);

    /* TODO: is Barrier here needed? */
    MPI_Barrier(fd->comm);

    return;

}
