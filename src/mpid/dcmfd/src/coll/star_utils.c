/*  (C)Copyright IBM Corp.  2007, 2008  */
/**
 * \file src/coll/STAR_utils.c
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
    Author: Ahmad Faraj
    contact: faraja@us.ibm.com
    date: 4/1/08
  */

#include "mpidi_star.h"
#include "mpido_coll.h"

FILE * MPIDO_STAR_fd = NULL;
char comm_shape_str[3][8] = {"COMWRLD", "RECT", "IRREG"};

void
STAR_AssignBestAlgorithm(STAR_Tuning_Session * session)
{
  STAR_MPI_Call call_type;
  int i, np, rank, bytes, total_algs, * best;
  double * algorithms_times;
  double * tmp;
  char * MPI;

  call_type = session->call_type;
  bytes = session->bytes;
  np = session->comm->local_size;
  rank = session->comm->rank;
  best = session->best;
  total_algs = session->total_examined;
  algorithms_times = session->algorithms_times;
  MPI = STAR_info.mpis[call_type];

  STAR_ALLOC(tmp, double, (2 + total_algs * STAR_info.invocs_per_algorithm));

  session->time[STAR_info.invocs_per_algorithm * total_algs] =
    session->tune_overhead;
  session->time[STAR_info.invocs_per_algorithm * total_algs + 1] =
    session->comm_overhead;

  /* compute the max of all algorithms time among all processes */
  MPIDO_Allreduce(session->time, tmp,
		  2 + STAR_info.invocs_per_algorithm * total_algs,
		  MPI_DOUBLE, MPI_MAX, session->comm);
  
  session->tune_overhead = tmp[STAR_info.invocs_per_algorithm * total_algs] /
                           session->tuning_calls;
  session->comm_overhead = tmp[STAR_info.invocs_per_algorithm * total_algs+1] /
                           session->tuning_calls;

#ifdef VBL2
  if (MPIDO_STAR_fd)
    fprintf(MPIDO_STAR_fd, 
	    "\n%s: comm: %s np: %d msize: %d performance overview\n", 
	    MPI, comm_shape_str[session -> comm -> dcmf.comm_shape],
            np, bytes);
#endif

  for (i = 0; i < total_algs; i++)
  {
    /* these are precautions steps :) */
    best[i] = -1;
    algorithms_times[i] = 0.0;

#ifdef VBL2
    if (MPIDO_STAR_fd)
    {
      fprintf(MPIDO_STAR_fd, "\n\n");
    }
#endif
    
    /* compute the performance of this algorithm */
    STAR_ComputePerformance(session, tmp, i);
  }

  /*
    once we compute performance of all algorithms, the next step below is to
    sort them and then set the index of the best algorithm to the first
    algorithm on the sorted list pointed by best[0].
  */

  STAR_SortAlgorithms(session);
  session->best_alg_index = session->best[0];

#ifdef VBL2  
  if (MPIDO_STAR_fd)
  {
    MPI = STAR_info.mpis[call_type];
    fprintf(MPIDO_STAR_fd, 
            "\nBest-algorithm: %s comm: %s np: %d msize: %d elapsed: "
            "%.2lf\n\n", 
            session -> repository[session -> best_alg_index].name, 
            comm_shape_str[session -> comm -> dcmf.comm_shape], np,
            bytes,
            1.0E6 * session->algorithms_times[session->best[0]]);
  }
#endif

  MPIU_Free(tmp);
}


