/* -*- Mode: C; c-basic-offset:4 ; -*- */
/* 
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "adio.h"
#include "adio_extern.h"

int hash_copy_fn(MPI_Datatype oldtype, int type_keyval, void *extra_state, 
		void * attr_val_in, void *attr_val_out, int *flag)
{
    ((ADIOI_hashed_dtype *)attr_val_in)->ref_count +=1;
    *((ADIOI_hashed_dtype **)attr_val_out) = (ADIOI_hashed_dtype *)attr_val_in;
    *flag = 1;
    return MPI_SUCCESS;
    
}
int hash_delete_fn(MPI_Datatype type, int type_keyval, 
		void *attr_val, void *extra_state) 
{
    ADIOI_hashed_dtype *hash;
    hash = (ADIOI_hashed_dtype *)attr_val;
    hash->ref_count -= 1;
    if (hash->ref_count == 0) {
        ADIOI_Free(hash);
    }
    return MPI_SUCCESS;
}

/* this used to be implemented in every file system as an fcntl.  It makes
 * deferred open easier if we know ADIO_Fcntl will always need a file to really
 * be open. set_view doesn't modify anything related to the open files.
 */
void ADIO_Set_view(ADIO_File fd, ADIO_Offset disp, MPI_Datatype etype, 
		MPI_Datatype filetype, MPI_Info info,  int *error_code) 
{
	int combiner, i, j, k, err, filetype_is_contig;
	int key;
	ADIOI_hashed_dtype *etype_hash, *ftype_hash;
	MPI_Datatype copy_etype, copy_filetype;
	ADIOI_Flatlist_node *flat_file;
	/* free copies of old etypes and filetypes and delete flattened 
       version of filetype if necessary */

	MPI_Type_get_envelope(fd->etype, &i, &j, &k, &combiner);
	if (combiner != MPI_COMBINER_NAMED) MPI_Type_free(&(fd->etype));

	ADIOI_Datatype_iscontig(fd->filetype, &filetype_is_contig);
	if (!filetype_is_contig) ADIOI_Delete_flattened(fd->filetype);

	MPI_Type_get_envelope(fd->filetype, &i, &j, &k, &combiner);
	if (combiner != MPI_COMBINER_NAMED) MPI_Type_free(&(fd->filetype));

	/* set new info */
	ADIO_SetInfo(fd, info, &err);

        /* set new etypes and filetypes */

	MPI_Type_get_envelope(etype, &i, &j, &k, &combiner);
	if (combiner == MPI_COMBINER_NAMED) fd->etype = etype;
	else {
	    MPI_Type_contiguous(1, etype, &copy_etype);
	    MPI_Type_commit(&copy_etype);
	    fd->etype = copy_etype;
	}
	MPI_Type_get_envelope(filetype, &i, &j, &k, &combiner);
	if (combiner == MPI_COMBINER_NAMED) 
	    fd->filetype = filetype;
	else {
	    MPI_Type_contiguous(1, filetype, &copy_filetype);
	    MPI_Type_commit(&copy_filetype);
	    fd->filetype = copy_filetype;
	    ADIOI_Flatten_datatype(fd->filetype);
            /* this function will not flatten the filetype if it turns out
               to be all contiguous. */
	}

	MPI_Type_size(fd->etype, &(fd->etype_size));
	fd->disp = disp;

	etype_hash = ADIOI_Calloc(1, sizeof(ADIOI_hashed_dtype));
	etype_hash->ref_count = 1;
	ADIOI_dtype_hash(fd->etype, etype_hash);
	MPI_Type_create_keyval(hash_copy_fn, hash_delete_fn,
			&key, etype_hash);
	fd->etype_hash_key = key;
	MPI_Type_set_attr(fd->etype, key, etype_hash);

	ftype_hash = ADIOI_Calloc(1, sizeof(ADIOI_hashed_dtype));
	ftype_hash->ref_count = 1;
	ADIOI_dtype_hash(fd->filetype, ftype_hash); 
	MPI_Type_create_keyval(hash_copy_fn, hash_delete_fn,
			&key, ftype_hash);
	fd->ftype_hash_key = key;
	MPI_Type_set_attr(fd->filetype, key, ftype_hash);




        /* reset MPI-IO file pointer to point to the first byte that can
           be accessed in this view. */

        ADIOI_Datatype_iscontig(fd->filetype, &filetype_is_contig);
	if (filetype_is_contig) fd->fp_ind = disp;
	else {
	    flat_file = ADIOI_Flatlist;
	    while (flat_file->type != fd->filetype) 
		flat_file = flat_file->next;
	    for (i=0; i<flat_file->count; i++) {
		if (flat_file->blocklens[i]) {
		    fd->fp_ind = disp + flat_file->indices[i];
		    break;
		}
	    }
	}
	*error_code = MPI_SUCCESS;
}
