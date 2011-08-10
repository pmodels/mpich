/*
 *  (C) 2001 by Argonne National Laboratory
 *      See COPYRIGHT in top-level directory.
 */

/*
 *  @author  Anthony Chan
 */

package viewer.zoomable;

import java.net.URL;
import java.awt.*;
// import java.awt.event.*;
import javax.swing.*;

import viewer.common.Const;
// import viewer.common.Dialogs;

public class InfoDialog extends JDialog
{
    private static final long  serialVersionUID = 3800L;

    public  static final Component  BOX_GLUE  = Box.createHorizontalGlue();
    public  static final Component  BOX_STRUT = Box.createHorizontalStrut(10);
    public  static final Component  BOX_RIGID = Box.createRigidArea(
                                                    new Dimension(40,0));

    private   JPanel   btn_panel;
    private   JButton  close_btn;
    private   double   clicked_time;

    public InfoDialog( final Frame   ancestor_frame,
                             String  title_str,
                             double  time )
    {
        super( ancestor_frame, title_str );
        clicked_time = time;
    }

    public InfoDialog( final Dialog  ancestor_dialog,
                             String  title_str,
                             double  time )
    {
        super( ancestor_dialog, title_str );
        clicked_time = time;
    }

    public void initCloseButton()
    {
        URL icon_URL;

        super.setDefaultCloseOperation( WindowConstants.DO_NOTHING_ON_CLOSE );
        /*
           Create a Close Button here but its ActionListener will be
           implemented in ViewportTime where List of InfoDialog lives
           so the Close ActionListener can do the clean up.
        */
        icon_URL = getURL( Const.IMG_PATH + "Stop24.gif" );
        if ( icon_URL != null )
            close_btn = new JButton( new ImageIcon( icon_URL ) );
        else
            close_btn = new JButton( "Close" );
        close_btn.setMargin( Const.SQ_BTN1_INSETS );
        close_btn.setToolTipText( "Close this Dialog box." );
        close_btn.setAlignmentX( Component.CENTER_ALIGNMENT );
    }

    // Provides an handle to Close button so
    // 1) ViewportTime can implement cleaning up ActionListener
    // 2) subclass can decide where close button should locate.
    public JButton getCloseButton()
    {
        return close_btn;
    }

    public void finalizeCloseButtonPanel( JPanel btn_panel )
    {
        Dimension  panel_max_size;
        panel_max_size        = btn_panel.getPreferredSize();
        panel_max_size.width  = Short.MAX_VALUE;
        btn_panel.setMaximumSize( panel_max_size );
        // addWindowListener( new InfoDialogWindowListener( this ) );
    }

    public double getClickedTime()
    {
        return clicked_time;
    }

    public void setVisibleAtLocation( final Point global_pt )
    {
        this.setLocation( global_pt );
        this.pack();
        this.setVisible( true );
        this.toFront();
    }

    /*
    private class InfoDialogWindowListener extends WindowAdapter
    {
        private InfoDialog  info_popup;

        public InfoDialogWindowListener( InfoDialog info )
        {
            info_popup = info;
        }

        public void windowClosing( WindowEvent e )
        {
            // info_popup.dispose();
            Dialogs.info( info_popup, "Use the CLOSE button please!", null );
        }
    }
    */

    protected URL getURL( String filename )
    {
        URL url = null;
        url = getClass().getResource( filename );
        return url;
    }
}
