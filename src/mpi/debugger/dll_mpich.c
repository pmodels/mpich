/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*  
 *  (C) 2005 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* Fixme: include the mpichconf.h file? */

/* Allow fprintf in debug statements */
/* style: allow:fprintf:5 sig:0 */

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "mpi.h"

/* #define DEBUG_MPIDBG_DLL 1 */

/* Define this to have the code print out details of its list traversal
   action.  This is primarily for use with dbgstub.c and the test programs
   such as tvtest.c */
/* #define DEBUG_LIST_ITER */

/* Define this to have the code print out its operation to a file.
   This may be used to help understand how the debugger is using this
   interface */
/* #define DEBUG_MPIDBG_LOGGING */
#ifdef DEBUG_MPIDBG_LOGGING
#include <stdio.h>
FILE *debugfp = 0;

static void initLogFile(void)
{
    if (!debugfp) {
	debugfp = fopen( "mpich-dbg-interface-log.txt", "w" );
    }
}
#else
/* no-op definition */
#define initLogFile()
#endif

/* MPIR_dll_name is defined in dbg_init.c; it must be part of the target image,
   not the debugger interface */

/* mpi_interface.h defines the interface to the debugger.  This interface
   is the same for any MPI implementation, for a given debugger 
   (a more precise name might be mpi_tv_interface.h) */
#include "mpi_interface.h"
/* mpich_dll_defs.h defines the structures for a particular MPI 
   implementation (MPICH in this case) */
#include "mpich_dll_defs.h"

/* style: allow:strncpy:1 sig:0 */

/* ------------------------------------------------------------------------ */
/* Local variables for this package */

static const mqs_basic_callbacks *mqs_basic_entrypoints = 0;
static int host_is_big_endian = -1;

/* ------------------------------------------------------------------------ */
/* Error values. */
enum {
    err_silent_failure = mqs_first_user_code, 

    err_no_current_communicator,
    err_bad_request,
    err_no_store, 
    err_all_communicators, 
    err_group_corrupt,

  err_failed_qhdr,
    err_unexpected,
    err_posted,

  err_failed_queue,
    err_first,

};

/* Internal structure we hold for each communicator */
typedef struct communicator_t
{
  struct communicator_t * next;
  group_t *               group;		/* Translations */
  int                     context_id;		/* To catch changes */
  int                     recvcontext_id;       /* May also be needed for 
						   matchine */
  int                     present;
  mqs_communicator        comm_info;		/* Info needed at the higher level */
} communicator_t;

/* Internal functions used only by routines in this package */
static void mqs_free_communicator_list( struct communicator_t *comm );

static int communicators_changed (mqs_process *proc);
static int rebuild_communicator_list (mqs_process *proc);
static int compare_comms (const void *a, const void *b);

static group_t * find_or_create_group (mqs_process *proc,
				       mqs_tword_t np,
				       mqs_taddr_t table);
static int translate (group_t *this, int idx);
#if 0
static int reverse_translate (group_t * this, int idx);
#endif
static void group_decref (group_t * group);
static communicator_t * find_communicator (mpich_process_info *p_info,
				   mqs_taddr_t comm_base, int recv_ctx);

/* ------------------------------------------------------------------------ */
/* 
 * Many of the services used by this file are performed by calling 
 * functions executed by the debugger.  In other words, these are routines
 * that the debugger must export to this package.  To make it easy to 
 * identify these functions as well as to make their use simple,
 * we use macros that start with dbgr_xxx (for debugger).  These 
 * function pointers are set early in the initialization phase.
 *
 * Note: to avoid any changes to the mpi_interface.h file, the fields in
 * the structures that contain the function pointers have not been 
 * renamed dbgr_xxx and continue to use their original mqs_ prefix.  
 * Using the dbgr_ prefix for the debugger-provided callbacks was done to
 * make it more obvious whether the debugger or the MPI interface DLL is
 * responsible for providing the function.
 */
#define dbgr_malloc           (mqs_basic_entrypoints->mqs_malloc_fp)
#define dbgr_free             (mqs_basic_entrypoints->mqs_free_fp)
#define dbgr_prints           (mqs_basic_entrypoints->mqs_eprints_fp)
#define dbgr_put_image_info   (mqs_basic_entrypoints->mqs_put_image_info_fp)
#define dbgr_get_image_info   (mqs_basic_entrypoints->mqs_get_image_info_fp)
#define dbgr_put_process_info (mqs_basic_entrypoints->mqs_put_process_info_fp)
#define dbgr_get_process_info (mqs_basic_entrypoints->mqs_get_process_info_fp)

/* These macros *RELY* on the function already having set up the conventional
 * local variables i_info or p_info.
 */
#define dbgr_find_type        (i_info->image_callbacks->mqs_find_type_fp)
#define dbgr_field_offset     (i_info->image_callbacks->mqs_field_offset_fp)
#define dbgr_get_type_sizes   (i_info->image_callbacks->mqs_get_type_sizes_fp)
#define dbgr_find_function    (i_info->image_callbacks->mqs_find_function_fp)
#define dbgr_find_symbol      (i_info->image_callbacks->mqs_find_symbol_fp)

#define dbgr_get_image        (p_info->process_callbacks->mqs_get_image_fp)
#define dbgr_get_global_rank  (p_info->process_callbacks->mqs_get_global_rank_fp)
#define dbgr_fetch_data       (p_info->process_callbacks->mqs_fetch_data_fp)
#define dbgr_target_to_host   (p_info->process_callbacks->mqs_target_to_host_fp)

