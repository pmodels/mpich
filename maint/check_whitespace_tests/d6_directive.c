/* expect: D6 */
#include <stdio.h>      /* tab stop */
#include <stdlib.h> /* one space */
#ifdef A        /* tab stop */
#define B 1     /* tab stop */
#else /* one space */
#define B 2
#endif /* one space */
#ifdef D
#endif  /* two spaces */
#ifdef E
#endif/* no space */
