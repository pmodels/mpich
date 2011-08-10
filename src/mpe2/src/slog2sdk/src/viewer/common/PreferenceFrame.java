/*
 *  (C) 2001 by Argonne National Laboratory
 *      See COPYRIGHT in top-level directory.
 */

/*
 *  @author  Anthony Chan
 */

package viewer.common;

import java.net.URL;

import java.awt.Container;
import java.awt.Component;
import java.awt.Dimension;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.WindowAdapter;
import java.awt.event.WindowEvent;
import javax.swing.*;

public class PreferenceFrame extends JFrame
                             implements ActionListener
{
    private static final long  serialVersionUID = 1600L;

    private PreferencePanel  pptys_panel;

    private JButton          update_btn;
    private JButton          save_btn;
    private JButton          close_btn;

    public PreferenceFrame()
    {
        super( "Preferences" );
        super.setDefaultCloseOperation( WindowConstants.DO_NOTHING_ON_CLOSE );
        TopWindow.Preference.disposeAll();
        TopWindow.Preference.setWindow( this );

        Container root_panel = this.getContentPane();
        root_panel.setLayout( new BoxLayout( root_panel, BoxLayout.Y_AXIS ) );

    // Since the ActionListener here updates ALL of the Parameters in memory
    // even when only one of fields/commbobox is updated, so we canNOT do
    // addActionListener() in the above creation function as the listener will
    // be invoked before each of member fields/comboboxes is created. 
    // Instead addAllActionListeners() has to be called after
    // updateAllParametersFromFields() in PreferenceFrame.
        JScrollPane  scroller;
            pptys_panel = new PreferencePanel();
            pptys_panel.updateAllFieldsFromParameters();
            pptys_panel.addSelfActionListeners();
            scroller   = new JScrollPane( pptys_panel );
            Dimension screen_size = Routines.getScreenSize();
            scroller.setMinimumSize(
                     new Dimension( 100, 100 ) );
            scroller.setMaximumSize(
                     new Dimension( screen_size.width / 2,
                                    screen_size.height * 4/5 ) );
            scroller.setPreferredSize(
                     new Dimension( pptys_panel.getPreferredSize().width * 10/9,
                                    screen_size.height * 3/5 ) );
        root_panel.add( scroller );

        URL    icon_URL;
        /*
        JPanel mid_panel = new JPanel();
        mid_panel.setLayout( new BoxLayout( mid_panel, BoxLayout.X_AXIS ) );
            mid_panel.add( Box.createHorizontalGlue() );

            update_btn = new JButton( "update" );
            update_btn.setToolTipText(
            "Update all parameters based on the current preference" );
            // update_btn.setAlignmentX( Component.CENTER_ALIGNMENT );
            update_btn.addActionListener( this );
            mid_panel.add( update_btn );

            mid_panel.add( Box.createHorizontalGlue() );

            save_btn = new JButton( "save" );
            save_btn.setToolTipText(
            "Save preference to Jumpshot-4 setup file" );
            // save_btn.setAlignmentX( Component.CENTER_ALIGNMENT );
            save_btn.addActionListener( this );
            mid_panel.add( save_btn );

            mid_panel.add( Box.createHorizontalGlue() );
        root_panel.add( mid_panel );
        */

        JPanel end_panel = new JPanel();
        end_panel.setLayout( new BoxLayout( end_panel, BoxLayout.X_AXIS ) );
            end_panel.add( Box.createHorizontalGlue() );

            icon_URL = getURL( Const.IMG_PATH + "SaveAll24.gif" );
            if ( icon_URL != null )
                save_btn = new JButton( new ImageIcon( icon_URL ) );
            else
                save_btn = new JButton( "Save" );
            save_btn.setMargin( Const.SQ_BTN2_INSETS );
            save_btn.setToolTipText(
            "Save preference to Jumpshot-4 setup file" );
            save_btn.setAlignmentX( Component.LEFT_ALIGNMENT );
            // save_btn.setAlignmentY( Component.CENTER_ALIGNMENT );
            save_btn.addActionListener( this );
            end_panel.add( save_btn );

            end_panel.add( Box.createHorizontalGlue() );

            icon_URL = getURL( Const.IMG_PATH + "Stop24.gif" );
            if ( icon_URL != null )
                close_btn = new JButton( new ImageIcon( icon_URL ) );
            else
                close_btn = new JButton( "Close" );
            close_btn.setMargin( Const.SQ_BTN2_INSETS );
            close_btn.setToolTipText( "Close this window" );
            close_btn.setAlignmentX( Component.RIGHT_ALIGNMENT );
            // close_btn.setAlignmentY( Component.CENTER_ALIGNMENT );
            close_btn.addActionListener( this );
            end_panel.add( close_btn );

            end_panel.add( Box.createHorizontalGlue() );
        root_panel.add( end_panel );

        addWindowListener( new WindowAdapter() {
            public void windowClosing( WindowEvent e ) {
                PreferenceFrame.this.setVisible( false );
            }
        } );
    }

    public void setVisible( boolean val )
    {
        super.setVisible( val );
        TopWindow.Control.setEditPreferenceButtonEnabled( !val );
    }

    public void updateAllParametersFromFields()
    {
        pptys_panel.updateAllParametersFromFields();
    }

    public void updateAllFieldsFromParameters()
    {
        pptys_panel.updateAllFieldsFromParameters();
    }

    protected URL getURL( String filename )
    {
        URL url = null;
        url = getClass().getResource( filename );
        return url;
    }


    public void actionPerformed( ActionEvent evt )
    {
        /*
        if ( evt.getSource() == this.update_btn ) {
            pptys_panel.updateAllParametersFromFields();
            Parameters.initStaticClasses();
        }
        else
        */
        if ( evt.getSource() == this.save_btn ) {
            pptys_panel.updateAllParametersFromFields();
            Parameters.initStaticClasses();
            Parameters.writeToSetupFile( this );
        }
        else if ( evt.getSource() == this.close_btn ) {
            PreferenceFrame.this.setVisible( false );
        }
    }
}
