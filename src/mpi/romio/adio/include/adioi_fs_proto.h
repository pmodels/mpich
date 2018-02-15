/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *   Copyright (C) 1997 University of Chicago.
 *   See COPYRIGHT notice in top-level directory.
 */


#ifndef ADIOI_FS_PROTO_H_INCLUDED
#define ADIOI_FS_PROTO_H_INCLUDED

#include <assert.h>
#ifdef ROMIO_NFS
extern struct ADIOI_Fns_struct ADIO_NFS_operations;
/* prototypes are in adio/ad_nfs/ad_nfs.h */
#endif

#ifdef ROMIO_PANFS
extern struct ADIOI_Fns_struct ADIO_PANFS_operations;
/* prototypes are in adio/ad_panfs/ad_panfs.h */
#endif

#ifdef ROMIO_UFS
extern struct ADIOI_Fns_struct ADIO_UFS_operations;
/* prototypes are in adio/ad_ufs/ad_ufs.h */
#endif

#ifdef ROMIO_XFS
extern struct ADIOI_Fns_struct ADIO_XFS_operations;
/* prototypes are in adio/ad_xfs/ad_xfs.h */
#endif

#ifdef ROMIO_LUSTRE
extern struct ADIOI_Fns_struct ADIO_LUSTRE_operations;
/* prototypes are in adio/ad_lustre/ad_lustre.h */
#endif

#ifdef ROMIO_PVFS2
extern struct ADIOI_Fns_struct ADIO_PVFS2_operations;
/* prototypes are in adio/ad_pvfs2/ad_pvfs2.h */
#endif

#ifdef ROMIO_TESTFS
extern struct ADIOI_Fns_struct ADIO_TESTFS_operations;
/* prototypes are in adio/ad_testfs/ad_testfs.h */
#endif

#ifdef ROMIO_GPFS
extern struct ADIOI_Fns_struct ADIO_GPFS_operations;
/* prototypes are in adio/ad_gpfs/ad_gpfs.h */
#endif

#ifdef ROMIO_IME
/* prototypes are in adio/ad_im/ad_im.h */
extern struct ADIOI_Fns_struct ADIO_IME_operations;
#endif

#ifdef ROMIO_DAOS
#include <daos_api.h>
/* prototypes are in adio/ad_daos/ad_daos.h */
extern struct ADIOI_Fns_struct ADIO_DAOS_operations;

enum {
	HANDLE_POOL,
	HANDLE_CO
};

static inline void
handle_share(daos_handle_t *hdl, int type, int rank, daos_handle_t poh,
             MPI_Comm comm)
{
	daos_iov_t	ghdl = { NULL, 0, 0 };
	int		rc;

	if (rank == 0) {
		/** fetch size of global handle */
		if (type == HANDLE_POOL)
			rc = daos_pool_local2global(*hdl, &ghdl);
		else
			rc = daos_cont_local2global(*hdl, &ghdl);
		assert(rc == 0);
	}

	/** broadcast size of global handle to all peers */
	rc = MPI_Bcast(&ghdl.iov_buf_len, 1, MPI_UINT64_T, 0, comm);
	assert(rc == MPI_SUCCESS);

	/** allocate buffer for global pool handle */
	ghdl.iov_buf = malloc(ghdl.iov_buf_len);
	ghdl.iov_len = ghdl.iov_buf_len;

	if (rank == 0) {
		/** generate actual global handle to share with peer tasks */
		if (type == HANDLE_POOL)
			rc = daos_pool_local2global(*hdl, &ghdl);
		else
			rc = daos_cont_local2global(*hdl, &ghdl);
		assert(rc == 0);
	}

	/** broadcast global handle to all peers */
	rc = MPI_Bcast(ghdl.iov_buf, ghdl.iov_len, MPI_BYTE, 0, comm);
	assert(rc == MPI_SUCCESS);

	if (rank != 0) {
		/** unpack global handle */
		if (type == HANDLE_POOL) {
			/* NB: Only pool_global2local are different */
			rc = daos_pool_global2local(ghdl, hdl);
		} else {
			rc = daos_cont_global2local(poh, ghdl, hdl);
		}
		assert(rc == 0);
	}

	free(ghdl.iov_buf);
	MPI_Barrier(comm);
}
#endif
#endif /* ADIOI_FS_PROTO_H_INCLUDED */
