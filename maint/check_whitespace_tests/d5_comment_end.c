/* expect: D5 */
#include <x.h>  /*inc*/
#if X   /* ok */
#define M(x) /*mid*/ x
#define N(x) (x) \
    + 1                 /*end*/
#endif /*endif*/

/*column 0*/
int a;                          /*decl*/
int f(void)
{
    /*own*/
    x = 1;      /* stmt*/
    y = 2;      /* multi
                 * line*/
/*col0 in block*/
    z = /*mid*/ 3;
    /*****boxed*****/
    /*!doc*/
    /* x   */
    w = 1;      /*/*/
    if (a) {    /*ifc*/
    }
    return 0;
}