void
STAR_CreateTuningSession(STAR_Tuning_Session ** ptr,
			 STAR_Callsite * collective_site,
			 STAR_Algorithm * repo, int total_algs)
{
  STAR_MPI_Call call_type;
  MPID_Comm * comm;

  int i, j, np, bytes, id;
  int op_type_support;

  call_type = collective_site -> call_type;
  bytes = collective_site -> bytes;
  comm = collective_site -> comm;
  np = collective_site -> comm -> local_size;
  id = collective_site -> id;
  op_type_support = collective_site -> op_type_support;

  STAR_ALLOC((*ptr), STAR_Tuning_Session, 1);

  /* initialize the tuning session fields */
  memset((*ptr), 0, sizeof(STAR_Tuning_Session));
  STAR_ALLOC((*ptr) -> curr_invoc, unsigned, total_algs);
  STAR_ALLOC((*ptr) -> best, int, total_algs);
  STAR_ALLOC((*ptr) -> algorithms_times, double, (total_algs + 2));
  STAR_ALLOC((*ptr) -> time, double,
             (2 + total_algs * STAR_info.invocs_per_algorithm));

  for (i = 0; i < total_algs; i++)
  {
    (*ptr) -> curr_invoc[i] = 0;
    for (j = 0; j < STAR_info.invocs_per_algorithm; j++)
      (*ptr) -> time[i * STAR_info.invocs_per_algorithm + j] = 0.0;
  }

  (*ptr) -> repository = repo;
  (*ptr) -> call_type = call_type;
  (*ptr) -> best_alg_index = -1;
  (*ptr) -> curr_alg_index = -1;
  (*ptr) -> get_next_alg = TRUE;
  (*ptr) -> skip = 2;
  (*ptr) -> callsite_id = id;
  (*ptr) -> factor = 4;
  (*ptr) -> bytes = bytes;
  (*ptr) -> comm = comm;
  (*ptr) -> handle = comm->handle;
  (*ptr) -> np = np;
  (*ptr) -> op_type_support = op_type_support;
}

STAR_Tuning_Session *
STAR_GetTuningSession(STAR_Callsite * collective_site,
		      STAR_Algorithm * repo, int total_algs)
{
  int rank;
  MPID_Comm * comm_world, * comm;
  STAR_Tuning_Session * reset_ptr, * return_ptr;
  STAR_MPI_Call call_type;

  int bytes, id, op_type_support, np;

  MPID_Comm_get_ptr(MPI_COMM_WORLD, comm_world);
  rank = comm_world -> rank;
  
  call_type = collective_site -> call_type;
  bytes = collective_site -> bytes;
  comm = collective_site -> comm;
  id = collective_site -> id;
  op_type_support =  collective_site -> op_type_support;
  np = collective_site -> comm -> local_size;
  
  reset_ptr = comm -> dcmf.tuning_session;

  /* if first time, means tuning_sessions are empty */
  if (!reset_ptr)
  {
    STAR_CreateTuningSession(&(comm -> dcmf.tuning_session), 
                             collective_site, repo, 
                             total_algs);
    return comm -> dcmf.tuning_session;
  }

  /* if we get here, it means we actually have something in the session list */
  while (1)
  {
    if (comm -> dcmf.tuning_session -> call_type == call_type &&
        comm -> dcmf.tuning_session -> callsite_id == id &&
        comm -> dcmf.tuning_session -> bytes == bytes &&
        comm -> dcmf.tuning_session -> np == np &&
        comm -> dcmf.tuning_session -> op_type_support == op_type_support)
    {
      return_ptr = comm -> dcmf.tuning_session;
      comm -> dcmf.tuning_session = reset_ptr;
      return return_ptr;
    }

    if (comm -> dcmf.tuning_session -> next)
      comm -> dcmf.tuning_session = comm -> dcmf.tuning_session -> next; 
    else
      break;
  }

  /* otherwise, create a new entry */
  STAR_CreateTuningSession(& comm -> dcmf.tuning_session -> next, 
			   collective_site, repo, 
			   total_algs);
  return_ptr = comm -> dcmf.tuning_session -> next;
  comm -> dcmf.tuning_session = reset_ptr;
  return return_ptr;
}

