/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 * Portions of this code were written by Microsoft. Those portions are
 * Copyright (c) 2007 Microsoft Corporation. Microsoft grants
 * permission to use, reproduce, prepare derivative works, and to
 * redistribute to others. The code is licensed "as is." The User
 * bears the risk of using it. Microsoft gives no express warranties,
 * guarantees or conditions. To the extent permitted by law, Microsoft
 * excludes the implied warranties of merchantability, fitness for a
 * particular purpose and non-infringement.
 */

#ifndef MPIIMPL_H_INCLUDED
#define MPIIMPL_H_INCLUDED

#include "mpichconfconst.h"
#include "mpichconf.h"

#include <stdio.h>
#ifdef STDC_HEADERS
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#else
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STDARG_H
#include <stdarg.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#endif

#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

/* for MAXHOSTNAMELEN under Linux and OSX */
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#if defined (HAVE_USLEEP)
#include <unistd.h>
#if defined (NEEDS_USLEEP_DECL)
int usleep(useconds_t usec);
#endif
#endif

#if defined(HAVE_LONG_LONG_INT)
/* Assume two's complement for determining LLONG_MAX (already assumed
 * elsewhere in MPICH). */
#ifndef LLONG_MAX
/* slightly tricky (values in binary):
 * - put a 1 in the second-to-msb digit                   (0100...0000)
 * - sub 1, giving all 1s starting at third-to-msb digit  (0011...1111)
 * - shift left 1                                         (0111...1110)
 * - add 1, yielding all 1s in positive space             (0111...1111) */
#define LLONG_MAX (((((long long) 1 << (sizeof(long long) * CHAR_BIT - 2)) - 1 ) << 1) + 1)
#endif
#endif /* defined(HAVE_LONG_LONG_INT) */

#if (!defined MAXHOSTNAMELEN) && (!defined MAX_HOSTNAME_LEN)
#define MAX_HOSTNAME_LEN 256
#elif !defined MAX_HOSTNAME_LEN
#define MAX_HOSTNAME_LEN MAXHOSTNAMELEN
#endif

/* This allows us to keep names local to a single file when we can use
   weak symbols */
#ifdef  USE_WEAK_SYMBOLS
#define PMPI_LOCAL static
#else
#define PMPI_LOCAL
#endif

/* Fix for universal endianess added in autoconf 2.62 */
#ifdef WORDS_UNIVERSAL_ENDIAN
#if defined(__BIG_ENDIAN__)
#elif defined(__LITTLE_ENDIAN__)
#define WORDS_LITTLEENDIAN
#else
#error 'Universal endianess defined without __BIG_ENDIAN__ or __LITTLE_ENDIAN__'
#endif
#endif

#if defined(HAVE_VSNPRINTF) && defined(NEEDS_VSNPRINTF_DECL) && !defined(vsnprintf)
int vsnprintf(char *str, size_t size, const char *format, va_list ap);
# endif

#if defined(MPL_HAVE_PUTENV) && defined(MPL_NEEDS_PUTENV_DECL)
extern int putenv(char *string);
#endif


/*****************************************************************************
 * We use the following ordering of information in this file:
 *
 *   1. Start with independent headers that do not have any
 *      dependencies on the rest of the MPICH implementation (e.g.,
 *      mpl, opa, mpi.h).
 *
 *   2. Next is forward declarations of MPIR structures (MPIR_Comm,
 *      MPIR_Win, etc.).
 *
 *   3. After that we have device-independent headers (MPIR
 *      functionality that does not have any dependency on MPID).
 *
 *   4. Next is the device "pre" header that defines device-level
 *      initial objects that would be used by the MPIR structures.
 *
 *   5. Then comes the device-dependent MPIR functionality, with the
 *      actual definitions of structures, function prototypes, etc.
 *      This functionality can only rely on the device "pre"
 *      functionality.
 *
 *   6. Finally, we'll add the device "post" header that is allowed to
 *      use anything from the MPIR layer.
 *****************************************************************************/


/*****************************************************************************/
/*********************** PART 1: INDEPENDENT HEADERS *************************/
/*****************************************************************************/

/* if we are defining this, we must define it before including mpl.h */
#if defined(MPICH_DEBUG_MEMINIT)
#define MPL_VG_ENABLED 1
#endif

#include "mpl.h"
#include "opa_primitives.h"
#include "mpi.h"


/*****************************************************************************/
/*********************** PART 2: FORWARD DECLARATION *************************/
/*****************************************************************************/

struct MPIR_Request;
typedef struct MPIR_Request MPIR_Request;

struct MPIR_Comm;
typedef struct MPIR_Comm MPIR_Comm;

struct MPIR_Datatype;
/* FIXME: waiting for Wes' patch revert to fix this */
/* typedef struct MPIR_Datatype MPIR_Datatype; */

struct MPIR_Win;
typedef struct MPIR_Win MPIR_Win;

struct MPIR_Info;
typedef struct MPIR_Info MPIR_Info;

struct MPIR_Group;
typedef struct MPIR_Group MPIR_Group;

struct MPIR_Topology;
typedef struct MPIR_Topology MPIR_Topology;


/*****************************************************************************/
/******************* PART 3: DEVICE INDEPENDENT HEADERS **********************/
/*****************************************************************************/

#include "mpir_misc.h"
#include "mpir_dbg.h"
#include "mpir_objects.h"
#include "mpir_strerror.h"
#include "mpir_type_defs.h"
#include "mpir_assert.h"
#include "mpir_pointers.h"
#include "mpir_refcount.h"
#include "mpir_mem.h"
#include "mpir_info.h"
#include "mpir_errhandler.h"
#include "mpir_attr_generic.h"
#include "mpir_contextid.h"
#include "mpir_status.h"
#include "mpir_debugger.h"
#include "mpir_op.h"
#include "mpir_topo.h"
#include "mpir_tags.h"
#include "mpir_pt2pt.h"
#include "mpir_ext.h"

#ifdef HAVE_CXX_BINDING
#include "mpii_cxxinterface.h"
#endif

#ifdef HAVE_FORTRAN_BINDING
#include "mpii_f77interface.h"
#endif


/*****************************************************************************/
/********************** PART 4: DEVICE PRE DECLARATION ***********************/
/*****************************************************************************/

#include "mpidpre.h"


/*****************************************************************************/
/********************* PART 5: DEVICE DEPENDENT HEADERS **********************/
/*****************************************************************************/

#include "mpir_thread.h"
#include "mpir_attr.h"
#include "mpir_group.h"
#include "mpir_comm.h"
#include "mpir_request.h"
#include "mpir_win.h"
#include "mpir_coll.h"
#include "mpir_func.h"
#include "mpir_err.h"
#include "mpir_datatype.h"
#include "mpir_nbc.h"
#include "mpir_process.h"
#include "mpir_cvars.h"
#include "mpir_misc_post.h"
#include "mpit.h"
#include "mpir_handlemem.h"

/*****************************************************************************/
/******************** PART 6: DEVICE "POST" FUNCTIONALITY ********************/
/*****************************************************************************/

#include "mpidpost.h"

/* avoid conflicts in source files with old-style "char FCNAME[]" vars */
#undef FUNCNAME
#undef FCNAME

#endif /* MPIIMPL_INCLUDED */
