#include <stdio.h>
#define DBG(a,b,c)
int main( int argc, char *argv[] )
{
    FILE *cf;
    int is_packed  = 1;
    int is_two     = 1;
    int is_four    = 1;
    int is_eight   = 1;
    int is_sixteen = 1;
    struct { char a; float b; } char_float;
    struct { float b; char a; } float_char;
    struct { char a; double b; } char_double;
    struct { double b; char a; } double_char;
#ifdef HAVE_LONG_DOUBLE
    struct { char a; long double b; } char_long_double;
    struct { long double b; char a; } long_double_char;
    struct { long double a; int b; char c; } long_double_int_char;
#endif
    int size, extent1, extent2;

    size = sizeof(char) + sizeof(float);
    extent1 = sizeof(char_float);
    extent2 = sizeof(float_char);
    if (size != extent1) is_packed = 0;
    if ( (extent1 % 2) != 0 && (extent2 % 2) != 0) is_two = 0;
    if ( (extent1 % 4) != 0 && (extent2 % 4) != 0) is_four = 0;
    if (sizeof(float) == 8 && (extent1 % 8) != 0 && (extent2 % 8) != 0)
	is_eight = 0;
    DBG("char_float",size,extent1);

    size = sizeof(char) + sizeof(double);
    extent1 = sizeof(char_double);
    extent2 = sizeof(double_char);
    if (size != extent1) is_packed = 0;
    if ( (extent1 % 2) != 0 && (extent2 % 2) != 0) is_two = 0;
    if ( (extent1 % 4) != 0 && (extent2 % 4) != 0) is_four = 0;
    if (sizeof(double) == 8 && (extent1 % 8) != 0 && (extent2 % 8) != 0)
	is_eight = 0;
    DBG("char_double",size,extent1);

#ifdef HAVE_LONG_DOUBLE
    size = sizeof(char) + sizeof(long double);
    extent1 = sizeof(char_long_double);
    extent2 = sizeof(long_double_char);
    if (size != extent1) is_packed = 0;
    if ( (extent1 % 2) != 0 && (extent2 % 2) != 0) is_two = 0;
    if ( (extent1 % 4) != 0 && (extent2 % 4) != 0) is_four = 0;
    if (sizeof(long double) >= 8 && (extent1 % 8) != 0 && (extent2 % 8) != 0)
	is_eight = 0;
    if (sizeof(long double) > 8 && (extent1 % 16) != 0
	&& (extent2 % 16) != 0) is_sixteen = 0;
    DBG("char_long-double",size,extent1);

    extent1 = sizeof(long_double_int_char);
    if ( (extent1 % 2) != 0) is_two = 0;
    if ( (extent1 % 4) != 0) is_four = 0;
    if (sizeof(long double) >= 8 && (extent1 % 8) != 0)	is_eight = 0;
    if (sizeof(long double) > 8 && (extent1 % 16) != 0) is_sixteen = 0;
#else
    is_sixteen = 0;
#endif

    if (is_sixteen) { is_eight = 0; is_four = 0; is_two = 0; }

    if (is_eight) { is_four = 0; is_two = 0; }

    if (is_four) is_two = 0;

    /* Tabulate the results */
    cf = fopen( "ctest.out", "w" );
    if (is_packed + is_two + is_four + is_eight + is_sixteen == 0) {
	fprintf( cf, "Could not determine alignment\n" );
    }
    else {
	if (is_packed + is_two + is_four + is_eight + is_sixteen != 1) {
	    fprintf( cf, "error!\n" );
	}
	else {
	    if (is_packed) fprintf( cf, "packed\n" );
	    if (is_two) fprintf( cf, "two\n" );
	    if (is_four) fprintf( cf, "four\n" );
	    if (is_eight) fprintf( cf, "eight\n" );
	    if (is_sixteen) fprintf( cf, "sixteen\n" );
	}
    }
    fclose( cf );
    return 0;
}
