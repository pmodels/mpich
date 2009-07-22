/*
 *  (C) 2001 by Argonne National Laboratory
 *      See COPYRIGHT in top-level directory.
 */

/*
 *  @author  Anthony Chan
 */

package logformat.clogTOdrawable;

class NoMatchingEventException extends Exception 
{
    private static final long  serialVersionUID = 200L;

    public NoMatchingEventException( String str )
    {
        super( str );
    }
}
