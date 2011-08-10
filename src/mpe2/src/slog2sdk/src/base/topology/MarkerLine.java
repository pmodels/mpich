/*
 *  (C) 2001 by Argonne National Laboratory
 *      See COPYRIGHT in top-level directory.
 */

/*
 *  @author  Anthony Chan
 */
package base.topology;

import java.awt.Graphics2D;
import java.awt.BasicStroke;
import java.awt.Stroke;
import java.awt.Color;
import java.awt.Point;
import java.awt.geom.Line2D;
import base.drawable.CoordPixelXform;

public class MarkerLine
{
    // MLine_Count should match Line_Strokes.length in Shadow.java
    private static int         MLine_Count      = 10;
    private static Stroke[][]  MLine_Strokes    = null;
    private static int         MLine_ColorCount = 4;
    private static Color[]     MLine_Colors     = null;

    public static final int    OPAQUE           = 255;
    public static final int    NEAR_OPAQUE      = 191;
    public static final int    HALF_OPAQUE      = 127;
    public static final int    NEAR_TRANSPARENT = 63;
    public static final int    TRANSPARENT      = 0;

    public static void setProperty( int color_count )
    {
        MLine_ColorCount = color_count;
        MLine_Strokes = new Stroke[ MLine_Count ][ MLine_ColorCount ];
        for ( int idx = 0; idx < MLine_Count; idx++ )
            for ( int jdx = 0; jdx < MLine_ColorCount; jdx++ ) {
                // (idx+1) matches Lines_Strokes[idx] in Shadow.java
                float width = 2.0f * (jdx+1) + (idx+1);
                MLine_Strokes[idx][jdx] = new BasicStroke( width,
                                                   BasicStroke.CAP_ROUND,
                                                   BasicStroke.JOIN_MITER );
            }

        // The MLine_Colors[] is optimized for yellow.
        MLine_Colors = new Color[ MLine_ColorCount ];
        MLine_Colors[0] = Color.yellow.darker();
        for ( int ii = 1 ; ii < MLine_ColorCount ; ii++ ) {
            MLine_Colors[ii] = MLine_Colors[ii-1].darker();
        }
    }

