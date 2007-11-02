/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifdef HAVE_WINDOWS_H
#include <winsock2.h>
#include <windows.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include "mpi.h"
#include "GetOpt.h"
#include <string.h>

#ifndef BOOL
typedef int BOOL;
#endif
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#define  TRIALS 	3
#define  REPEAT 	1000
int 	 g_NSAMP =	250;
#define  PERT		3
/*#define  LATENCYREPS	1000*/
int      g_LATENCYREPS = 1000;
#define  LONGTIME	1e99
#define  CHARSIZE	8
#define  PATIENCE	50
#define  RUNTM		0.25
double	 g_STOPTM = 	0.1;
#define  MAXINT 	2147483647

#define ABS(x)		(((x) < 0)?(-(x)):(x))
#define MIN(x, y)	(((x) < (y))?(x):(y))
#define MAX(x, y)	(((x) > (y))?(x):(y))

int g_nIproc = 0, g_nNproc = 0;

typedef struct protocolstruct ProtocolStruct;
struct protocolstruct
{
    int nbor, iproc;
};

typedef struct argstruct ArgStruct;
struct argstruct 
{
    /* This is the common information that is needed for all tests	*/
    char    *host;	/* Name of receiving host			*/
    char    *buff;	/* Transmitted buffer				*/
    char    *buff1;	/* Transmitted buffer				*/
    int     bufflen,	/* Length of transmitted buffer 		*/
	    tr, 	/* Transmit flag 				*/
	    nbuff;	/* Number of buffers to transmit		*/
    
    /* Now we work with a union of information for protocol dependent stuff */
    ProtocolStruct prot;	/* Structure holding necessary info for TCP */
};

typedef struct data Data;
struct data
{
    double t;
    double bps;
    double variance;
    int    bits;
    int    repeat;
};

double When(void);
int Setup(ArgStruct *p);
void Sync(ArgStruct *p);
void SendData(ArgStruct *p);
void RecvData(ArgStruct *p);
void SendRecvData(ArgStruct *p);
void SendTime(ArgStruct *p, double *t, int *rpt);
void RecvTime(ArgStruct *p, double *t, int *rpt);
int Establish(ArgStruct *p);
int  CleanUp(ArgStruct *p);
double TestLatency(ArgStruct *p);
double TestSyncTime(ArgStruct *p);
void PrintOptions(void);
int DetermineLatencyReps(ArgStruct *p);

void PrintOptions()
{
    printf("\n");
    printf("Usage: netpipe flags\n");
    printf(" flags:\n");
    printf("       -reps #iterations\n");
    printf("       -time stop_time\n");
    printf("       -start initial_msg_size\n");
    printf("       -end final_msg_size\n");
    printf("       -out outputfile\n");
    printf("       -nocache\n");
    printf("       -headtohead\n");
    printf("       -pert\n");
    printf("       -noprint\n");
    printf("       -onebuffer largest_buffer_size\n");
    printf("Requires exactly two processes\n");
    printf("\n");
}

