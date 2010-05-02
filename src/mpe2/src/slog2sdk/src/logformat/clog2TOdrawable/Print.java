/*
 *  (C) 2001 by Argonne National Laboratory
 *      See COPYRIGHT in top-level directory.
 */

/*
 *  @author  Anthony Chan
 */

package logformat.clog2TOdrawable;

import java.util.*;

import base.drawable.*;

public class Print
{
    public static final void main( String args[] )
    {
        String                 filename;
        InputLog               dobj_ins;
        List                   objdefs;   // Primitive def'n
        Map                    shadefs;   // Shadow   def'n
        List                   ycoordmaps;
        Kind                   next_kind;
        Category               objdef;
        Topology               topo;
        Primitive              drawobj;
        YCoordMap              ycoordmap;
        long                   Nobjs;

        if ( args.length != 1 ) {
            System.err.println( "It needs the filename to be the only command "
                              + "line arguemnt" );
            System.exit( 0 );
        }

        filename    = args[ 0 ];
        objdefs     = new ArrayList();
        shadefs     = new HashMap();
        ycoordmaps  = new ArrayList();
        Nobjs       = 0;

        /* */    Date time1 = new Date();
        dobj_ins = new InputLog( filename );
        /* */    Date time2 = new Date();
        while ( ( next_kind = dobj_ins.peekNextKind() ) != Kind.EOF ) {
            if ( next_kind == Kind.TOPOLOGY ) {
                topo = dobj_ins.getNextTopology();
                objdef = Category.getShadowCategory( topo );
                objdefs.add( objdef );
                shadefs.put( topo, objdef );
            }
            if ( next_kind == Kind.YCOORDMAP ) {
                // invoke InputLog().getNextYCoordMap() to get stream moving
                ycoordmaps.add( dobj_ins.getNextYCoordMap() );
            }
            if ( next_kind == Kind.CATEGORY ) {
                objdef = dobj_ins.getNextCategory();
                objdefs.add( objdef );
            } 
            if ( next_kind == Kind.PRIMITIVE ) {
                drawobj = dobj_ins.getNextPrimitive();
                System.out.println( (++Nobjs) + ", " + drawobj );
            }
        }
        dobj_ins.close();
        /* */    Date time3 = new Date();
        System.out.println( "\n\t Shadow Category Definitions : " );
        Iterator shadefs_itr = shadefs.entrySet().iterator();
        while ( shadefs_itr.hasNext() )
            System.out.println( shadefs_itr.next() );

        System.out.println( "\n\t Primitive Category Definitions : " );
        Iterator objdefs_itr = objdefs.iterator();
        while ( objdefs_itr.hasNext() ) {
            objdef = (Category) objdefs_itr.next();
            System.out.println( objdef.toString() );
        }

        System.out.println( "\n\t YCoordMap Definitions : " );
        Iterator ymaps_itr = ycoordmaps.iterator();
        while ( ymaps_itr.hasNext() ) {
            ycoordmap = (YCoordMap) ymaps_itr.next();
            System.out.println( ycoordmap.toString() );
        }

        System.out.println( "\n" );
        System.out.println( "Number of Primitives = " + Nobjs );
        System.out.println( "Number of Unmatched Events = "
                          + dobj_ins.getNumberOfUnMatchedEvents() );

        System.out.println( "Total ByteSize of the logfile = "
                          + dobj_ins.getTotalBytesRead() );
        // System.out.println( "time1 = " + time1 + ", " + time1.getTime() );
        // System.out.println( "time2 = " + time2 + ", " + time2.getTime() );
        // System.out.println( "time3 = " + time3 + ", " + time3.getTime() );
        System.out.println( "timeElapsed between 1 & 2 = "
                          + ( time2.getTime() - time1.getTime() ) + " msec" );
        System.out.println( "timeElapsed between 2 & 3 = "
                          + ( time3.getTime() - time2.getTime() ) + " msec" );
    }
}