    /*
        Draw a Line between 2 vertices
        (start_time, start_ypos) and (final_time, final_ypos)
        Asssume caller guarantees : start_time <= final_time
    */
    private static int  drawForward( Graphics2D g, Color color,
                                     BasicStroke stroke,
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

        boolean  isSlopeNonComputable = false;
        double   slope = 0.0;
        if ( iStart != iFinal )
            slope = (double) ( jFinal - jStart ) / ( iFinal - iStart );
        else {
            isSlopeNonComputable = true;
            if ( jStart != jFinal )
                if ( jFinal > jStart )
                    slope = Double.POSITIVE_INFINITY;
                else
                    slope = Double.NEGATIVE_INFINITY;
            else
                // iStart==iFinal, jStart==jFinal, same point
                slope = Double.NaN;
        }

        // Clipping with X-axis boundary: (*Start,*Final) -> (*Lead,*Last)
        boolean  isStartXvtxInImg, isFinalXvtxInImg;
        isStartXvtxInImg = iStart >= 0;
        isFinalXvtxInImg = iFinal <= last_Xpos;

        int iLead, iLast, jLead, jLast;

        // Determine (i/j)Lead
        // jLead = slope * ( iLead - iStart ) + jStart
        if ( isStartXvtxInImg ) {
            iLead = iStart;
            jLead = jStart;
        }
        else {
            if ( isSlopeNonComputable )
                return 0; // Arrow NOT in image
            iLead = 0;
            jLead = (int) Math.rint( slope * ( iLead - iStart ) + jStart );
        }

        // Determine (i/j)Last
        // jLast = slope * ( iLast - iFinal ) + jFinal
        if ( isFinalXvtxInImg ) {
            iLast = iFinal;
            jLast = jFinal;
        }
        else {
            if ( isSlopeNonComputable )
                return 0; // Arrow NOT in image
            iLast = last_Xpos;
            jLast = (int) Math.rint( slope * ( iLast - iFinal ) + jFinal );
        }

        // Clipping with Y-axis boundary: (*Lead,*Last) -> (*Head,*Tail)
        boolean  isStartYvtxInImg, isFinalYvtxInImg;
        isStartYvtxInImg = jStart >= 0 && jStart <= last_Ypos;
        isFinalYvtxInImg = jFinal >= 0 && jFinal <= last_Ypos;

        int iHead, iTail, jHead, jTail;

        // Determine (i/j)Head
        // iHead = ( jHead - jLead ) / slope + iLead
        if ( isStartYvtxInImg ) {
            jHead = jLead;
            iHead = iLead;
        }
        else {
            if ( jStart < 0 )
                jHead = 0;
            else  // if ( jStart > last_Ypos) )
                jHead = last_Ypos;

            if ( isSlopeNonComputable ) // i.e. slope = INFINITY
                iHead = iLead;
            else
                iHead = (int) Math.rint( (jHead - jLead) / slope + iLead );
        }

        // Determine (i/j)Tail
        // iTail = ( jTail - jLast ) / slope + iLast
        if ( isFinalYvtxInImg ) {
            jTail = jLast;
            iTail = iLast;
        }
        else {
            if ( jFinal < 0 )
                jTail = 0;
            else  // if ( jFinal > last_Ypos )
                jTail = last_Ypos;

            if ( isSlopeNonComputable ) // i.e. slope = INFINITY
                iTail = iLast;
            else
                iTail = (int) Math.rint( (jTail - jLast) / slope + iLast );
        }

        // Check if both *Head and *Tail are both out of the same side of image.
        if (  ( iHead <= 0 && iTail <= 0 )
           || ( iHead >= last_Xpos && iTail >= last_Xpos )
           || ( jHead <= 0 && jTail <= 0 )
           || ( jHead >= last_Ypos && jTail >= last_Ypos ) )
           return 0;

        // System.out.println( "MarkerLine.drawF(): "
        //                   + "iHead="+iHead+",jHead="+jHead + "  "
        //                   + "iTail="+iTail+",jTail="+jTail );

        Stroke orig_stroke = g.getStroke();

        // Fetch the line width from input stroke
        int line_idx, ilayer;
        if ( stroke != null )
            line_idx = (int) stroke.getLineWidth();
        else
            line_idx = (int) ((BasicStroke) orig_stroke).getLineWidth();

        // Paint the fancy border using MLine_Strokes[] and MLine_Colors[].
        // As width of Mline_Strokes[][ilayer] increases with ilayer,
        // so drawLine with decreasing ilayer.
        for ( ilayer = MLine_ColorCount-1; ilayer >= 1 ; ilayer-- ) {
            g.setColor( MLine_Colors[ ilayer ]  );
            g.setStroke( MLine_Strokes[ line_idx ][ ilayer ] );
            // Draw the MarkerLine
            g.drawLine( iHead, jHead, iTail, jTail );
        }

        g.setColor( color );
        if ( stroke != null )
            g.setStroke( stroke );
        else
            g.setStroke( orig_stroke );
        // Draw the Line
        g.drawLine( iHead, jHead, iTail, jTail );

        if ( stroke != null )
            g.setStroke( orig_stroke );

        return 1;
    }

    public static int  draw( Graphics2D g, Color color, BasicStroke stroke,
                             CoordPixelXform    coord_xform,
                             double start_time, float start_ypos,
                             double final_time, float final_ypos )
    {
        if ( start_time < final_time )
            return drawForward( g, color, stroke, coord_xform,
                                start_time, start_ypos,
                                final_time, final_ypos );
        else
            return drawForward( g, color, stroke, coord_xform,
                                final_time, final_ypos,
                                start_time, start_ypos );
    }
}
