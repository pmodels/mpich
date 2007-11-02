/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

static int create_derived_datatype(MPID_Request *req, MPID_Datatype **dtp);
static int do_accumulate_op(MPID_Request *rreq);

/*
 * MPIDI_CH3U_Handle_recv_req()
 *
 * NOTE: This routine must be reentrant.  Routines like MPIDI_CH3_iRead() are allowed to perform additional up-calls if they
 * complete the requested work immediately.
 *
 * *** Care must be take to avoid deep recursion.  With some thread packages, exceeding the stack space allocated to a thread can
 * *** result in overwriting the stack of another thread.
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3U_Handle_recv_req
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3U_Handle_recv_req(MPIDI_VC * vc, MPID_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3U_HANDLE_RECV_REQ);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3U_HANDLE_RECV_REQ);

    assert(rreq->dev.ca < MPIDI_CH3_CA_END_CH3);
    
    switch(rreq->dev.ca)
    {
	case MPIDI_CH3_CA_COMPLETE:
	{
	    /* mark data transfer as complete and decrement CC */
            rreq->dev.iov_count = 0;
            
            if (MPIDI_Request_get_type(rreq) ==
                                 MPIDI_REQUEST_TYPE_PUT_RESP) { 
                /* atomically decrement RMA completion counter */
                /* FIXME: MT: this has to be done atomically */
                if (rreq->dev.decr_ctr != NULL)
                    *(rreq->dev.decr_ctr) -= 1;
            }

            if (MPIDI_Request_get_type(rreq) ==
                       MPIDI_REQUEST_TYPE_ACCUM_RESP) { 

                /* accumulate data from tmp_buf into user_buf */
                mpi_errno = do_accumulate_op(rreq);
                if (mpi_errno) goto fn_exit;

                /* atomically decrement RMA completion counter */
                /* FIXME: MT: this has to be done atomically */
                if (rreq->dev.decr_ctr != NULL)
                    *(rreq->dev.decr_ctr) -= 1;
            }

            if (MPIDI_Request_get_type(rreq) ==
                MPIDI_REQUEST_TYPE_PUT_RESP_DERIVED_DT) {
                MPID_Request *newreq;
                MPID_Datatype *new_dtp;
                
                /* create derived datatype */
                create_derived_datatype(rreq, &new_dtp);

                /* create new request to get the data */
                newreq = MPID_Request_create();
                MPIU_Object_set_ref(newreq, 1);
                MPIDI_Request_set_type(newreq, MPIDI_REQUEST_TYPE_PUT_RESP);

                newreq->dev.user_buf = rreq->dev.user_buf;
                newreq->dev.user_count = rreq->dev.user_count;
                newreq->dev.datatype = new_dtp->handle;
                newreq->dev.decr_ctr = rreq->dev.decr_ctr;
                newreq->dev.recv_data_sz = new_dtp->size *
                                           rreq->dev.user_count; 
                
                newreq->dev.datatype_ptr = new_dtp;
                /* this will cause the datatype to be freed when the
                   request is freed. */  

                MPID_Segment_init(newreq->dev.user_buf,
                                  newreq->dev.user_count,
                                  newreq->dev.datatype,
                                  &newreq->dev.segment, 0);
                newreq->dev.segment_first = 0;
                newreq->dev.segment_size = newreq->dev.recv_data_sz;

                mpi_errno = MPIDI_CH3U_Request_load_recv_iov(newreq);
                if (mpi_errno != MPI_SUCCESS)
                {
                    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|loadrecviov", 0);
                    goto fn_exit;
                }

                mpi_errno = MPIDI_CH3_iRead(vc, newreq);
                if (mpi_errno != MPI_SUCCESS)
                {
                    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|recvdata", 0);
                    goto fn_exit;
                }

                /* free dtype_info here. the dataloop gets freed
                   when the datatype gets freed (ie when the new request
                   gets freed. */
                MPIU_Free(rreq->dev.dtype_info);
            }

            if (MPIDI_Request_get_type(rreq) ==
                             MPIDI_REQUEST_TYPE_ACCUM_RESP_DERIVED_DT) { 
                MPID_Request *newreq;
                MPID_Datatype *new_dtp;
                MPI_Aint true_lb, true_extent, extent;
                void *tmp_buf;
               
                /* create derived datatype */
                create_derived_datatype(rreq, &new_dtp);

                /* create new request to get the data */
                newreq = MPID_Request_create();
                MPIU_Object_set_ref(newreq, 1);
                MPIDI_Request_set_type(newreq, MPIDI_REQUEST_TYPE_ACCUM_RESP);

		MPIR_Nest_incr();
                mpi_errno = NMPI_Type_get_true_extent(new_dtp->handle, 
                                                      &true_lb, &true_extent);
		MPIR_Nest_decr();
		/* --BEGIN ERROR HANDLING-- */
                if (mpi_errno) {
		    goto fn_exit;
		}
		/* --END ERROR HANDLING-- */

                MPID_Datatype_get_extent_macro(new_dtp->handle, extent); 

                tmp_buf = MPIU_Malloc(rreq->dev.user_count * 
                                      (MPIR_MAX(extent,true_extent)));  
		/* --BEGIN ERROR HANDLING-- */
                if (!tmp_buf) {
                    mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
                    goto fn_exit;
                }
		/* --END ERROR HANDLING-- */

                /* adjust for potential negative lower bound in datatype */
                tmp_buf = (void *)((char*)tmp_buf - true_lb);

                newreq->dev.user_buf = tmp_buf;
                newreq->dev.user_count = rreq->dev.user_count;
                newreq->dev.datatype = new_dtp->handle;
                newreq->dev.op = rreq->dev.op;
                newreq->dev.decr_ctr = rreq->dev.decr_ctr;
                newreq->dev.recv_data_sz = new_dtp->size *
                                           rreq->dev.user_count; 
                newreq->dev.real_user_buf = rreq->dev.real_user_buf;
 
                newreq->dev.datatype_ptr = new_dtp;
                /* this will cause the datatype to be freed when the
                   request is freed. */  

                MPID_Segment_init(newreq->dev.user_buf,
                                  newreq->dev.user_count,
                                  newreq->dev.datatype,
                                  &newreq->dev.segment, 0);
                newreq->dev.segment_first = 0;
                newreq->dev.segment_size = newreq->dev.recv_data_sz;

                mpi_errno = MPIDI_CH3U_Request_load_recv_iov(newreq);
                if (mpi_errno != MPI_SUCCESS)
                {
                    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|loadrecviov", 0);
                    goto fn_exit;
                }

                mpi_errno = MPIDI_CH3_iRead(vc, newreq);
                if (mpi_errno != MPI_SUCCESS)
                {
                    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|recvdata", 0);
                    goto fn_exit;
                }

                /* free dtype_info here. the dataloop gets freed
                   when the datatype gets freed (ie when the new request
                   gets freed. */
                MPIU_Free(rreq->dev.dtype_info);
            }


            if (MPIDI_Request_get_type(rreq) ==
                             MPIDI_REQUEST_TYPE_GET_RESP_DERIVED_DT) { 
                MPID_Request *newreq;
                MPID_Datatype *new_dtp;
                MPIDI_CH3_Pkt_t upkt;
                MPIDI_CH3_Pkt_get_resp_t * get_resp_pkt = &upkt.get_resp;
                MPID_IOV iov[MPID_IOV_LIMIT];
                int iov_n;
                
                /* create derived datatype */
                create_derived_datatype(rreq, &new_dtp);

                /* create new request for sending data */
                newreq = MPID_Request_create();
                MPIU_Object_set_ref(newreq, 1);
                newreq->kind = MPID_REQUEST_SEND;
                MPIDI_Request_set_type(newreq, MPIDI_REQUEST_TYPE_GET_RESP);

                newreq->dev.user_buf = rreq->dev.user_buf;
                newreq->dev.user_count = rreq->dev.user_count;
                newreq->dev.datatype = new_dtp->handle;
                newreq->dev.decr_ctr = rreq->dev.decr_ctr;
                newreq->dev.recv_data_sz = new_dtp->size *
                                           rreq->dev.user_count; 
                
                newreq->dev.datatype_ptr = new_dtp;
                /* this will cause the datatype to be freed when the
                   request is freed. */  

                get_resp_pkt->type = MPIDI_CH3_PKT_GET_RESP;
                get_resp_pkt->request = rreq->dev.request;
                
                iov[0].MPID_IOV_BUF = (void*) get_resp_pkt;
                iov[0].MPID_IOV_LEN = sizeof(*get_resp_pkt);

                MPID_Segment_init(newreq->dev.user_buf,
                                  newreq->dev.user_count,
                                  newreq->dev.datatype,
                                  &newreq->dev.segment, 0);
                newreq->dev.segment_first = 0;
                newreq->dev.segment_size = newreq->dev.recv_data_sz;

                iov_n = MPID_IOV_LIMIT - 1;
                mpi_errno = MPIDI_CH3U_Request_load_send_iov(newreq, &iov[1], &iov_n);
                if (mpi_errno == MPI_SUCCESS)
                {
                    iov_n += 1;
		
                    mpi_errno = MPIDI_CH3_iSendv(vc, newreq, iov, iov_n);
                    if (mpi_errno != MPI_SUCCESS)
                    {
                        MPIU_Object_set_ref(newreq, 0);
                        MPIDI_CH3_Request_destroy(newreq);
                        newreq = NULL;
                        mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|rmamsg", 0);
                        goto fn_exit;
                    }
                }

                /* free dtype_info here. the dataloop gets freed
                   when the datatype gets freed (ie when the new request
                   gets freed. */
                MPIU_Free(rreq->dev.dtype_info);
            }

	    MPIDI_CH3U_Request_complete(rreq);
	    break;
	}
	
	case MPIDI_CH3_CA_UNPACK_UEBUF_AND_COMPLETE:
	{
	    int recv_pending;
	    
            MPIDI_Request_recv_pending(rreq, &recv_pending);
	    if (!recv_pending)
	    { 
		if (rreq->dev.recv_data_sz > 0)
		{
		    MPIDI_CH3U_Request_unpack_uebuf(rreq);
		    MPIU_Free(rreq->dev.tmpbuf);
		}
	    }
	    else
	    {
		/* The receive has not been posted yet.  MPID_{Recv/Irecv}() is responsible for unpacking the buffer. */
	    }
	    
	    /* mark data transfer as complete and decrement CC */
	    rreq->dev.iov_count = 0;
	    MPIDI_CH3U_Request_complete(rreq);
	    break;
	}
	
	case MPIDI_CH3_CA_UNPACK_SRBUF_AND_COMPLETE:
	{
	    MPIDI_CH3U_Request_unpack_srbuf(rreq);
	    /* mark data transfer as complete and decrement CC */
	    rreq->dev.iov_count = 0;

            if (MPIDI_Request_get_type(rreq) == MPIDI_REQUEST_TYPE_PUT_RESP) { 
                /* atomically decrement RMA completion counter */
                /* FIXME: MT: this has to be done atomically */
                if (rreq->dev.decr_ctr != NULL)
                    *(rreq->dev.decr_ctr) -= 1;
            }

            if (MPIDI_Request_get_type(rreq) == MPIDI_REQUEST_TYPE_ACCUM_RESP) { 
                /* accumulate data from tmp_buf into user_buf */
                mpi_errno = do_accumulate_op(rreq);
                if (mpi_errno) goto fn_exit;

                /* atomically decrement RMA completion counter */
                /* FIXME: MT: this has to be done atomically */
                if (rreq->dev.decr_ctr != NULL)
                    *(rreq->dev.decr_ctr) -= 1;
            }

	    MPIDI_CH3U_Request_complete(rreq);
	    break;
	}
	
	case MPIDI_CH3_CA_UNPACK_SRBUF_AND_RELOAD_IOV:
	{
	    MPIDI_CH3U_Request_unpack_srbuf(rreq);
	    mpi_errno = MPIDI_CH3U_Request_load_recv_iov(rreq);
	    if (mpi_errno != MPI_SUCCESS)
	    {
		mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|loadrecviov",
						 "**ch3|loadrecviov %s", "MPIDI_CH3_CA_UNPACK_SRBUF_AND_RELOAD_IOV");
		goto fn_exit;
	    }
	    mpi_errno = MPIDI_CH3_iRead(vc, rreq);
	    if (mpi_errno != MPI_SUCCESS)
	    {
		mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|recvdata",
						 "**ch3|recvdata %s", "MPIDI_CH3_CA_UNPACK_SRBUF_AND_RELOAD_IOV");
		goto fn_exit;
	    }
	    break;
	}
	
	case MPIDI_CH3_CA_RELOAD_IOV:
	{
	    mpi_errno = MPIDI_CH3U_Request_load_recv_iov(rreq);
	    if (mpi_errno != MPI_SUCCESS)
	    {
		mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|loadrecviov",
						 "**ch3|loadrecviov %s", "MPIDI_CH3_CA_RELOAD_IOV");
		goto fn_exit;
	    }
	    mpi_errno = MPIDI_CH3_iRead(vc, rreq);
	    if (mpi_errno != MPI_SUCCESS)
	    {
		mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|recvdata",
						 "**ch3|recvdata %s", "MPIDI_CH3_CA_RELOAD_IOV");
		goto fn_exit;
	    }
	    break;
	}
	
	default:
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_INTERN, "**ch3|badca",
					     "**ch3|badca %d", rreq->dev.ca);
	    break;
	}
    }

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3U_HANDLE_RECV_REQ);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3U_Handle_recv_req_rndv
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3U_Handle_recv_req_rndv(MPIDI_VC * vc, MPID_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3U_HANDLE_RECV_REQ_RNDV);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3U_HANDLE_RECV_REQ_RNDV);

    assert(rreq->dev.ca < MPIDI_CH3_CA_END_CH3);
    
    switch(rreq->dev.ca)
    {
	case MPIDI_CH3_CA_COMPLETE:
	{
	    /* mark data transfer as complete and decrement CC */
            rreq->dev.iov_count = 0;
            
            if (MPIDI_Request_get_type(rreq) ==
                                 MPIDI_REQUEST_TYPE_PUT_RESP) { 
                /* atomically decrement RMA completion counter */
                /* FIXME: MT: this has to be done atomically */
                if (rreq->dev.decr_ctr != NULL)
                    *(rreq->dev.decr_ctr) -= 1;
            }

            if (MPIDI_Request_get_type(rreq) ==
                       MPIDI_REQUEST_TYPE_ACCUM_RESP) { 

                /* accumulate data from tmp_buf into user_buf */
                mpi_errno = do_accumulate_op(rreq);
                if (mpi_errno) goto fn_exit;

                /* atomically decrement RMA completion counter */
                /* FIXME: MT: this has to be done atomically */
                if (rreq->dev.decr_ctr != NULL)
                    *(rreq->dev.decr_ctr) -= 1;
            }

            if (MPIDI_Request_get_type(rreq) ==
                MPIDI_REQUEST_TYPE_PUT_RESP_DERIVED_DT) {
                MPID_Request *newreq;
                MPID_Datatype *new_dtp;

		printf ("Can't do derived datatype puts with rdma --darius\n");
		MPID_Abort (NULL, MPI_SUCCESS, -1);
                
                /* create derived datatype */
                create_derived_datatype(rreq, &new_dtp);

                /* create new request to get the data */
                newreq = MPID_Request_create();
                MPIU_Object_set_ref(newreq, 1);
                MPIDI_Request_set_type(newreq, MPIDI_REQUEST_TYPE_PUT_RESP);

                newreq->dev.user_buf = rreq->dev.user_buf;
                newreq->dev.user_count = rreq->dev.user_count;
                newreq->dev.datatype = new_dtp->handle;
                newreq->dev.decr_ctr = rreq->dev.decr_ctr;
                newreq->dev.recv_data_sz = new_dtp->size *
                                           rreq->dev.user_count; 
                
                newreq->dev.datatype_ptr = new_dtp;
                /* this will cause the datatype to be freed when the
                   request is freed. */  

                MPID_Segment_init(newreq->dev.user_buf,
                                  newreq->dev.user_count,
                                  newreq->dev.datatype,
                                  &newreq->dev.segment, 0);
                newreq->dev.segment_first = 0;
                newreq->dev.segment_size = newreq->dev.recv_data_sz;

                mpi_errno = MPIDI_CH3U_Request_load_recv_iov(newreq);
                if (mpi_errno != MPI_SUCCESS)
                {
                    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|loadrecviov", 0);
                    goto fn_exit;
                }

                mpi_errno = MPIDI_CH3_iRead(vc, newreq);
                if (mpi_errno != MPI_SUCCESS)
                {
                    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|recvdata", 0);
                    goto fn_exit;
                }

                /* free dtype_info here. the dataloop gets freed
                   when the datatype gets freed (ie when the new request
                   gets freed. */
                MPIU_Free(rreq->dev.dtype_info);
            }

            if (MPIDI_Request_get_type(rreq) ==
                             MPIDI_REQUEST_TYPE_ACCUM_RESP_DERIVED_DT) { 
                MPID_Request *newreq;
                MPID_Datatype *new_dtp;
                MPI_Aint true_lb, true_extent, extent;
                void *tmp_buf;
               
 		printf ("Can't do derived datatype accumulates with rdma --darius\n");
		MPID_Abort (NULL, MPI_SUCCESS, -1);
                
               /* create derived datatype */
                create_derived_datatype(rreq, &new_dtp);

                /* create new request to get the data */
                newreq = MPID_Request_create();
                MPIU_Object_set_ref(newreq, 1);
                MPIDI_Request_set_type(newreq, MPIDI_REQUEST_TYPE_ACCUM_RESP);

                mpi_errno = NMPI_Type_get_true_extent(new_dtp->handle, 
                                                      &true_lb, &true_extent);
                if (mpi_errno) goto fn_exit;

                MPID_Datatype_get_extent_macro(new_dtp->handle, extent); 

                tmp_buf = MPIU_Malloc(rreq->dev.user_count * 
                                      (MPIR_MAX(extent,true_extent)));  
                if (!tmp_buf) {
                    mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
                    goto fn_exit;
                }
                /* adjust for potential negative lower bound in datatype */
                tmp_buf = (void *)((char*)tmp_buf - true_lb);

                newreq->dev.user_buf = tmp_buf;
                newreq->dev.user_count = rreq->dev.user_count;
                newreq->dev.datatype = new_dtp->handle;
                newreq->dev.op = rreq->dev.op;
                newreq->dev.decr_ctr = rreq->dev.decr_ctr;
                newreq->dev.recv_data_sz = new_dtp->size *
                                           rreq->dev.user_count; 
                newreq->dev.real_user_buf = rreq->dev.real_user_buf;
 
                newreq->dev.datatype_ptr = new_dtp;
                /* this will cause the datatype to be freed when the
                   request is freed. */  

                MPID_Segment_init(newreq->dev.user_buf,
                                  newreq->dev.user_count,
                                  newreq->dev.datatype,
                                  &newreq->dev.segment, 0);
                newreq->dev.segment_first = 0;
                newreq->dev.segment_size = newreq->dev.recv_data_sz;

                mpi_errno = MPIDI_CH3U_Request_load_recv_iov(newreq);
                if (mpi_errno != MPI_SUCCESS)
                {
                    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|loadrecviov", 0);
                    goto fn_exit;
                }

                mpi_errno = MPIDI_CH3_iRead(vc, newreq);
                if (mpi_errno != MPI_SUCCESS)
                {
                    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|recvdata", 0);
                    goto fn_exit;
                }

                /* free dtype_info here. the dataloop gets freed
                   when the datatype gets freed (ie when the new request
                   gets freed. */
                MPIU_Free(rreq->dev.dtype_info);
            }


            if (MPIDI_Request_get_type(rreq) ==
                             MPIDI_REQUEST_TYPE_GET_RESP_DERIVED_DT) { 
                MPID_Request *newreq;
                MPID_Datatype *new_dtp;
                MPIDI_CH3_Pkt_t upkt;
                MPIDI_CH3_Pkt_get_resp_t * get_resp_pkt = &upkt.get_resp;
                MPID_IOV iov[MPID_IOV_LIMIT];
                int iov_n;
                
  		printf ("Can't do derived datatype gets with rdma --darius\n");
		MPID_Abort (NULL, MPI_SUCCESS, -1);
		
               /* create derived datatype */
                create_derived_datatype(rreq, &new_dtp);

                /* create new request for sending data */
                newreq = MPID_Request_create();
                MPIU_Object_set_ref(newreq, 1);
                newreq->kind = MPID_REQUEST_SEND;
                MPIDI_Request_set_type(newreq, MPIDI_REQUEST_TYPE_GET_RESP);

                newreq->dev.user_buf = rreq->dev.user_buf;
                newreq->dev.user_count = rreq->dev.user_count;
                newreq->dev.datatype = new_dtp->handle;
                newreq->dev.decr_ctr = rreq->dev.decr_ctr;
                newreq->dev.recv_data_sz = new_dtp->size *
                                           rreq->dev.user_count; 
                
                newreq->dev.datatype_ptr = new_dtp;
                /* this will cause the datatype to be freed when the
                   request is freed. */  

                get_resp_pkt->type = MPIDI_CH3_PKT_GET_RESP;
                get_resp_pkt->request = rreq->dev.request;
                
                iov[0].MPID_IOV_BUF = (void*) get_resp_pkt;
                iov[0].MPID_IOV_LEN = sizeof(*get_resp_pkt);

                MPID_Segment_init(newreq->dev.user_buf,
                                  newreq->dev.user_count,
                                  newreq->dev.datatype,
                                  &newreq->dev.segment, 0);
                newreq->dev.segment_first = 0;
                newreq->dev.segment_size = newreq->dev.recv_data_sz;

                iov_n = MPID_IOV_LIMIT - 1;
                mpi_errno = MPIDI_CH3U_Request_load_send_iov(newreq, &iov[1], &iov_n);
                if (mpi_errno == MPI_SUCCESS)
                {
                    iov_n += 1;
		
                    mpi_errno = MPIDI_CH3_iSendv(vc, newreq, iov, iov_n);
                    if (mpi_errno != MPI_SUCCESS)
                    {
                        MPIU_Object_set_ref(newreq, 0);
                        MPIDI_CH3_Request_destroy(newreq);
                        newreq = NULL;
                        mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|rmamsg", 0);
                        goto fn_exit;
                    }
                }

                /* free dtype_info here. the dataloop gets freed
                   when the datatype gets freed (ie when the new request
                   gets freed. */
                MPIU_Free(rreq->dev.dtype_info);
            }

	    MPIDI_CH3U_Request_complete(rreq);
	    break;
	}
	
	case MPIDI_CH3_CA_UNPACK_UEBUF_AND_COMPLETE:
	{
	    int recv_pending;
	    
            MPIDI_Request_recv_pending(rreq, &recv_pending);
	    if (!recv_pending)
	    { 
		if (rreq->dev.recv_data_sz > 0)
		{
		    MPIDI_CH3U_Request_unpack_uebuf(rreq);
		    MPIU_Free(rreq->dev.tmpbuf);
		}
	    }
	    else
	    {
		/* The receive has not been posted yet.  MPID_{Recv/Irecv}() is responsible for unpacking the buffer. */
	    }
	    
	    /* mark data transfer as complete and decrement CC */
	    rreq->dev.iov_count = 0;
	    MPIDI_CH3U_Request_complete(rreq);
	    break;
	}
	
	case MPIDI_CH3_CA_UNPACK_SRBUF_AND_COMPLETE:
	{
	    MPIDI_CH3U_Request_unpack_srbuf(rreq);
	    /* mark data transfer as complete and decrement CC */
	    rreq->dev.iov_count = 0;

            if (MPIDI_Request_get_type(rreq) == MPIDI_REQUEST_TYPE_PUT_RESP) { 
                /* atomically decrement RMA completion counter */
                /* FIXME: MT: this has to be done atomically */
                if (rreq->dev.decr_ctr != NULL)
                    *(rreq->dev.decr_ctr) -= 1;
            }

            if (MPIDI_Request_get_type(rreq) == MPIDI_REQUEST_TYPE_ACCUM_RESP) { 
                /* accumulate data from tmp_buf into user_buf */
                mpi_errno = do_accumulate_op(rreq);
                if (mpi_errno) goto fn_exit;

                /* atomically decrement RMA completion counter */
                /* FIXME: MT: this has to be done atomically */
                if (rreq->dev.decr_ctr != NULL)
                    *(rreq->dev.decr_ctr) -= 1;
            }

	    MPIDI_CH3U_Request_complete(rreq);
	    break;
	}
	
	case MPIDI_CH3_CA_UNPACK_SRBUF_AND_RELOAD_IOV:
	{
	    MPIDI_CH3U_Request_unpack_srbuf(rreq);
	    mpi_errno = MPIDI_CH3U_Request_load_recv_iov(rreq);
	    if (mpi_errno != MPI_SUCCESS)
	    {
		mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|loadrecviov",
						 "**ch3|loadrecviov %s", "MPIDI_CH3_CA_UNPACK_SRBUF_AND_RELOAD_IOV");
		goto fn_exit;
	    }
	    break;
	}
	
	case MPIDI_CH3_CA_RELOAD_IOV:
	{
	    mpi_errno = MPIDI_CH3U_Request_load_recv_iov(rreq);
	    if (mpi_errno != MPI_SUCCESS)
	    {
		mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|loadrecviov",
						 "**ch3|loadrecviov %s", "MPIDI_CH3_CA_RELOAD_IOV");
		goto fn_exit;
	    }
	    break;
	}
	
	default:
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_INTERN, "**ch3|badca",
					     "**ch3|badca %d", rreq->dev.ca);
	    break;
	}
    }

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3U_HANDLE_RECV_REQ_RNDV);
    return mpi_errno;
}