/* Routines to access data within the process */
static mqs_taddr_t fetch_pointer (mqs_process * proc, mqs_taddr_t addr, 
				  mpich_process_info *p_info);
static mqs_tword_t fetch_int (mqs_process * proc, mqs_taddr_t addr, 
			      mpich_process_info *p_info);
static mqs_tword_t fetch_int16 (mqs_process * proc, mqs_taddr_t addr, 
				mpich_process_info *p_info);

/* ------------------------------------------------------------------------ */
/* Startup calls 
   These three routines are the first ones invoked by the debugger; they
   are used to ensure that the debug interface library is a known version.
*/
int mqs_version_compatibility ( void )
{
  return MQS_INTERFACE_COMPATIBILITY;
} 

char *mqs_version_string ( void )
{
    return (char *)"MPICH message queue support for MPICH " MPICH_VERSION " compiled on " __DATE__;
} 

/* Allow the debugger to discover the size of an address type */
int mqs_dll_taddr_width (void)
{
  return sizeof (mqs_taddr_t);
} 

/* ------------------------------------------------------------------------ */
/* Initialization 
   
   The function mqs_setup_basic_callbacks is used by the debugger to 
   inform the routines in this file of the addresses of functions that 
   it may call in the debugger.

   The function mqs_setup_image creates the image structure (local to this
   file) and tell the debugger about it

   The function mqs_image_has_queues initializes the image structure.  
   Much of the information that is saved in the image structure is information
   about the relative offset to data within an MPICH data structure.
   These offsets allow the debugger to retrieve information about the
   MPICH structures.  The debugger routine dbgr_find_type is used to 
   find information on an named type, and dbgr_field_offset is used 
   to get the offset of a named field within a type.

   The function mqs_setup_process(process, callbacks) creates a private 
   process information structure and stores a pointer to it in process 
   (using dbgr_put_process_info).  The use of a routine to store this 
   value rather than passing an address to the process structure is 
   done to give the debugger control over any operation that might store
   into the debuggers memory (instead, we'll use put_xxx_info).

   The function mqs_process_has_queues ??
 */
void mqs_setup_basic_callbacks (const mqs_basic_callbacks * cb)
{
  int t = 1;
  initLogFile();
  host_is_big_endian    = (*(char *)&t) != 1;
  mqs_basic_entrypoints = cb;
} 

/* 
   Allocate and setup the basic image data structure.  Also 
   save the callbacks provided by the debugger; these will be used
   to access information about the image.  This memory may be recovered
   with mqs_destroy_image_info.
 */
int mqs_setup_image (mqs_image *image, const mqs_image_callbacks *icb)
{
    mpich_image_info *i_info = 
      (mpich_image_info *)dbgr_malloc (sizeof (mpich_image_info));
    
    if (!i_info)
	return err_no_store;

    memset ((void *)i_info, 0, sizeof (mpich_image_info));
    i_info->image_callbacks = icb;		/* Before we do *ANYTHING* */
   
    /* Tell the debugger to associate i_info with image */
    dbgr_put_image_info (image, (mqs_image_info *)i_info);
    
    return mqs_ok;
} 

/*
 * Setup information needed to access the queues.  If successful, return
 * mqs_ok.  If not, return an erro rcode.  Also set the message pointer
 * with an explanatory message if there is a problem; otherwise, set it
 * to NULL.
 *
 * This routine is where much of the information specific to an MPI 
 * implementation is used.  In particular, the names of the structures
 * internal to an implementation and their fields are used here.  
 *
 * FIXME: some of this information is specific to particular devices.
 * For example, the message queues are defined by the device.  How do
 * we export this information?  Should the queue code itself be responsible
 * for this (either by calling a routine in the image, using 
 * dbgr_find_function (?) or by having the queue implementation provide a
 * separate file that can be included here to get the necessary information.
 */
