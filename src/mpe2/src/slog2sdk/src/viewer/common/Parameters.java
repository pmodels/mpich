/*
 *  (C) 2001 by Argonne National Laboratory
 *      See COPYRIGHT in top-level directory.
 */

/*
 *  @author  Anthony Chan
 */

package viewer.common;

// import java.awt.Font;
import java.util.Properties;
// import java.io.InputStream;
// import java.io.OutputStream;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.awt.Component;

import base.topology.Line;
import base.topology.Arrow;
import base.topology.StateBorder;
import base.topology.State;
import base.topology.Event;
import base.topology.PreviewState;
import base.topology.PreviewEvent;
import base.topology.SummaryState;
import base.topology.SummaryArrow;
import base.topology.Pointer;
import base.topology.MarkerEvent;
import base.topology.MarkerLine;
import base.topology.MarkerArrow;
import base.topology.MarkerState;
import base.drawable.Shadow;
import base.drawable.NestingStacks;

public class Parameters
{
    private static final String       VERSION_INFO             = "1.0.4.0";
    private static       String       setupfile_path           = null;

    // Options: Zoomable window reinitialization (requires window restart)
    public  static       String       Y_AXIS_ROOT_LABEL        = "SLOG-2";
    public  static       short        INIT_SLOG2_LEVEL_READ    = 4;
    public  static       boolean      AUTO_WINDOWS_LOCATION    = true;
    public  static       float        SCREEN_HEIGHT_RATIO      = 0.5f;
    public  static       float        TIME_SCROLL_UNIT_RATIO   = 0.01f;

    // Options: All zoomable windows
    public  static       boolean      Y_AXIS_ROOT_VISIBLE      = true;
    public  static       boolean      ACTIVE_REFRESH           = false;
    public  static       Alias        BACKGROUND_COLOR
                                      = Const.COLOR_BLACK;

    public  static       float        STATE_HEIGHT_FACTOR      = 0.90f;
    public  static       float        NESTING_HEIGHT_FACTOR    = 0.80f;
    public  static       Alias        ARROW_ANTIALIASING
                                      = Const.ANTIALIAS_DEFAULT;
    public  static       int          Y_AXIS_MIN_ROW_HEIGHT    = 4;
    public  static       int          MIN_WIDTH_TO_DRAG        = 4;
    public  static       int          CLICK_RADIUS_TO_LINE     = 3;
    public  static       boolean      LEFTCLICK_INSTANT_ZOOM   = true;
    public  static       boolean      POPUP_BOX_MINIMAL_VIEW   = true;
    public  static       String       LARGE_DURATION_FORMAT
                                      = TimeFormat.LARGE_DURATION_AUTO;

    // Options: Timeline zoomable window
    public  static       StateBorder  STATE_BORDER
                                      = StateBorder.COLOR_RAISED_BORDER;
    public  static       int          ARROW_HEAD_LENGTH        = 10;
    public  static       int          ARROW_HEAD_WIDTH         = 6;
    public  static       int          EVENT_BASE_WIDTH         = 8;

    public  static       String       PREVIEW_STATE_DISPLAY
                                      = PreviewState.CUMULATIVE_EXCLUSION;
    public  static       StateBorder  PREVIEW_STATE_BORDER
                                      = StateBorder.COLOR_XOR_BORDER;
    public  static       int          PREVIEW_STATE_BORDER_WIDTH  = 3;
    public  static       int          PREVIEW_STATE_BORDER_HEIGHT = 0;
    public  static       int          PREVIEW_STATE_LEGEND_HEIGHT = 2;
    public  static       int          PREVIEW_ARROW_LOG_BASE      = 5;

    public  static       boolean      HIGHLIGHT_CLICKED_OBJECT  = true;
    public  static       boolean      POINTER_ON_CLICKED_OBJECT = true;
    public  static       int          MARKER_POINTER_MIN_LENGTH = 20;
    public  static       int          MARKER_POINTER_MAX_LENGTH = 30;
    public  static       boolean      MARKER_STATE_STAYS_ON_TOP = false;
    public  static       int          MARKER_STATE_BORDER_WIDTH = 3;
    public  static       int          MARKER_ARROW_BORDER_WIDTH = 4;
    public  static       int          MARKER_LINE_BORDER_WIDTH  = 4;
    public  static       int          MARKER_EVENT_BORDER_WIDTH = 4;

