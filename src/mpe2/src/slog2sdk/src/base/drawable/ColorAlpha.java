/*
 *  (C) 2001 by Argonne National Laboratory
 *      See COPYRIGHT in top-level directory.
 */

/*
 *  @author  Anthony Chan
 */

package base.drawable;

import java.awt.Color;
import java.io.DataInput;
import java.io.DataOutput;
// import java.io.DataInputStream;
// import java.io.DataOutputStream;
// import java.io.FileInputStream;
// import java.io.FileOutputStream;
import java.util.Arrays;

import base.io.DataIO;

public class ColorAlpha extends Color
                        implements Comparable, DataIO
{
    private static final long       serialVersionUID = 100L;

    public static final int         BYTESIZE         = 5;

    public static final int         OPAQUE           = 255;
    public static final int         NEAR_OPAQUE      = 191;
    public static final int         HALF_OPAQUE      = 127;
    public static final int         NEAR_TRANSPARENT = 63;
    public static final int         TRANSPARENT      = 0;

    public static final ColorAlpha  WHITE_NEAR_OPAQUE
                                    = new ColorAlpha( ColorAlpha.white,
                                                      ColorAlpha.NEAR_OPAQUE );
    public static final ColorAlpha  WHITE_OPAQUE
                                    = new ColorAlpha( ColorAlpha.white,
                                                      ColorAlpha.OPAQUE );
    public static final ColorAlpha  YELLOW_OPAQUE
                                    = new ColorAlpha( ColorAlpha.yellow,
                                                      ColorAlpha.OPAQUE );
    public static final ColorAlpha  ORANGE_OPAQUE
                                    = new ColorAlpha( ColorAlpha.orange,
                                                      ColorAlpha.OPAQUE );
    public static final ColorAlpha  GRAY_OPAQUE
                                    = new ColorAlpha( ColorAlpha.gray,
                                                      ColorAlpha.OPAQUE );

    private boolean  isModifiable;

    public ColorAlpha()
    {
        // {red=255, green=192, blue= 203} is "pink" from jumpshot.colors
        // Initialize all color to "pink", and "opaque"
        super( 255, 192, 203, ColorAlpha.OPAQUE );
        isModifiable  = true;
    }

    public ColorAlpha( int red, int green, int blue )
    {
        super( red, green, blue, ColorAlpha.OPAQUE );
        isModifiable  = true;
    }

    public ColorAlpha( Color color, int alpha )
    {
        super( color.getRed(), color.getGreen(), color.getBlue(), alpha );
        isModifiable  = true;
    }

    // Used by logformat.slog2.update.UpdatedInputLog
    public ColorAlpha( Color color )
    {
        super( color.getRed(), color.getGreen(), color.getBlue(),
               color.getAlpha() );
        isModifiable  = true;
    }

    public ColorAlpha( int red, int green, int blue, int alpha,
                       boolean in_isModifiable )
    {
        super( red, green, blue, alpha );
        isModifiable  = in_isModifiable;
    }

    public void writeObject( DataOutput outs )
    throws java.io.IOException
    {
        outs.writeInt( super.getRGB() );
        outs.writeBoolean( isModifiable );
    }

    public ColorAlpha( DataInput ins )
    throws java.io.IOException
    {
        super( ins.readInt(), true );
        isModifiable  = ins.readBoolean();
    }
 
    // Since java.awt.Color cannot be altered after creation.
    // readObject( DataInput ) cannot be done/used.
    public void readObject( DataInput ins )
    throws java.io.IOException
    {
        System.err.println( "ColorAlpha.readObject() should NOT called" );
    }

    public String toString()
    {
        return "(" + getRed() + "," + getGreen() +  "," + getBlue() 
             + "," + getAlpha() + "," + isModifiable + ")";
    }

    public int getLengthSq()
    {
        return super.getRed() * super.getRed()
             + super.getGreen() * super.getGreen()
             + super.getBlue() * super.getBlue();
    }

    /*
    http://www.had2know.com/technology/color-contrast-calculator-web-design.html
    */
    public int getBrightness()
    {
        return (int) ( 0.299 * (double)super.getRed()
                     + 0.587 * (double)super.getGreen()
                     + 0.114 * (double)super.getBlue());
    }

    public int getBrightContrast( final ColorAlpha clr )
    {
        return Math.abs(this.getBrightness() - clr.getBrightness());
    }

    public int getHueContrast( final ColorAlpha clr )
    {
        return Math.abs(super.getRed() - clr.getRed())
             + Math.abs(super.getGreen() - clr.getGreen())
             + Math.abs(super.getBlue() - clr.getBlue());
    }

    public boolean equals( final ColorAlpha clr )
    {
        return    ( super.getRed()   == clr.getRed() )
               && ( super.getGreen() == clr.getGreen() )
               && ( super.getBlue()  == clr.getBlue() );
    }

    public int compareTo( Object obj )
    {
        // Since human is more sensitive to brightness, place more emphasis on
        // brightness, i.e. 4 to 1, relative to hue.
        ColorAlpha clr = (ColorAlpha) obj;
        if ( ! this.equals( clr ) )
            // return this.getLengthSq() - clr.getLengthSq();
            // return clr.getLengthSq() - this.getLengthSq();
            return 4 * this.getBrightContrast(clr) + this.getHueContrast(clr);
        else
            return 0;
    }

    public ColorAlpha getHalfOpaque()
    {
       return new ColorAlpha( (Color) this, HALF_OPAQUE );
    }

    public ColorAlpha getDarkerContrast()
    {
        ColorAlpha contrast = new ColorAlpha( this.darker().darker(),
                                              this.getAlpha() );
        if ( contrast.equals( this ) )
            contrast = new ColorAlpha( this.brighter().brighter(),
                                       this.getAlpha() );
        return contrast;
    }

    public ColorAlpha getBrighterContrast()
    {
        ColorAlpha contrast = new ColorAlpha( this.brighter().brighter(),
                                              this.getAlpha() );
        if ( contrast.equals( this ) )
            contrast = new ColorAlpha( this.darker().darker(),
                                       this.getAlpha() );
        return contrast;
    }

    private static ColorAlpha colors[];
    private static int        colors_step;
    private static int        next_color_index;

    /*
       possible RGB values are based on 6x6x6 Color Cube defined in 
       http://world.std.com/~wij/color/index.html
    */
    private static void initDefaultColors()
    {
        int         vals[] = { 0x0, 0x33, 0x66, 0x99, 0xCC, 0xFF };
        int         ired, igreen, iblue;
        int         vals_length, colors_length, idx;
        boolean     is_beg, is_end;

        vals_length    = vals.length;
        colors_length  = vals_length * vals_length * vals_length - 2;
        // colors_step    = vals_length * vals_length * vals_length / 3 - 1;
        colors_step    = vals_length * vals_length * vals_length / 2;
        colors         = new ColorAlpha[ colors_length ];
        idx = 0;
        for ( ired = 0; ired < vals_length; ired++ ) {
            for ( igreen = 0; igreen < vals_length; igreen++ ) {
                for ( iblue = 0; iblue < vals_length; iblue++ ) {
                    // Exclude the begining BLACK and ending WHITE
                    is_beg = (ired == 0 && igreen == 0 && iblue == 0);
                    is_end = (ired == vals_length-1
                           && igreen == ired && iblue == ired);
                    if ( !is_beg && !is_end ) {
                        colors[ idx ] = new ColorAlpha( vals[ ired ],
                                                        vals[ igreen ],
                                                        vals[ iblue ] );
                        idx++;
                    }
                }
            }
        }

        /*
           Sort the colors[] into accending natural order,
           This sorting guarantees the nearest neighbor colors in colors[]
           are always distinguishable, or are contrasting to each other.
        */
        Arrays.sort( colors );
    
        // Initialize the next available color index, to avoid white;
        next_color_index = 10;
    }

    public static ColorAlpha getNextDefaultColor()
    {
        int returning_color_index;

        if ( colors == null )
            ColorAlpha.initDefaultColors();

        returning_color_index = next_color_index %  colors.length;
        next_color_index     += colors_step;
        // next_color_index     += 1;
        return colors[ returning_color_index ];
    }

    

    public static final void main( String[] args )
    {
        ColorAlpha clr, prev;
        clr = ColorAlpha.getNextDefaultColor();
        System.out.println( clr );
        prev = clr;
        for ( int idx = 1; idx < 214; idx++ ) {
            clr = ColorAlpha.getNextDefaultColor();
            int bcontrast = clr.getBrightContrast(prev);
            int hcontrast = clr.getHueContrast(prev);
            boolean is_contrast = (bcontrast > 125) && (hcontrast > 500);
            System.out.println( idx + ": \t" + clr
                              + "\t" + bcontrast + "\t" + hcontrast
                              + "\t" + is_contrast);
            clr = prev;
        }
    }
/*
    public static final void main( String[] args )
    {
        final String filename    = "tmp_color.dat";
 
        String   io_str = args[ 0 ].trim();
        boolean  isWriting = io_str.equals( "write" );

        ColorAlpha colorX = null;

        if ( isWriting ) {
            colorX = new ColorAlpha( 10, -1, 30, 100, false );
            try {
                FileOutputStream fout = new FileOutputStream( filename );
                DataOutputStream dout = new DataOutputStream( fout );
                colorX.writeObject( dout );
                fout.close();
            } catch ( java.io.IOException ioerr ) {
                ioerr.printStackTrace();
                System.exit( 1 );
            }
            System.out.println( "ColorAlpha " + colorX
                              + " has been written to " + filename );
        }
        else {
            try {
                FileInputStream fin   = new FileInputStream( filename );
                DataInputStream din   = new DataInputStream( fin );
                colorX = new ColorAlpha( din );
                fin.close();
            } catch ( java.io.IOException ioerr ) {
                ioerr.printStackTrace();
                System.exit( 1 );
            }
            System.out.println( "ColorAlpha " + colorX
                              + " has been read from " + filename );
        }
    }
*/
}
