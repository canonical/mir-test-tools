#!/bin/bash
if [ $(id -u) == 0 ]; then
  export MIR_SERVER_VT=$(snapctl get vt)
  export MIR_SERVER_CONSOLE_PROVIDER=$(snapctl get console-provider)
fi
# Hack to workaround issue #704
if [[ $(uname -r) =~ ^.*raspi2$ ]]; then
  export MIR_MESA_KMS_DISABLE_MODESET_PROBE=on
fi
exec "$@"