/*
 *  (C) 2001 by Argonne National Laboratory
 *      See COPYRIGHT in top-level directory.
 */

/*
 *  @author  Anthony Chan
 */

package logformat.clog2;

import java.util.StringTokenizer;
import java.util.NoSuchElementException;
import java.io.DataInputStream;
import java.io.IOException;

// Class corresponds to CLOG_Premable_t
public class Preamble
{
    // BYTESIZE corresponds to CLOG_PREAMBLE_SIZE
    private final static int      BYTESIZE     = 1024;
    // VERSIONSIZE corresponds to CLOG_VERSION_STRLEN;
    // private final static int      VERSIONSIZE  = 12;

    private              String   version;
    //  this correspond to CLOG_Premable_t.is_big_endian
    private              String   is_big_endian_title;
    private              boolean  is_big_endian;
    //  this correspond to CLOG_Premable_t.is_finalized_
    private              String   is_finalized_title;
    private              boolean  is_finalized;
    //  this correspond to CLOG_Premable_t.block_size
    private              String   block_size_title;
    private              int      block_size;
    //  this correspond to CLOG_Premable_t.num_buffered_blocks
    private              String   num_blocks_title;
    private              int      num_blocks;
    //  this correspond to CLOG_Premable_t.max_comm_world_size
    private              String   max_world_size_title;
    private              int      max_world_size;
    //  this correspond to CLOG_Premable_t.max_thread_count
    private              String   max_thread_count_title;
    private              int      max_thread_count;
    //  this correspond to CLOG_Premable_t.known_eventID_start
    private              String   known_eventID_start_title;
    private              int      known_eventID_start;
    //  this correspond to CLOG_Premable_t.user_eventID_start
    private              String   user_eventID_start_title;
    private              int      user_eventID_start;
    //  this correspond to CLOG_Premable_t.known_solo_eventID_start
    private              String   known_solo_eventID_start_title;
    private              int      known_solo_eventID_start;
    //  this correspond to CLOG_Premable_t.user_solo_eventID_start
    private              String   user_solo_eventID_start_title;
    private              int      user_solo_eventID_start;
    //  this correspond to CLOG_Premable_t.known_stateID_count
    private              String   known_stateID_count_title;
    private              int      known_stateID_count;
    //  this correspond to CLOG_Premable_t.user_stateID_count
    private              String   user_stateID_count_title;
    private              int      user_stateID_count;
    //  this correspond to CLOG_Premable_t.known_solo_eventID_count
    private              String   known_solo_eventID_count_title;
    private              int      known_solo_eventID_count;
    //  this correspond to CLOG_Premable_t.user_solo_eventID_count
    private              String   user_solo_eventID_count_title;
    private              int      user_solo_eventID_count;
    //  this correspond to CLOG_Premable_t.commtable_fptr
    private              String   commtable_fptr_title;
    private              long     commtable_fptr;

