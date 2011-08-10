/*
 *  (C) 2001 by Argonne National Laboratory
 *      See COPYRIGHT in top-level directory.
 */

/*
 *  @author  Anthony Chan
 */

package base.topology;

import java.awt.Graphics2D;
import java.awt.Insets;
import java.awt.Color;
import java.awt.Point;
import base.drawable.CoordPixelXform;

public class MarkerState
{
    private static boolean     Is_Staying_On_Top = false;
    public  static int         Border_Width      = 3;

    public static void setProperty( boolean isOnTop, int new_border_width )
    {
        Is_Staying_On_Top = isOnTop;
        Border_Width      = new_border_width;
    }


    /*
        Draw a Rectangle between left-upper vertex (start_time, start_ypos) 
        and right-lower vertex (final_time, final_ypos)
        Assume caller guarantees the order of timestamps and ypos, such that
        start_time <= final_time  and  start_ypos <= final_ypos.
    */
    private static int  drawForward( Graphics2D g, Color color, Insets insets,
                                     CoordPixelXform    coord_xform,
                                     double start_time, float start_ypos,
                                     double final_time, float final_ypos )
    {
        int      last_Xpos, last_Ypos;
        last_Xpos  = coord_xform.getPixelWidth() - 1;
        last_Ypos  = coord_xform.getPixelHeight() - 1;

        int      iStart, jStart, iFinal, jFinal;
        iStart   = coord_xform.convertTimeToPixel( start_time );
        iFinal   = coord_xform.convertTimeToPixel( final_time );

        jStart   = coord_xform.convertRowToPixel( start_ypos );
        jFinal   = coord_xform.convertRowToPixel( final_ypos );

        if ( insets != null ) {
            iStart += insets.left;
            iFinal -= insets.right;
            jStart += insets.top;
            jFinal -= insets.bottom;
        }

        boolean  isStartXvtxInImg, isStartYvtxInImg;
        boolean  isFinalXvtxInImg, isFinalYvtxInImg;
        isStartXvtxInImg = ( iStart >= 0 ) ;
        isStartYvtxInImg = ( jStart >= 0 ) ;
        isFinalXvtxInImg = ( iFinal <= last_Xpos );
        isFinalYvtxInImg = ( jFinal <= last_Ypos );

        int iHead, iTail, jHead, jTail;
        // jHead = slope * ( iHead - iStart ) + jStart
        iHead = isStartXvtxInImg ? iStart : 0 ;
        jHead = isStartYvtxInImg ? jStart : 0 ;

        // jTail = slope * ( iTail - iFinal ) + jFinal
        iTail = isFinalXvtxInImg ? iFinal : last_Xpos;
        jTail = isFinalYvtxInImg ? jFinal : last_Ypos;

        // Check if both *Head and *Tail are both out of the same side of image.
        if (  ( iHead <= 0 && iTail <= 0 )
           || ( iHead >= last_Xpos && iTail >= last_Xpos )
           || ( jHead <= 0 && jTail <= 0 )
           || ( jHead >= last_Ypos && jTail >= last_Ypos ) )
           return 0;

        // System.out.println( "MarkerState.drawF(): "
        //                   + "iHead="+iHead+",jHead="+jHead + "  "
        //                   + "iTail="+iTail+",jTail="+jTail );

        // Fill the color of the rectangle
        if ( Is_Staying_On_Top ) {
            g.setColor( color );
            g.fillRect( iHead, jHead, iTail-iHead+1, jTail-jHead+1 );
        }
        // Draw the innermost left & top with a dark color
        int ii;

        // Draw left & top with a bright color
        if ( iHead >= 0 ) {
            g.setColor( Color.black );
                ii = 0;
                g.drawLine( iHead-ii, jHead-ii, iHead-ii, jTail+ii );  // left
            g.setColor( Color.white );
            for ( ii = 1; ii <= Border_Width; ii++ )
                g.drawLine( iHead-ii, jHead-ii, iHead-ii, jTail+ii );  // left
        }

        if ( jHead >= 0 ) {
            g.setColor( Color.black );
                ii = 0;
                g.drawLine( iHead-ii, jHead-ii, iTail+ii, jHead-ii );  // top
            g.setColor( Color.white );
            for ( ii = 1; ii <= Border_Width; ii++ )
                g.drawLine( iHead-ii, jHead-ii, iTail+ii, jHead-ii );  // top
        }

        // Draw the innermost right & bottom with a bright color
        if ( iTail <= last_Xpos ) {
            g.setColor( Color.white );
                ii = 0;
                g.drawLine( iTail+ii, jHead-ii, iTail+ii, jTail+ii );  // right
            // Draw right with a dark color
            g.setColor( Color.darkGray );
            for ( ii = 1; ii <= Border_Width; ii++ )
                g.drawLine( iTail+ii, jHead-ii, iTail+ii, jTail+ii );  // right
        }

        if ( jTail <= last_Ypos ) {
            g.setColor( Color.white );
                ii = 0;
                g.drawLine( iHead-ii, jTail+ii, iTail+ii, jTail+ii );  // bottom
            // Draw bottom with a dark color
            g.setColor( Color.darkGray );
            for ( ii = 1; ii <= Border_Width; ii++ )
                g.drawLine( iHead-ii, jTail+ii, iTail+ii, jTail+ii );  // bottom
        }

        return 1;
    }

    public static int  draw( Graphics2D g, Color color, Insets insets,
                             CoordPixelXform    coord_xform,
                             double start_time, float start_ypos,
                             double final_time, float final_ypos )
    {
         if ( start_time < final_time ) {
             if ( start_ypos < final_ypos )
                 return drawForward( g, color, insets, coord_xform,
                                     start_time, start_ypos,
                                     final_time, final_ypos );
             else
                 return drawForward( g, color, insets, coord_xform,
                                     start_time, final_ypos,
                                     final_time, start_ypos );
         }
         else {
             if ( start_ypos < final_ypos )
                 return drawForward( g, color, insets, coord_xform,
                                     final_time, start_ypos,
                                     start_time, final_ypos );
             else
                 return drawForward( g, color, insets, coord_xform,
                                     final_time, final_ypos,
                                     start_time, start_ypos );
         }
    }
}
