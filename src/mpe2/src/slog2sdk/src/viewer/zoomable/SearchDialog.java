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
import java.awt.event.*;
import javax.swing.*;

import viewer.common.Const;
import viewer.common.TopWindow;

public class SearchDialog extends JDialog 
                          implements ActionListener
{
    private static final long  serialVersionUID = 4400L;

    private Window             root_window;
    private ViewportTimeYaxis  viewport;

    private Container          root_panel;
    private JPanel             btn_panel;
    private JButton            close_btn;
    private JButton            searchBack_btn;
    private JButton            searchFore_btn;

    public SearchDialog( final Frame frame, ViewportTimeYaxis  vport )
    {
        super( frame, "Search Box" );
        root_window  = frame;
        viewport     = vport;
        this.init();
    }

/*
    public SearchDialog( final Dialog dialog, ViewportTimeYaxis  vport )
    {
        super( frame, "Search Box" );
        root_window  = dialog;
        viewport     = vport;
        this.init();
    }
*/

    private URL getURL( String filename )
    {
        URL url = null;
        url = getClass().getResource( filename );
        return url;
    }

    private void init()
    {
        super.setDefaultCloseOperation( WindowConstants.DO_NOTHING_ON_CLOSE );
        root_panel = super.getContentPane();
        root_panel.setLayout( new BoxLayout( root_panel, BoxLayout.Y_AXIS ) );

        btn_panel = new JPanel();
        btn_panel.setLayout( new BoxLayout( btn_panel, BoxLayout.X_AXIS ) );
        btn_panel.add( InfoDialog.BOX_RIGID );
        btn_panel.add( InfoDialog.BOX_GLUE );

            URL  icon_URL;
            icon_URL = getURL( Const.IMG_PATH + "FindBack24.gif" );
            if ( icon_URL != null )
                searchBack_btn = new JButton( new ImageIcon( icon_URL ) );
            else
                searchBack_btn = new JButton( "SearchBackward" );
            searchBack_btn.setMargin( Const.SQ_BTN1_INSETS );
            searchBack_btn.setToolTipText( "Search Backward in time" );
            searchBack_btn.setAlignmentX( Component.LEFT_ALIGNMENT );
            // searchBack_btn.setPreferredSize( btn_dim );
            searchBack_btn.addActionListener(
                           new ActionSearchBackward( viewport ) );
            btn_panel.add( searchBack_btn );
    
            icon_URL = getURL( Const.IMG_PATH + "FindFore24.gif" );
            if ( icon_URL != null )
                searchFore_btn = new JButton( new ImageIcon( icon_URL ) );
            else
                searchFore_btn = new JButton( "SearchForward" );
            searchFore_btn.setMargin( Const.SQ_BTN1_INSETS );
            searchFore_btn.setToolTipText( "Search Forward in time" );
            searchFore_btn.setAlignmentX( Component.RIGHT_ALIGNMENT );
            // searchFore_btn.setPreferredSize( btn_dim );
            searchFore_btn.addActionListener(
                           new ActionSearchForward( viewport ) );
            btn_panel.add( searchFore_btn );

        btn_panel.add( InfoDialog.BOX_GLUE );

            icon_URL = getURL( Const.IMG_PATH + "Stop24.gif" );
            if ( icon_URL != null )
                close_btn = new JButton( new ImageIcon( icon_URL ) );
            else
                close_btn = new JButton( "Close" );
            close_btn.setMargin( Const.SQ_BTN1_INSETS );
            close_btn.setToolTipText( "Close this dialog box." );
            close_btn.setAlignmentX( Component.CENTER_ALIGNMENT );
            close_btn.addActionListener( this );
        btn_panel.add( close_btn );
        btn_panel.setAlignmentX( Component.LEFT_ALIGNMENT );
        Dimension  panel_max_size;
        panel_max_size        = btn_panel.getPreferredSize();
        panel_max_size.width  = Short.MAX_VALUE;
        btn_panel.setMaximumSize( panel_max_size );

        super.addWindowListener( new WindowAdapter()
        {
            public void windowClosing( WindowEvent evt )
            {
                SearchDialog.this.setVisible( false );
                viewport.eraseSearchedDrawable();
                viewport.repaint();
            }
        } );

        super.setVisible( false );
    }

    private void setVisibleAtLocation( final Point global_pt )
    {
        this.setLocation( global_pt );
        this.pack();
        this.setVisible( true );
        this.toFront();
    }

    public void setVisibleAtDefaultLocation()
    {
        Rectangle rect   = null;
        Point     loc_pt = null;
        Frame     frame  = TopWindow.First.getWindow();
        if ( frame != null ) {
            rect    = frame.getBounds();
            loc_pt  = new Point( rect.x + rect.width, rect.y );
        }
        else {
            rect    = root_window.getBounds();
            loc_pt  = new Point( rect.x + rect.width, rect.y );
        }
        this.setVisibleAtLocation( loc_pt );
    }

    public void replace( Component cmpo_panel )
    {
        root_panel.removeAll();
        root_panel.add( cmpo_panel );
        root_panel.add( btn_panel );
        super.invalidate();
        super.validate();
    }

    public void actionPerformed( ActionEvent evt )
    {
        if ( evt.getSource() == close_btn ) {
            super.setVisible( false );
            viewport.eraseSearchedDrawable();
            viewport.repaint();
        }
    }

}
