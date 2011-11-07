/*
 *  (C) 2001 by Argonne National Laboratory
 *      See COPYRIGHT in top-level directory.
 */

/*
 *  @author  Anthony Chan
 */

package base.drawable;

// import java.util.Comparator;
import java.awt.Graphics2D;
import java.awt.Color;
import java.awt.Point;

import cern.colt.map.OpenIntIntHashMap;
import base.topology.Pointer;
import base.topology.MarkerState;

public abstract class Drawable extends InfoBox
{
    // exclusion, nesting_ftr & row_ID are for Nestable Drawable, i.e. state.
    public  static final float      NON_NESTABLE      = 1.0f; 
    public  static final int        INVALID_ROW       = Integer.MIN_VALUE; 

    // INCRE_STARTTIME_ORDER == DRAWING_ORDER
    public  static final Order      INCRE_STARTTIME_ORDER
            = new Order( TimeBoundingBox.INCRE_STARTTIME_ORDER );

    // INCRE_FINALTIME_ORDER == TRACING_ORDER
    public  static final Order      INCRE_FINALTIME_ORDER
            = new Order( TimeBoundingBox.INCRE_FINALTIME_ORDER );

    public  static final Order      DECRE_STARTTIME_ORDER
            = new Order( TimeBoundingBox.DECRE_STARTTIME_ORDER );

    public  static final Order      DECRE_FINALTIME_ORDER
            = new Order( TimeBoundingBox.DECRE_FINALTIME_ORDER );

    private              double     exclusion;     // For SLOG-2 Output API

    private              float      nesting_ftr;   // For SLOG-2 Input API
    private              int        row_ID;        // For SLOG-2 Input API

    // non-null parent => this Drawable is part of a Composite Drawable
    private              Drawable   parent;        // For SLOG-2 Input API


    public Drawable()
    {
        super();
        exclusion    = 0.0d;
        nesting_ftr  = NON_NESTABLE;
        parent       = null;
    }

    public Drawable( final Category in_type )
    {
        super( in_type );
        exclusion    = 0.0d;
        nesting_ftr  = NON_NESTABLE;
        parent       = null;
    }

    //  This is NOT a copy constructor,
    //  only Category and InfoType[] are copied, not InfoValue[].
    public Drawable( final Drawable dobj )
    {
        super( dobj );  // InfoBox( InfoBox );
        exclusion    = 0.0d;
        nesting_ftr  = NON_NESTABLE;
        // parent       = null; 
        parent       = dobj.parent; 
    }

    public Drawable( Category in_type, final Drawable dobj )
    {
        super( in_type, dobj );
        exclusion    = 0.0d;
        nesting_ftr  = NON_NESTABLE;
        // parent       = null; 
        parent       = dobj.parent;
    }

    //  For support of Trace API's  Primitive/Composite generation
    public Drawable( int in_type_idx, byte[] byte_infovals )
    {
        super( in_type_idx );
        super.setInfoBuffer( byte_infovals );
        exclusion    = 0.0d;
        nesting_ftr  = NON_NESTABLE;
        parent       = null;
    }

    //  For SLOG-2 Output API
    public void initExclusion( Object[] childshades )
    {
        exclusion = super.getDuration();
        for ( int idx = childshades.length-1; idx >= 0; idx-- )
            exclusion -= super.getIntersectionDuration(
                               (TimeBoundingBox) childshades[ idx ] );
    }

    //  For SLOG-2 Output API
    public void decrementExclusion( double decre )
    {
        exclusion -= decre;
    }

    //  For SLOG-2 Output API
    public double getExclusion()
    {
        return exclusion;
    }

    //  For SLOG-2 Input API
    public boolean isNestingFactorUninitialized()
    {
        return nesting_ftr == NON_NESTABLE;
    }

    //  For SLOG-2 Input API
    public void  setNestingFactor( float new_nesting_ftr )
    {
        nesting_ftr = new_nesting_ftr;
    }

    //  For SLOG-2 Input API
    public float getNestingFactor()
    {
        return nesting_ftr;
    }

