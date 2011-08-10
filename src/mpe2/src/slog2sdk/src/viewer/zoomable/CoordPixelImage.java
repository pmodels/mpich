/*
 *  (C) 2001 by Argonne National Laboratory
 *      See COPYRIGHT in top-level directory.
 */

/*
 *  @author  Anthony Chan
 */

package viewer.zoomable;

import base.drawable.TimeBoundingBox;
import base.drawable.CoordPixelXform;

import viewer.common.Parameters;

public class CoordPixelImage implements CoordPixelXform
{
    private ScrollableObject  img_obj;
    private int               row_hgt;
    private int               row_half_hgt;

    private TimeBoundingBox   img_endtimes;
    private double            img_starttime;
    private double            img_finaltime;

    private int               ipix_start;
    private int               ipix_final;
    private int               ipix_width;
    private int               jpix_start;
    private int               jpix_final;
    private int               jpix_height;

    /*
       ipix* and jpix* are for offset in real/pixel coordinate xform.
       the pixel coordinate could be the buffer image's or Viewport's.
    */
    public CoordPixelImage( ScrollableObject image_object )
    {
        img_obj        = image_object;
        row_hgt        = 0;
        row_half_hgt   = 0;

        // Initialize *pix_final/width to int's Max as if it is a huge canvas
        this.resetXaxisPixelBounds( 0, Integer.MAX_VALUE-1 );
        this.resetYaxisPixelBounds( 0, Integer.MAX_VALUE-1 );
    }

    public CoordPixelImage( ScrollableObject image_object, int row_height,
                            final TimeBoundingBox  image_timebounds )
    {
        this( image_object );
        this.resetRowHeight( row_height );
        this.resetTimeBounds( image_timebounds );
    }

    public void resetRowHeight( int row_height )
    {
        row_hgt        = row_height;
        row_half_hgt   = row_height / 2 + 1;
    }

    public void resetXaxisPixelBounds( int istart, int ifinal )
    {
        ipix_start     = istart;
        ipix_final     = ifinal;
        ipix_width     = ipix_final - ipix_start + 1;
    }

    public void resetYaxisPixelBounds( int jstart, int jfinal )
    {
        jpix_start     = jstart;
        jpix_final     = jfinal;
        jpix_height    = jpix_final - jpix_start + 1;
    }

    public void resetTimeBounds( final TimeBoundingBox  image_timebounds )
    {
        img_endtimes   = image_timebounds;
        img_starttime  = image_timebounds.getEarliestTime();
        img_finaltime  = image_timebounds.getLatestTime();
        this.resetXaxisPixelBounds( img_obj.time2pixel( img_starttime ),
                                    img_obj.time2pixel( img_finaltime ) );
    }

    public int     convertTimeToPixel( double time_coord )
    {
        return img_obj.time2pixel( time_coord ) - ipix_start;
    }

    public double  convertPixelToTime( int hori_pixel )
    {
        return img_obj.pixel2time( hori_pixel + ipix_start );
    }

    public int     convertRowToPixel( float rowID )
    {
        return Math.round( rowID * row_hgt + row_half_hgt ) - jpix_start;
    }

    public float   convertPixelToRow( int vert_pixel )
    {
        // return  (float) ( vert_pixel - row_half_hgt ) / row_hgt;
        /*
        if ( Parameters.Y_AXIS_ROOT_VISIBLE )
            return  vert_pixel / row_hgt + 1;
        else
            return  vert_pixel / row_hgt;
        */
        return  (float) ( vert_pixel + jpix_start - row_half_hgt ) / row_hgt;
    }

    /*
    public double  getBoundedStartTime( double starttime )
    {
        return ( starttime > img_starttime ?  starttime :  img_starttime );
    }

    public double  getBoundedFinalTime( double finaltime )
    {
        return ( finaltime < img_finaltime ?  finaltime :  img_finaltime );
    }
    */

    public boolean contains( double time_coord )
    {
        return img_endtimes.contains( time_coord );
    }

    public boolean overlaps( final TimeBoundingBox  timebox )
    {
        return img_endtimes.overlaps( timebox );
    }

    public int     getPixelWidth()
    {
        return ipix_width;
    }

    public int     getPixelHeight()
    {
        return jpix_height;
    }
}
