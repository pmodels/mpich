/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id: mpid_finalize.c,v 1.18 2002/10/22 22:26:14 David Exp $
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"
#include "pmi.h"

/*@
   MPID_Finalize - Terminates mm device

   Notes:

.N fortran

.N Errors
.N MPI_SUCCESS
.N ... others
@*/
int MPID_Finalize( void )
{
    /*MPIU_Timer_finalize();*/ /* called in MPI_Finalize */

    /* finalize the methods */
    packer_finalize();
    unpacker_finalize();
#ifdef WITH_METHOD_SHM
    shm_finalize();
#endif
#ifdef WITH_METHOD_TCP
    tcp_finalize();
#endif
#ifdef WITH_METHOD_SOCKET
    socket_finalize();
#endif
#ifdef WITH_METHOD_VIA
    via_finalize();
#endif
#ifdef WITH_METHOD_VIA_RDMA
    via_rdma_finalize();
#endif
#ifdef WITH_METHOD_IB
    ib_finalize();
#endif
#ifdef WITH_METHOD_NEW
    new_method_finalize();
#endif

    mm_car_finalize();
    mm_vc_finalize();

    /*dbg_printf("+PMI_Barrier");*/
    PMI_Barrier();
    /*dbg_printf("-\n+PMI_Finalize");*/
    PMI_Finalize();
    /*dbg_printf("-\n");*/

    bsocket_finalize();

    free(MPID_Process.pmi_kvsname);

    return MPI_SUCCESS;
}
