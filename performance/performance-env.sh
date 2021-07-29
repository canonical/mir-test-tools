#!/bin/bash

if ! snapctl is-connected x11 || echo "$@" | grep --quiet "\-\-gtest_filter="; then
  exec "$@"
else
  exec "$@" --gtest_filter=-GLMark2Xwayland.*
fi