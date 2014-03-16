/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*  
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* style: allow:printf:13 sig:0 */

#include "mpiimpl.h"
/* These are pointers to the static variables in src/mpid/ch3/src/ch3u_recvq.c
   that contains the *addresses* of the posted and unexpected queue head 
   pointers */
extern MPID_Request ** const MPID_Recvq_posted_head_ptr, 
    ** const MPID_Recvq_unexpected_head_ptr;

#include "mpi_interface.h"

/* This is from dbginit.c; it is not exported to other files */
typedef struct MPIR_Sendq {
    MPID_Request *sreq;
    int tag, rank, context_id;
    struct MPIR_Sendq *next;
} MPIR_Sendq;
extern MPIR_Sendq *MPIR_Sendq_head;
/* This is from dbginit.c; it is not exported to other files */
typedef struct MPIR_Comm_list {
    int sequence_number;   /* Used to detect changes in the list */
    MPID_Comm *head;       /* Head of the list */
} MPIR_Comm_list;

extern MPIR_Comm_list MPIR_All_communicators;

/* 
   This file contains emulation routines for the methods and functions normally
   provided by the debugger.  This file is only used for testing the 
   dll_mpich.c debugger interface.
 */

/* These are mock-ups of the find type and find offset routines.  Since 
   there are no portable routines to access the symbol table in an image,
   we hand-code the specific types that we use in this code. 
   These routines (more precisely, the field_offset routine) need to 
   known the layout of the internal data structures */
enum { TYPE_UNKNOWN = 0, 
       TYPE_MPID_COMM = 1, 
       TYPE_MPIR_COMM_LIST = 2, 
       TYPE_MPIDI_REQUEST = 3, 
       TYPE_MPIDI_MESSAGE_MATCH = 4,
       TYPE_MPID_REQUEST = 5, 
       TYPE_MPIR_SENDQ = 6,
       TYPE_MPIDI_MESSAGE_MATCH_PARTS = 7,
} KnownTypes;

/* The dll_mpich.c has a few places where it doesn't always use the most 
   recent type, so a static current type will not work.  Instead, we
   have an example of each type, and return that value. */

static int knownTypesArray[] = { TYPE_UNKNOWN, TYPE_MPID_COMM, 
				 TYPE_MPIR_COMM_LIST, TYPE_MPIDI_REQUEST, 
				 TYPE_MPIDI_MESSAGE_MATCH, TYPE_MPID_REQUEST, 
				 TYPE_MPIR_SENDQ, 
				 TYPE_MPIDI_MESSAGE_MATCH_PARTS };

mqs_type * dbgrI_find_type(mqs_image *image, char *name, 
			   mqs_lang_code lang)
{
    int curType = TYPE_UNKNOWN;

    if (strcmp(name,"MPID_Comm") == 0) {
	curType = TYPE_MPID_COMM;
    }
    else if (strcmp( name, "MPIR_Comm_list" ) == 0) {
	curType = TYPE_MPIR_COMM_LIST;
    }
    else if (strcmp( name, "MPIDI_Request" ) == 0) {
	curType = TYPE_MPIDI_REQUEST;
    }
    else if (strcmp( name, "MPIDI_Message_match" ) == 0) {
	curType = TYPE_MPIDI_MESSAGE_MATCH;
    }
    else if (strcmp( name, "MPIDI_Message_match_parts_t" ) == 0) {
	curType = TYPE_MPIDI_MESSAGE_MATCH_PARTS;
    }
    else if (strcmp( name, "MPID_Request" ) == 0) {
	curType = TYPE_MPID_REQUEST;
    }
    else if (strcmp( name, "MPIR_Sendq" ) == 0) {
	curType = TYPE_MPIR_SENDQ;
    }
    else {
	curType = TYPE_UNKNOWN;
    }
    return (mqs_type *)&knownTypesArray[curType];
}

