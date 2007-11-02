/*
   (C) 2001 by Argonne National Laboratory.
       See COPYRIGHT in top-level directory.
*/
#if !defined( _CLOG_MEM )
#define _CLOG_MEM

#if defined( NEEDS_STDLIB_PROTOTYPES ) && !defined ( malloc )
#include "protofix.h"
#endif

#if defined(MPIR_MEMDEBUG)
/* Enable memory tracing.  This requires MPICH's mpid/util/tr2.c codes */
#include "mpimem.h"             /* Chameleon memory debugging stuff */
#define MALLOC(a)       MPID_trmalloc((unsigned)(a),__LINE__,__FILE__)
#define FREE(a)         MPID_trfree(a,__LINE__,__FILE__)
#define REALLOC(a,b)    realloc(a,b)
#else
#define MALLOC(a)       malloc(a)
#define FREE(a)         free(a)
#define MPID_trvalid(a)
#define REALLOC(a,b)    realloc(a,b)
#endif

#endif /* of _CLOG_MEM */
