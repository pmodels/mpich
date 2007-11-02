MPIR_Copy_data(src_addr, src_count, src_datatype,
	       dest_buff_addr, dest_count, dest_datatype)
{
    if (MPIR_Datatype_iscontig(src_count, src_datatype) &&
	MPIR_Datatype_iscontig(dest_count, dest_datatype))
    {
	/*
	 * If the data arrangement is contiguous at both the src and dest
	 * buffers, then we can simply copy the raw bytes from the src buffer
	 * to the dest buffer.  Remember the type signatures have to match, so
	 * this is a safe operation.
	 *
	 * NOTE: This code assumes that the (dest_count, dest_datatype)
	 * represents enough space to accomodate (src_count, src_datatype) and
	 * that enough space exists in the dest buffer to cover (dest_count,
	 * dest_datatype).  This implies that truncation and buffer overruns
	 * errors must be caught before this routine is called.
	 *
	 * Q: Does a datatype be denoted as "contiguous" imply that the extent
	 * and the true extent (size) are the same?  In other words, can a
	 * contiguous datatype have padding?  If it can, then we can't use a
	 * straight memory copy because we will be copying unwanted bytes.
	 *
	 * For the simple case where neither the src nor the dest datatypes
	 * contain any padding a simple memory copy with work.  If padding
	 * exists in either datatype, we need a loop and extra logic to ensure
	 * that we don't touch the padded areas. This is true even if they are
	 * the two datatypes are the same.  We simply don't know if the padding
	 * represents a skip over unused memory resulting from alignment
	 * problems, or if it is being used to skip over locations containing
	 * data we don't need for this operation.  If the latter is true, then
	 * a straight memory copy would wipe out live data.
	 */
    }
    else if (MPIR_Datatype_iscontig(src_count, src_datatype))
    {
        /*
	 * The data arrangment at the src buffer is contiguous, but not at the
	 * dest.  We should be able to "unpack" the data directly into the dest
	 * buffer.
	 */
    }
    else if (MPIR_Datatype_iscontig(dest_count, dest_datatype))
    {
        /*
	 * The data arrangment at the dest buffer is contiguous, but not at the
	 * src.  We should be able to "pack" the data directly into the dest
	 * buffer.
	 */
    }
    else
    {
	/*
	 *  The data arrangement at both the src and dest buffers is
	 *  non-contiguous.  Theoretically, we should be able to manage the two
	 *  dataloops and copy the data without an intermediate buffer.  I need
	 *  to talk to Rob/Bill about this.
	 */
    }

    rc = MPI_SUCCESS;
}
/* MPIR_Copy_data() */


MPIR_Win_verify_dt_compat(src_count, src_datatype, dest_count, dest_datatype)
{
    /*
     * Verify that the datatypes are compatible for the copy from the src
     * buffer to the dest buffer.  This includes checking for truncation.
     */
    if (src_datatype != dest_datatype &&
	src_datatype != MPI_PACKED && src_datatype != MPI_PACKED)
    {
	/*
	 * If the datatypes are not the same basic type and neither is
	 * MPI_PACKED, so they are incompatible.
	 */
	
	rc = MPI_ERR_XXX;
    }

    /*
     * Since one of the datatypes can be MPI_PACKED, we have to compare the
     * byte counts in order to detect truncation.
     */
    if (dest_count * dest_datatype.size < src_count * src_datatype.size)
    {
	/* 
	 * NOTE: The standard does not define what the application should
	 * expect when truncation occurs.  We could copy as much data as
	 * possible before rc =ing an error, but that would only delay
	 * reporting the error to the application.  Besides, a portable
	 * application cannot rely on any of the data being placed into dest
	 * buffer, so it seems pointless to perform a partial copy.
	 */
	rc = MPI_ERR_TRUNCATE;
    }

    rc = MPI_SUCCESS;
}


MPIR_Win_verify_dt_op_compat(src_count, src_datatype,
			     dest_count, dest_datatype, op)
{
    /*
     * The standard requires that datatypes contain one and only one basic type
     * and that this basic type be the same for both datatypes.  See section
     * 6.3.4, page 120, line 6 for details.
     *
     * NOTE: Since we are presently assuming all datatypes are basic, this
     * check is easy.
     */
    if (src_datatype != dest_datatype)
    {
	rc = MPI_ERR_XXX;
    }

    /*
     * Verify that the datatypes are compatible with the specified operation.
     * See section 4.9.2, "Predefined reduce operations", of the MPI-1.1
     * standard for details.
     */
    if (MPIR_Op_dt_compat(op, src_datatype))
    {
	rc = MPI_ERR_XXX;
    }

    /*
     * Verify that truncation will not occur
     */
    if (dest_count < src_count)
    {
	/* 
	 * NOTE: The standard does not define what the application should
	 * expect when truncation occurs.  We could peform the operation on as
	 * much data as possible before rc =ing an error, but that would only
	 * delay reporting the error to the application.  Besides, a portable
	 * application cannot rely on any of the operations being completed, so
	 * it seems pointless to perform a partial accumulate.
	 */
	rc = MPI_ERR_TRUNCATE;
    }

    rc = MPI_SUCCESS;
}


MPIR_Win_verify_buffer(count, datatype, rank, disp, dwin)
{
    win_size = ((dwin->flags & MPID_WIN_CONST_SIZE)
		? dwin->size : dwin->sizes[target_size]);
    win_displ = ((dwin->flags & MPID_WIN_CONST_DISPL)
		 ? dwin->displs : dwin->displs[target_size]);
    
    /*
     * Verify that the target buffer is contained within the target's local
     * window.
     */
    byte_count = count * datatype.size;
    offset = win_displ * disp;
    
    if (offset >= win_size)
    {
	rc = MPI_ERR_DISP;
    }

    if (offset + byte_count > win_size)
    {
	rc = MPI_ERR_XXX;
    }

    rc = MPI_SUCCESS;
}
    

MPIR_Win_compute_addr(dwin, rank, disp)
{
    win_base = ((dwin->flags & MPID_WIN_CONST_BASE)
		? dwin->base : dwin->sizes[target_base]);
    win_displ = ((dwin->flags & MPID_WIN_CONST_DISPL)
		 ? dwin->displs : dwin->displs[target_size]);

    return (win_base + win_displ * disp);
}
