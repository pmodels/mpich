/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifdef HAVE_WINDOWS_H
#include <winsock2.h>
#include <windows.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mpi.h"
#include "GetOpt.h"

#ifndef BOOL
typedef int BOOL;
#endif
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#define MIN(x, y)	(((x) < (y))?(x):(y))
#define MAX(x, y)	(((x) > (y))?(x):(y))

#ifdef HAVE_WINDOWS_H
#define POINTER_TO_INT(a)   ( ( int )( 0xffffffff & (__int64) ( a ) ) )
#else
#define POINTER_TO_INT(a)   ( ( MPI_Aint )( a ) )
#endif

#define CREATE_DIFFERENCE_CURVES
#undef CREATE_SINGLE_CURVE

#define MAX_NUM_O12_TRIALS 18
#define TRIALS          7
#define PERT            3
#define LONGTIME        1e99
#define CHARSIZE        8
#define RUNTM		0.1
#define MAXINT          2147483647
#define MAX_LAT_TIME    2
#define LEFT_PROCESS    0
#define MIDDLE_PROCESS  1
#define RIGHT_PROCESS   2
#define MSG_TAG_01      9901
#define MSG_TAG_12      9912
#define MSG_TAG_012     9012

int     g_left_rank       = -1;
int     g_middle_rank     = -1;
int     g_right_rank      = -1;
int     g_proc_loc        = -1;
int 	g_NSAMP           = 250;
double	g_STOPTM          = 0.1;
int     g_latency012_reps = 1000;
int     g_nIproc          = 0;
int     g_nNproc          = 0;

typedef struct ArgStruct
{
    char *sbuff;        /* Send buffer      */
    char *rbuff;        /* Recv buffer      */
    int  bufflen;       /* Length of buffer */
    int  nbor, nbor2;   /* neighbor */
    int  iproc;         /* rank */
    int  tr;            /* transmitter/receiver flag */
    int  latency_reps;  /* reps needed to time latency */
} ArgStruct;

typedef struct Data
{
    double t;
    double bps;
    int    bits;
    int    repeat;
} Data;

int Setup(int middle_rank, ArgStruct *p01, ArgStruct *p12, ArgStruct *p012);
void Sync(ArgStruct *p);
void Sync012(ArgStruct *p);
void SendTime(ArgStruct *p, double *t);
void RecvTime(ArgStruct *p, double *t);
void SendReps(ArgStruct *p, int *rpt);
void RecvReps(ArgStruct *p, int *rpt);
double TestLatency(ArgStruct *p);
double TestLatency012(ArgStruct *p);
void PrintOptions(void);
int DetermineLatencyReps(ArgStruct *p);
int DetermineLatencyReps012(ArgStruct *p);

void PrintOptions()
{
    printf("\n");
    printf("Usage: adapt flags\n");
    printf(" flags:\n");
    printf("       -reps #iterations\n");
    printf("       -time stop_time\n");
    printf("       -start initial_msg_size\n");
    printf("       -end final_msg_size\n");
    printf("       -out outputfile\n");
    printf("       -nocache\n");
    printf("       -pert\n");
    printf("       -noprint\n");
    printf("       -middle rank_0_1_or_2\n");
    printf("Requires exactly three processes\n");
    printf("\n");
}