    //  For SLOG-2 Input API
    public boolean isRowIDUninitialized()
    {
        return row_ID == INVALID_ROW;
    }

    public void setRowID( int new_rowID )
    {
        row_ID = new_rowID;
    }

    public int getRowID()
    {
        return row_ID;
    }

    public void setParent( final Drawable dobj )
    {
        parent = dobj;
    }

    //  getParent() returns null == no parent
    public Drawable getParent()
    {
        return parent;
    }

    /*  getByteSize() cannot be declared ABSTRACT, 
        it would jeopardize the use of super.getByteSize( of InfoBox ) in
        subclass like Primitive/Composite.

    public abstract int       getByteSize();
    */
    // public abstract boolean   isTimeOrdered();

    public abstract int       getNumOfPrimitives();

    public abstract Integer[] getArrayOfLineIDs();

    public abstract Coord     getStartVertex();

    public abstract Coord     getFinalVertex();

    public abstract int       drawStateOnCanvas( Graphics2D        g,
                                                 CoordPixelXform   coord_xform,
                                                 OpenIntIntHashMap map_line2row,
                                                 DrawnBoxSet       drawn_boxes,
                                                 ColorAlpha        color );

    public abstract int       drawArrowOnCanvas( Graphics2D        g,
                                                 CoordPixelXform   coord_xform,
                                                 OpenIntIntHashMap map_line2row,
                                                 DrawnBoxSet       drawn_boxes,
                                                 ColorAlpha        color );

    public abstract int       drawEventOnCanvas( Graphics2D        g,
                                                 CoordPixelXform   coord_xform,
                                                 OpenIntIntHashMap map_line2row,
                                                 DrawnBoxSet       drawn_boxes,
                                                 ColorAlpha        color );

    public abstract boolean   isPixelInState( CoordPixelXform   coord_xform,
                                              OpenIntIntHashMap map_line2row,
                                              Point             pix_pt );

    public abstract boolean   isPixelOnArrow( CoordPixelXform   coord_xform,
                                              OpenIntIntHashMap map_line2row,
                                              Point             pix_pt );

    public abstract boolean   isPixelAtEvent( CoordPixelXform   coord_xform,
                                              OpenIntIntHashMap map_line2row,
                                              Point             pix_pt );

    public abstract int       drawStateOnViewport( Graphics2D     g,
                                                CoordPixelXform   coord_xform,
                                                OpenIntIntHashMap map_line2row,
                                                ColorAlpha        color );

    public abstract int       drawArrowOnViewport( Graphics2D     g,
                                                CoordPixelXform   coord_xform,
                                                OpenIntIntHashMap map_line2row,
                                                ColorAlpha        color );

    public abstract int       drawEventOnViewport( Graphics2D     g,
                                                CoordPixelXform   coord_xform,
                                                OpenIntIntHashMap map_line2row,
                                                ColorAlpha        color );

    public abstract boolean   containSearchable();

    // Since Searchable is assumed to be a State drawable,
    // drawSearchableOnViewport() still needs to be an abstract interface.
    // Because State drawable can be Shadow or Primitive which have different
    // way of obtaining the start and final timestamps.  In the simplest
    // possible scenario, when there is no Composite drawable, Shadow and
    // Primitive, could have identical GET timestamp methods, i.e.
    // getEarliestTime() and getLatestTime().  But just to be consistent with
    // drawStateOnViewport() which is an abstract interface, 
    // drawSearchableOnViewport() should be made abstract interface.
    public abstract int  drawSearchableOnViewport( Graphics2D     g,
                                                CoordPixelXform   coord_xform,
                                                OpenIntIntHashMap map_line2row
                                                 );



    private float getRowIDAtTimeOnEvent( OpenIntIntHashMap map_line2row,
                                         double            marker_time )
    {
        Coord vtx   = this.getStartVertex();
        return (float) map_line2row.get( vtx.lineID );
    }

    private float getRowIDAtTimeOnState( OpenIntIntHashMap map_line2row,
                                         double            marker_time )
    {
        Coord vtx   = this.getStartVertex();
        return (float) map_line2row.get( vtx.lineID );
    }

