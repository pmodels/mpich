/*
 * Copyright (c) 2001-2002 The Trustees of Indiana University.  
 *                         All rights reserved.
 * Copyright (c) 1998-2001 University of Notre Dame. 
 *                         All rights reserved.
 * Copyright (c) 1994-1998 The Ohio State University.  
 *                         All rights reserved.
 * 
 * This file is part of the LAM/MPI software package.  For license
 * information, see the LICENSE file in the top level directory of the
 * LAM/MPI source distribution.
 * 
 * $HEADER$
 *
 *	$Id: typical.h,v 1.1 2006/08/10 19:49:54 penoff Exp $
 *
 *	Function:	- typically used constants and macros
 */

#ifndef _TYPICAL
#define _TYPICAL

/*
 * constants
 */
#ifndef FALSE
#define FALSE		0
#define TRUE		1
#endif

#define LAMERROR	-1

#ifndef ERROR
#define ERROR		LAMERROR
#endif

/*
 * macros
 */
#define LAM_max(a,b)	(((a) > (b)) ? (a) : (b))
#define LAM_min(a,b)	(((a) < (b)) ? (a) : (b))
#define LAM_isNullStr(s)	(*(s) == 0)

/*
 * synonyms
 */
typedef unsigned int	unint;

#endif
