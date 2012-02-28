AC_INIT()

dnl Definitions will be placed in this file rather than in the DEFS variable
AC_CONFIG_HEADER(pmiconf.h)
AH_TOP([/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef PMICONF_H_INCLUDED
#define PMICONF_H_INCLUDED
])
AH_BOTTOM([#endif])

echo "RUNNING CONFIGURE FOR THE SLURM PMI"

PAC_ARG_CACHING
PAC_VPATH_CHECK()
PAC_LOAD_BASE_CACHE
PAC_PROG_MAKE

AC_ARG_WITH(slurm, AS_HELP_STRING([--with-slurm], [path to the slurm package]))
AC_CHECK_HEADER([slurm/pmi.h], [], [AC_MSG_ERROR([could not find slurm/pmi.h.  Configure aborted])])
AC_CHECK_LIB([pmi], [PMI_Init], [PAC_APPEND_FLAG([-lpmi], [WRAPPER_LIBS])],
		    AC_MSG_ERROR([could not find libpmi.  Configure aborted]))

AC_SUBST(WRAPPER_CFLAGS)
AC_SUBST(WRAPPER_LDFLAGS)
AC_SUBST(WRAPPER_LIBS)

AC_OUTPUT(localdefs)
AC_OUTPUT(Makefile)
