/*
   (C) 2001 by Argonne National Laboratory.
       See COPYRIGHT in top-level directory.
*/
#include "mpe_collchk_conf.h"

#if defined( STDC_HEADERS ) || defined( HAVE_UNISTD_H )
#include <unistd.h>
#endif
#if defined( STDC_HEADERS ) || defined( HAVE_STRING_H )
#include <string.h>
#endif
#if defined( STDC_HEADERS ) || defined( HAVE_STDLIB_H )
#include <stdlib.h>
#endif
#if defined( STDC_HEADERS ) || defined( HAVE_STDIO_H )
#include <stdio.h>
#endif

#include "mpi.h"

#if ! defined( HAVE_MPI_ERR_FNS )
int MPI_Add_error_class(int *errorclass);
int MPI_Add_error_code(int errorclass, int *errorcode);
int MPI_Add_error_string(int errorcode, char *string);
int MPI_Comm_call_errhandler(MPI_Comm comm, int errorcode);
#endif

#if defined( HAVE_MPI_IO )
/* file handlers */
typedef struct {
        MPI_File fh;
        MPI_Comm comm;
} CollChk_fh_t;
#endif

#if defined( HAVE_MPI_RMA )
/* windows */
typedef struct {
        MPI_Win win;
        MPI_Comm comm;
} CollChk_win_t;
#endif

/* the hash struct */
typedef struct {
        unsigned int value; 
        unsigned int count;
} CollChk_hash_t;

/* Global variables -- start */
#if defined( HAVE_MPI_IO )
extern CollChk_fh_t *CollChk_fh_list;

extern int CollChk_fh_cnt;
#endif

#if defined( HAVE_MPI_RMA )
extern CollChk_win_t *CollChk_win_list;

extern int CollChk_win_cnt;
#endif

/* begin string */
extern char CollChk_begin_str[128];

extern int     COLLCHK_CALLED_BEGIN;
extern int     COLLCHK_ERRORS;
extern int     COLLCHK_ERR_NOT_INIT;
extern int     COLLCHK_ERR_ROOT ;
extern int     COLLCHK_ERR_CALL;
extern int     COLLCHK_ERR_OP;
extern int     COLLCHK_ERR_INPLACE;
extern int     COLLCHK_ERR_DTYPE;
extern int     COLLCHK_ERR_HIGH_LOW;
extern int     COLLCHK_ERR_LL;
extern int     COLLCHK_ERR_TAG;
extern int     COLLCHK_ERR_DIMS;
extern int     COLLCHK_ERR_GRAPH;
extern int     COLLCHK_ERR_AMODE;
extern int     COLLCHK_ERR_WHENCE;
extern int     COLLCHK_ERR_DATAREP;
extern int     COLLCHK_ERR_PREVIOUS_BEGIN;
extern int     COLLCHK_ERR_FILE_NOT_OPEN;
/* Global variables -- End */


#if defined( HAVE_MPI_IO )
void CollChk_add_fh( MPI_File fh, MPI_Comm comm );
int CollChk_get_fh(MPI_File fh, MPI_Comm *comm);
#endif
#if defined( HAVE_MPI_RMA )
void CollChk_add_win( MPI_Win win, MPI_Comm comm );
int CollChk_get_win(MPI_Win win, MPI_Comm *comm);
#endif
void CollChk_set_begin(char* in);
void CollChk_unset_begin(void);
int CollChk_check_buff(MPI_Comm comm, void * buff, char* call);
int CollChk_check_dims(MPI_Comm comm, int ndims, int *dims, char* call);
int CollChk_check_graph(MPI_Comm comm, int nnodes, int *index, int* edges,
                        char* call);
int CollChk_check_size(MPI_Comm comm, int size, char* call);
int CollChk_err_han(char * err_str, int err_code, char * call, MPI_Comm comm);
int CollChk_is_init(void);
int CollChk_same_amode(MPI_Comm comm, int amode, char* call);
int CollChk_same_call(MPI_Comm comm, char* call);
int CollChk_same_datarep(MPI_Comm comm, char* datarep, char *call);

int CollChk_hash_equal(const CollChk_hash_t *alpha,
                       const CollChk_hash_t *beta);
void CollChk_dtype_hash(MPI_Datatype type, int cnt, CollChk_hash_t *dt_hash);
                          
int CollChk_dtype_bcast(MPI_Comm comm, MPI_Datatype type, int cnt, int root,
                        char* call);
int CollChk_dtype_scatter(MPI_Comm comm,
                          MPI_Datatype sendtype, int sendcnt,
                          MPI_Datatype recvtype, int recvcnt,
                          int root, int are2buffs, char *call);
int CollChk_dtype_scatterv(MPI_Comm comm,
                           MPI_Datatype sendtype, int *sendcnts,
                           MPI_Datatype recvtype, int recvcnt,
                           int root, int are2buffs, char *call);
int CollChk_dtype_allgather(MPI_Comm comm,
                            MPI_Datatype sendtype, int sendcnt,
                            MPI_Datatype recvtype, int recvcnt,
                            int are2buffs, char *call);
int CollChk_dtype_allgatherv(MPI_Comm comm,
                             MPI_Datatype sendtype, int sendcnt,
                             MPI_Datatype recvtype, int *recvcnts,
                             int are2buffs, char *call);
int CollChk_dtype_alltoallv(MPI_Comm comm,
                            MPI_Datatype sendtype, int *sendcnts,
                            MPI_Datatype recvtype, int *recvcnts,
                            char *call);
int CollChk_dtype_alltoallw(MPI_Comm comm,
                            MPI_Datatype *sendtypes, int *sendcnts,
                            MPI_Datatype *recvtypes, int *recvcnts,
                            char *call);

int CollChk_same_high_low(MPI_Comm comm, int high_low, char* call);
int CollChk_same_int(MPI_Comm comm, int val, char* call, char* check,
                     char* err_str);
int CollChk_same_local_leader(MPI_Comm comm, int ll, char* call);
int CollChk_same_op(MPI_Comm comm, MPI_Op op, char* call);
int CollChk_same_root(MPI_Comm comm, int root, char* call);
int CollChk_same_tag(MPI_Comm comm, int tag, char* call);
int CollChk_same_whence(MPI_Comm comm, int whence, char* call);


#define COLLCHK_NO_ERROR_STR   "no error"
#define COLLCHK_SM_STRLEN      32
#define COLLCHK_STD_STRLEN     256
#define COLLCHK_LG_STRLEN      1024
