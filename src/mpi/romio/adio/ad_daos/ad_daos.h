/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 * Copyright (C) 2018-2019 Intel Corporation
 *
 * GOVERNMENT LICENSE RIGHTS-OPEN SOURCE SOFTWARE
 * The Government's rights to use, modify, reproduce, release, perform, display,
 * or disclose this software are subject to the terms of the Apache License as
 * provided in Contract No. 8F-30005.
 * Any reproduction of computer software, computer software documentation, or
 * portions thereof marked with this legend must also reproduce the markings.
 */

#ifndef AD_DAOS_H_INCLUDED
#define AD_DAOS_H_INCLUDED

#include "adio.h"
#include <assert.h>
#include <gurt/list.h>
#include <daos.h>
#include <daos_fs.h>

/* #define D_PRINT_IO */
/* #define D_PRINT_IO_MEM */

#define PRINT_MSG(str, fmt, ...)                                        \
    do {                                                                \
        fprintf(str, "%s:%d %s() - " fmt"\n" ,                          \
                __FILE__, __LINE__, __func__, ##__VA_ARGS__);           \
    } while (0)

struct adio_daos_hdl {
    d_list_t entry;
    uuid_t uuid;
    daos_handle_t open_hdl;
    dfs_t *dfs;
    int ref;
    int type;
};

struct ADIO_DAOS_cont {
    /** container uuid */
    uuid_t cuuid;
    /** pool uuid */
    uuid_t puuid;
    /** Container name (Path to the file opened) */
    char *cont_name;
    /** Object name (File name) */
    char *obj_name;
    /** pool open handle */
    daos_handle_t poh;
    /** container open handle */
    daos_handle_t coh;
    /** flat namespace mount */
    dfs_t *dfs;
    /** dfs object for file */
    dfs_obj_t *obj;
    /** Array Object ID for the MPI file */
    daos_obj_id_t oid;
    /** Array OH for the MPI file */
    daos_handle_t oh;
    /** Object class of the DAOS object associated with the file */
    daos_oclass_id_t obj_class;
    /** data size to store in a dkey */
    daos_size_t chunk_size;
    /** file open mode */
    unsigned int amode;
    /** Event queue to store all async requests on file */
    daos_handle_t eqh;
    /** pool handle for directory holding the file object */
    struct adio_daos_hdl *p;
    /** container handle for directory holding the file object */
    struct adio_daos_hdl *c;
};

struct ADIO_DAOS_req {
    MPI_Request req;
    MPI_Offset nbytes;
    daos_event_t daos_event;
    daos_array_iod_t iod;
    daos_range_t rg;
    d_sg_list_t sgl;
    d_iov_t iov;
    daos_range_t *rgs;
    d_iov_t *iovs;
};

enum {
    HANDLE_POOL,
    HANDLE_CO,
    HANDLE_OBJ
};

static inline void
handle_share(daos_handle_t * hdl, int type, int rank, daos_handle_t parent, MPI_Comm comm)
{
    d_iov_t ghdl = { NULL, 0, 0 };
    int rc;

    if (rank == 0) {
        /** fetch size of global handle */
        if (type == HANDLE_POOL)
            rc = daos_pool_local2global(*hdl, &ghdl);
        else if (type == HANDLE_CO)
            rc = daos_cont_local2global(*hdl, &ghdl);
        else {
            assert(type == HANDLE_OBJ);
            rc = daos_array_local2global(*hdl, &ghdl);
        }
        assert(rc == 0);
    }

    /** broadcast size of global handle to all peers */
    rc = MPI_Bcast(&ghdl.iov_buf_len, 1, MPI_UINT64_T, 0, comm);
    assert(rc == MPI_SUCCESS);

    /** allocate buffer for global pool handle */
    ghdl.iov_buf = ADIOI_Malloc(ghdl.iov_buf_len);
    ghdl.iov_len = ghdl.iov_buf_len;

    if (rank == 0) {
        /** generate actual global handle to share with peer tasks */
        if (type == HANDLE_POOL)
            rc = daos_pool_local2global(*hdl, &ghdl);
        else if (type == HANDLE_CO)
            rc = daos_cont_local2global(*hdl, &ghdl);
        else
            rc = daos_array_local2global(*hdl, &ghdl);
        assert(rc == 0);
    }

    /** broadcast global handle to all peers */
    rc = MPI_Bcast(ghdl.iov_buf, ghdl.iov_len, MPI_BYTE, 0, comm);
    assert(rc == MPI_SUCCESS);

    if (rank != 0) {
        /** unpack global handle */
        if (type == HANDLE_POOL)
            rc = daos_pool_global2local(ghdl, hdl);
        else if (type == HANDLE_CO)
            rc = daos_cont_global2local(parent, ghdl, hdl);
        else
            rc = daos_array_global2local(parent, ghdl, 0, hdl);
        assert(rc == 0);
    }

    ADIOI_Free(ghdl.iov_buf);
    MPI_Barrier(comm);
}

/** initialize the DAOS library and hashtables for handles */
void ADIOI_DAOS_Init(int *error_code);

/** Container/Pool Handle Hash functions */
int adio_daos_hash_init(void);
void adio_daos_hash_finalize(void);
struct adio_daos_hdl *adio_daos_poh_lookup(const uuid_t uuid);
int adio_daos_poh_insert(uuid_t uuid, daos_handle_t poh, struct adio_daos_hdl **hdl);
int adio_daos_poh_lookup_connect(uuid_t uuid, struct adio_daos_hdl **hdl);
void adio_daos_poh_release(struct adio_daos_hdl *hdl);
struct adio_daos_hdl *adio_daos_coh_lookup(const uuid_t uuid);
int adio_daos_coh_insert(uuid_t uuid, daos_handle_t coh, struct adio_daos_hdl **hdl);
int adio_daos_coh_lookup_create(daos_handle_t poh, uuid_t uuid, int amode,
                                bool create, struct adio_daos_hdl **hdl);
void adio_daos_coh_release(struct adio_daos_hdl *hdl);

int ADIOI_DAOS_aio_free_fn(void *extra_state);
int ADIOI_DAOS_aio_poll_fn(void *extra_state, MPI_Status * status);
int ADIOI_DAOS_aio_wait_fn(int count, void **array_of_states, double timeout, MPI_Status * status);
int ADIOI_DAOS_err(const char *myname, const char *filename, int line, int rc);

void ADIOI_DAOS_Open(ADIO_File fd, int *error_code);
void ADIOI_DAOS_OpenColl(ADIO_File fd, int rank, int access_mode, int *error_code);
int ADIOI_DAOS_Feature(ADIO_File fd, int flag);
void ADIOI_DAOS_Flush(ADIO_File fd, int *error_code);
void ADIOI_DAOS_Resize(ADIO_File fd, ADIO_Offset size, int *error_code);
void ADIOI_DAOS_Close(ADIO_File fd, int *error_code);
void ADIOI_DAOS_Delete(const char *filename, int *error_code);
void ADIOI_DAOS_Fcntl(ADIO_File fd, int flag, ADIO_Fcntl_t * fcntl_struct, int *error_code);
void ADIOI_DAOS_SetInfo(ADIO_File fd, MPI_Info users_info, int *error_code);
void ADIOI_DAOS_ReadContig(ADIO_File fd, void *buf, int count,
                           MPI_Datatype datatype, int file_ptr_type,
                           ADIO_Offset offset, ADIO_Status * status, int *error_code);
void ADIOI_DAOS_WriteContig(ADIO_File fd, const void *buf, int count,
                            MPI_Datatype datatype, int file_ptr_type,
                            ADIO_Offset offset, ADIO_Status * status, int *error_code);
void ADIOI_DAOS_IReadContig(ADIO_File fd, void *buf, int count,
                            MPI_Datatype datatype, int file_ptr_type,
                            ADIO_Offset offset, MPI_Request * request, int *error_code);
void ADIOI_DAOS_IWriteContig(ADIO_File fd, const void *buf, int count,
                             MPI_Datatype datatype, int file_ptr_type,
                             ADIO_Offset offset, MPI_Request * request, int *error_code);
void ADIOI_DAOS_ReadStrided(ADIO_File fd, void *buf, int count,
                            MPI_Datatype datatype, int file_ptr_type,
                            ADIO_Offset offset, ADIO_Status * status, int *error_code);
void ADIOI_DAOS_WriteStrided(ADIO_File fd, const void *buf, int count,
                             MPI_Datatype datatype, int file_ptr_type,
                             ADIO_Offset offset, ADIO_Status * status, int *error_code);
void ADIOI_DAOS_IreadStrided(ADIO_File fd, void *buf, int count,
                             MPI_Datatype datatype, int file_ptr_type,
                             ADIO_Offset offset, ADIO_Request * request, int *error_code);
void ADIOI_DAOS_IwriteStrided(ADIO_File fd, const void *buf, int count,
                              MPI_Datatype datatype, int file_ptr_type,
                              ADIO_Offset offset, MPI_Request * request, int *error_code);
#endif /* AD_DAOS_H_INCLUDED */
