/******************************************************************************
			 Shared Memory Implementation
******************************************************************************/

MPID_shm_Win_create(base, size, disp_unit, info, comm, dwin)
{
    if (!arch.cc_shm)
    {
	dwin->no_prev_put = 0;
    }

    process_mutex_init(win.process_mutex);
}
/* MPID_shm_Win_create() */


MPID_shm_Win_free(dwin)
{
    process_mutex_destroy(dwin->process_mutex);
}
/* MPID_shm_Win_free() */


MPID_shm_Win_fence(assert, win)
{
    if (!arch.cc_shm && !(assert & MPI_MODE_NOSTORE))
    {
	/* flush cache and/or write buffer */
    }

    rc = MPI_Barrier(dwin->comm);

    if (!arch.cc_shm)
    {
	if (!dwin->no_prev_put)
        {
	    /* invlidate cache */
	}

	dwin->no_prev_put = assert & (MPI_MODE_NOPUT | MPI_MODE_NOSUCCEED);
    }

    rc = MPI_SUCCESS;
}
/* MPID_shm_Win_fence() */


MPID_shm_Get(origin_addr, origin_count, origin_datatype,
        target_rank, target_disp, target_count, target_datatype, win)
{
    target_addr = MPIR_Win_compute_addr(dwin, target_rank, target_disp);
    rc = MPIR_Copy_data(target_addr, target_count, target_datatype,
			origin_adrr, origin_count, origin_datatype);
}
/* MPID_shm_get() */


MPID_shm_Put(origin_addr, origin_count, origin_datatype,
        target_rank, target_disp, target_count, target_datatype, win)
{
    target_addr = MPIR_Win_compute_addr(dwin, target_rank, target_disp);
    rc = MPIR_Copy_data(origin_adrr, origin_count, origin_datatype,
			target_addr, target_count, target_datatype);
}
/* MPID_shm_Put() */


MPID_shm_Accumulate(origin_addr, origin_count, origin_datatype,
	       target_rank, target_disp, target_count, target_datatype,
	       op, win)
{
    target_addr = MPIR_Win_compute_addr(dwin, target_rank, target_disp);

    process_mutex_lock(win.process_mutex);
    {
	/* perform requested operations */
    }
    process_mutex_unlock(win.process_mutex);
}
/* MPID_shm_Accumulate() */