    // Options: Histogram zoomable window
    public  static       boolean      HISTOGRAM_ZERO_ORIGIN    = true;
    public  static       StateBorder  SUMMARY_STATE_BORDER
                                      = StateBorder.COLOR_RAISED_BORDER;
    public  static       int          SUMMARY_ARROW_LOG_BASE   = 5;

    // Options: Legend window
    public  static       boolean      LEGEND_PREVIEW_ORDER     = true;
    public  static       boolean      LEGEND_TOPOLOGY_ORDER    = true;


    public static final void initSetupFile()
    {
        String user_homedir, file_sep;
        user_homedir   = System.getProperty( "user.home" );
        file_sep       = System.getProperty( "file.separator" );
        setupfile_path = user_homedir + file_sep + ".jumpshot4.conf";
        System.out.println( "Jumpshot-4 setup file : " + setupfile_path );
    }

    public static void initStaticClasses()
    {
        // Define the Font used in ModelXXXXPanels and PreferencePanel
        LabeledTextField.setDefaultFont( Const.FONT );
        LabeledComboBox.setDefaultFont( Const.FONT );
        // Define the size of ArrowHead
        Arrow.setHeadProperty( Parameters.ARROW_HEAD_LENGTH,
                               Parameters.ARROW_HEAD_WIDTH );
        // Define the size of EventBase
        Event.setBaseWidth( Parameters.EVENT_BASE_WIDTH ); 
        // Define how a pixel is considered to be lying on a Line/Arrow/Event
        Line.setPixelClosenessTolerance( Parameters.CLICK_RADIUS_TO_LINE );
        Event.setPixelClosenessTolerance( Parameters.CLICK_RADIUS_TO_LINE );
        // Define state border type
        State.setBorderStyle( Parameters.STATE_BORDER );
        PreviewState.setBorderStyle( Parameters.PREVIEW_STATE_BORDER );
        PreviewState.setDisplayType( Parameters.PREVIEW_STATE_DISPLAY );
        PreviewState.setMinCategoryHeight( Parameters.PREVIEW_STATE_LEGEND_HEIGHT );
        // PreviewEvent.setBaseWidth( Parameters.EVENT_BASE_WIDTH );
        PreviewEvent.setPixelClosenessTolerance(
                                      Parameters.CLICK_RADIUS_TO_LINE );

        // Define Shadow State's insets dimension
        Shadow.setStateInsetsDimension(
                             Parameters.PREVIEW_STATE_BORDER_WIDTH,
                             Parameters.PREVIEW_STATE_BORDER_HEIGHT );
        // Define Shadow Arrow's thickness
        Shadow.setBaseOfLogOfObjectNumToArrowWidth(
               Parameters.PREVIEW_ARROW_LOG_BASE );
        // Define all nesting related properties
        NestingStacks.setInitialNestingHeight(
                      Parameters.STATE_HEIGHT_FACTOR );
        NestingStacks.setNestingHeightReduction(
                      Parameters.NESTING_HEIGHT_FACTOR );

        // Set various Markers' properties
        Pointer.setProperty( Parameters.MARKER_POINTER_MIN_LENGTH,
                             Parameters.MARKER_POINTER_MAX_LENGTH );
        MarkerState.setProperty( Parameters.MARKER_STATE_STAYS_ON_TOP,
                                 Parameters.MARKER_STATE_BORDER_WIDTH );
        MarkerArrow.setHeadAndBorderProperty(
                    Parameters.ARROW_HEAD_LENGTH,
                    Parameters.ARROW_HEAD_WIDTH,
                    Parameters.MARKER_ARROW_BORDER_WIDTH );
        MarkerLine.setProperty( Parameters.MARKER_LINE_BORDER_WIDTH );
        MarkerEvent.setBaseAndBorderProperty(
                    Parameters.EVENT_BASE_WIDTH,
                    Parameters.MARKER_EVENT_BORDER_WIDTH );

        // Define Histogram Dialog's drawing properties
        SummaryArrow.setBaseOfLogOfObjectNumToArrowWidth(
                     Parameters.SUMMARY_ARROW_LOG_BASE );
        SummaryState.setBorderStyle( Parameters.SUMMARY_STATE_BORDER );

        TimeFormat.setLargeDurationFormat( Parameters.LARGE_DURATION_FORMAT );
    }

