/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "trmem.h"

#include <stdio.h>
#include <string.h>

#if defined(MPICH_DEBUG_MEMINIT)
#  if defined(HAVE_VALGRIND_H) && defined(HAVE_MEMCHECK_H)
#    include <valgrind.h>
#    include <memcheck.h>
#    define USE_VALGRIND_MACROS 1
#  elif defined(HAVE_VALGRIND_VALGRIND_H) && defined(HAVE_VALGRIND_MEMCHECK_H)
#    include <valgrind/valgrind.h>
#    include <valgrind/memcheck.h>
#    define USE_VALGRIND_MACROS 1
#  endif
#endif

#ifndef USE_MEMORY_TRACING
int MPIU_trvalid( const char str[] );
#endif
int MPIU_trvalid_unsafe(const char str[]);

/* Temporary.  sig values will change */
/* style: allow:malloc:3 sig:0 */
/* style: allow:calloc:2 sig:0 */
/* style: allow:free:3 sig:0 */
/* style: allow:strdup:1 sig:0 */
/* style: define:malloc:1 sig:0 */
/* style: define:__strdup:1 sig:0 */
/* style: define:FPRINTF:1 sig:0 */

#ifdef malloc
/* Undefine these in case they were set to 'error' */
#undef malloc
#undef calloc
#undef free
#undef strdup
/* Some GNU implementations use __strdup for strdup */
#if defined(__strdup)
#define strdup(s) __strdup(s)
#endif
#endif

#if defined(HAVE_STDLIB_H) || defined(STDC_HEADERS)
#include <stdlib.h>
#else
/* We should test to see if these will work */
extern void *malloc( size_t );
extern void *calloc( size_t, size_t );
extern int free( void *);
#endif

#define FPRINTF fprintf

/*D
    MPIU_trspace - Routines for tracing space usage

    Description:
    MPIU_trmalloc replaces malloc and MPIU_trfree replaces free.  
    These routines
    have the same syntax and semantics as the routines that they replace,
    In addition, there are routines to report statistics on the memory
    usage, and to report the currently allocated space.  These routines
    are built on top of malloc and free, and can be used together with
    them as long as any space allocated with MPIU_trmalloc is only freed with
    MPIU_trfree.

    Note that the malloced data is scrubbed each time; you don't get
    random trash (or fortuitous zeros).  What you get is fc (bytes);
    this will usually create a "bad" value.

    As an aid in developing codes, a maximum memory threshold can 
    be set with MPIU_TrSetMaxMem.
 D*/

/* HEADER_DOUBLES is the number of doubles in a trSPACE header */
/* We have to be careful about alignment rules here */
#define USE_LONG_NAMES_IN_TRMEM
#ifdef USE_LONG_NAMES_IN_TRMEM
/* Assume worst case and align on 8 bytes */
#define TR_ALIGN_BYTES 8
#define TR_ALIGN_MASK  0x7
#define TR_FNAME_LEN   48
#define HEADER_DOUBLES 19
#else
#if SIZEOF_VOID_P > 4
#define TR_ALIGN_BYTES 8
#define TR_ALIGN_MASK  0x7
#define TR_FNAME_LEN   16
#define HEADER_DOUBLES 12
#else
#define TR_ALIGN_BYTES 4
#define TR_ALIGN_MASK  0x3
#define TR_FNAME_LEN   12
#define HEADER_DOUBLES 8
#endif
#endif

#define COOKIE_VALUE   0xf0e0d0c9
#define ALREADY_FREED  0x0f0e0d9c

typedef struct TRSPACE {
    unsigned long   size;
    int             id;
    int             lineno;
    int             freed_lineno;
    char            freed_fname[TR_FNAME_LEN];
    char            fname[TR_FNAME_LEN];
    struct TRSPACE * volatile next, *prev;
    unsigned long   cookie;        /* Cookie is always the last element 
				      inorder to catch the off-by-one
				      errors */
    } TRSPACE;
/* This union is used to ensure that the block passed to the user is
   aligned on a double boundary */
typedef union TrSPACE {
    TRSPACE sp;
    double  v[HEADER_DOUBLES];
    } TrSPACE;

/*
 * This package maintains some state about itself.  These globals hold
 * this information.
 */
static int     world_rank = -1;
static volatile long allocated = 0, frags = 0;
static TRSPACE * volatile TRhead = 0;
static volatile int     TRid = 0;
static volatile int     TRidSet = 0;
static volatile int     TRlevel = 0;
static unsigned char  TRDefaultByte = 0xda;
static unsigned char  TRFreedByte = 0xfc;
#define MAX_TR_STACK 20
static int     TRdebugLevel = 0;
#define TR_MALLOC 0x1
#define TR_FREE   0x2

