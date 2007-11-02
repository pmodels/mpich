/*
   (C) 2001 by Argonne National Laboratory.
       See COPYRIGHT in top-level directory.
*/
#include "mpe_logging_conf.h"

#if defined( STDC_HEADERS ) || defined( HAVE_STDIO_H )
#include <stdio.h>
#endif

#include "clog_record.h"
#include "clog_util.h"
#include "clog_uuid.h"
#include "clog_commset.h"


/*@
     CLOG_Rec_print_rectype - print abbreviation shorthand for record type
     [ Support routines for CLOG_Rec_xxx_print() ]

. rectype - record type

@*/
void CLOG_Rec_print_rectype( CLOG_int32_t rectype, FILE *stream )
{
    switch (rectype) {
        case CLOG_REC_UNDEF:       fprintf( stream, "udef " ); break;
        case CLOG_REC_ENDLOG:      fprintf( stream, "elog " ); break;
        case CLOG_REC_ENDBLOCK:    fprintf( stream, "eblk " ); break;
        case CLOG_REC_STATEDEF:    fprintf( stream, "sdef " ); break;
        case CLOG_REC_EVENTDEF:    fprintf( stream, "edef " ); break;
        case CLOG_REC_CONSTDEF:    fprintf( stream, "cdef " ); break;
        case CLOG_REC_BAREEVT:     fprintf( stream, "bare " ); break;
        case CLOG_REC_CARGOEVT:    fprintf( stream, "cago " ); break;
        case CLOG_REC_MSGEVT:      fprintf( stream, "msg  " ); break;
        case CLOG_REC_COLLEVT:     fprintf( stream, "coll " ); break;
        case CLOG_REC_COMMEVT:     fprintf( stream, "comm " ); break;
        case CLOG_REC_SRCLOC:      fprintf( stream, "loc  " ); break;
        case CLOG_REC_TIMESHIFT:   fprintf( stream, "shft " ); break;
        default:                   fprintf( stream, "unknown("i32fmt") ",
                                                    rectype);
    }
}

/*@
     CLOG_Rec_print_msgtype - print communication event type
     [ Support routines for CLOG_Rec_xxx_print() ]

. etype - event type for pt2pt communication event

@*/
void CLOG_Rec_print_msgtype( CLOG_int32_t etype, FILE *stream )
{
    switch (etype) {
        case CLOG_EVT_SENDMSG: fprintf( stream, "send " ); break;
        case CLOG_EVT_RECVMSG: fprintf( stream, "recv " ); break;
            /* none predefined */
        default:               fprintf( stream, "unk("i32fmt") ", etype );
    }
}

/*@
     CLOG_Rec_print_commtype - print communicator creation event type
     [ Support routines for CLOG_Rec_xxx_print() ]

. etype - event type for communicator creation event

@*/
void CLOG_Rec_print_commtype( CLOG_int32_t comm_etype, FILE *stream )
{
    switch (comm_etype) {
        case CLOG_COMM_WORLD_CREATE:
            fprintf( stream, "CommWorldCreate " );
            break;
        case CLOG_COMM_SELF_CREATE:
            fprintf( stream, "CommSelfCreate  " );
            break;
        case CLOG_COMM_FREE:
            fprintf( stream, "CommFree        " );
            break;
        case CLOG_COMM_INTRA_CREATE:
            fprintf( stream, "IntraCommCreate " );
            break;
        case CLOG_COMM_INTER_CREATE:
            fprintf( stream, "InterCommCreate " );
            break;
        case CLOG_COMM_INTRA_LOCAL:
            fprintf( stream, "LocalIntraComm  " );
            break;
        case CLOG_COMM_INTRA_REMOTE:
            fprintf( stream, "RemoteIntraComm " );
            break;
        default:
            fprintf( stream, "unknown("i32fmt") ", comm_etype );
    }
}

/*@
     CLOG_Rec_print_colltype - print collective event type
     [ Support routines for CLOG_Rec_xxx_print() ]

. etype - event type for collective event

@*/
void CLOG_Rec_print_colltype( CLOG_int32_t etype, FILE *stream )
{
    switch (etype) {
        /* none predefined */
        default: fprintf( stream, "unk("i32fmt")", etype );
    }
}




