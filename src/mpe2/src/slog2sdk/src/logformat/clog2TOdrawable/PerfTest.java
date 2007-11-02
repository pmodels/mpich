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

public class PerfTest
{
    public static final void main( String args[] )
    {
        String                 filename;
        InputLog               dobj_ins;
        List                   objdefs;   // Primitive def'n
        Map                    shadefs;   // Shadow   def'n
        Kind                   next_kind;
        Category               objdef;
        Topology               topo;
        Primitive              drawobj;
        long                   Nobjs;


        if ( args.length != 1 ) {
            System.err.println( "This program needs 1 argument: filename." );
            System.err.println( "\tUsage: clog2TOdrawable.PerfTimer filename" );
            System.exit( 0 );
        }

        filename = args[ 0 ];
        objdefs  = new ArrayList();
        shadefs  = new HashMap();
        Nobjs    = 0;

        Date time1, time2;

        dobj_ins = new InputLog( filename );
        time1 = new Date();
        while ( ( next_kind = dobj_ins.peekNextKind() ) != Kind.EOF ) {
            if ( next_kind == Kind.TOPOLOGY ) {
                topo = dobj_ins.getNextTopology();
                objdef = Category.getShadowCategory( topo );
                objdefs.add( objdef );
                shadefs.put( topo, objdef );
            }
            if ( next_kind == Kind.YCOORDMAP ) {
                // invoke InputLog().getNextYCoordMap() to get stream moving
                dobj_ins.getNextYCoordMap();
            }
            if ( next_kind == Kind.CATEGORY ) {
                objdef = dobj_ins.getNextCategory();
                objdefs.add( objdef );
            } 
            if ( next_kind == Kind.PRIMITIVE ) {
                drawobj = dobj_ins.getNextPrimitive();
                Nobjs++;
                // System.out.println( vport_Nobjs + ", " + drawobj );
            }
        }
        time2 = new Date();
        dobj_ins.close();
        /*
        System.err.println( "\n\t Shadow Category Definitions : " );
        Iterator shadefs_itr = shadefs.entrySet().iterator();
        while ( shadefs_itr.hasNext() )
            System.err.println( shadefs_itr.next() );

        System.err.println( "\n\t Primitive Category Definitions : " );
        Iterator objdefs_itr = objdefs.iterator();
        while ( objdefs_itr.hasNext() ) {
            objdef = (Category) objdefs_itr.next();
            System.err.println( objdef.toString() );
        }
        */

        System.err.println( "\n" );
        System.err.println( "Total Number of Primitives read = "
                          + Nobjs );
        System.err.println( "Total ByteSize of the logfile read = "
                          + dobj_ins.getTotalBytesRead() );
        // System.err.println( "time1 = " + time1 + ", " + time1.getTime() );
        // System.err.println( "time2 = " + time2 + ", " + time2.getTime() );
        // System.err.println( "time3 = " + time3 + ", " + time3.getTime() );
        System.err.println( "timeElapsed between 1 & 2 = "
                          + ( time2.getTime() - time1.getTime() ) + " msec" );
    }
}