void
STAR_NextAlgorithm(STAR_Tuning_Session * session,
		   STAR_Callsite * collective_site,
		   int total_algs)
{
  STAR_Algorithm * repository;
  MPID_Comm * comm;
  int i = 0, np, bytes, op_type_support;
  unsigned * alg_bytes, * alg_nprocs;
  unsigned * curr_alg_invoc, * session_total_examined;
  unsigned char * panic, * one_at_least, reset, skip;
  char * buff_attributes, * alg_name;

  MPIDO_Embedded_Info_Set * comm_info, * alg_info;
  MPIDO_Embedded_Info_Set tmp_comm;
  STAR_MPI_Call call_type;

  buff_attributes = collective_site->buff_attributes;
  call_type = session -> call_type;
  repository = session -> repository;
  comm = session -> comm;
  comm_info = &(comm -> dcmf.properties);
  np = session -> np;
  panic = &(session -> panic);
  bytes = session -> bytes;
  op_type_support = session -> op_type_support;
  session_total_examined = &(session->total_examined);
  one_at_least = &(session->one_at_least);

  MPIDO_INFO_ZERO(&tmp_comm);
  MPIDO_INFO_OR(comm_info, &tmp_comm);

  /* because torus algorithms can handle irreg comms */
  /* MPIDO_INFO_SET(&tmp_comm, MPIDO_IRREG_COMM);*/

  if (MPIDO_INFO_ISSET(&tmp_comm, MPIDO_USE_NOTREE_OPT_COLLECTIVES))
    MPIDO_INFO_UNSET(&tmp_comm, MPIDO_TREE_COMM);
  
  if (buff_attributes[0]) 
    MPIDO_INFO_SET(&tmp_comm, MPIDO_SBUFF_CONTIG);

  if (buff_attributes[1]) 
    MPIDO_INFO_SET(&tmp_comm, MPIDO_RBUFF_CONTIG);

  if (buff_attributes[2]) 
    MPIDO_INFO_SET(&tmp_comm, MPIDO_RBUFF_CONTIN);

  if (buff_attributes[3])
    MPIDO_INFO_SET(&tmp_comm, MPIDO_BUFF_ALIGNED);

  if (!(bytes % sizeof(int))) 
    MPIDO_INFO_SET(&tmp_comm, MPIDO_BUFF_SIZE_MUL4);

  if (op_type_support == MPIDO_TREE_SUPPORT ||
      op_type_support == MPIDO_TREE_MIN_SUPPORT)
    MPIDO_INFO_SET(&tmp_comm, MPIDO_TREE_OP_TYPE);

  if (op_type_support == MPIDO_TORUS_SUPPORT)
    MPIDO_INFO_SET(&tmp_comm, MPIDO_TORUS_OP_TYPE);

  if (op_type_support == MPIDO_NOT_SUPPORTED)
    MPIDO_INFO_SET(&tmp_comm, MPIDO_ANY_OP_TYPE);

  /*
    search for another algorithm in the repository if not done examinning
    all algorithms for the particular collective operation set in call_type
  */

  while (session -> total_examined < total_algs)
  {
    /* update the current algorithm index for the current session */
    session->curr_alg_index++;

    /* use index "i" to navigate in the repository structure */
    i = session->curr_alg_index;

    /* make sure conditions of algorithms meet that of communicator */
    alg_info = &(repository[i].properties);
    alg_bytes = repository[i].bytes;
    alg_nprocs = repository[i].nprocs;
    alg_name = &(repository[i].name[0]);
    curr_alg_invoc = &(session->curr_invoc[i]);

    skip = reset = 0;

    if ((alg_bytes[1] && alg_bytes[1] < bytes) || (alg_bytes[0] > bytes) ||
        (alg_nprocs[1] && alg_nprocs[1] < np) || (alg_nprocs[0] > np))
      skip = 1;

    /*
    if (!skip)
      if (call_type == BCAST_CALL && strstr(alg_name, "asyn"))
        if (comm -> dcmf.bcast_iter > 31)
          skip = 1;
    */
    if (!skip)
    {
      if (MPIDO_INFO_ISSET(alg_info, MPIDO_TORUS_OP_TYPE) &&
          (op_type_support == MPIDO_TREE_SUPPORT ||
           op_type_support == MPIDO_TREE_MIN_SUPPORT))
      {
        reset = 1;
        MPIDO_INFO_SET(alg_info, MPIDO_TREE_OP_TYPE);		
        MPIDO_INFO_UNSET(alg_info, MPIDO_TORUS_OP_TYPE);		
      }
	  
      if (!MPIDO_INFO_MET(alg_info, &tmp_comm))
        skip = 1;

      if (reset)
      {
        MPIDO_INFO_UNSET(alg_info, MPIDO_TREE_OP_TYPE);		
        MPIDO_INFO_SET(alg_info, MPIDO_TORUS_OP_TYPE);		
      }	 
    }

    if (skip)
    {
      /* below statements will cause a skip over noncompliant algorithms */
      (* curr_alg_invoc) = STAR_info.invocs_per_algorithm;
      (* session_total_examined)++;
    }
    else
    {
      * one_at_least = 1;
      break;
    }
  }


  /* if examined all algorithms and had no success, then PANIC */
  if (* session_total_examined == total_algs)
  {
    if (* one_at_least)
      STAR_AssignBestAlgorithm(session);
    else 
      (* panic) = 1;
  }
}

