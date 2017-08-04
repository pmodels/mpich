/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */

#ifndef AD_DAOS_INCLUDE
#define AD_DAOS_INCLUDE

#include <daos_types.h>
#include <daos_api.h>
#include <daos_addons.h>
#include <daos_event.h>

#include "adio.h"

/* MSC - Generalized requests extensions just to be able to compile.. can't
 * support async now since ompi doesn't have those extensions */
typedef int MPIX_Grequest_class;
typedef int (MPIX_Grequest_poll_function)(void *, MPI_Status *);
typedef int (MPIX_Grequest_wait_function)(int, void **, double, MPI_Status *);
int PMPIX_Grequest_class_create(MPI_Grequest_query_function *query_fn,
                                MPI_Grequest_free_function *free_fn,
                                MPI_Grequest_cancel_function *cancel_fn,
                                MPIX_Grequest_poll_function *poll_fn,
                                MPIX_Grequest_wait_function *wait_fn,
                                MPIX_Grequest_class *greq_class);
int PMPIX_Grequest_class_allocate(MPIX_Grequest_class greq_class, 
                                  void *extra_state, MPI_Request *request);

struct ADIO_DAOS_cont {
    /** container uuid */
    uuid_t		uuid;
    /** container daos OH */
    daos_handle_t	coh;
    /** Array Object ID for the MPI file */
    daos_obj_id_t	oid;
    /** Array OH for the MPI file */
    daos_handle_t	oh;
    /** data to store in a dkey block */
    daos_size_t		stripe_size;
    /** file open mode */
    unsigned int	amode;
    /** Event queue to store all async requests on file */
    daos_handle_t	eqh;
    /** epoch currently the handle is at */
    daos_epoch_t	epoch;
};

int ADIOI_DAOS_error_convert(int error);

void ADIOI_DAOS_Open(ADIO_File fd, int *error_code);
void ADIOI_DAOS_OpenColl(ADIO_File fd, int rank,
                         int access_mode, int *error_code);
int ADIOI_DAOS_Feature(ADIO_File fd, int flag);
void ADIOI_DAOS_Resize(ADIO_File fd, ADIO_Offset size, int *error_code);
void ADIOI_DAOS_Close(ADIO_File fd, int *error_code);
void ADIOI_DAOS_Delete(const char *filename, int *error_code);
void ADIOI_DAOS_Fcntl(ADIO_File fd, int flag, ADIO_Fcntl_t *fcntl_struct,
                      int *error_code);
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
#endif
