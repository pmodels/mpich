case $PATH in
        *prefix/bin*)
        true
        ;;
        *)
        PATH=prefix/bin:$PATH
esac


case $MANPATH in
        *prefix/share/man*)
        true
        ;;
        *)
        MANPATH=prefix/share/man:$MANPATH
        export MANPATH
esac


case $LD_LIBRARY_PATH in
        *libdir*)
        true
        ;;
        *)
        if [ -z "$LD_LIBRARY_PATH" ]; then
          LD_LIBRARY_PATH=libdir
        else
          LD_LIBRARY_PATH="libdir:$LD_LIBRARY_PATH"
        fi
        export LD_LIBRARY_PATH
esac
