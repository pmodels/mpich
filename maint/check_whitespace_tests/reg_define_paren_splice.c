/* expect: A4 */
#define F(x)                \
    do {                    \
        g(                  \
          (x));             \
    } while (0)
