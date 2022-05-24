#!/bin/bash

# For X11 we need .X11-unix/ in the snap's /tmp directory
mkdir -p /tmp/.X11-unix

if ! snapctl is-connected x11 || echo "$@" | grep --quiet -e "\-\-gtest_filter=" -e "/bin/bash"; then
  exec "$@"
else
  exec "$@" --gtest_filter=-GLMark2Xwayland.*
fi