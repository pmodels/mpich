/*
   (C) 2001 by Argonne National Laboratory.
       See COPYRIGHT in top-level directory.
*/
/*
   This file should be INCLUDED into log_mpi_core.c when adding the
   MPI2 communicator routines to the profiling list

   Also set MPE_MAX_KNOWN_STATES >= 200
*/
#define MPE_COMM_SPAWN_ID  201
#define MPE_COMM_SPAWN_MULTIPLE_ID 202
#define MPE_COMM_GET_PARENT_ID 203
#define MPE_COMM_ACCEPT_ID 204
#define MPE_COMM_CONNECT_ID 205
#define MPE_COMM_DISCONNECT_ID 206
#define MPE_COMM_JOIN_ID 207
#define MPE_COMM_SET_NAME_ID 208
#define MPE_COMM_GET_NAME_ID 209
#define MPE_OPEN_PORT_ID 210
#define MPE_CLOSE_PORT_ID 211
#define MPE_LOOKUP_NAME_ID 212
#define MPE_PUBLISH_NAME_ID 213
#define MPE_UNPUBLISH_NAME_ID 214

void MPE_Init_mpi_spawn( void )
{
    MPE_State *state;

    state = &states[MPE_COMM_SPAWN_ID];
    state->kind_mask = MPE_KIND_SPAWN;
    state->name = "MPI_Comm_spawn";
    state->color = "DarkSeaGreen1";

    state = &states[MPE_COMM_SPAWN_MULTIPLE_ID];
    state->kind_mask = MPE_KIND_SPAWN;
    state->name = "MPI_Comm_spawn_multiple";
    state->color = "DarkSeaGreen2";

    state = &states[MPE_COMM_GET_PARENT_ID];
    state->kind_mask = MPE_KIND_SPAWN;
    state->name = "MPI_Comm_get_parent";
    state->color = "ForestGreen";

    state = &states[MPE_COMM_ACCEPT_ID];
    state->kind_mask = MPE_KIND_SPAWN;
    state->name = "MPI_Comm_accept";
    state->color = "YellowGreen";

    state = &states[MPE_COMM_CONNECT_ID];
    state->kind_mask = MPE_KIND_SPAWN;
    state->name = "MPI_Comm_connect";
    state->color = "LawnGreen";

    state = &states[MPE_COMM_DISCONNECT_ID];
    state->kind_mask = MPE_KIND_SPAWN;
    state->name = "MPI_Comm_disconnect";
    state->color = "MediumSpringGreen";

    state = &states[MPE_COMM_JOIN_ID];
    state->kind_mask = MPE_KIND_SPAWN;
    state->name = "MPI_Comm_join";
    state->color = "DarkSeaGreen3";

    state = &states[MPE_COMM_SET_NAME_ID];
    state->kind_mask = MPE_KIND_SPAWN;
    state->name = "MPI_Comm_set_name";
    state->color = "purple";

    state = &states[MPE_COMM_GET_NAME_ID];
    state->kind_mask = MPE_KIND_SPAWN;
    state->name = "MPI_Comm_get_name";
    state->color = "purple";

    state = &states[MPE_OPEN_PORT_ID];
    state->kind_mask = MPE_KIND_SPAWN;
    state->name = "MPI_Open_port";
    state->color = "purple";

    state = &states[MPE_CLOSE_PORT_ID];
    state->kind_mask = MPE_KIND_SPAWN;
    state->name = "MPI_Close_port";
    state->color = "purple";

    state = &states[MPE_LOOKUP_NAME_ID];
    state->kind_mask = MPE_KIND_SPAWN;
    state->name = "MPI_Lookup_name";
    state->color = "purple";

    state = &states[MPE_PUBLISH_NAME_ID];
    state->kind_mask = MPE_KIND_SPAWN;
    state->name = "MPI_Publish_name";
    state->color = "purple";

    state = &states[MPE_UNPUBLISH_NAME_ID];
    state->kind_mask = MPE_KIND_SPAWN;
    state->name = "MPI_Unpublish_name";
    state->color = "purple";
}