void CLOG_Rec_Header_swap_bytes( CLOG_Rec_Header_t *hdr )
{
    CLOG_Util_swap_bytes( &(hdr->time), sizeof(CLOG_Time_t), 1 );
    CLOG_Util_swap_bytes( &(hdr->icomm), sizeof(CLOG_int32_t), 4 );
}

void CLOG_Rec_Header_print( CLOG_Rec_Header_t *hdr, FILE *stream )
{
    fprintf( stream, "ts=%f ",          hdr->time );
    fprintf( stream, "icomm="i32fmt" ", hdr->icomm );
    fprintf( stream, "rank="i32fmt" ",  hdr->rank );
    fprintf( stream, "thd="i32fmt" ",   hdr->thread );
    fprintf( stream, "type=" ); CLOG_Rec_print_rectype( hdr->rectype, stream );
}

void CLOG_Rec_StateDef_swap_bytes( CLOG_Rec_StateDef_t *statedef )
{
    CLOG_Util_swap_bytes( &(statedef->stateID), sizeof(CLOG_int32_t), 3 );
    /* We do not adjust the 'pad' field */
    /* 'color' and 'name' fields are not adjusted */
}

void CLOG_Rec_StateDef_print( CLOG_Rec_StateDef_t *statedef, FILE *stream )
{
    fprintf( stream, "state="i32fmt" ", statedef->stateID );
    fprintf( stream, "s_et="i32fmt" ",  statedef->startetype );
    fprintf( stream, "e_et="i32fmt" ",  statedef->finaletype );
    fprintf( stream, "color=%s ",       statedef->color );
    fprintf( stream, "name=%s ",        statedef->name );
    fprintf( stream, "fmt=%s\n",        statedef->format );
}

void CLOG_Rec_EventDef_swap_bytes( CLOG_Rec_EventDef_t *eventdef )
{
    CLOG_Util_swap_bytes( &(eventdef->etype), sizeof(CLOG_int32_t), 1 );
    /* 'pad' and 'name' are not adjusted */
}

void CLOG_Rec_EventDef_print( CLOG_Rec_EventDef_t *eventdef, FILE *stream )
{
    fprintf( stream, "et="i32fmt" ", eventdef->etype );
    fprintf( stream, "color=%s ",    eventdef->color );
    fprintf( stream, "name=%s ",     eventdef->name );
    fprintf( stream, "fmt=%s\n",     eventdef->format );
}

void CLOG_Rec_ConstDef_swap_bytes( CLOG_Rec_ConstDef_t *constdef )
{
    CLOG_Util_swap_bytes( &(constdef->etype), sizeof(CLOG_int32_t), 2 );
    /* 'name' are not adjusted */
}

void CLOG_Rec_ConstDef_print( CLOG_Rec_ConstDef_t *constdef, FILE *stream )
{
    fprintf( stream, "et="i32fmt" ",     constdef->etype );
    fprintf( stream, "name=%s ",         constdef->name );
    fprintf( stream, "value="i32fmt"\n", constdef->value );
}

void CLOG_Rec_BareEvt_swap_bytes( CLOG_Rec_BareEvt_t *bare )
{
    CLOG_Util_swap_bytes( &(bare->etype), sizeof(CLOG_int32_t), 1 );
}

void CLOG_Rec_BareEvt_print( CLOG_Rec_BareEvt_t *bare, FILE *stream )
{
    fprintf( stream, "et="i32fmt"\n", bare->etype );
}

void CLOG_Rec_CargoEvt_swap_bytes( CLOG_Rec_CargoEvt_t *cargo )
{
    CLOG_Util_swap_bytes( &(cargo->etype), sizeof(CLOG_int32_t), 1 );
    /* 'cargo->bytes[]' is adjusted by user */
}

void CLOG_Rec_CargoEvt_print( CLOG_Rec_CargoEvt_t *cargo, FILE *stream )
{
    fprintf( stream, "et="i32fmt" ",  cargo->etype );
    fprintf( stream, "bytes=%.32s\n", cargo->bytes );
}

void CLOG_Rec_MsgEvt_swap_bytes( CLOG_Rec_MsgEvt_t *msg )
{
    CLOG_Util_swap_bytes( &(msg->etype), sizeof(CLOG_int32_t), 5 );
    /* We do not adjust the 'pad' field */
}

