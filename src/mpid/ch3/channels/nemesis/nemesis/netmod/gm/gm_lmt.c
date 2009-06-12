/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "gm_impl.h"

#define FREE_LMT_QUEUE_ELEMENTS MPID_NEM_NUM_CELLS

MPID_nem_gm_lmt_queue_head_t MPID_nem_gm_lmt_queue = {0};
MPID_nem_gm_lmt_queue_t *MPID_nem_gm_lmt_free_queue = 0;

#undef FUNCNAME
#define FUNCNAME MPID_nem_gm_lmt_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_gm_lmt_init()
{
    int mpi_errno = MPI_SUCCESS;
    int i;
    
    MPID_nem_gm_lmt_queue.head = NULL;
    MPID_nem_gm_lmt_queue.tail = NULL;

    MPID_nem_gm_lmt_free_queue = NULL;
    
    for (i = 0; i < FREE_LMT_QUEUE_ELEMENTS; ++i)
    {
	MPID_nem_gm_lmt_queue_t *e;
	
	e = MPIU_Malloc (sizeof (MPID_nem_gm_lmt_queue_t));
        if (e == NULL) MPIU_CHKMEM_SETERR (mpi_errno, sizeof (MPID_nem_gm_send_queue_t), "gm module lmt queue");
	e->next = MPID_nem_gm_lmt_free_queue;
	MPID_nem_gm_lmt_free_queue = e;
    }

 fn_exit:
    return mpi_errno;
 fn_fail:
     goto fn_exit;
}

int
MPID_nem_gm_lmt_finalize()
{
    MPID_nem_gm_lmt_queue_t *e;

    while (MPID_nem_gm_lmt_free_queue)
    {
	e = MPID_nem_gm_lmt_free_queue;
	MPID_nem_gm_lmt_free_queue = e->next;
	MPIU_Free (e);
    }
    return MPI_SUCCESS;
}


static inline int
MPID_nem_gm_lmt_pre (struct iovec *iov, size_t n_iov, MPIDI_VC_t *remote_vc, struct iovec *cookie)
{
    int ret = 0;
    int i, j;
    struct iovec *iov_copy;
    
    for (i = 0; i < n_iov; ++i)
    {
	ret = MPID_nem_gm_register_mem (iov[i].iov_base, iov[i].iov_len);
	if (ret != 0)
	{
	    ret = -1;
	    goto error_exit;
	}
    }
    iov_copy = MPIU_Malloc (sizeof (struct iovec) * n_iov);
    if (iov_copy == 0)
    {
	ret = -1;
	goto error_exit;
    }
    MPIU_Memcpy (iov_copy, iov, sizeof (struct iovec) * n_iov);
    cookie->iov_base = iov_copy;
    cookie->iov_len = sizeof (struct iovec) * n_iov;

    return ret;
    
 error_exit:
    for (j = i-1; j <= 0; --j)
    {
	MPID_nem_gm_deregister_mem (iov[j].iov_base, iov[j].iov_len);
    }

    return ret;
}

int
MPID_nem_gm_lmt_send_pre (struct iovec *iov, size_t n_iov, MPIDI_VC_t *dest, struct iovec *cookie)
{
    return MPID_nem_gm_lmt_pre (iov, n_iov, dest, cookie);
}

int
MPID_nem_gm_lmt_recv_pre (struct iovec *iov, size_t n_iov, MPIDI_VC_t *src, struct iovec *cookie)
{
    return MPID_nem_gm_lmt_pre (iov, n_iov, src, cookie);
}

int
MPID_nem_gm_lmt_start_send (MPIDI_VC_t *dest, struct iovec s_cookie, struct iovec r_cookie, int *completion_ctr)
{
    /* We're using gets to transfer the data so, this should not be called */
    return -1;
}

