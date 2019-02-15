#!/bin/bash
export MIR_SERVER_VT=$(snapctl get vt)
export MIR_SERVER_CONSOLE_PROVIDER=$(snapctl get console-provider)
# Hack to workaround issue #704
if [[ $(uname -r) =~ ^.*raspi2$ ]]; then
  export MIR_MESA_KMS_DISABLE_MODESET_PROBE=on
fi
exec "$@"