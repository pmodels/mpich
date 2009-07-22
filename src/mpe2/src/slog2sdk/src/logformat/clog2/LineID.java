/*
 *  (C) 2001 by Argonne National Laboratory
 *      See COPYRIGHT in top-level directory.
 */

/*
 *  @author  Anthony Chan
 */

package logformat.clog2;

// import java.io.*;
// import java.util.*;


public class LineID
{
    private static int MaxCommWorldSize = 0;
    private static int MaxThreadCount   = 0;
    private static int MaxCommThdCount  = 0;

    public static void setCommRank2LineIDxForm( int max_comm_world_size,
                                                int max_thread_count )
    {
        MaxCommWorldSize  = max_comm_world_size;
        MaxThreadCount    = max_thread_count;
        MaxCommThdCount   = MaxCommWorldSize * MaxThreadCount;
    }

    /*
       The lineIDs generated with this formula are distinguishable from
       each other as long as CommWorldSize is max number of ranks within
       MPI_COMM_WORLD, true for MPI-1.
    */

    /* 
       The lineID is Global Unique Thread LineID which can ONLY be compared
       to its kind, i.e. can't be compared to Global Unique Process LineID.
    */
    public static int computeGlobalThreadID( int icomm, int rank, int thread )
    {
        return icomm * MaxCommThdCount + rank * MaxThreadCount + thread;
    }

    /* 
       The lineID is Global Unique Process LineID which can ONLY be compared
       to its kind, i.e. can't be compared to Global Unique Thread LineID.
    */
    public static int computeGlobalProcessID( int icomm, int rank )
    {
        return icomm * MaxCommWorldSize + rank;
    }
}