static int create_derived_datatype(MPID_Request *req, MPID_Datatype **dtp)
{
    MPIDI_RMA_dtype_info *dtype_info;
    void *dataloop;
    MPID_Datatype *new_dtp;
    int mpi_errno=MPI_SUCCESS;
    MPI_Aint ptrdiff;

    dtype_info = req->dev.dtype_info;
    dataloop = req->dev.dataloop;

    /* allocate new datatype object and handle */
    new_dtp = (MPID_Datatype *) MPIU_Handle_obj_alloc(&MPID_Datatype_mem);
    if (!new_dtp) {
        mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
        return mpi_errno;
    }

    *dtp = new_dtp;
            
    /* Note: handle is filled in by MPIU_Handle_obj_alloc() */
    MPIU_Object_set_ref(new_dtp, 1);
    new_dtp->is_permanent = 0;
    new_dtp->is_committed = 1;
    new_dtp->attributes   = 0;
    new_dtp->cache_id     = 0;
    new_dtp->name[0]      = 0;
    new_dtp->is_contig = dtype_info->is_contig;
    new_dtp->n_contig_blocks = dtype_info->n_contig_blocks; 
    new_dtp->size = dtype_info->size;
    new_dtp->extent = dtype_info->extent;
    new_dtp->dataloop_size = dtype_info->dataloop_size;
    new_dtp->dataloop_depth = dtype_info->dataloop_depth; 
    new_dtp->eltype = dtype_info->eltype;
    /* set dataloop pointer */
    new_dtp->dataloop = req->dev.dataloop;
    
    new_dtp->ub = dtype_info->ub;
    new_dtp->lb = dtype_info->lb;
    new_dtp->true_ub = dtype_info->true_ub;
    new_dtp->true_lb = dtype_info->true_lb;
    new_dtp->has_sticky_ub = dtype_info->has_sticky_ub;
    new_dtp->has_sticky_lb = dtype_info->has_sticky_lb;
    /* update pointers in dataloop */
    ptrdiff = (MPI_Aint)((char *) (new_dtp->dataloop) - (char *)
                         (dtype_info->dataloop));
    
    MPID_Dataloop_update(new_dtp->dataloop, ptrdiff);

    new_dtp->contents = NULL;

    return mpi_errno;
}


