/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "shmimpl.h"

#ifdef WITH_METHOD_SHM

#ifdef HAVE_UNISTD_H
#include <unistd.h> /* gethostname */
#endif

SHM_PerProcess SHM_Process;
MPIDI_VC_functions g_shm_vc_functions = 
{
    shm_post_read,
    NULL, /*shm_car_head_enqueue_read,*/
    shm_merge_with_unexpected,
    NULL, /*shm_merge_with_posted,*/
    NULL, /*shm_merge_unexpected_data,*/
    shm_post_write,
    NULL, /*shm_car_head_enqueue_write,*/
    NULL, /*shm_reset_car,*/
    NULL, /*shm_setup_packet_car,*/
    NULL /*shm_post_read_pkt*/
};

/*@
   shm_init - initialize the shared memory method

   Notes:
@*/
int shm_init( void )
{
#ifdef HAVE_WINDOWS_H
    DWORD n = 100;
    MPIDI_STATE_DECL(MPID_STATE_SHM_INIT);

    MPIDI_FUNC_ENTER(MPID_STATE_SHM_INIT);

    GetComputerName(SHM_Process.host, &n);
#else
    MPIDI_STATE_DECL(MPID_STATE_SHM_INIT);

    MPIDI_FUNC_ENTER(MPID_STATE_SHM_INIT);

    gethostname(SHM_Process.host, 100);
#endif

    MPIDI_FUNC_EXIT(MPID_STATE_SHM_INIT);
    return MPI_SUCCESS;
}


/*@
   shm_finalize - finalize the shared memory method

   Notes:
@*/
int shm_finalize( void )
{
    MPIDI_STATE_DECL(MPID_STATE_SHM_FINALIZE);
    MPIDI_FUNC_ENTER(MPID_STATE_SHM_FINALIZE);
    MPIDI_FUNC_EXIT(MPID_STATE_SHM_FINALIZE);
    return MPI_SUCCESS;
}

#endif
