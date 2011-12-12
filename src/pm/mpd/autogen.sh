#!/bin/sh
# we use an autogen.sh script here to be able to add the "-I ../../../confdb"
# args to the autoreconf line

${AUTORECONF:-autoreconf} ${autoreconf_args:-"-vif"} -I ../../../confdb

