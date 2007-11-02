/*
 *  (C) 2001 by Argonne National Laboratory
 *      See COPYRIGHT in top-level directory.
 */

/*
 *  @author  Anthony Chan
 */

package logformat.slog2.input;

import java.util.ArrayList;
import java.util.Iterator;
import java.io.File;
import java.io.FileInputStream;
import java.io.DataInputStream;
import java.io.IOException;

import base.drawable.*;
import logformat.slog2.*;
import logformat.slog2.input.InputLog;

public class PerfTest
{
    private static final short        PRINT_NONE        = 0;
    private static final short        PRINT_STUB        = 1;
    private static final short        PRINT_ALL         = 2;

    private static boolean            isVerbose         = false;

    private static TimeBoundingBox    timeframe_root    = null;

    private static String             in_slog2filename  = null;
    private static String             in_datafilename   = null;
    private static short              print_opt         = PRINT_NONE;
    private static short              depth_max;

    public static final void main( String[] args )
    throws IOException
    {
        InputLog          slog_ins;
        CategoryMap       objdefs;
        TreeTrunk         treetrunk;
        TreeNode          treeroot;
        TimeBoundingBox   timeframe_old, timeframe;
        short             depth;
        String            err_msg;

        parseCmdLineArgs( args );

        slog_ins   = new InputLog( in_slog2filename );
        if ( slog_ins == null ) {
            System.err.println( "Null input logfile!" );
            System.exit( 1 );
        }
        if ( ! slog_ins.isSLOG2() ) {
            System.err.println( in_slog2filename + " is NOT SLOG-2 file!." );
            System.exit( 1 );
        }
        if ( (err_msg = slog_ins.getCompatibleHeader()) != null ) {
            System.err.print( err_msg );
            InputLog.stdoutConfirmation();
        }
        slog_ins.initialize();
        // System.out.println( slog_ins );

        long         time1, time2;
        StringBuffer timing_str;

        // Initialize the TreeTrunk with same order as in Jumpshot.
        treetrunk  = new TreeTrunk( slog_ins, Drawable.INCRE_STARTTIME_ORDER );
        treetrunk.setDebuggingEnabled( isVerbose );
        treetrunk.initFromTreeTop();
        treeroot   = treetrunk.getTreeRoot();
        if ( treeroot == null ) {
            System.out.println( "SLOG-2 file, " + in_slog2filename + " "
                              + "contains no drawables" );
            slog_ins.close();
            System.exit( 0 );
        }
        timeframe_root  = new TimeBoundingBox( treeroot );
        depth_max       = treeroot.getTreeNodeID().depth;
        System.out.println( "# TimeWindow = " + timeframe_root
                          + " @ dmax = " + depth_max );

        File         data_file = new File( in_datafilename );
        TimeBoxList  tbox_list = new TimeBoxList( data_file );
        Iterator     itr_times = tbox_list.iterator();

        timeframe = timeframe_root;
        if ( itr_times.hasNext() )
            timeframe = (TimeBoundingBox) itr_times.next();
        else {
            System.out.println( "No more timeframe in datafile "
                              + in_datafilename + ".  Exiting..." );
            System.exit( 0 );
        }

        System.out.println( "# Srolling Duration = "
                          + timeframe.getDuration() );

        // Append xmgrace description of the graph
        StringBuffer xmgrace_strbuf = new StringBuffer();
        xmgrace_strbuf.append( "@with g0\n" );
        xmgrace_strbuf.append( "@    title \"" + in_slog2filename + "\"\n" );
        xmgrace_strbuf.append( "@    legend 0.4, 0.8\n" );
        xmgrace_strbuf.append( "@    xaxis label " );
        xmgrace_strbuf.append( "\"Time location in logfile (sec).\"\n" );
        xmgrace_strbuf.append( "@    yaxis label " );
        xmgrace_strbuf.append( "\"Time taken per scroll (msec).\"\n" );
        xmgrace_strbuf.append( "@    s0 symbol 1\n" );
        xmgrace_strbuf.append( "@    s0 symbol size 0.25\n" );
        xmgrace_strbuf.append( "@    s0 line type 0\n" );
        xmgrace_strbuf.append( "@    s0 legend " );
        xmgrace_strbuf.append( "\"Duration of each scroll = "
                             + (float)timeframe.getDuration() + " sec.\"\n" );
        xmgrace_strbuf.append( "@target G0.S0\n" );
        xmgrace_strbuf.append( "@type xy\n" );
        System.out.println( xmgrace_strbuf.toString() );

        timing_str = new StringBuffer();
        // timing_str.append( timeframe + " @ d = " + depth + ": " );
        timing_str.append( timeframe.getEarliestTime() + " " );
        time1  = System.nanoTime();

        // Grow to a fixed size first
        // depth           = depth_max; 
        depth           = 0; 
        treetrunk.growInTreeWindow( treeroot, depth, timeframe );
        switch (print_opt) {
            case PRINT_ALL :
                System.out.println( treetrunk.toString( timeframe ) );
                break;
            case PRINT_STUB :
                System.out.println( treetrunk.toStubString() );
                break;
            case PRINT_NONE :
            default :
        }
        timeframe_old = timeframe;

        time2  = System.nanoTime();
        // timing_str.append( (1.0e-6 * (time2 - time1)) + " msec." );
        timing_str.append( (1.0e-6 * (time2 - time1)) );
        System.out.println( timing_str.toString() );

        // Navigate the slog2 tree
        while ( itr_times.hasNext() ) {
            timeframe = (TimeBoundingBox) itr_times.next();
            timing_str = new StringBuffer();
            // timing_str.append( timeframe + " @ d = " + depth + ": " );
            timing_str.append( timeframe.getEarliestTime() + " " );
            time1  = System.nanoTime();

            if (   treetrunk.updateTimeWindow( timeframe_old, timeframe )
                 > TreeTrunk.TIMEBOX_EQUAL ) {
                switch (print_opt) {
                    case PRINT_ALL :
                        // System.out.println(
                        // treetrunk.toFloorString( timeframe ) );
                        System.out.println( treetrunk.toString( timeframe ) );
                        // System.out.println( treetrunk.toString() );
                        break;
                    case PRINT_STUB :
                        System.out.println( treetrunk.toStubString() );
                        break;
                    case PRINT_NONE :
                    default :
                }
                timeframe_old = timeframe;
            }

            time2  = System.nanoTime();
            // timing_str.append( (1.0e-6 * (time2 - time1)) + " msec." );
            timing_str.append( (1.0e-6 * (time2 - time1)) );
            System.out.println( timing_str.toString() );
        }

        slog_ins.close();
    }

    
    private static class TimeBoxList extends ArrayList
    {
        public TimeBoxList( File data_file )
        throws IOException
        {
            super( (int) ( data_file.length() / 16 ) );
            FileInputStream data_fis  = new FileInputStream( data_file );
            DataInputStream data_dis  = new DataInputStream( data_fis );

            TimeBoundingBox timebox = null;
            while ( data_fis.available() >= 16 ) {
                timebox = new TimeBoundingBox( data_dis );
                super.add( timebox );
            }
            data_fis.close();
        }
    }

