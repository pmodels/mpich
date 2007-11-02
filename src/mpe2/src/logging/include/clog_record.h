/*
   (C) 2001 by Argonne National Laboratory.
       See COPYRIGHT in top-level directory.
*/
#if !defined( _CLOG_RECORD )
#define _CLOG_RECORD

#include "clog_inttypes.h"
#include "clog_block.h"
#include "clog_timer.h"
#include "clog_commset.h"

/*
   We distinguish between record types and event types (kinds), and have a
   small number of pre-defined record types, including a raw one.  We keep all
   records double-aligned for the sake of the double timestamp field.  Lengths
   are given in doubles.  Log records will usually consist of a
   CLOG_Rec_header_t followed by one of the types that follow it below,
   but record types CLOG_Rec_endblock_t and CLOG_Rec_endlog_t consist of
   the header alone.
*/

/*
   Whenever fields are add/deleted or padding is added or made meaningful,
   synchronize the changes with all CLOG_Rec_xxx_swap_bytes() routines.
*/
typedef struct {
    CLOG_Time_t       time;    /* Crucial to be the 1st item */
    CLOG_CommLID_t    icomm;   /* LOCAL intracomm's internal local/global ID */
    CLOG_int32_t      rank;    /* rank within communicator labelled by icomm */
    CLOG_int32_t      thread;  /* local thread ID */
    CLOG_int32_t      rectype;
    CLOG_DataUnit_t   rest[1];
} CLOG_Rec_Header_t;
/*  3 CLOG_Time_t's */
#define CLOG_RECLEN_HEADER     ( sizeof(CLOG_Time_t) \
                               + 4 * sizeof(CLOG_int32_t) )

typedef char CLOG_Str_Color_t[ 3 * sizeof(CLOG_Time_t) ];
typedef char CLOG_Str_Desc_t[ 4 * sizeof(CLOG_Time_t) ];
typedef char CLOG_Str_File_t[ 5 * sizeof(CLOG_Time_t) ];
typedef char CLOG_Str_Format_t[ 5 * sizeof(CLOG_Time_t) ];
typedef char CLOG_Str_Bytes_t[ 4 * sizeof(CLOG_Time_t) ];

/* Rec_StateDef defines the attributes of a state which consists of 2 events */
typedef struct {
    CLOG_int32_t      stateID;   /* integer identifier for state */
    CLOG_int32_t      startetype;/* beginning event for state */
    CLOG_int32_t      finaletype;/* ending event for state */
    CLOG_int32_t      pad;
    CLOG_Str_Color_t  color;     /* string for color */
    CLOG_Str_Desc_t   name;      /* string describing state */
    CLOG_Str_Format_t format;    /* format string for state's decription data */
    CLOG_DataUnit_t   end[1];
} CLOG_Rec_StateDef_t;
/* 14 CLOG_Time_t's */
#define CLOG_RECLEN_STATEDEF   ( 4 * sizeof(CLOG_int32_t) \
                               + sizeof(CLOG_Str_Color_t) \
                               + sizeof(CLOG_Str_Desc_t) \
                               + sizeof(CLOG_Str_Format_t) )

/* Rec_EventDef defines the attributes of a event */
typedef struct {
    CLOG_int32_t      etype;     /* event ID */
    CLOG_int32_t      pad;
    CLOG_Str_Color_t  color;     /* string for color */
    CLOG_Str_Desc_t   name;      /* string describing event */
    CLOG_Str_Format_t format;    /* format string for the event */
    CLOG_DataUnit_t   end[1];
} CLOG_Rec_EventDef_t;
/* 13 CLOG_Time_t's */
#define CLOG_RECLEN_EVENTDEF   ( 2 * sizeof(CLOG_int32_t) \
                               + sizeof(CLOG_Str_Color_t) \
                               + sizeof(CLOG_Str_Desc_t) \
                               + sizeof(CLOG_Str_Format_t) )

/* Rec_Const defines some specific constant of the logfile */
typedef struct {
    CLOG_int32_t      etype;     /* raw event */
    CLOG_int32_t      value;     /* uninterpreted data */
    CLOG_Str_Desc_t   name;      /* uninterpreted string */
    CLOG_DataUnit_t   end[1];
} CLOG_Rec_ConstDef_t;
/*  5 CLOG_Time_t's */
#define CLOG_RECLEN_CONSTDEF   ( 2 * sizeof(CLOG_int32_t) \
                               + sizeof(CLOG_Str_Desc_t) )

