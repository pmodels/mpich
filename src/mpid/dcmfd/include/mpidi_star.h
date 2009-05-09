/*  (C)Copyright IBM Corp.  2007, 2008  */
/**
 * \file include/mpidi_star.h
 * \brief ???
 */
/******************************************************************************

Copyright (c) 2006, Ahmad Faraj & Xin Yuan,
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

  * Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer.

  * Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.

  * Neither the name of the Florida State University nor the names of its
    contributors may be used to endorse or promote products derived from this
    software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

  ***************************************************************************
  *     Any results obtained from executing this software require the       *
  *     acknowledgment and citation of the software and its owners.         *
  *     The full citation is given below:                                   *
  *                                                                         *
  *     A. Faraj, X. Yuan, and D. Lowenthal. "STAR-MPI: Self Tuned Adaptive *
  *     Routines for MPI Collective Operations." The 20th ACM International *
  *     Conference on Supercomputing (ICS), Queensland, Australia           *
  *     June 28-July 1, 2006.                                               *
  ***************************************************************************

******************************************************************************/

/*
  NOTE:
  STAR-MPI is not supported in threaded mode yet.
 */

#ifndef _MPIDI_STAR_H_
#define _MPIDI_STAR_H_

#include <dcmf.h>
#include <mpiimpl.h>
#include <defines.h>
#include <execinfo.h>


#define STAR_ROUTINES 10
#define STAR_FAILURE 1


/*
   if STAR is on by default, library developer can use these thresholds to
   decide when to enable star. I.E. if message size is more than 512, star is
   on, otherwise, it is off.
*/

#define STAR_ALLOC(x,t,c) (x = (t *) MPIU_Malloc(sizeof(t) * c))


typedef enum {NO_CALL = 0, ALLTOALL_CALL, ALLGATHER_CALL, ALLGATHERV_CALL,
              ALLREDUCE_CALL, BCAST_CALL, REDUCE_CALL, GATHER_CALL,
	      SCATTER_CALL, BARRIER_CALL} STAR_MPI_Call;

/* MPIDO-Like function pointers for major collectives */
typedef int (* bcast_fptr)     (void *, int, int, MPID_Comm *);

typedef int (* allreduce_fptr) (void *, void *, int, DCMF_Dt, DCMF_Op,
				MPI_Datatype, MPID_Comm *);

typedef int (* alltoall_fptr)  (void *, int, MPI_Datatype,
                                void *, int, MPI_Datatype,
                                MPID_Comm *);

typedef int (* allgather_fptr) (void *, int, MPI_Datatype,
				void *, int, MPI_Datatype,
				MPI_Aint, MPI_Aint,
				size_t, size_t, MPID_Comm *);

typedef int (* allgatherv_fptr) (void *, int, MPI_Datatype,
				 void *, int *, int, int *, MPI_Datatype,
				 MPI_Aint, MPI_Aint,
				 size_t, size_t, MPID_Comm *);

typedef int (* reduce_fptr) (void *, void *, int, DCMF_Dt, DCMF_Op,
			     MPI_Datatype, int, MPID_Comm *);

typedef int (* gather_fptr) (void *, int, MPI_Datatype,
                             void *, int, MPI_Datatype,
                             int, MPID_Comm *);

typedef int (* scatter_fptr) (void *, int, MPI_Datatype,
                              void *, int, MPI_Datatype,
                              int, MPID_Comm *);

typedef int (* barrier_fptr) (MPID_Comm *);

/*
   to save on variables,
   we use a union of function pointers for diff collectives
*/
typedef union
{
  alltoall_fptr   alltoall_func;
  allgather_fptr  allgather_func;
  allgatherv_fptr allgatherv_func;
  allreduce_fptr  allreduce_func;
  barrier_fptr    barrier_func;
  bcast_fptr      bcast_func;
  reduce_fptr     reduce_func;
  gather_fptr     gather_func;
  scatter_fptr    scatter_func;
} STAR_Func_Ptr;

/*
  this creates a struct of algorithms. This structure shall contain a list of
  algorithms for a given collective operation.
 */
typedef struct
{
  STAR_Func_Ptr func; /* a pointer to the function performing the op */

  /* [0] minimum number of processes this algorithm works with */
  /* [1] max number of processes this algorithm works with */
  unsigned int nprocs[2];

  /* [0] minimum number of bytes this algorithm works with */
  /* [1] maximum number of bytes this algorithm works with */
  unsigned int bytes[2];

  /* stores properties of a given algorithm in bits pattern variable */
  MPIDO_Embedded_Info_Set properties;

  char name[64]; /* name of alltoall, allgather, bcast, ... algorithm */
} STAR_Algorithm;