int
STAR_SortAlgorithms(STAR_Tuning_Session * session)
{
  char * MPI;
  int i, j, k, rank, used, total_algs;
  double min, max = 0;
  STAR_MPI_Call call_type;
  
  call_type = session->call_type;
  MPI = STAR_info.mpis[call_type];
  session->total_tuned_algorithms = 0;
  total_algs = session->total_examined;
  rank = session->comm->rank;

  /*
    we first count how many algorithms were tuned. In the case there is only
    one algorithm tuned, then the second statement in the for loop will set
    the index of the best algorithm in best[0] to that only algorithm that
    had a non-zero value in algorithms_times[i]
  */
  for (i = 0; i < total_algs; i++)
  {
    if (session->algorithms_times[i])
    {
      session->total_tuned_algorithms++;
      session->best[0] = i;
    }

    /* find maximum time among all algorithms */
    if (session->algorithms_times[i] > max)
      max = session->algorithms_times[i];
  }

  /* if only had one algorithm tuned, then we are done, it will be best[0] */
  if (session->total_tuned_algorithms == 1)
    return MPI_SUCCESS;

  /* otherwise, we need to sort the list of algorithms based on times */
  min = max;
  for (i = 0; i < total_algs; i++, min = max)
    for (j = 0; j < total_algs; j++)
      if (session->algorithms_times[j] &&
	  session->algorithms_times[j] <= min)
      {
        /*
          this for loop below checks to see if we have previously assigned
          such index to be one of the bext indicies of best algorithms
        */
        for (used = 0, k = 0; k < i; k++)
          if (session->best[k] == j)
          {
            used = 1;
            break;
          }

        /*
          if we have not assigned such index 'j', then the next best
          algorithm is set at index 'j'
        */
        if (!used)
        {
          session -> best[i] = j;
          min = session -> algorithms_times[j];
        }
      }

#ifdef VBL2
  if (MPIDO_STAR_fd)
  {
    fprintf(MPIDO_STAR_fd, "\nCurrent sorting\n");
    for (i = 0; i < total_algs; i++)
      if (session -> best[i] > -1)
        fprintf(MPIDO_STAR_fd, "%-39s best time %lfus\n",
                session->repository[session->best[i]].name,
                1.0E6 * session->algorithms_times[session->best[i]]);
    fflush(MPIDO_STAR_fd);
  }
#endif
  return MPI_SUCCESS;
}