int main(int argc, char *argv[])
{
    FILE *out=0;		/* Output data file 			*/
    char s[255]; 		/* Generic string			*/
    char *memtmp;
    char *memtmp1;
    
    int i, j, n, nq,		/* Loop indices				*/
	bufoffset = 0,		/* Align buffer to this			*/
	bufalign = 16*1024,	/* Boundary to align buffer to		*/
	nrepeat,		/* Number of time to do the transmission*/
	nzero = 0,
	len,			/* Number of bytes to be transmitted	*/
	inc = 1,		/* Increment value			*/
	detailflag = 0,		/* Set to examine the signature curve detail*/
	pert,			/* Perturbation value			*/
        ipert,                  /* index of the perturbation loop	*/
	start = 0,		/* Starting value for signature curve 	*/
	end = MAXINT,		/* Ending value for signature curve	*/
	streamopt = 0,		/* Streaming mode flag			*/
	printopt = 1;		/* Debug print statements flag		*/
    int one_buffer = 0;
    int onebuffersize = 100*1024*1024;
    int quit = 0;
    
    ArgStruct	args;		/* Argumentsfor all the calls		*/
    
    double t, t0, t1, t2,	/* Time variables			*/
	tlast,			/* Time for the last transmission	*/
	tzero = 0,
	latency,		/* Network message latency		*/
	synctime;		/* Network synchronization time 	*/
    
    Data *bwdata;		/* Bandwidth curve data 		*/
    
    BOOL bNoCache = FALSE;
    BOOL bHeadToHead = FALSE;
    BOOL bSavePert = FALSE;
    BOOL bUseMegaBytes = FALSE;

    MPI_Init(&argc, &argv);
    
    MPI_Comm_size(MPI_COMM_WORLD, &g_nNproc);
    MPI_Comm_rank(MPI_COMM_WORLD, &g_nIproc);
    
    if (g_nNproc != 2)
    {
	if (g_nIproc == 0)
	    PrintOptions();
	MPI_Finalize();
	exit(0);
    }
    
    GetOptDouble(&argc, &argv, "-time", &g_STOPTM);
    GetOptInt(&argc, &argv, "-reps", &g_NSAMP);
    GetOptInt(&argc, &argv, "-start", &start);
    GetOptInt(&argc, &argv, "-end", &end);
    one_buffer = GetOptInt(&argc, &argv, "-onebuffer", &onebuffersize);
    if (one_buffer)
    {
	if (onebuffersize < 1)
	{
	    one_buffer = 0;
	}
	else
	{
	    onebuffersize += bufalign;
	}
    }
    bNoCache = GetOpt(&argc, &argv, "-nocache");
    bHeadToHead = GetOpt(&argc, &argv, "-headtohead");
    bUseMegaBytes = GetOpt(&argc, &argv, "-mb");
    if (GetOpt(&argc, &argv, "-noprint"))
	printopt = 0;
    bSavePert = GetOpt(&argc, &argv, "-pert");
    
    bwdata = malloc((g_NSAMP+1) * sizeof(Data));

    if (g_nIproc == 0)
	strcpy(s, "Netpipe.out");
    GetOptString(&argc, &argv, "-out", s);
    
    if (start > end)
    {
	fprintf(stdout, "Start MUST be LESS than end\n");
	exit(420132);
    }
    
    args.nbuff = TRIALS;
    
    Setup(&args);
    Establish(&args);
    
    if (args.tr)
    {
	if ((out = fopen(s, "w")) == NULL)
	{
	    fprintf(stdout,"Can't open %s for output\n", s);
	    exit(1);
	}
    }
    
    latency = TestLatency(&args);
    synctime = TestSyncTime(&args);
 
    
    if (args.tr)
    {
	SendTime(&args, &latency, &nzero);
    }
    else
    {
	RecvTime(&args, &latency, &nzero);
    }
    if (args.tr && printopt)
    {
	printf("Latency: %0.9f\n", latency);
	fflush(stdout);
	printf("Sync Time: %0.9f\n", synctime);
	fflush(stdout);
	printf("Now starting main loop\n");
	fflush(stdout);
    }
    tlast = latency;
    inc = (start > 1 && !detailflag) ? start/2: inc;
    args.bufflen = start;

    if (one_buffer)
    {
	args.buff = (char *)malloc(onebuffersize);
	args.buff1 = (char*)malloc(onebuffersize);
    }

    /* Main loop of benchmark */
    for (nq = n = 0, len = start; 
         n < g_NSAMP && tlast < g_STOPTM && len <= end && !quit; 
	 len = len + inc, nq++)
    {
	if (nq > 2 && !detailflag)
	    inc = ((nq % 2))? inc + inc: inc;
	
	/* This is a perturbation loop to test nearby values */
	for (ipert = 0, pert = (!detailflag && inc > PERT + 1)? -PERT: 0;
	     pert <= PERT && !quit; 
	     ipert++, n++, 
		 pert += (!detailflag && inc > PERT + 1)? PERT: PERT + 1)
	{
	    
	    /* Calculate howmany times to repeat the experiment. */
	    if (args.tr)
	    {
		if (args.bufflen == 0)
		    nrepeat = g_LATENCYREPS;
		else
		    nrepeat = (int)(MAX((RUNTM / ((double)args.bufflen /
			           (args.bufflen - inc + 1.0) * tlast)), TRIALS));
		SendTime(&args, &tzero, &nrepeat);
	    }
	    else
	    {
		nrepeat = 1; /* Just needs to be greater than zero */
		RecvTime(&args, &tzero, &nrepeat);
	    }
	    
	    /* Allocate the buffer */
	    args.bufflen = len + pert;
	    if (one_buffer)
	    {
		if (bNoCache)
		{
		    if (args.bufflen * nrepeat + bufalign > onebuffersize)
		    {
			fprintf(stdout, "Exceeded user specified buffer size\n");
			fflush(stdout);
			quit = 1;
			break;
		    }
		}
		else
		{
		    if (args.bufflen + bufalign > onebuffersize)
		    {
			fprintf(stdout, "Exceeded user specified buffer size\n");
			fflush(stdout);
			quit = 1;
			break;
		    }
		}
	    }
	    else
	    {
		/* printf("allocating %d bytes\n", 
		   args.bufflen * nrepeat + bufalign); */
		if (bNoCache)
		{
		    if ((args.buff = (char *)malloc(args.bufflen * nrepeat + bufalign)) == (char *)NULL)
		    {
			fprintf(stdout,"Couldn't allocate memory\n");
			fflush(stdout);
			break;
		    }
		}
		else
		{
		    if ((args.buff = (char *)malloc(args.bufflen + bufalign)) == (char *)NULL)
		    {
			fprintf(stdout,"Couldn't allocate memory\n");
			fflush(stdout);
			break;
		    }
		}
		/* if ((args.buff1 = (char *)malloc(args.bufflen * nrepeat + bufalign)) == (char *)NULL) */
		if ((args.buff1 = (char *)malloc(args.bufflen + bufalign)) == (char *)NULL)
		{
		    fprintf(stdout,"Couldn't allocate memory\n");
		    fflush(stdout);
		    break;
		}
	    }
	    /* Possibly align the data buffer */
	    memtmp = args.buff;
	    memtmp1 = args.buff1;
	    
	    if (!bNoCache)
	    {
		if (bufalign != 0)
		{
		    args.buff += (bufalign - ((MPI_Aint)args.buff % bufalign) + bufoffset) % bufalign;
		    /* args.buff1 += (bufalign - ((MPI_Aint)args.buff1 % bufalign) + bufoffset) % bufalign; */
		}
	    }
	    args.buff1 += (bufalign - ((MPI_Aint)args.buff1 % bufalign) + bufoffset) % bufalign;
	    
	    if (args.tr && printopt)
	    {
		fprintf(stdout,"%3d: %9d bytes %4d times --> ",
		    n, args.bufflen, nrepeat);
		fflush(stdout);
	    }
	    
	    /* Finally, we get to transmit or receive and time */
	    if (args.tr)
	    {
		bwdata[n].t = LONGTIME;
		t2 = t1 = 0;
		for (i = 0; i < TRIALS; i++)
		{
		    if (bNoCache)
		    {
			if (bufalign != 0)
			{
			    args.buff = memtmp + ((bufalign - ((MPI_Aint)args.buff % bufalign) + bufoffset) % bufalign);
			    /* args.buff1 = memtmp1 + ((bufalign - ((MPI_Aint)args.buff1 % bufalign) + bufoffset) % bufalign); */
			}
			else
			{
			    args.buff = memtmp;
			    /* args.buff1 = memtmp1; */
			}
		    }
		    
		    Sync(&args);
		    t0 = When();
		    for (j = 0; j < nrepeat; j++)
		    {
			if (bHeadToHead)
			    SendRecvData(&args);
			else
			{
			    SendData(&args);
			    if (!streamopt)
			    {
				RecvData(&args);
			    }
			}
			if (bNoCache)
			{
			    args.buff += args.bufflen;
			    /* args.buff1 += args.bufflen; */
			}
		    }
		    t = (When() - t0)/((1 + !streamopt) * nrepeat);
		    
		    if (!streamopt)
		    {
			t2 += t*t;
			t1 += t;
			bwdata[n].t = MIN(bwdata[n].t, t);
		    }
		}
		if (!streamopt)
		    SendTime(&args, &bwdata[n].t, &nzero);
		else
		    RecvTime(&args, &bwdata[n].t, &nzero);
		
		if (!streamopt)
		    bwdata[n].variance = t2/TRIALS - t1/TRIALS * t1/TRIALS;
		
	    }
	    else
	    {
		bwdata[n].t = LONGTIME;
		t2 = t1 = 0;
		for (i = 0; i < TRIALS; i++)
		{
		    if (bNoCache)
		    {
			if (bufalign != 0)
			{
			    args.buff = memtmp + ((bufalign - ((MPI_Aint)args.buff % bufalign) + bufoffset) % bufalign);
			    /* args.buff1 = memtmp1 + ((bufalign - ((MPI_Aint)args.buff1 % bufalign) + bufoffset) % bufalign); */
			}
			else
			{
			    args.buff = memtmp;
			    /* args.buff1 = memtmp1; */
			}
		    }
		    
		    Sync(&args);
		    t0 = When();
		    for (j = 0; j < nrepeat; j++)
		    {
			if (bHeadToHead)
			    SendRecvData(&args);
			else
			{
			    RecvData(&args);
			    if (!streamopt)
				SendData(&args);
			}
			if (bNoCache)
			{
			    args.buff += args.bufflen;
			    /* args.buff1 += args.bufflen; */
			}
		    }
		    t = (When() - t0)/((1 + !streamopt) * nrepeat);
		    
		    if (streamopt)
		    {
			t2 += t*t;
			t1 += t;
			bwdata[n].t = MIN(bwdata[n].t, t);
		    }
		}
		if (streamopt)
		    SendTime(&args, &bwdata[n].t, &nzero);
		else
		    RecvTime(&args, &bwdata[n].t, &nzero);
		
		if (streamopt)
		    bwdata[n].variance = t2/TRIALS - t1/TRIALS * t1/TRIALS;
		
	    }
	    tlast = bwdata[n].t;
	    bwdata[n].bits = args.bufflen * CHARSIZE;
	    bwdata[n].bps = bwdata[n].bits / (bwdata[n].t * 1024 * 1024);
	    bwdata[n].repeat = nrepeat;
	    
	    if (args.tr)
	    {
		if (bSavePert)
		{
		/* fprintf(out,"%f\t%f\t%d\t%d\t%f\n", bwdata[n].t, bwdata[n].bps,
		    bwdata[n].bits, bwdata[n].bits / 8, bwdata[n].variance); */
		    if (bUseMegaBytes)
			fprintf(out,"%d\t%f\t%0.9f\n", bwdata[n].bits / 8, bwdata[n].bps / 8, bwdata[n].t);
		    else
			fprintf(out,"%d\t%f\t%0.9f\n", bwdata[n].bits / 8, bwdata[n].bps, bwdata[n].t);
		    fflush(out);
		}
	    }
	    if (!one_buffer)
	    {
		free(memtmp);
		free(memtmp1);
	    }
	    if (args.tr && printopt)
	    {
		if (bUseMegaBytes)
		    fprintf(stdout," %6.2f MBps in %0.9f sec\n", bwdata[n].bps / 8, tlast);
		else
		    fprintf(stdout," %6.2f Mbps in %0.9f sec\n", bwdata[n].bps, tlast);
		fflush(stdout);
	    }
	} /* End of perturbation loop */	
	if (!bSavePert && args.tr)
	{
	    /* if we didn't save all of the perturbation loops, find the max and save it */
	    int index = 1;
	    double dmax = bwdata[n-1].bps;
	    for (; ipert > 1; ipert--)
	    {
		if (bwdata[n-ipert].bps > dmax)
		{
		    index = ipert;
		    dmax = bwdata[n-ipert].bps;
		}
	    }
	    if (bUseMegaBytes)
		fprintf(out,"%d\t%f\t%0.9f\n", bwdata[n-index].bits / 8, bwdata[n-index].bps / 8, bwdata[n-index].t);
	    else
		fprintf(out,"%d\t%f\t%0.9f\n", bwdata[n-index].bits / 8, bwdata[n-index].bps, bwdata[n-index].t);
	    fflush(out);
	}
    } /* End of main loop  */
	
    if (args.tr)
	fclose(out);
    /* THE_END:		 */
    CleanUp(&args);
    free(bwdata);
    return 0;
}