int mqs_image_has_queues (mqs_image *image, char **message)
{
    mpich_image_info * i_info = 
	(mpich_image_info *)dbgr_get_image_info (image);

    /* Default failure message ! */
    *message = (char *)"The symbols and types in the MPICH library used by TotalView\n"
	"to extract the message queues are not as expected in\n"
	"the image '%s'\n"
	"No message queue display is possible.\n"
	"This is probably an MPICH version or configuration problem.";

    /* Force in the file containing our wait-for-debugger function, to ensure 
     * that types have been read from there before we try to look them up.
     */
    dbgr_find_function (image, (char *)"MPIR_WaitForDebugger", mqs_lang_c, NULL);

    /* Find the various global variables and structure definitions 
       that describe the communicator and message queue structures for
       the MPICH implementation */

    /* First, the communicator information.  This is in two parts:
       MPIR_All_Communicators - a structure containing the head of the
       list of all active communicators.  The type is MPIR_Comm_list.
       The communicators themselves are of type MPID_Comm.
    */
    {
	mqs_type *cl_type = dbgr_find_type( image, (char *)"MPIR_Comm_list", 
					    mqs_lang_c );
	if (cl_type) {
	    i_info->sequence_number_offs = 
		dbgr_field_offset( cl_type, (char *)"sequence_number" );
	    i_info->comm_head_offs = dbgr_field_offset( cl_type, (char *)"head" );
	}
    }
    {
	mqs_type *co_type = dbgr_find_type( image, (char *)"MPID_Comm", mqs_lang_c );
	if (co_type) {
	    i_info->comm_name_offs = dbgr_field_offset( co_type, (char *)"name" );
	    i_info->comm_next_offs = dbgr_field_offset( co_type, (char *)"comm_next" );
	    i_info->comm_rsize_offs = dbgr_field_offset( co_type, (char *)"remote_size" );
	    i_info->comm_rank_offs = dbgr_field_offset( co_type, (char *)"rank" );
	    i_info->comm_context_id_offs = dbgr_field_offset( co_type, (char *)"context_id" );
	    i_info->comm_recvcontext_id_offs = dbgr_field_offset( co_type, (char *)"recvcontext_id" );
	}
    }

    /* Now the receive queues.  The receive queues contain MPID_Request
       objects, and the various fields are within types in that object.
       To simplify the eventual access, we compute all offsets relative to the
       request.  This means diving into the types that make of the 
       request definition */
    {
	mqs_type *req_type = dbgr_find_type( image, (char *)"MPID_Request", mqs_lang_c );
	if (req_type) {
	    int dev_offs;
	    dev_offs = dbgr_field_offset( req_type, (char *)"dev" );
	    i_info->req_status_offs = dbgr_field_offset( req_type, (char *)"status" );
	    i_info->req_cc_offs     = dbgr_field_offset( req_type, (char *)"cc" );
	    if (dev_offs >= 0) {
		mqs_type *dreq_type = dbgr_find_type( image, (char *)"MPIDI_Request", 
						      mqs_lang_c );
		i_info->req_dev_offs = dev_offs;
		if (dreq_type) {
		    int loff, match_offs;
		    loff = dbgr_field_offset( dreq_type, (char *)"next" );
		    i_info->req_next_offs = dev_offs + loff;
		    loff = dbgr_field_offset( dreq_type, (char *)"user_buf" );
		    i_info->req_user_buf_offs = dev_offs + loff;
		    loff = dbgr_field_offset( dreq_type, (char *)"user_count" );
		    i_info->req_user_count_offs = dev_offs + loff;
		    loff = dbgr_field_offset( dreq_type, (char *)"datatype" );
		    i_info->req_datatype_offs = dev_offs + loff;
		    match_offs = dbgr_field_offset( dreq_type, (char *)"match" );
		    if (match_offs >= 0) {
			mqs_type *match_type = dbgr_find_type( image, (char *)"MPIDI_Message_match", mqs_lang_c );
			if (match_type) {
			    int parts_offs = dbgr_field_offset( match_type, (char *)"parts" );
			    if (parts_offs >= 0) {
				mqs_type *parts_type = dbgr_find_type( image, (char *)"MPIDI_Message_match_parts_t", mqs_lang_c );
				if (parts_type) {
				    int moff;
				    moff = dbgr_field_offset( parts_type, (char *)"tag" );
				    i_info->req_tag_offs = dev_offs + match_offs + moff;
				    moff = dbgr_field_offset( parts_type, (char *)"rank" );
				    i_info->req_rank_offs = dev_offs + match_offs + moff;
				    moff = dbgr_field_offset( parts_type, (char *)"context_id" );
				    i_info->req_context_id_offs = dev_offs + match_offs + moff;
				}
			    }
			}
		    }
		}
	    }
	}
    }

    /* Send queues use a separate system */
    {
	mqs_type *sreq_type = dbgr_find_type( image, (char *)"MPIR_Sendq", mqs_lang_c );
	if (sreq_type) {
	    i_info->sendq_next_offs = dbgr_field_offset( sreq_type, (char *)"next" );
	    i_info->sendq_tag_offs  = dbgr_field_offset( sreq_type, (char *)"tag" );
	    i_info->sendq_rank_offs = dbgr_field_offset( sreq_type, (char *)"rank" );
	    i_info->sendq_context_id_offs  = dbgr_field_offset( sreq_type, (char *)"context_id" );
	    i_info->sendq_req_offs  = dbgr_field_offset( sreq_type, (char *)"sreq" );
	}
    }

    return mqs_ok;
}
/* mqs_setup_process initializes the process structure.  
 * The memory allocated by this routine (and routines that modify this
 * structure) is freed with mqs_destroy_process_info 
 */
int mqs_setup_process (mqs_process *process, const mqs_process_callbacks *pcb)
{
    /* Extract the addresses of the global variables we need and save 
       them away */
    mpich_process_info *p_info = 
	(mpich_process_info *)dbgr_malloc (sizeof (mpich_process_info));

    if (p_info) {
	mqs_image        *image;
	mpich_image_info *i_info;
	
	p_info->process_callbacks = pcb;
	
	/* Now we can get the rest of the info ! */
	image  = dbgr_get_image (process);
	i_info = (mpich_image_info *)dbgr_get_image_info (image);
	
	/* Library starts at zero, so this ensures we go look to start with */
	p_info->communicator_sequence = -1;
	/* We have no communicators yet */
	p_info->communicator_list     = NULL;
	/* Ask the debugger to initialize the structure that contains
	   the sizes of basic items (short, int, long, long long, and 
	   void *) */
	dbgr_get_type_sizes (process, &p_info->sizes);
	
	/* Tell the debugger to associate p_info with process */
	dbgr_put_process_info (process, (mqs_process_info *)p_info);
	
	return mqs_ok;
    }
    else
	return err_no_store;
}
int mqs_process_has_queues (mqs_process *proc, char **msg)
{
    mpich_process_info *p_info = 
	(mpich_process_info *)dbgr_get_process_info (proc);
    mqs_image * image          = dbgr_get_image (proc);
    mpich_image_info *i_info   = 
	(mpich_image_info *)dbgr_get_image_info (image);
    mqs_taddr_t head_ptr;

    /* Don't bother with a pop up here, it's unlikely to be helpful */
    *msg = 0;

    /* Check first for the communicator list */
    if (dbgr_find_symbol (image, (char *)"MPIR_All_communicators", &p_info->commlist_base) != mqs_ok)
	return err_all_communicators;

    /* Check for the receive and send queues */
    if (dbgr_find_symbol( image, (char *)"MPID_Recvq_posted_head_ptr", &head_ptr ) != mqs_ok)
	return err_posted;
    p_info->posted_base = fetch_pointer( proc, head_ptr, p_info );

    if (dbgr_find_symbol( image, (char *)"MPID_Recvq_unexpected_head_ptr", &head_ptr ) != mqs_ok)
	return err_unexpected;
    p_info->unexpected_base = fetch_pointer( proc, head_ptr, p_info );

    /* Send queues are optional */
    if (dbgr_find_symbol( image, (char *)"MPIR_Sendq_head", &p_info->sendq_base) == 
	mqs_ok) {
	p_info->has_sendq = 1;
    }
    else {
	p_info->has_sendq = 0;
    }

    return mqs_ok;
}