int
MPID_nem_gm_lmt_start_recv (MPIDI_VC_t *src_vc, struct iovec s_cookie, struct iovec r_cookie, int *completion_ctr)
{
    int ret;
    struct iovec *s_iov;
    struct iovec *r_iov;
    int s_n_iov;
    int r_n_iov;
    int s_offset;
    int r_offset;

    s_iov = s_cookie.iov_base;
    s_n_iov = s_cookie.iov_len / sizeof (struct iovec);
    r_iov = r_cookie.iov_base;
    r_n_iov = r_cookie.iov_len / sizeof (struct iovec);
    r_offset = 0;
    s_offset = 0;
    
    ret = MPID_nem_gm_lmt_do_get (VC_FIELD(src_vc, gm_node_id), VC_FIELD(src_vc, gm_port_id), &r_iov, &r_n_iov, &r_offset, &s_iov, &s_n_iov, &s_offset,
				completion_ctr);
    if (ret == LMT_AGAIN)
    {
	MPID_nem_gm_lmt_queue_t *e = MPID_nem_gm_queue_alloc (lmt);
	if (!e)
	{
	    printf ("error: malloc failed\n");
	    return -1;
	}
	e->node_id = VC_FIELD(src_vc, gm_node_id);
	e->port_id = VC_FIELD(src_vc, gm_port_id);
	e->r_iov = r_iov;
	e->r_n_iov = r_n_iov;
	e->r_offset = r_offset;
	e->s_iov = s_iov;
	e->s_n_iov = s_n_iov;
	e->s_offset = s_offset;
	e->compl_ctr = completion_ctr;
	MPID_nem_gm_queue_enqueue (lmt, e);
    }
    else if (ret == LMT_FAILURE)
    {
	printf ("error: MPID_nem_gm_lmt_do_get() failed \n");
	return -1;	
    }
    return 0;
}

static inline int
MPID_nem_gm_lmt_post (struct iovec cookie)
{
    int ret = 0;
    int i;
    struct iovec *iov;
    int n_iov;

    iov = cookie.iov_base;
    n_iov = cookie.iov_len / sizeof (struct iovec);
    
    for (i = 0; i < n_iov; ++i)
    {
	MPID_nem_gm_deregister_mem (iov[i].iov_base, iov[i].iov_len);
    }
    
    MPIU_Free (iov);

    return ret;
}

int
MPID_nem_gm_lmt_send_post (struct iovec cookie)
{
    return MPID_nem_gm_lmt_post (cookie);
}

int
MPID_nem_gm_lmt_recv_post (struct iovec cookie)
{
    return MPID_nem_gm_lmt_post (cookie);
}

static void
get_callback (struct gm_port *p, void *completion_ctr, gm_status_t status)
{
    if (status != GM_SUCCESS)
    {
	gm_perror ("Get error", status);
    }

    ++MPID_nem_module_gm_num_send_tokens;

    OPA_decr_int ((int *)completion_ctr);
}


int
MPID_nem_gm_lmt_do_get (int node_id, int port_id, struct iovec **r_iov, int *r_n_iov, int *r_offset, struct iovec **s_iov, int *s_n_iov,
		      int *s_offset, int *compl_ctr)
{
    int s_i, r_i;
    char *s_buf;
    char *r_buf;
    int s_len;
    int r_len;
    int len;

    s_i = 0;
    r_i = 0;
    s_buf = (char *)((*s_iov)[s_i].iov_base) + *s_offset;
    r_buf = (char *)((*r_iov)[r_i].iov_base) + *r_offset;
    s_len = (*s_iov)[s_i].iov_len;
    r_len = (*r_iov)[r_i].iov_len;
    
    while (1)
    {
	if (MPID_nem_module_gm_num_send_tokens == 0)
	{
	    *s_offset = s_buf - (char *)((*s_iov)[s_i].iov_base);
	    *r_offset = r_buf - (char *)((*r_iov)[r_i].iov_base);
	    *s_iov += s_i;
	    *r_iov += r_i;
	    *s_n_iov -= s_i;
	    *r_n_iov -= r_i;
	    return LMT_AGAIN;
	}

	if (s_len < r_len)
	    len = s_len;
	else
	    len = r_len;
	if (len > 0)
	{
	    OPA_incr_int(compl_ctr);
/*             gm_get (MPID_nem_module_gm_port, (long)s_buf, r_buf, len, GM_LOW_PRIORITY, node_id, port_id, get_callback, compl_ctr); */
	    
	    --MPID_nem_module_gm_num_send_tokens;
	    
	    s_len -= len;
	    r_len -= len;
	    s_buf += len;
	    r_buf += len;
	}
	
	if (s_len == 0)
	{
	    ++s_i;
	    if (s_i == *s_n_iov)
		break;
	    s_buf = (*s_iov)[s_i].iov_base;
	    s_len = (*s_iov)[s_i].iov_len;
	}
	if (r_len == 0)
	{
	    ++r_i;
	    if (r_i == *r_n_iov)
		break;
	    r_buf = (*r_iov)[r_i].iov_base;
	    r_len = (*r_iov)[r_i].iov_len;
	}   
    }

    if (s_i != *s_n_iov || r_i != *r_n_iov)
    {
	printf ("error: iov mismatch in MPID_nem_gm_lmt_start_recv\n");
	return LMT_FAILURE;
    }
    return LMT_COMPLETE;
}