int main(int argc, char *argv[])
{
    FILE *out=0;		/* Output data file 			*/
    char s[255]; 		/* Generic string			*/
    char *memtmp;
    char *memtmp1;
    MPI_Status status;

    int ii, i, j, k, n, nq,	/* Loop indices				*/
	bufoffset = 0,		/* Align buffer to this			*/
	bufalign = 16*1024,	/* Boundary to align buffer to		*/
	nrepeat01, nrepeat12,	/* Number of time to do the transmission*/
	nrepeat012,
	len,			/* Number of bytes to be transmitted	*/
	inc = 1,		/* Increment value			*/
	pert,			/* Perturbation value			*/
        ipert,                  /* index of the perturbation loop	*/
	start = 0,		/* Starting value for signature curve 	*/
	end = MAXINT,		/* Ending value for signature curve	*/
	printopt = 1,		/* Debug print statements flag		*/
	middle_rank = 0,        /* rank 0, 1 or 2 where 2-0-1 or 0-1-2 or 1-2-0 */
	tint;
    
    ArgStruct	args01, args12, args012;/* Argumentsfor all the calls	*/
    
    double t, t0, t1,           /* Time variables			*/
	tlast01, tlast12, tlast012,/* Time for the last transmission	*/
	latency01, latency12,	/* Network message latency		*/
	latency012, tdouble;    /* Network message latency to go from 0 -> 1 -> 2 */
#ifdef CREATE_DIFFERENCE_CURVES
    int itrial, ntrials;
    double *dtrials;
#endif

    Data *bwdata01, *bwdata12, *bwdata012;/* Bandwidth curve data 	*/
    
    BOOL bNoCache = FALSE;
    BOOL bSavePert = FALSE;
    BOOL bUseMegaBytes = FALSE;

    MPI_Init(&argc, &argv);
    
    MPI_Comm_size(MPI_COMM_WORLD, &g_nNproc);
    MPI_Comm_rank(MPI_COMM_WORLD, &g_nIproc);
    
    if (g_nNproc != 3)
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
    bNoCache = GetOpt(&argc, &argv, "-nocache");
    bUseMegaBytes = GetOpt(&argc, &argv, "-mb");
    if (GetOpt(&argc, &argv, "-noprint"))
	printopt = 0;
    bSavePert = GetOpt(&argc, &argv, "-pert");
    GetOptInt(&argc, &argv, "-middle", &middle_rank);
    if (middle_rank < 0 || middle_rank > 2)
	middle_rank = 0;

    bwdata01 = malloc((g_NSAMP+1) * sizeof(Data));
    bwdata12 = malloc((g_NSAMP+1) * sizeof(Data));
    bwdata012 = malloc((g_NSAMP+1) * sizeof(Data));

    if (g_nIproc == 0)
	strcpy(s, "adapt.out");
    GetOptString(&argc, &argv, "-out", s);
    
    if (start > end)
    {
	fprintf(stdout, "Start MUST be LESS than end\n");
	exit(420132);
    }

    Setup(middle_rank, &args01, &args12, &args012);

    if (g_nIproc == 0)
    {
	if ((out = fopen(s, "w")) == NULL)
	{
	    fprintf(stdout,"Can't open %s for output\n", s);
	    exit(1);
	}
    }

    /* Calculate latency */
    switch (g_proc_loc)
    {
    case LEFT_PROCESS:
	latency01 = TestLatency(&args01);
	/*printf("[0] latency01 = %0.9f\n", latency01);fflush(stdout);*/
	RecvTime(&args01, &latency12);
	/*printf("[0] latency12 = %0.9f\n", latency12);fflush(stdout);*/
	break;
    case MIDDLE_PROCESS:
	latency01 = TestLatency(&args01);
	/*printf("[1] latency01 = %0.9f\n", latency01);fflush(stdout);*/
	SendTime(&args12, &latency01);
	latency12 = TestLatency(&args12);
	/*printf("[1] latency12 = %0.9f\n", latency12);fflush(stdout);*/
	SendTime(&args01, &latency12);
	break;
    case RIGHT_PROCESS:
	RecvTime(&args12, &latency01);
	/*printf("[2] latency01 = %0.9f\n", latency01);fflush(stdout);*/
	latency12 = TestLatency(&args12);
	/*printf("[2] latency12 = %0.9f\n", latency12);fflush(stdout);*/
	break;
    }

    latency012 = TestLatency012(&args012);

    if ((g_nIproc == 0) && printopt)
    {
	printf("Latency%d%d_ : %0.9f\n", g_left_rank, g_middle_rank, latency01);
	printf("Latency_%d%d : %0.9f\n", g_middle_rank, g_right_rank, latency12);
	printf("Latency%d%d%d : %0.9f\n", g_left_rank, g_middle_rank, g_right_rank, latency012);
	fflush(stdout);
	printf("Now starting main loop\n");
	fflush(stdout);
    }
    tlast01 = latency01;
    tlast12 = latency12;
    tlast012 = latency012;
    inc = (start > 1) ? start/2: inc;
    args01.bufflen = start;
    args12.bufflen = start;
    args012.bufflen = start;

#ifdef CREATE_DIFFERENCE_CURVES
    /* print the header line of the output file */
    if (g_nIproc == 0)
    {
	fprintf(out, "bytes\tMbits/s\ttime\tMbits/s\ttime");
	for (ii=1, itrial=0; itrial<MAX_NUM_O12_TRIALS; ii <<= 1, itrial++)
	    fprintf(out, "\t%d", ii);
	fprintf(out, "\n");
	fflush(out);
    }
    ntrials = MAX_NUM_O12_TRIALS;
    dtrials = malloc(sizeof(double)*ntrials);
#endif

    /* Main loop of benchmark */
    for (nq = n = 0, len = start; 
         n < g_NSAMP && tlast012 < g_STOPTM && len <= end; 
	 len = len + inc, nq++)
    {
	if (nq > 2)
	    inc = (nq % 2) ? inc + inc : inc;

	/* clear the old values */
	for (itrial = 0; itrial < ntrials; itrial++)
	{
	    dtrials[itrial] = LONGTIME;
	}

	/* This is a perturbation loop to test nearby values */
	for (ipert = 0, pert = (inc > PERT + 1) ? -PERT : 0;
	     pert <= PERT; 
	     ipert++, n++, pert += (inc > PERT + 1) ? PERT : PERT + 1)
	{


	    /*****************************************************/
	    /*         Run a trial between rank 0 and 1          */
	    /*****************************************************/

	    MPI_Barrier(MPI_COMM_WORLD);


	    if (g_proc_loc == RIGHT_PROCESS)
		goto skip_01_trial;

	    /* Calculate howmany times to repeat the experiment. */
	    if (args01.tr)
	    {
		if (args01.bufflen == 0)
		    nrepeat01 = args01.latency_reps;
		else
		    nrepeat01 = (int)(MAX((RUNTM / ((double)args01.bufflen /
			           (args01.bufflen - inc + 1.0) * tlast01)), TRIALS));
		SendReps(&args01, &nrepeat01);
	    }
	    else
	    {
		RecvReps(&args01, &nrepeat01);
	    }

	    /* Allocate the buffer */
	    args01.bufflen = len + pert;
	    /* printf("allocating %d bytes\n", args01.bufflen * nrepeat01 + bufalign); */
	    if (bNoCache)
	    {
		if ((args01.sbuff = (char *)malloc(args01.bufflen * nrepeat01 + bufalign)) == (char *)NULL)
		{
		    fprintf(stdout,"Couldn't allocate memory\n");
		    fflush(stdout);
		    break;
		}
	    }
	    else
	    {
		if ((args01.sbuff = (char *)malloc(args01.bufflen + bufalign)) == (char *)NULL)
		{
		    fprintf(stdout,"Couldn't allocate memory\n");
		    fflush(stdout);
		    break;
		}
	    }
	    /* if ((args01.rbuff = (char *)malloc(args01.bufflen * nrepeat01 + bufalign)) == (char *)NULL) */
	    if ((args01.rbuff = (char *)malloc(args01.bufflen + bufalign)) == (char *)NULL)
	    {
		fprintf(stdout,"Couldn't allocate memory\n");
		fflush(stdout);
		break;
	    }

	    /* save the original pointers in case alignment moves them */
	    memtmp = args01.sbuff;
	    memtmp1 = args01.rbuff;

	    /* Possibly align the data buffer */
	    if (!bNoCache)
	    {
		if (bufalign != 0)
		{
		    args01.sbuff += (bufalign - (POINTER_TO_INT(args01.sbuff) % bufalign) + bufoffset) % bufalign;
		    /* args01.rbuff += (bufalign - ((MPI_Aint)args01.rbuff % bufalign) + bufoffset) % bufalign; */
		}
	    }
	    args01.rbuff += (bufalign - (POINTER_TO_INT(args01.rbuff) % bufalign) + bufoffset) % bufalign;
	    
	    if (args01.tr && printopt)
	    {
		fprintf(stdout,"%3d: %9d bytes %4d times --> ",
		    n, args01.bufflen, nrepeat01);
		fflush(stdout);
	    }
	    
	    /* Finally, we get to transmit or receive and time */
	    if (args01.tr)
	    {
		bwdata01[n].t = LONGTIME;
		t1 = 0;
		for (i = 0; i < TRIALS; i++)
		{
		    if (bNoCache)
		    {
			if (bufalign != 0)
			{
			    args01.sbuff = memtmp + ((bufalign - (POINTER_TO_INT(args01.sbuff) % bufalign) + bufoffset) % bufalign);
			    /* args01.rbuff = memtmp1 + ((bufalign - ((MPI_Aint)args01.rbuff % bufalign) + bufoffset) % bufalign); */
			}
			else
			{
			    args01.sbuff = memtmp;
			    /* args01.rbuff = memtmp1; */
			}
		    }
		    
		    Sync(&args01);
		    t0 = MPI_Wtime();
		    for (j = 0; j < nrepeat01; j++)
		    {
			MPI_Send(args01.sbuff,  args01.bufflen, MPI_BYTE,  args01.nbor, MSG_TAG_01, MPI_COMM_WORLD);
			MPI_Recv(args01.rbuff,  args01.bufflen, MPI_BYTE,  args01.nbor, MSG_TAG_01, MPI_COMM_WORLD, &status);
			if (bNoCache)
			{
			    args01.sbuff += args01.bufflen;
			    /* args01.rbuff += args01.bufflen; */
			}
		    }
		    t = (MPI_Wtime() - t0)/(2 * nrepeat01);

		    t1 += t;
		    bwdata01[n].t = MIN(bwdata01[n].t, t);
		}
		SendTime(&args01, &bwdata01[n].t);
	    }
	    else
	    {
		bwdata01[n].t = LONGTIME;
		t1 = 0;
		for (i = 0; i < TRIALS; i++)
		{
		    if (bNoCache)
		    {
			if (bufalign != 0)
			{
			    args01.sbuff = memtmp + ((bufalign - (POINTER_TO_INT(args01.sbuff) % bufalign) + bufoffset) % bufalign);
			    /* args01.rbuff = memtmp1 + ((bufalign - ((MPI_Aint)args01.rbuff % bufalign) + bufoffset) % bufalign); */
			}
			else
			{
			    args01.sbuff = memtmp;
			    /* args01.rbuff = memtmp1; */
			}
		    }
		    
		    Sync(&args01);
		    t0 = MPI_Wtime();
		    for (j = 0; j < nrepeat01; j++)
		    {
			MPI_Recv(args01.rbuff,  args01.bufflen, MPI_BYTE,  args01.nbor, MSG_TAG_01, MPI_COMM_WORLD, &status);
			MPI_Send(args01.sbuff,  args01.bufflen, MPI_BYTE,  args01.nbor, MSG_TAG_01, MPI_COMM_WORLD);
			if (bNoCache)
			{
			    args01.sbuff += args01.bufflen;
			    /* args01.rbuff += args01.bufflen; */
			}
		    }
		    t = (MPI_Wtime() - t0)/(2 * nrepeat01);
		}
		RecvTime(&args01, &bwdata01[n].t);
	    }
	    tlast01 = bwdata01[n].t;
	    bwdata01[n].bits = args01.bufflen * CHARSIZE;
	    bwdata01[n].bps = bwdata01[n].bits / (bwdata01[n].t * 1024 * 1024);
	    bwdata01[n].repeat = nrepeat01;
	    
	    if (args01.tr)
	    {
		if (bSavePert)
		{
		    if (args01.iproc == 0)
		    {
			if (bUseMegaBytes)
			    fprintf(out, "%d\t%f\t%0.9f\t", bwdata01[n].bits / 8, bwdata01[n].bps / 8, bwdata01[n].t);
			else
			    fprintf(out, "%d\t%f\t%0.9f\t", bwdata01[n].bits / 8, bwdata01[n].bps, bwdata01[n].t);
			fflush(out);
		    }
		    else
		    {
			MPI_Send(&bwdata01[n].bits, 1, MPI_INT, 0, 1, MPI_COMM_WORLD);
			MPI_Send(&bwdata01[n].bps, 1, MPI_DOUBLE, 0, 1, MPI_COMM_WORLD);
			MPI_Send(&bwdata01[n].t, 1, MPI_DOUBLE, 0, 1, MPI_COMM_WORLD);
		    }
		}
	    }
	    
	    free(memtmp);
	    free(memtmp1);
	    
	    if (args01.tr && printopt)
	    {
		if (bUseMegaBytes)
		    printf(" %6.2f MBps in %0.9f sec\n", bwdata01[n].bps / 8, tlast01);
		else
		    printf(" %6.2f Mbps in %0.9f sec\n", bwdata01[n].bps, tlast01);
		fflush(stdout);
	    }

skip_01_trial:
	    if (g_proc_loc == RIGHT_PROCESS && g_nIproc == 0 && bSavePert)
	    {
		MPI_Recv(&tint, 1, MPI_INT, g_left_rank, 1, MPI_COMM_WORLD, &status);
		fprintf(out, "%d\t", tint/8);
		MPI_Recv(&tdouble, 1, MPI_DOUBLE, g_left_rank, 1, MPI_COMM_WORLD, &status);
		if (bUseMegaBytes)
		    tdouble = tdouble / 8.0;
		fprintf(out, "%f\t", tdouble);
		MPI_Recv(&tdouble, 1, MPI_DOUBLE, g_left_rank, 1, MPI_COMM_WORLD, &status);
		fprintf(out, "%0.9f\t", tdouble);
		fflush(out);
	    }


	    /*****************************************************/
	    /*         Run a trial between rank 1 and 2          */
	    /*****************************************************/

	    MPI_Barrier(MPI_COMM_WORLD);


	    if (g_proc_loc == LEFT_PROCESS)
		goto skip_12_trial;

	    /* Calculate howmany times to repeat the experiment. */
	    if (args12.tr)
	    {
		if (args12.bufflen == 0)
		    nrepeat12 = args12.latency_reps;
		else
		    nrepeat12 = (int)(MAX((RUNTM / ((double)args12.bufflen /
			           (args12.bufflen - inc + 1.0) * tlast12)), TRIALS));
		SendReps(&args12, &nrepeat12);
	    }
	    else
	    {
		RecvReps(&args12, &nrepeat12);
	    }
	    
	    /* Allocate the buffer */
	    args12.bufflen = len + pert;
	    /* printf("allocating %d bytes\n", args12.bufflen * nrepeat12 + bufalign); */
	    if (bNoCache)
	    {
		if ((args12.sbuff = (char *)malloc(args12.bufflen * nrepeat12 + bufalign)) == (char *)NULL)
		{
		    fprintf(stdout,"Couldn't allocate memory\n");
		    fflush(stdout);
		    break;
		}
	    }
	    else
	    {
		if ((args12.sbuff = (char *)malloc(args12.bufflen + bufalign)) == (char *)NULL)
		{
		    fprintf(stdout,"Couldn't allocate memory\n");
		    fflush(stdout);
		    break;
		}
	    }
	    /* if ((args12.rbuff = (char *)malloc(args12.bufflen * nrepeat12 + bufalign)) == (char *)NULL) */
	    if ((args12.rbuff = (char *)malloc(args12.bufflen + bufalign)) == (char *)NULL)
	    {
		fprintf(stdout,"Couldn't allocate memory\n");
		fflush(stdout);
		break;
	    }

	    /* save the original pointers in case alignment moves them */
	    memtmp = args12.sbuff;
	    memtmp1 = args12.rbuff;
	    
	    /* Possibly align the data buffer */
	    if (!bNoCache)
	    {
		if (bufalign != 0)
		{
		    args12.sbuff += (bufalign - (POINTER_TO_INT(args12.sbuff) % bufalign) + bufoffset) % bufalign;
		    /* args12.rbuff += (bufalign - ((MPI_Aint)args12.rbuff % bufalign) + bufoffset) % bufalign; */
		}
	    }
	    args12.rbuff += (bufalign - (POINTER_TO_INT(args12.rbuff) % bufalign) + bufoffset) % bufalign;
	    
	    if (args12.tr && printopt)
	    {
		printf("%3d: %9d bytes %4d times --> ", n, args12.bufflen, nrepeat12);
		fflush(stdout);
	    }
	    
	    /* Finally, we get to transmit or receive and time */
	    if (args12.tr)
	    {
		bwdata12[n].t = LONGTIME;
		t1 = 0;
		for (i = 0; i < TRIALS; i++)
		{
		    if (bNoCache)
		    {
			if (bufalign != 0)
			{
			    args12.sbuff = memtmp + ((bufalign - (POINTER_TO_INT(args12.sbuff) % bufalign) + bufoffset) % bufalign);
			    /* args12.rbuff = memtmp1 + ((bufalign - ((MPI_Aint)args12.rbuff % bufalign) + bufoffset) % bufalign); */
			}
			else
			{
			    args12.sbuff = memtmp;
			    /* args12.rbuff = memtmp1; */
			}
		    }
		    
		    Sync(&args12);
		    t0 = MPI_Wtime();
		    for (j = 0; j < nrepeat12; j++)
		    {
			MPI_Send(args12.sbuff,  args12.bufflen, MPI_BYTE,  args12.nbor, MSG_TAG_12, MPI_COMM_WORLD);
			MPI_Recv(args12.rbuff,  args12.bufflen, MPI_BYTE,  args12.nbor, MSG_TAG_12, MPI_COMM_WORLD, &status);
			if (bNoCache)
			{
			    args12.sbuff += args12.bufflen;
			    /* args12.rbuff += args12.bufflen; */
			}
		    }
		    t = (MPI_Wtime() - t0)/(2 * nrepeat12);

		    t1 += t;
		    bwdata12[n].t = MIN(bwdata12[n].t, t);
		}
		SendTime(&args12, &bwdata12[n].t);
	    }
	    else
	    {
		bwdata12[n].t = LONGTIME;
		t1 = 0;
		for (i = 0; i < TRIALS; i++)
		{
		    if (bNoCache)
		    {
			if (bufalign != 0)
			{
			    args12.sbuff = memtmp + ((bufalign - (POINTER_TO_INT(args12.sbuff) % bufalign) + bufoffset) % bufalign);
			    /* args12.rbuff = memtmp1 + ((bufalign - ((MPI_Aint)args12.rbuff % bufalign) + bufoffset) % bufalign); */
			}
			else
			{
			    args12.sbuff = memtmp;
			    /* args12.rbuff = memtmp1; */
			}
		    }
		    
		    Sync(&args12);
		    t0 = MPI_Wtime();
		    for (j = 0; j < nrepeat12; j++)
		    {
			MPI_Recv(args12.rbuff,  args12.bufflen, MPI_BYTE,  args12.nbor, MSG_TAG_12, MPI_COMM_WORLD, &status);
			MPI_Send(args12.sbuff,  args12.bufflen, MPI_BYTE,  args12.nbor, MSG_TAG_12, MPI_COMM_WORLD);
			if (bNoCache)
			{
			    args12.sbuff += args12.bufflen;
			    /* args12.rbuff += args12.bufflen; */
			}
		    }
		    t = (MPI_Wtime() - t0)/(2 * nrepeat12);
		}
		RecvTime(&args12, &bwdata12[n].t);
	    }
	    tlast12 = bwdata12[n].t;
	    bwdata12[n].bits = args12.bufflen * CHARSIZE;
	    bwdata12[n].bps = bwdata12[n].bits / (bwdata12[n].t * 1024 * 1024);
	    bwdata12[n].repeat = nrepeat12;

	    if (args12.tr)
	    {
		if (bSavePert)
		{
		    if (g_nIproc == 0)
		    {
			if (bUseMegaBytes)
			    fprintf(out,"%f\t%0.9f\t", bwdata12[n].bps / 8, bwdata12[n].t);
			else
			    fprintf(out,"%f\t%0.9f\t", bwdata12[n].bps, bwdata12[n].t);
			fflush(out);
		    }
		    else
		    {
			MPI_Send(&bwdata12[n].bps, 1, MPI_DOUBLE, 0, 1, MPI_COMM_WORLD);
			MPI_Send(&bwdata12[n].t, 1, MPI_DOUBLE, 0, 1, MPI_COMM_WORLD);
		    }
		}
	    }
	    
	    free(memtmp);
	    free(memtmp1);
	    
	    if (args12.tr && printopt)
	    {
		if (bUseMegaBytes)
		    printf(" %6.2f MBps in %0.9f sec\n", bwdata12[n].bps / 8, tlast12);
		else
		    printf(" %6.2f Mbps in %0.9f sec\n", bwdata12[n].bps, tlast12);
		fflush(stdout);
	    }

skip_12_trial:
	    if (g_proc_loc == LEFT_PROCESS && g_nIproc == 0 && bSavePert)
	    {
		MPI_Recv(&tdouble, 1, MPI_DOUBLE, g_middle_rank, 1, MPI_COMM_WORLD, &status);
		if (bUseMegaBytes)
		    tdouble = tdouble / 8.0;
		fprintf(out, "%f\t", tdouble);
		MPI_Recv(&tdouble, 1, MPI_DOUBLE, g_middle_rank, 1, MPI_COMM_WORLD, &status);
		fprintf(out, "%0.9f\t", tdouble);
		fflush(out);
	    }


#ifdef CREATE_DIFFERENCE_CURVES
	    /*****************************************************/
	    /*         Run a trial between rank 0, 1 and 2       */
	    /*****************************************************/

	    MPI_Barrier(MPI_COMM_WORLD);


	    /* Calculate howmany times to repeat the experiment. */
	    if (g_nIproc == 0)
	    {
		if (args012.bufflen == 0)
		    nrepeat012 = g_latency012_reps;
		else
		    nrepeat012 = (int)(MAX((RUNTM / ((double)args012.bufflen /
			           (args012.bufflen - inc + 1.0) * tlast012)), TRIALS));
		MPI_Bcast(&nrepeat012, 1, MPI_INT, 0, MPI_COMM_WORLD);
	    }
	    else
	    {
		MPI_Bcast(&nrepeat012, 1, MPI_INT, 0, MPI_COMM_WORLD);
	    }

	    /* Allocate the buffer */
	    args012.bufflen = len + pert;
	    /* printf("allocating %d bytes\n", args12.bufflen * nrepeat012 + bufalign); */
	    if (bNoCache)
	    {
		if ((args012.sbuff = (char *)malloc(args012.bufflen * nrepeat012 + bufalign)) == (char *)NULL)
		{
		    fprintf(stdout,"Couldn't allocate memory\n");
		    fflush(stdout);
		    break;
		}
	    }
	    else
	    {
		if ((args012.sbuff = (char *)malloc(args012.bufflen + bufalign)) == (char *)NULL)
		{
		    fprintf(stdout,"Couldn't allocate memory\n");
		    fflush(stdout);
		    break;
		}
	    }
	    /* if ((args012.rbuff = (char *)malloc(args012.bufflen * nrepeat012 + bufalign)) == (char *)NULL) */
	    if ((args012.rbuff = (char *)malloc(args012.bufflen + bufalign)) == (char *)NULL)
	    {
		fprintf(stdout,"Couldn't allocate memory\n");
		fflush(stdout);
		break;
	    }

	    /* save the original pointers in case alignment moves them */
	    memtmp = args012.sbuff;
	    memtmp1 = args012.rbuff;
	    
	    /* Possibly align the data buffer */
	    if (!bNoCache)
	    {
		if (bufalign != 0)
		{
		    args012.sbuff += (bufalign - (POINTER_TO_INT(args012.sbuff) % bufalign) + bufoffset) % bufalign;
		    /* args12.rbuff += (bufalign - ((MPI_Aint)args12.rbuff % bufalign) + bufoffset) % bufalign; */
		}
	    }
	    args012.rbuff += (bufalign - (POINTER_TO_INT(args012.rbuff) % bufalign) + bufoffset) % bufalign;
	    
	    if (g_nIproc == 0 && printopt)
	    {
		printf("%3d: %9d bytes %4d times --> ", n, args012.bufflen, nrepeat012);
		fflush(stdout);
	    }

	    for (itrial=0, ii=1; ii <= nrepeat012 && itrial < ntrials; ii <<= 1, itrial++)
	    {
		/* Finally, we get to transmit or receive and time */
		switch (g_proc_loc)
		{
		case LEFT_PROCESS:
		    bwdata012[n].t = LONGTIME;
		    t1 = 0;
		    for (i = 0; i < TRIALS; i++)
		    {
			if (bNoCache)
			{
			    if (bufalign != 0)
			    {
				args012.sbuff = memtmp + ((bufalign - (POINTER_TO_INT(args012.sbuff) % bufalign) + bufoffset) % bufalign);
				/* args012.rbuff = memtmp1 + ((bufalign - ((MPI_Aint)args012.rbuff % bufalign) + bufoffset) % bufalign); */
			    }
			    else
			    {
				args012.sbuff = memtmp;
				/* args012.rbuff = memtmp1; */
			    }
			}

			Sync012(&args012);
			t0 = MPI_Wtime();
			for (j = 0; j < nrepeat012; j++)
			{
			    MPI_Send(args012.sbuff, args012.bufflen, MPI_BYTE, args012.nbor, MSG_TAG_012, MPI_COMM_WORLD);
			    MPI_Recv(args012.rbuff, args012.bufflen, MPI_BYTE, args012.nbor, MSG_TAG_012, MPI_COMM_WORLD, &status);
			    if (bNoCache)
			    {
				args012.sbuff += args012.bufflen;
				/* args012.rbuff += args012.bufflen; */
			    }
			}
			t = (MPI_Wtime() - t0)/(2 * nrepeat012);

			t1 += t;
			bwdata012[n].t = MIN(bwdata012[n].t, t);
		    }
		    MPI_Bcast(&bwdata012[n].t, 1, MPI_DOUBLE, g_left_rank, MPI_COMM_WORLD);
		    break;
		case MIDDLE_PROCESS:
		    bwdata012[n].t = LONGTIME;
		    t1 = 0;
		    for (i = 0; i < TRIALS; i++)
		    {
			if (bNoCache)
			{
			    if (bufalign != 0)
			    {
				args012.sbuff = memtmp + ((bufalign - (POINTER_TO_INT(args012.sbuff) % bufalign) + bufoffset) % bufalign);
				/* args012.rbuff = memtmp1 + ((bufalign - ((MPI_Aint)args012.rbuff % bufalign) + bufoffset) % bufalign); */
			    }
			    else
			    {
				args012.sbuff = memtmp;
				/* args012.rbuff = memtmp1; */
			    }
			}

			Sync012(&args012);
			t0 = MPI_Wtime();

			/******* use the ii variable here !!! ******/

			for (j = 0; j <= nrepeat012-ii; j+=ii)
			{
			    for (k=0; k<ii; k++)
			    {
				MPI_Send(args012.sbuff, args012.bufflen, MPI_BYTE, args012.nbor2, MSG_TAG_012, MPI_COMM_WORLD);
				MPI_Recv(args012.rbuff, args012.bufflen, MPI_BYTE, args012.nbor2, MSG_TAG_012, MPI_COMM_WORLD, &status);
			    }
			    /* do the left process second because it does the timing and needs to include time to send to the right process. */
			    for (k=0; k<ii; k++)
			    {
				MPI_Recv(args012.rbuff, args012.bufflen, MPI_BYTE, args012.nbor, MSG_TAG_012, MPI_COMM_WORLD, &status);
				MPI_Send(args012.sbuff, args012.bufflen, MPI_BYTE, args012.nbor, MSG_TAG_012, MPI_COMM_WORLD);
			    }
			    if (bNoCache)
			    {
				args012.sbuff += args012.bufflen;
				/* args012.rbuff += args012.bufflen; */
			    }
			}
			j = nrepeat012 % ii;
			for (k=0; k < j; k++)
			{
			    MPI_Send(args012.sbuff, args012.bufflen, MPI_BYTE, args012.nbor2, MSG_TAG_012, MPI_COMM_WORLD);
			    MPI_Recv(args012.rbuff, args012.bufflen, MPI_BYTE, args012.nbor2, MSG_TAG_012, MPI_COMM_WORLD, &status);
			}
			/* do the left process second because it does the timing and needs to include time to send to the right process. */
			for (k=0; k < j; k++)
			{
			    MPI_Recv(args012.rbuff, args012.bufflen, MPI_BYTE, args012.nbor, MSG_TAG_012, MPI_COMM_WORLD, &status);
			    MPI_Send(args012.sbuff, args012.bufflen, MPI_BYTE, args012.nbor, MSG_TAG_012, MPI_COMM_WORLD);
			}
			t = (MPI_Wtime() - t0)/(2 * nrepeat012);
		    }
		    MPI_Bcast(&bwdata012[n].t, 1, MPI_DOUBLE, g_left_rank, MPI_COMM_WORLD);
		    break;
		case RIGHT_PROCESS:
		    bwdata012[n].t = LONGTIME;
		    t1 = 0;
		    for (i = 0; i < TRIALS; i++)
		    {
			if (bNoCache)
			{
			    if (bufalign != 0)
			    {
				args012.sbuff = memtmp + ((bufalign - (POINTER_TO_INT(args012.sbuff) % bufalign) + bufoffset) % bufalign);
				/* args012.rbuff = memtmp1 + ((bufalign - ((MPI_Aint)args012.rbuff % bufalign) + bufoffset) % bufalign); */
			    }
			    else
			    {
				args012.sbuff = memtmp;
				/* args012.rbuff = memtmp1; */
			    }
			}

			Sync012(&args012);
			t0 = MPI_Wtime();
			for (j = 0; j < nrepeat012; j++)
			{
			    MPI_Recv(args012.rbuff, args012.bufflen, MPI_BYTE, args012.nbor, MSG_TAG_012, MPI_COMM_WORLD, &status);
			    MPI_Send(args012.sbuff, args012.bufflen, MPI_BYTE, args012.nbor, MSG_TAG_012, MPI_COMM_WORLD);
			    if (bNoCache)
			    {
				args012.sbuff += args012.bufflen;
				/* args012.rbuff += args012.bufflen; */
			    }
			}
			t = (MPI_Wtime() - t0)/(2 * nrepeat012);
		    }
		    MPI_Bcast(&bwdata012[n].t, 1, MPI_DOUBLE, g_left_rank, MPI_COMM_WORLD);
		    break;
		}
		tlast012 = bwdata012[n].t;
		bwdata012[n].bits = args012.bufflen * CHARSIZE;
		bwdata012[n].bps = bwdata012[n].bits / (bwdata012[n].t * 1024 * 1024);
		bwdata012[n].repeat = nrepeat012;
		if (itrial < ntrials)
		{
		    dtrials[itrial] = MIN(dtrials[itrial], bwdata012[n].t);
		}

		if (g_nIproc == 0)
		{
		    if (bSavePert)
		    {
			fprintf(out, "\t%0.9f", bwdata012[n].t);
			fflush(out);
		    }
		    if (printopt)
		    {
			printf(" %0.9f", tlast012);
			fflush(stdout);
		    }
		}
	    }
	    if (g_nIproc == 0)
	    {
		if (bSavePert)
		{
		    fprintf(out, "\n");
		    fflush(out);
		}
		if (printopt)
		{
		    printf("\n");
		    fflush(stdout);
		}
	    }
	    
	    free(memtmp);
	    free(memtmp1);
#endif

#ifdef CREATE_SINGLE_CURVE
	    /*****************************************************/
	    /*         Run a trial between rank 0, 1 and 2       */
	    /*****************************************************/

	    MPI_Barrier(MPI_COMM_WORLD);


	    /* Calculate howmany times to repeat the experiment. */
	    if (g_nIproc == 0)
	    {
		if (args012.bufflen == 0)
		    nrepeat012 = g_latency012_reps;
		else
		    nrepeat012 = (int)(MAX((RUNTM / ((double)args012.bufflen /
			           (args012.bufflen - inc + 1.0) * tlast012)), TRIALS));
		MPI_Bcast(&nrepeat012, 1, MPI_INT, 0, MPI_COMM_WORLD);
	    }
	    else
	    {
		MPI_Bcast(&nrepeat012, 1, MPI_INT, 0, MPI_COMM_WORLD);
	    }

	    /* Allocate the buffer */
	    args012.bufflen = len + pert;
	    /* printf("allocating %d bytes\n", args12.bufflen * nrepeat012 + bufalign); */
	    if (bNoCache)
	    {
		if ((args012.sbuff = (char *)malloc(args012.bufflen * nrepeat012 + bufalign)) == (char *)NULL)
		{
		    fprintf(stdout,"Couldn't allocate memory\n");
		    fflush(stdout);
		    break;
		}
	    }
	    else
	    {
		if ((args012.sbuff = (char *)malloc(args012.bufflen + bufalign)) == (char *)NULL)
		{
		    fprintf(stdout,"Couldn't allocate memory\n");
		    fflush(stdout);
		    break;
		}
	    }
	    /* if ((args012.rbuff = (char *)malloc(args012.bufflen * nrepeat012 + bufalign)) == (char *)NULL) */
	    if ((args012.rbuff = (char *)malloc(args012.bufflen + bufalign)) == (char *)NULL)
	    {
		fprintf(stdout,"Couldn't allocate memory\n");
		fflush(stdout);
		break;
	    }

	    /* save the original pointers in case alignment moves them */
	    memtmp = args012.sbuff;
	    memtmp1 = args012.rbuff;
	    
	    /* Possibly align the data buffer */
	    if (!bNoCache)
	    {
		if (bufalign != 0)
		{
		    args012.sbuff += (bufalign - (POINTER_TO_INT(args012.sbuff) % bufalign) + bufoffset) % bufalign;
		    /* args12.rbuff += (bufalign - ((MPI_Aint)args12.rbuff % bufalign) + bufoffset) % bufalign; */
		}
	    }
	    args012.rbuff += (bufalign - (POINTER_TO_INT(args012.rbuff) % bufalign) + bufoffset) % bufalign;
	    
	    if (g_nIproc == 0 && printopt)
	    {
		printf("%3d: %9d bytes %4d times --> ", n, args012.bufflen, nrepeat012);
		fflush(stdout);
	    }
	    
	    /* Finally, we get to transmit or receive and time */
	    switch (g_proc_loc)
	    {
	    case LEFT_PROCESS:
		bwdata012[n].t = LONGTIME;
		t1 = 0;
		for (i = 0; i < TRIALS; i++)
		{
		    if (bNoCache)
		    {
			if (bufalign != 0)
			{
			    args012.sbuff = memtmp + ((bufalign - (POINTER_TO_INT(args012.sbuff) % bufalign) + bufoffset) % bufalign);
			    /* args012.rbuff = memtmp1 + ((bufalign - ((MPI_Aint)args012.rbuff % bufalign) + bufoffset) % bufalign); */
			}
			else
			{
			    args012.sbuff = memtmp;
			    /* args012.rbuff = memtmp1; */
			}
		    }
		    
		    Sync012(&args012);
		    t0 = MPI_Wtime();
		    for (j = 0; j < nrepeat012; j++)
		    {
			MPI_Send(args012.sbuff, args012.bufflen, MPI_BYTE, args012.nbor, MSG_TAG_012, MPI_COMM_WORLD);
			MPI_Recv(args012.rbuff, args012.bufflen, MPI_BYTE, args012.nbor, MSG_TAG_012, MPI_COMM_WORLD, &status);
			if (bNoCache)
			{
			    args012.sbuff += args012.bufflen;
			    /* args012.rbuff += args012.bufflen; */
			}
		    }
		    t = (MPI_Wtime() - t0)/(2 * nrepeat012);

		    t1 += t;
		    bwdata012[n].t = MIN(bwdata012[n].t, t);
		}
		MPI_Bcast(&bwdata012[n].t, 1, MPI_DOUBLE, g_left_rank, MPI_COMM_WORLD);
		break;
	    case MIDDLE_PROCESS:
		bwdata012[n].t = LONGTIME;
		t1 = 0;
		for (i = 0; i < TRIALS; i++)
		{
		    if (bNoCache)
		    {
			if (bufalign != 0)
			{
			    args012.sbuff = memtmp + ((bufalign - (POINTER_TO_INT(args012.sbuff) % bufalign) + bufoffset) % bufalign);
			    /* args012.rbuff = memtmp1 + ((bufalign - ((MPI_Aint)args012.rbuff % bufalign) + bufoffset) % bufalign); */
			}
			else
			{
			    args012.sbuff = memtmp;
			    /* args012.rbuff = memtmp1; */
			}
		    }
		    
		    Sync012(&args012);
		    t0 = MPI_Wtime();
		    for (j = 0; j < nrepeat012; j++)
		    {
			MPI_Recv(args012.rbuff, args012.bufflen, MPI_BYTE, args012.nbor, MSG_TAG_012, MPI_COMM_WORLD, &status);
			MPI_Send(args012.sbuff, args012.bufflen, MPI_BYTE, args012.nbor2, MSG_TAG_012, MPI_COMM_WORLD);
			MPI_Recv(args012.rbuff, args012.bufflen, MPI_BYTE, args012.nbor2, MSG_TAG_012, MPI_COMM_WORLD, &status);
			MPI_Send(args012.sbuff, args012.bufflen, MPI_BYTE, args012.nbor, MSG_TAG_012, MPI_COMM_WORLD);
			if (bNoCache)
			{
			    args012.sbuff += args012.bufflen;
			    /* args012.rbuff += args012.bufflen; */
			}
		    }
		    t = (MPI_Wtime() - t0)/(2 * nrepeat012);
		}
		MPI_Bcast(&bwdata012[n].t, 1, MPI_DOUBLE, g_left_rank, MPI_COMM_WORLD);
		break;
	    case RIGHT_PROCESS:
		bwdata012[n].t = LONGTIME;
		t1 = 0;
		for (i = 0; i < TRIALS; i++)
		{
		    if (bNoCache)
		    {
			if (bufalign != 0)
			{
			    args012.sbuff = memtmp + ((bufalign - (POINTER_TO_INT(args012.sbuff) % bufalign) + bufoffset) % bufalign);
			    /* args012.rbuff = memtmp1 + ((bufalign - ((MPI_Aint)args012.rbuff % bufalign) + bufoffset) % bufalign); */
			}
			else
			{
			    args012.sbuff = memtmp;
			    /* args012.rbuff = memtmp1; */
			}
		    }
		    
		    Sync012(&args012);
		    t0 = MPI_Wtime();
		    for (j = 0; j < nrepeat012; j++)
		    {
			MPI_Recv(args012.rbuff, args012.bufflen, MPI_BYTE, args012.nbor, MSG_TAG_012, MPI_COMM_WORLD, &status);
			MPI_Send(args012.sbuff, args012.bufflen, MPI_BYTE, args012.nbor, MSG_TAG_012, MPI_COMM_WORLD);
			if (bNoCache)
			{
			    args012.sbuff += args012.bufflen;
			    /* args012.rbuff += args012.bufflen; */
			}
		    }
		    t = (MPI_Wtime() - t0)/(2 * nrepeat012);
		}
		MPI_Bcast(&bwdata012[n].t, 1, MPI_DOUBLE, g_left_rank, MPI_COMM_WORLD);
		break;
	    }
	    tlast012 = bwdata012[n].t;
	    bwdata012[n].bits = args012.bufflen * CHARSIZE;
	    bwdata012[n].bps = bwdata012[n].bits / (bwdata012[n].t * 1024 * 1024);
	    bwdata012[n].repeat = nrepeat012;

	    if (g_nIproc == 0)
	    {
		if (bSavePert)
		{
		    if (bUseMegaBytes)
			fprintf(out, "%f\t%0.9f\n", bwdata012[n].bps / 8, bwdata012[n].t);
		    else
			fprintf(out, "%f\t%0.9f\n", bwdata012[n].bps, bwdata012[n].t);
		    fflush(out);
		}
	    }
	    
	    free(memtmp);
	    free(memtmp1);
	    
	    if (g_nIproc == 0 && printopt)
	    {
		if (bUseMegaBytes)
		    printf(" %6.2f MBps in %0.9f sec\n", bwdata012[n].bps / 8, tlast012);
		else
		    printf(" %6.2f Mbps in %0.9f sec\n", bwdata012[n].bps, tlast012);
		fflush(stdout);
	    }
#endif

	} /* End of perturbation loop */

	if (!bSavePert)/* && g_nIproc == 0)*/
	{
	    /* if we didn't save all of the perturbation loops, find the max and save it */
	    int index01 = 1, index12 = 1;
	    double dmax01 = bwdata01[n-1].bps;
	    double dmax12 = bwdata12[n-1].bps;
#ifdef CREATE_SINGLE_CURVE
	    int index012 = 1;
	    double dmax012 = bwdata012[n-1].bps;
#endif
	    for (; ipert > 1; ipert--)
	    {
		if (bwdata01[n-ipert].bps > dmax01)
		{
		    index01 = ipert;
		    dmax01 = bwdata01[n-ipert].bps;
		}
		if (bwdata12[n-ipert].bps > dmax12)
		{
		    index12 = ipert;
		    dmax12 = bwdata12[n-ipert].bps;
		}
#ifdef CREATE_SINGLE_CURVE
		if (bwdata012[n-ipert].bps > dmax012)
		{
		    index012 = ipert;
		    dmax012 = bwdata012[n-ipert].bps;
		}
#endif
	    }
	    /* get the left stuff out */
	    MPI_Bcast(&index01, 1, MPI_INT, g_left_rank, MPI_COMM_WORLD);
	    MPI_Bcast(&bwdata01[n-index01].bits, 1, MPI_INT, g_left_rank, MPI_COMM_WORLD);
	    MPI_Bcast(&bwdata01[n-index01].bps, 1, MPI_DOUBLE, g_left_rank, MPI_COMM_WORLD);
	    MPI_Bcast(&bwdata01[n-index01].t, 1, MPI_DOUBLE, g_left_rank, MPI_COMM_WORLD);
	    /* get the right stuff out */
	    MPI_Bcast(&index12, 1, MPI_INT, g_middle_rank, MPI_COMM_WORLD);
	    MPI_Bcast(&bwdata12[n-index12].bps, 1, MPI_DOUBLE, g_middle_rank, MPI_COMM_WORLD);
	    MPI_Bcast(&bwdata12[n-index12].t, 1, MPI_DOUBLE, g_middle_rank, MPI_COMM_WORLD);
	    if (g_nIproc == 0)
	    {
		if (bUseMegaBytes)
		{
		    fprintf(out, "%d\t%f\t%0.9f\t", bwdata01[n-index01].bits / 8, bwdata01[n-index01].bps / 8, bwdata01[n-index01].t);
		    fprintf(out, "%f\t%0.9f\t", bwdata12[n-index12].bps / 8, bwdata12[n-index12].t);
#ifdef CREATE_SINGLE_CURVE
		    fprintf(out, "%f\t%0.9f\n", bwdata012[n-index012].bps / 8, bwdata012[n-index012].t);
#endif
		}
		else
		{
		    fprintf(out, "%d\t%f\t%0.9f\t", bwdata01[n-index01].bits / 8, bwdata01[n-index01].bps, bwdata01[n-index01].t);
		    fprintf(out, "%f\t%0.9f\t", bwdata12[n-index12].bps, bwdata12[n-index12].t);
#ifdef CREATE_SINGLE_CURVE
		    fprintf(out, "%f\t%0.9f\n", bwdata012[n-index012].bps, bwdata012[n-index012].t);
#endif
		}
#ifdef CREATE_DIFFERENCE_CURVES
		for (itrial = 0; itrial < ntrials && dtrials[itrial] != LONGTIME; itrial++)
		{
		    fprintf(out, "%0.9f\t", dtrials[itrial]);
		}
		fprintf(out, "\n");
#endif
		fflush(out);
	    }
	}
    } /* End of main loop  */
	
    if (g_nIproc == 0)
	fclose(out);
    /* THE_END:		 */
    MPI_Finalize();
    free(bwdata01);
    free(bwdata12);
    free(bwdata012);
    return 0;
}