/* Return the current time in seconds, using a double precision number. 	 */
double When()
{
    return MPI_Wtime();
}

int Setup(ArgStruct *p)
{
    int nproc;
    char s[255];
    int len = 255;
    
    MPI_Comm_rank(MPI_COMM_WORLD, &p->prot.iproc);
    MPI_Comm_size(MPI_COMM_WORLD, &nproc);
    
    MPI_Get_processor_name(s, &len);
    /*gethostname(s, len);*/
    printf("%d: %s\n", p->prot.iproc, s);
    fflush(stdout);
    
    if (p->prot.iproc == 0)
	p->prot.nbor = 1;
    else
	p->prot.nbor = 0;
    
    if (nproc < 2)
    {
	printf("Need two processes\n");
	printf("nproc: %i\n", nproc);
	exit(-2);
    }
    
    if (p->prot.iproc == 0)
	p->tr = 1;
    else
	p->tr = 0;
    return 1;	
}	

void Sync(ArgStruct *p)
{
    char ch;
    MPI_Status status;
    if (p->tr)
    {
	MPI_Send(&ch, 0, MPI_BYTE, p->prot.nbor, 1, MPI_COMM_WORLD);
	MPI_Recv(&ch, 0, MPI_BYTE, p->prot.nbor, 1, MPI_COMM_WORLD, &status);
	MPI_Send(&ch, 0, MPI_BYTE, p->prot.nbor, 1, MPI_COMM_WORLD);
    }
    else
    {
	MPI_Recv(&ch, 0, MPI_BYTE, p->prot.nbor, 1, MPI_COMM_WORLD, &status);
	MPI_Send(&ch, 0, MPI_BYTE, p->prot.nbor, 1, MPI_COMM_WORLD);
	MPI_Recv(&ch, 0, MPI_BYTE, p->prot.nbor, 1, MPI_COMM_WORLD, &status);
    }
}