    public static final void writeToSetupFile( Component parent_window )
    {
        if ( ! Dialogs.confirm( parent_window,
                      "Save preferred settings to the setup file ?" ) ) 
            return;

        Properties pptys = new Properties();
        pptys.setProperty( "VERSION_INFO", VERSION_INFO );

        // Options: Zoomable window reinitialization (requires window restart)
        pptys.setProperty( "Y_AXIS_ROOT_LABEL", Y_AXIS_ROOT_LABEL );
        pptys.setProperty( "INIT_SLOG2_LEVEL_READ",
                           String.valueOf( INIT_SLOG2_LEVEL_READ ) );
        pptys.setProperty( "AUTO_WINDOWS_LOCATION",
                           String.valueOf( AUTO_WINDOWS_LOCATION ) );
        pptys.setProperty( "SCREEN_HEIGHT_RATIO",
                           String.valueOf( SCREEN_HEIGHT_RATIO ) );
        pptys.setProperty( "TIME_SCROLL_UNIT_RATIO",
                           String.valueOf( TIME_SCROLL_UNIT_RATIO ) );

        // Options: All zoomable windows
        pptys.setProperty( "Y_AXIS_ROOT_VISIBLE",
                           String.valueOf( Y_AXIS_ROOT_VISIBLE ) );
        pptys.setProperty( "ACTIVE_REFRESH",
                           String.valueOf( ACTIVE_REFRESH ) );
        pptys.setProperty( "BACKGROUND_COLOR",
                           String.valueOf( BACKGROUND_COLOR ) );

        pptys.setProperty( "STATE_HEIGHT_FACTOR",
                           String.valueOf( STATE_HEIGHT_FACTOR ) );
        pptys.setProperty( "NESTING_HEIGHT_FACTOR",
                           String.valueOf( NESTING_HEIGHT_FACTOR ) );
        pptys.setProperty( "ARROW_ANTIALIASING",
                           String.valueOf( ARROW_ANTIALIASING ) );
        pptys.setProperty( "Y_AXIS_MIN_ROW_HEIGHT",
                           String.valueOf( Y_AXIS_MIN_ROW_HEIGHT ) );
        pptys.setProperty( "MIN_WIDTH_TO_DRAG",
                           String.valueOf( MIN_WIDTH_TO_DRAG ) );
        pptys.setProperty( "CLICK_RADIUS_TO_LINE",
                           String.valueOf( CLICK_RADIUS_TO_LINE ) );
        pptys.setProperty( "LEFTCLICK_INSTANT_ZOOM",
                           String.valueOf( LEFTCLICK_INSTANT_ZOOM ) );
        pptys.setProperty( "POPUP_BOX_MINIMAL_VIEW",
                           String.valueOf( POPUP_BOX_MINIMAL_VIEW ) );
        pptys.setProperty( "LARGE_DURATION_FORMAT", LARGE_DURATION_FORMAT );

        // Options: Timeline zoomable window
        pptys.setProperty( "STATE_BORDER",
                           String.valueOf( STATE_BORDER ) );
        pptys.setProperty( "ARROW_HEAD_LENGTH",
                           String.valueOf( ARROW_HEAD_LENGTH ) );
        pptys.setProperty( "ARROW_HEAD_WIDTH",
                           String.valueOf( ARROW_HEAD_WIDTH ) );
        pptys.setProperty( "EVENT_BASE_WIDTH",
                           String.valueOf( EVENT_BASE_WIDTH ) );

        pptys.setProperty( "PREVIEW_STATE_DISPLAY", PREVIEW_STATE_DISPLAY );
        pptys.setProperty( "PREVIEW_STATE_BORDER",
                           String.valueOf( PREVIEW_STATE_BORDER ) );
        pptys.setProperty( "PREVIEW_STATE_BORDER_WIDTH",
                           String.valueOf( PREVIEW_STATE_BORDER_WIDTH ) );
        pptys.setProperty( "PREVIEW_STATE_BORDER_HEIGHT",
                           String.valueOf( PREVIEW_STATE_BORDER_HEIGHT ) );
        pptys.setProperty( "PREVIEW_STATE_LEGEND_HEIGHT",
                           String.valueOf( PREVIEW_STATE_LEGEND_HEIGHT ) );
        pptys.setProperty( "PREVIEW_ARROW_LOG_BASE",
                           String.valueOf( PREVIEW_ARROW_LOG_BASE ) );

        pptys.setProperty( "HIGHLIGHT_CLICKED_OBJECT",
                           String.valueOf( HIGHLIGHT_CLICKED_OBJECT ) );
        pptys.setProperty( "POINTER_ON_CLICKED_OBJECT",
                           String.valueOf( POINTER_ON_CLICKED_OBJECT ) );
        pptys.setProperty( "MARKER_POINTER_MIN_LENGTH",
                           String.valueOf( MARKER_POINTER_MIN_LENGTH ) );
        pptys.setProperty( "MARKER_POINTER_MAX_LENGTH",
                           String.valueOf( MARKER_POINTER_MAX_LENGTH ) );
        pptys.setProperty( "MARKER_STATE_STAYS_ON_TOP",
                           String.valueOf( MARKER_STATE_STAYS_ON_TOP ) );
        pptys.setProperty( "MARKER_STATE_BORDER_WIDTH",
                           String.valueOf( MARKER_STATE_BORDER_WIDTH ) );
        pptys.setProperty( "MARKER_ARROW_BORDER_WIDTH",
                           String.valueOf( MARKER_ARROW_BORDER_WIDTH ) );
        pptys.setProperty( "MARKER_STATE_BORDER_WIDTH",
                           String.valueOf( MARKER_STATE_BORDER_WIDTH ) );
        pptys.setProperty( "MARKER_EVENT_BORDER_WIDTH",
                           String.valueOf( MARKER_EVENT_BORDER_WIDTH ) );

        // Options: Histogram zoomable window
        pptys.setProperty( "HISTOGRAM_ZERO_ORIGIN",
                           String.valueOf( HISTOGRAM_ZERO_ORIGIN ) );
        pptys.setProperty( "SUMMARY_STATE_BORDER",
                           String.valueOf( SUMMARY_STATE_BORDER ) );
        pptys.setProperty( "SUMMARY_ARROW_LOG_BASE",
                           String.valueOf( SUMMARY_ARROW_LOG_BASE ) );

        // Options: Legend window
        pptys.setProperty( "LEGEND_PREVIEW_ORDER",
                           String.valueOf( LEGEND_PREVIEW_ORDER ) );
        pptys.setProperty( "LEGEND_TOPOLOGY_ORDER",
                           String.valueOf( LEGEND_TOPOLOGY_ORDER ) );


        try {
            FileOutputStream fouts = new FileOutputStream( setupfile_path );
            pptys.store( fouts, " Jumpshot-4 setup file" );
            fouts.close();
        } catch ( IOException ioerr ) {
            ioerr.printStackTrace();
            System.exit( 1 );
        }
        System.out.println( "Finalize Parameters: \n"
                          + Parameters.toInOutString() );
    }

