/*
 *  (C) 2001 by Argonne National Laboratory
 *      See COPYRIGHT in top-level directory.
 */

/*
 *  @author  Anthony Chan
 */

package viewer.common;

import java.text.NumberFormat;
import java.text.DecimalFormat;
import java.text.ChoiceFormat;

public class TimeFormat
{
    public  static final String   LARGE_DURATION_AUTO      = "hr, min, or sec";
    private static final int      LARGE_DURATION_AUTO_ID   = 0;
    public  static final String   LARGE_DURATION_HHMMSS    = "HH:MM:SS";
    private static final int      LARGE_DURATION_HHMMSS_ID = 1;
    public  static final String   LARGE_DURATION_SEC       = "sec"; 
    private static final int      LARGE_DURATION_SEC_ID    = 2;

    private static       int      LargeDurationFormat  = LARGE_DURATION_SEC_ID;

    private static final double[] LIMITS  = {Double.NEGATIVE_INFINITY, 0.0d,
                                             0.1E-9, 0.1E-6, 0.1E-3, 0.1d,
                                             300.0d, 9000.0d};
    private static final String[] UNITS   = {"-ve", "ps", "ns",
                                             "us", "ms", "s",
                                              "min", "hr" };
    private static final String   PATTERN = "#,##0.00###";

    private              DecimalFormat decfmt   = null;
    private              ChoiceFormat  unitfmt  = null;

    public static void setLargeDurationFormat( String new_format_type )
    {
        if ( new_format_type.equals( LARGE_DURATION_AUTO ) )
            LargeDurationFormat = LARGE_DURATION_AUTO_ID;
        else if ( new_format_type.equals( LARGE_DURATION_HHMMSS ) )
            LargeDurationFormat = LARGE_DURATION_HHMMSS_ID;
        else if ( new_format_type.equals( LARGE_DURATION_SEC ) )
            LargeDurationFormat = LARGE_DURATION_SEC_ID;
        else
            LargeDurationFormat = LARGE_DURATION_SEC_ID;
    }

    public TimeFormat()
    {
        decfmt = (DecimalFormat) NumberFormat.getInstance();
        decfmt.applyPattern( PATTERN );
        unitfmt = new ChoiceFormat( LIMITS, UNITS );
    }

    private static final String HH_MM_SS( double time )
    {
        long    hr, min, sec;
        double  rem;
        hr    = (long) Math.floor( time / 3600.0d );
        rem   = time % 3600.0d;
        min   = (long) Math.floor( rem / 60.0d );
        sec   = Math.round( rem % 60.0d );
        StringBuffer strbuf = new StringBuffer();
        if ( hr < 10 )
            strbuf.append( "0" );
        strbuf.append( hr + ":" );
        if ( min < 10 )
            strbuf.append( "0" );
        strbuf.append( min + ":" );
        if ( sec < 10 )
            strbuf.append( "0" );
        strbuf.append( sec );
        return strbuf.toString();
    }

    public String format( double time )
    {
        String unit = unitfmt.format( Math.abs( time ) );
        if ( unit.equals( "hr" ) )
            switch (LargeDurationFormat) {
                case LARGE_DURATION_AUTO_ID:
                    return decfmt.format(time / 3600.0d) + " hr";
                case LARGE_DURATION_HHMMSS_ID:
                    return HH_MM_SS( time );
                case LARGE_DURATION_SEC_ID:
                default:
                    return decfmt.format(time) + " sec";
            }
        else if ( unit.equals( "min" ) )
            switch (LargeDurationFormat) {
                case LARGE_DURATION_AUTO_ID:
                    return decfmt.format(time / 60.0d) + " min";
                case LARGE_DURATION_HHMMSS_ID:
                    return HH_MM_SS( time );
                case LARGE_DURATION_SEC_ID:
                default:
                    return decfmt.format(time) + " sec";
            }
        else if ( unit.equals( "s" ) )
            return decfmt.format(time) + " sec";
        else if ( unit.equals( "ms" ) )
            return decfmt.format(time * 1.0E3) + " msec";
        else if ( unit.equals( "us" ) )
            return decfmt.format(time * 1.0E6) + " usec";
        else if ( unit.equals( "ns" ) )
            return decfmt.format(time * 1.0E9) + " nsec";
        else if ( unit.equals( "ps" ) )
            return decfmt.format(time * 1.0E12) + " psec";
        else
            return decfmt.format(time) + " sec";
    }
}