    private static double zoom_ftr = 2.0d;

    private static String help_msg = "Usage: java slog2.input.PerfTimer "
                                   + "[options] slog2_filename data_filename.\n"
                                   + "Options: \n"
                                   + "\t [-h|-help|--help]                 "
                                   + " Display this message.\n"
                                   + "\t [-s|-stub]                        "
                                   + " Print TreeNode's stub (Default).\n"
                                   + "\t [-a|-all]                         "
                                   + " Print TreeNode's drawable content.\n"
                                   + "\t [-n|-none]                        "
                                   + " Print neither stub nor drawables.\n"
                                   + "\t [-v|-verbose]                     "
                                   + " Print detailed diagnostic message.\n";

    private static void parseCmdLineArgs( String argv[] )
    {
        String        arg_str;
        StringBuffer  err_msg = new StringBuffer();
        int           idx = 0;

            while ( idx < argv.length ) {
                if ( argv[ idx ].startsWith( "-" ) ) {
                    if (  argv[ idx ].equals( "-h" )
                       || argv[ idx ].equals( "-help" )
                       || argv[ idx ].equals( "--help" ) ) {
                        System.out.println( help_msg );
                        System.out.flush();
                        System.exit( 0 );
                    }
                    else if (  argv[ idx ].equals( "-s" )
                            || argv[ idx ].equals( "-stub" ) ) {
                         print_opt = PRINT_STUB;
                         idx++;
                    }
                    else if (  argv[ idx ].equals( "-a" )
                            || argv[ idx ].equals( "-all" ) ) {
                         print_opt = PRINT_ALL;
                         idx++;
                    }
                    else if (  argv[ idx ].equals( "-n" )
                            || argv[ idx ].equals( "-none" ) ) {
                         print_opt = PRINT_NONE;
                         idx++;
                    }
                    else if (  argv[ idx ].equals( "-v" )
                            || argv[ idx ].equals( "-verbose" ) ) {
                         isVerbose = true;
                         idx++;
                    }
                    else {
                        System.err.println( "Unrecognized option, "
                                          + argv[ idx ] + ", at "
                                          + indexOrderStr( idx+1 )
                                          + " command line argument" );
                        System.out.flush();
                        System.exit( 1 );
                    }
                }
                else {
                    in_slog2filename   = argv[ idx ];
                    idx++;
                    in_datafilename    = argv[ idx ];
                    idx++;
                }
            }

        if ( in_slog2filename == null ) {
            System.err.println( "The Program needs a SLOG-2 filename as "
                              + "part of command line arguments." );
            System.err.println( help_msg );
            System.exit( 1 );
        }
        if ( in_datafilename == null ) {
            System.err.println( "The Program needs a data filename as "
                              + "part of command line arguments." );
            System.err.println( help_msg );
            System.exit( 1 );
        }
    }

    private static String indexOrderStr( int idx )
    {
        switch (idx) {
            case 1  : return Integer.toString( idx ) + "st";
            case 2  : return Integer.toString( idx ) + "nd";
            case 3  : return Integer.toString( idx ) + "rd";
            default : return Integer.toString( idx ) + "th";
        }
    }
}
