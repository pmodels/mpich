/*
 *  (C) 2001 by Argonne National Laboratory
 *      See COPYRIGHT in top-level directory.
 */

/*
 *  @author  Anthony Chan
 */

package logformat.clog2;

import java.io.*;
// import java.util.*;


// Class corresponds to CLOG_Rec_CargoEvt
public class RecCargo
{
    public  static final int RECTYPE  = Const.RecType.CARGOEVT;
    private static final int BYTESIZE = 4 + 4 
                                      + StrBytes.BYTESIZE;
    public         Integer etype;                  // bare event number
    // private static int     pad;
    public         byte[]  bytes;                  // byte data

    //read the record from the given input stream
    public int readFromDataStream( MixedDataInputStream in )
    {
        try {
            etype   = new Integer( in.readInt() );
            // pad     = in.readInt();
            in.skipBytes( 4 );
            bytes   = new byte[ StrBytes.BYTESIZE ];
            in.readFully( bytes );
        } catch ( IOException ioerr ) {
            ioerr.printStackTrace();
            return 0;
        }

        return BYTESIZE;
    }

    public int skipBytesFromDataStream( DataInputStream in )
    {
        try {
            in.skipBytes( BYTESIZE );
        } catch ( IOException ioerr ) {
            ioerr.printStackTrace();
            return 0;
        }

        return BYTESIZE;
    }

    public String toString()
    {
        return ( "RecCargo"
               + "[ etype=" + etype
               // + ", pad=" + pad
               + ", bytes=" + new String( bytes )
               // + ", BYTESIZE=" + BYTESIZE
               + " ]" );
    }
}