void
STAR_CheckPerformanceOfBestAlg(STAR_Tuning_Session * session)
{
  STAR_MPI_Call call_type;
  char * MPI;
  double best_time;
  int second, alg_index, bytes, rank, np;

  call_type = session->call_type;
  bytes = session->bytes;
  MPI = STAR_info.mpis[call_type];
  np = session->comm->local_size;
  rank = session->comm->rank;

  /*
    max[0] is the average algorithm communication time over the so far
    monitoring calls. 
    max[1] is the average algorithm communication time for the last
    INVOCS_PER_ALGORITHMS number of invocations
  */

  session->max[0] /= session->monitor_calls;
  session->max[1] /= STAR_info.invocs_per_algorithm;

  /* reduce max0 and max1 over all processes using the max operation */
  
  MPIDO_Allreduce(MPI_IN_PLACE, session->max, 4, MPI_DOUBLE, MPI_MAX,
		  session->comm);

  session->monitor_overhead = session->max[2] / session->post_tuning_calls;
  session->post_tuning_time = session->max[3] / session->post_tuning_calls;

  /* second is index of second best algorithm */
  second = session->best[1];

  /*
    this is an optimization to prevent selection from bouncing between
    algorithms that are ver close in performance. So, we define best time as
    the time measured for the second best algorithm plus 10%. That is,
    if the second algorithm in the repository is better than the
    so far best algorithm by less than 10%, then we keep the so far best
    algorithm
  */
  best_time = 1.1 * session->algorithms_times[second];
  
#ifdef VBL2
  if (MPIDO_STAR_fd)
  {

    fprintf(MPIDO_STAR_fd, 
            "\n%s: comm: %s np: %d msize: %d Monitoring for %d invocs\n", 
            MPI, comm_shape_str[session -> comm -> dcmf.comm_shape], np, 
            bytes, session -> monitor_calls);
    fprintf(MPIDO_STAR_fd, "%-39s elp %.2lfus\n%-39s elp %.2lfus\n",
            session -> repository[session->best_alg_index].name,
            1.0E6 * session -> max[0], session -> repository[second].name,
            1.0E6 * session -> algorithms_times[second]);
    fflush(MPIDO_STAR_fd);
  }
#endif
  
  /*
    more optimizations: we only switch to the second best algorithm when
    the performance of the so far best algorithm deteriorates over all
    monitoring invocations AND over the last number of INVOCS_PER_ALGORITHMS.
    If that is the case, then we default to the second algorithm. Else, we
    give the so far best algorithm another chance and remonitor its
    performance. If the algorithm is still performing good, then we remonitor
    it for a longer period of time
  */

  if (session->max[0] >= best_time)
  {
    if (session->max[1] >= best_time)
    {
#ifdef VBL2
      if (MPIDO_STAR_fd)
      {
        fprintf(MPIDO_STAR_fd, "\nswitching to %s\n", 
                session -> repository[second].name);
        fflush(MPIDO_STAR_fd);
      }
#endif
      /*
        get the index of the best algorithm on hand, update its time
        using the average over all monitoring phase, set new best
        algorithm to point to the algorithm at 'second' index, resort the
        list of algorithms based on their times, and reset the monitor
        factor to 2 to initiate a new monitoring phase for the new best
        algorithm.
      */
      alg_index = session->best[0];
      session->algorithms_times[alg_index] = session->max[1];
      session->best_alg_index = second;
      STAR_SortAlgorithms(session);
      session->factor = 4;
    }
    else
    {
      alg_index = session->best[0];
      session->algorithms_times[alg_index] = session->max[1];
#ifdef VBL2
      if (MPIDO_STAR_fd)
        fprintf(MPIDO_STAR_fd,"**** Re-monitoring\n");
#endif
    }
  }
  else
  {
    session->factor *= 2;
#ifdef VBL2
    if (MPIDO_STAR_fd)
      fprintf(MPIDO_STAR_fd, "**** Keeping algorithm\n");
#endif
  }

  /* reset book-keeping variables */
  session->monitor_calls = 0;
  session->max[0] = 0.0;
  session->max[1] = 0.0;

  /* reset in order to destroy effect of allreduce done in this function */
  session->skip = 2;
}

/*
  computes performance of an algorithm.
*/
void
STAR_ComputePerformance(STAR_Tuning_Session * session,
			double * elapsed, int alg_index)
{
  char * MPI;
  int i, j, rank, np, bytes;
  double sum = 0.0, hold;

  np = session->comm->local_size;
  rank = session->comm->rank;
  MPI = STAR_info.mpis[session -> call_type];
  bytes = session -> bytes;

  
  /*
    XXXXX we will be using max, not ave
    for a given algorithm and for each invocation completion time,
    compute the ave time over all num_processes

    for (i = 0; i < STAR_info.invocs_per_algorithm; i++)
    elapsed[alg_index * STAR_info.invocs_per_algorithm + i] /= np;

  */

  /*
    now sort timing results from min to max. This makes the best completion
    times of a given algorithm occupy the timing array from low to high.
  */
  for (i = 0; i < STAR_info.invocs_per_algorithm - 1; i++)
    for (j = 0; j < STAR_info.invocs_per_algorithm - 1; j++)
      if (elapsed[alg_index * STAR_info.invocs_per_algorithm + j] >
	  elapsed[alg_index * STAR_info.invocs_per_algorithm + j + 1])
      {
        hold = elapsed[alg_index * STAR_info.invocs_per_algorithm + j];
        elapsed[alg_index * STAR_info.invocs_per_algorithm + j] =
          elapsed[alg_index * STAR_info.invocs_per_algorithm + j + 1];
        elapsed[alg_index * STAR_info.invocs_per_algorithm + j + 1] = hold;
      }

  /*
    We now compute the performance of an algorithm as the average time over
    the first (STAR_info.invocs_per_algorithm / 2) + 1 invocations. This sort 
    of eliminates any potential outliers in the time measurement conducted in
    the STAR_info.invocs_per_algorithm invocations.
  */
  for (i = 0; i < (STAR_info.invocs_per_algorithm / 2) + 1; i++)
  {
    sum += elapsed[alg_index * STAR_info.invocs_per_algorithm + i];

#ifdef VBL2
    if (MPIDO_STAR_fd && (sum > 0.0))
      fprintf(MPIDO_STAR_fd, "%-39s invoc %2d elapsed %lfus\n",
              session->repository[alg_index].name, i,
              1.0E6 * elapsed[alg_index * STAR_info.invocs_per_algorithm+i]);
#endif
  }

  /* now set the best time for each algorithm as the computed average */
  session->algorithms_times[alg_index] = sum /
    ((STAR_info.invocs_per_algorithm / 2) + 1);
}


