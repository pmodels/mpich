#ifndef SQUELCH_H_INCLUDED
#define SQUELCH_H_INCLUDED

static const int SQ_LIMIT = 10;
static int SQ_COUNT = 0;
static int SQ_VERBOSE = 0;

#define SQUELCH(X)                              \
  do {                                          \
    if (SQ_COUNT < SQ_LIMIT || SQ_VERBOSE) {    \
      SQ_COUNT++;                               \
      X                                         \
    }                                           \
  } while (0)

#endif
