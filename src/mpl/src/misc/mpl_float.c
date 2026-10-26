/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpl.h"
#include <stdint.h>
#include <string.h>

/* Conversion between the native long double and big-endian IEEE 754
 * binary128, see mpl_float.h. A value is decoded from its source format into
 * a sign, an exponent and a 128-bit significand, then encoded into the
 * destination format. The 128-bit arithmetic is done on pairs of uint64_t so
 * that no compiler support for 128-bit integers is needed. */

#ifdef MPL_HAVE_LDOUBLE_BINARY128_CONV

typedef struct {
    uint64_t hi;
    uint64_t lo;
} u128;

typedef enum {
    FPCLS_ZERO,
    FPCLS_FINITE,
    FPCLS_INF,
    FPCLS_NAN
} fp_class;

/* A decoded floating point value. For FINITE, value = sig * 2^(exp - 127),
 * with the leading bit of sig at bit 127. For NAN, sig holds the fraction
 * bits left aligned (bit 127 is the quiet bit). */
typedef struct {
    int sign;
    fp_class cls;
    int exp;
    u128 sig;
} fp_t;

/* A binary interchange format with prec significand bits (including the
 * leading bit), an exponent bias, and whether the leading bit is stored
 * (x87) or implicit (IEEE). The exponent field holds 2 * bias + 1 at most. */
typedef struct {
    int prec;
    int bias;
    int explicit_lead;
} fmt_t;

static const fmt_t fmt_binary128 = { 113, 16383, 0 };

/* 128-bit helpers; shift counts are in [0, 127] */

static u128 u128_make(uint64_t hi, uint64_t lo)
{
    u128 r = { hi, lo };
    return r;
}

static int u128_is_zero(u128 a)
{
    return (a.hi | a.lo) == 0;
}

static u128 u128_shl(u128 a, int n)
{
    if (n == 0) {
        return a;
    } else if (n < 64) {
        return u128_make((a.hi << n) | (a.lo >> (64 - n)), a.lo << n);
    } else {
        return u128_make(a.lo << (n - 64), 0);
    }
}

static u128 u128_shr(u128 a, int n)
{
    if (n == 0) {
        return a;
    } else if (n < 64) {
        return u128_make(a.hi >> n, (a.lo >> n) | (a.hi << (64 - n)));
    } else {
        return u128_make(0, a.hi >> (n - 64));
    }
}

/* 2^n - 1 for n in [0, 128] */
static u128 u128_mask(int n)
{
    if (n == 0) {
        return u128_make(0, 0);
    } else if (n < 64) {
        return u128_make(0, (UINT64_C(1) << n) - 1);
    } else if (n == 64) {
        return u128_make(0, UINT64_MAX);
    } else if (n < 128) {
        return u128_make((UINT64_C(1) << (n - 64)) - 1, UINT64_MAX);
    } else {
        return u128_make(UINT64_MAX, UINT64_MAX);
    }
}

static u128 u128_and(u128 a, u128 b)
{
    return u128_make(a.hi & b.hi, a.lo & b.lo);
}

static int u128_bit(u128 a, int n)
{
    return n < 64 ? (int) ((a.lo >> n) & 1) : (int) ((a.hi >> (n - 64)) & 1);
}

static u128 u128_set_bit(u128 a, int n)
{
    if (n < 64) {
        a.lo |= UINT64_C(1) << n;
    } else {
        a.hi |= UINT64_C(1) << (n - 64);
    }
    return a;
}

static u128 u128_clear_bit(u128 a, int n)
{
    if (n < 64) {
        a.lo &= ~(UINT64_C(1) << n);
    } else {
        a.hi &= ~(UINT64_C(1) << (n - 64));
    }
    return a;
}

static u128 u128_inc(u128 a)
{
    a.lo++;
    if (a.lo == 0) {
        a.hi++;
    }
    return a;
}

/* number of leading zeros, a must not be zero */
static int u128_clz(u128 a)
{
    int n = 0;
    uint64_t x = a.hi;
    if (x == 0) {
        n = 64;
        x = a.lo;
    }
    while (!(x & (UINT64_C(1) << 63))) {
        x <<= 1;
        n++;
    }
    return n;
}