/* This routine is called by the debugger to map an error code into a 
   printable string */
char * mqs_dll_error_string (int errcode)
{
    switch (errcode) {
    case err_silent_failure:
	return (char *)"";
    case err_no_current_communicator: 
	return (char *)"No current communicator in the communicator iterator";
    case err_bad_request:    
	return (char *)"Attempting to setup to iterate over an unknown queue of operations";
    case err_no_store: 
	return (char *)"Unable to allocate store";
    case err_group_corrupt:
	return (char *)"Could not read a communicator's group from the process (probably a store corruption)";
    case err_unexpected: 
	return (char *)"Failed to find symbol MPID_Recvq_unexpected_head_ptr";
    case err_posted: 
	return (char *)"Failed to find symbol MPID_Recvq_posted_head_ptr";
    }
    return (char *)"Unknown error code";
}
/* ------------------------------------------------------------------------ */
/* Queue Display
 *
 */

/* Communicator list.
 * 
 * To avoid problems that might be caused by having the list of communicators
 * change in the process that is being debugged, the communicator access 
 * routines make an internal copy of the communicator list.  
 * 
 */
/* update_communicator_list makes a copy of the list of currently active
 * communicators and stores it in the mqs_process structure.   
 */
int mqs_update_communicator_list (mqs_process *proc)
{
    if (communicators_changed (proc))
	return rebuild_communicator_list (proc);
    else
	return mqs_ok;
}
/* These three routines (setup_communicator_iterator, get_communicator,
 * and next_communicator) provide a way to access each communicator in the
 * list that is initialized by update_communicator_list.
 */
int mqs_setup_communicator_iterator (mqs_process *proc)
{
    mpich_process_info *p_info = 
	(mpich_process_info *)dbgr_get_process_info (proc);

    /* Start at the front of the list again */
    p_info->current_communicator = p_info->communicator_list;
    /* Reset the operation iterator too */
    p_info->next_msg = 0;
    
    return p_info->current_communicator == NULL ? mqs_end_of_list : mqs_ok;
}
int mqs_get_communicator (mqs_process *proc, mqs_communicator *comm)
{
    mpich_process_info *p_info = 
	(mpich_process_info *)dbgr_get_process_info (proc);

    if (p_info->current_communicator) {
	*comm = p_info->current_communicator->comm_info;
	return mqs_ok;
    }
    else
	return err_no_current_communicator;
}
int mqs_next_communicator (mqs_process *proc)
{
    mpich_process_info *p_info = 
	(mpich_process_info *)dbgr_get_process_info (proc);
    
    p_info->current_communicator = p_info->current_communicator->next;
  
    return (p_info->current_communicator != NULL) ? mqs_ok : mqs_end_of_list;
}
/* ------------------------------------------------------------------------ */
/* Iterate over the queues attached to the current communicator. */

/* Forward references for routines used to implement the operations */
static int fetch_send (mqs_process *proc, mpich_process_info *p_info,
		       mqs_pending_operation *res);
static int fetch_receive (mqs_process *proc, mpich_process_info *p_info,
			  mqs_pending_operation *res, int look_for_user_buffer);

int mqs_setup_operation_iterator (mqs_process *proc, int op)
{
    mpich_process_info *p_info = 
	(mpich_process_info *)dbgr_get_process_info (proc);
    /*    mqs_image * image          = dbgr_get_image (proc); */
/*    mpich_image_info *i_info   = 
      (mpich_image_info *)dbgr_get_image_info (image); */

  p_info->what = (mqs_op_class)op;

  switch (op) {
  case mqs_pending_sends:
      if (!p_info->has_sendq)
	  return mqs_no_information;
      else {
	  p_info->next_msg = p_info->sendq_base;
	  return mqs_ok;
      }

      /* The address on the receive queues is the address of a pointer to 
         the head of the list.  */
  case mqs_pending_receives:
      p_info->next_msg = p_info->posted_base;
      return mqs_ok;
      
  case mqs_unexpected_messages:
      p_info->next_msg = p_info->unexpected_base;
      return mqs_ok;
      
  default:
      return err_bad_request;
  }
}

