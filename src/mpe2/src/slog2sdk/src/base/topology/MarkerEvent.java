/*
 *  (C) 2001 by Argonne National Laboratory
 *      See COPYRIGHT in top-level directory.
 */

/*
 *  @author  Anthony Chan
 */

package base.topology;

import java.awt.Graphics2D;
import java.awt.Color;
import java.awt.Stroke;
import java.awt.BasicStroke;
import base.drawable.CoordPixelXform;

public class MarkerEvent
{
    private static       int     Base_Half_Width        = 6;
    private static       int     Base_Width             = 3;

    private static       int     layer_count      = 3;
    private static       Color   colors[];
    private static       Stroke  strokes[];

    //  For Viewer
    public static void setBaseAndBorderProperty( int new_width,
                                                 int new_layer_count )
    {
        Base_Half_Width = new_width / 2;
        if ( Base_Half_Width < 0 )
            Base_Half_Width = 0;
        Base_Width = Base_Half_Width + Base_Half_Width;
        layer_count = new_layer_count;

        strokes = new Stroke[layer_count+1];
        colors  = new Color[layer_count+1];
        // colors[] array is optimized for white, or light-color arrow.
        colors[0]  = Color.white;
        strokes[0] = new BasicStroke( 1.0f,
                                      BasicStroke.CAP_ROUND,
                                      BasicStroke.JOIN_MITER);
        for (int ii = 1; ii <= layer_count; ii++) {
            if ( ii < layer_count )
                colors[ii]  = colors[ii-1].darker();
            else
                colors[ii]  = Color.white;
            strokes[ii] = new BasicStroke( (float) (2*ii+1),
                                           BasicStroke.CAP_ROUND,
                                           BasicStroke.JOIN_MITER);
        }
    }

    public static int  draw( Graphics2D g, Color color, Stroke stroke,
                             CoordPixelXform    coord_xform,
                             double point_time, float point_ypos,
                             float start_ypos,  float final_ypos )
    {
        int      iPoint, jPoint, jStart, jFinal;
        iPoint   = coord_xform.convertTimeToPixel( point_time );
        jPoint   = coord_xform.convertRowToPixel( point_ypos );

        boolean  isPointXvtxInImg, isPointYvtxInImg;
        isPointXvtxInImg = iPoint >= 0 && iPoint < coord_xform.getPixelWidth();
        isPointYvtxInImg = jPoint >= 0 && jPoint < coord_xform.getPixelHeight();

        if ( ! (isPointXvtxInImg && isPointYvtxInImg) )
            return 0;

        jStart   = coord_xform.convertRowToPixel( start_ypos );
        jFinal   = coord_xform.convertRowToPixel( final_ypos );

        Stroke orig_stroke = g.getStroke();

        int  iCorner, jHeight;
        iCorner = iPoint - Base_Half_Width;
        jHeight = jPoint - jStart + 1;

        // Paint the fancy border using strokes[] with various colors[].
        int ilayer;
        for ( ilayer = layer_count; ilayer >= 1; ilayer-- ) {
            g.setColor( colors[ilayer] );
            g.setStroke( strokes[ilayer] );
            // Draw the mainline
            g.drawLine( iPoint, jPoint, iPoint, jFinal );
            // Draw the white ellipse boundary
            if ( jPoint != jFinal )
                g.drawArc( iCorner, jStart, Base_Width, jHeight, 0, 360 );
            else
                g.drawLine( iPoint - Base_Half_Width, jStart,
                            iPoint + Base_Half_Width, jStart );
        }

        g.setColor( color );
        if ( stroke != null )
            g.setStroke( stroke );
        else
            g.setStroke( orig_stroke );

        g.drawLine( iPoint, jPoint, iPoint, jFinal );
        // Fill the ellipse first
        if ( jHeight != 0 )
            g.fillArc( iCorner, jStart, Base_Width, jHeight, 0, 360 );

        g.setColor( Color.white );
        // Draw the white ellipse boundary
        if ( jPoint != jFinal )
            g.drawArc( iCorner, jStart, Base_Width, jHeight, 0, 360 );
        else
            g.drawLine( iPoint - Base_Half_Width, jStart,
                        iPoint + Base_Half_Width, jStart );

        if ( stroke != null )
            g.setStroke( orig_stroke );

        return 1;
    }
}