/* Used to keep track of allocations */
static volatile long    TRMaxMem = 0;
static volatile long    TRMaxMemId = 0;
/* Used to limit allocation */
static volatile long    TRMaxMemAllow = 0;

/*
 * Printing of addresses.  
 *
 * This is particularly difficult because there isn't a C integer type that is
 * known in advance to be the same size as an address, so we need some
 * way to convert a pointer into the characters the represent the address in 
 * hex.  We can't simply use %x or %lx since the size of an address might not
 * be an int or a long (e.g., it might be a long long).
 *
 * In order to handle this, we have our own routine to convert
 * an address to hex digits.  For thread safety, the character
 * string is allocated within the calling routine (rather than returning
 * a static string from the conversion routine).
 */

/* 8 bytes = 16 hex chars + 0x (2 chars) + the null is 19 */
#define MAX_ADDRESS_CHARS 19

static void addrToHex( void *addr, char string[MAX_ADDRESS_CHARS] );

/*+C
   MPIU_trinit - Setup the space package.  Only needed for 
   error messages and flags.
+*/
void MPIU_trinit( int rank )
{
    char *s;

    world_rank = rank;

    /* FIXME: We should use generalized parameter handling here
       to allow use of the command line as well as environment
       variables */
    s = getenv( "MPICH_TRMEM_VALIDATE" );
    if (s && *s && (strcmp(s,"YES") == 0 || strcmp(s,"yes") == 0)) {
	TRdebugLevel = 1;
    }
    s = getenv( "MPICH_TRMEM_INITZERO" );
    if (s && *s && (strcmp(s,"YES") == 0 || strcmp(s,"yes") == 0)) {
	TRDefaultByte = 0;
	TRFreedByte = 0;
    }
    
}
 
/*+C
    MPIU_trmalloc - Malloc with tracing

    Input Parameters:
+   a   - number of bytes to allocate
.   lineno - line number where used.  Use __LINE__ for this
-   fname  - file name where used.  Use __FILE__ for this

    Returns:
    double aligned pointer to requested storage, or null if not
    available.
 +*/
void *MPIU_trmalloc( unsigned int a, int lineno, const char fname[] )
{
    TRSPACE          *head;
    char             *new = NULL;
    unsigned long    *nend;
    unsigned int     nsize;
    int              l;

    /* Synchronization is needed because the TRhead and associated
     * structures are global and they are modified by this function. */
    MPIU_THREAD_CS_ENTER(MEMALLOC,);

    if (TRdebugLevel > 0) {
	char buf[256];
	MPIU_Snprintf( buf, 256, 
		       "Invalid MALLOC arena detected at line %d in %s\n", 
		       lineno, fname );
	if (MPIU_trvalid_unsafe( buf )) goto fn_exit;
    }

    nsize = a;
    if (nsize & TR_ALIGN_MASK) 
	nsize += (TR_ALIGN_BYTES - (nsize & TR_ALIGN_MASK));
    if ((allocated + (long)nsize > TRMaxMemAllow) && TRMaxMemAllow) {
	/* Return a null when memory would be exhausted */
	/* This is only called when additional debugging is enabled,
	   so the fact that this does not go through the regular error 
	   message system is not a problem. */
	MPIU_Error_printf( "Exceeded allowed memory! \n" );
        goto fn_exit;
    }

    new = malloc( (unsigned)( nsize + sizeof(TrSPACE) + sizeof(unsigned long) ) );
    if (!new) goto fn_exit;

    memset( new, TRDefaultByte, 
	    nsize + sizeof(TrSPACE) + sizeof(unsigned long) );
    head = (TRSPACE *)new;
    new  += sizeof(TrSPACE);

    if (TRhead) {
#if defined(USE_VALGRIND_MACROS)
        VALGRIND_MAKE_MEM_DEFINED(&TRhead->prev, sizeof(TRhead->prev)); 
	TRhead->prev = head;
        VALGRIND_MAKE_MEM_NOACCESS(&TRhead->prev, sizeof(TRhead->prev)); 
#else
	TRhead->prev = head;
#endif
    }
    head->next     = TRhead;
    TRhead         = head;
    head->prev     = 0;
    head->size     = nsize;
    head->id       = TRid;
    head->lineno   = lineno;
    if ((l = (int)strlen( fname )) > TR_FNAME_LEN-1 ) 
	fname += (l - (TR_FNAME_LEN-1));
    MPIU_Strncpy( head->fname, fname, TR_FNAME_LEN );
    head->fname[TR_FNAME_LEN-1]= 0;
    head->cookie   = COOKIE_VALUE;
    nend           = (unsigned long *)(new + nsize);
    nend[0]        = COOKIE_VALUE;

    allocated += nsize;
    if (allocated > TRMaxMem) {
	TRMaxMem   = allocated;
	TRMaxMemId = TRid;
    }
    frags     ++;

    if (TRlevel & TR_MALLOC) {
	/* Note that %08p (what we'd like to use) isn't accepted by
	   all compilers */
	MPIU_Error_printf( "[%d] Allocating %d bytes at %8p in %s:%d\n", 
		     world_rank, a, new, fname, lineno );
    }

#if defined(USE_VALGRIND_MACROS)
    /* Without these macros valgrind actually catches far fewer errors when
       using --enable-g=mem. Note that it would be nice to use
       VALGRIND_MALLOCLIKE_BLOCK and friends, but they don't work when the
       underlying source of the memory is malloc/free. */
    VALGRIND_MAKE_MEM_UNDEFINED(new, nsize); 
    VALGRIND_MAKE_MEM_NOACCESS(head, sizeof(TrSPACE)); 
    VALGRIND_MAKE_MEM_NOACCESS(nend, sizeof(unsigned long)); 
#endif
fn_exit:
    MPIU_THREAD_CS_EXIT(MEMALLOC,);
    return (void *)new;
}