typedef struct
{
  /* type of collective operation: bcast, reduce, ..etc */
  STAR_MPI_Call call_type;

  MPID_Comm * comm; /* communicator used in a given collective call site */
  unsigned bytes; /* number of bytes to be communicated in the call site */
  int id; /* identifier for a given call site, set by trace_back */
  int op_type_support; /* code represinting if there's support for op/type */
  char buff_attributes[4];
} STAR_Callsite;

typedef struct STAR_Tuning_Session
{
  /* a ptr to collective repository for an op */
  STAR_Algorithm * repository;

  /* communicator used for the particular call site */
  MPID_Comm * comm;

  /* type of MPI routine: Bcast, reduce, ..etc */
  STAR_MPI_Call call_type;
  int handle;
  int callsite_id; /* identifier for the particular call site */
  int op_type_support; /* code represinting if there's support for op/type */
  unsigned np; /* records the number of processes involved in this call site */
  unsigned bytes; /* message size in bytes */
  unsigned total_tuned_algorithms; /* counts the number of algorithms tuned */
  unsigned tuning_calls; /* counts number of invocs algorithms are tuned for */
  unsigned total_examined; /* counts what algorithms have been tried so far */
  int curr_alg_index; /* an index pointing to current algorithm in use */
  int best_alg_index; /* index pointing to best algorithm when found */

  /* counts number of invocations in monitor phase for a tuning session */
  unsigned monitor_calls;

  /* monitoring factor, set to 2, and then doubles each time an algorithm is
     decided to be kept */
  unsigned factor;

  /* checks to see whether try a new algorithm or keep using current one */
  unsigned char get_next_alg;

  unsigned char panic;

  unsigned char one_at_least; /* at least one algorithm is available */

  unsigned skip; /* indicates a skipping of a measurement record */

  unsigned post_tuning_calls; /* counts number of calls in the monitor phase */

  double initial_time; /* time at start of a tuning session */
  /*
     [0] holds ave of time measures of an algorithm over all monitor calls
     [1] holds ave of time measures of an algorithm over last
     INVOCS_PER_ALGORITHM
  */
  double max[4];

  double tune_overhead; /* measure time overhead in tuning phase */
  double monitor_overhead; /* measures time overhead in monitor phase */
  double comm_overhead; /* measures only comm. time to execute an algorithm  */

  /* measures only total comm. time for executing best algorithm over all
     phases post the initial tuning phase */
  double post_tuning_time;

  /* records current invocation number / trial of an algorithm */
  unsigned * curr_invoc;

  /* array to record timing measurement per algorithm per invocation */
  double * time;

  /* records each algorithm best time */
  double * algorithms_times;

  /* sorts algorithms based on timing results */
  int * best;

  struct STAR_Tuning_Session * next; /* pointer to next tuning session */
} STAR_Tuning_Session;


typedef struct
{
  /* env variable setting the use of STAR */
  int enabled;

  int alltoall_threshold; /* min message size to have star kick in */
  int allgather_threshold; /* min message size to have star kick in */
  int allgatherv_threshold; /* min message size to have star kick in */
  int allreduce_threshold; /* min message size to have star kick in */
  int reduce_threshold; /* min message size to have star kick in */
  int bcast_threshold; /* min message size to have star kick in */
  int gather_threshold; /* min message size to have star kick in */
  int scatter_threshold; /* min message size to have star kick in */
                       
  /* flag to indicate where is the control coming from, App or within BG lib */
  unsigned char internal_control_flow;

  unsigned char agree_on_callsite;

  /* env variable setting the levels to trace back a call */
  int traceback_levels;
  int debug;

  int bcast_algorithms;
  int barrier_algorithms;
  int alltoall_algorithms;
  int allreduce_algorithms;
  int allgather_algorithms;
  int allgatherv_algorithms;
  int reduce_algorithms;
  int scatter_algorithms;
  int gather_algorithms;

  unsigned invocs_per_algorithm;

  char mpis[STAR_ROUTINES][20];

} STAR_Info;

extern STAR_Info STAR_info;
extern FILE * MPIDO_STAR_fd;
extern char * MPID_Executable_name;

