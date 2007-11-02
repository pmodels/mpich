/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>

#include "mpidimpl.h"

#define ITERS 10

int main(int argc, char * argv[])
{
#if defined(MPIDI_CH3_MSGS_UNORDERED)
    {
	int rc;
	int size;
	int rank;
	int buf[ITERS];
	int i;
	
	rc = MPI_Init(&argc, &argv);
	assert(rc == MPI_SUCCESS);
    
	rc = MPI_Comm_size(MPI_COMM_WORLD, &size);
	assert(rc == MPI_SUCCESS);
	rc = MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	assert(rc == MPI_SUCCESS);

	if (size < 2)
	{
	    fprintf(stderr, "ERROR: at least 2 processes are required\n");
	    fflush(stderr);
	    return 1;
	}
    
	if (rank == 0)
	{
	    MPI_Request request;
	    MPID_Request * req;
	    MPI_Status status;
	    MPID_Comm * comm;
	    MPIDI_VC_t * vc;

	    MPID_Comm_get_ptr(MPI_COMM_WORLD, comm);
	    vc = comm->vcr[1];
	
	    for (i = 0; i < ITERS; i++)
	    {
		MPID_Seqnum_t seqnum = (i * 7) % ITERS;
		MPIDI_CH3_Pkt_eager_send_t pkt;

		pkt.type = MPIDI_CH3_PKT_EAGER_SEND;
		pkt.match.rank = 0;
		pkt.match.tag = i;
		pkt.match.context_id = comm->context_id + MPID_CONTEXT_INTRA_PT2PT;
		pkt.sender_req_id = MPI_REQUEST_NULL;
#               if defined(SEND_DATA)
		{
		    pkt.data_sz = seqnum * sizeof(int);
		}
#		else
		{ 
		    pkt.data_sz = 0;
		}
#		endif
		/* MPIDI_CH3U_VC_FAI_send_seqnum(vc, seqnum); */
		MPIDI_CH3U_Pkt_set_seqnum(&pkt, seqnum);

		printf("Sending msg %lu, tag=%d\n", seqnum, pkt.match.tag);
		fflush(stdout);
		
		if (pkt.data_sz > 0)
		{
		    int j;
		    MPID_IOV iov[2];
		    
		    for(j = 0; j < seqnum; j++)
		    {
			buf[j] = seqnum;
		    }
		    
		    iov[0].MPID_IOV_BUF = &pkt;
		    iov[0].MPID_IOV_LEN = sizeof(pkt);
		    iov[1].MPID_IOV_BUF = buf;
		    iov[1].MPID_IOV_LEN = pkt.data_sz;
		    req = MPIDI_CH3_iStartMsgv(vc, iov, 2);
		}
		else
		{
		    req = MPIDI_CH3_iStartMsg(vc, &pkt, sizeof(pkt));
		}

		if (req != NULL)
		{
		    MPIDI_CH3U_Request_set_seqnum(req, seqnum);
		    MPIDI_Request_set_type(req, MPIDI_REQUEST_TYPE_SEND);
		    req->comm = comm;
		    request = req->handle;
		}
		else
		{
		    request = MPI_REQUEST_NULL;
		}
		
		rc = MPI_Wait(&request, &status);
		assert(rc == MPI_SUCCESS);
	    }

	    vc->seqnum_send += ITERS;
	}
	else if (rank == 1)
	{
	    int errs = 0;
	    
	    for (i = 0; i < ITERS; i++)
	    {
		MPI_Status status;
		int j;
	    
		rc = MPI_Recv(buf, ITERS, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
		assert(rc == MPI_SUCCESS);
		
		rc = MPI_Get_count(&status, MPI_INT, &j);
		assert(rc == MPI_SUCCESS);

		printf("Recevied msg rank=%d tag=%d count=%d error=%d\n", status.MPI_SOURCE, status.MPI_TAG, j, status.MPI_ERROR);
		fflush(stdout);

#               if defined(SEND_DATA)
		{
		    if (j != i)
		    {
			errs++;
			printf("ERROR: expected count=%d, got count=%d\n", i, j);
			fflush(stdout);
			continue;
		    }

		    for (j = 0; j < i; j++)
		    {
			if (buf[j] != i)
			{
			    errs++;
			    printf("ERROR: expected buf[%d]=%d, got buf[%d]=%d\n", j, i, j, buf[j]);
			    fflush(stdout);
			}
		    }
		}
#		endif
	    }

	    if (errs == 0)
	    {
		printf("No Errors\n");
	    }
	    else
	    {
		printf("%d Errors\n", errs);
	    }
	    fflush(stdout);
	}
    
	rc = MPI_Finalize();
	assert(rc == MPI_SUCCESS);
    }
#   else
    {
	fprintf(stderr, "Test disabled.\n");
	fflush(stderr);
    }
#   endif
    return 0;
}