static int do_accumulate_op(MPID_Request *rreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPI_Aint true_lb, true_extent;
    MPI_User_function *uop;

    if (HANDLE_GET_KIND(rreq->dev.op) == HANDLE_KIND_BUILTIN) {
        /* get the function by indexing into the op table */
        uop = MPIR_Op_table[(rreq->dev.op)%16 - 1];
    }
    else {
        mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OP, "**opnotpredefined", "**opnotpredefined %d", rreq->dev.op );
        return mpi_errno;
    }
    
    if (HANDLE_GET_KIND(rreq->dev.datatype) == HANDLE_KIND_BUILTIN) {
        (*uop)(rreq->dev.user_buf, rreq->dev.real_user_buf,
               &(rreq->dev.user_count), &(rreq->dev.datatype));
    }
    else { /* derived datatype */
        MPID_Segment *segp;
        DLOOP_VECTOR *dloop_vec;
        MPI_Aint first, last;
        int vec_len, i, type_size, count;
        MPI_Datatype type;
        MPID_Datatype *dtp;
        
        segp = MPID_Segment_alloc();
        if (!segp) {
            mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 ); 
            return mpi_errno;
        }
        MPID_Segment_init(NULL, rreq->dev.user_count,
			  rreq->dev.datatype, segp, 0);
        first = 0;
        last  = SEGMENT_IGNORE_LAST;
        
        MPID_Datatype_get_ptr(rreq->dev.datatype, dtp);
        vec_len = dtp->n_contig_blocks * rreq->dev.user_count + 1; 
        /* +1 needed because Rob says so */
        dloop_vec = (DLOOP_VECTOR *)
            MPIU_Malloc(vec_len * sizeof(DLOOP_VECTOR));
        if (!dloop_vec) {
            mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 ); 
            return mpi_errno;
        }
        
        MPID_Segment_pack_vector(segp, first, &last, dloop_vec, &vec_len);
        
        type = dtp->eltype;
        type_size = MPID_Datatype_get_basic_size(type);
        for (i=0; i<vec_len; i++) {
            count = (dloop_vec[i].DLOOP_VECTOR_LEN)/type_size;
            (*uop)((char *)rreq->dev.user_buf + PtrToInt( dloop_vec[i].DLOOP_VECTOR_BUF ),
                   (char *)rreq->dev.real_user_buf + PtrToInt( dloop_vec[i].DLOOP_VECTOR_BUF ),
                   &count, &type);
        }
        
        MPID_Segment_free(segp);
        MPIU_Free(dloop_vec);
    }
    /* free the temporary buffer */
    mpi_errno = NMPI_Type_get_true_extent(rreq->dev.datatype, 
                                          &true_lb, &true_extent); 
    if (mpi_errno) return mpi_errno;
    
    MPIU_Free((char *) rreq->dev.user_buf + true_lb);

    return mpi_errno;
}
