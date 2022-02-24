# Configure Xterm Fonts And Colors

## Screenshot

If you want a xtem like

![xterm-color-zenburn.png](xterm-color-zenburn.png
"xterm-color-zenburn.png")

Do following steps:

  - Create or update your `.Xresources` as

```
XTerm*faceName: Bitstream Vera Serif Mono
xterm*faceSize: 11
xterm*vt100*geometry: 80x60
xterm*saveLines: 16384
xterm*loginShell: true
xterm*charClass: 33:48,35:48,37:48,43:48,45-47:48,64:48,95:48,126:48
xterm*termName: xterm-color
xterm*eightBitInput: false

!BLK Cursor
#define _color0        #000d18
#define _color8        #000d18
!RED Tag
#define _color1        #e89393
#define _color9        #e89393
!GRN SpecialKey
#define _color2        #9ece9e
#define _color10       #9ece9e
!YEL Keyword
#define _color3        #f0dfaf
#define _color11       #f0dfaf
!BLU Number
#define _color4        #8cd0d3
#define _color12       #8cd0d3
!MAG Precondit
#define _color5        #c0bed1
#define _color13       #c0bed1
!CYN Float
#define _color6        #dfaf8f
#define _color14       #dfaf8f
!WHT Search
#define _color7        #efefef
#define _color15       #efefef
!FMT Include, StatusLine, ErrorMsg
#define _colorBD       #ffcfaf
#define _colorUL       #ccdc90
#define _colorIT       #80d4aa
!TXT Normal, Normal, Cursor
#define _foreground    #dcdccc
#define _background    #1f1f1f
#define _cursorColor   #8faf9f
URxvt*color0         : _color0
URxvt*color1         : _color1
URxvt*color2         : _color2
URxvt*color3         : _color3
URxvt*color4         : _color4
URxvt*color5         : _color5
URxvt*color6         : _color6
URxvt*color7         : _color7
URxvt*color8         : _color8
URxvt*color9         : _color9
URxvt*color10        : _color10
URxvt*color11        : _color11
URxvt*color12        : _color12
URxvt*color13        : _color13
URxvt*color14        : _color14
URxvt*color15        : _color15
URxvt*colorBD        : _colorBD
URxvt*colorIT        : _colorIT
URxvt*colorUL        : _colorUL
URxvt*foreground     : _foreground
URxvt*background     : _background
URxvt*cursorColor    : _cursorColor
XTerm*color0         : _color0
XTerm*color1         : _color1
XTerm*color2         : _color2
XTerm*color3         : _color3
XTerm*color4         : _color4
XTerm*color5         : _color5
XTerm*color6         : _color6
XTerm*color7         : _color7
XTerm*color8         : _color8
XTerm*color9         : _color9
XTerm*color10        : _color10
XTerm*color11        : _color11
XTerm*color12        : _color12
XTerm*color13        : _color13
XTerm*color14        : _color14
XTerm*color15        : _color15
XTerm*colorBD        : _colorBD
XTerm*colorIT        : _colorIT
XTerm*colorUL        : _colorUL
XTerm*foreground     : _foreground
XTerm*background     : _background
XTerm*cursorColor    : _cursorColor
```

  - After update, do:

```
xrdb -merge ~/.Xresources
```

You are all set.

If you want more details about customization, see the below references for more information.

## References

1. <http://snk.tuxfamily.org/log/zenburn-terminal-color-scheme.html>
2. <http://askubuntu.com/questions/161652/how-to-change-the-default-font-size-of-xterm>
