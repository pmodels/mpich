/*
 *  (C) 2001 by Argonne National Laboratory
 *      See COPYRIGHT in top-level directory.
 */

/*
 *  @author  Anthony Chan
 */

package viewer.timelines;

import java.awt.*;
import javax.swing.*;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.util.Map;

import base.drawable.Drawable;
import viewer.zoomable.InfoDialog;
import viewer.zoomable.ModelTime;

public class InfoDialogForDrawable extends InfoDialog
{
    private static final  long  serialVersionUID = 13200L;
    private ModelTime time_model;
    private double    clk_time_pos;
    private JButton   left_btn;
    private JButton   home_btn;
    private JButton   right_btn;

    public InfoDialogForDrawable( final Frame     frame, 
                                  final double    clicked_time,
                                  final Map       map_line2treenodes,
                                  final String[]  y_colnames,
                                        ModelTime tmodel,
                                  final Drawable  clicked_dobj )
    {
        super( frame, "Drawable Info Box", clicked_time );
        time_model    = tmodel;
        clk_time_pos  = clicked_time;

        Container root_panel = this.getContentPane();
        root_panel.setLayout( new BoxLayout( root_panel, BoxLayout.Y_AXIS ) );

        root_panel.add( new InfoPanelForDrawable( map_line2treenodes,
                                                  y_colnames, clicked_dobj ) );

        JPanel btn_panel = super.getCloseButtonPanel();
        left_btn   = new JButton( "left" );
        home_btn   = new JButton( "home" );
        right_btn  = new JButton( "right" );
        left_btn.setAlignmentX( Component.LEFT_ALIGNMENT );
        home_btn.setAlignmentX( Component.LEFT_ALIGNMENT );
        right_btn.setAlignmentX( Component.RIGHT_ALIGNMENT );
        left_btn.addActionListener( new ActionListener() {
             public void actionPerformed( ActionEvent evt )
             {
                 double time = clicked_dobj.getStartVertex().time;
                 time_model.centerTimeViewPosition( time );
             }
        } );
        home_btn.addActionListener( new ActionListener() {
             public void actionPerformed( ActionEvent evt )
             {
                 time_model.centerTimeViewPosition( clk_time_pos );
             }
        } );
        right_btn.addActionListener( new ActionListener() {
             public void actionPerformed( ActionEvent evt )
             {
                 double time = clicked_dobj.getFinalVertex().time;
                 time_model.centerTimeViewPosition( time );
             }
        } );
        btn_panel.add( left_btn );
        btn_panel.add( home_btn );
        btn_panel.add( right_btn );
        root_panel.add( btn_panel );
    }
}
