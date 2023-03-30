#!/bin/bash

if ! snapctl is-connected x11; then
  # For X11 we need .X11-unix/ in the snap's /tmp directory
  mkdir -p /tmp/.X11-unix
  # We also need to avoid any sockets already used by the system
  touch /tmp/.X11-unix/X{0,1,2}
fi

if ! snapctl is-connected x11 || echo "$@" | grep --quiet -e "\-\-gtest_filter=" -e "/bin/bash"; then
  exec "$@"
else
  exec "$@" --gtest_filter=-GLMark2Xwayland.*
fi