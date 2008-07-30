/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#include "mpidi_ch3_impl.h"
#include "pmi.h"


/*  MPIDI_CH3U_Finalize_sshm - does scalable shared memory specific channel 
    finalization
 */

/* FIXME: Should this be registered as a finalize handler?  Should there be
   a corresponding abort handler? */

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3U_Finalize_sshm
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3U_Finalize_sshm(void)
{
    int mpi_errno = MPI_SUCCESS;

    MPIDI_PG_t * pg;
    MPIDI_PG_t * pg_next;
    MPIDI_CH3I_PG *pgch;
    MPIDI_PG_iterator iter;
    int inuse;

#if 0
    /* Free resources allocated in CH3_Init() */
    while (MPIDI_CH3I_Process.shm_reading_list)
    {
	pgch = (MPIDI_CH3I_PG *)MPIDI_CH3I_Process.shm_reading_list->channel_private;
	MPIDI_CH3I_SHM_Release_mem(&pgch->shm_read_queue_info);
	MPIDI_CH3I_Process.shm_reading_list = pgch->shm_next_reader;
    }
    while (MPIDI_CH3I_Process.shm_writing_list)
    {
	pgch = (MPIDI_CH3I_PG *)MPIDI_CH3I_Process.shm_writing_list->channel_private;
	MPIDI_CH3I_SHM_Release_mem(&pgch->shm_write_queue_info);
	MPIDI_CH3I_Process.shm_writing_list = pgch->shm_next_writer;
    }
#endif

    /* brad : used to unlink this within Init but now done in finalize in case 
     * someone spawned needs to attach to this bootstrapQ.
     */
    pgch = (MPIDI_CH3I_PG *)MPIDI_Process.my_pg->channel_private;
    mpi_errno = MPIDI_CH3I_BootstrapQ_unlink(pgch->bootstrapQ);
    if (mpi_errno != MPI_SUCCESS) {
	MPIU_ERR_SET(mpi_errno,MPI_ERR_OTHER, "**boot_unlink");
    }
    
    mpi_errno = MPIDI_CH3I_BootstrapQ_destroy(pgch->bootstrapQ);
    if (mpi_errno != MPI_SUCCESS) {
	MPIU_ERR_SET(mpi_errno,MPI_ERR_OTHER, "**finalize_boot");
    }
    
    /* brad : added for dynamic processes in ssm.  needed because the vct's 
     * can't be freed
     * earlier since the vc's themselves are still needed here to walk though 
     * and free their member fields.
     */

    MPIDI_PG_Get_iterator(&iter);
    MPIDI_PG_Get_next(&iter, &pg);
    /* This Get_next causes us to skip the process group associated with
       out MPI_COMM_WORLD.  */
    MPIDI_PG_Get_next(&iter, &pg);
    while(pg)
    {
	MPIDI_PG_Get_next(&iter, &pg_next);
        MPIDI_PG_release_ref(pg, &inuse);
        if (inuse == 0)
        {
            MPIDI_PG_Destroy(pg);
        }
        pg = pg_next;
    }
    
    return mpi_errno;
}