extern STAR_Algorithm * repository_ptr;
extern STAR_Algorithm * STAR_bcast_repository;
extern STAR_Algorithm * STAR_allreduce_repository;
extern STAR_Algorithm * STAR_reduce_repository;
extern STAR_Algorithm * STAR_allgather_repository;
extern STAR_Algorithm * STAR_allgatherv_repository;
extern STAR_Algorithm * STAR_alltoall_repository;
extern STAR_Algorithm * STAR_gather_repository;
extern STAR_Algorithm * STAR_scatter_repository;
extern STAR_Algorithm * STAR_barrier_repository;


extern void STAR_InitRepositories();
extern void STAR_FreeMem();

int  STAR_DisplayStatistics(MPID_Comm *);

void STAR_AssignBestAlgorithm(STAR_Tuning_Session *);
void STAR_NextAlgorithm(STAR_Tuning_Session *,
			STAR_Callsite *, int);
int  STAR_SortAlgorithms(STAR_Tuning_Session *);
void STAR_CheckPerformanceOfBestAlg(STAR_Tuning_Session *);
void STAR_ComputePerformance(STAR_Tuning_Session *,
			     double *,
			     int);
STAR_Tuning_Session *
STAR_GetTuningSession(STAR_Callsite *,
		      STAR_Algorithm *,
		      int);

STAR_Tuning_Session *
STAR_AssembleSession(STAR_Callsite *,
		     STAR_Algorithm *,
		     int);

void STAR_CreateSession(STAR_Tuning_Session **,
			STAR_Callsite *,
			STAR_Algorithm *, int);

int  STAR_ProcessMonitorPhase(STAR_Tuning_Session *, double);
void STAR_ProcessTuningPhase(STAR_Tuning_Session *, double);

int STAR_Bcast(char *,
	       int,
	       STAR_Callsite *,
	       STAR_Algorithm *,
	       int);

int STAR_Reduce(void * sbuff,
		void * rbuff,
		int count,
		DCMF_Dt dcmf_data,
		DCMF_Op dcmf_op,
		MPI_Datatype datatype,
		int root,
		STAR_Callsite * collective_site,
		STAR_Algorithm * repo,
		int total_algs);

int STAR_Allreduce(void *,
		   void *,
		   int,
		   DCMF_Dt,
		   DCMF_Op,
		   MPI_Datatype,
		   STAR_Callsite *,
		   STAR_Algorithm *,
		   int);

int STAR_Scatter(void * sendbuf,
		 int sendcount,
		 MPI_Datatype sendtype,
		 void * recvbuf,
		 int recvcount,
		 MPI_Datatype recvtype,
		 int root,
		 STAR_Callsite * collective_site,
		 STAR_Algorithm * repo,
		 int total_algs);

int STAR_Gather(void * sendbuf,
		int sendcount,
		MPI_Datatype sendtype,
		void * recvbuf,
		int recvcount,
		MPI_Datatype recvtype,
		int root,
		STAR_Callsite * collective_site,
		STAR_Algorithm * repo,
		int total_algs);

int STAR_Allgather(void * sendbuf,
		   int sendcount,
		   MPI_Datatype sendtype,
		   void * recvbuf,
		   int recvcount,
		   MPI_Datatype recvtype,
		   MPI_Aint send_true_lb,
		   MPI_Aint recv_true_lb,
		   size_t send_size,
		   size_t recv_size,
		   STAR_Callsite * collective_site,
		   STAR_Algorithm * repo,
		   int total_algs);

int STAR_Allgatherv(void * sendbuf,
		    int sendcount,
		    MPI_Datatype sendtype,
		    void * recvbuf,
		    int * recvcounts,
		    int buffer_sum,
		    int * displs,
		    MPI_Datatype recvtype,
		    MPI_Aint send_true_lb,
		    MPI_Aint recv_true_lb,
		    size_t send_size,
		    size_t recv_size,
		    STAR_Callsite * collective_site,
		    STAR_Algorithm * repo,
		    int total_algs);

int STAR_Alltoall(void * sendbuf,
		  int sendcount,
		  MPI_Datatype sendtype,
		  void * recvbuf,
		  int recvcount,
		  MPI_Datatype recvtype,
		  STAR_Callsite * collective_site,
		  STAR_Algorithm * repo,
		  int total_algs);
#endif