int MPI_Comm_spawn( char *command, char *argv[], int maxprocs,
                    MPI_Info info, int root, MPI_Comm comm,
                    MPI_Comm *intercomm, int array_of_errcodes[] )
{
    int   returnVal;
    MPE_LOG_STATE_DECL
    MPE_LOG_COMM_DECL
    MPE_LOG_THREADSTM_DECL

    MPE_LOG_THREADSTM_GET
    MPE_LOG_THREAD_LOCK
    MPE_LOG_STATE_BEGIN(comm,MPE_COMM_SPAWN_ID)
    MPE_LOG_THREAD_UNLOCK

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_OFF
#endif

    returnVal = PMPI_Comm_spawn( command, argv, maxprocs, info, root,
                                 comm, intercomm, array_of_errcodes );

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_ON
#endif

    MPE_LOG_THREAD_LOCK
    MPE_LOG_INTERCOMM(comm,*intercomm,CLOG_COMM_INTER_CREATE)

    MPE_LOG_STATE_END(comm,NULL)
    MPE_LOG_THREAD_UNLOCK

    return returnVal;
}

int MPI_Comm_spawn_multiple( int count, char *array_of_commands[],
                             char* *array_of_argv[], int array_of_maxprocs[],
                             MPI_Info array_of_info[], int root, MPI_Comm comm,
                             MPI_Comm *intercomm, int array_of_errcodes[] )
{
    int   returnVal;
    MPE_LOG_STATE_DECL
    MPE_LOG_COMM_DECL
    MPE_LOG_THREADSTM_DECL

    MPE_LOG_THREADSTM_GET
    MPE_LOG_THREAD_LOCK
    MPE_LOG_STATE_BEGIN(comm,MPE_COMM_SPAWN_MULTIPLE_ID)
    MPE_LOG_THREAD_UNLOCK

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_OFF
#endif

    returnVal = PMPI_Comm_spawn_multiple( count, array_of_commands,
                                          array_of_argv, array_of_maxprocs,
                                          array_of_info, root,
                                          comm, intercomm, array_of_errcodes );

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_ON
#endif

    MPE_LOG_THREAD_LOCK
    MPE_LOG_INTERCOMM(comm,*intercomm,CLOG_COMM_INTER_CREATE)

    MPE_LOG_STATE_END(comm,NULL)
    MPE_LOG_THREAD_UNLOCK

    return returnVal;
}

int MPI_Comm_get_parent( MPI_Comm *parent )
{
    int   returnVal;
    MPE_LOG_STATE_DECL
    MPE_LOG_COMM_DECL
    MPE_LOG_THREADSTM_DECL

    MPE_LOG_THREADSTM_GET
    MPE_LOG_THREAD_LOCK
    MPE_LOG_STATE_BEGIN(MPE_COMM_NULL,MPE_COMM_GET_PARENT_ID)
    MPE_LOG_THREAD_UNLOCK

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_OFF
#endif

    returnVal = PMPI_Comm_get_parent( parent );

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_ON
#endif

    MPE_LOG_THREAD_LOCK
    MPE_LOG_INTERCOMM(MPE_COMM_NULL,*parent,CLOG_COMM_INTER_CREATE)

    MPE_LOG_STATE_END(MPE_COMM_NULL,NULL)
    MPE_LOG_THREAD_UNLOCK

    return returnVal;
}

int MPI_Comm_accept( char *port_name, MPI_Info info, int root,
                     MPI_Comm comm, MPI_Comm *newcomm )
{
    int   returnVal;
    MPE_LOG_STATE_DECL
    MPE_LOG_COMM_DECL
    MPE_LOG_THREADSTM_DECL

    MPE_LOG_THREADSTM_GET
    MPE_LOG_THREAD_LOCK
    MPE_LOG_STATE_BEGIN(comm,MPE_COMM_ACCEPT_ID)
    MPE_LOG_THREAD_UNLOCK

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_OFF
#endif

    returnVal = PMPI_Comm_accept( port_name, info, root, comm, newcomm );

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_ON
#endif

    MPE_LOG_THREAD_LOCK
    MPE_LOG_INTERCOMM(comm,*newcomm,CLOG_COMM_INTER_CREATE)

    MPE_LOG_STATE_END(comm,NULL)
    MPE_LOG_THREAD_UNLOCK

    return returnVal;
}

