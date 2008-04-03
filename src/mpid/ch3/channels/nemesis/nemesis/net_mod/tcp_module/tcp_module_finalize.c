/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "tcp_module_impl.h"
#include <sched.h>

/*#define TRACE */
#define NEM_TCP_BUF_SIZE    MPID_NEM_OPT_HEAD_LEN
#define NEM_TCP_MASTER_RANK 0

#undef FUNCNAME
#define FUNCNAME MPID_nem_tcp_module_finalize
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_tcp_module_finalize ()
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_TCP_MODULE_FINALIZE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_TCP_MODULE_FINALIZE);

    if (MPID_nem_mem_region.ext_procs > 0)
    {
        node_t *nodes = MPID_nem_tcp_internal_vars.nodes ;
        char buff[NEM_TCP_BUF_SIZE] = TCP_END_STRING;
        int index;
        int grank;

#ifdef TRACE
        fprintf(stderr,"[%i] --- TCP END PENDING SEND \n",MPID_nem_mem_region.rank);
#endif
        while( MPID_nem_tcp_internal_vars.n_pending_send > 0 )
	{
            mpi_errno = MPID_nem_tcp_module_poll( MPID_NEM_POLL_OUT );
            if (mpi_errno) MPIU_ERR_POP (mpi_errno);
            sched_yield();
	}

#ifdef TRACE
        fprintf(stderr,"[%i] --- TCP END PENDING DONE  1\n",MPID_nem_mem_region.rank);
#endif


        for(index = 0 ; index < MPID_nem_mem_region.ext_procs ; index++)
	{
            grank = MPID_nem_mem_region.ext_ranks[index];
            if ((grank != MPID_nem_mem_region.rank) && (!MPID_NEM_IS_LOCAL (grank)))
	    {
                write(nodes[grank].desc, buff,NEM_TCP_BUF_SIZE);
#ifdef TRACE
                fprintf(stderr,"[%i] --- WROTE TO PROC %i on desc %i: %s, size %i\n",MPID_nem_mem_region.rank, grank, nodes[grank].desc, buff,NEM_TCP_BUF_SIZE);
#endif
	    }
	}

#ifdef TRACE
        fprintf(stderr,"[%i] --- TCP END PENDING  3 : waiting for %i processes \n",MPID_nem_mem_region.rank, MPID_nem_tcp_internal_vars.nb_procs);
#endif
        while (MPID_nem_tcp_internal_vars.nb_procs > 0)
	{
            mpi_errno = MPID_nem_tcp_module_poll_recv();
            if (mpi_errno) MPIU_ERR_POP (mpi_errno);
            sched_yield();
	}

#ifdef TRACE
        fprintf(stderr,"[%i] --- TCP END PENDING  4 : %i processes left \n",MPID_nem_mem_region.rank, MPID_nem_tcp_internal_vars.nb_procs);
#endif /* TRACE */

    }

    mpi_errno = MPID_nem_tcp_module_ckpt_shutdown();
    if (mpi_errno) MPIU_ERR_POP (mpi_errno);

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_TCP_MODULE_FINALIZE);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_tcp_module_ckpt_shutdown
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_tcp_module_ckpt_shutdown ()
{
    int mpi_errno = MPI_SUCCESS;
    int ret;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_TCP_MODULE_CKPT_SHUTDOWN);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_TCP_MODULE_CKPT_SHUTDOWN);

    if (MPID_nem_mem_region.ext_procs > 0)
    {
        int index;
        int grank;

        /* close the sockets */
        for (index = 0 ; index < MPID_nem_mem_region.ext_procs ; index++)
	{
            grank = MPID_nem_mem_region.ext_ranks[index];
            if ((grank != MPID_nem_mem_region.rank) && (!MPID_NEM_IS_LOCAL (grank)))
	    {
                ret = shutdown ((MPID_nem_tcp_internal_vars.nodes)[grank].desc, SHUT_RDWR);
                MPIU_ERR_CHKANDJUMP2 (ret == -1, mpi_errno, MPI_ERR_OTHER, "**shutdown", "**shutdown %s %d", strerror (errno), errno);
                ret = close ((MPID_nem_tcp_internal_vars.nodes)[grank].desc);
                MPIU_ERR_CHKANDJUMP2 (ret == -1, mpi_errno, MPI_ERR_OTHER, "**closesocket", "**closesocket %s %d", strerror (errno), errno);
	    }
	}

#ifdef TRACE
        fprintf(stderr,"[%i] --- sockets closed .... \n",MPID_nem_mem_region.rank);
#endif
    }

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_TCP_MODULE_CKPT_SHUTDOWN);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}
