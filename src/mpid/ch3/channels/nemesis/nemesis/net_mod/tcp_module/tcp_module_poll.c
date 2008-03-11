/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "tcp_module_impl.h"

int MPID_nem_alt_tcp_module_poll (MPID_nem_poll_dir_t in_or_out);

/* #define TRACE */
  
#undef FUNCNAME
#define FUNCNAME MPID_nem_tcp_module_poll_send
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_tcp_module_poll_send( void )
{
    int                  mpi_errno = MPI_SUCCESS;
    MPID_nem_cell_ptr_t  cell;
    MPID_nem_pkt_t      *pkt;
    int                  offset;
    int                  dest;
    int                  len;
    int                  index;
    int                  grank;
    node_t              *nodes     = MPID_nem_tcp_internal_vars.nodes;
   
    /* first, handle pending sends */
    if  (MPID_nem_tcp_internal_vars.n_pending_send > 0)
    {
        for(index = 0 ; index < MPID_nem_mem_region.ext_procs ;index++)
	{
            grank = MPID_nem_mem_region.ext_ranks[index];
            if((grank != MPID_nem_mem_region.rank ) && (!MPID_nem_tcp_internal_queue_empty (nodes[grank].internal_recv_queue)))
	    {	     
#ifdef TRACE 
                fprintf(stderr,"[%i] -- TCP RETRY SEND for %i ... \n",MPID_nem_mem_region.rank,grank);
                /*MPID_nem_dump_queue( nodes[grank].internal_recv_queue ); */
#endif		  	  
                pkt  = (MPID_nem_pkt_t *)MPID_NEM_CELL_TO_PACKET ( nodes[grank].internal_recv_queue.head ); /* cast away volatile */
                dest = pkt->mpich2.dest;
                len  = (MPID_NEM_PACKET_OPT_LEN(pkt)) - nodes[dest].left2write;
	      
                do
                {
                    offset = write(nodes[dest].desc,(char *)pkt + nodes[dest].left2write, len);
                }
                while (offset == -1 && errno == EINTR);
#ifdef TRACE 
                fprintf(stderr,"[%i] -- TCP RETRY SEND for %i/offset %i/remaining %i \n/pkt len : %i/curr offset : %i \n",
                        MPID_nem_mem_region.rank,
                        grank,
                        offset,
                        len,
                        MPID_NEM_PACKET_OPT_LEN(pkt),
                        nodes[dest].left2write);
#endif		  	  
                if(offset != -1)
		{
                    nodes[dest].left2write += offset;
		  
                    if(nodes[dest].left2write == (MPID_NEM_PACKET_OPT_LEN(pkt)))
		    {		 
#ifdef TRACE 
                        fprintf(stderr,"[%i] -- TCP SEND : sent PARTIAL MSG 2 %i len, [%i total/%i payload]\n",
                                MPID_nem_mem_region.rank,
                                offset,
                                nodes[dest].left2write,
                                pkt->mpich2.datalen);
#endif		  
		      
                        nodes[dest].left2write = 0;
                        MPID_nem_tcp_internal_queue_dequeue (&nodes[dest].internal_recv_queue, &cell);
                        MPID_nem_queue_enqueue (MPID_nem_process_free_queue, cell); 
                        MPID_nem_tcp_internal_vars.n_pending_send--;
                        MPID_nem_tcp_internal_vars.n_pending_sends[dest]--;
		    }
		}
                else 
		{

                    /* write() returned an error */
                    MPIU_ERR_CHKANDJUMP1 (errno != EAGAIN, mpi_errno, MPI_ERR_OTHER, "**write", "**write %s", strerror (errno));
		}
	    }
	}
    }

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_tcp_module_poll_recv
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_tcp_module_poll_recv( void )
{
    int                  mpi_errno    = MPI_SUCCESS;
    MPID_nem_cell_ptr_t  cell         = NULL;
    fd_set               read_set     = MPID_nem_tcp_internal_vars.set;
    int                  ret          = 0;
    int                  index;
    int                  grank;
    int                  outstanding2 = 0 ;   
    int                  offset;
    MPID_nem_pkt_t      *pkt          = NULL;
    struct timeval       time;
    node_t              *nodes        = MPID_nem_tcp_internal_vars.nodes ;
   
    time.tv_sec  = 0;
    time.tv_usec = 0;
    ret = select (MPID_nem_tcp_internal_vars.max_fd, &read_set, NULL, NULL, &time);
    MPIU_ERR_CHKANDJUMP1 (ret == -1, mpi_errno, MPI_ERR_OTHER, "**select", "**select %s", strerror (errno));

#ifdef TRACE    
    if(ret)
	fprintf(stderr,
		"[%i] -- RECV TCP select with ret : %i \n",
		MPID_nem_mem_region.rank,
		ret);
#endif
    
    while( (ret > 0) || (MPID_nem_tcp_internal_vars.outstanding > 0))
    {
	for(index = 0 ; index < MPID_nem_mem_region.ext_procs ; index++)
	{
	    grank = MPID_nem_mem_region.ext_ranks[index];
	    if(FD_ISSET(nodes[grank].desc,&read_set))
	    {
		FD_CLR(nodes[grank].desc,&read_set);
		ret--;
		nodes[grank].toread       = 0;
		MPID_nem_tcp_internal_vars.outstanding = 0;

#ifdef TRACE
		fprintf(stderr,"[%i] -- RECV TCP READ : desc is %i (index %i)\n",
			MPID_nem_mem_region.rank, 
			nodes[grank].desc,
			index);
		/*MPID_nem_dump_queue( nodes[grank].internal_free_queue );*/
#endif	    

            main_routine :
                /* handle pending recvs */
                if(!MPID_nem_tcp_internal_queue_empty (nodes[grank].internal_free_queue))
                {
                    pkt = (MPID_nem_pkt_t *)MPID_NEM_CELL_TO_PACKET (nodes[grank].internal_free_queue.head); /* cast away volatile */
                    if ((nodes[grank].left2read_head > 0) && (nodes[grank].left2read == 0))
                    {
                        do 
                        {
                            offset = read(nodes[grank].desc,
                                          (char *)pkt + nodes[grank].left2read_head,
                                          MPID_NEM_OPT_HEAD_LEN - nodes[grank].left2read_head);
                        }
                        while (offset == -1 && errno == EINTR);
                        if(offset != -1)
                        {
                            nodes[grank].left2read_head += offset;
                            if(nodes[grank].left2read_head == MPID_NEM_OPT_HEAD_LEN)
                            {	      
                                nodes[grank].left2read_head = 0;
                                if (pkt->mpich2.datalen > MPID_NEM_OPT_SIZE)
                                {
                                    nodes[grank].left2read = pkt->mpich2.datalen - MPID_NEM_OPT_SIZE; 
                                    do 
                                    {
                                        offset =  read(nodes[grank].desc,
                                                       (pkt->mpich2.payload + MPID_NEM_OPT_SIZE),
                                                       nodes[grank].left2read);				  
                                    }
                                    while (offset == -1 && errno == EINTR);

                                    if (offset != -1)
                                    {
                                        nodes[grank].left2read -= offset;
                                        if (nodes[grank].left2read == 0)
                                        {
                                            MPID_nem_tcp_internal_queue_dequeue (&nodes[grank].internal_free_queue, &cell);
                                            MPID_nem_queue_enqueue (MPID_nem_process_recv_queue, cell);	      
                                            MPID_nem_tcp_internal_vars.n_pending_recv--;
                                        }
                                    }
                                    else if (errno != EAGAIN)
                                    {
                                        /* read() returned an error */
/*                                         printf ("read (fd=%d buf=%p len=%d) grank=%d\n", nodes[grank].desc, /\*DARIUS*\/ */
/*                                                 (pkt->mpich2.payload + MPID_NEM_OPT_SIZE), /\*DARIUS*\/ */
/*                                                 nodes[grank].left2read, grank); /\*DARIUS*\/ */
                                        MPIU_ERR_SETANDJUMP1 (mpi_errno, MPI_ERR_OTHER, "**read", "**read %s", strerror (errno));
                                    }
                                    continue;
                                }
                                else
                                {
                                    nodes[grank].left2read = 0;
                                    MPID_nem_tcp_internal_queue_dequeue (&nodes[grank].internal_free_queue, &cell);
                                    MPID_nem_queue_enqueue (MPID_nem_process_recv_queue, cell);	      
                                    MPID_nem_tcp_internal_vars.n_pending_recv--;
                                    continue;
                                }
                                
                            }
#ifdef TRACE
                            else
                            {
                                fprintf(stderr,"[%i] -- RECV TCP READ : NOT FULL HEAD YET !!!\n",MPID_nem_mem_region.rank);
                            }
#endif
                        }
                        else
                        { 
                            if(errno != EAGAIN)
                            {
                                /* read() returned an error */
                                MPIU_ERR_SETANDJUMP1 (mpi_errno, MPI_ERR_OTHER, "**read", "**read %s", strerror (errno));
                            }                            
                        }		      
                    }
                    else if (nodes[grank].left2read > 0)
                    {
                        do 
                        {
                            offset = read(nodes[grank].desc,
                                          (char *)&(pkt->mpich2.payload) + (pkt->mpich2.datalen - nodes[grank].left2read),    
                                          nodes[grank].left2read);
                        }
                        while (offset == -1 && errno == EINTR);

                        if (offset != -1)
                        {
                            nodes[grank].left2read -= offset;			  
#ifdef TRACE 
                            {
                                int index;
                                fprintf(stderr,
                                        "[%i] -- RECV TCP READ : RETRY 3 for %i, got %i bytes [%i current/ %i total] \n",
                                        MPID_nem_mem_region.rank,
                                        grank,
                                        offset,
                                        (pkt->mpich2.datalen - nodes[grank].left2read),
                                        pkt->mpich2.datalen);

                            }
#endif	

                            if (nodes[grank].left2read == 0)
                            {
                                nodes[grank].left2read_head = 0;
                                MPID_nem_tcp_internal_queue_dequeue (&nodes[grank].internal_free_queue, &cell);
                                MPID_nem_queue_enqueue (MPID_nem_process_recv_queue, cell);	      
                                MPID_nem_tcp_internal_vars.n_pending_recv--;					  
                            }
                        }
                        else
                        { 
                            if (errno != EAGAIN)
                            {
                                /* read() returned an error */
                                MPIU_ERR_SETANDJUMP1 (mpi_errno, MPI_ERR_OTHER, "**read", "**read %s", strerror (errno));
                            }
                        }
                    }
                }
                else
                {
                    /* handle regular Net Q */
                    if (!MPID_nem_queue_empty(MPID_nem_module_tcp_free_queue))
                    {
                        MPID_nem_queue_dequeue (MPID_nem_module_tcp_free_queue, &cell);
                        do 
                        {
                            offset = read (nodes[grank].desc,
                                           (MPID_nem_pkt_mpich2_t *)&(cell->pkt.mpich2), /* cast away volatile */
                                           MPID_NEM_OPT_HEAD_LEN);
                        }
                        while (offset == -1 && errno == EINTR);
#ifdef TRACE 			     
                        {				
                            int index ;
                            for(index = 0 ; index < ((cell->pkt.mpich2.datalen)/sizeof(int)); index ++)
                            {
                                fprintf(stderr,"[%i] --- Got cell[%i] : %i\n",MPID_nem_mem_region.rank,index,((int *)&(cell->pkt.mpich2))[index] );
                            }
                        }
#endif 
			   
                        if (offset != -1)
                        {
                            nodes[grank].left2read_head += offset;		    
                            if (nodes[grank].left2read_head != 0)
                            {
                                if (nodes[grank].left2read_head < MPID_NEM_OPT_HEAD_LEN)
                                {
#ifdef TRACE 
                                    fprintf(stderr,"[%i] -- RECV TCP READ : got PARTIAL header [%i bytes/ %i total] \n",
                                            MPID_nem_mem_region.rank,
                                            offset, 
                                            MPID_NEM_OPT_HEAD_LEN);
#endif
										
                                    if (strncmp( (char *)((MPID_nem_pkt_mpich2_t *)&(cell->pkt.mpich2)),TCP_END_STRING,strlen(TCP_END_STRING)) == 0)
                                    {
#ifdef TRACE 
                                        fprintf(stderr,"[%i] -- RECV TCP READ : got TERM MSG (PARTIAL) : %s \n",
                                                MPID_nem_mem_region.rank,(char *)((MPID_nem_pkt_mpich2_t *)&(cell->pkt.mpich2)));
#endif 
                                        --MPID_nem_tcp_internal_vars.nb_procs;
                                        MPID_nem_queue_enqueue (MPID_nem_module_tcp_free_queue, cell);
                                        goto end;
                                    }
                                    else					
                                    {
                                        MPID_nem_tcp_internal_queue_enqueue (&nodes[grank].internal_free_queue, cell);
                                        MPID_nem_tcp_internal_vars.n_pending_recv++;
                                    }
                                }
                                else
                                {		    				
#ifdef TRACE	
                                    {
                                        int index;
                                        fprintf(stderr,"[%i] -- RECV TCP READ : got FULL header [%i bytes/ %i total] [src %i -- dest %i -- dlen %i -- seqno %i]\n",
                                                MPID_nem_mem_region.rank,
                                                offset, 
                                                MPID_NEM_OPT_HEAD_LEN,
                                                cell->pkt.mpich2.source,
                                                cell->pkt.mpich2.dest,
                                                cell->pkt.mpich2.datalen,
                                                cell->pkt.mpich2.seqno);
                                    }
#endif
					
                                    if (strncmp( (char *)((MPID_nem_pkt_mpich2_t *)&(cell->pkt.mpich2)),TCP_END_STRING,strlen(TCP_END_STRING)) == 0)
                                    {
#ifdef TRACE 
                                        fprintf(stderr,"[%i] -- RECV TCP READ : got TERM MSG (FULL) : %s, cell @ %p \n",
                                                MPID_nem_mem_region.rank,(char *)((MPID_nem_pkt_mpich2_t *)&(cell->pkt.mpich2)),cell);
#endif 
                                        --MPID_nem_tcp_internal_vars.nb_procs;
                                        MPID_nem_queue_enqueue (MPID_nem_module_tcp_free_queue, cell);
                                        goto end;
                                    }
					
                                    if ( (cell->pkt.mpich2.datalen) > (MPID_NEM_OPT_SIZE) )
                                    {
                                        nodes[grank].left2read = ((cell->pkt.mpich2.datalen) - (MPID_NEM_OPT_SIZE));
                                    }				       
                                    else
                                    {					    
                                        nodes[grank].left2read = 0;
                                    }
					
                                    if (nodes[grank].left2read != 0)
                                    {
                                        do 
                                        {
                                            offset = read(nodes[grank].desc,
                                                          ((char *)&(cell->pkt.mpich2) + (MPID_NEM_OPT_HEAD_LEN)),
                                                          nodes[grank].left2read );
                                        }
                                        while (offset == -1 && errno == EINTR);

                                        if (offset != -1)
                                        {
#ifdef TRACE 
                                            {
                                                int index;
                                                fprintf(stderr,"[%i] -- RECV TCP READ : got  [%i bytes/ %i total] \n",
                                                        MPID_nem_mem_region.rank,
                                                        offset,
                                                        nodes[grank].left2read);
                                            }
#endif				    			    
                                            nodes[grank].left2read_head = 0;
                                            nodes[grank].left2read     -= offset;
                                            if (nodes[grank].left2read == 0)
                                            {
                                                MPID_nem_queue_enqueue (MPID_nem_process_recv_queue, cell);	      
                                            }
                                            else				     
                                            {
                                                MPID_nem_tcp_internal_queue_enqueue (&nodes[grank].internal_free_queue, cell);
                                                MPID_nem_tcp_internal_vars.n_pending_recv++;
                                            }
                                        }
                                        else 
                                        {
                                            if (errno == EAGAIN)
                                            {
                                                MPID_nem_tcp_internal_queue_enqueue (&nodes[grank].internal_free_queue, cell);
                                                MPID_nem_tcp_internal_vars.n_pending_recv++;
                                            }
                                            else
                                            {
                                                /* read() returned an error */
                                                MPIU_ERR_SETANDJUMP1 (mpi_errno, MPI_ERR_OTHER, "**read", "**read %s", strerror (errno));
                                            }
                                        }
                                    }
                                    else
                                    {
                                        MPID_nem_queue_enqueue (MPID_nem_process_recv_queue, cell);
                                    end:
                                        nodes[grank].left2read      = 0;
                                        nodes[grank].left2read_head = 0;
                                    }
                                }
                            }
                            else
                            {
                                /* eof i guess */
                                MPID_nem_queue_enqueue (MPID_nem_module_tcp_free_queue, cell);
#ifdef TRACE			
                                perror("EOF SOCK");
#endif		
                            }
                        }
                        else 
                        {
                            if (errno == EAGAIN)
                            {
                                /* why is eagain handled differently? */
                                MPIU_ERR_SETANDJUMP1 (mpi_errno, MPI_ERR_OTHER, "**intern", "**intern %s", strerror (errno));
                            }
                            else
                            {
                                /* read() returned an error */
                                MPIU_ERR_SETANDJUMP1 (mpi_errno, MPI_ERR_OTHER, "**read", "**read %s", strerror (errno));
                            }
                        }
                    }
                    else {
                        /* Q is empty !!! */
                        nodes[grank].toread++;
                        outstanding2++;
                    }
                }
	    }
	    else
            {
#ifdef TRACE       
		fprintf(stderr,"[%i] -- RECV TCP READ NO desc (%i) (index %i) \n",
			MPID_nem_mem_region.rank, nodes[index].desc,grank);
#endif	   
		if (nodes[grank].toread > 0 )
                {
		    nodes[grank].toread--;
		    MPID_nem_tcp_internal_vars.outstanding--;
		    goto  main_routine;
                }
            }
	}
    }
    MPID_nem_tcp_internal_vars.outstanding  = outstanding2;
    outstanding2 = 0;

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_alt_tcp_module_poll
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_alt_tcp_module_poll (MPID_nem_poll_dir_t in_or_out)
{
    int mpi_errno = MPI_SUCCESS;
    
    if(MPID_nem_tcp_internal_vars.poll_freq >= 0)
    {
        if (in_or_out == MPID_NEM_POLL_OUT)
	{
	    if( MPID_nem_tcp_internal_vars.n_pending_send > 0 )
	    {
	        mpi_errno = MPID_nem_tcp_module_poll_send();
                if (mpi_errno) MPIU_ERR_POP (mpi_errno);
		if (MPID_nem_tcp_internal_vars.n_pending_recv > 0)
	        {
		    mpi_errno = MPID_nem_tcp_module_poll_recv();
                    if (mpi_errno) MPIU_ERR_POP (mpi_errno);
                } 
		else if (--(MPID_nem_tcp_internal_vars.poll_freq) == 0) 
		{
		    mpi_errno = MPID_nem_tcp_module_poll_recv();
                    if (mpi_errno) MPIU_ERR_POP (mpi_errno);
                    MPID_nem_tcp_internal_vars.poll_freq = MPID_nem_tcp_internal_vars.old_poll_freq;
		}
	    }
	    else if (--(MPID_nem_tcp_internal_vars.poll_freq) == 0)
	    {
	        mpi_errno = MPID_nem_tcp_module_poll_send();
                if (mpi_errno) MPIU_ERR_POP (mpi_errno);
                mpi_errno = MPID_nem_tcp_module_poll_recv();
                if (mpi_errno) MPIU_ERR_POP (mpi_errno);
                MPID_nem_tcp_internal_vars.poll_freq = MPID_nem_tcp_internal_vars.old_poll_freq;
	    }
	}
	else 
	{
	    if( MPID_nem_tcp_internal_vars.n_pending_recv > 0 )
	    {
                mpi_errno = MPID_nem_tcp_module_poll_recv();
                if (mpi_errno) MPIU_ERR_POP (mpi_errno);
                if (MPID_nem_tcp_internal_vars.n_pending_send > 0)
                {
                    mpi_errno = MPID_nem_tcp_module_poll_send();
                    if (mpi_errno) MPIU_ERR_POP (mpi_errno);
                }
                else if (--(MPID_nem_tcp_internal_vars.poll_freq) == 0) 
                {
                    mpi_errno = MPID_nem_tcp_module_poll_send();
                    if (mpi_errno) MPIU_ERR_POP (mpi_errno);
                    MPID_nem_tcp_internal_vars.poll_freq = MPID_nem_tcp_internal_vars.old_poll_freq;
                }
	    }
	    else if (--(MPID_nem_tcp_internal_vars.poll_freq) == 0)
	    {
	        mpi_errno = MPID_nem_tcp_module_poll_recv();
                if (mpi_errno) MPIU_ERR_POP (mpi_errno);
                mpi_errno = MPID_nem_tcp_module_poll_send();
                if (mpi_errno) MPIU_ERR_POP (mpi_errno);
                MPID_nem_tcp_internal_vars.poll_freq = MPID_nem_tcp_internal_vars.old_poll_freq;
	    }
	}
    }
 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_tcp_module_poll
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_tcp_module_poll (MPID_nem_poll_dir_t in_or_out)
{
    int mpi_errno = MPI_SUCCESS;
    
    if (MPID_nem_tcp_internal_vars.poll_freq >= 0)
    {
        if (in_or_out == MPID_NEM_POLL_OUT)
        {
	    mpi_errno = MPID_nem_tcp_module_poll_send();
            if (mpi_errno) MPIU_ERR_POP (mpi_errno);
	    mpi_errno = MPID_nem_tcp_module_poll_recv();
            if (mpi_errno) MPIU_ERR_POP (mpi_errno);
        }
	else 
	{
            mpi_errno = MPID_nem_tcp_module_poll_recv();
            if (mpi_errno) MPIU_ERR_POP (mpi_errno);
            mpi_errno = MPID_nem_tcp_module_poll_send();
            if (mpi_errno) MPIU_ERR_POP (mpi_errno);
	}
    }
    
 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


