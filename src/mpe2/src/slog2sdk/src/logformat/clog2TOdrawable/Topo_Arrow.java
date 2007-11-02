/*
 *  (C) 2001 by Argonne National Laboratory
 *      See COPYRIGHT in top-level directory.
 */

/*
 *  @author  Anthony Chan
 */

package logformat.clog2TOdrawable;

import java.util.List;
import java.util.LinkedList;
import java.util.Iterator;

import logformat.clog2.RecHeader;
import logformat.clog2.RecMsg;
import base.drawable.Coord;
import base.drawable.Topology;
import base.drawable.Category;
import base.drawable.Primitive;

public class Topo_Arrow extends Topology 
                        implements TwoEventsMatching
{
    private static   Class[]   argtypes      = null;
    private          List      sends         = null;
    private          List      recvs         = null;

    private          Category  type          = null;

    public Topo_Arrow()
    {
        super( Topology.ARROW_ID );
        if ( argtypes == null )
            argtypes = new Class[] { RecHeader.class, RecMsg.class };
        sends = new LinkedList();
        recvs = new LinkedList();
    }

/*
    public Topo_Arrow( String in_name )
    {
        super( in_name );
        if ( argtypes == null )
            argtypes = new Class[] { RecHeader.class, RecMsg.class };
        sends = new LinkedList();
        recvs = new LinkedList();
    }
*/

    public void setCategory( final Category in_type )
    {
        type = in_type;
    }

    public Category getCategory()
    {
        return type;
    }

    public Primitive matchStartEvent( final RecHeader header,
                                      final RecMsg    msg )
    {
        Obj_Arrow  arrow;

        // Check the recvs event list to see if there is any matching event
        // for the incoming send event
        Iterator itr = recvs.iterator();
        while ( itr.hasNext() ) {
            arrow = ( Obj_Arrow ) itr.next();
            if (    arrow.getMsgTag() == msg.tag
                 && arrow.getStartProcessLineID() == header.getProcessLineID()
                 && arrow.getFinalProcessLineID() == msg.getProcessLineID()
               ) {
                itr.remove();  // remove the partial arrow from recvs list
                arrow.setStartVertex( new Coord( header.time,
                                                 header.gthdLineID ) );
                arrow.setInfoBuffer();
                return arrow;
            }
        }

        // Set the StartVertex with header's time and gthdLineID.
        // Don't know the FinalVertex's time and gthdLineID yet.
        arrow = new Obj_Arrow( this.getCategory() );
        arrow.setStartVertex( new Coord( header.time, header.gthdLineID ) );
        arrow.setProcessLineIDs( header.getProcessLineID(),
                                 msg.getProcessLineID() );
        arrow.setMsgTag( msg.tag );
        // Arrow's msg size is determined by the send event NOT recv event
        arrow.setMsgSize( msg.size ); 
        sends.add( arrow );

        return null;
    }

    public Primitive matchFinalEvent( final RecHeader header,
                                      final RecMsg    msg )
    {
        Obj_Arrow  arrow;

        // Check the sends event list to see if there is any matching event
        // for the incoming recv event
        Iterator itr = sends.iterator();
        while ( itr.hasNext() ) {
            arrow = ( Obj_Arrow ) itr.next();
            if (    arrow.getMsgTag() == msg.tag
                 && arrow.getStartProcessLineID() == msg.getProcessLineID()
                 && arrow.getFinalProcessLineID() == header.getProcessLineID()
               ) {
                itr.remove();  // remove the partial arrow from sends list
                arrow.setFinalVertex( new Coord( header.time,
                                                 header.gthdLineID ) );
                arrow.setInfoBuffer();
                return arrow;
            }
        }

        // Set the FinalVertex with header's time and gthdLineID.
        // Don't know the StartVertex's time and gthdLineID yet.
        arrow = new Obj_Arrow( this.getCategory() );
        arrow.setFinalVertex( new Coord( header.time, header.gthdLineID ) );
        arrow.setProcessLineIDs( msg.getProcessLineID(),
                                 header.getProcessLineID() );
        arrow.setMsgTag( msg.tag );
        recvs.add( arrow );

        return null;
    }

    public ObjMethod getStartEventObjMethod()
    {
        ObjMethod obj_fn = new ObjMethod();
        obj_fn.obj = this;
        try {
            obj_fn.method = this.getClass().getMethod( "matchStartEvent",
                                                       argtypes );
        } catch ( NoSuchMethodException err ) {
            err.printStackTrace();
            return null;
        }
        return obj_fn;
    }

    public ObjMethod getFinalEventObjMethod()
    {
        ObjMethod obj_fn = new ObjMethod();
        obj_fn.obj = this;
        try {
            obj_fn.method = this.getClass().getMethod( "matchFinalEvent",
                                                       argtypes );
        } catch ( NoSuchMethodException err ) {
            err.printStackTrace();
            return null;
        }
        return obj_fn;
    }

    public List getPartialObjects()
    {
        List partialarrows = new LinkedList();
        partialarrows.addAll( sends );
        partialarrows.addAll( recvs );
        return partialarrows;
    }
}
