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
import java.awt.Point;
import base.drawable.CoordPixelXform;

public class Pointer
{
    // Math.toDegrees(rad) and Math.toRadians(deg) are inexact conversion.
    private static final int      POINTER_HALF_ANGLE_DEG = 20;
    private static final double   POINTER_HALF_ANGLE_RAD = Math.PI/9.0;
    private static final double   COS_POINTER_HALF_ANGLE
                                  = Math.cos( POINTER_HALF_ANGLE_RAD );
    private static final double   SIN_POINTER_HALF_ANGLE
                                  = Math.sin( POINTER_HALF_ANGLE_RAD );

    private static       int      Pointer_Min_Length     = 20; // in pixel 
    private static       int      Pointer_Max_Length     = 40; // in pixel 

    public static void setProperty( int min_length, int max_length )
    {
        Pointer_Min_Length = min_length;
        Pointer_Max_Length = max_length;
    }

    public static int drawUpper( Graphics2D g, Color color, Stroke stroke,
                                 CoordPixelXform    coord_xform,
                                 double point_time, float point_ypos,
                                 int    point_offset )
    {
        int      iPoint, jPoint, jEnd;
        iPoint  = coord_xform.convertTimeToPixel( point_time );
        jPoint  = coord_xform.convertRowToPixel( point_ypos ) + point_offset;
        jEnd    = coord_xform.convertRowToPixel( Math.round(point_ypos)-0.5f );

        boolean  isPointXvtxInImg, isPointYvtxInImg;
        isPointXvtxInImg = iPoint >= 0 && iPoint < coord_xform.getPixelWidth();
        isPointYvtxInImg = jPoint >= 0 && jPoint < coord_xform.getPixelHeight();

        if ( ! (isPointXvtxInImg && isPointYvtxInImg) )
            return 0; 

        Color brighter_color, darker_color;
        brighter_color = color.brighter().brighter();
        darker_color   = color.darker().darker();

        int   radius, diameter;
        int   arrow_Xoff, arrow_Yoff;
        radius          = jPoint - jEnd + 1;
        if ( radius > Pointer_Max_Length )
            radius      = Pointer_Max_Length;
        if ( radius < Pointer_Min_Length )
            radius      = Pointer_Min_Length;
        diameter        = 2 * radius;
        arrow_Xoff      = (int) (radius*SIN_POINTER_HALF_ANGLE + 0.5d);
        arrow_Yoff      = (int) (radius*COS_POINTER_HALF_ANGLE + 0.5d);

        Stroke orig_stroke = null;
        if ( stroke != null ) {
            orig_stroke = g.getStroke();
            g.setStroke( stroke );
        }

        g.setColor( darker_color );
        g.fillArc( iPoint-radius, jPoint-radius, diameter, diameter,
                   90, -POINTER_HALF_ANGLE_DEG );
        g.setColor( brighter_color );
        g.fillArc( iPoint-radius, jPoint-radius, diameter, diameter,
                   90, POINTER_HALF_ANGLE_DEG );
        // Draw upper arrowhead with border
        g.setColor( Color.white );
        g.drawLine( iPoint, jPoint-radius,
                    iPoint-arrow_Xoff, jPoint-arrow_Yoff );
        g.drawLine( iPoint, jPoint,
                    iPoint-arrow_Xoff, jPoint-arrow_Yoff );
        g.setColor( Color.darkGray );
        g.drawLine( iPoint, jPoint-radius,
                    iPoint+arrow_Xoff, jPoint-arrow_Yoff );
        g.setColor( Color.gray );
        g.drawLine( iPoint, jPoint,
                    iPoint+arrow_Xoff, jPoint-arrow_Yoff );

        if ( stroke != null )
            g.setStroke( orig_stroke );

        return 1; 
    }

    public static int drawLower( Graphics2D g, Color color, Stroke stroke,
                                 CoordPixelXform    coord_xform,
                                 double point_time, float point_ypos,
                                 int    point_offset )
    {
        int      iPoint, jPoint, jEnd;
        iPoint  = coord_xform.convertTimeToPixel( point_time );
        jPoint  = coord_xform.convertRowToPixel( point_ypos ) + point_offset;
        jEnd    = coord_xform.convertRowToPixel( Math.round(point_ypos)+0.5f );

        boolean  isPointXvtxInImg, isPointYvtxInImg;
        isPointXvtxInImg = iPoint >= 0 && iPoint < coord_xform.getPixelWidth();
        isPointYvtxInImg = jPoint >= 0 && jPoint < coord_xform.getPixelHeight();

        if ( ! (isPointXvtxInImg && isPointYvtxInImg) )
            return 0; 

        Color brighter_color, darker_color;
        brighter_color = color.brighter().brighter();
        darker_color   = color.darker().darker();

        int   radius, diameter;
        int   arrow_Xoff, arrow_Yoff;
        radius          = jEnd - jPoint + 1;
        if ( radius > Pointer_Max_Length )
            radius      = Pointer_Max_Length;
        if ( radius < Pointer_Min_Length )
            radius      = Pointer_Min_Length;
        diameter        = 2 * radius;
        arrow_Xoff      = (int) (radius*SIN_POINTER_HALF_ANGLE + 0.5d);
        arrow_Yoff      = (int) (radius*COS_POINTER_HALF_ANGLE + 0.5d);

        Stroke orig_stroke = null;
        if ( stroke != null ) {
            orig_stroke = g.getStroke();
            g.setStroke( stroke );
        }

        g.setColor( brighter_color );
        g.fillArc( iPoint-radius, jPoint-radius, diameter, diameter,
                   270, -POINTER_HALF_ANGLE_DEG );
        g.setColor( darker_color );
        g.fillArc( iPoint-radius, jPoint-radius, diameter, diameter,
                   270, POINTER_HALF_ANGLE_DEG );
        // Draw upper arrowhead with border
        g.setColor( Color.white );
        g.drawLine( iPoint, jPoint,
                    iPoint-arrow_Xoff, jPoint+arrow_Yoff );
        g.drawLine( iPoint, jPoint+radius,
                    iPoint-arrow_Xoff, jPoint+arrow_Yoff );
        g.setColor( Color.darkGray );
        g.drawLine( iPoint, jPoint,
                    iPoint+arrow_Xoff, jPoint+arrow_Yoff );
        g.setColor( Color.gray );
        g.drawLine( iPoint, jPoint+radius,
                    iPoint+arrow_Xoff, jPoint+arrow_Yoff );

        if ( stroke != null )
            g.setStroke( orig_stroke );

        return 1; 
    }
}
