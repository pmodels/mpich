/*  (C)Copyright IBM Corp.  2007, 2008  */
/**
 * \file src/coll/gather/STAR_gather.c
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


#include "mpido_coll.h"
#include "mpidi_star.h"
#include "mpidi_coll_prototypes.h"

  int
    STAR_BestGather(void * sendbuf,
                    int sendcount,
                    MPI_Datatype sendtype,
                    void * recvbuf,
                    int recvcount,
                    MPI_Datatype recvtype,
                    int root,
                    MPID_Comm * comm,
                    int index)

{
  /* load the right algorithm in the function pointer and execute */
  gather_fptr func = STAR_gather_repository[index].func.gather_func;
  return (func)(sendbuf, sendcount, sendtype,
		recvbuf, recvcount, recvtype,
		root, comm);
}


int
STAR_Gather(void * sendbuf,
            int sendcount,
            MPI_Datatype sendtype,
            void * recvbuf,
            int recvcount,
            MPI_Datatype recvtype,
            int root,
            STAR_Callsite * collective_site,
            STAR_Algorithm * repo,
            int total_algs)
{
  STAR_Tuning_Session * session;
  gather_fptr func;
  MPID_Comm * comm;
  double start, elapsed;
  int bytes, best_alg, rc;

  comm = collective_site->comm;
  bytes = collective_site->bytes;

  session = STAR_AssembleSession(collective_site, repo, total_algs);

  if (session->panic) return STAR_FAILURE;

  /* if index is > -1, it means we are done tuning and have a valid index */
  if ((best_alg = session->best_alg_index) > -1)
  {
    /*
      we are now in monitoring phase:
      execute best algorithm and measure its performance.
    */

    start = DCMF_Timer();
    rc = STAR_BestGather(sendbuf, sendcount, sendtype,
                         recvbuf, recvcount, recvtype,
                         root, comm, best_alg);
    elapsed = DCMF_Timer() - start;

    STAR_ProcessMonitorPhase(session, elapsed);
  }

  /*
    otherwise, we are in tuning phase, find the current algorithm index to
    tune and load it in func
  */

  else
  {
    /* execute candidate algorithm and measure its performance */
    func = repo[session->curr_alg_index].func.gather_func;

    start = DCMF_Timer();
    rc = (func)(sendbuf, sendcount, sendtype,
                recvbuf, recvcount, recvtype,
                root, comm);
    elapsed = DCMF_Timer() - start;

    STAR_ProcessTuningPhase(session, elapsed);
  }

  return rc;
}