void CLOG_Rec_MsgEvt_print( CLOG_Rec_MsgEvt_t *msg, FILE *stream )
{
    fprintf( stream,"et=" ); CLOG_Rec_print_msgtype( msg->etype, stream );
    fprintf( stream, "icomm="i32fmt" ", msg->icomm );
    fprintf( stream, "rank="i32fmt" ",  msg->rank );
    fprintf( stream, "tag="i32fmt" ",   msg->tag );
    fprintf( stream, "sz="i32fmt"\n",   msg->size );
}

void CLOG_Rec_CollEvt_swap_bytes( CLOG_Rec_CollEvt_t *coll )
{
    CLOG_Util_swap_bytes( &(coll->etype), sizeof(CLOG_int32_t), 3 );
}

void CLOG_Rec_CollEvt_print( CLOG_Rec_CollEvt_t *coll, FILE *stream )
{
    fprintf( stream, "et=" ); CLOG_Rec_print_colltype( coll->etype, stream );
    fprintf( stream, "root="i32fmt" ", coll->root);
    fprintf( stream, "sz="i32fmt"\n",  coll->size );
}

void CLOG_Rec_CommEvt_swap_bytes( CLOG_Rec_CommEvt_t *comm )
{
    CLOG_Util_swap_bytes( &(comm->etype), sizeof(CLOG_int32_t), 4 );
    CLOG_Uuid_swap_bytes( comm->gcomm );
    /* We do not adjust the 'pad' field */
}

void CLOG_Rec_CommEvt_print( CLOG_Rec_CommEvt_t *comm, FILE *stream )
{
    char uuid_str[CLOG_UUID_STR_SIZE] = {0};
    fprintf( stream, "et=" ); CLOG_Rec_print_commtype( comm->etype, stream );
    fprintf( stream, "icomm="i32fmt" ", comm->icomm );
    fprintf( stream, "rank="i32fmt" ",  comm->rank );
    fprintf( stream, "wrank="i32fmt" ", comm->wrank );
    CLOG_Uuid_sprint( comm->gcomm, uuid_str );
    fprintf( stream, "gcomm=%s\n",      uuid_str );
}

void CLOG_Rec_Srcloc_swap_bytes( CLOG_Rec_Srcloc_t *src )
{
    CLOG_Util_swap_bytes( &(src->srcloc), sizeof(CLOG_int32_t), 2 );
    /* 'filename' is not adjusted */
}

void CLOG_Rec_Srcloc_print( CLOG_Rec_Srcloc_t *src, FILE *stream )
{
    fprintf( stream, "srcid="i32fmt" ", src->srcloc );
    fprintf( stream, "line="i32fmt" ",  src->lineno );
    fprintf( stream, "file=%s\n",       src->filename );
}

void CLOG_Rec_Timeshift_swap_bytes( CLOG_Rec_Timeshift_t *tshift )
{
    CLOG_Util_swap_bytes( &(tshift->timeshift), sizeof(CLOG_Time_t), 1 );
}

void CLOG_Rec_Timeshift_print( CLOG_Rec_Timeshift_t *tshift, FILE *stream )
{
    fprintf( stream, "shift=%f\n", tshift->timeshift );
    /* fprintf( stream, "shift=%f,%Lx\n", tshift->timeshift, tshift->timeshift ); */
}