    public boolean readFromDataStream( DataInputStream in )
    {
        byte[]           buffer;
        StringTokenizer  tokens;
        String           str;

        buffer = new byte[ BYTESIZE ];
        try {
            in.readFully( buffer );
        } catch ( IOException ioerr ) {
            ioerr.printStackTrace();
            return false;
        }

        tokens  = new StringTokenizer( new String( buffer ), "\0" );
        try {
            version = tokens.nextToken().trim();
            is_big_endian_title = tokens.nextToken().trim();
            str                 = tokens.nextToken().trim();
            is_big_endian       =  str.equalsIgnoreCase( "true" )
                                || str.equalsIgnoreCase( "yes" );
            is_finalized_title  = tokens.nextToken().trim();
            str                 = tokens.nextToken().trim();
            is_finalized        =  str.equalsIgnoreCase( "true" )
                                || str.equalsIgnoreCase( "yes" );
            block_size_title    = tokens.nextToken().trim();
            block_size          = Integer.parseInt( tokens.nextToken().trim() );
            num_blocks_title    = tokens.nextToken().trim();
            num_blocks          = Integer.parseInt( tokens.nextToken().trim() );
            max_world_size_title            = tokens.nextToken().trim();
            max_world_size                  = Integer.parseInt(
                                              tokens.nextToken().trim() );
            max_thread_count_title          = tokens.nextToken().trim();
            max_thread_count                = Integer.parseInt(
                                              tokens.nextToken().trim() );
            known_eventID_start_title       = tokens.nextToken().trim();
            known_eventID_start             = Integer.parseInt(
                                              tokens.nextToken().trim() );
            user_eventID_start_title        = tokens.nextToken().trim();
            user_eventID_start              = Integer.parseInt(
                                              tokens.nextToken().trim() );
            known_solo_eventID_start_title  = tokens.nextToken().trim();
            known_solo_eventID_start        = Integer.parseInt(
                                              tokens.nextToken().trim() );
            user_solo_eventID_start_title   = tokens.nextToken().trim();
            user_solo_eventID_start         = Integer.parseInt(
                                              tokens.nextToken().trim() );
            known_stateID_count_title       = tokens.nextToken().trim();
            known_stateID_count             = Integer.parseInt(
                                              tokens.nextToken().trim() );
            user_stateID_count_title        = tokens.nextToken().trim();
            user_stateID_count              = Integer.parseInt(
                                              tokens.nextToken().trim() );
            known_solo_eventID_count_title  = tokens.nextToken().trim();
            known_solo_eventID_count        = Integer.parseInt(
                                              tokens.nextToken().trim() );
            user_solo_eventID_count_title   = tokens.nextToken().trim();
            user_solo_eventID_count         = Integer.parseInt(
                                              tokens.nextToken().trim() );
            commtable_fptr_title            = tokens.nextToken().trim();
            commtable_fptr                  = Long.parseLong(
                                              tokens.nextToken().trim() )
                                            * Long.parseLong(
                                              tokens.nextToken().trim() )
                                            + Long.parseLong(
                                              tokens.nextToken().trim() );
        } catch ( NoSuchElementException err ) {
            err.printStackTrace();
            return false;
        } catch ( NumberFormatException err ) {
            err.printStackTrace();
            return false;
        }

        /* Set the constants in (icomm,rank,thread) -> lineID transformation */
        LineID.setCommRank2LineIDxForm( max_world_size, max_thread_count );

        return true;
    }

    public String  getVersionString()
    { return version; }

    public boolean isVersionMatched()
    { return version.equalsIgnoreCase( Const.VERSION ); }

    public boolean isVersionCompatible()
    {
        String   old_version;
        boolean  isCompatible;
        int      idx;

        isCompatible = false;
        for ( idx = 0;
              idx < Const.COMPAT_VERSIONS.length && !isCompatible;
              idx++ ) {
            old_version   = Const.COMPAT_VERSIONS[ idx ];
            isCompatible  = version.equalsIgnoreCase( old_version );
        }
        return isCompatible;
    }

    public boolean isBigEndian()
    { return is_big_endian; }

    public boolean isFinalized()
    { return is_finalized; }

    public int getBlockSize()
    { return block_size; }

    public int getMaxCommWorldSize()
    { return max_world_size; }

    public int getMaxThreadCount()
    { return max_thread_count; }

    public int getKnownEventIDStart()
    { return known_eventID_start; }

    public int getUserEventIDStart()
    { return user_eventID_start; }

    public int getKnownSoloEventIDStart()
    { return known_solo_eventID_start; }

    public int getUserSoloEventIDStart()
    { return user_solo_eventID_start; }

    public int getKnownStateIDCount()
    { return known_stateID_count; }

    public int getUserStateIDCount()
    { return user_stateID_count; }

    public int getKnownSoloEventIDCount()
    { return known_solo_eventID_count; }

    public int getUserSoloEventIDCount()
    { return user_solo_eventID_count; }

    public long getCommTableFptr()
    { return commtable_fptr; }

    public String toString()
    {
         return ( version + "\n"
                + is_big_endian_title + is_big_endian + "\n"
                + is_finalized_title + is_finalized + "\n"
                + block_size_title + block_size + "\n"
                + num_blocks_title + num_blocks + "\n"
                + max_world_size_title + max_world_size + "\n"
                + max_thread_count_title + max_thread_count + "\n"
                + known_eventID_start_title + known_eventID_start + "\n"
                + user_eventID_start_title + user_eventID_start + "\n"
                + known_solo_eventID_start_title + known_solo_eventID_start+"\n"
                + user_solo_eventID_start_title + user_solo_eventID_start + "\n"
                + known_stateID_count_title + known_stateID_count + "\n"
                + user_stateID_count_title + user_stateID_count + "\n"
                + known_solo_eventID_count_title + known_solo_eventID_count+"\n"
                + user_solo_eventID_count_title + user_solo_eventID_count + "\n"
                + commtable_fptr_title + commtable_fptr + "\n"
                );
    }
}
