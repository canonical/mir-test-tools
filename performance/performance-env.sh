#!/bin/bash

# For X11 we need .X11-unix/ in the snap's /tmp directory
mkdir -p /tmp/.X11-unix

# We need to stop Mir conflicting with any existing X11 session.
# Mir detects existing sessions my scanning /tmp/.X11-unix
# but we don't have the real /tmp, but a snap specific one.
# So fake the existing $DISPLAY in the fake /tmp/.X11-unix
if [ -z "${DISPLAY}" ]
then touch /tmp/.X11-unix/X${DISPLAY:1}
fi
# A few more for luck
touch /tmp/.X11-unix/X0
touch /tmp/.X11-unix/X1
touch /tmp/.X11-unix/X2
touch /tmp/.X11-unix/X3
touch /tmp/.X11-unix/X4

exec "$@"
