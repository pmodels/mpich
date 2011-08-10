/*
 *  (C) 2001 by Argonne National Laboratory
 *      See COPYRIGHT in top-level directory.
 */

/*
 *  @author  Anthony Chan
 */

package viewer.histogram;

import java.awt.*;
import javax.swing.BoxLayout;
import javax.swing.JPanel;
import javax.swing.JTree;

import base.statistics.Summarizable;
import viewer.zoomable.InfoDialog;

public class InfoDialogForSummary extends InfoDialog
{
    private static final long  serialVersionUID  = 12200L;

    public InfoDialogForSummary( final Dialog        dialog, 
                                 final double        clicked_time,
                                 final JTree         tree_view,
                                 final String[]      y_colnames,
                                 final Summarizable  summarizable )
    {
        super( dialog, "Summary Info Box", clicked_time );

        Container root_panel = this.getContentPane();
        root_panel.setLayout( new BoxLayout( root_panel, BoxLayout.Y_AXIS ) );

        root_panel.add( new InfoPanelForSummary( tree_view, y_colnames,
                                                 summarizable ) );

        JPanel close_btn_panel = new JPanel();
        super.initCloseButton();
        close_btn_panel.add( super.getCloseButton() );
        close_btn_panel.setAlignmentX( Component.LEFT_ALIGNMENT );
        super.finalizeCloseButtonPanel( close_btn_panel );
        root_panel.add( close_btn_panel );
    }
}