int DetermineLatencyReps(ArgStruct *p)
{
    MPI_Status status;
    double t0, duration = 0;
    int reps = 1, prev_reps = 0;
    int i;

    /* prime the send/receive pipes */
    Sync(p);
    Sync(p);
    Sync(p);

    /* test how long it takes to send n messages 
     * where n = 1, 2, 4, 8, 16, 32, ...
     */
    t0 = When();
    t0 = When();
    t0 = When();
    while ( (duration < 1) ||
	    (duration < 3 && reps < 1000))
    {
	t0 = When();
	for (i=0; i<reps-prev_reps; i++)
	{
	    Sync(p);
	}
	duration += When() - t0;
	prev_reps = reps;
	reps = reps * 2;

	/* use duration from the root only */
	if (p->prot.iproc == 0)
	    MPI_Send(&duration, 1, MPI_DOUBLE, p->prot.nbor, 2, MPI_COMM_WORLD);
	else
	    MPI_Recv(&duration, 1, MPI_DOUBLE, p->prot.nbor, 2, MPI_COMM_WORLD, &status);
    }

    return reps;
}

double TestLatency(ArgStruct *p)
{
    double latency, t0;
    int i;

    g_LATENCYREPS = DetermineLatencyReps(p);
    if (g_LATENCYREPS < 1024 && p->prot.iproc == 0)
    {
	printf("Using %d reps to determine latency\n", g_LATENCYREPS);
	fflush(stdout);
    }

    p->bufflen = 0;
    p->buff = NULL; /*(char *)malloc(p->bufflen);*/
    p->buff1 = NULL; /*(char *)malloc(p->bufflen);*/
    Sync(p);
    t0 = When();
    t0 = When();
    t0 = When();
    t0 = When();
    for (i = 0; i < g_LATENCYREPS; i++)
    {
	if (p->tr)
	{
	    SendData(p);
	    RecvData(p);
	}
	else
	{
	    RecvData(p);
	    SendData(p);
	}
    }
    latency = (When() - t0)/(2 * g_LATENCYREPS);
    /*
    free(p->buff);
    free(p->buff1);
    */

    return latency;
}