int
STAR_DisplayStatistics(MPID_Comm * comm)
{
  STAR_Tuning_Session *ptr;
  STAR_MPI_Call call_type;
  STAR_Algorithm * repository;
  int bytes, np, i, total_algs;
  char * MPI;

  /* we need to get src line and file name info */

  ptr = comm->dcmf.tuning_session;
  np = comm -> local_size;

  //STAR_info.internal_control_flow = 1;
  while (ptr)
  {
    repository = ptr->repository;
    call_type = ptr->call_type;
    MPI = STAR_info.mpis[call_type];
    bytes = ptr->bytes;
    total_algs = ptr->total_examined;

    if (call_type == ALLGATHER_CALL) bytes /= np;

    if (ptr->monitor_overhead)
    {

      fprintf(MPIDO_STAR_fd,
              "\nComm: %s np: %d msize: %d #Tuning calls %d"
              " #Monitor calls %d Best-Algorithm: %s elapsed %.2lfus"
              " callsite: %x\n\n",
              comm_shape_str[ptr->comm->dcmf.comm_shape],
              np,
              bytes,
              ptr->tuning_calls, ptr->post_tuning_calls,
              ptr->repository[ptr->best_alg_index].name,
              1.0E6 * ptr->algorithms_times[ptr->best[0]],
              ptr->callsite_id);
      
      fprintf(MPIDO_STAR_fd, "Statistics\n");
      for (i = 0; i < total_algs; i++)
        if (ptr->best[i] > -1)
          fprintf(MPIDO_STAR_fd, "%-39s elapsed %.2lfus\n",
                  repository[ptr->best[i]].name,
                  1.0E6 * ptr->algorithms_times[ptr->best[i]]);


      fprintf(MPIDO_STAR_fd, "\nTuning Phase: Os: %.2lfus Oc: %.2lfus\n",
              1.0E6 * ptr->tune_overhead,
              1.0E6 * ptr->comm_overhead);
      //1.0E6 * ptr->tune_overhead / ptr->tuning_calls,
      //      1.0E6 * ptr->comm_overhead / ptr->tuning_calls);
     
      fprintf(MPIDO_STAR_fd, "Monitoring Phase:  Os: %.2lfus Oc: %.2lfus\n",
                          1.0E6 * ptr->monitor_overhead,
              1.0E6 * ptr->post_tuning_time);

      //1.0E6 * ptr->monitor_overhead * ptr->post_tuning_calls,
      //      1.0E6 * ptr->post_tuning_time * ptr->post_tuning_calls);
      
      fprintf(MPIDO_STAR_fd,
              "-----------------------------------------------------------\n");
      fflush(MPIDO_STAR_fd);
    }
    ptr = ptr->next;
  }
  
  if (comm -> handle == MPI_COMM_WORLD)
    fclose(MPIDO_STAR_fd);
  
  //STAR_info.internal_control_flow = 0;
  return 0;
}

void
STAR_FreeMem(MPID_Comm * comm)
{
  if (STAR_info.enabled)
  {
    STAR_Tuning_Session * ptr, * to_be_freed;
    ptr = comm -> dcmf.tuning_session;

    /* if we need to output debugging info to a file */
    if (MPIDO_STAR_fd)
      STAR_DisplayStatistics(comm);
      
    /* else free memory */
      
    while (ptr)
    {
      to_be_freed = ptr;
      ptr = ptr -> next;
      MPIU_Free(to_be_freed -> best);
      MPIU_Free(to_be_freed -> time);
      MPIU_Free(to_be_freed -> curr_invoc);
      MPIU_Free(to_be_freed -> algorithms_times);
      MPIU_Free(to_be_freed);
    }
  }

}