/*
   Assume readable(understandable) byte ordering before byteswapping
   i.e. byteswapping last.
*/
void CLOG_Rec_swap_bytes_last( CLOG_Rec_Header_t *hdr )
{
    int  rectype;
    /*
       Save hdr->rectype before byte swapping.
       After byteswapping hdr->rectype will not be understandable
    */
    rectype = hdr->rectype;
    CLOG_Rec_Header_swap_bytes( hdr );
    switch (rectype) {
        /*
        No CLOG_REC_UNDEF in CLOG_Rec_swap_bytes, i.e. emit error message
        */
        case CLOG_REC_ENDLOG:
            break;
        case CLOG_REC_ENDBLOCK:
            break;
        case CLOG_REC_STATEDEF:
            CLOG_Rec_StateDef_swap_bytes( (CLOG_Rec_StateDef_t *) hdr->rest );
            break;
        case CLOG_REC_EVENTDEF:
            CLOG_Rec_EventDef_swap_bytes( (CLOG_Rec_EventDef_t *) hdr->rest );
            break;
        case CLOG_REC_CONSTDEF:
            CLOG_Rec_ConstDef_swap_bytes( (CLOG_Rec_ConstDef_t *) hdr->rest );
            break;
        case CLOG_REC_BAREEVT:
            CLOG_Rec_BareEvt_swap_bytes( (CLOG_Rec_BareEvt_t *) hdr->rest );
            break;
        case CLOG_REC_CARGOEVT:
            CLOG_Rec_CargoEvt_swap_bytes( (CLOG_Rec_CargoEvt_t *) hdr->rest );
            break;
        case CLOG_REC_MSGEVT:
            CLOG_Rec_MsgEvt_swap_bytes( (CLOG_Rec_MsgEvt_t *) hdr->rest );
            break;
        case CLOG_REC_COLLEVT:
            CLOG_Rec_CollEvt_swap_bytes( (CLOG_Rec_CollEvt_t *) hdr->rest );
            break;
        case CLOG_REC_COMMEVT:
            CLOG_Rec_CommEvt_swap_bytes( (CLOG_Rec_CommEvt_t *) hdr->rest );
            break;
        case CLOG_REC_SRCLOC:
            CLOG_Rec_Srcloc_swap_bytes( (CLOG_Rec_Srcloc_t *) hdr->rest );
            break;
        case CLOG_REC_TIMESHIFT:
            CLOG_Rec_Timeshift_swap_bytes( (CLOG_Rec_Timeshift_t *) hdr->rest );
            break;
        default:
            fprintf( stderr, __FILE__":CLOG_Rec_swap_bytes_last() - Warning!\n"
                             "\t""Unknown CLOG record type "i32fmt"\n",
                             rectype );
            fflush( stderr );
            break;
    }
}

/*
   Assume non-readable byte ordering before byteswapping
   i.e. byteswapping first
*/
void CLOG_Rec_swap_bytes_first( CLOG_Rec_Header_t *hdr )
{
    CLOG_Rec_Header_swap_bytes( hdr );
    /* After byteswapping, hdr->rectype is understandable */
    switch (hdr->rectype) {
        /*
        No CLOG_REC_UNDEF in CLOG_Rec_pre_swap_bytes, i.e. emit error message
        */
        case CLOG_REC_ENDLOG:
            break;
        case CLOG_REC_ENDBLOCK:
            break;
        case CLOG_REC_STATEDEF:
            CLOG_Rec_StateDef_swap_bytes( (CLOG_Rec_StateDef_t *) hdr->rest );
            break;
        case CLOG_REC_EVENTDEF:
            CLOG_Rec_EventDef_swap_bytes( (CLOG_Rec_EventDef_t *) hdr->rest );
            break;
        case CLOG_REC_CONSTDEF:
            CLOG_Rec_ConstDef_swap_bytes( (CLOG_Rec_ConstDef_t *) hdr->rest );
            break;
        case CLOG_REC_BAREEVT:
            CLOG_Rec_BareEvt_swap_bytes( (CLOG_Rec_BareEvt_t *) hdr->rest );
            break;
        case CLOG_REC_CARGOEVT:
            CLOG_Rec_CargoEvt_swap_bytes( (CLOG_Rec_CargoEvt_t *) hdr->rest );
            break;
        case CLOG_REC_MSGEVT:
            CLOG_Rec_MsgEvt_swap_bytes( (CLOG_Rec_MsgEvt_t *) hdr->rest );
            break;
        case CLOG_REC_COLLEVT:
            CLOG_Rec_CollEvt_swap_bytes( (CLOG_Rec_CollEvt_t *) hdr->rest );
            break;
        case CLOG_REC_COMMEVT:
            CLOG_Rec_CommEvt_swap_bytes( (CLOG_Rec_CommEvt_t *) hdr->rest );
            break;
        case CLOG_REC_SRCLOC:
            CLOG_Rec_Srcloc_swap_bytes( (CLOG_Rec_Srcloc_t *) hdr->rest );
            break;
        case CLOG_REC_TIMESHIFT:
            CLOG_Rec_Timeshift_swap_bytes( (CLOG_Rec_Timeshift_t *) hdr->rest );
            break;
        default:
            fprintf( stderr, __FILE__":CLOG_Rec_swap_bytes_first() - Warning!\n"
                             "\t""Unknown CLOG record type "i32fmt"\n",
                             hdr->rectype );
            fflush( stderr );
            break;
    }
}

