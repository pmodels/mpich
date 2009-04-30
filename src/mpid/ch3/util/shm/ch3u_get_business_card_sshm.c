/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"
#include "pmi.h"
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif


/*  MPIDI_CH3U_Get_business_card_sshm - does sshm specific portion of getting 
 *                    a business card
 *     bc_val_p     - business card value buffer pointer, updated to the next 
 *                    available location or freed if published.
 *     val_max_sz_p - ptr to maximum value buffer size reduced by the number 
 *                    of characters written
 */

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3U_Get_business_card_sshm
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3U_Get_business_card_sshm(char **bc_val_p, int *val_max_sz_p)
{
    char queue_name[100];
    int mpi_errno;
    MPIDI_CH3I_PG *pgch = 
	(MPIDI_CH3I_PG *)MPIDI_Process.my_pg->channel_private;
#ifdef HAVE_SHARED_PROCESS_READ
    char pid_str[20];
#ifdef HAVE_WINDOWS_H
    DWORD pid;
#else
    int pid;
#endif
#endif

    /* must ensure shm_hostname is set prior to this upcall */
/*     printf("before first MPIU_Str_add_string_arg\n"); */
    mpi_errno = MPIU_Str_add_string_arg(bc_val_p, val_max_sz_p, 
					MPIDI_CH3I_SHM_HOST_KEY, 
					pgch->shm_hostname);
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno != MPIU_STR_SUCCESS)
    {
	if (mpi_errno == MPIU_STR_NOMEM) {
	    MPIU_ERR_SET(mpi_errno,MPI_ERR_OTHER, "**buscard_len");
	}
	else {
	    MPIU_ERR_SET(mpi_errno,MPI_ERR_OTHER, "**buscard");
	}
	return mpi_errno;
    }
    /* --END ERROR HANDLING-- */

/*     printf("before MPIDI_CH3I_BootstrapQ_tostring\n"); */
    /* brad: must ensure that KVS was already appropriately updated (i.e. bootstrapQ was set) */
    queue_name[0] = '\0';
    mpi_errno = MPIDI_CH3I_BootstrapQ_tostring(pgch->bootstrapQ, queue_name, 
					       sizeof(queue_name));
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno != 0) {
        mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_kvs_get", "**pmi_kvs_get %d", mpi_errno);
        return mpi_errno;
    }
    /* --END ERROR HANDLING-- */
    
    mpi_errno = MPIU_Str_add_string_arg(bc_val_p, val_max_sz_p, 
					MPIDI_CH3I_SHM_QUEUE_KEY, queue_name);
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno != MPIU_STR_SUCCESS)
    {
	if (mpi_errno == MPIU_STR_NOMEM) {
	    MPIU_ERR_SET(mpi_errno,MPI_ERR_OTHER, "**buscard_len");
	}
	else {
	    MPIU_ERR_SET(mpi_errno,MPI_ERR_OTHER, "**buscard");
	}
	return mpi_errno;
    }
    /* --END ERROR HANDLING-- */

#ifdef HAVE_SHARED_PROCESS_READ
#ifdef HAVE_WINDOWS_H
    pid = GetCurrentProcessId();
#else
    pid = getpid();
#endif
    MPIU_Snprintf(pid_str, 20, "%d", pid);
    mpi_errno = MPIU_Str_add_string_arg(bc_val_p, val_max_sz_p, 
					MPIDI_CH3I_SHM_PID_KEY, pid_str);
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno != MPIU_STR_SUCCESS)
    {
	if (mpi_errno == MPIU_STR_NOMEM) {
	    MPIU_ERR_SET(mpi_errno,MPI_ERR_OTHER, "**buscard_len");
	}
	else {
	    MPIU_ERR_SET(mpi_errno,MPI_ERR_OTHER, "**buscard");
	}
	return mpi_errno;
    }
    /* --END ERROR HANDLING-- */
#endif

    return MPI_SUCCESS;
}