int MPI_Comm_connect( char *port_name, MPI_Info info, int root,
                      MPI_Comm comm, MPI_Comm *newcomm )
{
    int   returnVal;
    MPE_LOG_STATE_DECL
    MPE_LOG_COMM_DECL
    MPE_LOG_THREADSTM_DECL

    MPE_LOG_THREADSTM_GET
    MPE_LOG_THREAD_LOCK
    MPE_LOG_STATE_BEGIN(comm,MPE_COMM_CONNECT_ID)
    MPE_LOG_THREAD_UNLOCK

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_OFF
#endif

    returnVal = PMPI_Comm_connect( port_name, info, root, comm, newcomm );

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_ON
#endif

    MPE_LOG_THREAD_LOCK
    MPE_LOG_INTERCOMM(comm,*newcomm,CLOG_COMM_INTER_CREATE)

    MPE_LOG_STATE_END(comm,NULL)
    MPE_LOG_THREAD_UNLOCK

    return returnVal;
}

int MPI_Comm_disconnect( MPI_Comm * comm )
{
    int   returnVal;
    MPE_LOG_STATE_DECL
    MPE_LOG_COMM_DECL
    MPE_LOG_THREADSTM_DECL

    MPE_LOG_THREADSTM_GET
    MPE_LOG_THREAD_LOCK
    MPE_LOG_STATE_BEGIN(*comm,MPE_COMM_DISCONNECT_ID)
    MPE_LOG_THREAD_UNLOCK

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_OFF
#endif

    returnVal = PMPI_Comm_disconnect( comm );

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_ON
#endif

    MPE_LOG_THREAD_LOCK
    if ( *comm == MPI_COMM_NULL ) {
        MPE_LOG_INTERCOMM(*comm,MPI_COMM_NULL,CLOG_COMM_FREE)
    }

    MPE_LOG_STATE_END(*comm,NULL)
    MPE_LOG_THREAD_UNLOCK

    return returnVal;
}

int MPI_Comm_join( int fd, MPI_Comm *intercomm )
{
    int   returnVal;
    MPE_LOG_STATE_DECL
    MPE_LOG_COMM_DECL
    MPE_LOG_THREADSTM_DECL

    MPE_LOG_THREADSTM_GET
    MPE_LOG_THREAD_LOCK
    MPE_LOG_STATE_BEGIN(MPE_COMM_NULL,MPE_COMM_JOIN_ID)
    MPE_LOG_THREAD_UNLOCK

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_OFF
#endif

    returnVal = PMPI_Comm_join( fd, intercomm );

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_ON
#endif

    MPE_LOG_THREAD_LOCK
    MPE_LOG_INTERCOMM(MPE_COMM_NULL,*intercomm,CLOG_COMM_INTER_CREATE)

    MPE_LOG_STATE_END(MPE_COMM_NULL,NULL)
    MPE_LOG_THREAD_UNLOCK

    return returnVal;
}

int MPI_Comm_set_name( MPI_Comm comm, char *comm_name )
{
    int   returnVal;
    MPE_LOG_STATE_DECL
    MPE_LOG_THREADSTM_DECL

    MPE_LOG_THREADSTM_GET
    MPE_LOG_THREAD_LOCK
    MPE_LOG_STATE_BEGIN(comm,MPE_COMM_SET_NAME_ID)
    MPE_LOG_THREAD_UNLOCK

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_OFF
#endif

    returnVal = PMPI_Comm_set_name( comm, comm_name );

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_ON
#endif

    MPE_LOG_THREAD_LOCK
    MPE_LOG_STATE_END(comm,NULL)
    MPE_LOG_THREAD_UNLOCK

    return returnVal;
}

int MPI_Comm_get_name( MPI_Comm comm, char *comm_name, int *resultlen )
{
    int   returnVal;
    MPE_LOG_STATE_DECL
    MPE_LOG_THREADSTM_DECL

    MPE_LOG_THREADSTM_GET
    MPE_LOG_THREAD_LOCK
    MPE_LOG_STATE_BEGIN(comm,MPE_COMM_GET_NAME_ID)
    MPE_LOG_THREAD_UNLOCK

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_OFF
#endif

    returnVal = PMPI_Comm_get_name( comm, comm_name, resultlen );

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_ON
#endif

    MPE_LOG_THREAD_LOCK
    MPE_LOG_STATE_END(comm,NULL)
    MPE_LOG_THREAD_UNLOCK

    return returnVal;
}