/* Fetch the next operation on the current communicator, from the 
   selected queue. Since MPICH does not (normally) use separate queues 
   for each communicator, we must compare the queue items with the
   current communicator.
*/
int mqs_next_operation (mqs_process *proc, mqs_pending_operation *op)
{
    mpich_process_info *p_info = 
	(mpich_process_info *)dbgr_get_process_info (proc);

    switch (p_info->what) {
    case mqs_pending_receives:
	return fetch_receive (proc,p_info,op,1);
    case mqs_unexpected_messages:
	return fetch_receive (proc,p_info,op,0);
    case mqs_pending_sends:
	return fetch_send (proc,p_info,op);
    default: return err_bad_request;
    }
} 
/* ------------------------------------------------------------------------ */
/* Clean up routines
 * These routines free any memory allocated when the process or image 
 * structures were allocated.
 */
void mqs_destroy_process_info (mqs_process_info *mp_info)
{
    mpich_process_info *p_info = (mpich_process_info *)mp_info;

    /* Need to handle the communicators and groups too */
    mqs_free_communicator_list( p_info->communicator_list );

    dbgr_free (p_info);
} 

void mqs_destroy_image_info (mqs_image_info *info)
{
    dbgr_free (info);
} 

/* ------------------------------------------------------------------------ */

/* ------------------------------------------------------------------------ */
/* Internal Routine 
 * 
 * These routine know about the internal structure of the MPI implementation.
 */

/* Get the next entry in the current receive queue (posted or unexpected) */

static int fetch_receive (mqs_process *proc, mpich_process_info *p_info,
			  mqs_pending_operation *res, int look_for_user_buffer)
{
    mqs_image * image          = dbgr_get_image (proc);
    mpich_image_info *i_info   = (mpich_image_info *)dbgr_get_image_info (image);
    communicator_t   *comm     = p_info->current_communicator;
    int16_t wanted_context     = comm->recvcontext_id;
    mqs_taddr_t base           = fetch_pointer (proc, p_info->next_msg, p_info);

#ifdef DEBUG_LIST_ITER
    initLogFile();
    fprintf( debugfp, "fetch receive base = %x, comm= %x, context = %d\n", 
	    base, comm, wanted_context );
#endif
    while (base != 0) {
	/* Check this entry to see if the context matches */
	int16_t actual_context = fetch_int16( proc, base + i_info->req_context_id_offs, p_info );

#ifdef DEBUG_LIST_ITER
	initLogFile();
	fprintf( debugfp, "fetch receive msg context = %d\n", actual_context );
#endif
	if (actual_context == wanted_context) {
	    /* Found a request for this communicator */
	    int tag = fetch_int( proc, base + i_info->req_tag_offs, p_info );
	    int rank = fetch_int16( proc, base + i_info->req_rank_offs, p_info );
	    int is_complete   = fetch_int ( proc, base + i_info->req_cc_offs, p_info);
	    mqs_tword_t user_buffer = fetch_pointer( proc,base+i_info->req_user_buf_offs, p_info);
	    int   user_count  = fetch_int( proc,base + i_info->req_user_count_offs, p_info );

	    /* Return -1 for ANY_TAG or ANY_SOURCE */
	    res->desired_tag         = (tag >= 0) ? tag : -1;
	    res->desired_local_rank  = (rank >= 0) ? rank : -1;
	    res->desired_global_rank = -1;   /* Convert to rank in comm world,
						if valid (in mpi-2, may
						not be available) */
	    res->desired_length      = user_count; /* Count, not bytes */
	    
	    res->tag_wild = (tag < 0);
	    res->buffer             = user_buffer;
	    /* We don't know the rest of these */
	    res->system_buffer      = 0;
	    res->actual_local_rank  = rank;
	    res->actual_global_rank = -1;
	    res->actual_tag         = tag;
	    res->actual_length      = -1;
	    res->extra_text[0][0]   = 0;

	    res->status = (is_complete != 0) ? mqs_st_pending : mqs_st_complete; 

	    /* Don't forget to step the queue ! */
	    p_info->next_msg = base + i_info->req_next_offs;
	    return mqs_ok;
	}
	else {
	    /* Try the next one */
	    base = fetch_pointer (proc, base + i_info->req_next_offs, p_info);
	}
    }
#if 0
  while (base != 0)
    { /* Well, there's a queue, at least ! */
      mqs_tword_t actual_context = fetch_int16(proc, base + i_info->context_id_offs, p_info);
      
      if (actual_context == wanted_context)
	{ /* Found a good one */
	  mqs_tword_t tag     = fetch_int (proc, base + i_info->tag_offs, p_info);
	  mqs_tword_t tagmask = fetch_int (proc, base + i_info->tagmask_offs, p_info);
	  mqs_tword_t lsrc    = fetch_int (proc, base + i_info->lsrc_offs, p_info);
	  mqs_tword_t srcmask = fetch_int (proc, base + i_info->srcmask_offs, p_info);
	  mqs_taddr_t ptr     = fetch_pointer (proc, base + i_info->ptr_offs, p_info);
	  
	  /* Fetch the fields from the MPIR_RHANDLE */
	  int is_complete     = fetch_int (proc, ptr + i_info->is_complete_offs, p_info);
	  mqs_taddr_t buf     = fetch_pointer (proc, ptr + i_info->buf_offs, p_info);
	  mqs_tword_t len     = fetch_int (proc, ptr + i_info->len_offs, p_info);
	  mqs_tword_t count   = fetch_int (proc, ptr + i_info->count_offs, p_info);

	  /* If we don't have start, then use buf instead... */
	  mqs_taddr_t start;
	  if (i_info->start_offs < 0)
	    start = buf;
	  else
	    start = fetch_pointer (proc, ptr + i_info->start_offs, p_info);

	  /* Hurrah, we should now be able to fill in all the necessary fields in the
	   * result !
	   */
	  res->status = is_complete ? mqs_st_complete : mqs_st_pending; /* We can't discern matched */
	  if (srcmask == 0)
	    {
	      res->desired_local_rank  = -1;
	      res->desired_global_rank = -1;
	    }
	  else
	    {
	      res->desired_local_rank  = lsrc;
	      res->desired_global_rank = translate (comm->group, lsrc);
	      
	    }
	  res->tag_wild       = (tagmask == 0);
	  res->desired_tag    = tag;
	  
	  if (look_for_user_buffer)
	    {
		res->system_buffer  = 0;
	      res->buffer         = buf;
	      res->desired_length = len;
	    }
	  else
	    {
	      res->system_buffer  = 1;
	      /* Correct an oddity. If the buffer length is zero then no buffer
	       * is allocated, but the descriptor is left with random data.
	       */
	      if (count == 0)
		start = 0;
	      
	      res->buffer         = start;
	      res->desired_length = count;
	    }

	  if (is_complete)
	    { /* Fill in the actual results, rather than what we were looking for */
	      mqs_tword_t mpi_source  = fetch_int (proc, ptr + i_info->MPI_SOURCE_offs, p_info);
	      mqs_tword_t mpi_tag  = fetch_int (proc, ptr + i_info->MPI_TAG_offs, p_info);

	      res->actual_length     = count;
	      res->actual_tag        = mpi_tag;
	      res->actual_local_rank = mpi_source;
	      res->actual_global_rank= translate (comm->group, mpi_source);
	    }

	  /* Don't forget to step the queue ! */
	  p_info->next_msg = base + i_info->next_offs;
	  return mqs_ok;
	}
      else
	{ /* Try the next one */
	  base = fetch_pointer (proc, base + i_info->next_offs, p_info);
	}
    }
#endif  
  p_info->next_msg = 0;
  return mqs_end_of_list;
}  /* fetch_receive */

