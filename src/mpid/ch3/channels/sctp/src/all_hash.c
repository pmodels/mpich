/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

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
 *      Software for Humanity
 *      RBD
 *
 *      This program is freely distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 *      Original file by Brian Barrett but slightly modified here.
 *
 *      Function:       - generic hash table management code
 *                      - fully dynamic version
 *                      - table entry must have int4 key as first field
 */

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "all_hash.h"
#include "typical.h"

static int is_prime(int4 n);
static int4 next_prime(int4 n);

/*
 *	ah_init
 *
 *	Function:	- create a hash table
 *	Accepts:	- size of hash table
 *			- size of hash table element
 *			- value of the null hash key
 *			- operation mode
 *	Returns:	- ptr to hash table descriptor or NULL
 */
HASH * 
ah_init(int4 size, int4 elemsize, int4 nullkey, int4 mode)
{
	HASH		*ahd;
	int4		i;		/* favourite counter */
	int4		*p;		/* favourite pointer */
	int		temp_errno;	/* temporary errno storage */
/* 
 * Handle the trivial cases.
 * The element must be big enough to hold the int4 key.
 */
	if ( (size <= 0) || (elemsize < sizeof(int4)) ) {
		errno = EINVAL;
		return((HASH *) 0);
	}
/*
 * Allocate the hash table descriptor.
 */
	if ((ahd = (HASH *) malloc(sizeof(HASH))) == 0) {
		return((HASH *) 0);
	}

	ahd->ah_maxnelem = size;
	ahd->ah_elemsize = elemsize;
	ahd->ah_nelem = 0;
	ahd->ah_nullkey = nullkey;
	ahd->ah_mode = mode;
	ahd->ah_cmp = 0;
/*
 * Allocate the hash table array.
 */
	ahd->ah_table = (void *) malloc((uint4) size * elemsize);
	if (ahd->ah_table == 0) {
		temp_errno = errno;
		free((char *) ahd);
		errno = temp_errno;
		return((HASH *) 0);
	}
/*
 * Allocate the table of LRU counters if required.
 */
	if ((mode & AHLRU) == AHLRU) {
		ahd->ah_lru = (int4 *) malloc((uint4) size * sizeof(int4));
		if (ahd->ah_lru == (int4 *) 0) {
			temp_errno = errno;
			free((char *) ahd->ah_table);
			free((char *) ahd);
			errno = temp_errno;
			return((HASH *) 0);
		}
	}
	else {
		ahd->ah_lru = 0;
	}
/*
 * Initialize the hash table if AHNOINIT is not specified.
 * Reset all LRU counters if AHLRU is specified.
 */
	if ((mode & AHNOINIT) != AHNOINIT) {
		for (i = 0, p = (int4 *) ahd->ah_table; i < size; i++) {
			*p = nullkey;
			p = (int4 *) ((char *) p + elemsize);
		}
	}

	if ((mode & AHLRU) == AHLRU) {
		for (i = 0; i < size; i++) {
			ahd->ah_lru[i] = INT4_NIL;
		}
	}

	return(ahd);
}


/*
 *	ah_setcmp
 *
 *	Function:	- set the compare function
 *	Accepts:	- ptr to hash table descriptor
 *			- compare function
 */
void
ah_setcmp(HASH *ahd, int (*cmp)())
{
	ahd->ah_cmp = cmp;
}


/*
 *	ah_free
 *
 *	Function:	- free a hash table
 *			- hash table must have been created with ah_init()
 *	Accepts:	- ptr to hash table descriptor
 */
void
ah_free(HASH *ahd)
{
	if (ahd) {
		if (ahd->ah_table) free((char *) ahd->ah_table);

		if ((ahd->ah_mode & AHLRU) && (ahd->ah_lru)) {
			free((char *) ahd->ah_lru);
		}

		free((char *) ahd);
	}
}

/*
 *	ah_insert
 *
 *	Function:	- insert an element in the given hash table
 *			- the first int4 entry of the element is
 *			  taken to be the hash key
 *	Accepts:	- ptr to hash table descriptor
 *			- ptr to the element
 *	Returns:	- 0 or LAMERROR
 */