int Setup(int middle_rank, ArgStruct *p01, ArgStruct *p12, ArgStruct *p012)
{
    char s[255];
    int len = 255;
    
    p01->iproc = p12->iproc = p012->iproc = g_nIproc;
    
    MPI_Get_processor_name(s, &len);
    /*gethostname(s, len);*/
    printf("%d: %s\n", p01->iproc, s);
    fflush(stdout);

    switch (middle_rank)
    {
    case 0:
	switch (g_nIproc)
	{
	case 0:
	    g_proc_loc = MIDDLE_PROCESS;
	    p01->nbor = 2;
	    p01->tr = FALSE;
	    p12->nbor = 1;
	    p12->tr = TRUE;
	    p012->nbor = 2;
	    p012->nbor2 = 1;
	    break;
	case 1:
	    g_proc_loc = RIGHT_PROCESS;
	    p01->nbor = -1;
	    p01->tr = FALSE;
	    p12->nbor = 0;
	    p12->tr = FALSE;
	    p012->nbor = 0;
	    p012->nbor2 = -1;
	    break;
	case 2:
	    g_proc_loc = LEFT_PROCESS;
	    p01->nbor = 0;
	    p01->tr = TRUE;
	    p12->nbor = -1;
	    p12->tr = FALSE;
	    p012->nbor = 0;
	    p012->nbor2 = -1;
	    break;
	}
	g_left_rank = 2;
	g_middle_rank = 0;
	g_right_rank = 1;
	break;
    case 1:
	switch (g_nIproc)
	{
	case 0:
	    g_proc_loc = LEFT_PROCESS;
	    p01->nbor = 1;
	    p01->tr = TRUE;
	    p12->nbor = -1;
	    p12->tr = FALSE;
	    p012->nbor = 1;
	    p012->nbor2 = -1;
	    break;
	case 1:
	    g_proc_loc = MIDDLE_PROCESS;
	    p01->nbor = 0;
	    p01->tr = FALSE;
	    p12->nbor = 2;
	    p12->tr = TRUE;
	    p012->nbor = 0;
	    p012->nbor2 = 2;
	    break;
	case 2:
	    g_proc_loc = RIGHT_PROCESS;
	    p01->nbor = -1;
	    p01->tr = FALSE;
	    p12->nbor = 1;
	    p12->tr = FALSE;
	    p012->nbor = 1;
	    p012->nbor2 = -1;
	    break;
	}
	g_left_rank = 0;
	g_middle_rank = 1;
	g_right_rank = 2;
	break;
    case 2:
	switch (g_nIproc)
	{
	case 0:
	    g_proc_loc = RIGHT_PROCESS;
	    p01->nbor = -1;
	    p01->tr = FALSE;
	    p12->nbor = 2;
	    p12->tr = FALSE;
	    p012->nbor = 2;
	    p012->nbor2 = -1;
	    break;
	case 1:
	    g_proc_loc = LEFT_PROCESS;
	    p01->nbor = 2;
	    p01->tr = TRUE;
	    p12->nbor = -1;
	    p12->tr = FALSE;
	    p012->nbor = 2;
	    p012->nbor2 = -1;
	    break;
	case 2:
	    g_proc_loc = MIDDLE_PROCESS;
	    p01->nbor = 1;
	    p01->tr = FALSE;
	    p12->nbor = 0;
	    p12->tr = TRUE;
	    p012->nbor = 1;
	    p012->nbor2 = 0;
	    break;
	}
	g_left_rank = 1;
	g_middle_rank = 2;
	g_right_rank = 0;
	break;
    }

    return 1;	
}	

