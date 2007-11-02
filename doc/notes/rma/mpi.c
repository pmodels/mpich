struct MPIR_Win
{
    MPI_Win				handle;
    
    MPI_Comm				comm;
    int					rank;
    int					np;
    
    void *				base;
    MPI_Aint				size;
    int					displ;

    void **				bases;
    MPI_Aint *				sizes;
    int *				displs;

    unsigned				flags;
}
/* struct MPIR_Win */


MPI_Win_create(base, size, disp_unit, info, comm, win)
{
    dwin = malloc(sizeof(MPID_Win));
    dwin->rank = MPI_Comm_rank(comm);
    dwin->np = MPI_Comm_size(comm);
    MPI_Comm_dup(comm, dwin_p.comm);

    rc = MPIR_ID_allocate(MPIR_Win_refs, dwin, &(dwin->handle));
    
    dwin->base = base;
    dwin->size = size;
    dwin->displ = disp_unit;
    
    dwin->bases = malloc(sizeof(void *) * dwin->np);
    dwin->sizes = malloc(sizeof(MPID_Win * dwin->np);
    dwin->displs = malloc(sizeof(MPID_Win) * dwin->np);
    
    rc = MPI_Allgather(dwin->base, dwin->bases, dwin->comm);
    rc = MPI_Allgather(dwin->size, dwin->sizes, dwin->comm);
    rc = MPI_Allgather(dwin->displ, dwin->displs, dwin->comm);

    dwin->flags = (MPID_WIN_CONST_BASE | MPID_WIN_CONST_SIZE |
		   MPID_WIN_CONST_DISPL);
    for(i = 0; i < dwin->np; i++)
    {
	if (dwin->bases[i] != dwin->base)
	{
	    dwin->flags &= ~MPID_WIN_CONST_BASE;
	}
	if (dwin->sizes[i] != dwin->size)
	{
	    dwin->flags &= ~MPID_WIN_CONST_SIZE;
	}
	if (dwin->displs[i] != dwin->displ)
	{
	    dwin->flags &= ~MPID_WIN_CONST_DISPL;
	}
    }
    
    rc = MPID_Win_create(dwin, info);

    *win = dwin->handle;
}
/* MPI_Win_create() */


MPI_Win_free(win)
{
    rc = MPIR_ID_lookup(win, &dwin);

    rc = MPID_Win_free(dwin);
    
    free(dwin->bases);
    free(dwin->sizes); 
    free(dwin->displs); 
    free(win);

    MPI_Comm_free(dwin->comm);

    MPIR_ID_free(win);
}
/* MPI_Win_free() */


MPI_Win_fence(assert, win)
{
    rc = MPIR_ID_lookup(win, &dwin);
    
    rc = MPID_Win_fence(assert, dwin);

    if (assert & MPI_MODE_NOSUCCEED)
    {
	dwin->flags |= MPID_WIN_EPOCH_OPEN;
    }
    else
    {
	dwin->flags &= ~MPID_WIN_EPOCH_OPEN;
    }
}
/* MPI_Win_fence() */


MPI_Get(origin_addr, origin_count, origin_datatype,
        target_rank, target_disp, target_count, target_datatype, win)
{
    rc = MPIR_ID_lookup(win, &dwin);
    
    if ((dwin->flags & MPID_WIN_EPOCH_OPEN) != MPID_WIN_EPOCH_OPEN)
    {
	rc = MPI_ERR_XXX;
    }
    
    rc = MPIR_Win_verify_dt_compat(target_count, target_datatype,
				   origin_count, origin_datatype);
    
    rc = MPIR_Win_verify_buffer(target_count, target_datatype, target_rank,
				target_disp, win);

    rc = MPID_Get(origin_addr, origin_count, origin_datatype,
		  target_rank, target_disp, target_count, target_datatype,
		  dwin);
}
/* MPI_get() */


MPI_Put(origin_addr, origin_count, origin_datatype,
        target_rank, target_disp, target_count, target_datatype, win)
{
    rc = MPIR_ID_lookup(win, &dwin);
    
    if ((dwin->flags & MPID_WIN_EPOCH_OPEN) != MPID_WIN_EPOCH_OPEN)
    {
	rc = MPI_ERR_XXX;
    }
    
    rc = MPIR_Win_verify_dt_compat(origin_count, origin_datatype,
				   target_count, target_datatype);
    
    rc = MPIR_Win_verify_buffer(target_count, target_datatype, target_rank,
				target_disp, win);

    rc = MPID_Put(origin_addr, origin_count, origin_datatype,
		  target_rank, target_disp, target_count, target_datatype,
		  dwin);
}
/* MPI_Put() */


MPI_Accumulate(origin_addr, origin_count, origin_datatype,
	       target_rank, target_disp, target_count, target_datatype,
	       op, win)
{
    rc = MPIR_ID_lookup(win, &dwin);
    
    if ((dwin->flags & MPID_WIN_EPOCH_OPEN) != MPID_WIN_EPOCH_OPEN)
    {
	rc = MPI_ERR_XXX;
    }
    
    rc = MPIR_Win_verify_dt_op_compat(origin_count, origin_datatype,
				      target_count, target_datatype, op);
    
    rc = MPIR_Win_verify_buffer(target_count, target_datatype, target_rank,
				target_disp, win);

    rc MPID_Accumulate(origin_addr, origin_count, origin_datatype,
		       target_rank, target_disp, target_count, target_datatype,
		       op, dwin);
}
/* MPI_Accumulate() */
