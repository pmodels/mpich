/*
 * Copyright (c) 2001-2002 The Trustees of Indiana University.
 *                         All rights reserved.
 * Copyright (c) 1998-2001 University of Notre Dame.
 *                         All rights reserved.
 * Copyright (c) 1994-1998 The Ohio State University.
 *                         All rights reserved.
 *
 * Parts of this file were part of the LAM/MPI software package.  For license
 * information, see the LICENSE-LAM file in .. (one directory up).
 *
 * Original file by Brian Barrett but slightly modified here.
 *
 *      Function:       - generic hash table templates and constants
 *                      - for both the static and dynamic versions
 */

#ifndef _ALLHASH
#define _ALLHASH


/*
 * flags defining the modes of operation
 */
#define AHLRU		((int4) 1)	/* LRU counters are used */
#define AHNOINIT	((int4) 2)	/* hash table not initialized */

#define EFULL 201
#define EDELETE 202
#define EIMPOSSIBLE 203

typedef int			int4;
typedef unsigned int		uint4;

#define INT4_NIL	((int4) 0)
#define INT4_MAX	((int4) 0x7FFFFFFF)
#define INT4_MIN	((int4) 0x80000000)
#define INT4_LSB	((int4) 0xFF)		/* least significant byte */
#define INT4_LSN	((int4) 0x0F)		/* least significant nibble */
#define INT4_MSN	((int4) 0xF0)		/* most significant nibble */
#define	INT4_SIGN	((int4) 0x80000000)	/* sign bit of an int4 */

#define UINT4_MAX	((uint4) 0xFFFFFFFF)	/* maximum uint4 */

/* 
 * structure definitions
 */
struct ah_desc {			/* hash table descriptor */
	int4		ah_maxnelem;	/* maximum nbr. elements */
	int4		ah_nelem;	/* current nbr. elements */
	int4		ah_elemsize;	/* size of element */
	int4		ah_nullkey;	/* null hash key value */
	int4		ah_mode;	/* mode of operation */
	int4		*ah_lru;	/* table of LRU counters */
	void		*ah_table;	/* ptr to the hash table */
	int		(*ah_cmp)(void*, void*);	/* comparison function */
};

/*
 * type definitions
 */
typedef struct ah_desc	HASH;
typedef struct ah_desc	SHASH;

/*
 * prototypes
 */

/* #ifdef __cplusplus */
/* extern "C" { */
/* #endif */

extern HASH		*ah_init(int4 size, int4 elemsize,
				int4 nullkey, int4 mode);
extern SHASH		*ahs_init(int4 size, int4 esize,
				int4 nullkey, int4 mode, void *htbl,
				int4 *lru, SHASH *ahsd);
extern int		ah_delete(HASH *ahd, int4 key);
extern int		ah_delete_elm(HASH *ahd, void *elem);
extern int		ah_expand(HASH *ahd, int4 newsize);
extern int		ah_insert(HASH *ahd, void *elem);
extern int		ah_kick(HASH *ahd, void *elem);
extern void		ah_free(HASH *ahd);
extern void		ah_setcmp(HASH *ahd, int (*cmp)(void*, void*));
extern void		*ah_find(HASH *ahd, int4 key);
extern void		*ah_find_elem(HASH *ahd, void *elem);
extern void		*ah_next(HASH *ahd, void *elem);

/* insert with expand if necessary */
extern int ah_insert_with_expand(HASH *ahd, void *elem);

/* nicer macro names */
#define hash_insert(a, b)              ah_insert_with_expand(a, b)
#define hash_init(a, b, c, d)          ah_init(a, b, c, d)
#define hash_find(a, b)                ah_find(a, b)
#define hash_delete(a, b)              ah_delete(a, b)
#define hash_free(a)                   ah_free(a)


/* #ifdef __cplusplus */
/* } */
/* #endif */

/*
 * function macros
 */
#define ah_count(ahd)		((ahd)->ah_nelem)
#define ah_size(ahd)		((ahd)->ah_maxnelem)
#define ah_top(ahd)		(ah_next(ahd, (void *) 0))

#define ahs_find(ahd, x)	ah_find(ahd, x)
#define ahs_find_elm(ahd, x)	ah_find_elm(ahd, x)
#define ahs_delete(ahd, x)	ah_delete(ahd, x)
#define ahs_delete_elm(ahd, x)	ah_delete_elm(ahd, x)
#define ahs_insert(ahd, x)	ah_insert(ahd, x)
#define ahs_kick(ahd, x)	ah_kick(ahd, x)
#define ahs_setcmp(ahd, x)	ah_setcmp(ahd, x)
#define ahs_count(ahd)		ah_count(ahd)
#define ahs_size(ahd)		ah_size(ahd)
#define ahs_top(ahd)		ah_top(ahd)
#define ahs_next(ahd, x)	ah_next(ahd, x)

#endif