int MPI_Open_port( MPI_Info info, char *port_name )
{
    int  returnVal;
    MPE_LOG_STATE_DECL
    MPE_LOG_THREADSTM_DECL

    MPE_LOG_THREADSTM_GET
    MPE_LOG_THREAD_LOCK
    MPE_LOG_STATE_BEGIN(MPE_COMM_NULL,MPE_OPEN_PORT_ID)
    MPE_LOG_THREAD_UNLOCK

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_OFF
#endif

    returnVal = PMPI_Open_port( info, port_name );

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_ON
#endif

    MPE_LOG_THREAD_LOCK
    MPE_LOG_STATE_END(MPE_COMM_NULL,NULL)
    MPE_LOG_THREAD_UNLOCK

    return returnVal;
}

int MPI_Close_port( char *port_name )
{
    int  returnVal;
    MPE_LOG_STATE_DECL
    MPE_LOG_THREADSTM_DECL

    MPE_LOG_THREADSTM_GET
    MPE_LOG_THREAD_LOCK
    MPE_LOG_STATE_BEGIN(MPE_COMM_NULL,MPE_CLOSE_PORT_ID)
    MPE_LOG_THREAD_UNLOCK

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_OFF
#endif

    returnVal = PMPI_Close_port( port_name );

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_ON
#endif

    MPE_LOG_THREAD_LOCK
    MPE_LOG_STATE_END(MPE_COMM_NULL,NULL)
    MPE_LOG_THREAD_UNLOCK

    return returnVal;
}

#if defined( HAVE_MPI_NAMING )
int MPI_Lookup_name( char *service_name, MPI_Info info, char *port_name )
{
    int  returnVal;
    MPE_LOG_STATE_DECL
    MPE_LOG_THREADSTM_DECL

    MPE_LOG_THREADSTM_GET
    MPE_LOG_THREAD_LOCK
    MPE_LOG_STATE_BEGIN(MPE_COMM_NULL,MPE_LOOKUP_NAME_ID)
    MPE_LOG_THREAD_UNLOCK

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_OFF
#endif

    returnVal = PMPI_Lookup_name( service_name, info, port_name );

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_ON
#endif

    MPE_LOG_THREAD_LOCK
    MPE_LOG_STATE_END(MPE_COMM_NULL,NULL)
    MPE_LOG_THREAD_UNLOCK

    return returnVal;
}

int MPI_Publish_name( char *service_name, MPI_Info info, char *port_name )
{
    int  returnVal;
    MPE_LOG_STATE_DECL
    MPE_LOG_THREADSTM_DECL

    MPE_LOG_THREADSTM_GET
    MPE_LOG_THREAD_LOCK
    MPE_LOG_STATE_BEGIN(MPE_COMM_NULL,MPE_PUBLISH_NAME_ID)
    MPE_LOG_THREAD_UNLOCK

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_OFF
#endif

    returnVal = PMPI_Publish_name( service_name, info, port_name );

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_ON
#endif

    MPE_LOG_THREAD_LOCK
    MPE_LOG_STATE_END(MPE_COMM_NULL,NULL)
    MPE_LOG_THREAD_UNLOCK

    return returnVal;
}

int MPI_Unpublish_name( char *service_name, MPI_Info info, char *port_name )
{
    int  returnVal;
    MPE_LOG_STATE_DECL
    MPE_LOG_THREADSTM_DECL

    MPE_LOG_THREADSTM_GET
    MPE_LOG_THREAD_LOCK
    MPE_LOG_STATE_BEGIN(MPE_COMM_NULL,MPE_UNPUBLISH_NAME_ID)
    MPE_LOG_THREAD_UNLOCK

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_OFF
#endif

    returnVal = PMPI_Unpublish_name( service_name, info, port_name );

#if defined( MAKE_SAFE_PMPI_CALL )
    MPE_LOG_ON
#endif

    MPE_LOG_THREAD_LOCK
    MPE_LOG_STATE_END(MPE_COMM_NULL,NULL)
    MPE_LOG_THREAD_UNLOCK

    return returnVal;
}
#endif    /* Endof if defined( HAVE_MPI_NAMING ) */

