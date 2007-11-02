/*
   (C) 2001 by Argonne National Laboratory.
       See COPYRIGHT in top-level directory.
*/
/*
   MPE_Graphics should not be included here in case the system does not
   support the graphics features.
*/

#ifndef _MPE_INCLUDE
#define _MPE_INCLUDE

#if defined(__cplusplus)
extern "C" {
#endif

#include "mpe_misc.h"
#include "mpe_log.h"
#ifdef MPE_GRAPHICS
#include "mpe_graphics.h"
#endif

#if defined(__cplusplus)
};
#endif

#endif
