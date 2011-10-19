/*
 *  (C) 2001 by Argonne National Laboratory
 *      See COPYRIGHT in top-level directory.
 */

/*
 *  @author  Anthony Chan
 */

package viewer.zoomable;

import java.awt.*;
import javax.swing.*;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.util.Map;
import java.net.URL;

import cern.colt.map.OpenIntIntHashMap;

import base.drawable.Coord;
import base.drawable.Coord_TimeRowID;
import base.drawable.Drawable;
import viewer.common.Const;
import viewer.common.Dialogs;
// import viewer.zoomable.InfoDialog;
// import viewer.zoomable.ModelTime;
// import viewer.zoomable.YaxisMaps;
// import viewer.zoomable.YaxisTree;

public class InfoDialogForDrawable extends InfoDialog
{
    private static final long       serialVersionUID = 13200L;

    private ModelTime          time_model;
    private YaxisMaps          y_maps;
    private YaxisTree          tree_view;
    private Drawable           clk_dobj;
    private JButton            left_btn;
    private JButton            home_btn;
    private JButton            right_btn;
    private Map                map_line2treeleaf;
    private OpenIntIntHashMap  map_line2row;

    private Coord_TimeRowID  marker_vtx;

    public InfoDialogForDrawable( final Frame            frame, 
                                  final Drawable         clicked_dobj,
                                  final Coord_TimeRowID  clicked_vtx,
                                  final String[]         y_colnames,
                                        YaxisMaps        yaxis_maps,
                                        ModelTime        tmodel )
    {
        super( frame, "Drawable Info Box", clicked_vtx.time );
        clk_dobj          = clicked_dobj;

        // Initialize marker_vtx with clicked_vtx 
        // marker_vtx is a local copy as it is updated by action listeners here.
        marker_vtx        = new Coord_TimeRowID( clicked_vtx.time,
                                                 clicked_vtx.rowID );

        time_model        = tmodel;
        y_maps            = yaxis_maps;
        tree_view         = y_maps.getTreeView();
        map_line2treeleaf = y_maps.getMapOfLineIDToTreeLeaf();

        map_line2row      = y_maps.getMapOfLineIDToRowID();
        if ( map_line2row == null ) {
            if ( ! y_maps.update() )
                Dialogs.error( SwingUtilities.windowForComponent( this ),
                               "Error in updating YaxisMaps!" );
            map_line2row = y_maps.getMapOfLineIDToRowID();
        }

        Container root_panel = this.getContentPane();
        root_panel.setLayout( new BoxLayout( root_panel, BoxLayout.Y_AXIS ) );

        root_panel.add( new InfoPanelForDrawable( map_line2treeleaf,
                                                  y_colnames, clk_dobj ) );

        JPanel close_btn_panel = new JPanel();
        close_btn_panel.setLayout( new BoxLayout( close_btn_panel,
                                                  BoxLayout.X_AXIS ) );
        close_btn_panel.add( InfoDialog.BOX_RIGID );
        close_btn_panel.add( InfoDialog.BOX_GLUE );
        // Only State/Arrow drawable needs left and right buttons.
        URL     icon_URL;
        if ( ! clk_dobj.getCategory().getTopology().isEvent() ) {
            icon_URL = getURL( Const.IMG_PATH + "Backward24.gif" );
            if ( icon_URL != null )
                left_btn = new JButton( new ImageIcon( icon_URL ) );
            else
                left_btn = new JButton( "StartVertex" );
            left_btn.setMargin( Const.SQ_BTN1_INSETS );
            left_btn.setToolTipText(
                     "Scroll to the Drawable's Starting Vertex." );
            left_btn.setAlignmentX( Component.LEFT_ALIGNMENT );
            left_btn.addActionListener( new ScrollLeftActionListener() );
            close_btn_panel.add( left_btn );
        }
        else
            left_btn = null;

            icon_URL = getURL( Const.IMG_PATH + "Home24.gif" );
            if ( icon_URL != null )
                home_btn = new JButton( new ImageIcon( icon_URL ) );
            else
                home_btn = new JButton( "Home" );
            home_btn.setMargin( Const.SQ_BTN1_INSETS );
            home_btn.setToolTipText(
                     "Scroll back to the Drawable's Right-Clicked Location." );
            home_btn.setAlignmentX( Component.CENTER_ALIGNMENT );
            home_btn.addActionListener( new ScrollHomeActionListener() );
            close_btn_panel.add( home_btn );

        if ( ! clk_dobj.getCategory().getTopology().isEvent() ) {
            icon_URL = getURL( Const.IMG_PATH + "Forward24.gif" );
            if ( icon_URL != null )
                right_btn = new JButton( new ImageIcon( icon_URL ) );
            else
                right_btn = new JButton( "FinalVertex" );
            right_btn.setMargin( Const.SQ_BTN1_INSETS );
            right_btn.setToolTipText(
                      "Scroll to the Drawable's Ending Vertex." );
            right_btn.setAlignmentX( Component.RIGHT_ALIGNMENT );
            right_btn.addActionListener( new ScrollRightActionListener() );
            close_btn_panel.add( right_btn );
        }
        else
            right_btn = null;
        close_btn_panel.add( InfoDialog.BOX_GLUE );
        super.initCloseButton();
        close_btn_panel.add( super.getCloseButton() );
        close_btn_panel.setAlignmentX( Component.LEFT_ALIGNMENT );
        super.finalizeCloseButtonPanel( close_btn_panel );
        root_panel.add( close_btn_panel );
    }

