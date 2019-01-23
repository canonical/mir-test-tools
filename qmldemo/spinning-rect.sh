#!/bin/bash
${SNAP}/usr/lib/qt5/bin/qmlscene ${SNAP}/qml/spinning-rect.qml &
pid=$!
result=-1
trap 'kill -s SIGTERM $pid && result=0' SIGTERM
wait $pid
exit $result
