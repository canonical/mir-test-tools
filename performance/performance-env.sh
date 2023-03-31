#!/bin/bash

if ! snapctl is-connected x11; then
  # For X11 we need .X11-unix/ in the snap's /tmp directory
  mkdir -p /tmp/.X11-unix
  # We also need to avoid any sockets already used by the system
  touch /tmp/.X11-unix/X{0,1,2,3,4,5}
else
  echo "WARNING: some of these tests (GLMark2Xwayland.*) fail with the x11 interface connected"

  if ! echo "$@" | grep --quiet -e "\-\-gtest_filter=" -e "/bin/bash"; then
    echo "ERROR:  Please either disconnect the x11 plug or provide a --gtest_filter:"
    echo "    sudo snap disconnect ${SNAP_INSTANCE_NAME}:x11"
    echo "  Or:"
    echo "    ${SNAP_INSTANCE_NAME}.performance-test --gtest_filter=-GLMark2Xwayland.*"
    exit 1
  fi
fi

exec "$@"