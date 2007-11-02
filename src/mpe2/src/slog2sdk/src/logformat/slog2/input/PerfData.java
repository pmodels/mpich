/*
 *  (C) 2001 by Argonne National Laboratory
 *      See COPYRIGHT in top-level directory.
 */

/*
 *  @author  Anthony Chan
 */

package logformat.slog2.input;

import java.util.Iterator;
import java.io.File;
import java.io.FileOutputStream;
import java.io.DataOutputStream;

import base.drawable.*;
import logformat.slog2.*;
import logformat.slog2.input.InputLog;

public class PerfData
{
    private static boolean            isVerbose         = false;

    private static TimeBoundingBox    timeframe_root    = null;

    private static String             in_filename;
    private static short              depth_max;

    public static final void main( String[] args )
    throws java.io.IOException
    {
        InputLog          slog_ins;
        CategoryMap       objdefs;
        TreeTrunk         treetrunk;
        TreeNode          treeroot;
        TimeBoundingBox   timeframe;
        String            err_msg;

        parseCmdLineArgs( args );

        slog_ins   = new InputLog( in_filename );
        if ( slog_ins == null ) {
            System.err.println( "Null input logfile!" );
            System.exit( 1 );
        }
        if ( ! slog_ins.isSLOG2() ) {
            System.err.println( in_filename + " is NOT SLOG-2 file!." );
            System.exit( 1 );
        }
        if ( (err_msg = slog_ins.getCompatibleHeader()) != null ) {
            System.err.print( err_msg );
            InputLog.stdoutConfirmation();
        }
        slog_ins.initialize();
        // System.out.println( slog_ins );

        // Initialize the TreeTrunk with same order as in Jumpshot.
        treetrunk  = new TreeTrunk( slog_ins, Drawable.INCRE_STARTTIME_ORDER );
        treetrunk.setDebuggingEnabled( isVerbose );
        treetrunk.initFromTreeTop();
        treeroot   = treetrunk.getTreeRoot();
        if ( treeroot == null ) {
            System.out.println( "SLOG-2 file, " + in_filename + " "
                              + "contains no drawables" );
            slog_ins.close();
            System.exit( 0 );
        }
        timeframe_root  = new TimeBoundingBox( treeroot );
        depth_max       = treeroot.getTreeNodeID().depth;
        System.out.println( "# TimeWindow = " + timeframe_root
                          + " @ dmax = " + depth_max );

        File             out_file;
        FileOutputStream out_fos;
        DataOutputStream out_dos;
        Iterator         itr_times;

        System.out.println( "****  Foreward  ****" );
        out_file = new File( append_name( in_filename, "_fscroll.dat" ) );
        out_fos  = new FileOutputStream( out_file );
        out_dos  = new DataOutputStream( out_fos );
        itr_times = new ItrOfForeScrollTimes( timeframe_root );
        while ( itr_times.hasNext() ) {
             timeframe = (TimeBoundingBox) itr_times.next();
             timeframe.writeObject( out_dos );
             System.out.println( timeframe );
        }
        out_fos.close();

        System.out.println( "****  Backward  ****" );
        out_file = new File( append_name( in_filename, "_bscroll.dat" ) );
        out_fos  = new FileOutputStream( out_file );
        out_dos  = new DataOutputStream( out_fos );
        itr_times = new ItrOfBackScrollTimes( timeframe_root );
        while ( itr_times.hasNext() ) {
             timeframe = (TimeBoundingBox) itr_times.next();
             timeframe.writeObject( out_dos );
             System.out.println( timeframe );
        }
        out_fos.close();

        System.out.println( "****  Alternate Zoom  ****" );
        out_file = new File( append_name( in_filename, "_altzoom.dat" ) );
        out_fos  = new FileOutputStream( out_file );
        out_dos  = new DataOutputStream( out_fos );
        itr_times = new ItrOfAltZoomTimes( timeframe_root );
        while ( itr_times.hasNext() ) {
             timeframe = (TimeBoundingBox) itr_times.next();
             timeframe.writeObject( out_dos );
             System.out.println( timeframe );
        }
        out_fos.close();

        slog_ins.close();
    }



    private static double zoom_ftr = 2.0d;

    private static short getZoomDepth()
    {
        short depth_zoom;
        if ( depth_max >= 3 ) {
            depth_zoom = depth_max;
            depth_zoom -= 3;
        }
        else
            depth_zoom = 0;

        return depth_zoom;
    }

    private static class ItrOfForeScrollTimes implements Iterator
    {
        private TimeBoundingBox  root_times = null;
        private TimeBoundingBox  next_times = null;

