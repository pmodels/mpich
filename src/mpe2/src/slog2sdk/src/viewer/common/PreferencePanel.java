/*
 *  (C) 2001 by Argonne National Laboratory
 *      See COPYRIGHT in top-level directory.
 */

/*
 *  @author  Anthony Chan
 */

package viewer.common;

// import java.awt.Dimension;
import java.awt.Component;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import javax.swing.*;

import base.topology.StateBorder;
import base.topology.PreviewState;
// import base.topology.SummaryState;

public class PreferencePanel extends JPanel
                             implements ActionListener
{
    private static final long             serialVersionUID    = 1500L;

    private static int                    VERTICAL_GAP_HEIGHT = 10;

    // Options: Zoomable window reinitialization (requires window restart)
    private        LabeledTextField       fld_Y_AXIS_ROOT_LABEL;
    private        LabeledTextField       fld_INIT_SLOG2_LEVEL_READ;
    private        LabeledComboBox        lst_AUTO_WINDOWS_LOCATION;
    private        LabeledFloatSlider     sdr_SCREEN_HEIGHT_RATIO;
    private        LabeledFloatSlider     sdr_TIME_SCROLL_UNIT_RATIO;

    // Options: All zoomable windows
    private        LabeledComboBox        lst_Y_AXIS_ROOT_VISIBLE;
    private        LabeledComboBox        lst_ACTIVE_REFRESH;
    private        LabeledComboBox        lst_BACKGROUND_COLOR;

    private        LabeledFloatSlider     sdr_STATE_HEIGHT_FACTOR;
    private        LabeledFloatSlider     sdr_NESTING_HEIGHT_FACTOR;
    private        LabeledComboBox        lst_ARROW_ANTIALIASING;
    private        LabeledTextField       fld_Y_AXIS_MIN_ROW_HEIGHT;
    private        LabeledTextField       fld_MIN_WIDTH_TO_DRAG;
    private        LabeledTextField       fld_CLICK_RADIUS_TO_LINE;
    private        LabeledComboBox        lst_LEFTCLICK_INSTANT_ZOOM;
    private        LabeledComboBox        lst_POPUP_BOX_MINIMAL_VIEW;
    private        LabeledComboBox        lst_LARGE_DURATION_FORMAT;

    // Options: Timeline zoomable window
    private        LabeledComboBox        lst_STATE_BORDER;
    private        LabeledTextField       fld_ARROW_HEAD_LENGTH;
    private        LabeledTextField       fld_ARROW_HEAD_WIDTH;
    private        LabeledTextField       fld_EVENT_BASE_WIDTH;

    private        LabeledComboBox        lst_PREVIEW_STATE_DISPLAY;
    private        LabeledComboBox        lst_PREVIEW_STATE_BORDER;
    private        LabeledTextField       fld_PREVIEW_STATE_BORDER_WIDTH;
    private        LabeledTextField       fld_PREVIEW_STATE_BORDER_HEIGHT;
    private        LabeledTextField       fld_PREVIEW_STATE_LEGEND_HEIGHT;
    private        LabeledTextField       fld_PREVIEW_ARROW_LOG_BASE;

    private        LabeledComboBox        lst_HIGHLIGHT_CLICKED_OBJECT;
    private        LabeledComboBox        lst_POINTER_ON_CLICKED_OBJECT;
    private        LabeledComboBox        lst_MARKER_STATE_STAYS_ON_TOP;
    private        LabeledTextField       fld_MARKER_POINTER_MIN_LENGTH;
    private        LabeledTextField       fld_MARKER_POINTER_MAX_LENGTH;
    private        LabeledTextField       fld_MARKER_STATE_BORDER_WIDTH;
    private        LabeledTextField       fld_MARKER_ARROW_BORDER_WIDTH;
    private        LabeledTextField       fld_MARKER_LINE_BORDER_WIDTH;
    private        LabeledTextField       fld_MARKER_EVENT_BORDER_WIDTH;

    // Options: Histogram zoomable window
    private        LabeledComboBox        lst_HISTOGRAM_ZERO_ORIGIN;
    private        LabeledComboBox        lst_SUMMARY_STATE_BORDER;
    private        LabeledTextField       fld_SUMMARY_ARROW_LOG_BASE;

    // Options: Legend window
    private        LabeledComboBox        lst_LEGEND_PREVIEW_ORDER;
    private        LabeledComboBox        lst_LEGEND_TOPOLOGY_ORDER;

    public PreferencePanel()
    {
        super();
        super.setLayout( new BoxLayout( this, BoxLayout.Y_AXIS ) );

        JPanel label_panel;
        JLabel label;

        /*  Options: Zoomable window reinitialization             */

        label_panel = new JPanel();
        label_panel.setLayout( new BoxLayout( label_panel, BoxLayout.X_AXIS ) );
            label = new JLabel( "Zoomable window reinitialization" );
            label.setToolTipText( "Options become effective after "
                                + "the zoomable window is restarted" );
        label_panel.add( Box.createHorizontalStrut( Const.LABEL_INDENTATION ) );
        label_panel.add( label );
        label_panel.add( Box.createHorizontalGlue() );
        label_panel.setAlignmentX( Component.LEFT_ALIGNMENT );
        super.add( label_panel );

        fld_Y_AXIS_ROOT_LABEL = new LabeledTextField( true,
                                    "Y_AXIS_ROOT_LABEL",
                                    Const.STRING_FORMAT );
        fld_Y_AXIS_ROOT_LABEL.setToolTipText(
        "Label for the root node of the Y-axis tree label in the left panel" );
        fld_Y_AXIS_ROOT_LABEL.setHorizontalAlignment( JTextField.CENTER );
        fld_Y_AXIS_ROOT_LABEL.addSelfDocumentListener();
        fld_Y_AXIS_ROOT_LABEL.setEditable( true );
        super.add( fld_Y_AXIS_ROOT_LABEL );

        fld_INIT_SLOG2_LEVEL_READ = new LabeledTextField( true,
                                        "INIT_SLOG2_LEVEL_READ",
                                        Const.SHORT_FORMAT );
        fld_INIT_SLOG2_LEVEL_READ.setToolTipText(
          "The number of SLOG-2 levels being read into memory when "
        + "timeline window is initialized, the number affects the "
        + "zooming and scrolling performance exponentially (in a "
        + "asymptotical sense)." );
        fld_INIT_SLOG2_LEVEL_READ.setHorizontalAlignment( JTextField.CENTER );
        fld_INIT_SLOG2_LEVEL_READ.addSelfDocumentListener();
        fld_INIT_SLOG2_LEVEL_READ.setEditable( true );
        super.add( fld_INIT_SLOG2_LEVEL_READ );

        lst_AUTO_WINDOWS_LOCATION = new LabeledComboBox(
                                        "AUTO_WINDOWS_LOCATION" );
        lst_AUTO_WINDOWS_LOCATION.addItem( Boolean.TRUE );
        lst_AUTO_WINDOWS_LOCATION.addItem( Boolean.FALSE );
        lst_AUTO_WINDOWS_LOCATION.setToolTipText(
        "Whether to let Jumpshot-4 automatically set windows placement." );
        super.add( lst_AUTO_WINDOWS_LOCATION );

        super.add( Box.createVerticalStrut( VERTICAL_GAP_HEIGHT ) );

        sdr_SCREEN_HEIGHT_RATIO = new LabeledFloatSlider(
                                      "SCREEN_HEIGHT_RATIO",
                                      0.0f, 1.0f );
        sdr_SCREEN_HEIGHT_RATIO.setToolTipText(
        "Ratio of the initial timeline canvas height to the screen height.");
        sdr_SCREEN_HEIGHT_RATIO.setHorizontalAlignment( JTextField.CENTER );
        sdr_SCREEN_HEIGHT_RATIO.setEditable( true );
        super.add( sdr_SCREEN_HEIGHT_RATIO );

        sdr_TIME_SCROLL_UNIT_RATIO = new LabeledFloatSlider(
                                         "TIME_SCROLL_UNIT_RATIO",
                                         0.0f, 1.0f );
        sdr_TIME_SCROLL_UNIT_RATIO.setToolTipText(
          "Unit increment of the horizontal scrollbar in the fraction of "
        + "timeline canvas's width." );
        sdr_TIME_SCROLL_UNIT_RATIO.setHorizontalAlignment( JTextField.CENTER );
        sdr_TIME_SCROLL_UNIT_RATIO.setEditable( true );
        super.add( sdr_TIME_SCROLL_UNIT_RATIO );

        super.add( Box.createVerticalStrut( 2 * VERTICAL_GAP_HEIGHT ) );

        /*  Options: All zoomable windows                         */

        label_panel = new JPanel();
        label_panel.setLayout( new BoxLayout( label_panel, BoxLayout.X_AXIS ) );
            label = new JLabel( "All zoomable windows" );
            label.setToolTipText( "Options become effective after return "
                                + "and the zoomable window is redrawn" );
        label_panel.add( Box.createHorizontalStrut( Const.LABEL_INDENTATION ) );
        label_panel.add( label );
        label_panel.add( Box.createHorizontalGlue() );
        label_panel.setAlignmentX( Component.LEFT_ALIGNMENT );
        super.add( label_panel );

        lst_Y_AXIS_ROOT_VISIBLE = new LabeledComboBox( "Y_AXIS_ROOT_VISIBLE" );
        lst_Y_AXIS_ROOT_VISIBLE.addItem( Boolean.TRUE );
        lst_Y_AXIS_ROOT_VISIBLE.addItem( Boolean.FALSE );
        lst_Y_AXIS_ROOT_VISIBLE.setToolTipText(
        "Whether to show the top of the Y-axis tree-styled directory label." );
        super.add( lst_Y_AXIS_ROOT_VISIBLE );

        //  Temporary disable it as this field is not being used
        lst_ACTIVE_REFRESH = new LabeledComboBox( "ACTIVE_REFRESH" );
        lst_ACTIVE_REFRESH.addItem( Boolean.TRUE );
        lst_ACTIVE_REFRESH.addItem( Boolean.FALSE );
        lst_ACTIVE_REFRESH.setToolTipText(
        "Whether to let Jumpshot-4 actively update the timeline canvas." );
        super.add( lst_ACTIVE_REFRESH );
        lst_ACTIVE_REFRESH.setEnabled( false );

        lst_BACKGROUND_COLOR = new LabeledComboBox( "BACKGROUND_COLOR" );
        lst_BACKGROUND_COLOR.addItem( Const.COLOR_BLACK );
        lst_BACKGROUND_COLOR.addItem( Const.COLOR_DARKGRAY );
        lst_BACKGROUND_COLOR.addItem( Const.COLOR_GRAY );
        lst_BACKGROUND_COLOR.addItem( Const.COLOR_LIGHTGRAY );
        lst_BACKGROUND_COLOR.addItem( Const.COLOR_WHITE );
        lst_BACKGROUND_COLOR.setToolTipText(
        "Background color of the timeline canvas" );
        super.add( lst_BACKGROUND_COLOR );

        /*
        fld_Y_AXIS_ROW_HEIGHT = new LabeledTextField( true,
                                    "Y_AXIS_ROW_HEIGHT",
                                    Const.INTEGER_FORMAT );
        fld_Y_AXIS_ROW_HEIGHT.setToolTipText(
        "Row height of Y-axis tree in pixels, i.e. height for each timeline." );
        fld_Y_AXIS_ROW_HEIGHT.setHorizontalAlignment( JTextField.CENTER );;
        fld_Y_AXIS_ROW_HEIGHT.addSelfDocumentListener();
        fld_Y_AXIS_ROW_HEIGHT.setEditable( true );
        super.add( fld_Y_AXIS_ROW_HEIGHT );
        */

        super.add( Box.createVerticalStrut( VERTICAL_GAP_HEIGHT ) );

        sdr_STATE_HEIGHT_FACTOR = new LabeledFloatSlider(
                                      "STATE_HEIGHT_FACTOR",
                                      0.0f, 1.0f );
        sdr_STATE_HEIGHT_FACTOR.setToolTipText(
          "Ratio of the outermost rectangle height to the row height. The "
        + "larger the factor is, the larger the outermost rectangle will "
        + "be with respect to the row height." );
        sdr_STATE_HEIGHT_FACTOR.setHorizontalAlignment( JTextField.CENTER );
        sdr_STATE_HEIGHT_FACTOR.setEditable( true );
        super.add( sdr_STATE_HEIGHT_FACTOR );

        sdr_NESTING_HEIGHT_FACTOR = new LabeledFloatSlider(
                                        "NESTING_HEIGHT_FACTOR",
                                        0.0f, 1.0f );
        sdr_NESTING_HEIGHT_FACTOR.setToolTipText(
          "The gap ratio between successive nesting rectangles. The "
        + "larger the factor is, the smaller the gap will be." );
        sdr_NESTING_HEIGHT_FACTOR.setHorizontalAlignment( JTextField.CENTER );
        sdr_NESTING_HEIGHT_FACTOR.setEditable( true );
        super.add( sdr_NESTING_HEIGHT_FACTOR );

        lst_ARROW_ANTIALIASING = new LabeledComboBox( "ARROW_ANTIALIASING" );
        lst_ARROW_ANTIALIASING.addItem( Const.ANTIALIAS_DEFAULT );
        lst_ARROW_ANTIALIASING.addItem( Const.ANTIALIAS_OFF );
        lst_ARROW_ANTIALIASING.addItem( Const.ANTIALIAS_ON );
        lst_ARROW_ANTIALIASING.setToolTipText(
          "Whether to draw arrow with antialiasing lines. Turning this on "
        + "will slow down the canvas drawing by a factor of ~3" );
        super.add( lst_ARROW_ANTIALIASING );

        fld_Y_AXIS_MIN_ROW_HEIGHT = new LabeledTextField( true,
                                        "Y_AXIS_MIN_ROW_HEIGHT",
                                        Const.INTEGER_FORMAT );
        fld_Y_AXIS_MIN_ROW_HEIGHT.setToolTipText(
          "Minimum Y-axis row height in pixels.  Minimum row height is used "
        + "to prevent timelines from being too short.  When row height < 1, "
        + "various errors occur in Timeline Canvas and Row Adjustments." );
        fld_Y_AXIS_MIN_ROW_HEIGHT.setHorizontalAlignment( JTextField.CENTER );
        fld_Y_AXIS_MIN_ROW_HEIGHT.addSelfDocumentListener();
        fld_Y_AXIS_MIN_ROW_HEIGHT.setEditable( true );
        super.add( fld_Y_AXIS_MIN_ROW_HEIGHT );

        fld_MIN_WIDTH_TO_DRAG = new LabeledTextField( true,
                                    "MIN_WIDTH_TO_DRAG",
                                    Const.INTEGER_FORMAT );
        fld_MIN_WIDTH_TO_DRAG.setToolTipText(
        "Minimum width in pixels to be considered a dragged operation." );
        fld_MIN_WIDTH_TO_DRAG.setHorizontalAlignment( JTextField.CENTER );
        fld_MIN_WIDTH_TO_DRAG.addSelfDocumentListener();
        fld_MIN_WIDTH_TO_DRAG.setEditable( true );
        super.add( fld_MIN_WIDTH_TO_DRAG );

        fld_CLICK_RADIUS_TO_LINE = new LabeledTextField( true,
                                       "CLICK_RADIUS_TO_LINE",
                                       Const.INTEGER_FORMAT );
        fld_CLICK_RADIUS_TO_LINE.setToolTipText(
        "Radius in pixels for a click to be considered on the arrow." );
        fld_CLICK_RADIUS_TO_LINE.setHorizontalAlignment( JTextField.CENTER );
        fld_CLICK_RADIUS_TO_LINE.addSelfDocumentListener();
        fld_CLICK_RADIUS_TO_LINE.setEditable( true );
        super.add( fld_CLICK_RADIUS_TO_LINE );

        lst_LEFTCLICK_INSTANT_ZOOM = new LabeledComboBox(
                                         "LEFTCLICK_INSTANT_ZOOM" );
        lst_LEFTCLICK_INSTANT_ZOOM.addItem( Boolean.TRUE );
        lst_LEFTCLICK_INSTANT_ZOOM.addItem( Boolean.FALSE );
        lst_LEFTCLICK_INSTANT_ZOOM.setToolTipText(
        "Whether to zoom in immediately after left mouse click on canvas." );
        super.add( lst_LEFTCLICK_INSTANT_ZOOM );

        lst_POPUP_BOX_MINIMAL_VIEW = new LabeledComboBox(
                                         "POPUP_BOX_MINIMAL_VIEW" );
        lst_POPUP_BOX_MINIMAL_VIEW.addItem( Boolean.TRUE );
        lst_POPUP_BOX_MINIMAL_VIEW.addItem( Boolean.FALSE );
        lst_POPUP_BOX_MINIMAL_VIEW.setToolTipText(
        "Whether to have minimal view of the initial popup dialog box." );
        super.add( lst_POPUP_BOX_MINIMAL_VIEW );

        lst_LARGE_DURATION_FORMAT = new LabeledComboBox(
                                        "LARGE_DURATION_FORMAT" );
        lst_LARGE_DURATION_FORMAT.addItem( TimeFormat.LARGE_DURATION_AUTO);
        lst_LARGE_DURATION_FORMAT.addItem( TimeFormat.LARGE_DURATION_HHMMSS );
        lst_LARGE_DURATION_FORMAT.addItem( TimeFormat.LARGE_DURATION_SEC );
        lst_LARGE_DURATION_FORMAT.setToolTipText(
        "Display formats for the large duration(> 1 sec) in popup info box." );
        super.add( lst_LARGE_DURATION_FORMAT );

        super.add( Box.createVerticalStrut( 2 * VERTICAL_GAP_HEIGHT ) );

        /*  Options: Timeline zoomable window                     */

        label_panel = new JPanel();
        label_panel.setLayout( new BoxLayout( label_panel, BoxLayout.X_AXIS ) );
            label = new JLabel( "Timeline zoomable window" );
            label.setToolTipText( "Options become effective after return "
                                + "and the Timeline window is redrawn" );
        label_panel.add( Box.createHorizontalStrut( Const.LABEL_INDENTATION ) );
        label_panel.add( label );
        label_panel.add( Box.createHorizontalGlue() );
        label_panel.setAlignmentX( Component.LEFT_ALIGNMENT );
        super.add( label_panel );

        lst_STATE_BORDER = new LabeledComboBox( "STATE_BORDER" );
        lst_STATE_BORDER.addItem( StateBorder.COLOR_RAISED_BORDER );
        lst_STATE_BORDER.addItem( StateBorder.COLOR_LOWERED_BORDER );
        lst_STATE_BORDER.addItem( StateBorder.WHITE_RAISED_BORDER );
        lst_STATE_BORDER.addItem( StateBorder.WHITE_LOWERED_BORDER );
        lst_STATE_BORDER.addItem( StateBorder.WHITE_PLAIN_BORDER );
        lst_STATE_BORDER.addItem( StateBorder.EMPTY_BORDER );
        lst_STATE_BORDER.setToolTipText( "Border style of real states" );
        super.add( lst_STATE_BORDER );

        fld_ARROW_HEAD_LENGTH = new LabeledTextField( true,
                                    "ARROW_HEAD_LENGTH",
                                    Const.INTEGER_FORMAT );
        fld_ARROW_HEAD_LENGTH.setToolTipText(
        "Length of the arrow head in pixels." );
        fld_ARROW_HEAD_LENGTH.setHorizontalAlignment( JTextField.CENTER );
        fld_ARROW_HEAD_LENGTH.addSelfDocumentListener();
        fld_ARROW_HEAD_LENGTH.setEditable( true );
        super.add( fld_ARROW_HEAD_LENGTH );

        fld_ARROW_HEAD_WIDTH = new LabeledTextField( true,
                                   "ARROW_HEAD_WIDTH",
                                   Const.INTEGER_FORMAT );
        fld_ARROW_HEAD_WIDTH.setToolTipText(
        "Width of the arrow head's base in pixels(Even number)." );
        fld_ARROW_HEAD_WIDTH.setHorizontalAlignment( JTextField.CENTER );
        fld_ARROW_HEAD_WIDTH.addSelfDocumentListener();
        fld_ARROW_HEAD_WIDTH.setEditable( true );
        super.add( fld_ARROW_HEAD_WIDTH );

        fld_EVENT_BASE_WIDTH = new LabeledTextField( true,
                                   "EVENT_BASE_WIDTH",
                                   Const.INTEGER_FORMAT );
        fld_EVENT_BASE_WIDTH.setToolTipText(
        "Width of the event triangle's base in pixels(Even number)." );
        fld_EVENT_BASE_WIDTH.setHorizontalAlignment( JTextField.CENTER );
        fld_EVENT_BASE_WIDTH.addSelfDocumentListener();
        fld_EVENT_BASE_WIDTH.setEditable( true );
        super.add( fld_EVENT_BASE_WIDTH );

        super.add( Box.createVerticalStrut( VERTICAL_GAP_HEIGHT ) );

        lst_PREVIEW_STATE_DISPLAY = new LabeledComboBox(
                                        "PREVIEW_STATE_DISPLAY" );
        lst_PREVIEW_STATE_DISPLAY.addItem( PreviewState.FIT_MOST_LEGENDS );
        lst_PREVIEW_STATE_DISPLAY.addItem( PreviewState.OVERLAP_INCLUSION );
        lst_PREVIEW_STATE_DISPLAY.addItem( PreviewState.CUMULATIVE_INCLUSION );
        lst_PREVIEW_STATE_DISPLAY.addItem( PreviewState.OVERLAP_EXCLUSION );
        lst_PREVIEW_STATE_DISPLAY.addItem( PreviewState.CUMULATIVE_EXCLUSION );
        lst_PREVIEW_STATE_DISPLAY.addItem(
                                  PreviewState.CUMULATIVE_EXCLUSION_BASE );
        lst_PREVIEW_STATE_DISPLAY.setToolTipText(
        "Display options for the Preview state." );
        super.add( lst_PREVIEW_STATE_DISPLAY );

        lst_PREVIEW_STATE_BORDER = new LabeledComboBox(
                                       "PREVIEW_STATE_BORDER" );
        lst_PREVIEW_STATE_BORDER.addItem( StateBorder.COLOR_XOR_BORDER );
        lst_PREVIEW_STATE_BORDER.addItem( StateBorder.COLOR_RAISED_BORDER );
        lst_PREVIEW_STATE_BORDER.addItem( StateBorder.COLOR_LOWERED_BORDER );
        lst_PREVIEW_STATE_BORDER.addItem( StateBorder.WHITE_RAISED_BORDER );
        lst_PREVIEW_STATE_BORDER.addItem( StateBorder.WHITE_LOWERED_BORDER );
        lst_PREVIEW_STATE_BORDER.addItem( StateBorder.WHITE_PLAIN_BORDER );
        lst_PREVIEW_STATE_BORDER.addItem( StateBorder.EMPTY_BORDER );
        lst_PREVIEW_STATE_BORDER.setToolTipText(
        "Border style of Preview state." );
        super.add( lst_PREVIEW_STATE_BORDER );

        fld_PREVIEW_STATE_BORDER_WIDTH = new LabeledTextField( true,
                                         "PREVIEW_STATE_BORDER_WIDTH",
                                         Const.INTEGER_FORMAT );
        fld_PREVIEW_STATE_BORDER_WIDTH.setToolTipText(
        "The empty border insets' width in pixels for the Preview state." );
        fld_PREVIEW_STATE_BORDER_WIDTH.setHorizontalAlignment(
                                       JTextField.CENTER );
        fld_PREVIEW_STATE_BORDER_WIDTH.addSelfDocumentListener();
        fld_PREVIEW_STATE_BORDER_WIDTH.setEditable( true );
        super.add( fld_PREVIEW_STATE_BORDER_WIDTH );

        fld_PREVIEW_STATE_BORDER_HEIGHT = new LabeledTextField( true,
                                         "PREVIEW_STATE_BORDER_HEIGHT",
                                         Const.INTEGER_FORMAT );
        fld_PREVIEW_STATE_BORDER_HEIGHT.setToolTipText(
        "The empty border insets' height in pixels for the Preview state." );
        fld_PREVIEW_STATE_BORDER_HEIGHT.setHorizontalAlignment(
                                        JTextField.CENTER );
        fld_PREVIEW_STATE_BORDER_HEIGHT.addSelfDocumentListener();
        fld_PREVIEW_STATE_BORDER_HEIGHT.setEditable( true );
        super.add( fld_PREVIEW_STATE_BORDER_HEIGHT );

        fld_PREVIEW_STATE_LEGEND_HEIGHT = new LabeledTextField( true,
                                        "PREVIEW_STATE_LEGEND_HEIGHT",
                                        Const.INTEGER_FORMAT );
        fld_PREVIEW_STATE_LEGEND_HEIGHT.setToolTipText(
        "Minimum height of the legend divison in pixels for the Preview state" );
        fld_PREVIEW_STATE_LEGEND_HEIGHT.setHorizontalAlignment(
                                        JTextField.CENTER );
        fld_PREVIEW_STATE_LEGEND_HEIGHT.addSelfDocumentListener();
        fld_PREVIEW_STATE_LEGEND_HEIGHT.setEditable( true );
        super.add( fld_PREVIEW_STATE_LEGEND_HEIGHT );

        fld_PREVIEW_ARROW_LOG_BASE = new LabeledTextField( true,
                                         "PREVIEW_ARROW_LOG_BASE",
                                         Const.INTEGER_FORMAT );
        fld_PREVIEW_ARROW_LOG_BASE.setToolTipText(
          "The logarithmic base of the number of arrows in Preview arrow.\n"
        + "This determines the Preview arrow's width." );
        fld_PREVIEW_ARROW_LOG_BASE.setHorizontalAlignment( JTextField.CENTER );
        fld_PREVIEW_ARROW_LOG_BASE.addSelfDocumentListener();
        fld_PREVIEW_ARROW_LOG_BASE.setEditable( true );
        super.add( fld_PREVIEW_ARROW_LOG_BASE );

        super.add( Box.createVerticalStrut( VERTICAL_GAP_HEIGHT ) );

        lst_HIGHLIGHT_CLICKED_OBJECT = new LabeledComboBox(
                                            "HIGHLIGHT_CLICKED_OBJECT" );
        lst_HIGHLIGHT_CLICKED_OBJECT.addItem( Boolean.TRUE );
        lst_HIGHLIGHT_CLICKED_OBJECT.addItem( Boolean.FALSE );
        lst_HIGHLIGHT_CLICKED_OBJECT.setToolTipText(
        "Whether the clicked drawables should be highlighted." );
        super.add( lst_HIGHLIGHT_CLICKED_OBJECT );

        lst_POINTER_ON_CLICKED_OBJECT = new LabeledComboBox(
                                            "POINTER_ON_CLICKED_OBJECT" );
        lst_POINTER_ON_CLICKED_OBJECT.addItem( Boolean.TRUE );
        lst_POINTER_ON_CLICKED_OBJECT.addItem( Boolean.FALSE );
        lst_POINTER_ON_CLICKED_OBJECT.setToolTipText(
        "Whether pointers are used to locate the ends of clicked drawable." );
        super.add( lst_POINTER_ON_CLICKED_OBJECT );

        lst_MARKER_STATE_STAYS_ON_TOP = new LabeledComboBox(
                                            "MARKER_STATE_STAYS_ON_TOP" );
        lst_MARKER_STATE_STAYS_ON_TOP.addItem( Boolean.TRUE );
        lst_MARKER_STATE_STAYS_ON_TOP.addItem( Boolean.FALSE );
        lst_MARKER_STATE_STAYS_ON_TOP.setToolTipText(
          "Whether the clicked or searched STATE is shown on top of the nested "
        + "stack, states that are on top of the clicked state will be hidden" );
        super.add( lst_MARKER_STATE_STAYS_ON_TOP );

        fld_MARKER_POINTER_MIN_LENGTH = new LabeledTextField( true,
                                            "MARKER_POINTER_MIN_LENGTH",
                                            Const.INTEGER_FORMAT );
        fld_MARKER_POINTER_MIN_LENGTH.setToolTipText(
        "Minimum Length of the Pointer marker's arrow in pixels." );
        fld_MARKER_POINTER_MIN_LENGTH.setHorizontalAlignment(
                                      JTextField.CENTER );
        fld_MARKER_POINTER_MIN_LENGTH.addSelfDocumentListener();
        fld_MARKER_POINTER_MIN_LENGTH.setEditable( true );
        super.add( fld_MARKER_POINTER_MIN_LENGTH );

        fld_MARKER_POINTER_MAX_LENGTH = new LabeledTextField( true,
                                            "MARKER_POINTER_MAX_LENGTH",
                                            Const.INTEGER_FORMAT );
        fld_MARKER_POINTER_MAX_LENGTH.setToolTipText(
        "Maximum Length of the Pointer marker's arrow in pixels." );
        fld_MARKER_POINTER_MAX_LENGTH.setHorizontalAlignment(
                                      JTextField.CENTER );
        fld_MARKER_POINTER_MAX_LENGTH.addSelfDocumentListener();
        fld_MARKER_POINTER_MAX_LENGTH.setEditable( true );
        super.add( fld_MARKER_POINTER_MAX_LENGTH );

        fld_MARKER_STATE_BORDER_WIDTH = new LabeledTextField( true,
                                            "MARKER_STATE_BORDER_WIDTH",
                                            Const.INTEGER_FORMAT );
        fld_MARKER_STATE_BORDER_WIDTH.setToolTipText(
        "Border width in pixels of the STATE highlight marker." );
        fld_MARKER_STATE_BORDER_WIDTH.setHorizontalAlignment(
                                      JTextField.CENTER );
        fld_MARKER_STATE_BORDER_WIDTH.addSelfDocumentListener();
        fld_MARKER_STATE_BORDER_WIDTH.setEditable( true );
        super.add( fld_MARKER_STATE_BORDER_WIDTH );

        fld_MARKER_ARROW_BORDER_WIDTH = new LabeledTextField( true,
                                            "MARKER_ARROW_BORDER_WIDTH",
                                            Const.INTEGER_FORMAT );
        fld_MARKER_ARROW_BORDER_WIDTH.setToolTipText(
        "Border width in pixels of the ARROW highlight marker." );
        fld_MARKER_ARROW_BORDER_WIDTH.setHorizontalAlignment(
                                      JTextField.CENTER );
        fld_MARKER_ARROW_BORDER_WIDTH.addSelfDocumentListener();
        fld_MARKER_ARROW_BORDER_WIDTH.setEditable( true );
        super.add( fld_MARKER_ARROW_BORDER_WIDTH );

        fld_MARKER_LINE_BORDER_WIDTH = new LabeledTextField( true,
                                           "MARKER_LINE_BORDER_WIDTH",
                                           Const.INTEGER_FORMAT );
        fld_MARKER_LINE_BORDER_WIDTH.setToolTipText(
        "Border width in pixels of the PREVIEW ARROW highlight marker." );
        fld_MARKER_LINE_BORDER_WIDTH.setHorizontalAlignment(
                                     JTextField.CENTER );
        fld_MARKER_LINE_BORDER_WIDTH.addSelfDocumentListener();
        fld_MARKER_LINE_BORDER_WIDTH.setEditable( true );
        super.add( fld_MARKER_LINE_BORDER_WIDTH );

        fld_MARKER_EVENT_BORDER_WIDTH = new LabeledTextField( true,
                                            "MARKER_EVENT_BORDER_WIDTH",
                                            Const.INTEGER_FORMAT );
        fld_MARKER_EVENT_BORDER_WIDTH.setToolTipText(
        "Border width in pixels of the Event highlight marker." );
        fld_MARKER_EVENT_BORDER_WIDTH.setHorizontalAlignment(
                                      JTextField.CENTER );
        fld_MARKER_EVENT_BORDER_WIDTH.addSelfDocumentListener();
        fld_MARKER_EVENT_BORDER_WIDTH.setEditable( true );
        super.add( fld_MARKER_EVENT_BORDER_WIDTH );

        super.add( Box.createVerticalStrut( 2 * VERTICAL_GAP_HEIGHT ) );

        /*  Options: Histogram zoomable window                    */

        label_panel = new JPanel();
        label_panel.setLayout( new BoxLayout( label_panel, BoxLayout.X_AXIS ) );
            label = new JLabel( "Histogram zoomable window" );
            label.setToolTipText( "Options become effective after return "
                                + "and the Histogram window is redrawn" );
        label_panel.add( Box.createHorizontalStrut( Const.LABEL_INDENTATION ) );
        label_panel.add( label );
        label_panel.add( Box.createHorizontalGlue() );
        label_panel.setAlignmentX( Component.LEFT_ALIGNMENT );
        super.add( label_panel );

        lst_HISTOGRAM_ZERO_ORIGIN = new LabeledComboBox(
                                        "HISTOGRAM_ZERO_ORIGIN" );
        lst_HISTOGRAM_ZERO_ORIGIN.addItem( Boolean.TRUE );
        lst_HISTOGRAM_ZERO_ORIGIN.addItem( Boolean.FALSE );
        lst_HISTOGRAM_ZERO_ORIGIN.setToolTipText(
        "Whether to the time ruler is in duration, i.e. starts with 0.0." );
        super.add( lst_HISTOGRAM_ZERO_ORIGIN );

        lst_SUMMARY_STATE_BORDER = new LabeledComboBox(
                                       "SUMMARY_STATE_BORDER" );
        lst_SUMMARY_STATE_BORDER.addItem( StateBorder.COLOR_XOR_BORDER );
        lst_SUMMARY_STATE_BORDER.addItem( StateBorder.COLOR_RAISED_BORDER );
        lst_SUMMARY_STATE_BORDER.addItem( StateBorder.COLOR_LOWERED_BORDER );
        lst_SUMMARY_STATE_BORDER.addItem( StateBorder.WHITE_RAISED_BORDER );
        lst_SUMMARY_STATE_BORDER.addItem( StateBorder.WHITE_LOWERED_BORDER );
        lst_SUMMARY_STATE_BORDER.addItem( StateBorder.WHITE_PLAIN_BORDER );
        lst_SUMMARY_STATE_BORDER.addItem( StateBorder.EMPTY_BORDER );
        lst_SUMMARY_STATE_BORDER.setToolTipText(
        "Border style of the Summary state in the histogram window." );
        super.add( lst_SUMMARY_STATE_BORDER );

        fld_SUMMARY_ARROW_LOG_BASE = new LabeledTextField( true,
                                         "SUMMARY_ARROW_LOG_BASE",
                                         Const.INTEGER_FORMAT );
        fld_SUMMARY_ARROW_LOG_BASE.setToolTipText(
          "The logarithmic base of the number of arrows in Summary arrow.\n"
        + "This determines the Summary arrow's width." );
        fld_SUMMARY_ARROW_LOG_BASE.setHorizontalAlignment( JTextField.CENTER );
        fld_SUMMARY_ARROW_LOG_BASE.addSelfDocumentListener();
        fld_SUMMARY_ARROW_LOG_BASE.setEditable( true );
        super.add( fld_SUMMARY_ARROW_LOG_BASE );

        super.add( Box.createVerticalStrut( 2 * VERTICAL_GAP_HEIGHT ) );

        /*  Options: Legend window                                */

        label_panel = new JPanel();
        label_panel.setLayout( new BoxLayout( label_panel, BoxLayout.X_AXIS ) );
            label = new JLabel( "Legend window" );
            label.setToolTipText( "Options become effective after return "
                                + "and the Legend window is redrawn" );
        label_panel.add( Box.createHorizontalStrut( Const.LABEL_INDENTATION ) );
        label_panel.add( label );
        label_panel.add( Box.createHorizontalGlue() );
        label_panel.setAlignmentX( Component.LEFT_ALIGNMENT );
        super.add( label_panel );

        lst_LEGEND_PREVIEW_ORDER = new LabeledComboBox(
                                       "LEGEND_PREVIEW_ORDER" );
        lst_LEGEND_PREVIEW_ORDER.addItem( Boolean.TRUE );
        lst_LEGEND_PREVIEW_ORDER.addItem( Boolean.FALSE );
        lst_LEGEND_PREVIEW_ORDER.setToolTipText(
        "Whether to arrange the legends with a hidden Preview order." );
        super.add( lst_LEGEND_PREVIEW_ORDER );

        lst_LEGEND_TOPOLOGY_ORDER = new LabeledComboBox(
                                       "LEGEND_TOPOLOGY_ORDER" );
        lst_LEGEND_TOPOLOGY_ORDER.addItem( Boolean.TRUE );
        lst_LEGEND_TOPOLOGY_ORDER.addItem( Boolean.FALSE );
        lst_LEGEND_TOPOLOGY_ORDER.setToolTipText(
        "Whether to arrange the legends with a hidden Topology order." );
        super.add( lst_LEGEND_TOPOLOGY_ORDER );

        super.add( Box.createVerticalStrut( VERTICAL_GAP_HEIGHT ) );

        super.setBorder( BorderFactory.createEtchedBorder() );
    }

    public void updateAllFieldsFromParameters()
    {
        // Options: Zoomable window reinitialization (requires window restart)
        fld_Y_AXIS_ROOT_LABEL.setText( Parameters.Y_AXIS_ROOT_LABEL );
        fld_INIT_SLOG2_LEVEL_READ.setShort( Parameters.INIT_SLOG2_LEVEL_READ );
        lst_AUTO_WINDOWS_LOCATION.setSelectedBooleanItem(
                                  Parameters.AUTO_WINDOWS_LOCATION );
        sdr_SCREEN_HEIGHT_RATIO.setFloat( Parameters.SCREEN_HEIGHT_RATIO );
        sdr_TIME_SCROLL_UNIT_RATIO.setFloat(
                                   Parameters.TIME_SCROLL_UNIT_RATIO );

        // Options: All zoomable windows
        lst_Y_AXIS_ROOT_VISIBLE.setSelectedBooleanItem(
                                Parameters.Y_AXIS_ROOT_VISIBLE );
        lst_ACTIVE_REFRESH.setSelectedBooleanItem( Parameters.ACTIVE_REFRESH );
        lst_BACKGROUND_COLOR.setSelectedItem( Parameters.BACKGROUND_COLOR );
        // fld_Y_AXIS_ROW_HEIGHT.setInteger( Parameters.Y_AXIS_ROW_HEIGHT );

        sdr_STATE_HEIGHT_FACTOR.setFloat( Parameters.STATE_HEIGHT_FACTOR );
        sdr_NESTING_HEIGHT_FACTOR.setFloat( Parameters.NESTING_HEIGHT_FACTOR );
        lst_ARROW_ANTIALIASING.setSelectedItem( Parameters.ARROW_ANTIALIASING );
        fld_Y_AXIS_MIN_ROW_HEIGHT.setInteger(
                                  Parameters.Y_AXIS_MIN_ROW_HEIGHT );
        fld_MIN_WIDTH_TO_DRAG.setInteger( Parameters.MIN_WIDTH_TO_DRAG );
        fld_CLICK_RADIUS_TO_LINE.setInteger( Parameters.CLICK_RADIUS_TO_LINE );
        lst_LEFTCLICK_INSTANT_ZOOM.setSelectedBooleanItem(
                                   Parameters.LEFTCLICK_INSTANT_ZOOM );
        lst_POPUP_BOX_MINIMAL_VIEW.setSelectedBooleanItem(
                                   Parameters.POPUP_BOX_MINIMAL_VIEW );
        lst_LARGE_DURATION_FORMAT.setSelectedItem(
                                  Parameters.LARGE_DURATION_FORMAT );

        // Options: Timeline zoomable window
        lst_STATE_BORDER.setSelectedItem( Parameters.STATE_BORDER );
        fld_ARROW_HEAD_LENGTH.setInteger( Parameters.ARROW_HEAD_LENGTH );
        fld_ARROW_HEAD_WIDTH.setInteger( Parameters.ARROW_HEAD_WIDTH );
        fld_EVENT_BASE_WIDTH.setInteger( Parameters.EVENT_BASE_WIDTH );

        lst_PREVIEW_STATE_DISPLAY.setSelectedItem(
                                  Parameters.PREVIEW_STATE_DISPLAY );
        lst_PREVIEW_STATE_BORDER.setSelectedItem(
                                 Parameters.PREVIEW_STATE_BORDER );
        fld_PREVIEW_STATE_BORDER_WIDTH.setInteger(
                                   Parameters.PREVIEW_STATE_BORDER_WIDTH );
        fld_PREVIEW_STATE_BORDER_HEIGHT.setInteger(
                                   Parameters.PREVIEW_STATE_BORDER_HEIGHT );
        fld_PREVIEW_STATE_LEGEND_HEIGHT.setInteger(
                                   Parameters.PREVIEW_STATE_LEGEND_HEIGHT );
        fld_PREVIEW_ARROW_LOG_BASE.setInteger(
                                   Parameters.PREVIEW_ARROW_LOG_BASE );

        lst_HIGHLIGHT_CLICKED_OBJECT.setSelectedBooleanItem(
                                     Parameters.HIGHLIGHT_CLICKED_OBJECT );
        lst_POINTER_ON_CLICKED_OBJECT.setSelectedBooleanItem(
                                      Parameters.POINTER_ON_CLICKED_OBJECT );
        lst_MARKER_STATE_STAYS_ON_TOP.setSelectedBooleanItem(
                                      Parameters.MARKER_STATE_STAYS_ON_TOP );
        fld_MARKER_POINTER_MIN_LENGTH.setInteger(
                                      Parameters.MARKER_POINTER_MIN_LENGTH );
        fld_MARKER_POINTER_MAX_LENGTH.setInteger(
                                      Parameters.MARKER_POINTER_MAX_LENGTH );
        fld_MARKER_STATE_BORDER_WIDTH.setInteger(
                                      Parameters.MARKER_STATE_BORDER_WIDTH );
        fld_MARKER_ARROW_BORDER_WIDTH.setInteger(
                                      Parameters.MARKER_ARROW_BORDER_WIDTH );
        fld_MARKER_LINE_BORDER_WIDTH.setInteger(
                                     Parameters.MARKER_LINE_BORDER_WIDTH );
        fld_MARKER_EVENT_BORDER_WIDTH.setInteger(
                                      Parameters.MARKER_EVENT_BORDER_WIDTH );

        // Options: Histogram zoomable window
        lst_HISTOGRAM_ZERO_ORIGIN.setSelectedBooleanItem(
                                  Parameters.HISTOGRAM_ZERO_ORIGIN );
        lst_SUMMARY_STATE_BORDER.setSelectedItem(
                                 Parameters.SUMMARY_STATE_BORDER );
        fld_SUMMARY_ARROW_LOG_BASE.setInteger(
                                   Parameters.SUMMARY_ARROW_LOG_BASE );

        // Options: Legend window
        lst_LEGEND_PREVIEW_ORDER.setSelectedBooleanItem(
                                 Parameters.LEGEND_PREVIEW_ORDER );
        lst_LEGEND_TOPOLOGY_ORDER.setSelectedBooleanItem(
                                  Parameters.LEGEND_TOPOLOGY_ORDER );
    }

    public void updateAllParametersFromFields()
    {
        // Options: Zoomable window reinitialization (requires window restart)
        Parameters.Y_AXIS_ROOT_LABEL
                  = fld_Y_AXIS_ROOT_LABEL.getText();
        Parameters.INIT_SLOG2_LEVEL_READ
                  = fld_INIT_SLOG2_LEVEL_READ.getShort();
        Parameters.AUTO_WINDOWS_LOCATION
                  = lst_AUTO_WINDOWS_LOCATION.getSelectedBooleanItem();
        Parameters.SCREEN_HEIGHT_RATIO
                  = sdr_SCREEN_HEIGHT_RATIO.getFloat();
        Parameters.TIME_SCROLL_UNIT_RATIO
                  = sdr_TIME_SCROLL_UNIT_RATIO.getFloat();

        // Options: All zoomable windows
        Parameters.Y_AXIS_ROOT_VISIBLE
                  = lst_Y_AXIS_ROOT_VISIBLE.getSelectedBooleanItem();
        Parameters.ACTIVE_REFRESH
                  = lst_ACTIVE_REFRESH.getSelectedBooleanItem();
        Parameters.BACKGROUND_COLOR
                  = (Alias) lst_BACKGROUND_COLOR.getSelectedItem();
        // Parameters.Y_AXIS_ROW_HEIGHT
        //           = fld_Y_AXIS_ROW_HEIGHT.getInteger();

        Parameters.STATE_HEIGHT_FACTOR
                  = sdr_STATE_HEIGHT_FACTOR.getFloat();
        Parameters.NESTING_HEIGHT_FACTOR
                  = sdr_NESTING_HEIGHT_FACTOR.getFloat();
        Parameters.ARROW_ANTIALIASING
                  = (Alias) lst_ARROW_ANTIALIASING.getSelectedItem();
        Parameters.Y_AXIS_MIN_ROW_HEIGHT
                  = fld_Y_AXIS_MIN_ROW_HEIGHT.getInteger();
        Parameters.MIN_WIDTH_TO_DRAG
                  = fld_MIN_WIDTH_TO_DRAG.getInteger();
        Parameters.CLICK_RADIUS_TO_LINE
                  = fld_CLICK_RADIUS_TO_LINE.getInteger();
        Parameters.LEFTCLICK_INSTANT_ZOOM
                  = lst_LEFTCLICK_INSTANT_ZOOM.getSelectedBooleanItem();
        Parameters.POPUP_BOX_MINIMAL_VIEW
                  = lst_POPUP_BOX_MINIMAL_VIEW.getSelectedBooleanItem();
        Parameters.LARGE_DURATION_FORMAT
                  = (String) lst_LARGE_DURATION_FORMAT.getSelectedItem();

        // Options: Timeline zoomable window
        Parameters.STATE_BORDER
                  = (StateBorder) lst_STATE_BORDER.getSelectedItem();
        Parameters.ARROW_HEAD_LENGTH
                  = fld_ARROW_HEAD_LENGTH.getInteger();
        Parameters.ARROW_HEAD_WIDTH
                  = fld_ARROW_HEAD_WIDTH.getInteger();
        Parameters.EVENT_BASE_WIDTH
                  = fld_EVENT_BASE_WIDTH.getInteger();

        Parameters.PREVIEW_STATE_DISPLAY
                  = (String) lst_PREVIEW_STATE_DISPLAY.getSelectedItem();
        Parameters.PREVIEW_STATE_BORDER
                  = (StateBorder) lst_PREVIEW_STATE_BORDER.getSelectedItem();
        Parameters.PREVIEW_STATE_BORDER_WIDTH
                  = fld_PREVIEW_STATE_BORDER_WIDTH.getInteger();
        Parameters.PREVIEW_STATE_BORDER_HEIGHT
                  = fld_PREVIEW_STATE_BORDER_HEIGHT.getInteger();
        Parameters.PREVIEW_STATE_LEGEND_HEIGHT
                  = fld_PREVIEW_STATE_LEGEND_HEIGHT.getInteger();
        Parameters.PREVIEW_ARROW_LOG_BASE
                  = fld_PREVIEW_ARROW_LOG_BASE.getInteger();

        Parameters.HIGHLIGHT_CLICKED_OBJECT
                  = lst_HIGHLIGHT_CLICKED_OBJECT.getSelectedBooleanItem();
        Parameters.POINTER_ON_CLICKED_OBJECT
                  = lst_POINTER_ON_CLICKED_OBJECT.getSelectedBooleanItem();
        Parameters.MARKER_STATE_STAYS_ON_TOP
                  = lst_MARKER_STATE_STAYS_ON_TOP.getSelectedBooleanItem();
        Parameters.MARKER_POINTER_MIN_LENGTH
                  = fld_MARKER_POINTER_MIN_LENGTH.getInteger();
        Parameters.MARKER_POINTER_MAX_LENGTH
                  = fld_MARKER_POINTER_MAX_LENGTH.getInteger();
        Parameters.MARKER_STATE_BORDER_WIDTH
                  = fld_MARKER_STATE_BORDER_WIDTH.getInteger();
        Parameters.MARKER_ARROW_BORDER_WIDTH
                  = fld_MARKER_ARROW_BORDER_WIDTH.getInteger();
        Parameters.MARKER_LINE_BORDER_WIDTH
                  = fld_MARKER_LINE_BORDER_WIDTH.getInteger();
        Parameters.MARKER_EVENT_BORDER_WIDTH
                  = fld_MARKER_EVENT_BORDER_WIDTH.getInteger();

        // Options: Histogram zoomable window
        Parameters.HISTOGRAM_ZERO_ORIGIN
                  = lst_HISTOGRAM_ZERO_ORIGIN.getSelectedBooleanItem();
        Parameters.SUMMARY_STATE_BORDER
                  = (StateBorder) lst_SUMMARY_STATE_BORDER.getSelectedItem();
        Parameters.SUMMARY_ARROW_LOG_BASE
                  = fld_SUMMARY_ARROW_LOG_BASE.getInteger();

        // Options: Legend window
        Parameters.LEGEND_PREVIEW_ORDER
                  = lst_LEGEND_PREVIEW_ORDER.getSelectedBooleanItem();
        Parameters.LEGEND_TOPOLOGY_ORDER
                  = lst_LEGEND_TOPOLOGY_ORDER.getSelectedBooleanItem();
    }

    // Since the ActionListener here updates ALL of the Parameters in memory
    // even when only one of fields/commbobox is updated, so we canNOT do
    // addActionListener() in the above creation function as the listener will
    // be invoked before each of member fields/comboboxes is created. 
    // Instead addAllActionListeners() has to be called after
    // updateAllParametersFromFields() in PreferenceFrame.
    public void addSelfActionListeners()
    {
        // Options: Zoomable window reinitialization (requires window restart)
        fld_Y_AXIS_ROOT_LABEL.addActionListener( this );
        fld_INIT_SLOG2_LEVEL_READ.addActionListener( this );
        lst_AUTO_WINDOWS_LOCATION.addActionListener( this );
        // sdr_SCREEN_HEIGHT_RATIO.addActionListener( this );
        // sdr_TIME_SCROLL_UNIT_RATIO.addActionListener( this );

        // Options: All zoomable windows
        lst_Y_AXIS_ROOT_VISIBLE.addActionListener( this );
        lst_ACTIVE_REFRESH.addActionListener( this );
        lst_BACKGROUND_COLOR.addActionListener( this );

        // sdr_STATE_HEIGHT_FACTOR.addActionListener( this );
        // sdr_NESTING_HEIGHT_FACTOR.addActionListener( this );
        lst_ARROW_ANTIALIASING.addActionListener( this );
        fld_Y_AXIS_MIN_ROW_HEIGHT.addActionListener( this );
        fld_MIN_WIDTH_TO_DRAG.addActionListener( this );
        fld_CLICK_RADIUS_TO_LINE.addActionListener( this );
        lst_LEFTCLICK_INSTANT_ZOOM.addActionListener( this );
        lst_POPUP_BOX_MINIMAL_VIEW.addActionListener( this );
        lst_LARGE_DURATION_FORMAT.addActionListener( this );

        // Options: Timeline zoomable window
        lst_STATE_BORDER.addActionListener( this );
        fld_ARROW_HEAD_LENGTH.addActionListener( this );
        fld_ARROW_HEAD_WIDTH.addActionListener( this );
        fld_EVENT_BASE_WIDTH.addActionListener( this );

        lst_PREVIEW_STATE_DISPLAY.addActionListener( this );
        lst_PREVIEW_STATE_BORDER.addActionListener( this );
        fld_PREVIEW_STATE_BORDER_WIDTH.addActionListener( this );
        fld_PREVIEW_STATE_BORDER_HEIGHT.addActionListener( this );
        fld_PREVIEW_STATE_LEGEND_HEIGHT.addActionListener( this );
        fld_PREVIEW_ARROW_LOG_BASE.addActionListener( this );

        lst_HIGHLIGHT_CLICKED_OBJECT.addActionListener( this );
        lst_POINTER_ON_CLICKED_OBJECT.addActionListener( this );
        lst_MARKER_STATE_STAYS_ON_TOP.addActionListener( this );
        fld_MARKER_POINTER_MIN_LENGTH.addActionListener( this );
        fld_MARKER_POINTER_MAX_LENGTH.addActionListener( this );
        fld_MARKER_STATE_BORDER_WIDTH.addActionListener( this );
        fld_MARKER_ARROW_BORDER_WIDTH.addActionListener( this );
        fld_MARKER_LINE_BORDER_WIDTH.addActionListener( this );
        fld_MARKER_EVENT_BORDER_WIDTH.addActionListener( this );

        // Options: Histogram zoomable window
        lst_HISTOGRAM_ZERO_ORIGIN.addActionListener( this );
        lst_SUMMARY_STATE_BORDER.addActionListener( this );
        fld_SUMMARY_ARROW_LOG_BASE.addActionListener( this );

        // Options: Legend window
        lst_LEGEND_PREVIEW_ORDER.addActionListener( this );
        lst_LEGEND_TOPOLOGY_ORDER.addActionListener( this );
    }

    public void actionPerformed( ActionEvent evt )
    {
        this.updateAllParametersFromFields();
        Parameters.initStaticClasses();
    }
}