    public static final void readFromSetupFile( Component parent_window )
    {
        String   ppty_val;
        boolean  isFileFound;

        isFileFound = false;
        Properties pptys = new Properties();
        try {
            FileInputStream fins = new FileInputStream( setupfile_path );
            pptys.load( fins );
            fins.close();
            isFileFound = true;
        } catch ( FileNotFoundException ioerr ) {
            System.out.println( "Creating Jumpshot-4 setup file ..." );
            Dialogs.info( parent_window,
                     "It seems this is your first time using Jumpshot-4,\n"
                   + "a setup file will be created in your home directory\n"
                   + "with the default settings.", null );
            writeToSetupFile( parent_window );
        } catch ( IOException ioerr ) {
            ioerr.printStackTrace();
            System.exit( 1 );
        }

        ppty_val = pptys.getProperty( "VERSION_INFO" );
        if ( ! VERSION_INFO.equals( ppty_val ) && isFileFound )
            Dialogs.warn( parent_window,
                          "Version mismatch! This Jumpshot-4 is of version "
                        + VERSION_INFO +" not version " + ppty_val + " that "
                        + "is specified in your setup file.\n"
                        + "You may want to SAVE your preferences again in the "
                        + "Preference window to avoid this warning message." );

        // Options: Zoomable window reinitialization (requires window restart)
        ppty_val = pptys.getProperty( "Y_AXIS_ROOT_LABEL" );
        if ( ppty_val != null )
            Y_AXIS_ROOT_LABEL = ppty_val;
        ppty_val = pptys.getProperty( "INIT_SLOG2_LEVEL_READ" );
        if ( ppty_val != null )
            INIT_SLOG2_LEVEL_READ = Short.parseShort( ppty_val );
        ppty_val = pptys.getProperty( "AUTO_WINDOWS_LOCATION" );
        if ( ppty_val != null )
            AUTO_WINDOWS_LOCATION =    ppty_val.equalsIgnoreCase( "true" )
                                    || ppty_val.equalsIgnoreCase( "yes" );
        ppty_val = pptys.getProperty( "SCREEN_HEIGHT_RATIO" );
        if ( ppty_val != null )
            SCREEN_HEIGHT_RATIO = Float.parseFloat( ppty_val );
        ppty_val = pptys.getProperty( "TIME_SCROLL_UNIT_RATIO" );
        if ( ppty_val != null )
            TIME_SCROLL_UNIT_RATIO = Float.parseFloat( ppty_val );


        // Options: All zoomable windows
        ppty_val = pptys.getProperty( "Y_AXIS_ROOT_VISIBLE" );
        if ( ppty_val != null ) 
            Y_AXIS_ROOT_VISIBLE =    ppty_val.equalsIgnoreCase( "true" )
                                  || ppty_val.equalsIgnoreCase( "yes" );
        /*
        ppty_val = pptys.getProperty( "ACTIVE_REFRESH" );
        if ( ppty_val != null )
            ACTIVE_REFRESH =    ppty_val.equalsIgnoreCase( "true" )
                             || ppty_val.equalsIgnoreCase( "yes" );
        */
        ppty_val = pptys.getProperty( "BACKGROUND_COLOR" );
        if ( ppty_val != null )
            BACKGROUND_COLOR = Const.parseBackgroundColor( ppty_val );

        ppty_val = pptys.getProperty( "STATE_HEIGHT_FACTOR" );
        if ( ppty_val != null )
            STATE_HEIGHT_FACTOR = Float.parseFloat( ppty_val );
        ppty_val = pptys.getProperty( "NESTING_HEIGHT_FACTOR" );
        if ( ppty_val != null )
            NESTING_HEIGHT_FACTOR = Float.parseFloat( ppty_val );
        ppty_val = pptys.getProperty( "ARROW_ANTIALIASING" );
        if ( ppty_val != null )
            ARROW_ANTIALIASING = Const.parseAntiAliasing( ppty_val );
        ppty_val = pptys.getProperty( "Y_AXIS_MIN_ROW_HEIGHT" );
        if ( ppty_val != null )
            Y_AXIS_MIN_ROW_HEIGHT = Integer.parseInt( ppty_val );
        ppty_val = pptys.getProperty( "MIN_WIDTH_TO_DRAG" );
        if ( ppty_val != null )
            MIN_WIDTH_TO_DRAG = Integer.parseInt( ppty_val );
        ppty_val = pptys.getProperty( "CLICK_RADIUS_TO_LINE" );
        if ( ppty_val != null )
            CLICK_RADIUS_TO_LINE = Integer.parseInt( ppty_val );
        ppty_val = pptys.getProperty( "LEFTCLICK_INSTANT_ZOOM" );
        if ( ppty_val != null )
            LEFTCLICK_INSTANT_ZOOM =    ppty_val.equalsIgnoreCase( "true" )
                                     || ppty_val.equalsIgnoreCase( "yes" );
        ppty_val = pptys.getProperty( "POPUP_BOX_MINIMAL_VIEW" );
        if ( ppty_val != null )
            POPUP_BOX_MINIMAL_VIEW =    ppty_val.equalsIgnoreCase( "true" )
                                     || ppty_val.equalsIgnoreCase( "yes" );
        ppty_val = pptys.getProperty( "LARGE_DURATION_FORMAT" );
        if ( ppty_val != null )
            LARGE_DURATION_FORMAT = ppty_val;

        // Options: Timeline zoomable window
        ppty_val = pptys.getProperty( "STATE_BORDER" );
        if ( ppty_val != null )
            STATE_BORDER = StateBorder.parseString( ppty_val );
        ppty_val = pptys.getProperty( "ARROW_HEAD_LENGTH" );
        if ( ppty_val != null )
            ARROW_HEAD_LENGTH = Integer.parseInt( ppty_val );
        ppty_val = pptys.getProperty( "ARROW_HEAD_WIDTH" );
        if ( ppty_val != null )
            ARROW_HEAD_WIDTH = Integer.parseInt( ppty_val );
        ppty_val = pptys.getProperty( "EVENT_BASE_WIDTH" );
        if ( ppty_val != null )
            EVENT_BASE_WIDTH = Integer.parseInt( ppty_val );

        ppty_val = pptys.getProperty( "PREVIEW_STATE_DISPLAY" );
        if ( ppty_val != null )
            PREVIEW_STATE_DISPLAY = ppty_val;
        ppty_val = pptys.getProperty( "PREVIEW_STATE_BORDER" );
        if ( ppty_val != null )
            PREVIEW_STATE_BORDER = StateBorder.parseString( ppty_val );
        ppty_val = pptys.getProperty( "PREVIEW_STATE_BORDER_WIDTH" );
        if ( ppty_val != null )
            PREVIEW_STATE_BORDER_WIDTH = Integer.parseInt( ppty_val );
        ppty_val = pptys.getProperty( "PREVIEW_STATE_BORDER_HEIGHT" );
        if ( ppty_val != null )
            PREVIEW_STATE_BORDER_HEIGHT = Integer.parseInt( ppty_val );
        ppty_val = pptys.getProperty( "PREVIEW_STATE_LEGEND_HEIGHT" );
        if ( ppty_val != null )
            PREVIEW_STATE_LEGEND_HEIGHT = Integer.parseInt( ppty_val );
        ppty_val = pptys.getProperty( "PREVIEW_ARROW_LOG_BASE" );
        if ( ppty_val != null )
            PREVIEW_ARROW_LOG_BASE = Integer.parseInt( ppty_val );

        ppty_val = pptys.getProperty( "HIGHLIGHT_CLICKED_OBJECT" );
        if ( ppty_val != null )
            HIGHLIGHT_CLICKED_OBJECT =    ppty_val.equalsIgnoreCase( "true" )
                                       || ppty_val.equalsIgnoreCase( "yes" );
        ppty_val = pptys.getProperty( "POINTER_ON_CLICKED_OBJECT" );
        if ( ppty_val != null )
            POINTER_ON_CLICKED_OBJECT =    ppty_val.equalsIgnoreCase( "true" )
                                        || ppty_val.equalsIgnoreCase( "yes" );
        ppty_val = pptys.getProperty( "MARKER_POINTER_MIN_LENGTH" );
        if ( ppty_val != null )
            MARKER_POINTER_MIN_LENGTH = Integer.parseInt( ppty_val );
        ppty_val = pptys.getProperty( "MARKER_POINTER_MAX_LENGTH" );
        if ( ppty_val != null )
            MARKER_POINTER_MAX_LENGTH = Integer.parseInt( ppty_val );
        ppty_val = pptys.getProperty( "MARKER_STATE_STAYS_ON_TOP" );
        if ( ppty_val != null )
            MARKER_STATE_STAYS_ON_TOP =    ppty_val.equalsIgnoreCase( "true" )
                                        || ppty_val.equalsIgnoreCase( "yes" );
        ppty_val = pptys.getProperty( "MARKER_STATE_BORDER_WIDTH" );
        if ( ppty_val != null )
            MARKER_STATE_BORDER_WIDTH = Integer.parseInt( ppty_val );
        ppty_val = pptys.getProperty( "MARKER_ARROW_BORDER_WIDTH" );
        if ( ppty_val != null )
            MARKER_ARROW_BORDER_WIDTH = Integer.parseInt( ppty_val );
        ppty_val = pptys.getProperty( "MARKER_LINE_BORDER_WIDTH" );
        if ( ppty_val != null )
            MARKER_LINE_BORDER_WIDTH = Integer.parseInt( ppty_val );
        ppty_val = pptys.getProperty( "MARKER_EVENT_BORDER_WIDTH" );
        if ( ppty_val != null )
            MARKER_EVENT_BORDER_WIDTH = Integer.parseInt( ppty_val );

        // Options: Histogram zoomable window
        ppty_val = pptys.getProperty( "HISTOGRAM_ZERO_ORIGIN" );
        if ( ppty_val != null )
            HISTOGRAM_ZERO_ORIGIN =    ppty_val.equalsIgnoreCase( "true" )
                                    || ppty_val.equalsIgnoreCase( "yes" );
        ppty_val = pptys.getProperty( "SUMMARY_STATE_BORDER" );
        if ( ppty_val != null )
            SUMMARY_STATE_BORDER = StateBorder.parseString( ppty_val );
        ppty_val = pptys.getProperty( "SUMMARY_ARROW_LOG_BASE" );
        if ( ppty_val != null )
            SUMMARY_ARROW_LOG_BASE = Integer.parseInt( ppty_val );

        // Options: Legend window
        ppty_val = pptys.getProperty( "LEGEND_PREVIEW_ORDER" );
        if ( ppty_val != null ) 
            LEGEND_PREVIEW_ORDER =    ppty_val.equalsIgnoreCase( "true" )
                                   || ppty_val.equalsIgnoreCase( "yes" );
        ppty_val = pptys.getProperty( "LEGEND_TOPOLOGY_ORDER" );
        if ( ppty_val != null ) 
            LEGEND_TOPOLOGY_ORDER =    ppty_val.equalsIgnoreCase( "true" )
                                    || ppty_val.equalsIgnoreCase( "yes" );

        System.out.println( "Initialize Parameters: \n"
                          + Parameters.toInOutString() );
    }

