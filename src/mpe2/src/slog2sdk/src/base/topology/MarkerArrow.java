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
// import java.awt.geom.Line2D;
import base.drawable.CoordPixelXform;

public class MarkerArrow
{
    private static       int     Head_Length      = 20;
    private static       int     Head_Half_Width  = 8;

    private static       int     layer_count      = 3;
    private static       Color   colors[];
    private static       Stroke  strokes[];

    // For Viewer
    public static void setHeadAndBorderProperty( int new_length, int new_width,
                                                 int new_layer_count )
    {
        Head_Length = new_length;
        Head_Half_Width = new_width / 2;
        if ( Head_Half_Width < 1 )
            Head_Half_Width = 1;
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
                colors[ii]  = colors[ii-1].darker().darker();
            else
                colors[ii]  = Color.white;
            strokes[ii] = new BasicStroke( (float) (2*ii+1),
                                           BasicStroke.CAP_ROUND,
                                           BasicStroke.JOIN_MITER);
        }
    }

    /*
        Draw an Arrow between 2 vertices
        (start_time, start_ypos) and (final_time, final_ypos)
        Asssume caller guarantees : start_time <= final_time
    */
    private static int  drawForward( Graphics2D g, Color color, Stroke stroke,
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

        // System.out.println( "MarkerArrow.drawF(): "
        //                   + "iHead="+iHead+",jHead="+jHead + "  "
        //                   + "iTail="+iTail+",jTail="+jTail );

        int iLeft, jLeft, iRight, jRight;

        iLeft = 0; jLeft = 0; iRight = 0; jRight = 0;
        // Compute arrow head's 2 side vertices: *Left, *Right
        if ( isFinalXvtxInImg && isFinalYvtxInImg ) {
            /* The left line */
            double cosA, sinA;
            double xBase, yBase, xOff, yOff;
            if ( isSlopeNonComputable ) {
                if ( slope == Double.NaN ) {
                    cosA =  1.0d;
                    sinA =  0.0d;
                }
                else {
                    if ( slope == Double.POSITIVE_INFINITY ) {
                        cosA =  0.0d;
                        sinA =  1.0d;
                    }
                    else {
                        cosA =  0.0d;
                        sinA = -1.0d;
                    }
                }
            }
            else {
                cosA = 1.0d / Math.sqrt( 1.0d + slope * slope );
                sinA = slope * cosA;
            }
            xBase  = iTail - Head_Length * cosA;
            yBase  = jTail - Head_Length * sinA;
            xOff   = Head_Half_Width * sinA;
            yOff   = Head_Half_Width * cosA;
            iLeft  = (int) Math.round( xBase + xOff );
            jLeft  = (int) Math.round( yBase - yOff );
            iRight = (int) Math.round( xBase - xOff );
            jRight = (int) Math.round( yBase + yOff );
        }

        Stroke orig_stroke = g.getStroke();

        // Paint the fancy border using strokes[] with various colors[].
        int ilayer;
        for ( ilayer = layer_count; ilayer >= 1; ilayer-- ) {
            g.setColor( colors[ilayer] );
            g.setStroke( strokes[ilayer] );
            // Draw the mainline
            g.drawLine( iHead, jHead, iTail, jTail );
            // Draw the arrow head
            if ( isFinalXvtxInImg && isFinalYvtxInImg ) {
                g.drawLine( iTail,  jTail,   iLeft,  jLeft );
                g.drawLine( iLeft,  jLeft,   iRight, jRight );
                g.drawLine( iRight, jRight,  iTail,  jTail );
            }
        }

        g.setColor( color );
        if ( stroke != null )
            g.setStroke( stroke );
        else
            g.setStroke( orig_stroke );

        // Draw the main line with possible characteristic from stroke
        g.drawLine( iHead, jHead, iTail, jTail );

        // Draw the arrow head
        if ( isFinalXvtxInImg && isFinalYvtxInImg ) {
            g.drawLine( iTail,  jTail,   iLeft,  jLeft );
            g.drawLine( iLeft,  jLeft,   iRight, jRight );
            g.drawLine( iRight, jRight,  iTail,  jTail );
        }

        if ( stroke != null )
            g.setStroke( orig_stroke );

        return 1;
    }

    /*
        Draw an Arrow between 2 vertices
        (start_time, start_ypos) and (final_time, final_ypos)
        Asssume caller guarantees : final_time <= start_time
    */
    private static int  drawBackward( Graphics2D g, Color color, Stroke stroke,
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
            slope = (double) ( jStart - jFinal ) / ( iStart - iFinal );
        else {
            isSlopeNonComputable = true;
            if ( jStart != jFinal )
                if ( jStart > jFinal )
                    slope = Double.POSITIVE_INFINITY;
                else
                    slope = Double.NEGATIVE_INFINITY;
            else
                // iStart==iFinal, jStart==jFinal, same point
                slope = Double.NaN;
        }

        // Clipping with X-axis boundary: (*Start,*Final) -> (*Lead,*Last)
        boolean  isStartXvtxInImg, isFinalXvtxInImg;
        isStartXvtxInImg = iStart <= last_Xpos;
        isFinalXvtxInImg = iFinal >= 0;

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
            iLead = last_Xpos;
            jLead = (int) Math.rint( jStart + slope * ( iLead - iStart ) );
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
            iLast = 0;
            jLast = (int) Math.rint( jFinal - slope * iFinal );
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
            else  // if ( jStart > last_Ypos )
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
        // This is equivalent to checking of [ !coord_xform.overlaps( dobj )
        // and !coord_xform.overlaps( dobj.Y1, dobj.Y2 ) ]
        if (  ( iHead <= 0 && iTail <= 0 )
           || ( iHead >= last_Xpos && iTail >= last_Xpos )
           || ( jHead <= 0 && jTail <= 0 )
           || ( jHead >= last_Ypos && jTail >= last_Ypos ) )
           return 0;

        int iLeft, jLeft, iRight, jRight;

        iLeft = 0; jLeft = 0; iRight = 0; jRight = 0;
        // Compute arrow head's 2 side vertices: *Left, *Right
        if ( isFinalXvtxInImg && isFinalYvtxInImg ) {
            /* The left line */
            double cosA, sinA;
            double xBase, yBase, xOff, yOff;
            if ( isSlopeNonComputable ) {
                if ( slope == Double.NaN ) {
                    cosA = -1.0d;
                    sinA =  0.0d;
                }
                else {
                    if ( slope == Double.POSITIVE_INFINITY ) {
                        cosA =  0.0d;
                        sinA = -1.0d;
                    }
                    else {
                        cosA =  0.0d;
                        sinA =  1.0d;
                    }
                }
            }
            else {
                cosA = - 1.0d / Math.sqrt( 1.0d + slope * slope );
                sinA = slope * cosA;
            }
            xBase  = iTail - Head_Length * cosA;
            yBase  = jTail - Head_Length * sinA;
            xOff   = Head_Half_Width * sinA;
            yOff   = Head_Half_Width * cosA;
            iLeft  = (int) Math.round( xBase + xOff );
            jLeft  = (int) Math.round( yBase - yOff );
            iRight = (int) Math.round( xBase - xOff );
            jRight = (int) Math.round( yBase + yOff );
        }

        Stroke orig_stroke = g.getStroke();

        int ilayer;
        for ( ilayer = layer_count; ilayer >= 1; ilayer-- ) {
            g.setColor( colors[ilayer] );
            g.setStroke( strokes[ilayer] );
            // Draw the mainline
            g.drawLine( iTail, jTail, iHead, jHead );
            // Draw the arrow head
            if ( isFinalXvtxInImg && isFinalYvtxInImg  ) {
                g.drawLine( iTail,  jTail,   iLeft,  jLeft );
                g.drawLine( iLeft,  jLeft,   iRight, jRight );
                g.drawLine( iRight, jRight,  iTail,  jTail );
            }
        }

        g.setColor( color );
        if ( stroke != null )
            g.setStroke( stroke );
        else
            g.setStroke( orig_stroke );

        // Draw the main line with possible characteristic from stroke
        g.drawLine( iTail, jTail, iHead, jHead );

        // Draw the arrow head
        if ( isFinalXvtxInImg && isFinalYvtxInImg ) {
            g.drawLine( iTail,  jTail,   iLeft,  jLeft );
            g.drawLine( iLeft,  jLeft,   iRight, jRight );
            g.drawLine( iRight, jRight,  iTail,  jTail );
        }

        if ( stroke != null )
            g.setStroke( orig_stroke );

        return 1;
    }


    public static int  draw( Graphics2D g, Color color, Stroke stroke,
                             CoordPixelXform    coord_xform,
                             double start_time, float start_ypos,
                             double final_time, float final_ypos )
    {
        if ( start_time < final_time )
            return drawForward( g, color, stroke, coord_xform,
                                start_time, start_ypos,
                                final_time, final_ypos );
        else
            return drawBackward( g, color, stroke, coord_xform,
                                 start_time, start_ypos,
                                 final_time, final_ypos ); 
    }
}