int
ah_insert(HASH *ahd, void *elem)
{
	int4		key;		/* element key to hash */
	int4		i;		/* favourite index */
	int4		start;		/* starting index position */
	int4		*p;		/* favourite pointer */

	key = *((int4 *) elem);
	if (key == ahd->ah_nullkey) {
		errno = EINVAL;
		return(LAMERROR);
	}

	start = i = abs(key) % ahd->ah_maxnelem;
/*
 * From that starting point, loop looking for the first empty location.
 */
	do {
		p = (int4 *) ((char *) ahd->ah_table + (i * ahd->ah_elemsize));
		if (*p == ahd->ah_nullkey) {
/*
 * Found an empty slot.  Fill it with the new element.
 * Update the number of elements in the hash table.
 */
			memcpy((char *) p, (char *) elem, ahd->ah_elemsize);
			(ahd->ah_nelem)++;
			return(0);
		}

		i = (i + 1) % ahd->ah_maxnelem;
	} while (i != start);
/* 
 * No empty slots found.
 */
	errno = EFULL;
	return (LAMERROR);
}

/* 
 *  this hash insert will expand if out of slots
 */
int
ah_insert_with_expand(HASH *ahd, void *elem) {

  int result, newsize;
  result = ah_insert(ahd, elem);

  if(errno == EFULL) {
    newsize = ah_size(ahd);
    newsize = next_prime(newsize + newsize);
    if(ah_expand(ahd, newsize))
      return (LAMERROR);
    
    /* try again */
    result = ah_insert(ahd, elem);
  } 

  return result;

}


/*
 *	ah_find
 *
 *	Function:	- find an element in the hash table by key
 *			- increment the LRU counters if they exist
 *	Accepts:	- ptr to hash table descriptor
 *			- key of element to locate
 *	Returns:	- ptr to element or NULL
 */
void *
ah_find(HASH *ahd, int4 key)
{
	int4		i;		/* favourite index */
	int4		start;		/* starting index position */
	int4		*p;		/* favourite pointer */

	if (key == ahd->ah_nullkey) {
		errno = EINVAL;
		return((void *) 0);
	}

	start = i = abs(key) % ahd->ah_maxnelem;
/*
 * From starting point, loop searching for first element having the key. 
 */
	do {
		p = (int4 *) ((char *) ahd->ah_table + (i * ahd->ah_elemsize));
		if (*p == key) {
/*
 * Found an element.  Increment the elements' LRU counter if used and
 * return a pointer to the element.
 */
			if ((ahd->ah_mode & AHLRU) &&
				((ahd->ah_lru)[i] < INT4_MAX)) {
				
				(ahd->ah_lru)[i]++;
			}
			return((void *) p);
		}

		i = (i + 1) % ahd->ah_maxnelem;
	} while (i != start);
/*
 * Not such element was found.
 */
	errno = EINVAL;
	return((void *) 0);
}

/*
 *	ah_find_elm
 *
 *	Function:	- find entry in hash table that compares to element
 *			- the first int4 element entry is the hash key
 *			- uses compare func. to distinguish same-key elements
 *	Accepts:	- ptr to hash table descriptor
 *			- ptr to comparable entry
 *	Returns:	- ptr to element or NULL
 */
void *
ah_find_elem(HASH *ahd, void *celem)
{
	void		*pfirst;	/* ptr first element found */
	void		*p;		/* favourite pointer */
	int		i;		/* favourite index */

	pfirst = ah_find(ahd, *((int4 *) celem));

	if ((pfirst == 0) || (ahd->ah_cmp == 0) ||
			((*(ahd->ah_cmp))(pfirst, celem) == 0)) {
		return(pfirst);
	}
/*
 * The element found doesn't match, though it has the same key.
 * If LRU is enabled, decrement the element's count.
 */
	if (ahd->ah_mode & AHLRU) {
		i = ((int) ((char *) pfirst - (char *) ahd->ah_table)) /
						ahd->ah_elemsize;
		--(ahd->ah_lru)[i];
	}
/*
 * Loop finding the matching element further in the table.
 */
	p = ah_next(ahd, pfirst);
	if (p == 0) p = ah_top(ahd);

	for ( ; p != pfirst; p = ah_next(ahd, p)) {

		if (p == 0) p = ah_top(ahd);
/*
 * If a matching element is found, increment its LRU count if used.
 */
		if ((*(ahd->ah_cmp))(p, celem) == 0) {

			if (ahd->ah_mode & AHLRU) {
				i = ((int) ((char *) p -
					(char *) ahd->ah_table)) /
						ahd->ah_elemsize;
				--(ahd->ah_lru)[i];
			}

			return(p);
		}
	}
/*
 * No matching element found.
 */
	errno = EINVAL;
	return((void *) 0);
}

/* 
 *	ah_kick
 *
 *	Function:	- force-insert an element in the given hash table
 *			- the first int4 entry of the element is
 *			  taken to be the hash key
 *			- if the table is full, kick out an old element
 *			- in LRU mode, kick element with smaller LRU counter
 *			- in FIXED mode, kick the element hashed to
 *
 *	Accepts:	- ptr to hash table descriptor
 *			- ptr to the element
 *
 *	Returns:	- 0 or LAMERROR
 */
