/*
 *  (C) 2001 by Argonne National Laboratory
 *      See COPYRIGHT in top-level directory.
 */

/*
 *  @author  Anthony Chan
 */

package viewer.zoomable;

import java.awt.event.*;

public class ActionSearchForward implements ActionListener
{
    private ViewportTimeYaxis  canvas_vport;

    public ActionSearchForward( ViewportTimeYaxis  in_vport )
    {
        canvas_vport  = in_vport;
    }

    public void actionPerformed( ActionEvent event )
    {
        canvas_vport.searchForward();

        if ( Debug.isActive() )
            Debug.println( "Action for Search Forward button. " );
    }
}
