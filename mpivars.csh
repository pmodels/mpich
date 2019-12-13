set -f path= ( prefix/bin $path )

if ($?MANPATH) then
    if ( "$MANPATH" !~ *prefix/man* ) then
        setenv MANPATH prefix/man:$MANPATH
    endif
else
    setenv MANPATH prefix/man:
endif

if ($?LD_LIBRARY_PATH) then
    setenv LD_LIBRARY_PATH libdir:$LD_LIBRARY_PATH
else
    setenv LD_LIBRARY_PATH libdir
endif