int
ah_kick(HASH *ahd, void *elem)
{
	int4		i_min;		/* index to min LRU counter */
	int4		*p_min;		/* ptr to min LRU counter */
	int4		i;		/* favourite index */
	int4		j;		/* another index */
	int4		*p;		/* favourite pointer */
/*
 * First try to insert the element as usual.
 */
	if (ah_insert(ahd, elem) != LAMERROR) return(0);
/*
 * It's a true error condition.
 */
	else if (errno != EFULL) return(LAMERROR);
/*
 * The table is full, depending on the mode of operation, find
 * and element to delete (either LRU or FIXED mode).
 * For the LRU mode, start the loop at the first hash entry point,
 * this way, if it is one of the LRU solutions it will be picked by
 * default.  This will give a better performance when accessing it
 * at no extra cost in this O(N) search.
 * For the FIXED mode, the first hash entry point is the solution.
 */
	i_min = abs( *((int4 *) elem) ) % ahd->ah_maxnelem;

	if (ahd->ah_mode & AHLRU) {
		p_min = ahd->ah_lru + i_min;
		for (i = i_min + 1, p = p_min + 1, j = 0;
			j < ahd->ah_maxnelem; i++, p++, j++) {

			if (i >= ahd->ah_maxnelem) {
				i -= ahd->ah_maxnelem;
				p -= ahd->ah_maxnelem;
			}

			if (*p < *p_min) {
				p_min = p;
				i_min = i;
			}
		}
	}

	p_min = (int4 *)
		(((char *) ahd->ah_table) + (i_min * ahd->ah_elemsize));
/*
 * Replace the element with the new one and reset the LRU counter if needed.
 */
	memcpy((char *) p_min, (char *) elem, ahd->ah_elemsize);

	if (ahd->ah_mode & AHLRU) (ahd->ah_lru)[i_min] = INT4_NIL;

	return(0);
}

/*
 *	ah_delete
 *
 *	Function:	- delete an element from the hash table
 *	Accepts:	- ptr to hash table descriptor
 *			- key of element to delete
 *	Returns:	- 0 or LAMERROR
 */
int
ah_delete(HASH *ahd, int4 key)
{
	int4		i;		/* favourite index */
	int4		start;		/* starting index position */
	int4		*p;		/* favourite pointer */

	if (key == ahd->ah_nullkey) {
		errno = EINVAL;
		return(LAMERROR);
	}

	start = i = abs(key) % ahd->ah_maxnelem;
/*
 * From that starting point, loop looking for the first element having the key.
 */
	do {
		p = (int4 *) ((char *) ahd->ah_table + (i * ahd->ah_elemsize));
		if (*p == key) {
/*
 * Found an element.  Delete it and reset the LRU counter if needed
 * and decrease the total number of elements.
 */
			*p = ahd->ah_nullkey;
			(ahd->ah_nelem)--;
			if (ahd->ah_mode & AHLRU) (ahd->ah_lru)[i] = INT4_NIL;
			return(0);
		}

		i = (i + 1) % ahd->ah_maxnelem;
	} while (i != start);
/*
 * Not such element was found.
 */
	errno = EDELETE;
	return(LAMERROR);
}

/*
 *	ah_delete_elm
 *
 *	Function:	- delete comparable element from hash table
 *	Accepts:	- ptr to hash table descriptor
 *			- ptr to comparable entry to delete
 *	Returns:	- 0 or LAMERROR
 */
int
ah_delete_elm(HASH *ahd, void *celem)
{
	int4		i;		/* favourite index */
	int4		start;		/* starting index position */
	int4		*p;		/* favourite pointer */
	int4		key;		/* hashing key */

	key = *((int4 *) celem);
	if (key == ahd->ah_nullkey) {
		errno = EINVAL;
		return(LAMERROR);
	}

	start = i = abs(key) % ahd->ah_maxnelem;
/*
 * From that starting point, loop looking for the first element
 * having the key and comparable to the given element template.
 */
	do {
		p = (int4 *) ((char *) ahd->ah_table + (i * ahd->ah_elemsize));
		if ((*p == key) &&
			((*(ahd->ah_cmp))((void *) p, celem) == 0)) {
/*
 * Found an element.  Delete it and reset the LRU counter if needed
 * and decrease the total number of elements.
 */
			*p = ahd->ah_nullkey;
			(ahd->ah_nelem)--;
			if (ahd->ah_mode & AHLRU) (ahd->ah_lru)[i] = INT4_NIL;
			return(0);
		}

		i = (i + 1) % ahd->ah_maxnelem;
	} while (i != start);
/*
 * Not such element was found.
 */
	errno = EDELETE;
	return(LAMERROR);
}