    public Drawable getDrawable()
    {
       return clk_dobj;
    }

    public void updateMarkerVertex()
    {
        marker_vtx.rowID = clk_dobj.getRowIDAtTime( map_line2row,
                                                    marker_vtx.time );
    }

    public Coord_TimeRowID getMarkerVertex()
    {
        return marker_vtx;
    }

    private class ScrollLeftActionListener implements ActionListener
    {
        public void actionPerformed( ActionEvent evt )
        {
            Coord start_vtx  = clk_dobj.getStartVertex();
            marker_vtx.time  = start_vtx.time;
            marker_vtx.rowID = (float) map_line2row.get( start_vtx.lineID );
            time_model.centerTimeViewPosition( marker_vtx.time );
            tree_view.scrollRowToVisible( (int) marker_vtx.rowID );
            /*
            if ( left_btn != null )
                left_btn.setEnabled( false );
            // home_btn.setEnabled( true );
            if ( right_btn != null )
                right_btn.setEnabled( true );
            */
        }
    }

    private class ScrollHomeActionListener implements ActionListener
    {
        public void actionPerformed( ActionEvent evt )
        {
            marker_vtx.time  = InfoDialogForDrawable.super.getClickedTime();
            marker_vtx.rowID = clk_dobj.getRowIDAtTime( map_line2row,
                                                        marker_vtx.time );
            time_model.centerTimeViewPosition( marker_vtx.time );
            tree_view.scrollRowToVisible( (int) marker_vtx.rowID );
            /*
            if ( left_btn != null )
                left_btn.setEnabled( true );
            // home_btn.setEnabled( false );
            if ( right_btn != null )
                right_btn.setEnabled( true );
            */
        }
    }

    private class ScrollRightActionListener implements ActionListener
    {
        public void actionPerformed( ActionEvent evt )
        {
            Coord final_vtx = clk_dobj.getFinalVertex();
            marker_vtx.time = final_vtx.time;
            marker_vtx.rowID = (float) map_line2row.get( final_vtx.lineID );
            time_model.centerTimeViewPosition( marker_vtx.time );
            tree_view.scrollRowToVisible( (int) marker_vtx.rowID );
            /*
            if ( left_btn != null )
                left_btn.setEnabled( true );
            // home_btn.setEnabled( true );
            if ( right_btn != null )
                right_btn.setEnabled( false );
            */
        }
    }
}