    private float getRowIDAtTimeOnArrow( OpenIntIntHashMap map_line2row,
                                         double            marker_time )
    {
        Coord  start_vtx, final_vtx;
        start_vtx = this.getStartVertex();
        final_vtx = this.getFinalVertex();

        double tStart, tFinal;
        tStart = start_vtx.time;
        tFinal = final_vtx.time;

        double  rStart, rFinal;
        rStart = (double) map_line2row.get( start_vtx.lineID );
        rFinal = (double) map_line2row.get( final_vtx.lineID );

        boolean  isSlopeNonComputable = false;
        double   slope = 0.0;
        if ( tStart != tFinal )
            slope = (double) ( rFinal - rStart ) / ( tFinal - tStart );
        else {
            isSlopeNonComputable = true;
            if ( rStart != rFinal )
                if ( rFinal > rStart )
                    slope = Double.POSITIVE_INFINITY;
                else
                    slope = Double.NEGATIVE_INFINITY;
            else
                // rStart==rFinal, tStart==tFinal, same point
                slope = Double.NaN;
        }

        double marker_rowID;
        if ( ! isSlopeNonComputable )
            marker_rowID = slope * ( marker_time - tStart ) + rStart;
        else
            marker_rowID = ( rStart + rFinal ) / 2.0;

        return (float) marker_rowID;
    }

    public float getRowIDAtTime( OpenIntIntHashMap  map_line2row,
                                 double             marker_time )
    {
        Category type = super.getCategory();
        Topology topo = type.getTopology();
        if ( topo.isEvent() ) {
            return this.getRowIDAtTimeOnEvent( map_line2row, marker_time );
        }
        else if ( topo.isState() ) {
            return this.getRowIDAtTimeOnState( map_line2row, marker_time );
        }
        else if ( topo.isArrow() ) {
            return this.getRowIDAtTimeOnArrow( map_line2row, marker_time );
        }
        else
            System.err.println( "Non-recognized Primitive type! " + this );
        return 0;
    }

    private int drawEventPointerOnViewport( Graphics2D       g,
                                            CoordPixelXform  coord_xform,
                                            Coord_TimeRowID  marker_vtx )
    {
        Color color = super.getCategory().getColor();
        int marker_rowID_i = Math.round( marker_vtx.rowID );
        Pointer.drawLower( g, color, null, coord_xform,
                           marker_vtx.time, marker_rowID_i+0.3f, 0 );
        return 1;
    }

    private int drawStatePointerOnViewport( Graphics2D       g,
                                            CoordPixelXform  coord_xform,
                                            Coord_TimeRowID  marker_vtx )
    {
        Color color = super.getCategory().getColor();
        int marker_rowID_i = Math.round( marker_vtx.rowID );
        float  nestingftr;
        /* assume NestingFactor has been calculated */
        nestingftr  = this.getNestingFactor();

        float  rStart, rFinal;
        rStart = (float) marker_rowID_i - nestingftr / 2.0f;
        rFinal = rStart + nestingftr;

        Pointer.drawUpper( g, color, null, coord_xform,
                           marker_vtx.time, rStart, -MarkerState.Border_Width );
        Pointer.drawLower( g, color, null, coord_xform,
                           marker_vtx.time, rFinal, MarkerState.Border_Width );
        return 1;
    }

    private int drawArrowPointerOnViewport( Graphics2D       g,
                                            CoordPixelXform  coord_xform,
                                            Coord_TimeRowID  marker_vtx )
    {
        Color color = super.getCategory().getColor();
        // Check if marker's rowID is an integral value, or close to one.
        if ( Math.abs( marker_vtx.rowID - (int)marker_vtx.rowID ) < 0.01f ) {
            Pointer.drawUpper( g, color, null, coord_xform,
                               marker_vtx.time, marker_vtx.rowID-0.3f, 0 );
            Pointer.drawLower( g, color, null, coord_xform,
                               marker_vtx.time, marker_vtx.rowID+0.3f, 0 );
        }
        else {
            if ( marker_vtx.rowID - Math.round( marker_vtx.rowID ) < 0.0f )
                Pointer.drawUpper( g, color, null, coord_xform,
                                   marker_vtx.time, marker_vtx.rowID, 0 );
            else
                Pointer.drawLower( g, color, null, coord_xform,
                                   marker_vtx.time, marker_vtx.rowID, 0 );
        }
        return 1;
    }

