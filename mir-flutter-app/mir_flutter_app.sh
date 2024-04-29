#!/bin/bash
${SNAP}/bin/mir_flutter_app&
pid=$!
trap 'kill -s SIGTERM $pid && exit 0' SIGTERM
wait $pid
exit -1