/* Rec_BareEvt defines the simplest event, i.e. bare */
typedef struct {
    CLOG_int32_t      etype;     /* basic event */
    CLOG_int32_t      pad;
    CLOG_DataUnit_t   end[1];
} CLOG_Rec_BareEvt_t;
/*  1 CLOG_Time_t's */
#define CLOG_RECLEN_BAREEVT    ( 2 * sizeof(CLOG_int32_t) )

/* Rec_CargoEvt defines a event with a payload */
typedef struct {
    CLOG_int32_t      etype;     /* basic event */
    CLOG_int32_t      pad;
    CLOG_Str_Bytes_t  bytes;     /* byte storage */
    CLOG_DataUnit_t   end[1];
} CLOG_Rec_CargoEvt_t;
/*  5 CLOG_Time_t's */
#define CLOG_RECLEN_CARGOEVT   ( 2 * sizeof(CLOG_int32_t) \
                               + sizeof(CLOG_Str_Bytes_t) )

/* Rec_MsgEvt defines a message event pairs */
typedef struct {
    CLOG_int32_t      etype;   /* kind of message event */
    CLOG_CommLID_t    icomm;   /* REMOTE intracomm's internal local/global ID */
    CLOG_int32_t      rank;    /* src/dest rank in send/recv in REMOTE icomm */
    CLOG_int32_t      tag;     /* message tag */
    CLOG_int32_t      size;    /* length in bytes */
    CLOG_int32_t      pad;
    CLOG_DataUnit_t   end[1];
} CLOG_Rec_MsgEvt_t;
/*  2 CLOG_Time_t's */
#define CLOG_RECLEN_MSGEVT        ( 6 * sizeof(CLOG_int32_t) )

/* Rec_CollEvt defines events for collective operation */
typedef struct {
    CLOG_int32_t      etype;   /* type of collective event */
    CLOG_int32_t      root;    /* root of collective op */
    CLOG_int32_t      size;    /* length in bytes */
    CLOG_int32_t      pad;
    CLOG_DataUnit_t   end[1];
} CLOG_Rec_CollEvt_t;
/*  2 CLOG_Time_t's */
#define CLOG_RECLEN_COLLEVT    ( 4 * sizeof(CLOG_int32_t) )

/* Rec_CommEvt defines events for {Intra|Inter}Communicator */
/* Assume CLOG_CommGID_t is of size of multiple of sizeof(double) */
typedef struct {
    CLOG_int32_t      etype;      /* type of communicator creation */
    CLOG_CommLID_t    icomm;      /* communicator's internal local/global ID */
    CLOG_int32_t      rank;       /* rank in icomm */
    CLOG_int32_t      wrank;      /* rank in MPI_COMM_WORLD */
    CLOG_CommGID_t    gcomm;      /* globally unique ID */
    CLOG_DataUnit_t   end[1];
} CLOG_Rec_CommEvt_t;
/*  2 CLOG_Time_t's */
#define CLOG_RECLEN_COMMEVT    ( 4 * sizeof(CLOG_int32_t) + CLOG_UUID_SIZE )

typedef struct {
    CLOG_int32_t      srcloc;    /* id of source location */
    CLOG_int32_t      lineno;    /* line number in source file */
    CLOG_Str_File_t   filename;  /* source file of log statement */
    CLOG_DataUnit_t   end[1];
} CLOG_Rec_Srcloc_t;
/*  6 CLOG_Time_t's */
#define CLOG_RECLEN_SRCLOC     ( 2 * sizeof(CLOG_int32_t) \
                               + sizeof(CLOG_Str_File_t) )

typedef struct {
    CLOG_Time_t       timeshift; /* time shift for this process */
    CLOG_DataUnit_t   end[1];
} CLOG_Rec_Timeshift_t;
/*  1 CLOG_Time_t's */
#define CLOG_RECLEN_TIMESHIFT  ( sizeof(CLOG_Time_t) )

/*
   Total number of CLOG_REC_XXX listed below,
   starting from 0 to CLOG_REC_NUM, excluding CLOG_REC_UNDEF
*/
#define CLOG_REC_NUM         12

/* predefined record types (all include header) */
#define CLOG_REC_UNDEF       -1     /* undefined */
#define CLOG_REC_ENDLOG       0     /* end of log marker */
#define CLOG_REC_ENDBLOCK     1     /* end of block marker */
#define CLOG_REC_STATEDEF     2     /* state description */
#define CLOG_REC_EVENTDEF     3     /* event description */
#define CLOG_REC_CONSTDEF     4     /* constant description */
#define CLOG_REC_BAREEVT      5     /* arbitrary record */
#define CLOG_REC_CARGOEVT     6     /* arbitrary record */
#define CLOG_REC_MSGEVT       7     /* message event */
#define CLOG_REC_COLLEVT      8     /* collective event */
#define CLOG_REC_COMMEVT      9     /* communicator construction/destruction  */
#define CLOG_REC_SRCLOC      10     /* identifier of location in source */
#define CLOG_REC_TIMESHIFT   11     /* time shift calculated for this process */