/* Decode the fields of a value in format fmt. frac holds the stored
 * significand bits right aligned: prec - 1 bits, or prec bits when the leading
 * bit is explicit. */
static fp_t decode(const fmt_t * fmt, int sign, int exp_field, u128 frac)
{
    fp_t fp;
    int frac_bits = fmt->prec - 1;
    int exp_max = 2 * fmt->bias + 1;

    fp.sign = sign;
    fp.exp = 0;
    fp.sig = u128_make(0, 0);

    if (exp_field == exp_max) {
        if (fmt->explicit_lead) {
            frac = u128_clear_bit(frac, frac_bits);
        }
        if (u128_is_zero(frac)) {
            fp.cls = FPCLS_INF;
        } else {
            fp.cls = FPCLS_NAN;
            fp.sig = u128_shl(frac, 128 - frac_bits);
        }
        return fp;
    }

    u128 m = frac;
    if (!fmt->explicit_lead && exp_field != 0) {
        m = u128_set_bit(m, frac_bits);
    }
    if (u128_is_zero(m)) {
        fp.cls = FPCLS_ZERO;
        return fp;
    }

    /* value = m * 2^(e - bias - frac_bits), with e = 1 for subnormals */
    int e = exp_field ? exp_field : 1;
    int lz = u128_clz(m);
    fp.cls = FPCLS_FINITE;
    fp.sig = u128_shl(m, lz);
    fp.exp = e - fmt->bias - frac_bits + 127 - lz;
    return fp;
}

/* Encode a decoded value in format fmt, rounding to nearest, ties to even.
 * Returns the exponent field and sets *frac_out as described for
 * decode. */
static int encode(const fmt_t * fmt, fp_t fp, u128 * frac_out)
{
    int frac_bits = fmt->prec - 1;
    int exp_max = 2 * fmt->bias + 1;
    int emin = 1 - fmt->bias;
    u128 m;

    switch (fp.cls) {
        case FPCLS_ZERO:
            *frac_out = u128_make(0, 0);
            return 0;
        case FPCLS_INF:
            goto fn_inf;
        case FPCLS_NAN:
            /* keep the leading payload bits and set the quiet bit, as a
             * hardware conversion does */
            m = u128_shr(fp.sig, 128 - frac_bits);
            m = u128_set_bit(m, frac_bits - 1);
            if (fmt->explicit_lead) {
                m = u128_set_bit(m, frac_bits);
            }
            *frac_out = m;
            return exp_max;
        case FPCLS_FINITE:
            break;
    }

    /* keep the top prec bits, or fewer for a subnormal result */
    int e = fp.exp;
    int shift = 128 - fmt->prec;
    if (e < emin) {
        shift += emin - e;
    }

    int round, sticky;
    if (shift > 128) {
        m = u128_make(0, 0);
        round = 0;
        sticky = 1;
    } else if (shift == 128) {
        m = u128_make(0, 0);
        round = u128_bit(fp.sig, 127);
        sticky = !u128_is_zero(u128_and(fp.sig, u128_mask(127)));
    } else {
        m = u128_shr(fp.sig, shift);
        round = shift ? u128_bit(fp.sig, shift - 1) : 0;
        sticky = shift > 1 && !u128_is_zero(u128_and(fp.sig, u128_mask(shift - 1)));
    }
    if (round && (sticky || (m.lo & 1))) {
        m = u128_inc(m);
    }

    int exp_field;
    if (e >= emin) {
        if (u128_bit(m, fmt->prec)) {
            /* rounding carried into a new leading bit */
            m = u128_shr(m, 1);
            e++;
        }
        if (e > fmt->bias) {
            goto fn_inf;
        }
        exp_field = e + fmt->bias;
    } else {
        /* subnormal, or the smallest normal if rounding carried into the
         * leading bit */
        exp_field = u128_bit(m, frac_bits) ? 1 : 0;
    }
    if (!fmt->explicit_lead) {
        m = u128_clear_bit(m, frac_bits);
    }
    *frac_out = m;
    return exp_field;

  fn_inf:
    m = u128_make(0, 0);
    if (fmt->explicit_lead) {
        m = u128_set_bit(m, frac_bits);
    }
    *frac_out = m;
    return exp_max;
}