void CLOG_Rec_print( CLOG_Rec_Header_t *hdr, FILE *stream )
{
    CLOG_Rec_Header_print( hdr, stream );
    switch (hdr->rectype) {
        /*
        No CLOG_REC_UNDEF in CLOG_Rec_print, i.e. emit error message
        */
        case CLOG_REC_ENDLOG:
            fprintf( stream, "\n\n\n" );
            break;
        case CLOG_REC_ENDBLOCK:
            fprintf( stream, "\n\n" );
            break;
        case CLOG_REC_STATEDEF:
            CLOG_Rec_StateDef_print( (CLOG_Rec_StateDef_t *) hdr->rest,
                                     stream );
            break;
        case CLOG_REC_EVENTDEF:
            CLOG_Rec_EventDef_print( (CLOG_Rec_EventDef_t *) hdr->rest,
                                     stream );
            break;
        case CLOG_REC_CONSTDEF:
            CLOG_Rec_ConstDef_print( (CLOG_Rec_ConstDef_t *) hdr->rest,
                                     stream );
            break;
        case CLOG_REC_BAREEVT:
            CLOG_Rec_BareEvt_print( (CLOG_Rec_BareEvt_t *) hdr->rest,
                                    stream );
            break;
        case CLOG_REC_CARGOEVT:
            CLOG_Rec_CargoEvt_print( (CLOG_Rec_CargoEvt_t *) hdr->rest,
                                     stream );
            break;
        case CLOG_REC_MSGEVT:
            CLOG_Rec_MsgEvt_print( (CLOG_Rec_MsgEvt_t *) hdr->rest,
                                   stream );
            break;
        case CLOG_REC_COLLEVT:
            CLOG_Rec_CollEvt_print( (CLOG_Rec_CollEvt_t *) hdr->rest,
                                    stream );
            break;
        case CLOG_REC_COMMEVT:
            CLOG_Rec_CommEvt_print( (CLOG_Rec_CommEvt_t *) hdr->rest,
                                    stream );
            break;
        case CLOG_REC_SRCLOC:
            CLOG_Rec_Srcloc_print( (CLOG_Rec_Srcloc_t *) hdr->rest,
                                   stream );
            break;
        case CLOG_REC_TIMESHIFT:
            CLOG_Rec_Timeshift_print( (CLOG_Rec_Timeshift_t *) hdr->rest,
                                      stream );
            break;
        default:
            fprintf( stderr, __FILE__":CLOG_Rec_print() - \n"
                             "\t""Unrecognized CLOG record type "i32fmt"\n",
                             hdr->rectype );
            fflush( stderr );
            break;
    }
    fflush( stream );
}

static int clog_reclens[ CLOG_REC_NUM ];
static int clog_reclen_max;

