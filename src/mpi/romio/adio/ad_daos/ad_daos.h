/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */

#ifndef AD_DAOS_INCLUDE
#define AD_DAOS_INCLUDE

#include <daos_types.h>
#include <daos_api.h>
#include <daos_addons.h>
#include <daos_event.h>

#include "adio.h"

#define PRINT_MSG(str, fmt, ...)                                            \
    do {                                                                \
        fprintf(str, "%s:%d %s() - " fmt"\n" ,                          \
                __FILE__, __LINE__, __func__, ##__VA_ARGS__);                         \
    } while (0)

struct ADIO_DAOS_cont {
    /** container uuid */
    uuid_t		uuid;
    /** Container name (Path to the file opened) */
    char		*cont_name;
    /** Object name (File name) */
    char		*obj_name;
    /** container daos OH */
    daos_handle_t	coh;
    /** Array Object ID for the MPI file */
    daos_obj_id_t	oid;
    /** Array OH for the MPI file */
    daos_handle_t	oh;
    /** data to store in a dkey block */
    daos_size_t		block_size;
    /** file open mode */
    unsigned int	amode;
    /** Event queue to store all async requests on file */
    daos_handle_t	eqh;
    /** epoch currently the handle is at */
    daos_epoch_t	epoch;
};

struct ADIO_DAOS_req {
    MPI_Request req;
    MPI_Offset nbytes;
    daos_event_t daos_event;
    daos_array_ranges_t ranges;
    daos_range_t rg;
    daos_sg_list_t sgl;
    daos_iov_t iov;
    daos_range_t *rgs;
    daos_iov_t *iovs;
};

int ADIOI_DAOS_aio_free_fn(void *extra_state);
int ADIOI_DAOS_aio_poll_fn(void *extra_state, MPI_Status *status);
int ADIOI_DAOS_aio_wait_fn(int count, void ** array_of_states,
                           double timeout, MPI_Status *status);
int ADIOI_DAOS_error_convert(int error);

void ADIOI_DAOS_Open(ADIO_File fd, int *error_code);
void ADIOI_DAOS_OpenColl(ADIO_File fd, int rank,
                         int access_mode, int *error_code);
int ADIOI_DAOS_Feature(ADIO_File fd, int flag);
void ADIOI_DAOS_Flush(ADIO_File fd, int *error_code);
void ADIOI_DAOS_Resize(ADIO_File fd, ADIO_Offset size, int *error_code);
void ADIOI_DAOS_Close(ADIO_File fd, int *error_code);
void ADIOI_DAOS_Delete(const char *filename, int *error_code);
void ADIOI_DAOS_Fcntl(ADIO_File fd, int flag, ADIO_Fcntl_t *fcntl_struct,
                      int *error_code);
void ADIOI_DAOS_SetInfo(ADIO_File fd, MPI_Info users_info, int *error_code);
void ADIOI_DAOS_ReadContig(ADIO_File fd, void *buf, int count,
			   MPI_Datatype datatype, int file_ptr_type,
			   ADIO_Offset offset, ADIO_Status *status,
			   int *error_code);
void ADIOI_DAOS_WriteContig(ADIO_File fd, const void *buf, int count,
			    MPI_Datatype datatype, int file_ptr_type,
			    ADIO_Offset offset, ADIO_Status *status,
			    int *error_code);
void ADIOI_DAOS_IReadContig(ADIO_File fd, void *buf, int count,
			    MPI_Datatype datatype, int file_ptr_type,
			    ADIO_Offset offset, MPI_Request *request,
			    int *error_code);
void ADIOI_DAOS_IWriteContig(ADIO_File fd, const void *buf, int count,
                             MPI_Datatype datatype, int file_ptr_type,
                             ADIO_Offset offset, MPI_Request *request,
                             int *error_code);
void ADIOI_DAOS_ReadStrided(ADIO_File fd, void *buf, int count,
                            MPI_Datatype datatype, int file_ptr_type,
                            ADIO_Offset offset, ADIO_Status *status,
                            int *error_code);
void ADIOI_DAOS_WriteStrided(ADIO_File fd, const void *buf, int count,
                             MPI_Datatype datatype, int file_ptr_type,
                             ADIO_Offset offset, ADIO_Status *status,
                             int *error_code);
void ADIOI_DAOS_IreadStrided(ADIO_File fd, void *buf, int count, 
                             MPI_Datatype datatype, int file_ptr_type,
                             ADIO_Offset offset, ADIO_Request *request,
                             int *error_code);
void ADIOI_DAOS_IwriteStrided(ADIO_File fd, const void *buf, int count,
                              MPI_Datatype datatype, int file_ptr_type,
                              ADIO_Offset offset, MPI_Request *request,
                              int *error_code);
#endif