double TestSyncTime(ArgStruct *p)
{
    double synctime, t0;
    int i;

    t0 = When();
    t0 = When();
    t0 = When();
    t0 = When();
    t0 = When();
    t0 = When();
    for (i = 0; i < g_LATENCYREPS; i++)
	Sync(p);
    synctime = (When() - t0)/g_LATENCYREPS;

    return synctime;
}

void SendRecvData(ArgStruct *p)
{
    MPI_Status status;
    
    /*MPI_Sendrecv(p->buff, p->bufflen, MPI_BYTE, p->prot.nbor, 1, p->buff1, p->bufflen, MPI_BYTE, p->prot.nbor, 1, MPI_COMM_WORLD, &status);*/

    MPI_Request request;
    MPI_Irecv(p->buff1, p->bufflen, MPI_BYTE, p->prot.nbor, 1, MPI_COMM_WORLD, &request);
    MPI_Send(p->buff, p->bufflen, MPI_BYTE, p->prot.nbor, 1, MPI_COMM_WORLD);
    MPI_Wait(&request, &status);

    /*
    MPI_Send(p->buff, p->bufflen, MPI_BYTE, p->prot.nbor, 1, MPI_COMM_WORLD);
    MPI_Recv(p->buff1, p->bufflen, MPI_BYTE, p->prot.nbor, 1, MPI_COMM_WORLD, &status);
    */
}

