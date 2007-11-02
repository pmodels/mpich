
#include <stdio.h>

int main( int argc, char *argv[] )
{
    typedef struct { float a; int b; } floatint;
    typedef struct { double a; int b; } doubleint;
    typedef struct { long a; int b; } longint;
    typedef struct { short a; int b; } shortint;
    typedef struct { int a; int b; } twoint;
    typedef struct { long double a; int b; } longdoubleint;

    printf( "CROSS_SIZEOF_SHORT=%d\n", sizeof(short) );
    printf( "CROSS_SIZEOF_INT=%d\n", sizeof(int) );
    printf( "CROSS_SIZEOF_LONG=%d\n", sizeof(long) );
    printf( "CROSS_SIZEOF_LONG_LONG=%d\n", sizeof(long long) );
    printf( "CROSS_SIZEOF_FLOAT=%d\n", sizeof(float) );
    printf( "CROSS_SIZEOF_DOUBLE=%d\n", sizeof(double) );
    printf( "CROSS_SIZEOF_LONG_DOUBLE=%d\n", sizeof(long double) );
    printf( "CROSS_SIZEOF_WCHAR_T=%d\n", sizeof(wchar_t) );
    printf( "CROSS_SIZEOF_FLOAT_INT=%d\n", sizeof(floatint) );
    printf( "CROSS_SIZEOF_DOUBLE_INT=%d\n", sizeof(doubleint) );
    printf( "CROSS_SIZEOF_LONG_INT=%d\n", sizeof(longint) );
    printf( "CROSS_SIZEOF_SHORT_INT=%d\n", sizeof(shortint) );
    printf( "CROSS_SIZEOF_2_INT=%d\n", sizeof(twoint) );
    printf( "CROSS_SIZEOF_LONG_DOUBLE_INT=%d\n", sizeof(longdoubleint) );
    printf( "CROSS_SIZEOF_VOID_P=%d\n", sizeof(void *) );    
    /* printf( "CROSS_SIZEOF_=%d\n", sizeof() ); */

    /* The following code attempts to determine the structure alignment */
    {
#define DBG(a,b,c)
	int is_packed  = 1;
	int is_two     = 1;
	int is_four    = 1;
	int is_eight   = 1;
	int is_largest = 1;
	struct { char a; int b; } char_int;
	struct { char a; short b; } char_short;
	struct { char a; long b; } char_long;
	struct { char a; float b; } char_float;
	struct { char a; double b; } char_double;
	struct { char a; int b; char c; } char_int_char;
	struct { char a; short b; char c; } char_short_char;
#ifdef HAVE_LONG_DOUBLE
	struct { char a; long double b; } char_long_double;
#endif
	int size, extent;

	size = sizeof(char) + sizeof(int);
	extent = sizeof(char_int);
	if (size != extent) is_packed = 0;
	if ( (extent % sizeof(int)) != 0) is_largest = 0;
	if ( (extent % 2) != 0) is_two = 0;
	if ( (extent % 4) != 0) is_four = 0;
	if (sizeof(int) == 8 && (extent % 8) != 0) is_eight = 0;
	DBG("char_int",size,extent);

	size = sizeof(char) + sizeof(short);
	extent = sizeof(char_short);
	if (size != extent) is_packed = 0;
	if ( (extent % sizeof(short)) != 0) is_largest = 0;
	if ( (extent % 2) != 0) is_two = 0;
	if (sizeof(short) == 4 && (extent % 4) != 0) is_four = 0;
	if (sizeof(short) == 8 && (extent % 8) != 0) is_eight = 0;
	DBG("char_short",size,extent);

	size = sizeof(char) + sizeof(long);
	extent = sizeof(char_long);
	if (size != extent) is_packed = 0;
	if ( (extent % sizeof(long)) != 0) is_largest = 0;
	if ( (extent % 2) != 0) is_two = 0;
	if ( (extent % 4) != 0) is_four = 0;
	if (sizeof(long) == 8 && (extent % 8) != 0) is_eight = 0;
	DBG("char_long",size,extent);

	size = sizeof(char) + sizeof(float);
	extent = sizeof(char_float);
	if (size != extent) is_packed = 0;
	if ( (extent % sizeof(float)) != 0) is_largest = 0;
	if ( (extent % 2) != 0) is_two = 0;
	if ( (extent % 4) != 0) is_four = 0;
	if (sizeof(float) == 8 && (extent % 8) != 0) is_eight = 0;
	DBG("char_float",size,extent);

	size = sizeof(char) + sizeof(double);
	extent = sizeof(char_double);
	if (size != extent) is_packed = 0;
	if ( (extent % sizeof(double)) != 0) is_largest = 0;
	if ( (extent % 2) != 0) is_two = 0;
	if ( (extent % 4) != 0) is_four = 0;
	if (sizeof(double) == 8 && (extent % 8) != 0) is_eight = 0;
	DBG("char_double",size,extent);

#ifdef HAVE_LONG_DOUBLE
	size = sizeof(char) + sizeof(long double);
	extent = sizeof(char_long_double);
	if (size != extent) is_packed = 0;
	if ( (extent % sizeof(long double)) != 0) is_largest = 0;
	if ( (extent % 2) != 0) is_two = 0;
	if ( (extent % 4) != 0) is_four = 0;
	if (sizeof(long double) >= 8 && (extent % 8) != 0) is_eight = 0;
	DBG("char_long-double",size,extent);
#endif

	/* char int char helps separate largest from 4/8 aligned */
	size = sizeof(char) + sizeof(int) + sizeof(char);
	extent = sizeof(char_int_char);
	if (size != extent) is_packed = 0;
	if ( (extent % sizeof(int)) != 0) is_largest = 0;
	if ( (extent % 2) != 0) is_two = 0;
	if ( (extent % 4) != 0) is_four = 0;
	if (sizeof(int) == 8 && (extent % 8) != 0) is_eight = 0;
	DBG("char_int_char",size,extent);

    /* char short char helps separate largest from 4/8 aligned */
	size = sizeof(char) + sizeof(short) + sizeof(char);
	extent = sizeof(char_short_char);
	if (size != extent) is_packed = 0;
	if ( (extent % sizeof(short)) != 0) is_largest = 0;
	if ( (extent % 2) != 0) is_two = 0;
	if (sizeof(short) == 4 && (extent % 4) != 0) is_four = 0;
	if (sizeof(short) == 8 && (extent % 8) != 0) is_eight = 0;
	DBG("char_short_char",size,extent);

    /* If aligned mod 8, it will be aligned mod 4 */
	if (is_eight) { is_four = 0; is_two = 0; }

	if (is_four) is_two = 0;

    /* largest superceeds eight */
	if (is_largest) is_eight = 0;

	/* Tabulate the results */
	if (is_packed + is_largest + is_two + is_four + is_eight == 0) {
	    printf( "CROSS_STRUCT_ALIGN=\"unknown\"\n" );
	}
	else {
	    if (is_packed + is_largest + is_two + is_four + is_eight != 1) {
		/* Multiple cases, treat as unknown */
		printf( "CROSS_STRUCT_ALIGN=\"unknown\"\n" );
	    }
	    else {
		if (is_packed) printf( "CROSS_STRUCT_ALIGN=\"packed\"\n" );
		if (is_largest) printf( "CROSS_STRUCT_ALIGN=\"largest\"\n" );
		if (is_two) printf( "CROSS_STRUCT_ALIGN=\"two\"\n" );
		if (is_four) printf( "CROSS_STRUCT_ALIGN=\"four\"\n" );
		if (is_eight) printf( "CROSS_STRUCT_ALIGN=\"eight\"\n" );
	    }

	}
    }
    return 0;
}