/* Get the next entry in the send queue, if there is one.  The assumption is 
   that the MPI implementation is quiescent while these queue probes are
   taking place, so we can simply keep track of the location of the "next"
   entry. (in the next_msg field) */
static int fetch_send (mqs_process *proc, mpich_process_info *p_info,
		       mqs_pending_operation *res)
{
    mqs_image * image        = dbgr_get_image (proc);
    mpich_image_info *i_info = (mpich_image_info *)dbgr_get_image_info (image);
    communicator_t   *comm   = p_info->current_communicator;
    int wanted_context       = comm->context_id;
    mqs_taddr_t base         = fetch_pointer (proc, p_info->next_msg, p_info);
    
    if (!p_info->has_sendq)
	return mqs_no_information;
    
#ifdef DEBUG_LIST_ITER
    if (base) {
	initLogFile();
	fprintf( debugf, "comm ptr = %p, comm context = %d\n", 
		 comm, comm->context_id );
    }
#endif
    /* Say what operation it is. We can only see non blocking send operations
     * in MPICH. Other MPI systems may be able to show more here. 
     */
    /* FIXME: handle size properly (declared as 64 in mpi_interface.h) */
    strncpy ((char *)res->extra_text[0],"Non-blocking send",20);
    res->extra_text[1][0] = 0;
    
    while (base != 0) {
	/* Check this entry to see if the context matches */
	int actual_context = fetch_int16( proc, base + i_info->sendq_context_id_offs, p_info );
	
	if (actual_context == wanted_context) {
	    /* Fill in some of the fields */
	    mqs_tword_t target = fetch_int (proc, base+i_info->sendq_rank_offs,      p_info);
	    mqs_tword_t tag    = fetch_int (proc, base+i_info->sendq_tag_offs,         p_info);
	    mqs_tword_t length = 0;
	    mqs_taddr_t data   = 0;
	    mqs_taddr_t sreq = fetch_pointer(proc, base+i_info->sendq_req_offs, p_info );
	    mqs_tword_t is_complete = fetch_int( proc, sreq+i_info->req_cc_offs, p_info );
	    data = fetch_pointer( proc, sreq+i_info->req_user_buf_offs, p_info );
	    length = fetch_int( proc, sreq+i_info->req_user_count_offs, p_info );
	    /* mqs_tword_t complete=0; */

#ifdef DEBUG_LIST_ITER
	    initLogFile();
	    fprintf( debugpf, "sendq entry = %p, rank off = %d, tag off = %d, context = %d\n", 
		    base, i_info->sendq_rank_offs, i_info->sendq_tag_offs, actual_context );
#endif
	    
	    /* Ok, fill in the results */
	    res->status = (is_complete != 0) ? mqs_st_pending : mqs_st_complete; 
	    res->actual_local_rank = res->desired_local_rank = target;
	    res->actual_global_rank= res->desired_global_rank= translate (comm->group, target);
	    res->tag_wild       = 0;
	    res->actual_tag     = res->desired_tag = tag;
	    res->desired_length = res->actual_length = length;
	    res->system_buffer  = 0;
	    res->buffer         = data;
	    
	    
	    /* Don't forget to step the queue ! */
	    p_info->next_msg = base + i_info->sendq_next_offs;
	    return mqs_ok;
	}
	else {
	    /* Try the next one */
	    base = fetch_pointer (proc, base + i_info->sendq_next_offs, p_info);
	}
    }
#if 0
  while (base != 0)
    { /* Well, there's a queue, at least ! */
      /* Check if it's one we're interested in ? */
      mqs_taddr_t commp = fetch_pointer (proc, base+i_info->db_comm_offs, p_info);
      mqs_taddr_t next  = base+i_info->db_next_offs;

      if (commp == comm->comm_info.unique_id)
	{ /* Found one */
	  mqs_tword_t target = fetch_int (proc, base+i_info->db_target_offs,      p_info);
	  mqs_tword_t tag    = fetch_int (proc, base+i_info->db_tag_offs,         p_info);
	  mqs_tword_t length = fetch_int (proc, base+i_info->db_byte_length_offs, p_info);
	  mqs_taddr_t data   = fetch_pointer (proc, base+i_info->db_data_offs,    p_info);
	  mqs_taddr_t shandle= fetch_pointer (proc, base+i_info->db_shandle_offs, p_info);
	  mqs_tword_t complete=fetch_int (proc, shandle+i_info->is_complete_offs, p_info);

	  /* Ok, fill in the results */
	  res->status = complete ? mqs_st_complete : mqs_st_pending; /* We can't discern matched */
	  res->actual_local_rank = res->desired_local_rank = target;
	  res->actual_global_rank= res->desired_global_rank= translate (comm->group, target);
	  res->tag_wild   = 0;
	  res->actual_tag = res->desired_tag = tag;
	  res->desired_length = res->actual_length = length;
	  res->system_buffer  = 0;
	  res->buffer = data;

	  p_info->next_msg = next;
	  return mqs_ok;
	}
      
      base = fetch_pointer (proc, next, p_info);
    }

  p_info->next_msg = 0;
#endif
  return mqs_end_of_list;
} /* fetch_send */