/*
   Be sure NO CLOG internal states are overlapped with ANY of MPI states
   and MPE's internal states which are defined in log_mpi_core.c.
*/
/*
   280 <= CLOG's internal stateID < 300 = MPE_MAX_KNOWN_STATES
   whose start_evtID = 2 * stateID,  final_evtID = 2 * stateID + 1

   This is done so the CLOG internal stateID/evetIDs are included in
   the clog2TOslog2's predefined MPI uninitialized states.

   CLOG internal state, CLOG_Buffer_write2disk
*/
#define CLOG_STATEID_BUFFERWRITE     280
#define CLOG_EVT_BUFFERWRITE_START   560
#define CLOG_EVT_BUFFERWRITE_FINAL   561

/* special event type for message type events */
#define CLOG_EVT_SENDMSG   -101
#define CLOG_EVT_RECVMSG   -102

/* special event type for defining constants */
#define CLOG_EVT_CONST     -201

void CLOG_Rec_print_rectype( CLOG_int32_t rectype, FILE *stream );
void CLOG_Rec_print_msgtype( CLOG_int32_t etype, FILE *stream );
void CLOG_Rec_print_commtype( CLOG_int32_t etype, FILE *stream );
void CLOG_Rec_print_colltype( CLOG_int32_t etype, FILE *stream );

void CLOG_Rec_Header_swap_bytes( CLOG_Rec_Header_t *hdr );
void CLOG_Rec_Header_print( CLOG_Rec_Header_t *hdr, FILE *stream );

void CLOG_Rec_StateDef_swap_bytes( CLOG_Rec_StateDef_t *statedef );
void CLOG_Rec_StateDef_print( CLOG_Rec_StateDef_t *statedef, FILE *stream );

void CLOG_Rec_EventDef_swap_bytes( CLOG_Rec_EventDef_t *eventdef );
void CLOG_Rec_EventDef_print( CLOG_Rec_EventDef_t *eventdef, FILE *stream );

void CLOG_Rec_ConstDef_swap_bytes( CLOG_Rec_ConstDef_t *constdef );
void CLOG_Rec_ConstDef_print( CLOG_Rec_ConstDef_t *constdef, FILE *stream );

void CLOG_Rec_BareEvt_swap_bytes( CLOG_Rec_BareEvt_t *bare );
void CLOG_Rec_BareEvt_print( CLOG_Rec_BareEvt_t *bare, FILE *stream );

void CLOG_Rec_CargoEvt_swap_bytes( CLOG_Rec_CargoEvt_t *cargo );
void CLOG_Rec_CargoEvt_print( CLOG_Rec_CargoEvt_t *cargo, FILE *stream );

void CLOG_Rec_MsgEvt_swap_bytes( CLOG_Rec_MsgEvt_t *msg );
void CLOG_Rec_MsgEvt_print( CLOG_Rec_MsgEvt_t *msg, FILE *stream );

void CLOG_Rec_CollEvt_swap_bytes( CLOG_Rec_CollEvt_t *coll );
void CLOG_Rec_CollEvt_print( CLOG_Rec_CollEvt_t *coll, FILE *stream );

void CLOG_Rec_CommEvt_swap_bytes( CLOG_Rec_CommEvt_t *comm );
void CLOG_Rec_CommEvt_print( CLOG_Rec_CommEvt_t *comm, FILE *stream );

void CLOG_Rec_Srcloc_swap_bytes( CLOG_Rec_Srcloc_t *src );
void CLOG_Rec_Srcloc_print( CLOG_Rec_Srcloc_t *src, FILE *stream );

void CLOG_Rec_Timeshift_swap_bytes( CLOG_Rec_Timeshift_t *tshift );
void CLOG_Rec_Timeshift_print( CLOG_Rec_Timeshift_t *tshift, FILE *stream );

void CLOG_Rec_swap_bytes_last( CLOG_Rec_Header_t *hdr );
void CLOG_Rec_swap_bytes_first( CLOG_Rec_Header_t *hdr );
void CLOG_Rec_print( CLOG_Rec_Header_t *hdr, FILE *stream );
void CLOG_Rec_sizes_init( void );
int CLOG_Rec_size( CLOG_int32_t rectype );
int CLOG_Rec_size_max( void );

#endif  /* of _CLOG_RECORD */
