/*
   (C) 2001 by Argonne National Laboratory.
       See COPYRIGHT in top-level directory.
*/
#if !defined( _CLOG_SYNC )
#define _CLOG_SYNC

#define CLOG_SYNC_PREP_READY        801
#define CLOG_SYNC_MASTER_READY      802
#define CLOG_SYNC_SLAVE_READY       803
#define CLOG_SYNC_TIME_QUERY        804
#define CLOG_SYNC_TIME_ANSWER       805

#define CLOG_SYNC_FREQ_COUNT          3
#define CLOG_SYNC_AGRM_DEFAULT        0
#define CLOG_SYNC_AGRM_SEQ            1
#define CLOG_SYNC_AGRM_BITREE         2
#define CLOG_SYNC_AGRM_ALTNGBR        3

#define CLOG_SYNC_LARGEST_GAP    1.0e10

/* world_rank and world_size are values for MPI_COMM_WORLD */
typedef struct {
   int                 is_ok_to_sync;
   int                 root;
   int                 frequency;
   int                 algorithm_ID;
   int                 world_size;
   int                 world_rank;
   CLOG_Time_t         best_gpofst[2];
} CLOG_Sync_t;

CLOG_Sync_t *CLOG_Sync_create( int num_mpi_procs, int local_mpi_rank );

void CLOG_Sync_free( CLOG_Sync_t **sync_handle );

void CLOG_Sync_init( CLOG_Sync_t *sync );

CLOG_Time_t CLOG_Sync_run( CLOG_Sync_t *sync );

const char *CLOG_Sync_print_type( const CLOG_Sync_t *sync );

#endif