/* ------------------------------------------------------------------------ */
/* Communicator */
static int communicators_changed (mqs_process *proc)
{
    mpich_process_info *p_info = 
	(mpich_process_info *)dbgr_get_process_info (proc);
    mqs_image * image          = dbgr_get_image (proc);
    mpich_image_info *i_info   = 
	(mpich_image_info *)dbgr_get_image_info (image);
    mqs_tword_t new_seq = fetch_int (proc, 
				     p_info->commlist_base+i_info->sequence_number_offs,
				     p_info);
    int  res = (new_seq != p_info->communicator_sequence);
    
    /* Save the sequence number for next time */
    p_info->communicator_sequence = new_seq;
    
    return res;
}

/***********************************************************************
 * Find a matching communicator on our list. We check the recv context
 * as well as the address since the communicator structures may be
 * being re-allocated from a free list, in which case the same
 * address will be re-used a lot, which could confuse us.
 */
static communicator_t * find_communicator ( mpich_process_info *p_info,
					    mqs_taddr_t comm_base, int recv_ctx)
{
  communicator_t * comm = p_info->communicator_list;

  for (; comm; comm=comm->next)
    {
      if (comm->comm_info.unique_id == comm_base &&
	  comm->recvcontext_id == recv_ctx)
	return comm;
    }

  return NULL;
} /* find_communicator */
/* This is the comparison function used in the qsort call in 
   rebuild_communicator_list */
static int compare_comms (const void *a, const void *b)
{
  communicator_t * ca = *(communicator_t **)a;
  communicator_t * cb = *(communicator_t **)b;

  return cb->recvcontext_id - ca->recvcontext_id;
} /* compare_comms */
static int rebuild_communicator_list (mqs_process *proc)
{
    mpich_process_info *p_info = 
	(mpich_process_info *)dbgr_get_process_info (proc);
    mqs_image * image          = dbgr_get_image (proc);
    mpich_image_info *i_info   = 
	(mpich_image_info *)dbgr_get_image_info (image);
    mqs_taddr_t comm_base = fetch_pointer (proc, 
					   p_info->commlist_base+i_info->comm_head_offs,
					   p_info);

    communicator_t **commp;
    int commcount = 0;
    
    /* Iterate over the list in the process comparing with the list
     * we already have saved. This is n**2, because we search for each
     * communicator on the existing list. I don't think it matters, though
     * because there aren't that many communicators to worry about, and
     * we only ever do this if something changed.
     */
    while (comm_base) {
	/* We do have one to look at, so extract the info */
	int recv_ctx = fetch_int16 (proc, comm_base+i_info->comm_recvcontext_id_offs, p_info);
	int send_ctx = fetch_int16 (proc, comm_base+i_info->comm_context_id_offs, p_info);
	communicator_t *old = find_communicator (p_info, comm_base, recv_ctx);

	const char *name = "--unnamed--";
	char namebuffer[64];
	/* In MPICH, the name is preallocated and of size MPI_MAX_OBJECT_NAME */
	if (dbgr_fetch_data( proc, comm_base+i_info->comm_name_offs,64,
			     namebuffer) == mqs_ok && namebuffer[0] != 0) {
	    name = namebuffer;
	}

	if (old) {
	    old->present = 1;		/* We do want this communicator */
	    strncpy (old->comm_info.name, name, sizeof(old->comm_info.name) ); /* Make sure the name is up to date,
						    * it might have changed and we can't tell.
						    */
	}
	else {
	    mqs_taddr_t group_base = fetch_pointer (proc, comm_base+i_info->lrank_to_grank_offs,
					      p_info);
	    int np     = fetch_int (proc, comm_base+i_info->comm_rsize_offs,p_info);
	    group_t *g = find_or_create_group (proc, np, group_base);
	    communicator_t *nc;

#if 0	    
	    if (!g)
		return err_group_corrupt;
#endif

	    nc = (communicator_t *)dbgr_malloc (sizeof (communicator_t));
	    
	    /* Save the results */
	    nc->next = p_info->communicator_list;
	    p_info->communicator_list = nc;
	    nc->present               = 1;
	    nc->group                 = g;
	    nc->context_id            = send_ctx;
	    nc->recvcontext_id        = recv_ctx;
	    
	    strncpy (nc->comm_info.name, name, sizeof( nc->comm_info.name ) );
	    nc->comm_info.unique_id = comm_base;
	    nc->comm_info.size      = np;
	    nc->comm_info.local_rank = fetch_int (proc, comm_base+i_info->comm_rank_offs,p_info);
#ifdef DEBUG_LIST_ITER
	    initLogFile();
	    fprintf( debugfp, "Adding communicator %p, send context=%d, recv context=%d, size=%d, name=%s\n",
		    comm_base, send_ctx, recv_ctx, np, name );
#endif	    
#if 0
	    nc->comm_info.local_rank= reverse_translate (g, dbgr_get_global_rank (proc));
#endif
	}
	/* Step to the next communicator on the list */
	comm_base = fetch_pointer (proc, comm_base+i_info->comm_next_offs, p_info);
    }

    /* Now iterate over the list tidying up any communicators which
     * no longer exist, and cleaning the flags on any which do.
     */
    commp = &p_info->communicator_list;

    for (; *commp; commp = &(*commp)->next) {
	communicator_t *comm = *commp;

	if (comm->present) {
	    comm->present = 0;
	    commcount++;
	}
	else {
	    /* It needs to be deleted */
	    *commp = comm->next;			/* Remove from the list */
	    group_decref (comm->group);		/* Group is no longer referenced from here */
	    dbgr_free (comm);
	}
    }
    
    if (commcount) {
	/* Sort the list so that it is displayed in some semi-sane order. */
	communicator_t ** comm_array = (communicator_t **) dbgr_malloc (
					commcount * sizeof (communicator_t *));
	communicator_t *comm = p_info->communicator_list;
	int i;
	for (i=0; i<commcount; i++, comm=comm->next)
	    comm_array [i] = comm;
	
	/* Do the sort */
	qsort (comm_array, commcount, sizeof (communicator_t *), compare_comms);

	/* Re build the list */
	p_info->communicator_list = NULL;
	for (i=0; i<commcount; i++) {
	    comm = comm_array[i];
	    comm->next = p_info->communicator_list;
	    p_info->communicator_list = comm;
	}

	dbgr_free (comm_array);
    }

    return mqs_ok;
} /* rebuild_communicator_list */

