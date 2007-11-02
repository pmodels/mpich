/*
 *  (C) 2001 by Argonne National Laboratory
 *      See COPYRIGHT in top-level directory.
 */

/*
 *  @author  Anthony Chan
 */

package logformat.clog2TOdrawable;

import java.util.Map;
import java.util.TreeMap;
import java.util.List;
import java.util.ArrayList;
import java.util.Iterator;

import logformat.clog2.RecComm;

import base.drawable.Coord;
import base.drawable.Primitive;
import base.drawable.YCoordMap;

/*
   Define a TreeMap where the order is determined by gthdLineID in
   CommProcThdID class.
   The object that use this map is drawable whose lineID field is int and
   not an Object.  In order to avoid creating Integer Object of lineID
   for every drawable from clog2 file to search CommProcThdID in this class,
   we will use Coord as key because lineID field in drawable is embedded
   within its Coord member.  The class will use Comparator in
   Coord's LINEID_ORDER to sort/search lineID.
*/
public class CommProcThdIDMap extends TreeMap
{
    private int  max_thread_count;

    public CommProcThdIDMap( int max_thread_count )
    {
        super( Coord.LINEID_ORDER );
        this.max_thread_count = max_thread_count;
    }

    public void addComm( RecComm comm )
    {
        /*
           Added all possible thread lineIDs, 0...max_thread_count-1,
           for the given RecComm event.  A more scalable way of doing
           this is to log the thread event for the newly created thread.
        */
        CommProcThdID  cptID;
        int            thd;
        for ( thd = 0; thd < max_thread_count; thd++ ) {
             cptID = new CommProcThdID( comm, thd );
             super.put( new Coord( 0.0, cptID.gthdLineID ), cptID );
        }
    }

    public void setCommProcThdIDUsed( Primitive  prime )
    {
        CommProcThdID  cptID;

        /* State and Event only need to have its 1st vertex checked */
        cptID = (CommProcThdID) super.get( prime.getStartVertex() );
        if ( cptID == null ) {
            System.err.println( "CommProcThdIDMap: missing CommProcThdID "
                              + "element with gthdLineID "
                              + prime.getStartVertex().lineID + "!" );
        }
        cptID.setUsed( true );
        if ( prime.getCategory().getTopology().isArrow() ) {
            cptID = (CommProcThdID) super.get( prime.getFinalVertex() );
            if ( cptID == null ) {
                System.err.println( "CommProcThdIDMap: missing CommProcThdID "
                                  + "element with gthdLineID "
                                  + prime.getFinalVertex().lineID + "!" );
            }
            cptID.setUsed( true );
        }
    }

    public void initialize() {}

    // Cannot use finalize() as function name,
    // finalize() overrides Object.finalize().
    public void finish()
    {
        CommProcThdID  cptID;
        // Remove any "unUsed" CommProcThdID elements from the map;
        Iterator cptID_itr  = super.values().iterator();
        while ( cptID_itr.hasNext() ) {
            cptID = (CommProcThdID) cptID_itr.next();
            if ( ! cptID.isUsed() )
                cptID_itr.remove();
        }
    }

    public List createYCoordMapList()
    {
        CommProcThdID  cptID;
        Iterator       cptID_itr;
        List           ycoordmap_list;
        String[]       col_titles;
        int            num_rows;

        ycoordmap_list = new ArrayList( 2 );

        // Set number of rows of int[]
        num_rows       = super.size();

        if ( max_thread_count > 1 ) {
            int   wthd_elems[], gthd_elems[];
            int   num_cols4wthd, num_cols4gthd;
            int   wthd_idx, gthd_idx;

            // wthd_elems[] has 3 columns: lineID -> (wrank,thd)
            num_cols4wthd  = 3;
            wthd_idx       = 0;
            wthd_elems     = new int[ num_rows * num_cols4wthd ];

            // gthd_elems[] has 3 columns: lineID -> (icomm,rank,thd)
            num_cols4gthd  = 4;
            gthd_idx       = 0;
            gthd_elems     = new int[ num_rows * num_cols4gthd ];

            // Setting the XXXX_elems[]
            cptID_itr = super.values().iterator();
            while ( cptID_itr.hasNext() ) {
                cptID = (CommProcThdID) cptID_itr.next();

                wthd_elems[ wthd_idx++ ]   = cptID.gthdLineID;
                wthd_elems[ wthd_idx++ ]   = cptID.wrank;
                wthd_elems[ wthd_idx++ ]   = cptID.thd;
 
                gthd_elems[ gthd_idx++ ]   = cptID.gthdLineID;
                gthd_elems[ gthd_idx++ ]   = cptID.icomm;
                gthd_elems[ gthd_idx++ ]   = cptID.rank;
                gthd_elems[ gthd_idx++ ]   = cptID.thd;
            }

            // Create ProcessThreadViewMap
            YCoordMap   wthd_viewmap;
            col_titles    = new String[] {"world_rank", "thread"};
            wthd_viewmap  = new YCoordMap( num_rows, num_cols4wthd,
                                           "Process-Thread View",
                                           col_titles, wthd_elems, null );
            ycoordmap_list.add( wthd_viewmap );

            // Create CommunicatorThreadViewMap
            YCoordMap   gthd_viewmap;
            col_titles    = new String[] {"commID", "comm_rank", "thread"};
            gthd_viewmap  = new YCoordMap( num_rows, num_cols4gthd,
                                           "Communicator-Thread View",
                                           col_titles, gthd_elems, null );
            ycoordmap_list.add( gthd_viewmap );
        }
        else { // if ( max_thread_count > 1 )
            int            world_elems[], comm_elems[];
            int            num_cols4world, num_cols4comm;
            int            world_idx, comm_idx;
    
            // world_elems[] has 2 columns: lineID -> (wrank)
            num_cols4world = 2;
            world_idx      = 0;
            world_elems    = new int[ num_rows * num_cols4world ];
    
            // comm_elems[] has 3 columns: lineID -> (icomm,rank)
            num_cols4comm  = 3;
            comm_idx       = 0;
            comm_elems     = new int[ num_rows * num_cols4comm ];
    
            // Setting the XXXX_elems[]
            cptID_itr = super.values().iterator();
            while ( cptID_itr.hasNext() ) {
                cptID = (CommProcThdID) cptID_itr.next();
    
                world_elems[ world_idx++ ] = cptID.gthdLineID;
                world_elems[ world_idx++ ] = cptID.wrank;
    
                comm_elems[ comm_idx++ ]   = cptID.gthdLineID;
                comm_elems[ comm_idx++ ]   = cptID.icomm;
                comm_elems[ comm_idx++ ]   = cptID.rank;
            }
          
            // Create ProcessViewMap
            YCoordMap  world_viewmap;
            col_titles    = new String[] {"world_rank"};
            world_viewmap = new YCoordMap( num_rows, num_cols4world,
                                           "Process View",
                                           col_titles, world_elems, null );
            ycoordmap_list.add( world_viewmap );
    
            // Create CommunicatorViewMap
            YCoordMap   comm_viewmap;
            col_titles    = new String[] {"commID", "comm_rank"};
            comm_viewmap  = new YCoordMap( num_rows, num_cols4comm,
                                           "Communicator View",
                                           col_titles, comm_elems, null );
            ycoordmap_list.add( comm_viewmap );
        }  // Endof if ( max_thread_count > 1 )

        return ycoordmap_list;
    }
}
