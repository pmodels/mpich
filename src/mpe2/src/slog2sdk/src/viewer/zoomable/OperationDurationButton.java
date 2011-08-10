/*
 *  (C) 2001 by Argonne National Laboratory
 *      See COPYRIGHT in top-level directory.
 */

/*
 *  @author  Anthony Chan
 */

package viewer.zoomable;

import java.awt.Insets;
import java.awt.Component;
import java.awt.Dialog;
import java.awt.Rectangle;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import javax.swing.*;
import javax.swing.border.*;
import java.net.URL;

import base.drawable.TimeBoundingBox;
// import logformat.slog2.LineIDMap;
import viewer.common.Const;
import viewer.common.Parameters;
import viewer.common.SwingWorker;

public class OperationDurationButton extends JButton
{
    private static final long                serialVersionUID = 4100;

    private              TimeBoundingBox     timebox;
    private              SummarizableView    summarizable;
    private              InitializableDialog summary_dialog;
    private              Dialog              root_dialog;

    public OperationDurationButton( final TimeBoundingBox   times,
                                    final SummarizableView  summary )
    {
        super();

        timebox         = times;
        summarizable    = summary;
        summary_dialog  = null;
        root_dialog     = null;

        URL         icon_URL   = getURL( Const.IMG_PATH + "Stat110x40.gif" );
        ImageIcon   icon, icon_shaded;
        Border  raised_border, lowered_border, big_border, huge_border;
        if ( icon_URL != null ) {
            icon        = new ImageIcon( icon_URL );
            icon_shaded = new ImageIcon(
                          GrayFilter.createDisabledImage( icon.getImage() ) );
            super.setIcon( icon );
            super.setPressedIcon( icon_shaded );
            raised_border  = BorderFactory.createRaisedBevelBorder();
            lowered_border = BorderFactory.createLoweredBevelBorder();
            big_border = BorderFactory.createCompoundBorder( raised_border,
                                                             lowered_border );
            huge_border = BorderFactory.createCompoundBorder( raised_border,
                                                              big_border );
            super.setBorder( huge_border );
        }
        else
            super.setText( "Sumary Statistics" );
        super.setMargin( Const.SQ_BTN2_INSETS );
        super.setToolTipText(
        "Summary Statistics for the selected duration, timelines & legends" );
        super.addActionListener( new StatBtnActionListener() );
    }

    private URL getURL( String filename )
    {
        URL url = null;
        url = getClass().getResource( filename );
        return url;
    }

    private void createSummaryDialog()
    {
        SwingWorker           create_statline_worker;

        root_dialog = (Dialog) SwingUtilities.windowForComponent( this );
        create_statline_worker = new SwingWorker() {
            public Object construct()
            {
                summary_dialog = summarizable.createSummary( root_dialog,
                                                             timebox );
                return null;  // returned value is not needed
            }
            public void finished()
            {
                summary_dialog.pack();
                if ( Parameters.AUTO_WINDOWS_LOCATION ) {
                    Rectangle bounds = root_dialog.getBounds();
                    summary_dialog.setLocation( bounds.x + bounds.width/2,
                                                bounds.y + bounds.height/2 );
                }
                summary_dialog.setVisible( true );
                summary_dialog.init();
            }
        };
        create_statline_worker.start();
    }

    private class StatBtnActionListener implements ActionListener
    {
        public void actionPerformed( ActionEvent evt )
        {
            createSummaryDialog();
        }
    }
}