/*+C
   MPIU_trfree - Free with tracing

   Input Parameters:
+  a    - pointer to a block allocated with trmalloc
.  line - line in file where called
-  file - Name of file where called
 +*/
void MPIU_trfree( void *a_ptr, int line, const char file[] )
{
    TRSPACE  *head;
    char     *ahead;
    char     *a = (char *)a_ptr;
    unsigned long *nend;
    int      l, nset;
    char     hexstring[MAX_ADDRESS_CHARS];

/* Don't try to handle empty blocks */
    if (!a) return;

    /* Synchronization is needed because the TRhead and associated
     * structures are global and they are modified by this function. */
    MPIU_THREAD_CS_ENTER(MEMALLOC,);

    if (TRdebugLevel > 0) {
	if (MPIU_trvalid_unsafe( "Invalid MALLOC arena detected by FREE" )) goto fn_exit;
    }

    ahead = a;
    a     = a - sizeof(TrSPACE);
    head  = (TRSPACE *)a;

#if defined(USE_VALGRIND_MACROS)
    /* We need to mark the memory as defined before performing our own error
       checks or valgrind will flag the trfree function as erroneous.  The real
       free() at the end of this function will mark the whole block as NOACCESS
       again.  See the corresponding block in the trmalloc function for more
       info. */
    VALGRIND_MAKE_MEM_DEFINED(a, sizeof(TrSPACE));
#endif

    if (head->cookie != COOKIE_VALUE) {
	/* Damaged header */
	/* Note that %08p (what we'd like to use) isn't accepted by
	   all compilers */
	MPIU_Error_printf( 
		     "[%d] Block at address %8p is corrupted; cannot free;\n\
may be block not allocated with MPIU_trmalloc or MALLOC\n\
called in %s at line %d\n", world_rank, a, file, line );
	goto fn_exit;
    }
    nend = (unsigned long *)(ahead + head->size);
/* Check that nend is properly aligned */
    if ((sizeof(long) == 4 && ((long)nend & 0x3) != 0) ||
	(sizeof(long) == 8 && ((long)nend & 0x7) != 0)) {
	MPIU_Error_printf(
 "[%d] Block at address %lx is corrupted (invalid address or header)\n\
called in %s at line %d\n", world_rank, (long)a + sizeof(TrSPACE), 
		 file, line );
	goto fn_exit;
    }

#if defined(USE_VALGRIND_MACROS)
    VALGRIND_MAKE_MEM_DEFINED(nend, sizeof(*nend)); 
#endif
    if (*nend != COOKIE_VALUE) {
	if (*nend == ALREADY_FREED) {
	    addrToHex( (char *)a + sizeof(TrSPACE), hexstring );
	    if (TRidSet) {
		MPIU_Error_printf( 
		     "[%d] Block [id=%d(%lu)] at address %s was already freed\n", 
		     world_rank, head->id, head->size, hexstring );
	    }
	    else {
		MPIU_Error_printf( 
		     "[%d] Block at address %s was already freed\n", 
		     world_rank, hexstring );
	    }
	    head->fname[TR_FNAME_LEN-1]	  = 0;  /* Just in case */
	    head->freed_fname[TR_FNAME_LEN-1] = 0;  /* Just in case */
	    MPIU_Error_printf(
		     "[%d] Block freed in %s[%d]\n", world_rank, 
		     head->freed_fname, head->freed_lineno );
	    MPIU_Error_printf(
		     "[%d] Block allocated at %s[%d]\n", 
		     world_rank, head->fname, head->lineno );
	    goto fn_exit;
	}
	else {
	    /* Damaged tail */
	    addrToHex( a, hexstring );
	    if (TRidSet) {
		MPIU_Error_printf(
		     "[%d] Block [id=%d(%lu)] at address %s is corrupted (probably write past end)\n", 
		     world_rank, head->id, head->size, hexstring );
	    }
	    else {
		MPIU_Error_printf(
		     "[%d] Block at address %s is corrupted (probably write past end)\n", 
		     world_rank, hexstring );
	    }
	    head->fname[TR_FNAME_LEN-1]= 0;  /* Just in case */
	    MPIU_Error_printf(
		     "[%d] Block allocated in %s[%d]\n", world_rank, 
		     head->fname, head->lineno );
	}
    }
/* Mark the location freed */
    *nend		   = ALREADY_FREED;
    head->freed_lineno = line;
    if ((l = (int)strlen( file )) > TR_FNAME_LEN-1 ) 
	file += (l - (TR_FNAME_LEN-1));
    MPIU_Strncpy( head->freed_fname, file, TR_FNAME_LEN );

    allocated -= head->size;
    frags     --;
    if (head->prev) {
#if defined(USE_VALGRIND_MACROS)
        VALGRIND_MAKE_MEM_DEFINED(&head->prev->next, sizeof(head->prev->next)); 
	head->prev->next = head->next;
        VALGRIND_MAKE_MEM_NOACCESS(&head->prev->next, sizeof(head->prev->next)); 
#else
	head->prev->next = head->next;
#endif
    }
    else {
	TRhead = head->next;
    }

    if (head->next) {
#if defined(USE_VALGRIND_MACROS)
        VALGRIND_MAKE_MEM_DEFINED(&head->next->prev, sizeof(head->next->prev)); 
	head->next->prev = head->prev;
        VALGRIND_MAKE_MEM_NOACCESS(&head->next->prev, sizeof(head->next->prev)); 
#else
	head->next->prev = head->prev;
#endif
    }

    if (TRlevel & TR_FREE) {
	addrToHex( (char *)a + sizeof(TrSPACE), hexstring );
	MPIU_Error_printf( "[%d] Freeing %lu bytes at %s in %s:%d\n", 
		     world_rank, head->size, hexstring, file, line );
    }
    
    /* 
       Now, scrub the data (except possibly the first few ints) to
       help catch access to already freed data 
     */
    /* FIXME why do we skip the first few ints? [goodell@] */
    nset = head->size -  2 * sizeof(int);
    if (nset > 0)  {
#if defined(USE_VALGRIND_MACROS)
        /* If an upper layer (like the handle allocation code) ever used the
           VALGRIND_MAKE_MEM_NOACCESS macro on part/all of the data we gave
           them then our memset will elicit "invalid write" errors from
           valgrind.  Mark it as accessible but undefined here to prevent this. */
        VALGRIND_MAKE_MEM_UNDEFINED(ahead + 2 * sizeof(int), nset); 
#endif
	memset( ahead + 2 * sizeof(int), TRFreedByte, nset );
    }
    free( a );
fn_exit:
    MPIU_THREAD_CS_EXIT(MEMALLOC,);
}