/* Internal routine to free the communicator list */
static void mqs_free_communicator_list( struct communicator_t *comm )
{
    while (comm) {
	communicator_t *next = comm->next;
	
	/* Release the group data structures */
	/* group_decref (comm->group);		 */
	dbgr_free (comm);
      
	comm = next;
    }
}

/* ------------------------------------------------------------------------ */
/* Internal routine to fetch data from the process */
static mqs_taddr_t fetch_pointer (mqs_process * proc, mqs_taddr_t addr, 
				  mpich_process_info *p_info)
{
    int asize = p_info->sizes.pointer_size;
    char data [8];			/* ASSUME a pointer fits in 8 bytes */
    mqs_taddr_t res = 0;

    if (mqs_ok == dbgr_fetch_data (proc, addr, asize, data))
	dbgr_target_to_host (proc, data, 
			     ((char *)&res) + (host_is_big_endian ? sizeof(mqs_taddr_t)-asize : 0), 
			     asize);
    
    return res;
} 
static mqs_tword_t fetch_int (mqs_process * proc, mqs_taddr_t addr, 
			      mpich_process_info *p_info)
{
    int isize = p_info->sizes.int_size;
    char buffer[8];			/* ASSUME an integer fits in 8 bytes */
    mqs_tword_t res = 0;

    if (mqs_ok == dbgr_fetch_data (proc, addr, isize, buffer))
	dbgr_target_to_host (proc, buffer, 
			     ((char *)&res) + (host_is_big_endian ? sizeof(mqs_tword_t)-isize : 0), 
			     isize);
    
    return res;
} 
static mqs_tword_t fetch_int16 (mqs_process * proc, mqs_taddr_t addr, 
				mpich_process_info *p_info)
{
    char buffer[8];			/* ASSUME an integer fits in 8 bytes */
    int16_t res = 0;

    if (mqs_ok == dbgr_fetch_data (proc, addr, 2, buffer))
	dbgr_target_to_host (proc, buffer, 
			     ((char *)&res) + (host_is_big_endian ? sizeof(mqs_tword_t)-2 : 0), 
			     2);
    
    return res;
} 

/* ------------------------------------------------------------------------- */
/* With each communicator we need to translate ranks to/from their
   MPI_COMM_WORLD equivalents.  This code is not yet implemented 
*/
/* ------------------------------------------------------------------------- */
/* idx is rank in group this; return rank in MPI_COMM_WORLD */
static int translate (group_t *this, int idx) 
{ 	
    return -1;
}
#if 0
/* idx is rank in MPI_COMM_WORLD, return rank in group this */
static int reverse_translate (group_t * this, int idx) 
{ 	
    return -1;
}
#endif
static group_t * find_or_create_group (mqs_process *proc,
				       mqs_tword_t np,
				       mqs_taddr_t table)
{
    return 0;
}
static void group_decref (group_t * group)
{
    if (--(group->ref_count) == 0) {
	dbgr_free (group->local_to_global);
	dbgr_free (group);
    }
} /* group_decref */
