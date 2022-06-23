# E Tags
Because the MPICH source has multiple devices and channels, there may be
several implementations of a particular function. This makes it tricky
to get etags to find the right function definitions for you. I've
written a script to run etags on the MPICH source tree and prune
unwanted channels. The script is run like this:

```
make-etags [channel|all]
```

Nemesis is the default channel. Specifying "all" will run etags on all
channels.

Hint: Channel functions are called by CH3 using a macro CH3U_CALL(),
e.g.,:

```
MPIU_CALL(MPIDI_CH3,iStartMsg(vc, eager_pkt, sizeof(*eager_pkt), &sreq));
```

So if you're using emacs and doing a tags-search to search for the call
sites of a MPIDI_CH3 function, you'll want to replace the second "_"
with a "." to match either a "_" or ",". e.g., `MPIDI_CH3.iStartMsgv`.

## Make E Tags:

```
#!/bin/bash
CHANNEL=nemesis
[ $# == 0 ] || CHANNEL=$1

label=channel
[ "$CHANNEL" = "dcmf" ] && label=device

[ "$CHANNEL" = "all" ] && label=source

echo Making etags for $CHANNEL $label...

rm -f TAGS
{
if [ "$CHANNEL" = "all" ] ; then
    find src \(                 \
                -name \*.\[chi\] -o     \
                -name \*.h.in           \
         \) -a              \
             ! -name state_names.h -a       \
             ! -name mpiallstates.h -a      \
             ! -name defmsg.h -a        \
             ! -name windefmsg.h -a     \
             ! -name tokens.\[ch\] -a       \
             ! -name parser.\[ch\] -a       \
             ! -name \*.#\* -a              \
             -type f                \
             -print
else
    find src \(                 \
             \( -path src/mpid -o       \
                    -path src/pm/smpd -o    \
                    -path src/pmi -o        \
                    -path src/mpi/romio -o  \
                -path src/binding -o        \
                -path src/mpe2 -o           \
                    -path src/util/multichannel \
             \) -prune -o           \
                -name \*.\[chi\] -o     \
                -name \*.h.in           \
         \) -a              \
             ! -name state_names.h -a       \
             ! -name mpiallstates.h -a      \
             ! -name defmsg.h -a        \
             ! -name windefmsg.h -a     \
             ! -name tokens.\[ch\] -a       \
             ! -name parser.\[ch\] -a       \
             ! -name \*.#\* -a              \
             -type f                \
             -print

    if [ "$CHANNEL" != "dcmf" ] ; then
        find src/pmi/pmi2                         \
             src/pmi/simple                       \
         src/mpid/ch3/channels/${CHANNEL}                 \
         src/mpid/common                          \
         src/mpid/ch3                         \
         \( -path src/mpid/common/sock/iocp -o            \
            -path src/mpid/ch3/channels -o                \
            -path src/mpid/ch3/channels/nemesis/nemesis/netmod/wintcp \
         \) -prune -o                         \
         \( -name \*.\[chi\] -o -name \*.h.in \) -a           \
             ! -name \*.#\*                       \
             ! -name tokens.\[ch\] -a                     \
             ! -name parser.\[ch\] -a                     \
         -print
    else
        find src/mpid/dcmfd             \
             src/mpid/common                \
         \( -name \*.\[chi\] -o -name \*.h.in \) -a \
             ! -name \*.#\*             \
         -print
    fi
fi
} | etags -

find src -name errnames.txt -print | etags -a --regex="/[*][*][^ :]*/" -
```