/*
    Pre-compute the record size or disk footprint of a complete record,
    i.e. CLOG_Rec_Header_t + CLOG_Rec_xxx_t, as given by CLOG_Rec_size().
*/
void CLOG_Rec_sizes_init( void )
{
    clog_reclen_max                    = 0;

    clog_reclens[ CLOG_REC_ENDLOG ]    = CLOG_RECLEN_HEADER;
    if ( clog_reclens[ CLOG_REC_ENDLOG ] > clog_reclen_max )
        clog_reclen_max  = clog_reclens[ CLOG_REC_ENDLOG ];

    clog_reclens[ CLOG_REC_ENDBLOCK ]  = CLOG_RECLEN_HEADER;
    if ( clog_reclens[ CLOG_REC_ENDBLOCK ] > clog_reclen_max )
        clog_reclen_max  = clog_reclens[ CLOG_REC_ENDBLOCK ];

    clog_reclens[ CLOG_REC_STATEDEF ]  = CLOG_RECLEN_HEADER
                                       + CLOG_RECLEN_STATEDEF;
    if ( clog_reclens[ CLOG_REC_STATEDEF ] > clog_reclen_max )
        clog_reclen_max  = clog_reclens[ CLOG_REC_STATEDEF ];

    clog_reclens[ CLOG_REC_EVENTDEF ]  = CLOG_RECLEN_HEADER
                                       + CLOG_RECLEN_EVENTDEF;
    if ( clog_reclens[ CLOG_REC_EVENTDEF ] > clog_reclen_max )
        clog_reclen_max  = clog_reclens[ CLOG_REC_EVENTDEF ];

    clog_reclens[ CLOG_REC_CONSTDEF ]  = CLOG_RECLEN_HEADER
                                       + CLOG_RECLEN_CONSTDEF;
    if ( clog_reclens[ CLOG_REC_CONSTDEF ] > clog_reclen_max )
        clog_reclen_max  = clog_reclens[ CLOG_REC_CONSTDEF ];

    clog_reclens[ CLOG_REC_BAREEVT ]   = CLOG_RECLEN_HEADER
                                       + CLOG_RECLEN_BAREEVT;
    if ( clog_reclens[ CLOG_REC_BAREEVT ] > clog_reclen_max )
        clog_reclen_max  = clog_reclens[ CLOG_REC_BAREEVT ];

    clog_reclens[ CLOG_REC_CARGOEVT ]  = CLOG_RECLEN_HEADER
                                       + CLOG_RECLEN_CARGOEVT;
    if ( clog_reclens[ CLOG_REC_CARGOEVT ] > clog_reclen_max )
        clog_reclen_max  = clog_reclens[ CLOG_REC_CARGOEVT ];

    clog_reclens[ CLOG_REC_MSGEVT ]    = CLOG_RECLEN_HEADER
                                       + CLOG_RECLEN_MSGEVT;
    if ( clog_reclens[ CLOG_REC_MSGEVT ] > clog_reclen_max )
        clog_reclen_max  = clog_reclens[ CLOG_REC_MSGEVT ];

    clog_reclens[ CLOG_REC_COLLEVT ]   = CLOG_RECLEN_HEADER
                                       + CLOG_RECLEN_COLLEVT;
    if ( clog_reclens[ CLOG_REC_COLLEVT ] > clog_reclen_max )
        clog_reclen_max  = clog_reclens[ CLOG_REC_COLLEVT ];

    clog_reclens[ CLOG_REC_COMMEVT ]   = CLOG_RECLEN_HEADER
                                       + CLOG_RECLEN_COMMEVT;
    if ( clog_reclens[ CLOG_REC_COMMEVT ] > clog_reclen_max )
        clog_reclen_max  = clog_reclens[ CLOG_REC_COMMEVT ];

    clog_reclens[ CLOG_REC_SRCLOC ]    = CLOG_RECLEN_HEADER
                                       + CLOG_RECLEN_SRCLOC;
    if ( clog_reclens[ CLOG_REC_SRCLOC ] > clog_reclen_max )
        clog_reclen_max  = clog_reclens[ CLOG_REC_SRCLOC ];

    clog_reclens[ CLOG_REC_TIMESHIFT ] = CLOG_RECLEN_HEADER
                                       + CLOG_RECLEN_TIMESHIFT;
    if ( clog_reclens[ CLOG_REC_TIMESHIFT ] > clog_reclen_max )
        clog_reclen_max  = clog_reclens[ CLOG_REC_TIMESHIFT ];
}

/*
    CLOG_Rec_size() - returns complete record size on the disk,
                      ie. disk footprint.
*/
int CLOG_Rec_size( CLOG_int32_t rectype )
{
    if ( rectype < CLOG_REC_NUM && rectype >= 0 )
        return clog_reclens[ rectype ];
    else {
        fprintf( stderr, __FILE__":CLOG_Rec_size() - Warning!"
                        "\t""Unknown record type "i32fmt"\n", rectype );
        fflush( stderr );
        /*
           Assume it has at least a CLOG_Rec_Header_t,
           so length of CLOG_REC_ENDLOG

           Instead of "return clog_reclens[ CLOG_REC_ENDLOG ];",
           force a coredump
        */
        return clog_reclens[ rectype ];
    }
}

int CLOG_Rec_size_max( void )
{
    return clog_reclen_max;
}
