#!/bin/bash

debuild -us -uc -I.git

RETVAL=$?
if [ $RETVAL -ne 0 ]; then
    echo "-----------------------------"
    echo "Oh no!, $0 failed!"
    echo "-----------------------------"
    if [ $RETVAL -eq 29 ]; then
        echo "You appear to have uncommitted files in your git tree."
        echo "$0 only can run when 'git status' reports a clean source tree."
        echo ""
        echo "If you simply want to build your current source as a temporary debian package"
        echo "run the following commands:"
        echo ""
        echo "    $ fakeroot debian/rules clean binary"
        echo ""
        echo "(You may omit 'clean' to attempt an incremental build)"
    else
        echo "Please note the build error in the log above."
    fi
    exit $RETVAL
fi