/* The real implementation of MPIU_trvalid but does not acquire any
 * critical sections.  For use by functions that already hold the
 * MEMALLOC critical section. */
int MPIU_trvalid_unsafe(const char str[])
{
    TRSPACE *head;
    char    *a;
    unsigned long *nend;
    int     errs = 0;
    char    hexstring[MAX_ADDRESS_CHARS];

    head = TRhead;
    while (head) {
#if defined(USE_VALGRIND_MACROS)
        /* mark defined before accessing head contents */
        VALGRIND_MAKE_MEM_DEFINED(head, sizeof(*head));
#endif
	if (head->cookie != COOKIE_VALUE) {
	    if (!errs) MPIU_Error_printf( "%s\n", str );
	    errs++;
	    addrToHex( head, hexstring );
	    MPIU_Error_printf( "[%d] Block at address %s is corrupted (invalid cookie in head)\n", 
			 world_rank, hexstring );
#if defined(USE_VALGRIND_MACROS)
            VALGRIND_MAKE_MEM_NOACCESS(head, sizeof(*head));
#endif
	    /* Must stop because if head is invalid, then the data in the
	       head is probably also invalid, and using could lead to 
	       SEGV or BUS  */
	    goto fn_exit;
	}
	a    = (char *)(((TrSPACE*)head) + 1);
	nend = (unsigned long *)(a + head->size);
#if defined(USE_VALGRIND_MACROS)
        /* mark defined before accessing nend contents */
        VALGRIND_MAKE_MEM_DEFINED(nend, sizeof(*nend));
#endif
	if (nend[0] != COOKIE_VALUE) {
	    if (!errs) MPIU_Error_printf( "%s\n", str );
	    errs++;
	    head->fname[TR_FNAME_LEN-1]= 0;  /* Just in case */
	    addrToHex( a, hexstring );
	    if (TRidSet) {
		MPIU_Error_printf( 
"[%d] Block [id=%d(%lu)] at address %s is corrupted (probably write past end)\n", 
			 world_rank, head->id, head->size, hexstring );
	    }
	    else {
		MPIU_Error_printf( 
"[%d] Block at address %s is corrupted (probably write past end)\n", 
			 world_rank, hexstring );
	    }
	    MPIU_Error_printf(
			 "[%d] Block allocated in %s[%d]\n", 
			 world_rank, head->fname, head->lineno );
	}
#if defined(USE_VALGRIND_MACROS)
        /* set both regions back to NOACCESS */
        VALGRIND_MAKE_MEM_NOACCESS(head, sizeof(*head));
        VALGRIND_MAKE_MEM_NOACCESS(nend, sizeof(*nend));
#endif
	head = head->next;
    }
fn_exit:
    return errs;
}