void Sync(ArgStruct *p)
{
    MPI_Status status;
    if (p->tr)
    {
	MPI_Send(NULL, 0, MPI_BYTE, p->nbor, 1, MPI_COMM_WORLD);
	MPI_Recv(NULL, 0, MPI_BYTE, p->nbor, 1, MPI_COMM_WORLD, &status);
	MPI_Send(NULL, 0, MPI_BYTE, p->nbor, 1, MPI_COMM_WORLD);
    }
    else
    {
	MPI_Recv(NULL, 0, MPI_BYTE, p->nbor, 1, MPI_COMM_WORLD, &status);
	MPI_Send(NULL, 0, MPI_BYTE, p->nbor, 1, MPI_COMM_WORLD);
	MPI_Recv(NULL, 0, MPI_BYTE, p->nbor, 1, MPI_COMM_WORLD, &status);
    }
}

void Sync012(ArgStruct *p)
{
    MPI_Status status;
    switch (g_proc_loc)
    {
    case LEFT_PROCESS:
	MPI_Send(NULL, 0, MPI_BYTE, p->nbor, 1, MPI_COMM_WORLD);
	MPI_Recv(NULL, 0, MPI_BYTE, p->nbor, 1, MPI_COMM_WORLD, &status);
	MPI_Send(NULL, 0, MPI_BYTE, p->nbor, 1, MPI_COMM_WORLD);
	break;
    case MIDDLE_PROCESS:
	MPI_Recv(NULL, 0, MPI_BYTE, p->nbor, 1, MPI_COMM_WORLD, &status);
	MPI_Send(NULL, 0, MPI_BYTE, p->nbor2, 1, MPI_COMM_WORLD);
	MPI_Recv(NULL, 0, MPI_BYTE, p->nbor2, 1, MPI_COMM_WORLD, &status);
	MPI_Send(NULL, 0, MPI_BYTE, p->nbor, 1, MPI_COMM_WORLD);
	MPI_Recv(NULL, 0, MPI_BYTE, p->nbor, 1, MPI_COMM_WORLD, &status);
	MPI_Send(NULL, 0, MPI_BYTE, p->nbor2, 1, MPI_COMM_WORLD);
	break;
    case RIGHT_PROCESS:
	MPI_Recv(NULL, 0, MPI_BYTE, p->nbor, 1, MPI_COMM_WORLD, &status);
	MPI_Send(NULL, 0, MPI_BYTE, p->nbor, 1, MPI_COMM_WORLD);
	MPI_Recv(NULL, 0, MPI_BYTE, p->nbor, 1, MPI_COMM_WORLD, &status);
	break;
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
    t0 = MPI_Wtime();
    t0 = MPI_Wtime();
    t0 = MPI_Wtime();
    while ( (duration < RUNTM) ||
	    (duration < MAX_LAT_TIME && reps < 1000))
    {
	t0 = MPI_Wtime();
	for (i=0; i<reps-prev_reps; i++)
	{
	    Sync(p);
	}
	duration += MPI_Wtime() - t0;
	prev_reps = reps;
	reps = reps * 2;

	/* use duration from the root only */
	if (p->tr)
	    MPI_Send(&duration, 1, MPI_DOUBLE, p->nbor, 2, MPI_COMM_WORLD);
	else
	    MPI_Recv(&duration, 1, MPI_DOUBLE, p->nbor, 2, MPI_COMM_WORLD, &status);
    }

    return reps;
}

int DetermineLatencyReps012(ArgStruct *p)
{
    double t0, duration = 0;
    int reps = 1, prev_reps = 0;
    int i;

    /* prime the send/receive pipes */
    Sync012(p);
    Sync012(p);
    Sync012(p);

    /* test how long it takes to send n messages 
     * where n = 1, 2, 4, 8, 16, 32, ...
     */
    t0 = MPI_Wtime();
    t0 = MPI_Wtime();
    t0 = MPI_Wtime();
    while ( (duration < RUNTM) ||
	    (duration < MAX_LAT_TIME && reps < 1000))
    {
	t0 = MPI_Wtime();
	for (i=0; i<reps-prev_reps; i++)
	{
	    Sync012(p);
	}
	duration += MPI_Wtime() - t0;
	prev_reps = reps;
	reps = reps * 2;

	/* use duration from the root only */
	MPI_Bcast(&duration, 1, MPI_DOUBLE, g_left_rank, MPI_COMM_WORLD);
    }

    return reps;
}

double TestLatency(ArgStruct *p)
{
    double latency, t0, min_latency = LONGTIME;
    int i, j;
    MPI_Status status;
    char str[100];

    /* calculate the latency between rank 0 and rank 1 */
    p->latency_reps = DetermineLatencyReps(p);
    if (/*p->latency_reps < 1024 &&*/ p->tr)
    {
	if (g_proc_loc == LEFT_PROCESS)
	{
	    sprintf(str, "%d <-> %d      ", p->iproc, p->nbor);
	}
	else
	{
	    sprintf(str, "      %d <-> %d", p->iproc, p->nbor);
	}
	/*printf("To determine %s latency, using %d reps\n", p->iproc == 0 ? "0 -> 1     " : "     1 -> 2", p->latency_reps);*/
	printf("To determine %s latency, using %d reps.\n", str, p->latency_reps);
	fflush(stdout);
    }

    for (j=0; j<TRIALS; j++)
    {
	Sync(p);
	t0 = MPI_Wtime();
	t0 = MPI_Wtime();
	t0 = MPI_Wtime();
	t0 = MPI_Wtime();
	for (i = 0; i < p->latency_reps; i++)
	{
	    if (p->tr)
	    {
		MPI_Send(NULL, 0, MPI_BYTE, p->nbor, 1, MPI_COMM_WORLD);
		MPI_Recv(NULL, 0, MPI_BYTE, p->nbor, 1, MPI_COMM_WORLD, &status);
	    }
	    else
	    {
		MPI_Recv(NULL, 0, MPI_BYTE, p->nbor, 1, MPI_COMM_WORLD, &status);
		MPI_Send(NULL, 0, MPI_BYTE, p->nbor, 1, MPI_COMM_WORLD);
	    }
	}
	latency = (MPI_Wtime() - t0)/(2 * p->latency_reps);
	min_latency = MIN(min_latency, latency);
    }

    return min_latency;
}

double TestLatency012(ArgStruct *p)
{
    double latency, t0, min_latency = LONGTIME;
    int i, j;
    MPI_Status status;

    g_latency012_reps = DetermineLatencyReps012(p);
    if (g_proc_loc == MIDDLE_PROCESS)
    {
	printf("To determine %d <-- %d --> %d latency, using %d reps\n", p->nbor, p->iproc, p->nbor2, g_latency012_reps);
	fflush(stdout);
    }

    for (j=0; j<TRIALS; j++)
    {
	Sync012(p);
	t0 = MPI_Wtime();
	t0 = MPI_Wtime();
	t0 = MPI_Wtime();
	t0 = MPI_Wtime();
	for (i = 0; i < g_latency012_reps; i++)
	{
	    switch (g_proc_loc)
	    {
	    case LEFT_PROCESS:
		MPI_Send(NULL, 0, MPI_BYTE, p->nbor, 1, MPI_COMM_WORLD);
		MPI_Recv(NULL, 0, MPI_BYTE, p->nbor, 1, MPI_COMM_WORLD, &status);
		break;
	    case MIDDLE_PROCESS:
		MPI_Recv(NULL, 0, MPI_BYTE, p->nbor, 1, MPI_COMM_WORLD, &status);
		MPI_Send(NULL, 0, MPI_BYTE, p->nbor2, 1, MPI_COMM_WORLD);
		MPI_Recv(NULL, 0, MPI_BYTE, p->nbor2, 1, MPI_COMM_WORLD, &status);
		MPI_Send(NULL, 0, MPI_BYTE, p->nbor, 1, MPI_COMM_WORLD);
		break;
	    case RIGHT_PROCESS:
		MPI_Recv(NULL, 0, MPI_BYTE, p->nbor, 1, MPI_COMM_WORLD, &status);
		MPI_Send(NULL, 0, MPI_BYTE, p->nbor, 1, MPI_COMM_WORLD);
		break;
	    }
	}
	latency = (MPI_Wtime() - t0)/(2 * g_latency012_reps);
	min_latency = MIN(min_latency, latency);
    }

    return min_latency;
}

void SendTime(ArgStruct *p, double *t)
{
    MPI_Send(t, 1, MPI_DOUBLE, p->nbor, 2, MPI_COMM_WORLD);
}

void RecvTime(ArgStruct *p, double *t)
{
    MPI_Status status;
    MPI_Recv(t, 1, MPI_DOUBLE, p->nbor, 2, MPI_COMM_WORLD, &status);
}

void SendReps(ArgStruct *p, int *rpt)
{
    MPI_Send(rpt, 1, MPI_INT, p->nbor, 2, MPI_COMM_WORLD);
}

void RecvReps(ArgStruct *p, int *rpt)
{
    MPI_Status status;
    MPI_Recv(rpt, 1, MPI_INT, p->nbor, 2, MPI_COMM_WORLD, &status);
}
