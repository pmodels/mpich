
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