void SendData(ArgStruct *p)
{
    MPI_Send(p->buff, p->bufflen, MPI_BYTE, p->prot.nbor, 1, MPI_COMM_WORLD);
}

void RecvData(ArgStruct *p)
{
    MPI_Status status;
    MPI_Recv(p->buff1, p->bufflen, MPI_BYTE, p->prot.nbor, 1, MPI_COMM_WORLD, &status);
}


void SendTime(ArgStruct *p, double *t, int *rpt)
{
    if (*rpt > 0)
	MPI_Send(rpt, 1, MPI_INT, p->prot.nbor, 2, MPI_COMM_WORLD);
    else
	MPI_Send(t, 1, MPI_DOUBLE, p->prot.nbor, 2, MPI_COMM_WORLD);
}

void RecvTime(ArgStruct *p, double *t, int *rpt)
{
    MPI_Status status;
    if (*rpt > 0)
	MPI_Recv(rpt, 1, MPI_INT, p->prot.nbor, 2, MPI_COMM_WORLD, &status);
    else
	MPI_Recv(t, 1, MPI_DOUBLE, p->prot.nbor, 2, MPI_COMM_WORLD, &status);
}

int Establish(ArgStruct *p)
{
    return 1;
}

int  CleanUp(ArgStruct *p)
{
  /*MPI_Barrier(MPI_COMM_WORLD);*/
    MPI_Finalize();
    return 1;
}

