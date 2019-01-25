#!/bin/bash

if [ ! -f "$XDG_CONFIG_HOME/gnome-chess/engines.conf" ]; then
    mkdir -p $XDG_CONFIG_HOME/gnome-chess
    cp $SNAP/etc/gnome-chess/engines.conf $XDG_CONFIG_HOME/gnome-chess
fi

exec desktop-launch gnome-chess&
pid=$!
trap 'kill -s SIGTERM $pid && exit 0' SIGTERM
wait $pid
exit -1