/*+C
   MPIU_trvalid - test the allocated blocks for validity.  This can be used to
   check for memory overwrites.

   Input Parameter:
.  str - string to write out only if an error is detected.

   Return value:
   The number of errors detected.
   
   Output Effect:
   Error messages are written to stdout.  These have the form of either

$   Block [id=%d(%d)] at address %lx is corrupted (probably write past end)
$   Block allocated in <filename>[<linenumber>]

   if the sentinal at the end of the block has been corrupted, and

$   Block at address %lx is corrupted

   if the sentinal at the begining of the block has been corrupted.

   The address is the actual address of the block.  The id is the
   value of TRID.

   No output is generated if there are no problems detected.
+*/
int MPIU_trvalid( const char str[] )
{
    int retval;
    MPIU_THREAD_CS_ENTER(MEMALLOC,);
    retval = MPIU_trvalid_unsafe(str);
    MPIU_THREAD_CS_EXIT(MEMALLOC,);
    return retval;
}

/*+C
   MPIU_trspace - Return space statistics
   
   Output parameters:
.   space - number of bytes currently allocated
.   frags - number of blocks currently allocated
 +*/
void MPIU_trspace( int *space, int *fr )
{
    MPIU_THREAD_CS_ENTER(MEMALLOC,);

    /* We use ints because systems without prototypes will usually
       allow calls with ints instead of longs, leading to unexpected
       behavior */
    *space = (int)allocated;
    *fr    = (int)frags;

    MPIU_THREAD_CS_EXIT(MEMALLOC,);
}

/*+C
  MPIU_trdump - Dump the allocated memory blocks to a file

  Input Parameter:
+  fp  - file pointer.  If fp is NULL, stderr is assumed.
-  minid - Only print allocated memory blocks whose id is at least 'minid'

 +*/
void MPIU_trdump( FILE *fp, int minid )
{
    TRSPACE *head;
    char    hexstring[MAX_ADDRESS_CHARS];

    /* Synchronization is needed because the TRhead and associated
     * structures are global and they are modified by this function. */
    MPIU_THREAD_CS_ENTER(MEMALLOC,);

    if (fp == 0) fp = stderr;
    head = TRhead;
    while (head) {
	if (head->id >= minid) {
	    addrToHex( (char *)head + sizeof(TrSPACE), hexstring );
	    FPRINTF( fp, "[%d] %lu at [%s], ", 
		 world_rank, head->size, hexstring );
	    head->fname[TR_FNAME_LEN-1] = 0; /* Be extra careful */
	    if (TRidSet) {
		/* For head->id >= 0, we can add code to map the id to
		   the name of a package, rather than using a raw number */
		FPRINTF( fp, "id = %d %s[%d]\n", 
			 head->id, head->fname, head->lineno );
	    }
	    else {
		FPRINTF( fp, "%s[%d]\n", head->fname, head->lineno );
	    }
	}
	head = head->next;
    }
/*
    msg_fprintf( fp, "# [%d] The maximum space allocated was %ld bytes [%ld]\n", 
	     world_rank, TRMaxMem, TRMaxMemId );
 */

    MPIU_THREAD_CS_EXIT(MEMALLOC,);
}

