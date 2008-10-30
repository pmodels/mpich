/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef HYDRA_MEM_H_INCLUDED
#define HYDRA_MEM_H_INCLUDED

#include "hydra.h"

#define HYDU_MALLOC(p, type, size, status) \
    { \
	(p) = (type) MPIU_Malloc((size)); \
	if ((p) == NULL) { 	      \
	    HYDU_Error_printf("failed trying to allocate %d bytes\n", (size)); \
	    (status) = HYD_NO_MEM; \
	    goto fn_fail; \
	} \
    }

#define HYDU_FREE(p) \
    { \
	MPIU_Free(p); \
    }

#define HYDU_STRDUP(src, dest, type, status)	\
    { \
	(dest) = (type) MPIU_Strdup((src)); \
	if ((p) == NULL) { 	      \
	    HYDU_Error_printf("failed duping string %s\n", (src)); \
	    (status) = HYD_INTERNAL_ERROR; \
	    goto fn_fail; \
	} \
    }

#define HYDU_STR_ALLOC_AND_JOIN(strlist, strjoin, status) \
    { \
	int len = 0, i, count;		     \
	for (i = 0; (strlist)[i] != NULL; i++) \
	    len += strlen((strlist)[i]); \
	HYDU_MALLOC((strjoin), char *, len + 1, (status)); \
	count = 0; \
	(strjoin)[0] = 0; \
	for (i = 0; (strlist)[i] != NULL; i++) { 	   \
	    MPIU_Snprintf((strjoin) + count, len - count + 1, "%s", (strlist)[i]); \
	    count += strlen((strlist)[i]); \
	} \
    }

#define HYDU_Int_to_str(x, str, status)		\
    { \
	int len = 1, max = 10, y;			\
	if ((x) < 0) { \
	    len++;	       \
	    y = -(x); \
	}		       \
	else \
	    y = (x); \
	while (y >= max) {   \
	    len++; \
	    max *= 10; \
	} \
	(str) = (char *) MPIU_Malloc(len + 1); \
	if ((str) == NULL) { \
	    HYDU_Error_printf("failed trying to allocate %d bytes\n", len + 1); \
	    (status) = HYD_NO_MEM; \
	    goto fn_fail; \
	} \
	MPIU_Snprintf((str), len + 1, "%d", (x)); \
    }

#endif /* HYDRA_MEM_H_INCLUDED */
