/* -*- Mode: C; c-basic-offset:4 ; -*- */
/* 
 *
 *   Copyright (C) 2011 UChicago/Argonne, LLC
 *   See COPYRIGHT notice in top-level directory.
 */

#include <adio.h>


/* build_answer_array:  instead of doing 4 allreduces (or 8 since we don't have
 * equality), stuff all these values into an array:
 *
 * ---------------------------------------------------------
 * |disp|reqsize|offset|ehash[0]|ehash[1]|fhash[0]|fhash[1]|
 * ---------------------------------------------------------
 */
static ADIO_Offset * build_answer_array(ADIO_Offset disp, ADIO_Offset reqsize, 
	ADIO_Offset offset, 
	ADIOI_hashed_dtype *etype_hash, ADIOI_hashed_dtype *ftype_hash, int *nitems)  
{

    ADIO_Offset *tmp_array;
    int i, idx=0;
    tmp_array = ADIOI_Calloc(1 + 1 + 1 + ADIOI_HASH_COUNT + ADIOI_HASH_COUNT, 
	    sizeof(ADIO_Offset));

    tmp_array[idx++] = disp;
    tmp_array[idx++] = reqsize;
    tmp_array[idx++] = offset;

    for (i=0; i< ADIOI_HASH_COUNT; i++) {
	tmp_array[idx++] = etype_hash->hash[i];
    }
    for (i=0; i< ADIOI_HASH_COUNT; i++) {
	tmp_array[idx++] = ftype_hash->hash[i];
    }
    *nitems=idx;

    return tmp_array;
}

int ADIOI_allsame_access(ADIO_File fd, 
		int count, MPI_Datatype datatype, ADIO_Offset offset)
{
    /* we determine equivalence of access this way:
     * - file views are identical: same type signature, same type map.  For
     *   efficency at set_view time we marshaled the type and hashed the result 
     * - same displacement when file view set
     * - amount of data requested (a memory region defined by 'buf, count,
     *   datatype' tuple) needs to be the same.  It can be laid out any which
     *   way in memory. 
     * - all processors need to access data at the same offset
     */

    /* we hash the marshaled representation but maybe it's better to compare
     * the marshaled version? */

    ADIOI_hashed_dtype *etype_hash, *ftype_hash;
    ADIO_Offset *my_result, *max_result, *min_result;
    
    int eflag, fflag, nitems, i, ret;
    int memtype_size;


    MPI_Type_get_attr(fd->etype, fd->etype_hash_key, &etype_hash, &eflag);
    if (!eflag) {
	/* unlikely unless no file view was set ?*/
	etype_hash = ADIOI_Calloc(1, sizeof(*etype_hash));
	ADIOI_dtype_hash(fd->etype, etype_hash);
    }
    
    MPI_Type_get_attr(fd->filetype, fd->ftype_hash_key, &ftype_hash, &fflag);
    if (!fflag) {
	/* unlikely unless no file view was set ?*/
	ftype_hash = ADIOI_Calloc(1, sizeof(*ftype_hash));
	ADIOI_dtype_hash(fd->etype, etype_hash);
    }

    /* does not matter what the memory type looks like; can put data anywhere
     * as long as identical amount of data read from file */
    MPI_Type_size(datatype, &memtype_size);

    /* we now have the four pieces of infomaiton we need: the disp, the hashed
     * etype and ftype, and the size of the request */
    my_result = build_answer_array(fd->disp, memtype_size*count, offset, ftype_hash,
	    etype_hash, &nitems);
    min_result = ADIOI_Calloc(nitems, sizeof(ADIO_Offset));
    max_result = ADIOI_Calloc(nitems, sizeof(ADIO_Offset));

    /* reduction of integers can be pretty fast on some platforms */
    MPI_Allreduce(my_result, min_result, nitems, ADIO_OFFSET, MPI_MIN, fd->comm);
    MPI_Allreduce(my_result, max_result, nitems, ADIO_OFFSET, MPI_MAX, fd->comm);

    ret = 1;
    for (i=0; i<nitems; i++) {
	if (min_result[i] != max_result[i]) {
	    ret = 0;
	    break;
	}
    }

fn_exit:
    if (!eflag) ADIOI_Free(etype_hash);
    if (!fflag) ADIOI_Free(ftype_hash);
    ADIOI_Free(my_result);
    ADIOI_Free(min_result);
    ADIOI_Free(max_result);

    return ret;
}
/* 
 * vim: ts=8 sts=4 sw=4 noexpandtab 
 */