/* Configure will set HAVE_SEARCH for these systems.  We assume that
   the system does NOT have search.h unless otherwise noted.
   The otherwise noted lets the non-configure approach work on our
   two major systems */
#if defined(HAVE_SEARCH_H)

/* The following routine uses the tsearch routines to summarize the
   memory heap by id */

#include <search.h>
typedef struct TRINFO { int id, size, lineno; char *fname; } TRINFO;

static int IntCompare( TRINFO *a, TRINFO *b )
{
    return a->id - b->id;
}

static volatile FILE *TRFP = 0;
/*ARGSUSED*/
static void PrintSum( TRINFO **a, VISIT order, int level )
{ 
    if (order == postorder || order == leaf) 
	FPRINTF( TRFP, "[%d]%s[%d] has %d\n", 
		 (*a)->id, (*a)->fname, (*a)->lineno, (*a)->size );
}

/*+C
  MPIU_trSummary - Summarize the allocate memory blocks by id

  Input Parameter:
+  fp  - file pointer.  If fp is NULL, stderr is assumed.
-  minid - Only print allocated memory blocks whose id is at least 'minid'

  Note:
  This routine is the same as MPIU_trDump on those systems that do not include
  /usr/include/search.h .
 +*/
void MPIU_trSummary( FILE *fp, int minid )
{
    TRSPACE *head;
    TRINFO  *root, *key, **fnd;
    TRINFO  nspace[1000];

    MPIU_THREAD_CS_ENTER(MEMALLOC,);

    if (fp == 0) fp = stderr;
    root = 0;
    head = TRhead;
    key  = nspace;
    while (head) {
	if (head->id >= minid) {
	    key->id     = head->id;
	    key->size   = 0;
	    key->lineno = head->lineno;
	    key->fname  = head->fname;
#if defined(USE_TSEARCH_WITH_CHARP)
	    fnd    = (TRINFO **)tsearch( (char *) key, (char **) &root, 
					 IntCompare );
#else
	    fnd    = (TRINFO **)tsearch( (void *) key, (void **) &root, 
					 (int (*)())IntCompare );
#endif
	    if (*fnd == key) {
		key->size = 0;
		key++;
	    }
	    (*fnd)->size += head->size;
	}
	head = head->next;
    }
    
    /* Print the data */
    TRFP = fp;
    twalk( (char *)root, (void (*)())PrintSum );
    /*
      msg_fprintf( fp, "# [%d] The maximum space allocated was %d bytes [%d]\n", 
      world_rank, TRMaxMem, TRMaxMemId );
    */
    MPIU_THREAD_CS_EXIT(MEMALLOC,);
}
#else
void MPIU_trSummary( FILE *fp, int minid )
{
    MPIU_THREAD_CS_ENTER(MEMALLOC,);
    if (fp == 0) fp = stderr;
    FPRINTF( fp, 
		 "# [%d] The maximum space allocated was %ld bytes [%ld]\n", 
		 world_rank, TRMaxMem, TRMaxMemId );
    MPIU_THREAD_CS_EXIT(MEMALLOC,);
}	
#endif

/*+
  MPIU_trid - set an "id" field to be used with each fragment
 +*/
void MPIU_trid( int id )
{
    /* Synchronization is needed because the TRhead and associated
     * structures are global and they are modified by this function. */
    MPIU_THREAD_CS_ENTER(MEMALLOC,);
    TRid    = id;
    TRidSet = 1;   /* Recall whether we ever use this feature to 
		      help clean up output */
    MPIU_THREAD_CS_EXIT(MEMALLOC,);
}

/*+C
  MPIU_trlevel - Set the level of output to be used by the tracing routines
 
  Input Parameters:
. level = 0 - notracing
. level = 1 - trace mallocs
. level = 2 - trace frees

  Note:
  You can add levels together to get combined tracing.
 +*/