    public static String toInOutString()
    {
        StringBuffer rep;
        rep = new StringBuffer();
        rep.append( "Y_AXIS_ROOT_LABEL = "     + Y_AXIS_ROOT_LABEL     + "\n" );
        rep.append( "INIT_SLOG2_LEVEL_READ = " + INIT_SLOG2_LEVEL_READ + "\n" );
        rep.append( "AUTO_WINDOWS_LOCATION = " + AUTO_WINDOWS_LOCATION + "\n" );
        rep.append( "SCREEN_HEIGHT_RATIO = "   + SCREEN_HEIGHT_RATIO   + "\n" );
        rep.append( "TIME_SCROLL_UNIT_RATIO = "+ TIME_SCROLL_UNIT_RATIO+ "\n" );

        rep.append( "Y_AXIS_ROOT_VISIBLE = "   + Y_AXIS_ROOT_VISIBLE   + "\n" );
        rep.append( "ACTIVE_REFRESH = "        + ACTIVE_REFRESH        + "\n" );
        rep.append( "BACKGROUND_COLOR = "      + BACKGROUND_COLOR      + "\n" );

        rep.append( "STATE_HEIGHT_FACTOR = "   + STATE_HEIGHT_FACTOR   + "\n" );
        rep.append( "NESTING_HEIGHT_FACTOR = " + NESTING_HEIGHT_FACTOR + "\n" );
        rep.append( "ARROW_ANTIALIASING = "    + ARROW_ANTIALIASING    + "\n" );
        rep.append( "Y_AXIS_MIN_ROW_HEIGHT = " + Y_AXIS_MIN_ROW_HEIGHT + "\n" );
        rep.append( "MIN_WIDTH_TO_DRAG = "     + MIN_WIDTH_TO_DRAG     + "\n" );
        rep.append( "CLICK_RADIUS_TO_LINE = "  + CLICK_RADIUS_TO_LINE  + "\n" );
        rep.append( "LEFTCLICK_INSTANT_ZOOM = "+ LEFTCLICK_INSTANT_ZOOM+ "\n" );
        rep.append( "POPUP_BOX_MINIMAL_VIEW = "+ POPUP_BOX_MINIMAL_VIEW+ "\n" );
        rep.append( "LARGE_DURATION_FORMAT = " + LARGE_DURATION_FORMAT + "\n" );

        rep.append( "STATE_BORDER = "          + STATE_BORDER          + "\n" );
        rep.append( "ARROW_HEAD_LENGTH = "     + ARROW_HEAD_LENGTH     + "\n" );
        rep.append( "ARROW_HEAD_WIDTH = "      + ARROW_HEAD_WIDTH      + "\n" );
        rep.append( "EVENT_BASE_WIDTH = "      + EVENT_BASE_WIDTH      + "\n" );
        //
        rep.append( "PREVIEW_STATE_DISPLAY = " + PREVIEW_STATE_DISPLAY + "\n" );
        rep.append( "PREVIEW_STATE_BORDER = "  + PREVIEW_STATE_BORDER  + "\n" );
        rep.append( "PREVIEW_STATE_BORDER_WIDTH = "
                  + PREVIEW_STATE_BORDER_WIDTH + "\n" );
        rep.append( "PREVIEW_STATE_BORDER_HEIGHT = "
                  + PREVIEW_STATE_BORDER_HEIGHT + "\n" );
        rep.append( "PREVIEW_STATE_LEGEND_HEIGHT = "
                  + PREVIEW_STATE_LEGEND_HEIGHT + "\n" );
        rep.append( "PREVIEW_ARROW_LOG_BASE = "
                  + PREVIEW_ARROW_LOG_BASE + "\n" );
        //
        rep.append( "HIGHLIGHT_CLICKED_OBJECT = "
                  + HIGHLIGHT_CLICKED_OBJECT + "\n" );
        rep.append( "POINTER_ON_CLICKED_OBJECT = "
                  + POINTER_ON_CLICKED_OBJECT + "\n" );
        rep.append( "MARKER_POINTER_MIN_LENGTH = "
                  + MARKER_POINTER_MIN_LENGTH + "\n" );
        rep.append( "MARKER_POINTER_MAX_LENGTH = "
                  + MARKER_POINTER_MAX_LENGTH + "\n" );
        rep.append( "MARKER_STATE_STAYS_ON_TOP = "
                  + MARKER_STATE_STAYS_ON_TOP + "\n" );
        rep.append( "MARKER_STATE_BORDER_WIDTH = "
                  + MARKER_STATE_BORDER_WIDTH + "\n" );
        rep.append( "MARKER_ARROW_BORDER_WIDTH = "
                  + MARKER_ARROW_BORDER_WIDTH + "\n" );
        rep.append( "MARKER_LINE_BORDER_WIDTH = "
                  + MARKER_LINE_BORDER_WIDTH + "\n" );
        rep.append( "MARKER_EVENT_BORDER_WIDTH = "
                  + MARKER_EVENT_BORDER_WIDTH + "\n" );

        rep.append( "HISTOGRAM_ZERO_ORIGIN = " + HISTOGRAM_ZERO_ORIGIN + "\n" );
        rep.append( "SUMMARY_STATE_BORDER = "  + SUMMARY_STATE_BORDER  + "\n" );
        rep.append( "SUMMARY_ARROW_LOG_BASE = "+ SUMMARY_ARROW_LOG_BASE+ "\n" );

        rep.append( "LEGEND_PREVIEW_ORDER = "  + LEGEND_PREVIEW_ORDER  + "\n" );
        rep.append( "LEGEND_TOPOLOGY_ORDER = " + LEGEND_TOPOLOGY_ORDER + "\n" );

        return rep.toString();
    }

    public static void main( String args[] )
    {
        Parameters.initSetupFile();
        if ( args[ 0 ].trim().equals( "write" ) )
            Parameters.writeToSetupFile( null );
        if ( args[ 0 ].trim().equals( "read" ) )
            Parameters.readFromSetupFile( null );
    }
}
