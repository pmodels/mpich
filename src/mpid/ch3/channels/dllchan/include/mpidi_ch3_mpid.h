/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#if !defined(MPICH_MPIDI_CH3_MPID_H_INCLUDED)
#define MPICH_MPIDI_CH3_MPID_H_INCLUDED
#define HAVE_CH3_PRELOAD

/* Define the ABI version for the channel interface.  This will
   be checked with the version in the dynamically loaded library */
#define MPICH_CH3ABIVERSION "1.1"

/* The void * argument in iSend and iStartMsg is really a pointer to 
   a message packet. However, to simplify the definition of "private" 
   packets, there is no universal packet type (there is the 
   MPIDI_CH3_Pkt_t, but the "extension" packets are not of this type).
   Using a void * pointer avoids unnecessary complaints from the
   compilers 

   In addition to the obvious functions, in an environment where the 
   dynamic library is loaded into a static executable, it may not
   be possible to link with routines that are in the executable. 
   Instead, the dynamically loaded library will need to use a function
   table, and that table must be provided by the program that loads 
   this library.  There are two categories of functions: Ones that 
   are predefined and used by many channels (e.g., some of the PMI functions)
   and ones that are specific to one channel.  As much as possible, the
   specific functions should be linked directly into the DLL for that channel,
   but where this is not possible, we need a way to extend the function name
   support.  Here is a possible extension:

   ExportFnTable( MPIDI_CH3DLL_FnTable * ) - The calling program initializes
   the FnTable with pointers to the functions and invokes this routine that
   is loaded from the DLL.  The FnTable is defined as part of the DLL version.
   One of the functions in that table is

       ImportFn( const char *name, (void)(*fn) )

   that can be used by the DLL to attempt to import another function with
   the given name.

   Finally, we don't always want to bother with this - in many environments,
   it isn't necessary to go to these lengths.  The dllchan code (which 
   will be in the calling program, will check to see if the ExportFnTable
   function exists.  If not, then this step (of setting up the export tables)
   is skipped.  

   A channel will enable this option by setting

   USE_EXPORT_FN_TABLE

   All calls to routines that might be in that table are made through
   the function call macro 

        MPIU_EXP_CALL(MPIDI_CH3EXP,fn(...))

   We use a different name from MPIU_CALL (used in mpid/ch3/src) to 
   allow us to support both styles with one source code; the "EXP"
   refers to "exported".  
*/
typedef struct MPIDI_CH3_Funcs {
    int (*Init)( int, MPIDI_PG_t *, int );
    int (*InitCompleted)(void);
    int (*Finalize)(void);
    int (*iSend)( MPIDI_VC_t *, MPID_Request *, void *, int );
    int (*iSendv)( MPIDI_VC_t *, MPID_Request *, MPID_IOV *, int );
    int (*iStartMsg)( MPIDI_VC_t *, void *, int, MPID_Request ** );
    int (*iStartMsgv)( MPIDI_VC_t *, MPID_IOV *, int, MPID_Request ** );
    int (*Progress)( int, MPID_Progress_state * );
    /*    int (*Progress_test)( void );
	  int (*Progress_wait)( MPID_Progress_state * ); */

    int (*VC_Init)( struct MPIDI_VC * );
    int (*VC_Destroy)( struct MPIDI_VC * );
    int (*PG_Init)( struct MPIDI_PG * );
    int (*PG_Destroy)( struct MPIDI_PG * );
    int (*Connection_terminate)( MPIDI_VC_t * );
    int (*RMAFnsInit)( struct MPIDI_RMA_Ops * );

    /* These are used in support of dynamic process features */
    int (*PortFnsInit)( MPIDI_PortFns * );
    int (*Connect_to_root)( const char *, MPIDI_VC_t ** );
    int (*Get_business_card)( int, char *, int );

    /* VC_GetStateString is only used for debugging, and may be a no-op */
    const char *(*VC_GetStateString)( struct MPIDI_VC * );
} MPIDI_CH3_Funcs;

/* This is a shared structure that defines the CH3 functions.  It is 
   defined to work with the MPIU_CALL macro */
extern struct MPIDI_CH3_Funcs MPIU_CALL_MPIDI_CH3;

#endif /* MPICH_MPIDI_CH3_MPID_H_INCLUDED */