inline STAR_Tuning_Session *
STAR_AssembleSession(STAR_Callsite * collective_site,
		     STAR_Algorithm * repo, int total_algs)
{
  STAR_Tuning_Session * session;
  double t0 = DCMF_Timer();

  /* find, get the tuning session that handles the current collective site */
  session = STAR_GetTuningSession(collective_site, repo, total_algs);

  session -> initial_time = t0;

  /* if current session indicates trying a new algorithm */
  if (session -> get_next_alg)
  {
    /*
      reset to avoid getting another algorithm before current one finishes
      running across the STAR_INVOCS_PER_ALGORITHM
    */
    session -> get_next_alg = FALSE;
      
    /* if examined all algorithms, find best algorithm for this session */
    if (session -> total_examined == total_algs)
      STAR_AssignBestAlgorithm(session);
    else
      /* find next algorithm to tune for this session */
      STAR_NextAlgorithm(session, collective_site, total_algs);
  }

  return session;
}

inline int
STAR_ProcessMonitorPhase(STAR_Tuning_Session * session, double elapsed)
{
  double initial_time;
  int curr_total_monitor_invocs;

  initial_time = session->initial_time;

  session->max[3] += elapsed;

  /*
    if this is first time running in monitor phase, then trash a number of
    measurements since they are spoiled from the effect of the last
    invocation in the tuning phase (allreduce, ..etc effects)
    This is like a Primer phase.
  */
  if (session->skip)
  {
    session->skip--;
    return MPI_SUCCESS;
  }


  //session->post_tuning_time += elapsed;
  session->post_tuning_calls++;

  /* if have one algorithm, no need to monitor */
  if (session->total_tuned_algorithms == 1)
  {
    session->max[2] += ((DCMF_Timer() - initial_time) - elapsed);
    //    session->monitor_overhead += ((DCMF_Timer() - initial_time) - elapsed);
    return MPI_SUCCESS;
  }

  /* this computes the average so far over monitoring calls */
  session->max[0] += elapsed;

  curr_total_monitor_invocs = session->factor * STAR_info.invocs_per_algorithm;

  /*
    if we are in the last STAR_INVOCS_PER_ALGORITHM invocations of overall
    monitoring phase, then we need also to capture the performance of the
    last STAR_INVOCS_PER_ALGORITHM
  */
  if ((curr_total_monitor_invocs - session->monitor_calls) <=
      STAR_info.invocs_per_algorithm)
    session->max[1] += elapsed;

  session->monitor_calls++;

  /* if done monitoring current best algorithm for the period specified */
  if (session->monitor_calls == curr_total_monitor_invocs)
    STAR_CheckPerformanceOfBestAlg(session);

  /* this compute only STAR monitoring software/logic overhead */
  session->max[2] += ((DCMF_Timer() - initial_time) - elapsed);
  //session->monitor_overhead += ((DCMF_Timer() - initial_time) - elapsed);
  return MPI_SUCCESS;
}

inline void
STAR_ProcessTuningPhase(STAR_Tuning_Session * session, double elapsed)
{
  double initial_time;
  int alg_index, invoc;

  initial_time = session->initial_time;

  /* this computes total comm_overhead: time for trying all algorithms */
  session->comm_overhead += elapsed;

  alg_index = session->curr_alg_index;
  invoc = alg_index * STAR_info.invocs_per_algorithm + 
    session->curr_invoc[alg_index];
  session->time[invoc] = elapsed;

  /* update the invocation counter for the current algorithm */
  session->curr_invoc[alg_index]++;

  /*
    if we have tried the current algorithm in the tuning phase for
    INVOCS_PER_ALGORITHMS number of invocations, then we need to try another
    one next. We reset the get_next_algo to true and increment the number of
    algorithms so far examined.
  */
  if (session->curr_invoc[alg_index] == STAR_info.invocs_per_algorithm)
  {
    session->get_next_alg = TRUE;
    session->total_examined++;
  }

  session->tuning_calls++;

  /* this computes only STAR tuning software/logic overhead */
  session->tune_overhead += ((DCMF_Timer() - initial_time) - elapsed);
}