void MPIU_trlevel( int level )
{
    MPIU_THREAD_CS_ENTER(MEMALLOC,);
    TRlevel = level;
    MPIU_THREAD_CS_EXIT(MEMALLOC,);
}


/*+C
    MPIU_trDebugLevel - set the level of debugging for the space management
    routines

    Input Parameter:
.   level - level of debugging.  Currently, either 0 (no checking) or 1
    (use MPIU_trvalid at each MPIU_trmalloc or MPIU_trfree).
+*/
void MPIU_trDebugLevel( int level )
{
    MPIU_THREAD_CS_ENTER(MEMALLOC,);
    TRdebugLevel = level;
    MPIU_THREAD_CS_EXIT(MEMALLOC,);
}

/*+C
    MPIU_trcalloc - Calloc with tracing

    Input Parameters:
.   nelem  - number of elements to allocate
.   elsize - size of each element
.   lineno - line number where used.  Use __LINE__ for this
.   fname  - file name where used.  Use __FILE__ for this

    Returns:
    Double aligned pointer to requested storage, or null if not
    available.
 +*/
void *MPIU_trcalloc( unsigned int nelem, unsigned int elsize, int lineno, 
		     const char fname[] )
{
    void *p;
    
    p = MPIU_trmalloc( (unsigned)(nelem*elsize), lineno, fname );
    if (p) {
	memset(p,0,nelem*elsize);
    }
    return p;
}

/*+C
    MPIU_trrealloc - Realloc with tracing

    Input Parameters:
.   p      - pointer to old storage
.   size   - number of bytes to allocate
.   lineno - line number where used.  Use __LINE__ for this
.   fname  - file name where used.  Use __FILE__ for this

    Returns:
    Double aligned pointer to requested storage, or null if not
    available.  This implementation ALWAYS allocates new space and copies 
    the contents into the new space.
 +*/
void *MPIU_trrealloc( void *p, int size, int lineno, const char fname[] )
{
    void    *pnew;
    char    *pa;
    int     nsize;
    TRSPACE *head=0;
    char    hexstring[MAX_ADDRESS_CHARS];

/* We should really use the size of the old block... */
    if (p) {
	pa   = (char *)p;
	head = (TRSPACE *)(pa - sizeof(TrSPACE));
#if defined(USE_VALGRIND_MACROS)
        VALGRIND_MAKE_MEM_DEFINED(head, sizeof(*head)); /* mark defined before accessing contents */
#endif
	if (head->cookie != COOKIE_VALUE) {
	    /* Damaged header */
	    addrToHex( pa, hexstring );
	    MPIU_Error_printf( 
		"[%d] Block at address %s is corrupted; cannot realloc;\n\
may be block not allocated with MPIU_trmalloc or MALLOC\n", 
		world_rank, hexstring );
	    return 0;
	}
    }
    pnew = MPIU_trmalloc( (unsigned)size, lineno, fname );

#if defined(USE_VALGRIND_MACROS)
    /* If trmalloc failed, p should be left alone and will not be freed.  So we
       need to mark the head as NOACCESS before returning. */
    if (!pnew)
        VALGRIND_MAKE_MEM_NOACCESS(head, sizeof(*head));
#endif

    if (p && pnew) {
	nsize = size;
	if (head->size < (unsigned long)nsize) nsize = (int)(head->size);
	MPIU_Memcpy( pnew, p, nsize );
	MPIU_trfree( p, lineno, fname );
    }

    /* If the MPIU_trmalloc failed above pnew will be NULL, just like a
       regular realloc failure. */
    return pnew;
}

/*+C
    MPIU_trstrdup - Strdup with tracing

    Input Parameters:
.   str    - string to duplicate
.   lineno - line number where used.  Use __LINE__ for this
.   fname  - file name where used.  Use __FILE__ for this

    Returns:
    Pointer to copy of the input string.
 +*/
void *MPIU_trstrdup( const char *str, int lineno, const char fname[] )
{
    void *p;
    unsigned len = (unsigned)strlen( str ) + 1;

    p = MPIU_trmalloc( len, lineno, (char *)fname );
    if (p) {
	MPIU_Memcpy( p, str, len );
    }
    return p;
}

#define TR_MAX_DUMP 100
/*
   The following routine attempts to give useful information about the
   memory usage when an "out-of-memory" error is encountered.  The rules are:
   If there are less than TR_MAX_DUMP blocks, output those.
   Otherwise, try to find multiple instances of the same routine/line #, and
   print a summary by number:
   file line number-of-blocks total-number-of-blocks

   We have to do a sort-in-place for this
 */