int dbgrI_field_offset(mqs_type *type, char *name)
{
    int off = -1;

    int curType = *(int*)type;

    /* printf ( "curtype is %d\n", curType ); */

    switch (curType) {
    case TYPE_MPID_COMM:
	{
	    MPID_Comm c;
	    if (strcmp( name, "name" ) == 0) {
		off = ((char*)&(c.name[0]) - (char*)&c.handle);
	    }
	    else if (strcmp( name, "comm_next" ) == 0) {
		off = ((char*)&c.comm_next - (char*)&c.handle);
	    }
	    else if (strcmp( name, "remote_size" ) == 0) {
		off = ((char*)&c.remote_size - (char*)&c.handle);
	    }
	    else if (strcmp( name, "rank" ) == 0) {
		off = ((char*)&c.rank - (char*)&c.handle);
	    }
	    else if (strcmp( name, "context_id" ) == 0) {
		off = ((char*)&c.context_id - (char*)&c.handle);
	    }
	    else if (strcmp( name, "recvcontext_id" ) == 0) {
		off = ((char*)&c.recvcontext_id - (char*)&c.handle);
	    }
	    else {
		printf( "Panic! Unrecognized COMM field %s\n", name );
	    }
	}
	break;
    case TYPE_MPIR_COMM_LIST:
	{
	    MPIR_Comm_list c;
	    if (strcmp( name, "sequence_number" ) == 0) {
		off = ((char*)&c.sequence_number - (char*)&c);
	    }
	    else if (strcmp( name, "head" ) == 0) {
		off = ((char*)&c.head - (char*)&c);
	    }
	    else {
		printf( "Panic! Unrecognized Comm List field %s\n", name );
	    }
	}
	break;
    case TYPE_MPIDI_REQUEST:
	{
	    struct MPIDI_Request c;
	    if (strcmp( name, "next" ) == 0) {
		off = ((char *)&c.next - (char *)&c);
	    }
	    else if (strcmp( name, "match" ) == 0) {
		off = ((char *)&c.match - (char *)&c);
	    }
	    else if (strcmp( name, "user_buf" ) == 0) {
		off = ((char *)&c.user_buf - (char *)&c);
	    }
	    else if (strcmp( name, "user_count" ) == 0) {
		off = ((char *)&c.user_count - (char *)&c);
	    }
	    else if (strcmp( name, "datatype" ) == 0) {
		off = ((char *)&c.datatype - (char *)&c);
	    }
	    else {
		printf( "Panic! Unrecognized mpidi request field %s\n", name );
	    }
	}
	break;
    case TYPE_MPIDI_MESSAGE_MATCH:
	{
	    if (strcmp( name, "parts" ) == 0) {
		off = 0;
	    }
	    else {
		printf( "Panic: Unrecognized mpidi_match fields %s\n", name );
	    }
	}
	break;
    case TYPE_MPIDI_MESSAGE_MATCH_PARTS:
	{
	    MPIDI_Message_match c;
	    if (strcmp( name, "tag" ) == 0) {
		off = ((char *)&c.parts.tag - (char *)&c);
	    }
	    else if (strcmp( name, "rank" ) == 0) {
		off = ((char *)&c.parts.rank - (char *)&c);
	    }
	    else if (strcmp( name, "context_id" ) == 0) {
		off = ((char *)&c.parts.context_id - (char *)&c);
	    }
	    else {
		printf( "Panic! Unrecognized message match parts field %s\n", name );
	    }
	}
	break;
    case TYPE_MPID_REQUEST:
	{
	    MPID_Request c;
	    if (strcmp( name, "dev" ) == 0) {
		off = ((char *)&c.dev - (char *)&c);
	    }
	    else if (strcmp( name, "status" ) == 0) {
		off = ((char *)&c.status - (char *)&c);
	    }
	    else if (strcmp( name, "cc" ) == 0) {
		off = ((char *)&c.cc - (char *)&c);
	    }
	    /* else if (strcmp( name, "next" ) == 0) {
		off = ((char *)&c.next - (char *)&c);
		} */
	    else {
		printf( "Panic! Unrecognized request field %s\n", name );
	    }
	}
	break;
    case TYPE_MPIR_SENDQ:
	{
	    struct MPIR_Sendq c;
	    if (strcmp( name, "next" ) == 0) {
		off = ((char *)&c.next - (char *)&c);
	    }
	    else if (strcmp( name, "tag" ) == 0) {
		off = ((char *)&c.tag - (char *)&c);
	    }
	    else if (strcmp( name, "rank" ) == 0) {
		off = ((char *)&c.rank - (char *)&c);
	    }
	    else if (strcmp( name, "context_id" ) == 0) {
		off = ((char *)&c.context_id - (char *)&c);
	    }
	    else if (strcmp( name, "sreq" ) == 0) {
		off = ((char *)&c.sreq - (char *)&c);
	    }
	    else {
		printf( "Panic! Unrecognized Sendq field %s\n", name );
	    }
	}
	break;
    case TYPE_UNKNOWN:
	off = -1;
	break;
    default:
	off = -1;
	break;
    }
    return off;
}

/* Simulate converting name to the address of a variable/symbol */
int dbgrI_find_symbol( mqs_image *image, char *name, mqs_taddr_t * loc )
{
    if (strcmp( name, "MPIR_All_communicators" ) == 0) {
	*loc = (mqs_taddr_t)&MPIR_All_communicators;
	/* The following printfs can help is diagnosing problems when the
	   extern variable MPIR_All_communicators appears not to be 
	   correctly resolved. */
	printf( "all communicators head at %p\n", *loc );
	printf( "all communicators head as pointer %p\n", &MPIR_All_communicators );
	printf( "head is %p\n", MPIR_All_communicators.head );
	return mqs_ok;
    }
    else if (strcmp( name, "MPID_Recvq_posted_head_ptr" ) == 0) {
	*loc = (mqs_taddr_t)&MPID_Recvq_posted_head_ptr;
	printf( "Address of ptr to posted head ptr = %p\n", &MPID_Recvq_posted_head_ptr );
	printf( "Address of posted head ptr = %p\n", MPID_Recvq_posted_head_ptr );
	return mqs_ok;
    }
    else if (strcmp( name, "MPID_Recvq_unexpected_head_ptr" ) == 0) {
	*loc = (mqs_taddr_t)&MPID_Recvq_unexpected_head_ptr;
	return mqs_ok;
    }
    else if (strcmp( name, "MPIR_Sendq_head" ) == 0) {
	*loc = (mqs_taddr_t)&MPIR_Sendq_head;
	return mqs_ok;
    }
    else {
	printf( "Panic! Unrecognized symbol %s\n", name );
    }
    return 1;
}