    public int drawPointerOnViewport( Graphics2D       g,
                                      CoordPixelXform  coord_xform,
                                      Coord_TimeRowID  marker_vtx )
    {
        Category type = super.getCategory();
        Topology topo = type.getTopology();
        if ( topo.isEvent() ) {
            return this.drawEventPointerOnViewport( g, coord_xform,
                                                    marker_vtx );
        }
        else if ( topo.isState() ) {
            return this.drawStatePointerOnViewport( g, coord_xform,
                                                    marker_vtx );
        }
        else if ( topo.isArrow() ) {
            return this.drawArrowPointerOnViewport( g, coord_xform,
                                                    marker_vtx );
        }
        else
            System.err.println( "Non-recognized Primitive type! " + this );
        return 0;
    }



    /* Caller needs to be sure that the Drawable displayed is a State */
    public void setStateRowAndNesting( CoordPixelXform    coord_xform,
                                       OpenIntIntHashMap  map_line2row,
                                       NestingStacks      nesting_stacks )
    {
        Coord  start_vtx;
        start_vtx = this.getStartVertex();
        /*
        Coord  final_vtx;
        final_vtx = this.getFinalVertex();
        */

        row_ID      = map_line2row.get( start_vtx.lineID );
        nesting_ftr = nesting_stacks.getNestingFactorFor( this );
    }

    /* return number of primitives drawn */
    // drawOnCanvas() implementation DOES NOT check
    // if drawable is completely out of image bound.
    public int       drawOnCanvas( Graphics2D         g,
                                   CoordPixelXform    coord_xform,
                                   OpenIntIntHashMap  map_line2row,
                                   DrawnBoxSet        drawn_boxes )
    {
        Category type = super.getCategory();
        Topology topo = type.getTopology();
        if ( topo.isEvent() ) {
            return this.drawEventOnCanvas( g, coord_xform, map_line2row,
                                           drawn_boxes, type.getColor() );
        }
        else if ( topo.isState() ) {
            return this.drawStateOnCanvas( g, coord_xform, map_line2row,
                                           drawn_boxes, type.getColor() );
        }
        else if ( topo.isArrow() ) {
            return this.drawArrowOnCanvas( g, coord_xform, map_line2row,
                                           drawn_boxes, type.getColor() );
        }
        else
            System.err.println( "Non-recognized Primitive type! " + this );
        return 0;
    }

    public Drawable getDrawableAt( CoordPixelXform    coord_xform,
                                   OpenIntIntHashMap  map_line2row,
                                   Point              pix_pt )
    {
        Category type = super.getCategory();
        Topology topo = type.getTopology();
        if ( topo.isEvent() ) {
            if ( this.isPixelAtEvent( coord_xform, map_line2row, pix_pt ) )
                return this;
        }
        else if ( topo.isState() ) {
            if ( this.isPixelInState( coord_xform, map_line2row, pix_pt ) )
                return this;
        }
        else if ( topo.isArrow() ) {
            if ( this.isPixelOnArrow( coord_xform, map_line2row, pix_pt ) )
                return this;
        }
        else
            System.err.println( "Non-recognized Primitive type! " + this );
        return null;
    }