/*
 *	ah_expand
 *
 *	Function:	- expand the size of the hash table
 *			- new size must be bigger than old size
 *			- if used, the LRU counters are reset to zero
 *			- the new hash table has AHNOINIT turned off
 *	Accepts:	- ptr to hash table descriptor
 *			- new hash table size
 *	Returns:	- 0 or LAMERROR
 */
int
ah_expand(HASH *ahd, int4 newsize)
{
	HASH		*new_ahd;		/* new hash table desc. */
	int4		i;			/* favourite counter */
	int4		*tmp_lru;		/* used for swap of ah_lru */
	void		*tmp_table;		/* used for swap of ah_table */
	char		*p;			/* favourite pointer */

	if (newsize < ahd->ah_maxnelem) {
		errno = EINVAL;
		return(LAMERROR);
	}
	else if (newsize == ahd->ah_maxnelem) {
		return(0);
	}
/*
 * Create a new hash table descriptor of a bigger size.
 */
	if ((new_ahd = ah_init(newsize, ahd->ah_elemsize, ahd->ah_nullkey,
				ahd->ah_mode & (~ AHNOINIT))) == 0) {
		return(LAMERROR);
	}
/*
 * Insert all the elements into the new hash table.
 */
	p = (char *) ahd->ah_table;
	for (i = 0; i < ahd->ah_maxnelem; i++, p += ahd->ah_elemsize) {
		if ( *((int4 *) p) != ahd->ah_nullkey) {
			if (ah_insert(new_ahd, (void *) p)) {
				ah_free(new_ahd);
				errno = EIMPOSSIBLE;
				return(LAMERROR);
			}
		}
	}
/*
 * Store the new hash table descriptor into the old one
 * (i.e. the one the user expects to maintain) and destroy
 * the new descriptor along with the old arrays.
 */
	tmp_table = ahd->ah_table;
	tmp_lru = ahd->ah_lru;
	memcpy((char *) ahd, (char *) new_ahd, sizeof(HASH));
	new_ahd->ah_table = tmp_table;
	new_ahd->ah_lru = tmp_lru;
	ah_free(new_ahd);

	return(0);
}


/*
 *	ah_next
 *
 *	Function:	- get the next element in the hash table
 *			- used to loop through all elements with no
 *			  prior knowledge of the element keys
 *			- does not affect the LRU counters if used
 *	Accepts:	- ptr to hash table descriptor
 *			- ptr to current element
 *	Returns:	- ptr to element or NULL
 */
void *
ah_next(HASH *ahd, void *elem)
{
	char		*end_table;
/*
 * Set up the starting point in the table.  It is either the
 * top of the table or the next element in the array.
 */
	elem = (elem == 0) ? ahd->ah_table :
				(void *) (((char *) elem) + ahd->ah_elemsize);
/*
 * Loop till the first non-null element, or the end of the table.
 */
	end_table = (char *) ahd->ah_table +
				ahd->ah_maxnelem * ahd->ah_elemsize;

	while ((char *) elem < end_table) {
		if ( *((int4 *) elem) != ahd->ah_nullkey) return(elem);

		elem = (void *) (((char *) elem) + ahd->ah_elemsize);
	}

	return((void *) 0);
}

/*
 *	is_prime
 *
 *	Function:	- check if the number is prime
 *	Accepts:	- a positive integer
 *	Returns:	- TRUE or FALSE
 */
static int is_prime(int4 n)
{
	int4		i;		/* favourite counter */
	int4		i_2;		/* i squared */

	if (n <= 0) {
		return(FALSE);
	} else if ((n == 1) || (n == 2)) {
		return(TRUE);
	} else if ((n % 2) == 0) {
		return(FALSE);
	}
/*
 * Loop through all odd number smaller than the largest divisor
 * checking if any divides evenly the given number.
 */
	else {
		for (i = 3, i_2 = 9; i_2 <= n; i += 2) {
			if ((n % i) == 0) {
				return(FALSE);
			}

			i_2 += (i + 1) << 2;
		}
	}

	return(TRUE);
}


/*
 *	next_prime
 *
 *	Function:	- get the first prime # >= the given number
 *	Accepts:	- a positive number
 *	Returns:	- a prime number or ERROR
 */
static int4 next_prime(int4 n)
{
	if (n < 0) {
		return((int4) ERROR);
	} else if (n < 1) {
		return((int4) 1);
	} else if (n < 2) {
		return((int4) 2);
	} else {
/*
 * Find the next prime number in the general case.
 */
		if ((n % 2) == 0) {
			n++;
		}

		while (! is_prime(n)) {
			n += 2;
		}

		return(n);
	}
}
