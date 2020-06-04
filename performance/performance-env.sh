#!/bin/sh
if [ $(id -u) == 0 ]; then
  export MIR_SERVER_VT=$(snapctl get vt)
  export MIR_SERVER_CONSOLE_PROVIDER=$(snapctl get console-provider)
fi
# Hack to workaround issue #704
if [ $(uname -r) =~ ^.*raspi2$ ]; then
  export MIR_MESA_KMS_DISABLE_MODESET_PROBE=on
fi

# Use libmiral3 as a proxy to test for fixes to Mir's X11 handling (in Mir 2.x)
if [ -d $SNAP/usr/share/doc/libmiral3 ]; then
  # With Mir 1.x:
  #   Xwayland . . : fails to create an X11 socket inside the snap
  #   ClientLatency: uses a console-services stub that conflicts with MIR_SERVER_CONSOLE_PROVIDER
  export MIR_SERVER_ENABLE_MIRCLIENT="on"
  exec "$@ --gtest_filter=-GLMark2Xwayland*:ClientLatency*"
else
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
fi