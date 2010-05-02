/*
 *  (C) 2001 by Argonne National Laboratory
 *      See COPYRIGHT in top-level directory.
 */

/*
 *  @author  Anthony Chan
 */

package logformat.clog2TOdrawable;

import logformat.clog2.RecComm;
import logformat.clog2.LineID;

public class CommProcThdID
{
    public         int       icomm;      // created comm's ID
    public         int       rank;       // rank of the process in icomm
    public         int       thd;        // thread ID in process rank
    public         int       wrank;      // MPI_COMM_WORLD rank of the process
    public         int       etype;      // type of communicator creation
    public         int       gthdLineID; // lineID used in drawable
    private        boolean   isUsed;

    public CommProcThdID( RecComm comm_rec, int in_thdID )
    {
        icomm       = comm_rec.icomm;
        rank        = comm_rec.rank;
        thd         = in_thdID;
        wrank       = comm_rec.wrank;
        etype       = comm_rec.etype.intValue();
        gthdLineID  = LineID.computeGlobalThreadID( icomm, rank, thd );
        isUsed      = false;
    }

    public void setUsed( boolean flag )
    {
        isUsed  = flag;
    }

    public boolean isUsed()
    {
        return isUsed;
    }

    public String toString()
    {
     return ( "CommProcThdID"
               + "[ icomm=" + icomm
               + ", rank=" + rank
               + ", thd=" + thd
               + ", wrank=" + wrank
               + ", etype=" + RecComm.toCommTypeString( etype )
               + ", gthdLineID=" + gthdLineID
               + ", isUsed=" + isUsed
               + " ]");
    }
}
