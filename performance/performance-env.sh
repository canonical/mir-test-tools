#!/bin/bash
MIR_SERVER_VT=$(snapctl get vt)
MIR_SERVER_CONSOLE_PROVIDER=$(snapctl get console-provider)
# Hack to workaround issue #704
if [[ $(uname -r) =~ ^.*raspi2$ ]]; then
  MIR_MESA_KMS_DISABLE_MODESET_PROBE=on
fi
exec "$@"