    /* return number of primitives drawn */
    // drawOnViewport() implementation DOES check
    // if drawable is completely out of image bound.
    public int       drawOnViewport( Graphics2D         g,
                                     CoordPixelXform    coord_xform,
                                     OpenIntIntHashMap  map_line2row )
    {
        Category type = super.getCategory();
        Topology topo = type.getTopology();
        if ( topo.isEvent() ) {
            return this.drawEventOnViewport( g, coord_xform, map_line2row,
                                             type.getColor() );
        }
        else if ( topo.isState() ) {
            return this.drawStateOnViewport( g, coord_xform, map_line2row,
                                             type.getColor() );
        }
        else if ( topo.isArrow() ) {
            return this.drawArrowOnViewport( g, coord_xform, map_line2row,
                                             type.getColor() );
        }
        else
            System.err.println( "Non-recognized Primitive type! " + this );
        return 0;
    }





    /*
        Define the Drawable.Order be an extended alias of java.util.Comparator.
        The extension is the extra method, getTimeBoundingBoxOrder().

        If the Comparator of TimeBoundingBox is INCRE_STARTTIME_ORDER,
        this comparator to Collections.sort() will arrange drawables
        in increasing starttime order.  If starttimes are equals, drawables
        will then be arranged in decreasing finaltime order.

        If the Comparator of TimeBoundingBox is INCRE_FINALTIME_ORDER,
        this comparator to Collections.sort() will arrange drawables
        in increasing finaltime order.  If starttimes are equals, drawables
        will then be arranged in decreasing starttime order.

        Since the comparator is used in the TreeMap of class
        logformat/slog2/input/TreeFloorList.ForeItrOfDrawables
        where the likelihood of equal starttime is high.  In order to avoid
        over-written of iterators due to false equivalent drawables,
        i.e. using starttime and endtime may not be enough.  We implement
        a very strict form of comparator for drawable for drawing order.
    */
    public static class Order implements TimeBoundingBox.Order
    {
        private TimeBoundingBox.Order  timebox_order;

        public Order( TimeBoundingBox.Order tbox_order )
        {
            timebox_order = tbox_order;
        }

        public TimeBoundingBox.Order getTimeBoundingBoxOrder()
        {
            return timebox_order;
        }

        public boolean isIncreasingTimeOrdered()
        {
            return timebox_order.isIncreasingTimeOrdered();
        }

        public boolean isStartTimeOrdered()
        {
            return timebox_order.isStartTimeOrdered();
        }

        public String toString()
        {
            return "Drawable." + timebox_order.toString();
        }

        public int compare( Object o1, Object o2 )
        {
            Drawable  dobj1, dobj2;
            dobj1 = (Drawable) o1;
            dobj2 = (Drawable) o2;
            int dobj_time_order;
            dobj_time_order = timebox_order.compare( (TimeBoundingBox) dobj1,
                                                     (TimeBoundingBox) dobj2 );
            if ( dobj_time_order != 0 )
                return dobj_time_order;
            else {
                if ( dobj1 == dobj2 )
                    return 0;
                else {
                    int dobj1_typeidx, dobj2_typeidx;
                    dobj1_typeidx = dobj1.getCategoryIndex();
                    dobj2_typeidx = dobj2.getCategoryIndex();
                    if ( dobj1_typeidx != dobj2_typeidx )
                        // arbitary order
                        return dobj1_typeidx - dobj2_typeidx;
                    else {
                        int        dobj1_lineID, dobj2_lineID;
                        dobj1_lineID = dobj1.getStartVertex().lineID;
                        dobj2_lineID = dobj2.getStartVertex().lineID;
                        if ( dobj1_lineID != dobj2_lineID )
                            // arbitary order
                            return dobj1_lineID - dobj2_lineID;
                        else {
                            dobj1_lineID = dobj1.getFinalVertex().lineID;
                            dobj2_lineID = dobj2.getFinalVertex().lineID;
                            if ( dobj1_lineID != dobj2_lineID )
                                // arbitary order
                                return dobj1_lineID - dobj2_lineID;
                            else {
                                System.err.println( "Drawable.Order: "
                                          + "WARNING! Equal Drawables?\n"
                                          + dobj1 + "\n" + dobj2 );
                                return 0;
                            }   // FinalVertex's lineID
                        }   // StartVertex's lineID
                    }   // CategoryIndex
                }   // direct dobjX comparison
            }
        }
    }

}
