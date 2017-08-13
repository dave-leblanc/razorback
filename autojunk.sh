#!/bin/sh
# $Id: $
# the list of commands that need to run before we do a compile
IGNORE="nuggets/razorTwit|nuggets/avgNugget|nuggets/avastNugget"
echo Generating top level autojunk
aclocal -I m4
automake --add-missing --copy
autoconf
echo ===========================================================
if [ "x$1" = "x" ]; then
    for I in api dispatcher ui `find nuggets -maxdepth 1 -type d -not -iname nuggets -and -not -iname .svn | egrep -v ${IGNORE}`; do 
        (
        echo Generating autojunk for: $I
        cd $I && ./autojunk.sh
        echo ===========================================================
        )
    done
fi
