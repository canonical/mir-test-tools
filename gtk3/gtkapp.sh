#!/bin/bash

exec desktop-launch gnome-sudoku&
pid=$!
trap 'kill -s SIGTERM $pid && exit 0' SIGTERM
wait $pid
exit -1
