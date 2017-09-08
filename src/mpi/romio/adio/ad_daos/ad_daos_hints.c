/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */

#include "ad_daos.h"

#include "hint_fns.h"

void ADIOI_DAOS_SetInfo(ADIO_File fd, MPI_Info users_info, int *error_code)
{
    char *value;
    int flag, tmp_value;
    static char myname[] = "ADIOI_DAOS_SETINFO";

    if ((fd->info) == MPI_INFO_NULL) {
	/* part of the open call */
	MPI_Info_create(&(fd->info));
        ADIOI_Info_set(fd->info, "romio_daos_block_size", "1048576");
        ADIOI_Info_set(fd->info, "romio_daos_group_size", "4");
        ADIOI_Info_set(fd->info, "romio_daos_block_nr", "3");
        fd->hints->fs_hints.daos.block_size = 1048576;
        fd->hints->fs_hints.daos.grp_size = 4;
        fd->hints->fs_hints.daos.block_nr = 3;

	if (users_info != MPI_INFO_NULL) {
	    /* Block size in each dkey */
	    ADIOI_Info_check_and_install_int(fd, users_info, "romio_daos_block_size",
		    &(fd->hints->fs_hints.daos.block_size), myname, error_code);

	    /* Number of dkeys in each dkey group */
	    ADIOI_Info_check_and_install_int(fd, users_info, "romio_daos_group_size",
		    &(fd->hints->fs_hints.daos.grp_size), myname, error_code);

	    /* Number of Blocks to store in each dkey */
	    ADIOI_Info_check_and_install_int(fd, users_info, "romio_daos_block_nr",
		    &(fd->hints->fs_hints.daos.block_nr), myname, error_code);
	}
    }

    ADIOI_GEN_SetInfo(fd, users_info, error_code);
    *error_code = MPI_SUCCESS;
}