/*
  Sort by file/line number.  Do this without calling a system routine or
  allocating ANY space (space is being optimized here).

  We do this by first recursively sorting halves of the list and then
  merging them.  
 */
/* Forward refs for these local (hence static) routines */
static TRSPACE *MPIU_trImerge ( TRSPACE *, TRSPACE * );
static TRSPACE *MPIU_trIsort  ( TRSPACE *, int );
static void MPIU_trSortBlocks ( void );
 
/* Merge two lists, returning the head of the merged list */
static TRSPACE *MPIU_trImerge( TRSPACE *l1, TRSPACE *l2 )
{
    TRSPACE *head = 0, *tail = 0;
    int     sign;
    while (l1 && l2) {
	sign = strncmp(l1->fname, l2->fname, TR_FNAME_LEN - 1 );
	if (sign > 0 || (sign == 0 && l1->lineno >= l2->lineno)) {
	    if (head) tail->next = l1; 
	    else      head = tail = l1;
	    tail = l1;
	    l1   = l1->next;
	}
	else {
	    if (head) tail->next = l2; 
	    else      head = tail = l2;
	    tail = l2;
	    l2   = l2->next;
	}
    }
    /* Add the remaining elements to the end */
    if (l1) tail->next = l1;
    if (l2) tail->next = l2;
    
    return head;
}
/* Sort head with n elements, returning the head */
/* assumes that the MEMALLOC critical section is currently held */
static TRSPACE *MPIU_trIsort( TRSPACE *head, int n )
{
    TRSPACE *p, *l1, *l2;
    int     m, i;

    if (n <= 1) return head;
    
    /* This guarentees that m, n are both > 0 */
    m = n / 2;
    p = head;
    for (i=0; i<m-1; i++) p = p->next;
    /* p now points to the END of the first list */
    l2 = p->next;
    p->next = 0;
    l1 = MPIU_trIsort( head, m );
    l2 = MPIU_trIsort( l2,   n - m );
    return MPIU_trImerge( l1, l2 );
}

/* assumes that the MEMALLOC critical section is currently held */
static void MPIU_trSortBlocks( void )
{
    TRSPACE *head;
    int     cnt;
    
    head = TRhead;
    cnt  = 0;
    while (head) {
	cnt ++;
	head = head->next;
    }
    TRhead = MPIU_trIsort( TRhead, cnt );
}

/* Takes sorted input and dumps as an aggregate */
void MPIU_trdumpGrouped( FILE *fp, int minid )
{
    TRSPACE *head, *cur;
    int     nblocks, nbytes;
    
    if (fp == 0) fp = stderr;
    
    MPIU_THREAD_CS_ENTER(MEMALLOC,);

    MPIU_trSortBlocks();
    head = TRhead;
    cur  = 0;
    while (head) {
	cur     = head->next;
	if (head->id >= minid) {
	    nblocks = 1;
	    nbytes  = (int)head->size;
	    while (cur && 
		   strncmp(cur->fname,head->fname,TR_FNAME_LEN-1) == 0 && 
		   cur->lineno == head->lineno ) {
		nblocks++;
		nbytes += (int)cur->size;
		cur    = cur->next;
	    }
	    FPRINTF( fp, 
		     "[%d] File %13s line %5d: %d bytes in %d allocation%c\n", 
		     world_rank, head->fname, head->lineno, nbytes, nblocks, 
		     (nblocks > 1) ? 's' : ' ' );
	}
	head = cur;
    }
    fflush( fp );

    MPIU_THREAD_CS_EXIT(MEMALLOC,);
}

void MPIU_TrSetMaxMem( int size )
{
    MPIU_THREAD_CS_ENTER(MEMALLOC,);
    TRMaxMemAllow = size;
    MPIU_THREAD_CS_EXIT(MEMALLOC,);
}

static void addrToHex( void *addr, char string[MAX_ADDRESS_CHARS] )
{
    int i;
    static char hex[16] = { '0', '1', '2', '3', '4', '5', '6', '7',
			    '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };
    /* The following construction is used to keep compilers from issuing
       a warning message about casting a pointer to an integer type */
    MPI_Aint iaddr = (MPI_Aint) ((char *)addr - (char*)0);

    /* Initial location */
    i = sizeof(void*) * 2;
    string[i+2] = 0;
    while (i) {
	string[i+1] = hex[iaddr & 0xF];
	iaddr >>= 4;
	i--;
    }
    string[0] = '0';
    string[1] = 'x';
}
