#!/bin/bash

# For X11 we need .X11-unix/ in the snap's /tmp directory
mkdir -p /tmp/.X11-unix

exec "$@"
