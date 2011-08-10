/*
 *  (C) 2001 by Argonne National Laboratory
 *      See COPYRIGHT in top-level directory.
 */

/*
 *  @author  Anthony Chan
 */

package base.drawable;

// This class is NOT similar to Coord 'casuse the "int lineID" of Coord
// is replaced by "float rowID". It seems it is a real number version
// Coord.  But it is more than that. "float rowID" is used to store
// the rowID, Y-axis identifier or row in YaxisTree( a JTree ).
// There is an extra translation between lineID and rowID through
// YaxisMaps.map_line2row. 
public class Coord_TimeRowID
{
    public double time;  // time
    public float  rowID; // y axis rowID for the YaxisTree,
                         // not timeline ID of Drawable's Coord.

    public Coord_TimeRowID( double in_time, float in_task )
    {
        time  = in_time;
        rowID = in_task;
    }
}