/* binary128, big-endian bytes */

static uint64_t load_be64(const unsigned char *p)
{
    uint64_t x = 0;
    for (int i = 0; i < 8; i++) {
        x = (x << 8) | p[i];
    }
    return x;
}

static void store_be64(uint64_t x, unsigned char *p)
{
    for (int i = 7; i >= 0; i--) {
        p[i] = (unsigned char) (x & 0xff);
        x >>= 8;
    }
}

static fp_t binary128_decode(const void *src)
{
    const unsigned char *p = src;
    uint64_t hi = load_be64(p);
    uint64_t lo = load_be64(p + 8);
    int sign = (int) (hi >> 63);
    int exp_field = (int) ((hi >> 48) & 0x7fff);
    u128 frac = u128_make(hi & ((UINT64_C(1) << 48) - 1), lo);
    return decode(&fmt_binary128, sign, exp_field, frac);
}

static void binary128_encode(fp_t fp, void *dest)
{
    unsigned char *p = dest;
    u128 frac;
    int exp_field = encode(&fmt_binary128, fp, &frac);
    uint64_t hi = ((uint64_t) fp.sign << 63) | ((uint64_t) exp_field << 48) | frac.hi;
    store_be64(hi, p);
    store_be64(frac.lo, p + 8);
}

#if MPL_LDBL_MANT_DIG == 53
/* binary64, native byte order */

static const fmt_t fmt_binary64 = { 53, 1023, 0 };

static fp_t binary64_decode(const void *src)
{
    uint64_t u;
    memcpy(&u, src, sizeof(u));
    int sign = (int) (u >> 63);
    int exp_field = (int) ((u >> 52) & 0x7ff);
    u128 frac = u128_make(0, u & ((UINT64_C(1) << 52) - 1));
    return decode(&fmt_binary64, sign, exp_field, frac);
}

static void binary64_encode(fp_t fp, void *dest)
{
    u128 frac;
    int exp_field = encode(&fmt_binary64, fp, &frac);
    uint64_t u = ((uint64_t) fp.sign << 63) | ((uint64_t) exp_field << 52) | frac.lo;
    memcpy(dest, &u, sizeof(u));
}

#else
/* x87 extended precision, little-endian: a 64-bit significand with an
 * explicit leading bit, followed by a 16-bit sign and exponent. Only the 10
 * value bytes are read or written. */

static const fmt_t fmt_x87 = { 64, 16383, 1 };

static fp_t x87_decode(const void *src)
{
    uint64_t m;
    uint16_t se;
    memcpy(&m, src, sizeof(m));
    memcpy(&se, (const char *) src + 8, sizeof(se));
    return decode(&fmt_x87, se >> 15, se & 0x7fff, u128_make(0, m));
}

static void x87_encode(fp_t fp, void *dest)
{
    u128 frac;
    int exp_field = encode(&fmt_x87, fp, &frac);
    uint16_t se = (uint16_t) ((fp.sign << 15) | exp_field);
    memcpy(dest, &frac.lo, sizeof(frac.lo));
    memcpy((char *) dest + 8, &se, sizeof(se));
}
#endif

#if MPL_LDBL_MANT_DIG == 53
#define native_decode binary64_decode
#define native_encode binary64_encode
#else
#define native_decode x87_decode
#define native_encode x87_encode
#endif

void MPL_ldouble_to_binary128_be(const void *src, void *dest)
{
    binary128_encode(native_decode(src), dest);
}

void MPL_binary128_be_to_ldouble(const void *src, void *dest)
{
    native_encode(binary128_decode(src), dest);
}

#endif /* MPL_HAVE_LDOUBLE_BINARY128_CONV */