        public ItrOfForeScrollTimes( final TimeBoundingBox endtimes )
        {
            root_times = new TimeBoundingBox( endtimes );

            /* Set initial next_times = middle of root_times */
            short  depth0   = getZoomDepth();
            double ctr_zoom = ( root_times.getEarliestTime()
                              + root_times.getLatestTime() )
                               / 2.0d;
            double zoom_width = Math.pow( zoom_ftr, (double) depth0 );
            double ctr_span = root_times.getDuration() / 2.0d
                            / (zoom_width + 1.0d);
            next_times = new TimeBoundingBox( ctr_zoom - ctr_span,
                                              ctr_zoom + ctr_span );
        }

        public boolean hasNext()
        {
            return root_times.covers( next_times );
        }

        public Object next()
        {
            /* next_times = prev_times */
            TimeBoundingBox  prev_times = new TimeBoundingBox( next_times );
            /* next_times = prev_times + delta_times */
            next_times.setEarliestTime( prev_times.getLatestTime() );
            next_times.setLatestFromEarliest( prev_times.getDuration() );
            if ( root_times.overlaps( prev_times ) )
                return prev_times;
            else
                return null;
        }

        public void remove() {}
    }

    private static class ItrOfBackScrollTimes implements Iterator
    {
        private TimeBoundingBox  root_times = null;
        private TimeBoundingBox  next_times = null;

        public ItrOfBackScrollTimes( final TimeBoundingBox endtimes )
        {
            root_times = new TimeBoundingBox( endtimes );

            /* Set initial next_times = middle of root_times */
            short  depth0   = getZoomDepth();
            double ctr_zoom = ( root_times.getEarliestTime()
                              + root_times.getLatestTime() )
                               / 2.0d;
            double zoom_width = Math.pow( zoom_ftr, (double) depth0 );
            double ctr_span = root_times.getDuration() / 2.0d
                            / (zoom_width + 1.0d);
            next_times = new TimeBoundingBox( ctr_zoom - ctr_span,
                                              ctr_zoom + ctr_span );
        }

        public boolean hasNext()
        {
            return root_times.covers( next_times );
        }

        public Object next()
        {
            /* next_times = prev_times */
            TimeBoundingBox  prev_times = new TimeBoundingBox( next_times );
            /* next_times = prev_times - delta_times */
            next_times.setLatestTime( prev_times.getEarliestTime() );
            next_times.setEarliestFromLatest( prev_times.getDuration() );
            if ( root_times.overlaps( prev_times ) )
                return prev_times;
            else
                return null;
              
        }

        public void remove() {}
    }

    private static class ItrOfAltZoomTimes implements Iterator
    {
        private TimeBoundingBox  root_times = null;
        private TimeBoundingBox  next_times = null;
        private double           ctr_zoom;
        private double           ctr_span;
        private double           off_span;
        private int              isign;
        private int              count;
        private int              max_count;

        public ItrOfAltZoomTimes( final TimeBoundingBox endtimes )
        {
            root_times = new TimeBoundingBox( endtimes );

            /* Set initial next_times = middle of root_times */
            short  depth0     = getZoomDepth();
            double zoom_width = Math.pow( zoom_ftr, (double) depth0 );
            ctr_zoom = ( root_times.getEarliestTime()
                       + root_times.getLatestTime() )
                     / 2.0d;
            ctr_span = root_times.getDuration() / 2.0d
                     / (zoom_width + 1.0d);
            next_times = new TimeBoundingBox( ctr_zoom - ctr_span,
                                              ctr_zoom + ctr_span );
            off_span = 0.0; 
            count      = 0;
            isign      = 1;
            max_count  = 10;
        }

        public boolean hasNext()
        {
            return count < max_count && root_times.covers( next_times );
        }

        public Object next()
        {
            double  off_zoom;
            /* next_times = prev_times */
            TimeBoundingBox  prev_times = next_times;
            /* next_times = prev_times +/- N * delta_times */
            isign *= -1;
            if ( isign < 0 )
                off_span += 4 * ctr_span;
            off_zoom = ctr_zoom + off_span * isign;
            next_times = new TimeBoundingBox( off_zoom - ctr_span,
                                              off_zoom + ctr_span ); 
            if ( root_times.overlaps( prev_times ) )
                return prev_times;
            else
                return null;
        }

        public void remove() {}
    }





    private static String help_msg  = "Usage: java slog2.input.PerfTimer "
                                    + "[options] slog2_filename.\n"
                                    + "Options: \n"
                                    + "\t [-h|-help|--help]                 "
                                    + " Display this message.\n";

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
                    in_filename   = argv[ idx ];
                    idx++;
                }
            }

        if ( in_filename == null ) {
            System.err.println( "The Program needs a SLOG-2 filename as "
                              + "a command line argument." );
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

    private static String append_name( String in_name, String add_name )
    {
        int loc_idx  = in_name.lastIndexOf( ".slog2" );
        if ( loc_idx > 0 )
            return in_name.substring( 0, loc_idx ) + add_name;
        else
            return in_name + add_name;
    }
}


